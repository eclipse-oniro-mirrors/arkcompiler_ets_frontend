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

set(CPP_SOURCES
    test_entry.cpp
    type_conversion/type_conversion.cpp
    native_object/native_object.cpp
    bridge_templates/bridge_templates.cpp
)

set(ETS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(ETS_SOURCES
    ${ETS_SRC_DIR}/test_entry.ets
    ${ETS_SRC_DIR}/native.ets
    ${ETS_SRC_DIR}/type_conversion/type_conversion.ets
    ${ETS_SRC_DIR}/native_object/native_object.ets
    ${ETS_SRC_DIR}/bridge_templates/bridge_templates.ets
)

panda_add_library(arkts_bindings_tests_converters_native SHARED ${CPP_SOURCES})
panda_target_link_libraries(arkts_bindings_tests_converters_native arkruntime)
panda_target_include_directories(arkts_bindings_tests_converters_native
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${PANDA_ETS_PLUGIN_SOURCE}/runtime/ani
    PRIVATE ${ES2PANDA_PATH}/arkts_bindings/native/src
)

set(TEST_INTEROP_MODULE "-DETS_INTEROP_MODULE=TestNativeModule")

set_target_properties(arkts_bindings_tests_converters_native PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
panda_target_compile_definitions(arkts_bindings_tests_converters_native PRIVATE ${TEST_INTEROP_MODULE})
panda_target_compile_options(arkts_bindings_tests_converters_native
    PRIVATE -fexceptions -Werror=shadow
)
panda_add_sanitizers(TARGET arkts_bindings_tests_converters_native 
    SANITIZERS ${PANDA_SANITIZERS_LIST}
)

set(RUNTIME_ARGS "--native-library-path=${CMAKE_CURRENT_BINARY_DIR}")

run_arkts_bindings_test(arkts_bindings_tests_converters
    "${ETS_SOURCES}"
    ${CMAKE_CURRENT_BINARY_DIR}
    RUNTIME_EXTRA_OPTIONS ${RUNTIME_ARGS}
)

add_dependencies(arkts_bindings_tests_converters arkts_bindings_tests_converters_native)
add_dependencies(arkts_bindings_tests arkts_bindings_tests_converters)