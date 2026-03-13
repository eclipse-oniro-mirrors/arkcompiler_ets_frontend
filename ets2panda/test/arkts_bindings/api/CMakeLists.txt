# Copyright (c) 2026 Huawei Device Co., Ltd.
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

# NOTE(eokolnov): Disable asan check due to existing memory leak issues in Plugin API
if (PANDA_ENABLE_ADDRESS_SANITIZER)
    return()
endif()

file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CURRENT_SOURCE_DIR)
file(REAL_PATH "${CMAKE_CURRENT_BINARY_DIR}" CURRENT_BINARY_DIR)
file(REAL_PATH "${CMAKE_BINARY_DIR}" CMAKE_BINARY_ROOT)

set(TEST_PACKAGE ArktsBindingsTest)
set(TEST_CACHE_DIR "${CURRENT_BINARY_DIR}/decls")
set(ARKTS_BINDINGS_LIB ${CMAKE_BINARY_ROOT}/tools/es2panda/arkts_bindings/arkts_bindings.abc)

set(ETS_CONFIG ${CURRENT_BINARY_DIR}/arktsconfig.json)
configure_file(${CURRENT_SOURCE_DIR}/arktsconfig.in.json ${ETS_CONFIG})

set(TEST_CASE_FILE ${CURRENT_SOURCE_DIR}/testcases/func.ets)

add_custom_target(arkts_bindings_tests_api_clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${TEST_CACHE_DIR}"
)

add_custom_target(arkts_bindings_tests_api)
add_dependencies(arkts_bindings_tests_api arkts_bindings_tests_api_clean)
add_dependencies(arkts_bindings_tests arkts_bindings_tests_api)

set(RUNTIME_ARGS "--native-library-path=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

run_arkts_bindings_test(arkts_bindings_tests_api-plugin
    ${CURRENT_SOURCE_DIR}/plugin/compile.ets
    ${CURRENT_BINARY_DIR}/plugin
    ARKTS_CONFIG ${ETS_CONFIG}
    ENTRYPOINT ${TEST_PACKAGE}.plugin/compile
    ABC_LIBS ${ARKTS_BINDINGS_LIB}
    RUNTIME_EXTRA_OPTIONS ${RUNTIME_ARGS}
    PROGRAM_ARGS ${TEST_CASE_FILE} ${PANDA_BINARY_ROOT}/bin/arktsconfig.json
)

add_dependencies(arkts_bindings_tests_api arkts_bindings_tests_api-plugin)