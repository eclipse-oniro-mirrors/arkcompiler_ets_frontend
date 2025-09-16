# Copyright (c) 2024 Huawei Device Co., Ltd.
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

if(NOT PANDA_WITH_ETS)
    return()
endif()

ets2panda_add_gtest(scopes_initialization_test
    CPP_SOURCES scopes_initialization.cpp
)
ets2panda_add_gtest(string_constants_test
    CPP_SOURCES string_constants.cpp
)
ets2panda_add_gtest(const_expression_test
    CPP_SOURCES const_expression.cpp
)
ets2panda_add_gtest(top_level_statements_test
    CPP_SOURCES top_level_statements.cpp
)
ets2panda_add_gtest(node_history_test
    CPP_SOURCES node_history.cpp
)

# ets2panda_add_gtest(convert_primitive_cast_method_call
#     CPP_SOURCES convert_primitive_cast_method_call.cpp
# )

ets2panda_add_gtest(interface_object_literal_test
    CPP_SOURCES interface_object_literal.cpp
)
