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
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/navigate_to.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::ir::AstNode;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::PatternMatcher;

class LspNavigateToInternalsTests : public LSPAPITests {};

// =============================================================================
// P2E-NAV-01: PatternMatcher_Constructor_ValidPattern
// =============================================================================
TEST_F(LspNavigateToInternalsTests, PatternMatcher_Constructor_ValidPattern)
{
    PatternMatcher matcher("foo", true);
    EXPECT_TRUE(matcher.IsPatternValid());
}

// =============================================================================
// P2E-NAV-02: PatternMatcher_Constructor_InvalidPattern
// =============================================================================
// Unit twin of null_check_false_branch_test.cpp NavigateTo_InvalidPattern_ReturnsEmpty:
// direct IsPatternValid seam vs pipeline-empty seam.
TEST_F(LspNavigateToInternalsTests, PatternMatcher_Constructor_InvalidPattern)
{
    // Empty pattern when case-sensitive: the regex " " matches all strings,
    // so IsPatternValid returns true.  Use a truly invalid regex pattern
    // that causes std::regex to throw (e.g. unmatched bracket).
    PatternMatcher matcher("[invalid", true);
    EXPECT_FALSE(matcher.IsPatternValid());
}

// =============================================================================
// P2E-NAV-03: PatternMatcher_MatchesExact_True
// =============================================================================
// Direct regex seam of navigate_to_test.cpp ExactMatchFromSingleFile;
// adds the case-sensitive no-match negative.
TEST_F(LspNavigateToInternalsTests, PatternMatcher_MatchesExact_True)
{
    PatternMatcher matcher("foo", true);
    EXPECT_TRUE(matcher.MatchesExact("foo"));
    // Case-sensitive: "Foo" should not match "foo"
    EXPECT_FALSE(matcher.MatchesExact("Foo"));
}

// =============================================================================
// P2E-NAV-04: PatternMatcher_MatchesPrefix_True
// =============================================================================
TEST_F(LspNavigateToInternalsTests, PatternMatcher_MatchesPrefix_True)
{
    PatternMatcher matcher("foo", true);
    EXPECT_TRUE(matcher.MatchesPrefix("foobar"));
    // Shorter candidate should not match
    EXPECT_FALSE(matcher.MatchesPrefix("fo"));
    // Different prefix should not match
    EXPECT_FALSE(matcher.MatchesPrefix("barfoo"));
}

// =============================================================================
// P2E-NAV-05: PatternMatcher_MatchesSubstring_True
// =============================================================================
// Direct regex_search seam of navigate_to_test.cpp MultiFileSubstringMatch.
TEST_F(LspNavigateToInternalsTests, PatternMatcher_MatchesSubstring_True)
{
    PatternMatcher matcher("foo", true);
    EXPECT_TRUE(matcher.MatchesSubstring("xfooy"));
    EXPECT_TRUE(matcher.MatchesSubstring("foo"));
    // Non-matching string
    EXPECT_FALSE(matcher.MatchesSubstring("bar"));
}

// =============================================================================
// P2E-NAV-06: GetContainerNodes_ReturnsContainer
// =============================================================================
// BLOCKED_DESIGN_MAPPING
// GetContainerNodes (plural) is declared in navigate_to.h:66 but never defined
// in any .cpp file.  The actual implementation GetContainerNode (singular) is
// in quick_info.cpp:574, declared in quick_info.h:53.  The navigate_to.h
// declaration is stale.  Calling GetContainerNodes would cause a linker error.
// NEXT ACTION: Either remove the stale declaration from navigate_to.h or
// implement GetContainerNodes as a wrapper around GetContainerNode.

// =============================================================================
// P2E-NAV-07: GetItemsFromNamedDeclaration_ReturnsItems
// =============================================================================
// BLOCKED_DESIGN_MAPPING
// navigate_to.h:67 declares GetItemsFromNamedDeclaration with 2 parameters
// (const SourceFile &, const PatternMatcher &).  navigate_to.cpp:113 defines
// it with 3 parameters (es2panda_Context *, const SourceFile &, const PatternMatcher &).
// The header declaration is stale; calling the 2-param version would cause a
// linker error.  NEXT ACTION: Update the header declaration to match the
// implementation.

}  // namespace