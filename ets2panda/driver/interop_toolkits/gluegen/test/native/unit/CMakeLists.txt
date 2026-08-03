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

# Googletest-based unit tests for gluegen's native (C++) sources, following
# the same `panda_add_gtest` pattern used elsewhere in the repo for
# standalone tools (e.g. runtime_core/static_core/libziparchive/CMakeLists.txt),
# rather than `ets2panda_add_gtest` (which bundles the full es2panda compiler
# test utilities and is meant for testing es2panda itself, not driver tools).
if(NOT PANDA_WITH_TESTS)
    return()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

set(GLUEGEN_NATIVE_DIR ${GLUEGEN_ROOT_DIR}/native)

# `gluegen` is built as a single executable with no separate `gluegen_lib`
# target (see native/CMakeLists.txt), so its sources are recompiled directly
# into the test binary here instead of being linked from a shared library.
# main.cpp is intentionally excluded (it defines `main()`; gtest's own
# launcher main is supplied by `panda_add_gtest`).
set(GLUEGEN_UNIT_TEST_LIB_SRC
    ${GLUEGEN_NATIVE_DIR}/src/cli.cpp
    ${GLUEGEN_NATIVE_DIR}/src/diagnostic.cpp
    ${GLUEGEN_NATIVE_DIR}/src/gluec.cpp
    ${GLUEGEN_NATIVE_DIR}/src/gluel.cpp
    ${GLUEGEN_NATIVE_DIR}/src/gluegen.cpp
    ${GLUEGEN_NATIVE_DIR}/src/symbol.cpp
    ${GLUEGEN_NATIVE_DIR}/src/utils.cpp
)

# Add new test source files to this list as they're added to this directory.
set(GLUEGEN_UNIT_TEST_SRC
    utils_test.cpp
    symbol_test.cpp
    gluec_test.cpp
    gluel_test.cpp
    gluegen_test.cpp
    diagnostic_test.cpp
    logger_test.cpp
)

find_package(Threads REQUIRED)

panda_add_gtest(
    NAME gluegen_unit_tests
    SOURCES
        ${GLUEGEN_UNIT_TEST_LIB_SRC}
        ${GLUEGEN_UNIT_TEST_SRC}
    INCLUDE_DIRS
        ${GLUEGEN_NATIVE_DIR}/include
        ${OUTPUT_DIR}
        ${ES2PANDA_ROOT}
    LIBRARIES
        es2panda-public
        es2panda-lib
        arkassembler
        Threads::Threads
    SANITIZERS
        ${PANDA_SANITIZERS_LIST}
)

panda_target_include_directories(gluegen_unit_tests PRIVATE ${PANDA_THIRD_PARTY_SOURCES_DIR}/json/include)
panda_target_compile_options(gluegen_unit_tests PRIVATE -Werror=shadow)
