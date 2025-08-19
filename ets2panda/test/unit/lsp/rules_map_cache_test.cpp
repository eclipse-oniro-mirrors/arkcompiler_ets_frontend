/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "lsp/include/formatting/rule.h"
#include "lsp/include/formatting/rules_map.h"
#include "lsp/include/formatting/formatting_context.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>

namespace {

class LSPRulesMapCacheTests : public LSPAPITests {
protected:
    using RulesMapCache = ark::es2panda::lsp::RulesMapCache;
    using RuleAction = ark::es2panda::lsp::RuleAction;
    using Rule = ark::es2panda::lsp::Rule;
    using RuleSpec = ark::es2panda::lsp::RuleSpec;
    using RuleFlags = ark::es2panda::lsp::RuleFlags;
    using Token = ark::es2panda::lexer::Token;
    using TokenType = ark::es2panda::lexer::TokenType;
    using TokenRange = ark::es2panda::lsp::TokenRange;
    using FormattingContext = ark::es2panda::lsp::FormattingContext;

    FormattingContext MakeContext(TokenType current, TokenType next)
    {
        FormattingContext ctx("");
        Token currTok;
        Token nextTok;
        currTok.SetTokenType(current);
        nextTok.SetTokenType(next);
        ctx.SetCurrentToken(currTok);
        ctx.SetNextToken(nextTok);
        return ctx;
    }
    TokenRange Single(TokenType t, bool specific = true)
    {
        return TokenRange {std::vector<TokenType> {t}, specific};
    }
};

TEST_F(LSPRulesMapCacheTests, GetInsertionIndex_ZeroBitmap)
{
    const uint32_t bm0 = 0;
    const uint32_t bm5 = 5;
    const uint32_t bm25 = 10;
    EXPECT_EQ(RulesMapCache::GetInsertionIndex(bm0, bm0), bm0);
    EXPECT_EQ(RulesMapCache::GetInsertionIndex(bm0, bm5), bm0);
    EXPECT_EQ(RulesMapCache::GetInsertionIndex(bm0, bm25), bm0);
}

TEST_F(LSPRulesMapCacheTests, GetInsertionIndex_WithCounts)
{
    const uint32_t bm0 = 0;
    const uint32_t bm2 = 2;
    const uint32_t bm5 = 5;
    const uint32_t bm9 = 9;
    const uint32_t bm10 = 10;
    const uint32_t maskBit0 = 0;
    const uint32_t maskBit2 = 2;
    const uint32_t maskBit3 = 3;
    const uint32_t maskBit4 = 4;
    const uint32_t maskBit5 = 5;
    const uint32_t maskBit10 = 10;
    int bitmap = (maskBit2 << maskBit0) | (maskBit3 << maskBit5) | (maskBit4 << maskBit10);

    EXPECT_EQ(RulesMapCache::GetInsertionIndex(bitmap, bm0), bm2);
    EXPECT_EQ(RulesMapCache::GetInsertionIndex(bitmap, bm5), bm5);
    EXPECT_EQ(RulesMapCache::GetInsertionIndex(bitmap, bm10), bm9);
}

TEST_F(LSPRulesMapCacheTests, IncreaseInsertionIndex_EmptyBitmap)
{
    const unsigned int bmp0 = 0;
    const unsigned int bmp1 = 1;
    const unsigned int bm5 = 5;
    const unsigned int bm10 = 10;
    unsigned int bm0 = RulesMapCache::IncreaseInsertionIndex(bmp0, bmp0);
    EXPECT_EQ(bm0 & 0b11111U, bmp1);

    unsigned int bm1 = RulesMapCache::IncreaseInsertionIndex(bmp0, bm5);
    EXPECT_EQ((bm1 >> bm5) & 0b11111U, bmp1);

    unsigned int bm2 = RulesMapCache::IncreaseInsertionIndex(bmp0, bm10);
    EXPECT_EQ((bm2 >> bm10) & 0b11111U, bmp1);
}

TEST_F(LSPRulesMapCacheTests, IncreaseInsertionIndex_NonZeroBitmap)
{
    const unsigned int bm0 = 0;
    const unsigned int bm3 = 3;
    const unsigned int bm10 = 10;
    const unsigned int maskBit1 = 1;
    const unsigned int maskBit2 = 2;
    const unsigned int maskBit10 = 10;
    unsigned int bitmap = (maskBit2 << maskBit10);
    unsigned int updated = RulesMapCache::IncreaseInsertionIndex(bitmap, bm10);

    EXPECT_EQ((updated >> bm10) & 0b11111U, bm3);
    EXPECT_EQ((updated & ((maskBit1 << bm10) - maskBit1)), bm0);
}

TEST_F(LSPRulesMapCacheTests, GetRuleActionExclusion_StopSpace)
{
    RuleAction mask = RulesMapCache::GetRuleActionExclusion(RuleAction::STOP_PROCESSING_SPACE_ACTIONS);

    EXPECT_NE(mask & RuleAction::INSERT_SPACE, RuleAction::NONE);
    EXPECT_NE(mask & RuleAction::INSERT_NEWLINE, RuleAction::NONE);
    EXPECT_NE(mask & RuleAction::DELETE_SPACE, RuleAction::NONE);
}

TEST_F(LSPRulesMapCacheTests, GetRuleActionExclusion_StopToken)
{
    RuleAction mask = RulesMapCache::GetRuleActionExclusion(RuleAction::STOP_PROCESSING_TOKEN_ACTIONS);

    EXPECT_NE(mask & RuleAction::DELETE_TOKEN, RuleAction::NONE);
    EXPECT_NE(mask & RuleAction::INSERT_TRAILING_SEMICOLON, RuleAction::NONE);
}

TEST_F(LSPRulesMapCacheTests, GetRuleBucketIndex_Calculation)
{
    int mapRowLength = static_cast<int>(ark::es2panda::lsp::LAST_TOKEN) + 1;
    int leftVal = static_cast<int>(TokenType::KEYW_LET);
    int rightVal = static_cast<int>(TokenType::PUNCTUATOR_EQUAL);

    int expected = leftVal * mapRowLength + rightVal;
    int actual = RulesMapCache::GetRuleBucketIndex(TokenType::KEYW_LET, TokenType::PUNCTUATOR_EQUAL);
    EXPECT_EQ(actual, expected);
}

TEST_F(LSPRulesMapCacheTests, AddRule_InsertAndBitmapUpdate)
{
    auto alwaysTrue = [](ark::es2panda::lsp::FormattingContext *) { return true; };
    Rule ruleA({alwaysTrue}, RuleAction::STOP_PROCESSING_SPACE_ACTIONS, {});
    const unsigned int bm1 = 1;
    const unsigned int bm5 = 5;
    TokenRange lr = Single(TokenType::KEYW_LET);
    TokenRange rr = Single(TokenType::PUNCTUATOR_EQUAL);
    RuleSpec specA(ruleA, lr, rr);

    std::vector<RuleSpec> bucket;
    unsigned int bitmap = 0;

    RulesMapCache::AddRule(bucket, specA, true, bitmap);

    ASSERT_EQ(bucket.size(), 1U);
    EXPECT_EQ(bucket[0].GetRule().GetRuleAction(), RuleAction::STOP_PROCESSING_SPACE_ACTIONS);
    EXPECT_EQ(bitmap & 0b11111U, bm1);

    RuleSpec specB(ruleA, lr, rr);
    RulesMapCache::AddRule(bucket, specB, false, bitmap);
    ASSERT_EQ(bucket.size(), 2U);

    EXPECT_EQ(bucket[bm1].GetRule().GetRuleAction(), RuleAction::STOP_PROCESSING_SPACE_ACTIONS);

    EXPECT_EQ((bitmap >> bm5) & 0b11111U, bm1);
}

TEST_F(LSPRulesMapCacheTests, BuildMap_SingleRuleGoesInCorrectBucket)
{
    auto alwaysTrue = [](FormattingContext *) { return true; };
    Rule rule({alwaysTrue}, RuleAction::INSERT_SPACE, RuleFlags {});
    RuleSpec spec(rule, Single(TokenType::KEYW_LET), Single(TokenType::PUNCTUATOR_EQUAL));

    auto bucketMap = RulesMapCache::BuildMap(std::vector<RuleSpec> {spec});
    int idx = RulesMapCache::GetRuleBucketIndex(TokenType::KEYW_LET, TokenType::PUNCTUATOR_EQUAL);
    auto it = bucketMap.find(idx);
    ASSERT_NE(it, bucketMap.end()) << "Expected bucket for LET->EQUAL";
    EXPECT_EQ(it->second.size(), 1U);
    EXPECT_EQ(it->second[0].GetRule().GetRuleAction(), RuleAction::INSERT_SPACE);
}

TEST_F(LSPRulesMapCacheTests, BuildMap_InsertionOrder)
{
    auto alwaysTrue = [](FormattingContext *) { return true; };
    Rule ruleA({alwaysTrue}, RuleAction::INSERT_SPACE, RuleFlags {});
    Rule ruleB({alwaysTrue}, RuleAction::INSERT_NEWLINE, RuleFlags {});

    RuleSpec specA(ruleA, Single(TokenType::KEYW_LET, true), Single(TokenType::PUNCTUATOR_EQUAL, true));
    RuleSpec specB(ruleB, Single(TokenType::KEYW_LET, false), Single(TokenType::PUNCTUATOR_EQUAL, false));

    auto bucketMap = RulesMapCache::BuildMap({specA, specB});
    int idx = RulesMapCache::GetRuleBucketIndex(TokenType::KEYW_LET, TokenType::PUNCTUATOR_EQUAL);

    auto &bucket = bucketMap[idx];
    ASSERT_EQ(bucket.size(), 2U) << "Should have two rules in the bucket";

    EXPECT_EQ(bucket[0].GetRule().GetRuleAction(), RuleAction::INSERT_SPACE);
    EXPECT_EQ(bucket[1].GetRule().GetRuleAction(), RuleAction::INSERT_NEWLINE);
}

TEST_F(LSPRulesMapCacheTests, BuildMap_StopRulesComeBeforeContextRules)
{
    auto stopPred = [](FormattingContext *) { return true; };
    auto contextPred = [](FormattingContext *) { return true; };

    Rule stopRule({stopPred}, RuleAction::STOP_PROCESSING_SPACE_ACTIONS, RuleFlags {});
    Rule ctxRule({contextPred}, RuleAction::INSERT_SPACE, RuleFlags {});

    RuleSpec specStop(stopRule, Single(TokenType::KEYW_LET), Single(TokenType::PUNCTUATOR_EQUAL));
    RuleSpec specCtx(ctxRule, Single(TokenType::KEYW_LET), Single(TokenType::PUNCTUATOR_EQUAL));

    auto bucketMap = RulesMapCache::BuildMap({specStop, specCtx});
    int idx = RulesMapCache::GetRuleBucketIndex(TokenType::KEYW_LET, TokenType::PUNCTUATOR_EQUAL);

    auto &bucket = bucketMap[idx];
    ASSERT_EQ(bucket.size(), 2U);

    EXPECT_EQ(bucket[0].GetRule().GetRuleAction(), RuleAction::STOP_PROCESSING_SPACE_ACTIONS);
    EXPECT_EQ(bucket[1].GetRule().GetRuleAction(), RuleAction::INSERT_SPACE);
}
}  // namespace
