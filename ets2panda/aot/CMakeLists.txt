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

set(ES2PANDA_AOT_SRC
  main.cpp
)

function(add_es2panda_exec E2P_EXEC_NAME)
  panda_frontend_add_executable(${E2P_EXEC_NAME} ${ES2PANDA_AOT_SRC})
  panda_target_link_libraries(${E2P_EXEC_NAME} es2panda-public es2panda-lib arkassembler arkbytecodeopt)
  panda_target_include_directories(${E2P_EXEC_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  panda_target_include_directories(${E2P_EXEC_NAME} PRIVATE ${OUTPUT_DIR})
  panda_target_include_directories(${E2P_EXEC_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
  if (PANDA_FUZZILLI)
    panda_target_compile_options(${E2P_EXEC_NAME}
        PRIVATE -fPIC
    )
  endif()

  panda_target_compile_options(${E2P_EXEC_NAME}
    PRIVATE -Werror=shadow
  )

  panda_add_sanitizers(TARGET ${E2P_EXEC_NAME} SANITIZERS ${PANDA_SANITIZERS_LIST})
endfunction()

# A bootstrapper to provide reliable stdlib declarations within build tree (in case of stdlib/es2panda modifications).
# This is needed since es2panda actually depends on stdlib. The 'es2panda' dependency on 'etsstdlib' is introduced in 
# plugins/ets/CMakeLists.txt since 'plugins' directory are included after 'tools'.
add_es2panda_exec(es2panda_stdlib_compiler)
add_es2panda_exec(es2panda)
