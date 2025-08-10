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
  ./src/register_code_fix/add_missing_declare_property.cpp
  ./src/register_code_fix/fix_missing_call_parantheses.cpp
  ./src/register_code_fix/fix_nan_equality.cpp
  ./src/register_code_fix/forgetten_this_property_access.cpp
  ./src/register_code_fix/import_fixes.cpp
  ./src/register_code_fix/ui_plugin_suggest.cpp
  ./src/get_name_or_dotted_name_span.cpp
)

panda_frontend_add_library(${LSP_LIB} SHARED ${ES2PANDA_LSP_SRC})
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
