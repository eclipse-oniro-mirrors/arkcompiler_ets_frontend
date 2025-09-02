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
    set(PROJECT_TARGET_NAME es2panda-tsconfig-test-decl-${project})
    set(ETS_CONFIG_DECL ${CMAKE_CURRENT_BINARY_DIR}/${project}/arktsconfig_decl.json)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${project}/arktsconfig.in.decl.json ${ETS_CONFIG_DECL})
    add_custom_target(${PROJECT_TARGET_NAME}
        COMMENT "Testing tsconfig for: ${project}"
        DEPENDS es2panda
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND python3 test.py
            --es2panda '${PANDA_RUN_PREFIX} $<TARGET_FILE:es2panda>'
            --arktsconfig ${CMAKE_CURRENT_BINARY_DIR}/${project}/arktsconfig_decl.json 
            --stdlib ${PANDA_ROOT}/plugins/ets/stdlib
            --target ${project}/main.ets
    )
    add_dependencies(es2panda_tests ${PROJECT_TARGET_NAME})
endfunction()
# 26272
# add_test(resolve-dynamic-paths)
# add_test(check-extend-dynamic)
add_test(check-decl-path)
add_test(check-export-decl)
add_test(typecheck-decl)