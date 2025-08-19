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

function(panda_code_fix_gen)
    set(singlevalues SOURCE DESTINATION TARGET_NAME)
    set(multivalues DATA TEMPLATES REQUIRES EXTRA_DEPENDENCIES EXTRA_ARGV)
    cmake_parse_arguments(
        CODE_FIX_GEN_ARG
        ""
        "${singlevalues}"
        "${multivalues}"
        ${ARGN}
    )
    list(LENGTH CODE_FIX_GEN_ARG_DATA DATA_COUNT)
    set(CODE_FIX_API "${ES2PANDA_ROOT}/lsp/code_fix_register.rb")
    foreach(i RANGE 1 ${DATA_COUNT})
        list(APPEND API_LIST ${CODE_FIX_API})
    endforeach()

    panda_gen(DATA ${CODE_FIX_GEN_ARG_DATA}
        API ${API_LIST}
        TEMPLATES ${CODE_FIX_GEN_ARG_TEMPLATES}
        SOURCE ${CODE_FIX_GEN_ARG_SOURCE}
        TARGET_NAME ${CODE_FIX_GEN_ARG_TARGET_NAME}
        DESTINATION ${CODE_FIX_GEN_ARG_DESTINATION}
        REQUIRES ${CODE_FIX_GEN_ARG_REQUIRES}
        EXTRA_DEPENDENCIES ${CODE_FIX_GEN_ARG_EXTRA_DEPENDENCIES}
    )
endfunction()

set(DIAGNOSTIC_DIR
   ${ES2PANDA_ROOT}/util/diagnostic/
)

panda_code_fix_gen(
    DATA
        ${DIAGNOSTIC_DIR}/syntax.yaml
        ${DIAGNOSTIC_DIR}/semantic.yaml
        ${DIAGNOSTIC_DIR}/warning.yaml
        ${DIAGNOSTIC_DIR}/fatal.yaml
        ${ES2PANDA_ROOT}/declgen_ets2ts/declgen_ets2ts_error.yaml
        ${ES2PANDA_ROOT}/declgen_ets2ts/declgen_ets2ts_warning.yaml
        ${ES2PANDA_ROOT}/declgen_ets2ts/isolated_declgen.yaml
        ${DIAGNOSTIC_DIR}/arktsconfig_error.yaml
    TARGET_NAME es2panda_code_fix_register_gen
    TEMPLATES code_fix_register.h.erb
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${GENERATED_DIR}
)

set(ES2PANDA_LSP_SRC
  ./src/api.cpp
  ./src/class_hierarchy.cpp
  ./src/class_hierarchy_info.cpp
  ./src/classifier.cpp
  ./src/internal_api.cpp
  ./src/isolated_declaration.cpp
  ./src/cancellation_token.cpp
  ./src/completions.cpp
  ./src/organize_imports.cpp
  ./src/quick_info.cpp
  ./src/completions_details.cpp
  ./src/references.cpp
  ./src/get_adjusted_location.cpp
  ./src/get_safe_delete_info.cpp
  ./src/find_rename_locations.cpp
  ./src/find_safe_delete_location.cpp
  ./src/find_references.cpp
  ./src/refactors/refactor_types.cpp
  ./src/applicable_refactors.cpp
  ./src/refactor_provider.cpp
  ./src/refactors/convert_chain.cpp
  ./src/refactors/convert_export.cpp
  ./src/refactors/convert_function.cpp
  ./src/refactors/convert_import.cpp
  ./src/refactors/convert_template.cpp
  ./src/formatting/formatting_context.cpp
  ./src/formatting/formatting_settings.cpp
  ./src/formatting/formatting.cpp
  ./src/formatting/rules_map.cpp
  ./src/formatting/rules.cpp
  ./src/string_completions.cpp
  ./src/rename.cpp
  ./src/generate_constructor.cpp
  ./src/suggestion_diagnostics.cpp
  ./src/brace_matching.cpp
  ./src/services/services.cpp
  ./src/services/utilities.cpp
  ./src/line_column_offset.cpp
  ./src/services/text_change/change_tracker.cpp
  ./src/code_fix_provider.cpp
  ./src/inlay_hints.cpp
  ./src/get_class_property_info.cpp
  ./src/create_type_help_items.cpp
  ./src/script_element_kind.cpp
  ./src/signature_help_items.cpp
  ./src/signature_help.cpp
  ./src/todo_comments.cpp
  ./src/get_definition_and_bound_span.cpp
  ./src/types.cpp
  ./src/navigate_to.cpp
  ./src/code_fix_provider.cpp
  ./src/register_code_fix/add_local_variable.cpp
  ./src/register_code_fix/add_missing_new_operator.cpp
  ./src/register_code_fix/fix_class_doesnt_implement_inherited_abstract_member.cpp
  ./src/register_code_fix/fix_expected_comma.cpp
  ./src/register_code_fix/fix_extends_interface_becomes_implements.cpp
  ./src/register_code_fix/fix_class_super_must_precede_this_access.cpp
  ./src/register_code_fix/fix_return_type_in_async_function.cpp
  ./src/register_code_fix/add_name_to_nameless_parameter.cpp
  ./src/register_code_fix/add_missing_declare_property.cpp
  ./src/register_code_fix/convert_const_to_let.cpp
  ./src/register_code_fix/constructor_for_derived_need_super_call
  ./src/register_code_fix/fix_missing_call_parantheses.cpp
  ./src/register_code_fix/fix_import_non_exported_member.cpp
  ./src/register_code_fix/fix_nan_equality.cpp
  ./src/register_code_fix/forgotten_this_property_access.cpp
  ./src/register_code_fix/import_fixes.cpp
  ./src/register_code_fix/remove_accidental_call_parentheses.cpp
  ./src/register_code_fix/fix_remove_override_modifier.cpp
  ./src/register_code_fix/fix_add_function_return_statement.cpp
  ./src/register_code_fix/ui_plugin_suggest.cpp
  ./src/register_code_fix/fix_spelling.cpp
  ./src/register_code_fix/fix_class_incorrectly_implements_interface.cpp
  ./src/get_signature.cpp
  ./src/get_name_or_dotted_name_span.cpp
  ./src/get_node.cpp
  ./src/node_matchers.cpp
)

panda_frontend_add_library(${LSP_LIB} SHARED ${ES2PANDA_LSP_SRC})
add_dependencies(${LSP_LIB} es2panda_code_fix_register_gen)
add_dependencies(frontend_bins ${LSP_LIB})

panda_target_include_directories(${LSP_LIB}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${ES2PANDA_ROOT}
  PRIVATE ${OUTPUT_DIR}
)

panda_target_compile_options(${LSP_LIB}
  PRIVATE -fexceptions -Werror=shadow -Wno-return-type-c-linkage
)

panda_target_link_libraries(${LSP_LIB}
  PUBLIC arkbase es2panda-lib es2panda-public
  PRIVATE arkbytecodeopt
)

panda_add_sanitizers(TARGET ${LSP_LIB} SANITIZERS
  ${PANDA_SANITIZERS_LIST})

