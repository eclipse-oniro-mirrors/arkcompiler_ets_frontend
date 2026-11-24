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

# These tests create host projects and use the filesystem-enabled ArkTsConfig.
if(NOT PANDA_WITH_ETS OR PANDA_TARGET_MOBILE)
    return()
endif()

ets2panda_add_gtest(es2panda_import_path_manager_tests
    CPP_SOURCES
        import_path_manager_test.cpp
        import_dependencies_test.cpp
        import_interop_test.cpp
        import_programs_test.cpp
        import_bytecode_test.cpp
        arktsconfig_diagnostics_test.cpp
        arktsconfig_project_test.cpp
        arktsconfig_resolution_test.cpp
        import_identity_test.cpp
        import_incremental_test.cpp
        import_graph_edits_test.cpp
        arktsconfig_workspace_test.cpp
        import_session_test.cpp
        import_archive_reader_test.cpp
        import_cache_lookup_test.cpp
        import_resolution_edges_test.cpp
)
panda_target_compile_definitions(es2panda_import_path_manager_tests PRIVATE
    IPM_TEST_STDLIB="${PANDA_ROOT}/plugins/ets/stdlib"
)

panda_target_include_directories(es2panda_import_path_manager_tests SYSTEM PRIVATE
    "${PANDA_THIRD_PARTY_SOURCES_DIR}/flatbuffers/include"
    "${CMAKE_BINARY_DIR}/third_party/flatbuffers/generated"
)

add_custom_target(es2panda_module_resolution_tests
    DEPENDS es2panda_import_path_manager_tests_gtests es2panda_arktsconfig_parser_test_gtests
)
