# Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
    "util.cpp"
)

set(PLUGIN_TESTS
    #"test_name test_sts_file how_to_test_mode plugin_file_extension(c|cpp) how_to_compile_mode"
    "e2p_test_plugin_dump_json compile.ets ${RUNTIME_MODE} c ${LIBRARY_PLUGIN}"
    "e2p_test_plugin_ets_src compile.ets ${RUNTIME_MODE} c ${LIBRARY_PLUGIN}"
    "e2p_test_plugin_ets_benchmark_test compile.ets ${RUNTIME_MODE} cpp ${LIBRARY_PLUGIN}"
    "e2p_test_plugin_ets_node_types compile.ets ${RUNTIME_MODE} c ${LIBRARY_PLUGIN}"
    "e2p_test_plugin_ets_varibles_and_types compile.ets ${RUNTIME_MODE} c ${LIBRARY_PLUGIN}"
    "e2p_test_plugin_change_func runtime_change_func_call.ets ${RUNTIME_MODE} cpp ${LIBRARY_PLUGIN}"
    "e2p_test_plugin_recheck compile.ets ${COMPILE_MODE} c ${LIBRARY_PLUGIN}"
    "e2p_test_plugin compile.ets ${EXPECTED_MODE} c ${LIBRARY_PLUGIN}"
    "e2p_test_plugin_is compile.ets ${EXPECTED_MODE} c ${LIBRARY_PLUGIN}"
    "e2p_test_plugin_after_bind compile.ets ${EXPECTED_MODE} cpp ${LIBRARY_PLUGIN}"
    "plugin_proceed_to_state compile.ets ${EXPECTED_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_update_statements compile.ets ${RUNTIME_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_ts_mode compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_update_statements_lambda compile.ets ${RUNTIME_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_change_func runtime_change_func_call.ets ${RUNTIME_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_change_call_lambda compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_test_annotation_change compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_test_class_decl_annotation compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_check_manual_capi compile_with_external_source.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_check_script_function_flags compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_find_import_declaration compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    # See #22753.
    # "plugin_proceed_to_state_test_number_literal compile.ets ${RUNTIME_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_rerun_scopes_after_check compile.ets ${RUNTIME_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_rerun_scopes_on_import import.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    # "plugin_proceed_to_state_rerun_scopes_after_lowered compile.ets ${RUNTIME_MODE} cpp ${EXECUTABLE_PLUGIN}"
    # "plugin_proceed_to_state_run_verifier compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_ast_node_check compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_resolve_path compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_arktsconfig compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_find_method_decl compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_find_method_decl_by_name import.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_find_identifier_decl compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_find_import_ident_decl import.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_test_import_external_sources import.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_rebind compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_is_class_declaration compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_struct_declaration compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_create_function_type_ir compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_function_type_checker compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_create_ets_parameter_expression compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_ast_node_type_const compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_ast_node_transform_children_recursively compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_create_object_expression compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_misc_expr compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_return_expression compile.ets ${EXPECTED_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_function_to_arrow compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_string_optimize compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_update_import_declaration compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_create_import_declaration compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_update_import_specifier compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_create_non_null_expression compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_ast_node_type_analyzer compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_create_annotation compile.ets ${EXPECTED_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_update_class compile.ets ${EXPECTED_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_new_expression compile.ets ${EXPECTED_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_misc_expression compile.ets ${EXPECTED_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_member_expression compile.ets ${EXPECTED_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_this_into_method compile.ets ${EXPECTED_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_checker_type compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_is_accessor compile.ets ${EXPECTED_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_create_as_expression compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_create_interface_declaration compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_create_import compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_set_from_struct_modifier compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_default_access_modifier compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_dynamic_class_recheck compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_assignment_expression_set_result compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_create_diagnostic_kind compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_is_optional_declaration compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_update_function_declaration compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_update_function_expression compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_create_ets_new_expression compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_log_diagnostic_with_suggestion compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "use_plugin_to_test_export_table compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_check_jsdoc compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_test_global_func_call_dump compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_test_interface_duplicate_setter compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_test_case_block_dump compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "plugin_proceed_to_state_function_dump compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
    "pugin_proceed_to_state_annotationUsage_source_range_access compile.ets ${COMPILE_MODE} cpp ${EXECUTABLE_PLUGIN}"
)

set(RUNTIME_ARGUMENTS
    --boot-panda-files=${PANDA_BINARY_ROOT}/plugins/ets/etsstdlib.abc${EXTRA_BOOT_PANDAFILES}
    --load-runtimes=ets
    --compiler-enable-jit=true
)

set(ENTRY_POINT ETSGLOBAL::main)

foreach(TEST_DATA IN ITEMS ${PLUGIN_TESTS})
    string(REPLACE " " ";" TEST_DATA_ELEM "${TEST_DATA}")
    list(GET TEST_DATA_ELEM 0 TEST_NAME)
    list(GET TEST_DATA_ELEM 3 EXTENSION)
    list(GET TEST_DATA_ELEM 4 PLUGIN_MODE)
    if(${PLUGIN_MODE} STREQUAL ${LIBRARY_PLUGIN})
        panda_frontend_add_library(${TEST_NAME} SHARED ${TEST_NAME}.${EXTENSION} ${COMMON_SOURCE_FILES})
    else()
        panda_frontend_add_executable(${TEST_NAME} ${TEST_NAME}.${EXTENSION} ${COMMON_SOURCE_FILES} OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        panda_add_sanitizers(TARGET ${TEST_NAME} SANITIZERS ${PANDA_SANITIZERS_LIST})
    endif()
    panda_target_include_directories(${TEST_NAME}
        PRIVATE ${ES2PANDA_PATH}
        PRIVATE ${PANDA_ROOT}/libpandafile
        PRIVATE ${PANDA_ROOT}/assembler
        PRIVATE ${OUTPUT_DIR}
        PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC ${CMAKE_SOURCE_DIR}/libpandabase
        PUBLIC ${CMAKE_SOURCE_DIR}/runtime
        PUBLIC ${CMAKE_BINARY_DIR}/libpandabase
        PUBLIC ${CMAKE_BINARY_DIR}/libpandafile/include
        PUBLIC ${CMAKE_BINARY_DIR}/abc2program
        ${CMAKE_BINARY_DIR}
    )
    panda_target_link_libraries(${TEST_NAME} es2panda-public arkassembler arkbase arkfile abc2program)
endforeach()

add_custom_target(es2panda-plugin-test)

foreach(TEST_DATA IN ITEMS ${PLUGIN_TESTS})
    string(REPLACE " " ";" TEST_DATA_ELEM "${TEST_DATA}")
    list(GET TEST_DATA_ELEM 0 TEST_NAME)
    list(GET TEST_DATA_ELEM 1 TEST_STS_FILE)
    list(GET TEST_DATA_ELEM 2 TEST_MODE)
    list(GET TEST_DATA_ELEM 4 PLUGIN_MODE)
    if(${PLUGIN_MODE} STREQUAL ${LIBRARY_PLUGIN})
        add_custom_target(es2panda-plugin-test-compile-${TEST_NAME}
            COMMAND ${CMAKE_COMMAND} -E env
                LD_LIBRARY_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${PANDA_RUN_PREFIX} $<TARGET_FILE:es2panda>
                --plugins=${TEST_NAME} --extension=ets --ets-unnamed --output="${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}.abc"
                "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_STS_FILE}" > "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}.out"
        )
    endif()
    if(${PLUGIN_MODE} STREQUAL ${EXECUTABLE_PLUGIN})
        add_custom_target(es2panda-plugin-test-compile-${TEST_NAME}
            COMMAND ${CMAKE_COMMAND} -E env
                LD_LIBRARY_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${PANDA_RUN_PREFIX}
                ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME} $<TARGET_FILE:es2panda>
                --extension=ets --ets-unnamed --output="${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}.abc"
                "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_STS_FILE}" > "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}.out" 2>&1
        )
    endif()
    add_dependencies(es2panda-plugin-test-compile-${TEST_NAME} es2panda ${TEST_NAME} es2panda-lib)
    if(${TEST_MODE} STREQUAL ${EXPECTED_MODE})
        add_custom_target(es2panda-plugin-test-expected-${TEST_NAME}
            COMMAND ${CMAKE_COMMAND} -E compare_files
                "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}.out" "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}-expected.txt"
        )
        add_dependencies(es2panda-plugin-test-expected-${TEST_NAME} es2panda-plugin-test-compile-${TEST_NAME} es2panda-lib)
        add_dependencies(es2panda-plugin-test es2panda-plugin-test-expected-${TEST_NAME})
    endif()
    if(${TEST_MODE} STREQUAL ${RUNTIME_MODE})
        add_custom_target(es2panda-plugin-test-runtime-${TEST_NAME}
            COMMAND ${CMAKE_COMMAND} -E env
                LD_LIBRARY_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${PANDA_RUN_PREFIX} $<TARGET_FILE:ark>
                ${RUNTIME_ARGUMENTS} "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}.abc" ${ENTRY_POINT}
                > "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}-runtime.out"
        )
        add_dependencies(es2panda-plugin-test-runtime-${TEST_NAME} es2panda-plugin-test-compile-${TEST_NAME} ark es2panda-lib)
        add_dependencies(es2panda-plugin-test es2panda-plugin-test-runtime-${TEST_NAME})
    endif()
    if(${TEST_MODE} STREQUAL ${COMPILE_MODE})
        add_dependencies(es2panda-plugin-test es2panda-plugin-test-compile-${TEST_NAME})
    endif()
endforeach()

add_custom_target(update-es2panda-plugin-test-expected)

foreach(TEST_DATA IN ITEMS ${PLUGIN_TESTS})
    string(REPLACE " " ";" TEST_DATA_ELEM "${TEST_DATA}")
    list(GET TEST_DATA_ELEM 0 TEST_NAME)
    list(GET TEST_DATA_ELEM 1 TEST_STS_FILE)
    list(GET TEST_DATA_ELEM 2 TEST_MODE)
    list(GET TEST_DATA_ELEM 4 PLUGIN_MODE)
    if(${TEST_MODE} STREQUAL ${EXPECTED_MODE})
        if(${PLUGIN_MODE} STREQUAL ${LIBRARY_PLUGIN})
            add_custom_target(update-es2panda-plugin-test-expected-${TEST_NAME}
                COMMAND ${CMAKE_COMMAND} -E env
                    LD_LIBRARY_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${PANDA_RUN_PREFIX} $<TARGET_FILE:es2panda>
                    --plugins=${TEST_NAME} --extension=ets --ets-unnamed "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_STS_FILE}" >
                    "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}-expected.txt")
        else()
            add_custom_target(update-es2panda-plugin-test-expected-${TEST_NAME}
                COMMAND ${CMAKE_COMMAND} -E env
                    LD_LIBRARY_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${PANDA_RUN_PREFIX}
                    ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME} $<TARGET_FILE:es2panda>
                    "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_STS_FILE}" >
                    "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}-expected.txt")
        endif()
        add_dependencies(update-es2panda-plugin-test-expected-${TEST_NAME} es2panda ${TEST_NAME} es2panda-lib)
        add_dependencies(update-es2panda-plugin-test-expected update-es2panda-plugin-test-expected-${TEST_NAME})
    endif()
endforeach()

add_dependencies(es2panda_tests es2panda-plugin-test)
