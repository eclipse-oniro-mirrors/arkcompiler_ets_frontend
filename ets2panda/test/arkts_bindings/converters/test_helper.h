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

#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include "converters-ani.h"

#include "ani.h"

#include <iostream>
#include <stdexcept>

template <typename... Args>
inline void LOG_ERROR(Args &&...args)
{
    (std::cerr << ... << args);
    std::cerr << "\n";
}

template <typename Type1, typename Type2>
inline void PrintAssertMessage(const char *message, Type1 val1, Type2 val2, const char *op)
{
    LOG_ERROR(message, '\'', val1, '\'', op, '\'', val2, '\'');
}

template <>
inline void PrintAssertMessage<int8_t, int8_t>(const char *message, int8_t val1, int8_t val2, const char *op)
{
    LOG_ERROR(message, '\'', static_cast<int>(val1), '\'', op, '\'', static_cast<int>(val2), '\'');
}

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

// CC-OFFNXT(G.PRE.02-CPP) error handling
#define ASSERT_PRINT(message, val1, val2, op)                     \
    LOG_ERROR("\nAssertion failed at ", __FILE__, ':', __LINE__); \
    PrintAssertMessage(message, val1, val2, op);                  \
    ThrowEtsError("", "std.core.AssertionError")

// CC-OFFNXT(G.PRE.02-CPP) error handling
#define ASSERT_IMPL(condition, message, val1, val2, op) \
    if (!(condition)) {                                 \
        ASSERT_PRINT(message, val1, val2, op);          \
        /* CC-OFFNXT(G.PRE.05) error handling */        \
        return;                                         \
    }

// CC-OFFNXT(G.PRE.02-CPP) error handling
#define ASSERT_IMPL_DEFAULT(condition, message, val1, val2, op, result) \
    if (!(condition)) {                                                 \
        ASSERT_PRINT(message, val1, val2, op);                          \
        /* CC-OFFNXT(G.PRE.05) error handling */                        \
        return result;                                                  \
    }

// CC-OFFNXT(G.PRE.02-CPP) error handling
#define ASSERT_EQ(val1, val2) ASSERT_IMPL((val1) == (val2), "Expected equality failed: ", val1, val2, " == ")

// CC-OFFNXT(G.PRE.02-CPP) error handling
#define ASSERT_EQ_DEFAULT(val1, val2, result) \
    ASSERT_IMPL_DEFAULT((val1) == (val2), "Expected equality failed: ", val1, val2, " == ", result)

// CC-OFFNXT(G.PRE.02-CPP) error handling
#define ASSERT_STREQ(val1, val2) \
    ASSERT_IMPL(std::strcmp(val1, val2) == 0, "Expected equality failed: ", val1, val2, " == ")

// CC-OFFNXT(G.PRE.02-CPP) error handling
#define ASSERT_STREQ_DEFAULT(val1, val2, result) \
    ASSERT_IMPL(std::strcmp(val1, val2) == 0, "Expected equality failed: ", val1, val2, " == ", result)

// CC-OFFNXT(G.PRE.02-CPP) error handling
#define ASSERT_LT(val1, val2) ASSERT_IMPL((val1) < (val2), "Expected non-equality failed: ", val1, val2, " < ")

// CC-OFFNXT(G.PRE.02-CPP) error handling
#define ASSERT_LT_DEFAULT(val1, val2, result) \
    ASSERT_IMPL_DEFAULT((val1) < (val2), "Expected non-equality failed: ", val1, val2, " < ", result)

// NOLINTEND(cppcoreguidelines-macro-usage)

#endif  // TEST_HELPER_H
