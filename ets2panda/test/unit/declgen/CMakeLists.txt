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

if (PANDA_ENABLE_ADDRESS_SANITIZER OR PANDA_ENABLE_THREAD_SANITIZER)
    return()
endif()

set(COMMON_SOURCE_FILES
    "util.cpp"
)

set(DECLGEN_PLUGIN_TESTS
    "test_ets2ts_isolated_declgen cpp"
)

set(ETS2TS_ISOLATED_TESTS
    "./ets2ts_isolated/test_ets2ts_isolated_class.ets"
    "./ets2ts_isolated/test_ets2ts_isolated_enum.ets"
    "./ets2ts_isolated/test_ets2ts_isolated_module.ets"
    "./ets2ts_isolated/test_ets2ts_isolated_namespace.ets"
    "./ets2ts_isolated/test_ets2ts_isolated_interface.ets"
    "./ets2ts_isolated/test_ets2ts_isolated_function_with_optional_parameter.ets"
    "./ets2ts_isolated/test_ets2ts_infer_function_return_type_with_import_symbol.ets"
)

foreach(TEST_DATA IN ITEMS ${DECLGEN_PLUGIN_TESTS})
    string(REPLACE " " ";" TEST_DATA_ELEM "${TEST_DATA}")
    list(GET TEST_DATA_ELEM 0 TEST_NAME)
    list(GET TEST_DATA_ELEM 1 EXTENSION)

    panda_frontend_add_executable(${TEST_NAME} ${TEST_NAME}.${EXTENSION} ${COMMON_SOURCE_FILES} OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    panda_add_sanitizers(TARGET ${TEST_NAME} SANITIZERS ${PANDA_SANITIZERS_LIST})

    panda_target_include_directories(${TEST_NAME}
        PRIVATE ${ES2PANDA_PATH}
        PRIVATE ${PANDA_ROOT}/assembler
        PRIVATE ${OUTPUT_DIR}
        PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC ${CMAKE_SOURCE_DIR}/libpandabase
        PUBLIC ${CMAKE_SOURCE_DIR}/runtime
        PUBLIC ${CMAKE_BINARY_DIR}/libpandabase
        ${CMAKE_BINARY_DIR}
    )
    panda_target_link_libraries(${TEST_NAME} es2panda-public arkassembler arkbase arkfile)
endforeach()

add_custom_target(es2panda-declgen-plugin-test)

set(ETS2TS "ets2ts_isolated")
foreach(TEST_DATA IN ITEMS ${DECLGEN_PLUGIN_TESTS})
    string(REPLACE " " ";" TEST_DATA_ELEM "${TEST_DATA}")
    list(GET TEST_DATA_ELEM 0 TEST_NAME)
    list(GET TEST_DATA_ELEM 1 EXTENSION)
    foreach(TEST_ETS_FILE IN ITEMS ${ETS2TS_ISOLATED_TESTS})
        cmake_path(SET PATH_ITEM "${TEST_ETS_FILE}")
        cmake_path(GET PATH_ITEM STEM FILE_NAME)
        add_custom_target(es2panda-declgen-plugin-test-compile-${FILE_NAME}
            COMMAND ${CMAKE_COMMAND} -E env
                LD_LIBRARY_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${PANDA_RUN_PREFIX}
                ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME} $<TARGET_FILE:es2panda>
                --extension=ets --ets-unnamed --output="${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.abc"
                "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_ETS_FILE}" > "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.out" 2>&1
        )
        add_dependencies(es2panda-declgen-plugin-test-compile-${FILE_NAME} es2panda ${TEST_NAME} es2panda-lib)
        add_dependencies(es2panda-declgen-plugin-test es2panda-declgen-plugin-test-compile-${FILE_NAME})

        add_custom_target(es2panda-declgen-plugin-test-expected-${FILE_NAME}
            COMMAND ${CMAKE_COMMAND} -E compare_files
                "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.d.ets" "${CMAKE_CURRENT_SOURCE_DIR}/${ETS2TS}/${FILE_NAME}-expected.txt"
        )
        add_dependencies(es2panda-declgen-plugin-test-expected-${FILE_NAME} es2panda-declgen-plugin-test-compile-${FILE_NAME} es2panda-lib)
        add_dependencies(es2panda-declgen-plugin-test es2panda-declgen-plugin-test-expected-${FILE_NAME})
    endforeach()
endforeach()

add_dependencies(es2panda_tests es2panda-declgen-plugin-test)