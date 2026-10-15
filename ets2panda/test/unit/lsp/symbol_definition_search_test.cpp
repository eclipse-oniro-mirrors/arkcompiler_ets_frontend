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
using ark::es2panda::lsp::BuildSymbolReferenceIndexForContextWithExternal;
using ark::es2panda::lsp::ClearSymbolReferenceIndex;
using ark::es2panda::lsp::FindExportSymbolDefinitionsByPrefix;
using ark::es2panda::lsp::FindSymbolDefinitionsByName;
using ark::es2panda::lsp::GetFileReferencesFromIndex;
using ark::es2panda::lsp::GetIndexedFileSource;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::SymbolDefSearchResult;

class LspSymbolDefinitionSearchTests : public LSPAPITests {
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

    static bool HasSymbol(const std::vector<SymbolDefSearchResult> &results, const std::string &name)
    {
        for (const auto &result : results) {
            if (result.symbolName == name) {
                return true;
            }
        }
        return false;
    }
};

// Negative/case branches; the exact-match arm is pinned by EARLIER symbol_reference_index_
// test_2.cpp (byNameRepo) - the weaker ExactMatch duplicate was removed.
TEST_F(LspSymbolDefinitionSearchTests, FindSymbolDefinitionsByName_NoMatch)
{
    const auto paths = CreateTempFile({"symbol_no_match.ets"}, {"export class MyClass {}"});
    ASSERT_EQ(paths.size(), 1U);
    Initializer initializer;
    auto *context = initializer.CreateContext(paths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(BuildSymbolReferenceIndexForContext(context));

    EXPECT_TRUE(FindSymbolDefinitionsByName("NonExistent", "").empty());

    initializer.DestroyContext(context);
}

TEST_F(LspSymbolDefinitionSearchTests, FindSymbolDefinitionsByName_CaseSensitive)
{
    const auto paths = CreateTempFile({"symbol_case.ets"}, {"export class MyClass {}"});
    ASSERT_EQ(paths.size(), 1U);
    Initializer initializer;
    auto *context = initializer.CreateContext(paths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(BuildSymbolReferenceIndexForContext(context));

    EXPECT_TRUE(FindSymbolDefinitionsByName("myclass", "").empty());

    initializer.DestroyContext(context);
}

TEST_F(LspSymbolDefinitionSearchTests, FindExportSymbolDefinitionsByPrefix_EmptyPrefix)
{
    const auto paths = CreateTempFile({"symbol_empty_prefix.ets"}, {R"(
export class MyClass {}
export class MyFunc {}
)"});
    ASSERT_EQ(paths.size(), 1U);
    Initializer initializer;
    auto *context = initializer.CreateContext(paths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(BuildSymbolReferenceIndexForContext(context));

    const auto results = FindExportSymbolDefinitionsByPrefix("", "");
    ASSERT_EQ(results.size(), 2U);
    EXPECT_TRUE(HasSymbol(results, "MyClass"));
    EXPECT_TRUE(HasSymbol(results, "MyFunc"));
    for (const auto &r : results) {
        EXPECT_EQ(r.fileName, paths[0]);
        EXPECT_FALSE(r.isDefaultExport);
    }

    initializer.DestroyContext(context);
}

// Boundary miss branches; the import-hit arm is pinned by EARLIER
// FileReferencesFromIndexForImportingFile - duplicate copies were removed.
TEST_F(LspSymbolDefinitionSearchTests, GetFileReferencesFromIndex_EmptyFileName)
{
    const auto paths = CreateTempFile({"symbol_empty_name.ets"}, {"export class MyClass {}"});
    ASSERT_EQ(paths.size(), 1U);
    Initializer initializer;
    auto *context = initializer.CreateContext(paths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);
    ASSERT_TRUE(BuildSymbolReferenceIndexForContext(context));

    EXPECT_TRUE(GetFileReferencesFromIndex(context, "", false).referenceInfos.empty());

    initializer.DestroyContext(context);
}

}  // namespace
