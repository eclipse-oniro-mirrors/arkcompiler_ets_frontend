# Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

option(ENABLE_ES2PANDA_COVERAGE "Enable coverage calculation for es2panda" false)

include(${PANDA_ROOT}/cmake/toolchain/coverage/unit_tests_lcov.cmake)

add_custom_target(es2panda_coverage DEPENDS etsstdlib es2panda verifier ark)

if (NOT DEFINED ES2PANDA_PATH)
    file(REAL_PATH ${PANDA_ROOT}/tools/es2panda ES2PANDA_PATH)
endif()

add_custom_command(TARGET es2panda_coverage POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND bash ${ES2PANDA_PATH}/scripts/es2panda_coverage.sh --binary-dir=${PANDA_BINARY_ROOT} --root-dir=${PANDA_ROOT}
)

if(ENABLE_ES2PANDA_COVERAGE)
    collect_coverage_for_target(
        TARGET_NAME es2panda_coverage
        INCLUDE_DIR_PATTERN '*/es2panda/*'
    )
else()
    message(STATUS "Coverage will not be calculated (may be enabled by -DENABLE_ES2PANDA_COVERAGE=true ).")
endif(ENABLE_ES2PANDA_COVERAGE)
