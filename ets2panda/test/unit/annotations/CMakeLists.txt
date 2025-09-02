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

ets2panda_add_gtest(es2panda_annotations_emit_standard_test
    CPP_SOURCES standard_test.cpp
)
ets2panda_add_gtest(es2panda_mutiple_annotations_for_class
    CPP_SOURCES mutiple_annotations_for_class.cpp    
)
ets2panda_add_gtest(es2panda_mutiple_annotations_for_function
    CPP_SOURCES mutiple_annotations_for_function.cpp    
)
ets2panda_add_gtest(es2panda_annotations_for_interface
    CPP_SOURCES annotations_for_interface.cpp    
)
ets2panda_add_gtest(es2panda_annotations_for_function
    CPP_SOURCES annotations_for_function.cpp    
)
ets2panda_add_gtest(es2panda_annotations_for_class
    CPP_SOURCES annotations_for_class.cpp    
)
ets2panda_add_gtest(annotations_retention_policy
    CPP_SOURCES annotations_retention_policy.cpp    
)
ets2panda_add_gtest(annotations_for_namespace
    CPP_SOURCES annotations_for_namespace.cpp    
)
ets2panda_add_gtest(annotations_module
    CPP_SOURCES annotations_module.cpp
)
ets2panda_add_gtest(annotations_for_functional_objects
    CPP_SOURCES annotations_for_functional_objects.cpp
)
