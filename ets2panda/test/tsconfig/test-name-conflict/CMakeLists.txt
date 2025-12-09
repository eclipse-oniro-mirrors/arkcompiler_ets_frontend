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

function(add_test project)
    set(PROJECT_TARGET_NAME es2panda-tsconfig-${project})
    add_custom_target(${PROJECT_TARGET_NAME}
        COMMENT "Testing name conflict with internal the internal Module decleration"
        DEPENDS es2panda
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND python3 test.py
            --es2panda '${PANDA_RUN_PREFIX} $<TARGET_FILE:es2panda>'
            --config arktsconfig.json
            --stdlib ${PANDA_ROOT}/plugins/ets/stdlib
    )
    add_dependencies(es2panda_tests ${PROJECT_TARGET_NAME})
endfunction()

add_test(module-name-conflict)
