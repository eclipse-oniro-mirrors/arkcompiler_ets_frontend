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

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "lsp/include/navigate_to.h"
#include "test/unit/lsp/lsp_api_test.h"

namespace {
using ark::es2panda::SourceFile;
using ark::es2panda::lsp::GetNavigateToItems;
using ark::es2panda::lsp::Initializer;

class NavigateToEdgeTests : public LSPAPITests {};

// LSP_COVERAGE_V5 P3 tail batch - navigate_to.cpp micro-arms.
// NOTE: GetNavigateToItems keeps a function-static per-search-term budget
// (navigate_to.cpp:153-154).  Each binary owns its own copy of that map, so
// every test below uses its own unique search term and never depends on terms
// used by other suites.

// NT-EDGE-01: invalid regex pattern arms (navigate_to.cpp:27-31): std::regex
// throws while constructing the case-sensitive and icase variants, the catch
// stores nullopt, and GetNavigateToItems early-returns empty via the
// !IsPatternValid() arm of navigate_to.cpp:160-162.  Exact-behavior pin: no
// crash, empty result, budget untouched.
TEST_F(NavigateToEdgeTests, InvalidRegexPatternReturnsEmpty)
{
    Initializer initializer;
    const char *source = "function edgeProbe() {\n    let a = 1;\n    return a;\n}\n";
    es2panda_Context *ctx = initializer.CreateContext("edge_navigate_probe.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_NE(ctx, nullptr);

    std::vector<SourceFile> files = {{"edge_navigate_probe.ets", source}};

    // Case-sensitive invalid regex: unmatched parenthesis.
    auto resSensitive = GetNavigateToItems(ctx, files, 10U, "(", true);
    ASSERT_TRUE(resSensitive.empty());
    // Case-insensitive invalid regex: unmatched bracket.
    auto resInsensitive = GetNavigateToItems(ctx, files, 10U, "[", false);
    ASSERT_TRUE(resInsensitive.empty());

    initializer.DestroyContext(ctx);
}

// NT-EDGE-02: maxResultCount exhaustion mid-iteration
// (navigate_to.cpp:166-174): with two distinct matches and a limit of 1, the
// first emit drives TryEmitItem's `remaining` to 0 so it returns false
// (:145-146), GetNavigateToItems takes the !TryEmitItem branch at :169,
// pins the static budget with `emittedSoFar = maxResultCount` (:170) and
// returns early (:171).  The follow-up call with the same term then takes the
// `emittedSoFar >= maxResultCount` arm of :160-162 and returns nothing:
// the per-term static budget (navigate_to.cpp:153-154) survives across calls.
TEST_F(NavigateToEdgeTests, LimitExhaustionMidIterationThenBudgetBlocksRepeatCall)
{
    const char *source =
        "function limitprobe() {\n    let a = 1;\n    return a;\n}\n"
        "function limitprobe2() {\n    let b = 2;\n    return b;\n}\n";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("edge_navigate_limit.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_NE(ctx, nullptr);

    std::vector<SourceFile> files = {{"edge_navigate_limit.ets", source}};

    auto firstCall = GetNavigateToItems(ctx, files, 1U, "limitprobe", false);
    ASSERT_EQ(firstCall.size(), 1U);
    ASSERT_EQ(firstCall[0].name, "limitprobe");
    ASSERT_EQ(firstCall[0].matchKind, ark::es2panda::lsp::MatchKind::EXACT);

    auto secondCall = GetNavigateToItems(ctx, files, 1U, "limitprobe", false);
    ASSERT_TRUE(secondCall.empty());

    initializer.DestroyContext(ctx);
}

// NT-EDGE-03: the static budget accumulates across repeat calls even when the
// first call did not exhaust its own limit (navigate_to.cpp:153-154,
// :160-164, :176): call 1 with limit 5 emits both matches and stores
// emittedSoFar = 2; call 2 with limit 3 gets remaining = 1, emits only the
// first match, then exhausts mid-iteration - the second call is capped below
// its own maxResultCount by the leftover budget from the first call.
TEST_F(NavigateToEdgeTests, RepeatCallBudgetAccumulatesAcrossCalls)
{
    const char *source =
        "function budgetprobe() {\n    let a = 1;\n    return a;\n}\n"
        "function budgetprobe2() {\n    let b = 2;\n    return b;\n}\n";
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext("edge_navigate_budget.ets", ES2PANDA_STATE_CHECKED, source);
    ASSERT_NE(ctx, nullptr);

    std::vector<SourceFile> files = {{"edge_navigate_budget.ets", source}};

    auto firstCall = GetNavigateToItems(ctx, files, 5U, "budgetprobe", false);
    ASSERT_EQ(firstCall.size(), 2U);
    ASSERT_EQ(firstCall[0].name, "budgetprobe");
    ASSERT_EQ(firstCall[0].matchKind, ark::es2panda::lsp::MatchKind::EXACT);
    ASSERT_EQ(firstCall[1].name, "budgetprobe2");
    ASSERT_EQ(firstCall[1].matchKind, ark::es2panda::lsp::MatchKind::PREFIX);

    auto secondCall = GetNavigateToItems(ctx, files, 3U, "budgetprobe", false);
    ASSERT_EQ(secondCall.size(), 1U);
    ASSERT_EQ(secondCall[0].name, "budgetprobe");

    initializer.DestroyContext(ctx);
}
}  // namespace
