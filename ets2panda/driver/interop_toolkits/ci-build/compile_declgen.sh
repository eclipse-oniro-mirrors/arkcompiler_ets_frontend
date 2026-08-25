#!/bin/bash
# Copyright (c) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e

readonly TSC_PATH="$1"
readonly WORK_PATH="$2"
shift 2
readonly TARGET_PATHS=("$@")

readonly TMP_TSC_PATH="$WORK_PATH/tsc_tmp"
readonly TSC_PACKAGE_PATH="$TMP_TSC_PATH/ohos-typescript-4.9.5-r4.tgz"

target_files=("lib" "bin" "package.json" "LICENSE" "README.md" "README.OpenSource" "SECURITY.md" "ThirdPartyNoticeText.txt")

# clean and create temporary directory for the tsc package
[ -n "$TMP_TSC_PATH" ] && rm -rf "$TMP_TSC_PATH" && mkdir -p "$TMP_TSC_PATH"
for file in "${target_files[@]}"; do
    if [ -e "$TSC_PATH/$file" ]; then
        cp -rP "$TSC_PATH/$file" "$TMP_TSC_PATH/"
    fi
done

# pack ohos-typescript-4.9.5-r4.tgz in TMP_TSC_PATH
pushd "$TMP_TSC_PATH" > /dev/null
    npm pack
popd > /dev/null

# decompress ohos-typescript-4.9.5-r4.tgz and install it as the
# "typescript" module under every target's node_modules
for target_path in "${TARGET_PATHS[@]}"; do
    node_modules_path="$target_path/node_modules"
    mkdir -p "$node_modules_path"
    tar -xzf "$TSC_PACKAGE_PATH" -C "$node_modules_path"
    rm -rf "$node_modules_path/typescript"
    mv "$node_modules_path/package" "$node_modules_path/typescript"
done
