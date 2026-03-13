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

project(arkts_bindings)

add_subdirectory(native)

set(ETS_CONFIG ${CMAKE_CURRENT_BINARY_DIR}/arktsconfig.json)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/arktsconfig.in.json ${ETS_CONFIG})

set(ABC_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/arkts_bindings.abc)

set(COMPILER_OPTIONS
    --ets-module
    --extension=ets
    --simultaneous
    --arktsconfig ${ETS_CONFIG}
    --emit-declaration
    --stdlib="${PANDA_ROOT}/plugins/ets/stdlib"
    --output ${ABC_OUTPUT}
)

add_custom_target(
    arkts_bindings_ets
    COMMENT "Compiling ETS code"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMAND $<TARGET_FILE:es2panda> ${COMPILER_OPTIONS}
    DEPENDS es2panda etsstdlib
)

add_custom_target(arkts_bindings)
add_dependencies(arkts_bindings arkts_bindings_native arkts_bindings_ets)