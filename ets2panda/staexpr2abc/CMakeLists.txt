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

panda_frontend_add_executable(ark_staexpr2abc main.cpp)
panda_target_link_libraries(ark_staexpr2abc es2panda-public es2panda-lib arkassembler arkbytecodeopt)
panda_target_include_directories(ark_staexpr2abc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
panda_target_include_directories(ark_staexpr2abc PRIVATE ${OUTPUT_DIR})
panda_target_include_directories(ark_staexpr2abc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
panda_target_compile_options(ark_staexpr2abc PRIVATE -Werror=shadow)

panda_add_sanitizers(TARGET ark_staexpr2abc SANITIZERS ${PANDA_SANITIZERS_LIST})
