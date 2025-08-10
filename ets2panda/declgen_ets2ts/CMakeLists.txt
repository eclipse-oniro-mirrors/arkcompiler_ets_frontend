# Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

set(DECLGEN_ETS2TS_SRC
  main.cpp
  declgenEts2Ts.cpp
)

set(DECLGEN_ETS2TS_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

panda_frontend_add_executable(declgen_ets2ts ${DECLGEN_ETS2TS_SRC})
panda_target_link_libraries(declgen_ets2ts es2panda-public es2panda-lib arkassembler arkbytecodeopt)
panda_target_include_directories(declgen_ets2ts PRIVATE ${DECLGEN_ETS2TS_INCLUDE_DIRS})
panda_target_compile_options(declgen_ets2ts PRIVATE -fexceptions)

panda_add_sanitizers(TARGET declgen_ets2ts SANITIZERS ${PANDA_SANITIZERS_LIST})
