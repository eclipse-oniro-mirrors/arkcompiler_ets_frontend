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

# now plugin tests with sanitizers triggers CI failure, which could not be reproduce locally, will be fixed in future
if (PANDA_ENABLE_ADDRESS_SANITIZER OR PANDA_ENABLE_THREAD_SANITIZER)
    return()
endif()

# how_to_test_mode
set(COMPILE_MODE "COMPILE")
set(RUNTIME_MODE "RUNTIME")
set(EXPECTED_MODE "EXPECTED")

# how_to_compile_mode
set(LIBRARY_PLUGIN "LIB")
set(EXECUTABLE_PLUGIN "EXE")

set(COMMON_SOURCE_FILES
    "../plugin/util.cpp"
)

set(PLUGIN_UT_TESTS
    #"test_name test_sts_file how_to_test_mode plugin_file_extension(c|cpp) how_to_compile_mode"
    "plugin_ut_variables_api compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_ut_new_array_instance_expression_api compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_ut_binary_expression_api compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
)

set(RUNTIME_ARGUMENTS
    --boot-panda-files=${PANDA_BINARY_ROOT}/plugins/ets/etsstdlib.abc${EXTRA_BOOT_PANDAFILES}
    --load-runtimes=ets
    --compiler-enable-jit=true
)

set(ENTRY_POINT ETSGLOBAL::main)

foreach(TEST_DATA IN ITEMS ${PLUGIN_UT_TESTS})
    string(REPLACE " " ";" TEST_DATA_ELEM "${TEST_DATA}")
    list(GET TEST_DATA_ELEM 0 TEST_NAME)
    list(GET TEST_DATA_ELEM 3 EXTENSION)

    panda_frontend_add_executable(${TEST_NAME} ${TEST_NAME}.${EXTENSION} ${COMMON_SOURCE_FILES} OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    panda_add_sanitizers(TARGET ${TEST_NAME} SANITIZERS ${PANDA_SANITIZERS_LIST})

    panda_target_include_directories(${TEST_NAME}
        PRIVATE "${ES2PANDA_PATH}"
        PRIVATE ${OUTPUT_DIR}
    )
    panda_target_link_libraries(${TEST_NAME} es2panda-public)
endforeach()

add_custom_target(es2panda-plugin-ut-test)

foreach(TEST_DATA IN ITEMS ${PLUGIN_UT_TESTS})
    string(REPLACE " " ";" TEST_DATA_ELEM "${TEST_DATA}")
    list(GET TEST_DATA_ELEM 0 TEST_NAME)
    list(GET TEST_DATA_ELEM 1 TEST_STS_FILE)
    list(GET TEST_DATA_ELEM 2 TEST_MODE)

    add_custom_target(es2panda-plugin-ut-test-compile-${TEST_NAME}
        COMMAND ${CMAKE_COMMAND} -E env
            LD_LIBRARY_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${PANDA_RUN_PREFIX}
            ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME} $<TARGET_FILE:es2panda>
            --extension=ets --ets-unnamed --output="${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}.abc"
            "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_STS_FILE}" > "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}.out" 2>&1
    )

    add_dependencies(es2panda-plugin-ut-test-compile-${TEST_NAME} es2panda ${TEST_NAME} es2panda-lib)
    if(${TEST_MODE} STREQUAL ${EXPECTED_MODE})
        add_custom_target(es2panda-plugin-ut-test-expected-${TEST_NAME}
            COMMAND ${CMAKE_COMMAND} -E compare_files
                "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}.out" "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}-expected.txt"
        )
        add_dependencies(es2panda-plugin-ut-test-expected-${TEST_NAME} es2panda-plugin-ut-test-compile-${TEST_NAME} es2panda-lib)
        add_dependencies(es2panda-plugin-ut-test es2panda-plugin-ut-test-expected-${TEST_NAME})
    endif()
    if(${TEST_MODE} STREQUAL ${RUNTIME_MODE})
        add_custom_target(es2panda-plugin-ut-test-runtime-${TEST_NAME}
            COMMAND ${CMAKE_COMMAND} -E env
                LD_LIBRARY_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${PANDA_RUN_PREFIX} $<TARGET_FILE:ark>
                ${RUNTIME_ARGUMENTS} "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}.abc" ${ENTRY_POINT}
                > "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}-runtime.out"
        )
        add_dependencies(es2panda-plugin-ut-test-runtime-${TEST_NAME} es2panda-plugin-ut-test-compile-${TEST_NAME} ark es2panda-lib)
        add_dependencies(es2panda-plugin-ut-test es2panda-plugin-ut-test-runtime-${TEST_NAME})
    endif()
    if(${TEST_MODE} STREQUAL ${COMPILE_MODE})
        add_dependencies(es2panda-plugin-ut-test es2panda-plugin-ut-test-compile-${TEST_NAME})
    endif()
endforeach()

add_custom_target(update-es2panda-plugin-ut-test-expected)

foreach(TEST_DATA IN ITEMS ${PLUGIN_UT_TESTS})
    string(REPLACE " " ";" TEST_DATA_ELEM "${TEST_DATA}")
    list(GET TEST_DATA_ELEM 0 TEST_NAME)
    list(GET TEST_DATA_ELEM 1 TEST_STS_FILE)
    list(GET TEST_DATA_ELEM 2 TEST_MODE)
    if(${TEST_MODE} STREQUAL ${EXPECTED_MODE})
        add_custom_target(update-es2panda-plugin-ut-test-expected-${TEST_NAME}
            COMMAND ${CMAKE_COMMAND} -E env
                LD_LIBRARY_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${PANDA_RUN_PREFIX}
                ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME} $<TARGET_FILE:es2panda>
                "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_STS_FILE}" >
                "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}-expected.txt")

        add_dependencies(update-es2panda-plugin-ut-test-expected-${TEST_NAME} es2panda ${TEST_NAME} es2panda-lib)
        add_dependencies(update-es2panda-plugin-ut-test-expected update-es2panda-plugin-ut-test-expected-${TEST_NAME})
    endif()
endforeach()

add_dependencies(es2panda_tests es2panda-plugin-ut-test)
