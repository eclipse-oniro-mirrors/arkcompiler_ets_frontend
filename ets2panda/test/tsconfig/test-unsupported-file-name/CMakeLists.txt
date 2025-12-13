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

add_custom_target(es2panda-unit-test-fatal-error-unsupported_file_name1
    COMMENT "Testing test-fatal-error for: unsupported_file_name1"
    DEPENDS es2panda
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMAND python3 test_fatal_script.py
        --es2panda '${PANDA_RUN_PREFIX} $<TARGET_FILE:es2panda>'
        --config arktsconfig.json
        --filepath test.ets
        --stdlib ${PANDA_ROOT}/plugins/ets/stdlib
)

add_custom_target(es2panda-unit-test-fatal-error-unsupported_file_name2
    COMMENT "Testing test-fatal-error for: unsupported_file_name2"
    DEPENDS es2panda
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMAND python3 test_fatal_script.py
        --es2panda '${PANDA_RUN_PREFIX} $<TARGET_FILE:es2panda>'
        --config arktsconfig.json
        --filepath test.ets
        --stdlib ${PANDA_ROOT}/plugins/ets/stdlib
)

add_dependencies(es2panda_tests es2panda-unit-test-fatal-error-unsupported_file_name1)
add_dependencies(es2panda_tests es2panda-unit-test-fatal-error-unsupported_file_name2)
