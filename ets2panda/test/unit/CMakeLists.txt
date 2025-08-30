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

if(NOT PANDA_REGRESSION_TESTS)
    return()
endif()

add_subdirectory(cfg)
add_subdirectory(declgen)
add_subdirectory(dynamic)
add_subdirectory(lowerings)
add_subdirectory(public)
add_subdirectory(plugin)
add_subdirectory(plugin_ut)
add_subdirectory(plugin_conversion_rule)
add_subdirectory(arktsconfig-parser)
add_subdirectory(annotations)
add_subdirectory(lsp)
add_subdirectory(relative_path)
add_subdirectory(any_ins_test)
add_subdirectory(source_file_message)
add_subdirectory(reg_allocator)

ets2panda_add_gtest(es2panda_astdumper_tests
    CPP_SOURCES ast_dumper_test.cpp
)

ets2panda_add_gtest(es2panda_name_mangling_tests
    CPP_SOURCES name_mangling_test.cpp
)

ets2panda_add_gtest(es2panda_union_normalization_tests_1
    CPP_SOURCES union_normalization_test_1.cpp
)
ets2panda_add_gtest(es2panda_union_normalization_tests_2
    CPP_SOURCES union_normalization_test_2.cpp
)

ets2panda_add_gtest(es2panda_globalETSObjectType_tests
    CPP_SOURCES globalETSObjectType_test.cpp
)

ets2panda_add_gtest(es2panda_union_emit_standard_test
    CPP_SOURCES union_emit_test.cpp
)

if (PANDA_TARGET_LINUX AND PANDA_TARGET_64)
    ets2panda_add_gtest(sizeof_node_tests
        CPP_SOURCES sizeof_node_test.cpp
    )
endif()

# NOTE: es2panda_rest_parameter_flag test runs a lot of time on qemu, so let's disable it
if (NOT PANDA_QEMU_BUILD)
    ets2panda_add_gtest(es2panda_rest_parameter_flag
        CPP_SOURCES rest_parameter_flag_test.cpp
    )
    ets2panda_add_gtest(es2panda_extern_flag_tests
        CPP_SOURCES extern_flag_test.cpp
    )
endif()

if(NOT PANDA_WITH_ETS)
    return()
endif()

add_subdirectory(ets_specific_optimizer)
ets2panda_add_gtest(es2panda_checker_tests
    CPP_SOURCES checker_test.cpp
)
