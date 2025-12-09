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

set -e

readonly TEST_DIR="$1"
readonly NODE_DIR="$2"
readonly SDK_DIR="$3"
readonly CWD="${TEST_DIR}/../"
readonly CURRENT_NPM="${NODE_DIR}/npm"
readonly CURRENT_NODE="${NODE_DIR}/node"

cp -rfp -- "$SDK_DIR" "$TEST_DIR"
cd "$CWD" && "$CURRENT_NPM" run test:build
"$CURRENT_NODE" dist-test/test/run_tests.js ./test
