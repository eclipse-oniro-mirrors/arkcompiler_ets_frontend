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

static constexpr EtsBoolean BOOLEAN_TEST_DATA[] {true, false, true, false, false, true, false, false};
static constexpr EtsByte BYTE_TEST_DATA[] {-128, -127, -35, 0, 1, 27, 100, 127};
static constexpr EtsShort SHORT_TEST_DATA[] {-32768, -15325, -30, 0, 1, 12345, 32767};
static constexpr EtsInt INT_TEST_DATA[] {1,         0,          -1,         82,          -125,      123456,
                                         205452356, -300000000, 1234567890, -2147483648, 2147483647};
static constexpr EtsLong LONG_TEST_DATA[] {0,
                                           -100,
                                           200,
                                           123456789012345,
                                           1'000'000'000'000'000'000,
                                           9'223'372'036'854'775'807,
                                           -9'223'372'036'854'775'807 - 1};
static constexpr EtsFloat FLOAT_TEST_DATA[] {0, 0.0f, 1.0f, -20.1f, 1.56f, 10 / 3.0f, 12345e-3f};
static constexpr EtsDouble DOUBLE_TEST_DATA[] {-10.0, 0.0, 1.2, -300.6125, 2 / 3.0, 100 / 3.0};
static constexpr const char *STRING_TEST_DATA[] {"",
                                                 "  ",
                                                 "1",
                                                 "200",
                                                 "a",
                                                 "b",
                                                 "z",
                                                 "qwerty123",
                                                 "500600123456foobar",
                                                 "   whitespaces at the beginning",
                                                 "whitespaces at the end   ",
                                                 "   whitespaces     everywhere   ",
                                                 "/home/project/src/dir1/dir2/dir3/file.ets"};
static const std::vector<const char *> STRING_ARRAY_TEST_DATA[] = {
    std::vector<const char *> {}, std::vector<const char *> {""}, std::vector<const char *> {"hello world"},
    std::vector<const char *> {"qwe", "rty", "hello"}, std::vector<const char *> {"1", "20", "300", "4000", "50000"}};

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

// CC-OFFNXT(G.PRE.06) code generation, test logic
#define TEST_TYPE_CONVERSION(name, Type, TEST_DATA)                \
    void impl_EtsToNative_##name(Type value, EtsInt testIdx)       \
    {                                                              \
        int testDataSize {static_cast<int>(std::size(TEST_DATA))}; \
        ASSERT_LT(testIdx, testDataSize);                          \
        ASSERT_EQ(value, (TEST_DATA)[testIdx]);                    \
    }                                                              \
    ETS_INTEROP_V2(EtsToNative_##name, Type, EtsInt)               \
                                                                   \
    Type impl_NativeToEts_##name(EtsInt testIdx)                   \
    {                                                              \
        int testDataSize {static_cast<int>(std::size(TEST_DATA))}; \
        ASSERT_LT_DEFAULT(testIdx, testDataSize, false);           \
        /* CC-OFFNXT(G.PRE.05) code generation */                  \
        return (TEST_DATA)[testIdx];                               \
    }                                                              \
    ETS_INTEROP_1(NativeToEts_##name, Type, EtsInt)

// NOLINTEND(cppcoreguidelines-macro-usage)

TEST_TYPE_CONVERSION(Boolean, EtsBoolean, BOOLEAN_TEST_DATA)
TEST_TYPE_CONVERSION(Byte, EtsByte, BYTE_TEST_DATA)
TEST_TYPE_CONVERSION(Short, EtsShort, SHORT_TEST_DATA)
TEST_TYPE_CONVERSION(Int, EtsInt, INT_TEST_DATA)
TEST_TYPE_CONVERSION(Long, EtsLong, LONG_TEST_DATA)
TEST_TYPE_CONVERSION(Float, EtsFloat, FLOAT_TEST_DATA)
TEST_TYPE_CONVERSION(Double, EtsDouble, DOUBLE_TEST_DATA)

void impl_EtsToNative_String(EtsStringPtr &ptr, EtsInt testIdx)
{
    int testDataSize {static_cast<int>(std::size(STRING_TEST_DATA))};
    ASSERT_LT(testIdx, testDataSize);
    ASSERT_STREQ(ptr.Data(), STRING_TEST_DATA[testIdx]);
}
ETS_INTEROP_V2(EtsToNative_String, EtsStringPtr, EtsInt)

EtsStringPtr impl_NativeToEts_String(EtsInt testIdx)
{
    int testDataSize {static_cast<int>(std::size(STRING_TEST_DATA))};
    ASSERT_LT_DEFAULT(testIdx, testDataSize, EtsStringPtr(nullptr));
    return EtsStringPtr(STRING_TEST_DATA[testIdx]);
}
ETS_INTEROP_1(NativeToEts_String, EtsStringPtr, EtsInt)

EtsCString impl_NativeToEts_CString(EtsInt testIdx)
{
    int testDataSize {static_cast<int>(std::size(STRING_TEST_DATA))};
    ASSERT_LT_DEFAULT(testIdx, testDataSize, nullptr);
    return STRING_TEST_DATA[testIdx];
}
ETS_INTEROP_1(NativeToEts_CString, EtsCString, EtsInt)

void impl_EtsToNative_StringArray(EtsStringArray strArr, EtsInt len, EtsInt testIdx)
{
    int testDataSize {static_cast<int>(std::size(STRING_ARRAY_TEST_DATA))};
    ASSERT_LT(testIdx, testDataSize);

    for (int i = 0; i < len; ++i) {
        ASSERT_STREQ(strArr[i], STRING_ARRAY_TEST_DATA[testIdx].at(i));
    }
}
ETS_INTEROP_V3(EtsToNative_StringArray, EtsStringArray, EtsInt, EtsInt)
