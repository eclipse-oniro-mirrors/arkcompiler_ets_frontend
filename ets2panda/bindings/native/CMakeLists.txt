# Copyright (c) 2025 Huawei Device Co., Ltd.
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


function(panda_add_node_library TARGET)
    # Parse arguments
    cmake_parse_arguments(
        ARG
        ""
        "LIBRARY_OUTPUT_DIRECTORY"
        "SOURCES;LIBRARIES;OUTPUT_SUFFIX"
        ${ARGN}
    )

    if(NOT DEFINED ARG_LIBRARY_OUTPUT_DIRECTORY)
        # Set default value
        set(ARG_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/bindings)
    endif()

    panda_frontend_add_library(${TARGET} SHARED ${ARG_SOURCES})
    panda_target_link_libraries(${TARGET} ${ARG_LIBRARIES})

    if(PANDA_TARGET_OHOS)
        panda_target_link_libraries(${TARGET} ace_napi.z  -Wl,--no-undefined)
    else()
        panda_target_include_directories(${TARGET} SYSTEM PRIVATE ${NAPI_HEADERS_PATH})
        set_target_properties(${TARGET}
            PROPERTIES
                # Set module name
                PREFIX ""
                OUTPUT_NAME "${TARGET}"
                SUFFIX ${ARG_OUTPUT_SUFFIX}

                # Set output directory
                LIBRARY_OUTPUT_DIRECTORY ${ARG_LIBRARY_OUTPUT_DIRECTORY}
        )
    endif()
endfunction()

# install node
set(NODE_VERSION "v18.13.0")
set(DISTRO "linux-x64")

execute_process(COMMAND ${PANDA_ROOT}/scripts/install-third-party --node
        WORKING_DIRECTORY ${PANDA_ROOT}
        RESULT_VARIABLE NODEJS_DOWNLOAD_OK)
if (NOT NODEJS_DOWNLOAD_OK EQUAL 0)
    message(FATAL_ERROR "Unable to install required nodejs dependencies")
endif()

set(NODE_THIRD_PARTY_SOURCES_DIR ${PANDA_THIRD_PARTY_SOURCES_DIR}/nodejs)
set(NAPI_HEADERS_PATH ${NODE_THIRD_PARTY_SOURCES_DIR}/node-${NODE_VERSION}-${DISTRO}/include/node)
set(NODE_BINARY ${PANDA_ROOT}/third_party/nodejs/node-${NODE_VERSION}-${DISTRO}/bin/node)

set(NAPI_BINDINGS_LIB "ts_bindings")

set(BINDINGS_NAPI_SRC
  ./src/common-interop.cpp
  ./src/convertors-napi.cpp
  ./src/callback-resource.cpp
  ./src/lsp.cpp
)

# PUBLIC arkbase es2panda-lib es2panda-public
# PRIVATE arkbytecodeopt
set(ES2PANDA_LIBS
  es2panda-lib
  es2panda-public
  es2panda_lsp
)

set(TS_INTEROP_MODULE 
  "-DTS_INTEROP_MODULE=NativeModule"
  "-DTS_USE_NODE_VM"
  "-DINTEROP_LIBRARY_NAME=ts_bindings"
  "-DTS_NAPI"
)

if(PANDA_TARGET_WINDOWS)
  set(NODE_API_LIB libnode_api.a)
  set(NODE_WIN_API_LIB_PATH ${NODE_THIRD_PARTY_SOURCES_DIR}/node-${NODE_VERSION}-${DISTRO}/lib/${NODE_API_LIB})
  list(APPEND ES2PANDA_LIBS ${NODE_WIN_API_LIB_PATH})
  list(APPEND BINDINGS_NAPI_SRC ./src/win-dynamic-node.cpp)
endif()

panda_add_node_library(${NAPI_BINDINGS_LIB} 
  SOURCES ${BINDINGS_NAPI_SRC}
  LIBRARIES ${ES2PANDA_LIBS}
  OUTPUT_SUFFIX ".node"
)

add_dependencies(frontend_bins ${NAPI_BINDINGS_LIB})

panda_target_include_directories(${NAPI_BINDINGS_LIB}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
  SYSTEM PRIVATE ${NAPI_HEADERS_PATH}
  PRIVATE ${ES2PANDA_ROOT}
  PRIVATE ${OUTPUT_DIR}
)

panda_target_compile_definitions(${NAPI_BINDINGS_LIB} PRIVATE ${TS_INTEROP_MODULE})
panda_target_compile_options(${NAPI_BINDINGS_LIB}
  PRIVATE -fexceptions -Werror=shadow
)

panda_add_sanitizers(TARGET ${NAPI_BINDINGS_LIB} 
  SANITIZERS ${PANDA_SANITIZERS_LIST}
)
