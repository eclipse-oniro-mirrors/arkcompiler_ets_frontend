/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_UTIL_ES2PANDA_MACROS_H
#define ES2PANDA_UTIL_ES2PANDA_MACROS_H

#include "libarkbase/macros.h"
#include "libarkbase/mem/arena_allocator.h"
#include "lexer/token/sourceLocation.h"

#include "libarkbase/mem/arena_allocator.h"

namespace ark::es2panda::parser {
class Program;
}  // namespace ark::es2panda::parser

namespace ark::es2panda {
lexer::SourcePosition GetPositionForDiagnostic();
void CompilerBugAction(const lexer::SourcePosition &pos);
}  // namespace ark::es2panda

#ifndef NDEBUG
// CC-OFFNXT(G.PRE.06) solid logic
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ES2PANDA_ASSERT_POS(cond, position)         \
    if (UNLIKELY(!(cond))) {                        \
        ark::es2panda::CompilerBugAction(position); \
        ASSERT_FAIL(#cond);                         \
    }

// CC-OFFNXT(G.PRE.06) solid logic
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ES2PANDA_ASSERT(cond) ES2PANDA_ASSERT_POS(cond, GetPositionForDiagnostic())

// CC-OFFNXT(G.PRE.06) solid logic
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ES2PANDA_UNREACHABLE_POS(position)      \
    ark::es2panda::CompilerBugAction(position); \
    UNREACHABLE()

// CC-OFFNXT(G.PRE.06, G.PRE.09) solid logic
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ES2PANDA_UNREACHABLE() ES2PANDA_UNREACHABLE_POS(GetPositionForDiagnostic())

#else  // NDEBUG
// CC-OFFNXT(G.PRE.06) solid logic
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ES2PANDA_ASSERT_POS(cond, position) static_cast<void>(0)
// CC-OFFNXT(G.PRE.06) solid logic
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ES2PANDA_ASSERT(cond) static_cast<void>(0)
// CC-OFFNXT(G.PRE.06) solid logic
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ES2PANDA_UNREACHABLE_POS(position) __builtin_unreachable()
// CC-OFFNXT(G.PRE.06) solid logic
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ES2PANDA_UNREACHABLE() __builtin_unreachable()
#endif

#endif  // ES2PANDA_UTIL_ES2PANDA_MACROS_H
