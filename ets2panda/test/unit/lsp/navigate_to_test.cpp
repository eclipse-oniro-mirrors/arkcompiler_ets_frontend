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

#include <gtest/gtest.h>
#include <cstddef>
#include "lsp_api_test.h"
#include "lsp/include/navigate_to.h"

namespace {

using ark::es2panda::SourceFile;
using ark::es2panda::lsp::GetNavigateToItems;
using ark::es2panda::lsp::Initializer;
using std::string;
using std::vector;

class NavigateToTest : public LSPAPITests {
protected:
    static void SetUpTestSuite()
    {
        initializer_ = new Initializer();
        GenerateContexts(*initializer_);
    }

    static void TearDownTestSuite()
    {
        for (auto ctx : contexts_) {
            initializer_->DestroyContext(ctx);
        }
        delete initializer_;
        initializer_ = nullptr;
    }
    static void GenerateContexts(Initializer &initializer)
    {
        for (const auto &file : files_) {
            std::string sourceStr(file.source);
            es2panda_Context *ctx =
                initializer.CreateContext(file.filePath.data(), ES2PANDA_STATE_CHECKED, sourceStr.c_str());
            contexts_.push_back(ctx);
        }
    };
    // NOLINTBEGIN(fuchsia-statically-constructed-objects, cert-err58-cpp)
    static inline std::vector<es2panda_Context *> contexts_ = {};
    static inline Initializer *initializer_ = nullptr;
    static inline std::vector<SourceFile> files_ = {{"ExactPrefixAndSubstringMatch1.sts", R"(
        function foo() {
            let a = 1;
            return a;
        }
        )"},
                                                    {"ExactPrefixAndSubstringMatch2.sts", R"(
        function foobar() {
            let b = 2;
            return b;
        }
        )"},
                                                    {"ExactPrefixAndSubstringMatch3.sts", R"(
        function my_foo() {
            let c = 3;
            return c;
        }
        )"},
                                                    {"exatchMatch.sts", R"(
            class Test {
                yeke: number = 2;
                method() {
                    let b = 3;
                    return b;
                }
            }
            )"}};
    // NOLINTEND(fuchsia-statically-constructed-objects, cert-err58-cpp)
};

TEST_F(NavigateToTest, ExactMatchFromSingleFile)
{
    std::vector<SourceFile> files = {};
    Initializer initializer;
    std::vector<ark::es2panda::lsp::NavigateToItem> allResults;
    const int maxResultCount = 10;
    const int expectedResultSize = 1;
    std::string searchTerm = "yeke";
    std::string containerName = "Test";

    std::vector<SourceFile> singleFile = {{files_[3].filePath, files_[3].source}};
    auto results = GetNavigateToItems(contexts_[3], singleFile, maxResultCount, searchTerm, false);
    allResults.insert(allResults.end(), results.begin(), results.end());

    ASSERT_EQ(allResults.size(), expectedResultSize);
    ASSERT_EQ(allResults[0].name, searchTerm);
    ASSERT_EQ(allResults[0].matchKind, ark::es2panda::lsp::MatchKind::EXACT);
    ASSERT_EQ(allResults[0].containerName, containerName);
}

TEST_F(NavigateToTest, ExactPrefixAndSubstringMatch)
{
    std::vector<ark::es2panda::lsp::NavigateToItem> allResults;
    const int maxResultCount = 10;
    const int expectedResultSize = 3;
    std::string searchTerm = "foo";
    const int thirdValueOfArray = 2;
    std::string searchTermFoobar = "foobar";
    std::string searchTermMyFoo = "my_foo";

    for (size_t i = 0; i < files_.size() - 1; i++) {
        std::vector<SourceFile> singleFile = {{files_[i].filePath, files_[i].source}};
        auto results = GetNavigateToItems(contexts_[i], singleFile, maxResultCount, searchTerm, false);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }

    ASSERT_EQ(allResults.size(), expectedResultSize);  // "foo", "foobar", "my_foo"
    ASSERT_EQ(allResults[0].name, searchTerm);
    ASSERT_EQ(allResults[0].matchKind, ark::es2panda::lsp::MatchKind::EXACT);
    ASSERT_EQ(allResults[1].name, searchTermFoobar);
    ASSERT_EQ(allResults[1].matchKind, ark::es2panda::lsp::MatchKind::PREFIX);
    ASSERT_EQ(allResults[thirdValueOfArray].name, searchTermMyFoo);
    ASSERT_EQ(allResults[thirdValueOfArray].matchKind, ark::es2panda::lsp::MatchKind::SUBSTRING);
}

TEST_F(NavigateToTest, CaseInsensitiveMatch)
{
    std::vector<ark::es2panda::lsp::NavigateToItem> allResults;
    const int maxResultCount = 10;
    const int expectedResultSize = 3;
    std::string searchTerm = "FOO";

    for (size_t i = 0; i < files_.size() - 1; i++) {
        std::vector<SourceFile> singleFile = {{files_[i].filePath, files_[i].source}};
        auto results = GetNavigateToItems(contexts_[i], singleFile, maxResultCount, searchTerm, false);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }
    ASSERT_EQ(allResults.size(), expectedResultSize);
}

TEST_F(NavigateToTest, CaseSensitiveMismatch)
{
    std::vector<ark::es2panda::lsp::NavigateToItem> allResults;
    const int maxResultCount = 10;
    std::string searchTerm = "FOO";

    for (size_t i = 0; i < files_.size() - 1; i++) {
        std::vector<SourceFile> singleFile = {{files_[i].filePath, files_[i].source}};
        auto results = GetNavigateToItems(contexts_[i], singleFile, maxResultCount, searchTerm, true);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }
    ASSERT_TRUE(allResults.empty());
}

TEST_F(NavigateToTest, NoMatchFound)
{
    std::vector<ark::es2panda::lsp::NavigateToItem> allResults;
    const int maxResultCount = 10;
    std::string searchTerm = "nonexistent";

    for (size_t i = 0; i < files_.size() - 1; i++) {
        std::vector<SourceFile> singleFile = {{files_[i].filePath, files_[i].source}};
        auto results = GetNavigateToItems(contexts_[i], singleFile, maxResultCount, searchTerm, false);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }
    ASSERT_TRUE(allResults.empty());
}

TEST_F(NavigateToTest, MatchLimitRespected)
{
    const int maxResultCountTwo = 2;
    const int expectedResultSize = 2;
    std::string searchTerm = "foo";
    std::vector<ark::es2panda::lsp::NavigateToItem> allResults;

    for (size_t i = 0; i < files_.size() - 1; i++) {
        std::vector<SourceFile> singleFile = {{files_[i].filePath, files_[i].source}};
        auto results = GetNavigateToItems(contexts_[i], singleFile, maxResultCountTwo, searchTerm, false);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }

    ASSERT_LE(allResults.size(), expectedResultSize);
}

TEST_F(NavigateToTest, MultiFileSubstringMatch)
{
    const int maxResultCount = 10;
    size_t totalMatches = 0;
    std::string searchTerm = "_foo";
    const int expectedResultSize = 1;
    for (size_t i = 0; i < files_.size() - 1; i++) {
        std::vector<SourceFile> singleFile = {{files_[i].filePath, files_[i].source}};
        auto results = GetNavigateToItems(contexts_[i], singleFile, maxResultCount, searchTerm, false);
        totalMatches += results.size();
    }
    ASSERT_EQ(totalMatches, expectedResultSize);
}

TEST_F(NavigateToTest, PrefixMatchOnly)
{
    const int maxResultCount = 10;
    size_t prefixCount = 0;
    const int expectedResultSize = 1;
    std::string searchTerm = "foo";
    for (size_t i = 0; i < files_.size() - 1; i++) {
        std::vector<SourceFile> singleFile = {{files_[i].filePath, files_[i].source}};
        auto results = GetNavigateToItems(contexts_[i], singleFile, maxResultCount, searchTerm, false);
        for (const auto &match : results) {
            if (match.matchKind == ark::es2panda::lsp::MatchKind::PREFIX) {
                ++prefixCount;
            }
        }
    }

    ASSERT_EQ(prefixCount, expectedResultSize);  // Only "foobar" is a PREFIX of "foo"
}

TEST_F(NavigateToTest, MatchFromSecondFile)
{
    const int maxResultCount = 10;
    const int expectedResultSize = 1;
    std::string searchTerm = "foobar";
    std::vector<ark::es2panda::lsp::NavigateToItem> matches;

    for (size_t i = 0; i < files_.size() - 1; i++) {
        std::vector<SourceFile> singleFile = {{files_[i].filePath, files_[i].source}};
        auto results = GetNavigateToItems(contexts_[i], singleFile, maxResultCount, searchTerm, false);
        matches.insert(matches.end(), results.begin(), results.end());
    }
    ASSERT_EQ(matches.size(), expectedResultSize);
    ASSERT_EQ(matches[0].name, searchTerm);
}

TEST_F(NavigateToTest, MatchOnClassMember)
{
    const int maxResultCount = 10;
    const int expectedResultSize = 1;
    std::string searchTerm = "yeke";
    std::string containerName = "Test";
    std::vector<ark::es2panda::lsp::NavigateToItem> results;

    for (size_t i = 1; i < files_.size(); i++) {
        std::vector<SourceFile> singleFile = {{files_[i].filePath, files_[i].source}};
        auto items = GetNavigateToItems(contexts_[i], singleFile, maxResultCount, searchTerm, false);
        results.insert(results.end(), items.begin(), items.end());
    }

    ASSERT_EQ(results.size(), expectedResultSize);
    ASSERT_EQ(results[0].name, searchTerm);
    ASSERT_EQ(results[0].containerName, containerName);
}

}  // namespace
