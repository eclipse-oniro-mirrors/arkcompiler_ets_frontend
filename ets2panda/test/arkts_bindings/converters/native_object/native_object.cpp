/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "converters-ani.h"
#include "test_helper.h"

#include "ani.h"

#include <array>

struct TestPair {
public:
    int a {0};
    int b {0};

    TestPair(int aa, int bb) : a(aa), b(bb) {}
};

static constexpr EtsInt PAIR_TEST_DATA[][2] {{10, 20}, {30, 40}};

EtsNativePointer impl_CreatePair(EtsInt a, EtsInt b)
{
    TestPair *pair = new TestPair(a, b);
    return pair;
}
ETS_INTEROP_2(CreatePair, EtsNativePointer, EtsInt, EtsInt)

void impl_ModifyPair(EtsNativePointer ptr, EtsInt a, EtsInt b)
{
    auto pair = reinterpret_cast<TestPair *>(ptr);
    pair->a = a;
    pair->b = b;
}
ETS_INTEROP_V3(ModifyPair, EtsNativePointer, EtsInt, EtsInt)

void impl_ValidatePair(EtsNativePointer ptr, EtsInt testIdx)
{
    int testDataSize {static_cast<int>(std::size(PAIR_TEST_DATA))};
    ASSERT_LT(testIdx, testDataSize);

    auto pair = reinterpret_cast<TestPair *>(ptr);
    auto expectedValues = PAIR_TEST_DATA[testIdx];
    ASSERT_EQ(pair->a, expectedValues[0]);
    ASSERT_EQ(pair->b, expectedValues[1]);
}
ETS_INTEROP_V2(ValidatePair, EtsNativePointer, EtsInt)

void impl_DestroyPair(EtsNativePointer ptr)
{
    auto pair = reinterpret_cast<TestPair *>(ptr);
    delete pair;
}
ETS_INTEROP_V1(DestroyPair, EtsNativePointer)