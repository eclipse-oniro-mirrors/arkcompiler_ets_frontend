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

# 32-bits pointers optimization in Panda for objects => addresses usage low 4GB
# We need use linker scripts for section replacement above 4GB
# Note: AddressSanitizer reserves (mmap) addresses for its own needs,
#       so we have difference start addresses for asan and default buildings

function(panda_frontend_add_executable target)
    if(PANDA_FRONTEND_BUILD)
        set(ARGV "${ARGV};FORCE_REBUILD")
    endif()
    panda_add_executable(${ARGV})
endfunction()

function(panda_frontend_add_library target)
    if(PANDA_FRONTEND_BUILD)
        set(ARGV "${ARGV};FORCE_REBUILD")
    endif()
    panda_add_library(${ARGV})
endfunction()
