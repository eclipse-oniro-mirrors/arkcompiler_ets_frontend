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

add_definitions(-DCASES_DIR=\"${CMAKE_SOURCE_DIR}/tools/es2panda/test/unit/lsp/benchmark/cases\")

# Prefer an existing benchmark target from upper-level CMake to avoid
# configuring third_party/benchmark tests in this subtree.
set(LSP_BENCHMARK_LIB_TARGET "")
if(TARGET benchmark)
    set(LSP_BENCHMARK_LIB_TARGET benchmark)
elseif(TARGET benchmark::benchmark)
    if(NOT TARGET benchmark)
        panda_add_library(benchmark ALIAS benchmark::benchmark)
    endif()
    set(LSP_BENCHMARK_LIB_TARGET benchmark)
else()
    # Fallback: build benchmark locally, but disable its own tests.
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/benchmark
                     ${CMAKE_BINARY_DIR}/third_party/benchmark_build)

    if(TARGET benchmark)
        set(LSP_BENCHMARK_LIB_TARGET benchmark)
    elseif(TARGET benchmark::benchmark)
        if(NOT TARGET benchmark)
            panda_add_library(benchmark ALIAS benchmark::benchmark)
        endif()
        set(LSP_BENCHMARK_LIB_TARGET benchmark)
    else()
        message(FATAL_ERROR "Cannot find target (benchmark or benchmark::benchmark)")
    endif()
endif()

function(lsp_add_benchmark TARGET)
    cmake_parse_arguments(
        ARG
        ""
        ""
        "CPP_SOURCES"
        ${ARGN}
    )

    set(LSP_BENCHMARK_COMMON_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/lspMemoryManager.cpp
    )

    panda_add_executable(${TARGET}
        ${ARG_CPP_SOURCES}
        ${LSP_BENCHMARK_COMMON_SOURCES}
    )

    panda_target_link_libraries(${TARGET} ${LSP_BENCHMARK_LIB_TARGET})

    panda_target_include_directories(${TARGET} PRIVATE
        ${ES2PANDA_PATH}
        ${ES2PANDA_BINARY_ROOT}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../benchmark
    )

    panda_target_link_libraries(${TARGET}
        es2panda-public
        es2panda-lib
        arkassembler
        arkbytecodeopt
        ${LSP_LIB}
    )

endfunction(lsp_add_benchmark)

add_subdirectory(src)
