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

ets2panda_add_gtest(ast_builder_test
    CPP_SOURCES ast_builder_test.cpp
)

ets2panda_add_gtest(es2panda_public_test
    CPP_SOURCES es2panda_public_test.cpp
)

add_custom_target(qemu_excluded_gtests)

ets2panda_add_gtest(ast_verifier_test_1
    CPP_SOURCES
        ast_verifier_short_test.cpp
        ast_verifier_private_protected_public_access_correct_test.cpp
        ast_verifier_private_access_negative_test.cpp
        ast_verifier_protected_access_correct_test.cpp
        ast_verifier_protected_access_negative_test.cpp
        ast_verifier_reference_typeannotation_test.cpp
    TEST_GROUP qemu_excluded_gtests
)

ets2panda_add_gtest(ast_verifier_test_2
    CPP_SOURCES
        ast_verifier_identifier_has_variable_test.cpp
        ast_verifier_variable_has_enclosing_scope_test.cpp
        ast_verifier_variable_has_scope_test.cpp
        ast_verifier_every_child_has_valid_parent_test.cpp
    TEST_GROUP qemu_excluded_gtests
)

ets2panda_add_gtest(ast_verifier_test_3
    CPP_SOURCES
        ast_verifier_check_abstract_call_test.cpp
        ast_verifier_getter_setter_test.cpp
        ast_verifier_check_const_properties_test.cpp
        ast_verifier_getter_setter_neg_test.cpp
        ast_verifier_check_scope_declaration_test.cpp
    TEST_GROUP qemu_excluded_gtests
)

ets2panda_add_gtest(ast_verifier_test_4
        CPP_SOURCES
        ast_verifier_check_struct_declaration_test.cpp
        TEST_GROUP qemu_excluded_gtests
)

if (NOT PANDA_QEMU_BUILD)
    add_dependencies(es2panda_gtests qemu_excluded_gtests)
endif()
