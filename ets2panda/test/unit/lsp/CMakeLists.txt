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

ets2panda_add_gtest(lsp_get_definition_at_position_test CPP_SOURCES
    get_definition_at_position_test.cpp
)

ets2panda_add_gtest(lsp_get_file_references_test CPP_SOURCES
    get_file_references_test.cpp
)

ets2panda_add_gtest(lsp_api_script_element_kind CPP_SOURCES
    script_element_kind_test.cpp
)

ets2panda_add_gtest(lsp_get_references_at_position_test CPP_SOURCES
    get_references_at_position_test.cpp
)

ets2panda_add_gtest(lsp_document_highlights_test CPP_SOURCES
    document_highlights.cpp
)

ets2panda_add_gtest(lsp_diagnostic_test CPP_SOURCES
    diagnostic_test.cpp
)

ets2panda_add_gtest(lsp_isolated_declaration_test CPP_SOURCES
    isolated_declaration.cpp
)

ets2panda_add_gtest(lsp_cancellation_token_test CPP_SOURCES
    cancellation_token_test.cpp
)

ets2panda_add_gtest(lsp_get_span_of_enclosing_comment_test CPP_SOURCES
    get_span_of_enclosing_comment_test.cpp
)

ets2panda_add_gtest(lsp_user_preferences_test CPP_SOURCES
    user_preferences.cpp
)

ets2panda_add_gtest(lsp_keyword_completion_data_test CPP_SOURCES
    keyword_completion_data_test.cpp
)

ets2panda_add_gtest(lsp_rename_test CPP_SOURCES
    lsp_rename_test.cpp
)

ets2panda_add_gtest(lsp_get_current_token_value_test CPP_SOURCES
    get_current_token_value_test.cpp
)

ets2panda_add_gtest(lsp_get_file_references_impl_test CPP_SOURCES
    get_file_references_impl_test.cpp
)

ets2panda_add_gtest(lsp_get_preceding_token_test CPP_SOURCES
    get_preceding_token_test.cpp
)

ets2panda_add_gtest(lsp_get_touching_token_test CPP_SOURCES
    get_touching_token_test.cpp
)

ets2panda_add_gtest(lsp_get_type_of_symbol_at_location_test CPP_SOURCES
    get_type_of_symbol_at_location_test.cpp
)

ets2panda_add_gtest(lsp_api_test_organize_imports CPP_SOURCES
    organize_imports_test.cpp
)

ets2panda_add_gtest(lsp_quick_info_api_test CPP_SOURCES
    quick_info_test.cpp
)

ets2panda_add_gtest(lsp_quick_info_api_test_1 CPP_SOURCES
    quick_info_test_1.cpp
)

ets2panda_add_gtest(lsp_quick_info_api_test_api CPP_SOURCES
    quick_info_api_test.cpp
)

ets2panda_add_gtest(lsp_api_completions_entry_details_test CPP_SOURCES
    get_completions_entry_details.cpp
)

ets2panda_add_gtest(lsp_api_classifier_test CPP_SOURCES
    classifier_test.cpp
)

ets2panda_add_gtest(lsp_api_test_class_hierarchies CPP_SOURCES
    class_hierarchys_test.cpp
)

ets2panda_add_gtest(lsp_api_test_refactors_convert_chain CPP_SOURCES
    refactors_convert_chain_test.cpp
)

ets2panda_add_gtest(lsp_api_test_refactors_convert_export CPP_SOURCES
    refactors_convert_export_test.cpp
)

ets2panda_add_gtest(lsp_api_test_refactors_convert_function CPP_SOURCES
    refactors_convert_function_test.cpp
)

ets2panda_add_gtest(lsp_api_test_refactors_convert_import CPP_SOURCES
    refactors_convert_import_test.cpp
)

ets2panda_add_gtest(lsp_api_test_refactors_convert_template CPP_SOURCES
    refactors_convert_template_test.cpp
)

ets2panda_add_gtest(lsp_api_diagnostics_test CPP_SOURCES
    get_compiler_options_diagnostics_test.cpp
    get_diagnostics.cpp
)

ets2panda_add_gtest(lsp_api_get_adjusted_location_test CPP_SOURCES
    get_adjusted_location_test.cpp
)

ets2panda_add_gtest(lsp_api_test_class_hierarchy CPP_SOURCES
    class_hierarchy_test.cpp
)

ets2panda_add_gtest(lsp_api_test_class_hierarchy2 CPP_SOURCES
    class_hierarchy2_test.cpp
)

ets2panda_add_gtest(lsp_api_test_class_hierarchy3 CPP_SOURCES
    class_hierarchy3_test.cpp
)

ets2panda_add_gtest(lsp_api_test_class_hierarchy4 CPP_SOURCES
    class_hierarchy4_test.cpp
)

ets2panda_add_gtest(lsp_api_test_find_safe_delete_location CPP_SOURCES
    find_safe_delete_location_test.cpp
)

ets2panda_add_gtest(lsp_api_test_find_references CPP_SOURCES
    find_references_test.cpp
)

ets2panda_add_gtest(lsp_api_slc_tests CPP_SOURCES
    string_completions_test.cpp
)

ets2panda_add_gtest(lsp_api_test_suggestion_diagnostics CPP_SOURCES
    suggestion_diagnostics_test.cpp
)

ets2panda_add_gtest(lsp_api_test_suggestion_diagnostics_1 CPP_SOURCES
    suggestion_diagnostics_test_1.cpp
)

ets2panda_add_gtest(lsp_api_test_get_class_property_info CPP_SOURCES
    get_class_property_info_test.cpp
)

ets2panda_add_gtest(lsp_api_test_brace_matching CPP_SOURCES
    brace_matching_test.cpp
)

ets2panda_add_gtest(lsp_api_completions_test CPP_SOURCES
    get_completions.cpp
)

ets2panda_add_gtest(lsp_api_completions_test_1 CPP_SOURCES
    get_completions_1.cpp
)

ets2panda_add_gtest(lsp_api_ui_suggest_test CPP_SOURCES
    code_fix/ui_plugin_suggest.cpp
)

ets2panda_add_gtest(lsp_api_test_get_class_hierarchy_info CPP_SOURCES
    class_hierarchy_info_test.cpp
)

ets2panda_add_gtest(lsp_api_gilap_test CPP_SOURCES
    get_implementation_location.cpp
)

ets2panda_add_gtest(lsp_api_lco_test CPP_SOURCES
    line_column_offset_test.cpp
)

ets2panda_add_gtest(lsp_api_get_constructor_test CPP_SOURCES
    get_constructor_test.cpp
)

ets2panda_add_gtest(lsp_api_test_auto_completion CPP_SOURCES
    get_auto_completion_test.cpp
)

ets2panda_add_gtest(lsp_api_test_inlay_hints CPP_SOURCES
    inlay_hints_test.cpp
)

ets2panda_add_gtest(lsp_api_test_refactor_provider_test CPP_SOURCES
    refactor_provider_test.cpp
)

ets2panda_add_gtest(lsp_api_test_find_rename_locations CPP_SOURCES
    find_rename_locations_test.cpp
)

ets2panda_add_gtest(lsp_api_test_find_rename_locations_2 CPP_SOURCES
    find_rename_locations_test_2.cpp
)

ets2panda_add_gtest(lsp_api_test_fix_expected_comma CPP_SOURCES
    fix_expected_comma_test.cpp
)

ets2panda_add_gtest(lsp_api_test_change_tracker CPP_SOURCES
    change_tracker_test.cpp
)

ets2panda_add_gtest(lsp_api_test_get_safe_delete_info CPP_SOURCES
    get_safe_delete_info_test.cpp
)

ets2panda_add_gtest(lsp_api_test_create_type_help_items CPP_SOURCES
    create_type_help_items_test.cpp
)

ets2panda_add_gtest(lsp_api_test_signature_help_items CPP_SOURCES
    signature_help_items_test.cpp
)

ets2panda_add_gtest(lsp_api_test_get_def_and_bound_span CPP_SOURCES
    get_definition_and_bound_span_test.cpp
)

ets2panda_add_gtest(lsp_api_test_formatting_context CPP_SOURCES
    formatting_context_test.cpp
)

ets2panda_add_gtest(lsp_api_test_formatting CPP_SOURCES
    formatting_test.cpp
)

ets2panda_add_gtest(lsp_api_test_formatting_1 CPP_SOURCES
    formatting_test_1.cpp
)

ets2panda_add_gtest(lsp_api_test_formatting_2 CPP_SOURCES
    formatting_test_2.cpp
)

ets2panda_add_gtest(lsp_api_test_rulesmap CPP_SOURCES
    rules_map_cache_test.cpp
)

ets2panda_add_gtest(lsp_api_test_todo_comments CPP_SOURCES
    todo_comments_test.cpp
)

ets2panda_add_gtest(lsp_api_test_navigate_to CPP_SOURCES
    navigate_to_test.cpp
)

ets2panda_add_gtest(lsp_api_test_offset CPP_SOURCES
    get_offset_by_col_and_line.cpp
)

ets2panda_add_gtest(lsp_api_test_code_fix_registration CPP_SOURCES
    code_fix_registration_test.cpp
)
ets2panda_add_gtest(lsp_api_test_super_must_precede_this CPP_SOURCES
    fix_class_super_must_precede_this_access_test.cpp
)

ets2panda_add_gtest(lsp_api_test_fix_return_type_in_async_func CPP_SOURCES
    fix_return_type_in_async_function_test.cpp
)

ets2panda_add_gtest(lsp_api_test_fix_abstract_member CPP_SOURCES
    fix_class_doesnt_implement_inherited_abstract_member_test.cpp
)

ets2panda_add_gtest(lsp_api_test_fix_class_incorrectly_implements_interface CPP_SOURCES
    fix_class_incorrectly_implements_interface_test.cpp
)

ets2panda_add_gtest(lsp_api_test_fix_add_function_return_statement_test CPP_SOURCES
    fix_add_function_return_statement_test.cpp
)

ets2panda_add_gtest(lsp_api_test_get_name_or_dotted_name_span CPP_SOURCES
    get_name_or_dotted_name_span_test.cpp
)

ets2panda_add_gtest(lsp_api_test_fix_convert_const_to_let CPP_SOURCES
    fix_convert_const_to_let_test.cpp
)

ets2panda_add_gtest(lsp_api_test_fix_extends_interface_becomes_implements CPP_SOURCES
    fix_extends_interface_becomes_implements_test.cpp
)

ets2panda_add_gtest(lsp_api_test_forgotten_this_property_access CPP_SOURCES
    forgotten_this_property_access_test.cpp
)

ets2panda_add_gtest(lsp_api_test_remove_accidental_call_parentheses CPP_SOURCES
    remove_accidental_call_parentheses_test.cpp
)

ets2panda_add_gtest(lsp_api_test_fix_remove_override_modifier_test CPP_SOURCES
    fix_remove_override_modifier_test.cpp
)

ets2panda_add_gtest(lsp_api_add_missing_new_operator CPP_SOURCES
    add_missing_new_operator_test.cpp
)

ets2panda_add_gtest(lsp_api_test_add_local_variable CPP_SOURCES
    add_local_variable_test.cpp
)

ets2panda_add_gtest(lsp_get_import_file_path_test CPP_SOURCES
    get_import_file_path_test.cpp
)

ets2panda_add_gtest(lsp_get_signature_test CPP_SOURCES
    get_signature_test.cpp
)

ets2panda_add_gtest(lsp_api_test_fix_import_non_exported_member CPP_SOURCES
    fix_import_non_exported_member_test.cpp
)
ets2panda_add_gtest(lsp_api_get_node_test CPP_SOURCES
    get_node_test.cpp
)

ets2panda_add_gtest(lsp_api_get_node_infos_test CPP_SOURCES
    get_node_infos_test.cpp
)

ets2panda_add_gtest(lsp_api_get_definition_from_node_test CPP_SOURCES
    get_definition_from_node_test.cpp
)

ets2panda_add_gtest(lsp_api_spelling_test CPP_SOURCES
    fix_spelling_test.cpp
)

ets2panda_add_gtest(lsp_api_get_node_expression_test CPP_SOURCES
    get_node_expression_test.cpp
)

ets2panda_add_gtest(lsp_api_test_constructor_for_derived_need_super_call CPP_SOURCES
    constructor_for_derived_need_super_call_test.cpp
)

ets2panda_add_gtest(lsp_api_test_add_name_to_nameless_parameter CPP_SOURCES
    add_name_to_nameless_parameter_test.cpp
)

ets2panda_add_gtest(lsp_api_get_node_interface_test CPP_SOURCES
    get_node_interface_test.cpp
)

ets2panda_add_gtest(lsp_api_get_node_type_namespace_test CPP_SOURCES
    get_node_type_namespace_test.cpp
)

ets2panda_add_gtest(lsp_api_get_node_export_test CPP_SOURCES
    get_node_export_test.cpp
)

ets2panda_add_gtest(lsp_api_get_node_ts_class_Implements_test CPP_SOURCES
    get_node_ts_class_Implements_test.cpp
)
