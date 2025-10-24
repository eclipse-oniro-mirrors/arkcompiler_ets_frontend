#!/usr/bin/env bash

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

# This script will replace the old paths with the new ones in all files within the testcases directory.

set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Check if the 'ets' directory exists in SCRIPT_DIR
if [ ! -d "${SCRIPT_DIR}/ets" ]; then
    echo "Error: 'ets' directory not found in ${SCRIPT_DIR}."
    echo "Please make sure the 'ets' directory exists before running bindings test."
    exit 1
fi

RESTORE_MODE=0
if [ "$1" == "--restore" ]; then
    RESTORE_MODE=1
fi

if [ $RESTORE_MODE -eq 1 ]; then
    rm "${SCRIPT_DIR}/../ets2panda"
    echo 'Remove the symbolic link to ets2panda'
else
    ln -s "${SCRIPT_DIR}/ets/static/build-tools/ets2panda" "${SCRIPT_DIR}/../ets2panda"
    echo 'Create a symbolic link to ets2panda'
fi

exit 0
