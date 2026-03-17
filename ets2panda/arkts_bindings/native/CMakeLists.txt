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

set(BINDINGS_NATIVE_SRC
  ./src/bridges.cpp
  ./src/common.cpp
  ./src/common-interop.cpp
  ./src/converters-ani.cpp
  ./src/generated/bridges.cpp
)

set(ES2PANDA_LIBS
  arkruntime
)

set(ETS_INTEROP_MODULE "-DETS_INTEROP_MODULE=Es2pandaNativeModule")

panda_frontend_add_library(arkts_bindings_native SHARED ${BINDINGS_NATIVE_SRC})

panda_target_link_libraries(arkts_bindings_native ${ES2PANDA_LIBS})

panda_target_include_directories(arkts_bindings_native
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
  PRIVATE ${ES2PANDA_ROOT}
  PRIVATE ${OUTPUT_DIR}
  PRIVATE ${PANDA_ETS_PLUGIN_SOURCE}/runtime/ani
)

add_dependencies(arkts_bindings_native
  es2panda-lib
  es2panda-public
)

panda_target_compile_definitions(arkts_bindings_native PRIVATE ${ETS_INTEROP_MODULE})
panda_target_compile_options(arkts_bindings_native
  PRIVATE -fexceptions -Werror=shadow
)

panda_add_sanitizers(TARGET arkts_bindings_native 
  SANITIZERS ${PANDA_SANITIZERS_LIST}
)