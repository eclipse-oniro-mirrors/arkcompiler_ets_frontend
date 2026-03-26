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

lsp_add_benchmark(lsp_start_perf CPP_SOURCES
    start_perf.cpp
)

lsp_add_benchmark(lsp_get_definition_at_position_perf CPP_SOURCES
    get_definition_at_position_perf.cpp
)

lsp_add_benchmark(lsp_find_rename_locations_perf CPP_SOURCES
    find_rename_locations_perf.cpp
)

lsp_add_benchmark(lsp_to_line_column_offset_perf CPP_SOURCES
    to_line_column_offset_perf.cpp
)

lsp_add_benchmark(lsp_get_completion_at_position_perf CPP_SOURCES
    get_completion_at_position_perf.cpp
)

lsp_add_benchmark(lsp_get_document_high_lights_perf CPP_SOURCES
    get_document_high_lights_perf.cpp
)

lsp_add_benchmark(lsp_get_formatting_edits_perf CPP_SOURCES
    get_formatting_edits_perf.cpp
)

lsp_add_benchmark(lsp_get_offset_by_col_and_line_perf CPP_SOURCES
    get_offset_by_col_and_line_perf.cpp
)

lsp_add_benchmark(lsp_get_organize_imports_perf CPP_SOURCES
    get_organize_imports_perf.cpp
)

lsp_add_benchmark(lsp_get_quick_info_at_position_perf CPP_SOURCES
    get_quick_info_at_position_perf.cpp
)

lsp_add_benchmark(lsp_get_references_at_position_perf CPP_SOURCES
    get_references_at_position_perf.cpp
)

lsp_add_benchmark(lsp_get_rename_info_perf CPP_SOURCES
    get_rename_info_perf.cpp
)

lsp_add_benchmark(lsp_refactor_extract_symbol_perf CPP_SOURCES
    refactor_extract_symbol_perf.cpp
)

lsp_add_benchmark(lsp_get_implementation_location_perf CPP_SOURCES
    get_implementation_location_perf.cpp
)

lsp_add_benchmark(lsp_get_syntactic_diagnostics_perf CPP_SOURCES
    get_syntactic_diagnostics_perf.cpp
)

lsp_add_benchmark(lsp_get_semantic_diagnostics_perf CPP_SOURCES
    get_semantic_diagnostics_perf.cpp
)

lsp_add_benchmark(lsp_get_safe_delete_info_perf CPP_SOURCES
    get_safe_delete_info_perf.cpp
)

lsp_add_benchmark(lsp_get_current_token_value_perf CPP_SOURCES
    get_current_token_value_perf.cpp
)
