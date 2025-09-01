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

set(DEP_ANALYZER_SRC
  dep_analyzer.cpp
)

panda_frontend_add_executable(dependency_analyzer main.cpp)

panda_frontend_add_library(dependency_analyzer_lib ${PANDA_DEFAULT_LIB_TYPE}  ${DEP_ANALYZER_SRC})

panda_target_link_libraries(dependency_analyzer_lib es2panda-public es2panda-lib arkassembler)
panda_target_include_directories(dependency_analyzer_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
panda_target_include_directories(dependency_analyzer_lib PRIVATE ${OUTPUT_DIR})
panda_target_include_directories(dependency_analyzer_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..)

panda_target_link_libraries(dependency_analyzer dependency_analyzer_lib es2panda-public es2panda-lib arkassembler)
panda_target_include_directories(dependency_analyzer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
panda_target_include_directories(dependency_analyzer PRIVATE ${OUTPUT_DIR})
panda_target_include_directories(dependency_analyzer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..)

if (PANDA_FUZZILLI)
    panda_target_compile_options(dependency_analyzer
        PRIVATE -fPIC
    )
endif()

panda_target_compile_options(dependency_analyzer
  PRIVATE -Werror=shadow
)

panda_add_sanitizers(TARGET dependency_analyzer SANITIZERS ${PANDA_SANITIZERS_LIST})