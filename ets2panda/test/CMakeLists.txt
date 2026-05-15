# Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

# target to test compilation issue on arm32 platforms
# see issue https://gitee.com/openharmony-sig/arkcompiler_ets_frontend/issues/I84VFY
if (PANDA_TARGET_ARM32)
  add_custom_target(ets_tests_arm32_punctuator_any_equal
      COMMAND ${PANDA_ROOT}/tests/tests-u-runner/runner.sh
              --build-dir ${PANDA_BINARY_ROOT}
              --arm32-qemu
              --ets-runtime
              --test-file PunctuatorAnyEqual.ets
  )
endif()

# ignore ARM32 because some files (not gnueabi or gnueabihf) are incompatible with 32x processors
# this will be fixed in the future
if(PANDA_TARGET_ARM32 OR PANDA_ARM64_TESTS_WITH_SANITIZER)
    return()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

add_custom_target(es2panda_tests COMMENT "Running es2panda test suites")
add_custom_target(es2panda_gtests COMMENT "Running es2panda gtest suites")
set(DEFAULT_TEST_GROUP ets_tests)

# NOTE: These URunner targets are for local developer workflows only.
# Do not add them under umbrella `tests` or CI-oriented target chains.
add_urunner_target(ur_ets_runtime panda-int ets-runtime)
add_urunner_target(ur_ets_astchecker es2panda-verifier astchecker)
add_urunner_target(ur_ets_func panda-int ets-func-tests)
add_urunner_target(ur_ets_cts_70 panda-int ets-cts-7.0)
add_urunner_target(ur_ets_cts panda-int ets-cts)

set(URUNNER_BUILD_WRAPPER "${PANDA_BINARY_ROOT}/runner.sh")
add_custom_target(self_check_frontend
    COMMAND ${CMAKE_COMMAND} -E chdir "${PANDA_BINARY_ROOT}"
            "${URUNNER_BUILD_WRAPPER}" panda-int ets-runtime
    COMMAND ${CMAKE_COMMAND} -E chdir "${PANDA_BINARY_ROOT}"
            "${URUNNER_BUILD_WRAPPER}" panda-int ets-func-tests
    COMMAND ${CMAKE_COMMAND} -E chdir "${PANDA_BINARY_ROOT}"
            "${URUNNER_BUILD_WRAPPER}" es2panda-verifier astchecker
    COMMAND ${CMAKE_COMMAND} -E chdir "${PANDA_BINARY_ROOT}"
            "${URUNNER_BUILD_WRAPPER}" es2panda-verifier parser
    COMMAND ${CMAKE_COMMAND} -E chdir "${PANDA_BINARY_ROOT}"
            "${URUNNER_BUILD_WRAPPER}" panda-int ets-cts-7.0
    COMMAND ${CMAKE_COMMAND} -E chdir "${PANDA_BINARY_ROOT}"
            "${URUNNER_BUILD_WRAPPER}" panda-int ets-cts
    COMMENT "Running frontend local URunner self-check suites sequentially"
    USES_TERMINAL
)

add_dependencies(self_check_frontend code-style-check clang-tidy-changed-check)
add_dependencies(self_check_frontend es2panda ark verifier)

# List for accumulation of all ets gtests binary paths.
# It's used by CI to archive these binaries into a single artifact
# and send it to second stage where unit tests will use them.
set_property(GLOBAL PROPERTY ets_stash_list "")

function(ets2panda_add_gtest TARGET)
    # Parse arguments
    cmake_parse_arguments(
            ARG
            ""
            ""
            "CPP_SOURCES"
            ${ARGN}
    )

    panda_add_gtest(
            TEST_GROUP es2panda_gtests
            STASH_LIST ets_stash_list
            NAME ${TARGET}
            SOURCES
                ${ES2PANDA_ROOT}/test/utils/asm_test.cpp
                ${ES2PANDA_ROOT}/test/utils/ast_verifier_test.cpp
                ${ES2PANDA_ROOT}/test/utils/metadata_test.cpp
                ${ES2PANDA_ROOT}/test/utils/checker_test.cpp
                ${ES2PANDA_ROOT}/test/utils/plugin_conversion_rule_test.cpp
                ${ES2PANDA_ROOT}/test/utils/panda_executable_path_getter.cpp
                ${ARG_CPP_SOURCES}
            LIBRARIES
                es2panda-public
                es2panda-lib
                arkassembler
                arkbytecodeopt
                abc2program
                ${LSP_LIB}
            INCLUDE_DIRS
                ${ES2PANDA_PATH}
                ${ES2PANDA_BINARY_ROOT}
            SANITIZERS
                ${PANDA_SANITIZERS_LIST}
            DEPS_TARGETS
                es2panda
            ${ARG_UNPARSED_ARGUMENTS}
    )
    panda_target_compile_definitions(${TARGET} PRIVATE
        ES2PANDA_BIN_PATH=\"\$<TARGET_FILE:es2panda>\"
    )
endfunction(ets2panda_add_gtest)


if(PANDA_WITH_ETS)
    if (NOT (PANDA_ENABLE_ADDRESS_SANITIZER OR PANDA_ENABLE_THREAD_SANITIZER) OR
        NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR
        PANDA_CI_TESTING_MODE STREQUAL "Nightly")
            set(PANDA_REGRESSION_TESTS 1)
    endif()
endif()

add_subdirectory(tsconfig)
add_subdirectory(options)
add_subdirectory(unit)
add_subdirectory(depanalyzer)
add_subdirectory(arkts_bindings)
add_subdirectory(metadata)

set(FLATBUFFERS_CXX_FLAGS "-Wno-error")
set(FLATBUFFERS_DIR "${PANDA_THIRD_PARTY_SOURCES_DIR}/flatbuffers")
set(FLATBUFFERS_BIN_DIR "${CMAKE_BINARY_DIR}/third_party/flatbuffers")

panda_target_include_directories(es2panda_metadata_test SYSTEM PRIVATE ${FLATBUFFERS_DIR}/include/ PRIVATE ${FLATBUFFERS_BIN_DIR}/generated/)
