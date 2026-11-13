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

# NOTE: every case here drives an in-process ES2PANDA_STATE_CHECKED context. The checker is
# arena-allocated and never destructed, so its std containers leak (#30057) and leak sanitizer
# fails the run. Excluded from asan builds like the other CHECKED-state suites.
if (PANDA_ENABLE_ADDRESS_SANITIZER)
    return()
endif()

ets2panda_add_gtest(es2panda_strict_initialization_tests
    CPP_SOURCES strict_initialization_test.cpp
)
