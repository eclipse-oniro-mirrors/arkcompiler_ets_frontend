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

if(NOT PANDA_REGRESSION_TESTS OR NOT PANDA_WITH_ETS)
    return()
endif()

set(GEN_INCLUDE_DIR "${PANDA_BINARY_ROOT}/runtime/include")

add_custom_command(OUTPUT ${GEN_INCLUDE_DIR}/path_getter.h
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/path_getter.rb
            ${CMAKE_CURRENT_SOURCE_DIR}/path_getter.h.erb
            ${GEN_INCLUDE_DIR}/path_getter.h
            "${PANDA_BINARY_ROOT}/bin-gtests/es2panda_depanalyz_tests" "${CMAKE_CURRENT_SOURCE_DIR}/test"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/path_getter.h.erb)

add_custom_target(path_getter
    DEPENDS ${GEN_INCLUDE_DIR}/path_getter.h)
add_dependencies(panda_gen_files path_getter)

panda_add_gtest(
        TEST_GROUP es2panda_gtests
        STASH_LIST ets_stash_list
        NAME es2panda_depanalyz_tests
        SOURCES
            test.cpp
        LIBRARIES
            es2panda-public
            es2panda-lib
            arkassembler
            dependency_analyzer_lib
        INCLUDE_DIRS
            ${ES2PANDA_PATH}
            ${ES2PANDA_BINARY_ROOT}
            ${GEN_INCLUDE_DIR}
        SANITIZERS
            ${PANDA_SANITIZERS_LIST}
        DEPS_TARGETS
            etsstdlib
)

add_dependencies(es2panda_depanalyz_tests path_getter)
