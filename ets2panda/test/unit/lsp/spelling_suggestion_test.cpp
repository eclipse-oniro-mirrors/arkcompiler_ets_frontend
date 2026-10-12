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

#include "lsp_api_test.h"

#include <gtest/gtest.h>

#include "lsp/include/symbol_reference_index.h"

namespace {

using ark::es2panda::lsp::BuildSymbolReferenceIndexForContext;
using ark::es2panda::lsp::ClearSymbolReferenceIndex;
using ark::es2panda::lsp::FindSimilarSymbolNames;
using ark::es2panda::lsp::GetSpellingSuggestion;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::JaccardSimilarity;
using ark::es2panda::lsp::LevenshteinDistance;

class LspSpellingSuggestionTests : public LSPAPITests {
public:
    void SetUp() override
    {
        LSPAPITests::SetUp();
        ClearSymbolReferenceIndex();
    }

    void TearDown() override
    {
        ClearSymbolReferenceIndex();
        LSPAPITests::TearDown();
    }
};

TEST_F(LspSpellingSuggestionTests, JaccardSimilarity_IdenticalStrings)
{
    EXPECT_DOUBLE_EQ(JaccardSimilarity("abc", "abc"), 1.0);
}

TEST_F(LspSpellingSuggestionTests, JaccardSimilarity_NoCommonChars)
{
    EXPECT_DOUBLE_EQ(JaccardSimilarity("abc", "xyz"), 0.0);
}

TEST_F(LspSpellingSuggestionTests, JaccardSimilarity_PartialOverlap)
{
    const double partialOverlapSimilarity = 0.5;
    EXPECT_DOUBLE_EQ(JaccardSimilarity("abc", "abd"), partialOverlapSimilarity);
}

TEST_F(LspSpellingSuggestionTests, JaccardSimilarity_EmptyStrings)
{
    EXPECT_DOUBLE_EQ(JaccardSimilarity("", ""), 0.0);
}

TEST_F(LspSpellingSuggestionTests, GetSpellingSuggestion_FindsClosest)
{
    const std::vector<std::string> candidates = {"length", "lens", "legend"};

    EXPECT_EQ(GetSpellingSuggestion("lengt", candidates), "length");
}

TEST_F(LspSpellingSuggestionTests, GetSpellingSuggestion_NoCloseMatch)
{
    const std::vector<std::string> candidates = {"cat", "dog", "bird"};

    EXPECT_TRUE(GetSpellingSuggestion("xyz", candidates).empty());
}

// Accept-best path for normal-length names; EARLIER symbol_reference_index_test_2.cpp
// SpellingSuggestionSkipsShortNamesWithoutCaseMatch covers the short-name skip path.
// LevenshteinDistance_IdenticalStrings was removed as a subset of LevenshteinDistanceEmptyOperandEdges.
TEST_F(LspSpellingSuggestionTests, FindSimilarSymbolNames_ReturnsSimilar)
{
    const auto paths = CreateTempFile({"spelling_symbols.ets"}, {R"(
class length {}
class lens {}
class legend {}
)"});
    ASSERT_EQ(paths.size(), 1U);

    Initializer initializer;
    auto *context = initializer.CreateContext(paths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(BuildSymbolReferenceIndexForContext(context));

    const auto result = FindSimilarSymbolNames("lengt", paths[0]);
    ASSERT_FALSE(result.empty());
    EXPECT_EQ(result.front(), "length");

    initializer.DestroyContext(context);
}

TEST_F(LspSpellingSuggestionTests, FindSimilarSymbolNames_EmptyIndex)
{
    EXPECT_TRUE(FindSimilarSymbolNames("lengt", "missing.ets").empty());
}

}  // namespace
