#!/bin/bash
# Copyright (c) 2025 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -ex
set -o pipefail

function about() {
    cat <<-ENDHELP
    Script to dowload and prepare the ArkUI environment
    where
        --help            Show this help and exit
        --nexus-repo      Nexus repo, like nexus.example.com:1234
        --openlab-token   Token to access to openlab
        (you can ask this information from Titova Tatiana)
    Please use this file to set up your .npmrc
        BZ - https://gitee.com/rri_opensource/koala_projects/wikis/Environment%20Setup/%20npmrc%20(blue%20zone)
        GZ - https://gitee.com/rri_opensource/koala_projects/wikis/Environment%20Setup/npmrc%20(yellow%20and%20green%20zone)
    Use these instructions to manually prepare ArkUI Hello app
        HOST - https://gitee.com/titovatatiana/arkcompiler_ets_frontend/wikis/Guide:%20How%20to%20Download%20and%20Build%20ArkUI%20Project
        DEVICE - https://gitee.com/rri_opensource/koala_projects/wikis/Setup%20Guide/Trivial%20Build%20%2526%20Setup%20Guide
    Use these instructions to install custom compiler
        https://gitee.com/titovatatiana/arkcompiler_ets_frontend/wikis/How%20to%20manage%20the%20integration%20process%20with%20ArkUI%20jobs
ENDHELP
}

while [ -n "$1" ]; do
    case "$1" in
        -h | --help)
            about
            exit 0
            ;;
        --demo)
            DEMO="${2}"
            shift 2
            ;;
        --nexus-repo)
            NEXUS_REPO="${2}"
            shift 2
            ;;
        --openlab-token)
            KOALA_TOKEN="${2}"
            shift 2
            ;;
        *)
            echo "Unknown option" "${1}"
            exit 1
            ;;
    esac
done

if [ -z "${NEXUS_REPO}" ]; then
    echo "Please set NEXUS_REPO"
    echo "export NEXUS_REPO=nexus.example.com:1234"
    exit 1
fi

if [ -z "${NPROC_PER_JOB}" ]; then
    NPROC_PER_JOB=16
fi

HUAWEI_MIRROR="${HUAWEI_MIRROR:-https://repo.huaweicloud.com/repository/npm/}"
KOALA_REGISTRY="${KOALA_REGISTRY:-https://$NEXUS_REPO/repository/koala-npm/}"
export NINJA_OPTIONS="-j ${NPROC_PER_JOB}"

retry() {
  local -r -i max_attempts="$1"; shift
  local -r cmd="$@"
  local -i attempt_num=1
  local -i delay=5

  until $cmd
    do
      if (( attempt_num == max_attempts ))
      then
        echo "Attempt $attempt_num failed and there are no more attempts left!"
         return 1
      else
        echo "Attempt $((attempt_num++)) failed! Trying again in $((delay *= attempt_num)) seconds..."
        sleep ${delay}
      fi
    done
}

function do_checkout() {
    local repo=$1
    local rev=$2
    local dest=$3
    local patch=$4
    [ -n "${repo}" ] || exit 1
    [ -n "${rev}" ] || exit 1
    [ -n "${dest}" ] || exit 1
    mkdir -p "${dest}"
    CMD_TIMEOUT=10m
    pushd "${dest}" || exit 1
        git init && git remote add origin "${repo}"
        retry 5 timeout ${CMD_TIMEOUT} git fetch --depth 1 origin "${rev}" || {
            echo "(Some error occurred while fetching rev: ${rev}"
            exit 1
        } && {
            git checkout FETCH_HEAD || exit 1
            [ -n "${patch}" ] && git apply "${patch}"
        }
        git log -1
    popd >/dev/null 2>&1 || exit 1
}

SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")"
source "${SCRIPT_DIR}"/arkui.properties

ARKUI_DEV_REPO="${ARKUI_DEV_REPO:-https://gitee.com/rri_opensource/koala_projects.git}"
ARKUI_DEV_BRANCH="${ARKUI_DEV_BRANCH:-master}"
ARKUI_DEST="${ARKUI_DEST:-koala-sig}"

do_checkout "${ARKUI_DEV_REPO}" "${ARKUI_DEV_BRANCH}" "${ARKUI_DEST}"

cd "${ARKUI_DEST}" || exit 1

npm config set package-lock false
npm config set strict-ssl false
npm config set registry "${HUAWEI_MIRROR}"
npm config set @koalaui:registry "${KOALA_REGISTRY}"
npm config set @idlizer:registry "${KOALA_REGISTRY}"
npm config set @panda:registry "https://$NEXUS_REPO/repository/koala-npm/"
npm config set @ohos:registry "https://repo.harmonyos.com/npm/"
if [ -z "${KOALA_REPO}" ] ; then
    npm config set "//$NEXUS_REPO/repository/koala-npm/:_auth=$KOALA_TOKEN"
fi

npm install -d

pushd incremental/tools/panda/ || exit 1
if [ -z "${PANDA_SDK_HOST_TARBALL}" ] ; then
    npm run panda:sdk:install
else
    npm install "${PANDA_SDK_HOST_TARBALL}"
    if [ -n "${PANDA_SDK_DEV_TARBALL}" ] ; then
        npm install "${PANDA_SDK_DEV_TARBALL}"
    else
        echo "PANDA_SDK_DEV_TARBALL is not set, skipping!"
    fi
fi
popd >/dev/null 2>&1 || exit 1

function run_script() {
    npm run $1 | tee out.txt
    if [ -n "$(grep 'Error:' out.txt)" ] ; then
        exit 1
    fi
}

export ENABLE_BUILD_CACHE=0

# Compile libarkts
pushd ui2abc/libarkts || exit 1
run_script "regenerate"
run_script "compile --prefix ../fast-arktsc"
run_script "run"
popd >/dev/null 2>&1 || exit 1

# Compile memo-plugin, ui-plugins

# need to fix ui2abc tests
# run_script "all --prefix ui2abc"
run_script "build:all --prefix ui2abc"

run_script "build:deps --prefix ets-tests"

if [ -z "${DEMO}" ] ; then
    echo "Just compiled ArkUI, but no demo specified."
    exit 1
fi

case "${DEMO}" in
    "shopping")
        run_script "run:node --prefix arkoala-arkts/shopping/user"
        ;;
    "trivial")
        run_script "run --prefix arkoala-arkts/trivial/user"
        ;;
    "empty")
        ;;
    *)
        echo "Unknown demo" "${DEMO}"
        exit 1
        ;;
esac

echo "ArkUI ${DEMO} demo completed successfully."
