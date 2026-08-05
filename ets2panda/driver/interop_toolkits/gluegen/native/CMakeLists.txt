# Copyright (c) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

set(GLUEGEN_NATIVE_SRC
  src/cli.cpp
  src/diagnostic.cpp
  src/gluec.cpp
  src/gluel.cpp
  src/gluegen.cpp
  src/main.cpp
  src/symbol.cpp
  src/utils.cpp
)

find_package(Threads REQUIRED)

panda_frontend_add_executable(gluegen ${GLUEGEN_NATIVE_SRC})

panda_target_link_libraries(gluegen es2panda-public es2panda-lib arkassembler Threads::Threads)
panda_target_include_directories(gluegen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
panda_target_include_directories(gluegen PRIVATE ${PANDA_THIRD_PARTY_SOURCES_DIR}/json/include)
panda_target_include_directories(gluegen PRIVATE ${OUTPUT_DIR})
panda_target_include_directories(gluegen PRIVATE ${ES2PANDA_ROOT})

if (PANDA_FUZZILLI)
    panda_target_compile_options(gluegen
        PRIVATE -fPIC
    )
endif()

panda_target_compile_options(gluegen
  PRIVATE -Werror=shadow
)

panda_add_sanitizers(TARGET gluegen SANITIZERS ${PANDA_SANITIZERS_LIST})

# Copy the built binary next to the npm package (apps/gluegen/bin/) instead of
# installing it anywhere shared, so the TS side can resolve it relative to
# __dirname. Mirrors the DESTINATION_PATH copy step in bindings/native/CMakeLists.txt.
get_target_property(gluegen_is_imported gluegen IMPORTED)
if(NOT gluegen_is_imported)
  set(GLUEGEN_NATIVE_DESTINATION_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../bin)
  add_custom_command(TARGET gluegen POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GLUEGEN_NATIVE_DESTINATION_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:gluegen> ${GLUEGEN_NATIVE_DESTINATION_DIR}/
  )
endif()
