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

#include "lsp/include/completions.h"
#include <gtest/gtest.h>
#include "lsp_api_test.h"

class LspKeywordCompletionTests : public LSPAPITests {};

static void AssertCompletionsContainAndNotContainEntries(const std::vector<std::string> &entries,
                                                         const std::vector<std::string> &expectedEntries,
                                                         const std::vector<std::string> &unexpectedEntries)
{
    auto emptyCheck = expectedEntries.empty() && !entries.empty();
    ASSERT_FALSE(emptyCheck) << "Expected empty but the result is not. Actual account: " << entries.size();

    for (const auto &expectedEntry : expectedEntries) {
        bool found = false;
        for (const auto &entry : entries) {
            if (entry == expectedEntry) {
                found = true;
                break;
            }
        }
        ASSERT_TRUE(found) << "Expected completion '" << expectedEntry << "' not found";
    }

    for (const auto &unexpectedEntry : unexpectedEntries) {
        bool found = false;
        for (const auto &entry : entries) {
            if (entry == unexpectedEntry) {
                found = true;
                break;
            }
        }
        ASSERT_FALSE(found) << "Unexpected completion '" << unexpectedEntry << "' found";
    }
}

TEST_F(LspKeywordCompletionTests, GetKeywordCompletionsStartWithA)
{
    std::string input = "a";
    ark::es2panda::lsp::Request result = ark::es2panda::lsp::KeywordCompletionData(input);

    std::vector<std::string> expected = {"abstract", "any", "anyref", "arguments", "as", "asserts", "async", "await"};

    std::vector<std::string> actual;
    for (const auto &entry : result.keywordCompletions) {
        actual.push_back(entry.GetName());
    }

    AssertCompletionsContainAndNotContainEntries(actual, expected, {});
}
TEST_F(LspKeywordCompletionTests, GetKeywordCompletionsStartWithAs)
{
    std::string input = "as";

    std::vector<std::string> expected = {"as", "asserts", "async"};
    ark::es2panda::lsp::Request result = ark::es2panda::lsp::KeywordCompletionData(input);

    std::vector<std::string> actual;
    for (const auto &entry : result.keywordCompletions) {
        actual.push_back(entry.GetName());
    }

    AssertCompletionsContainAndNotContainEntries(actual, expected, {});
}
TEST_F(LspKeywordCompletionTests, GetKeywordCompletionsStartWithAs2)
{
    std::string input = "asse";
    ark::es2panda::lsp::Request result = ark::es2panda::lsp::KeywordCompletionData(input);

    std::vector<std::string> expected = {"asserts"};

    std::vector<std::string> actual;
    for (const auto &entry : result.keywordCompletions) {
        actual.push_back(entry.GetName());
    }

    AssertCompletionsContainAndNotContainEntries(actual, expected, {});
}
TEST_F(LspKeywordCompletionTests, GetKeywordCompletionsStartWithRe)
{
    std::string input = "re";
    ark::es2panda::lsp::Request result = ark::es2panda::lsp::KeywordCompletionData(input);

    std::vector<std::string> expected = {"readonly", "return", "require"};

    std::vector<std::string> actual;
    for (const auto &entry : result.keywordCompletions) {
        actual.push_back(entry.GetName());
    }

    AssertCompletionsContainAndNotContainEntries(actual, expected, {});
}
TEST_F(LspKeywordCompletionTests, GetKeywordCompletionsStartWithRet)
{
    std::string input = "ret";
    ark::es2panda::lsp::Request result = ark::es2panda::lsp::KeywordCompletionData(input);

    std::vector<std::string> expected = {"return"};

    std::vector<std::string> actual;
    for (const auto &entry : result.keywordCompletions) {
        actual.push_back(entry.GetName());
    }

    AssertCompletionsContainAndNotContainEntries(actual, expected, {});
}
TEST_F(LspKeywordCompletionTests, GetKeywordCompletionsStartWithI)
{
    std::string input = "i";
    ark::es2panda::lsp::Request result = ark::es2panda::lsp::KeywordCompletionData(input);

    std::vector<std::string> expected = {"i8",         "i16",    "i31ref", "i32",     "i64",        "if",
                                         "implements", "import", "in",     "infer",   "instanceof", "int",
                                         "interface",  "is",     "isize",  "internal"};

    std::vector<std::string> actual;
    for (const auto &entry : result.keywordCompletions) {
        actual.push_back(entry.GetName());
    }

    AssertCompletionsContainAndNotContainEntries(actual, expected, {});
}
TEST_F(LspKeywordCompletionTests, GetKeywordCompletionsStartWithInt)
{
    std::string input = "int";
    ark::es2panda::lsp::Request result = ark::es2panda::lsp::KeywordCompletionData(input);

    std::vector<std::string> expected = {"int", "interface", "internal"};

    std::vector<std::string> actual;
    for (const auto &entry : result.keywordCompletions) {
        actual.push_back(entry.GetName());
    }

    AssertCompletionsContainAndNotContainEntries(actual, expected, {});
}
TEST_F(LspKeywordCompletionTests, GetKeywordCompletionsSensitiveStartWithT)
{
    std::string input = "T";
    ark::es2panda::lsp::Request result = ark::es2panda::lsp::KeywordCompletionData(input);
    std::vector<std::string> expected = {"target", "this", "throw", "true", "try", "type", "typeof"};

    std::vector<std::string> actual;
    for (const auto &entry : result.keywordCompletions) {
        actual.push_back(entry.GetName());
    }

    AssertCompletionsContainAndNotContainEntries(actual, expected, {});
}
TEST_F(LspKeywordCompletionTests, GetKeywordCompletionsSensitiveStartWithTy)
{
    std::string input = "TY";
    ark::es2panda::lsp::Request result = ark::es2panda::lsp::KeywordCompletionData(input);
    std::vector<std::string> expected = {"type", "typeof"};

    std::vector<std::string> actual;
    for (const auto &entry : result.keywordCompletions) {
        actual.push_back(entry.GetName());
    }

    AssertCompletionsContainAndNotContainEntries(actual, expected, {});
}
TEST_F(LspKeywordCompletionTests, GetKeywordCompletionsInvalid)
{
    std::string input = "xyzabc";
    ark::es2panda::lsp::Request result = ark::es2panda::lsp::KeywordCompletionData(input);
    auto completions = result.keywordCompletions;

    ASSERT_TRUE(completions.empty());
}
