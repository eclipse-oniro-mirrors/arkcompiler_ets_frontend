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

#include "gtest/gtest.h"
#include "../lsp_api_test.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "generated/code_fix_register.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/symbol_reference_index.h"

namespace {
using ark::es2panda::lsp::Initializer;

constexpr std::string_view IMPORT_FIXES_ID = "ImportFixes";
// G_IMPORT_FIXES_CODE in import_fixes.cpp
constexpr int IMPORT_FIXES_ERROR_CODE = 1005;
constexpr int DEFAULT_THROTTLE = 20;
constexpr size_t SOURCE_FILE_INDEX = 0;
constexpr size_t CONSUMER_FILE_INDEX = 1;

class ImportFixesTest2 : public LSPAPITests {
public:
    void SetUp() override
    {
        LSPAPITests::SetUp();
        ark::es2panda::lsp::ClearSymbolReferenceIndex();
    }

    void TearDown() override
    {
        ark::es2panda::lsp::ClearSymbolReferenceIndex();
    }

    static ark::es2panda::lsp::CancellationToken CreateToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static void BuildSymbolIndex(Initializer &initializer, const std::string &filePath)
    {
        auto *context = initializer.CreateContext(filePath.c_str(), ES2PANDA_STATE_CHECKED);
        ASSERT_NE(context, nullptr);
        ASSERT_TRUE(ark::es2panda::lsp::BuildSymbolReferenceIndexForContext(context));
        initializer.DestroyContext(context);
    }

    static std::vector<CodeFixActionInfo> GetFixesAtSpan(es2panda_Context *context, size_t start, size_t length)
    {
        std::vector<int> errorCodes {IMPORT_FIXES_ERROR_CODE};
        CodeFixOptions options = {CreateToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
        return ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    }

    static std::vector<const CodeFixActionInfo *> CollectImportFixes(const std::vector<CodeFixActionInfo> &fixes)
    {
        std::vector<const CodeFixActionInfo *> result;
        for (const auto &fix : fixes) {
            if (fix.fixName_ == IMPORT_FIXES_ID) {
                result.push_back(&fix);
            }
        }
        return result;
    }

    static const CodeFixActionInfo *RequireSingleImportFix(const std::vector<CodeFixActionInfo> &fixes)
    {
        std::vector<const CodeFixActionInfo *> collected = CollectImportFixes(fixes);
        EXPECT_EQ(collected.size(), 1U);
        return collected.size() == 1U ? collected[0] : nullptr;
    }

    static std::string ApplyFirstChange(const std::string &source, const CodeFixActionInfo &action)
    {
        EXPECT_FALSE(action.changes_.empty());
        EXPECT_FALSE(action.changes_[0].textChanges.empty());
        const auto &change = action.changes_[0].textChanges[0];
        return source.substr(0, change.span.start) + change.newText +
               source.substr(change.span.start + change.span.length);
    }

private:
    class NullCancellationToken : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            return false;
        }
    };

    static NullCancellationToken &GetNullHost()
    {
        static NullCancellationToken instance;
        return instance;
    }
};

// A span whose start misses every identifier (it opens on the call parenthesis) still resolves
// through the end-position probe when the end offset lands inside the callee name.
TEST_F(ImportFixesTest2, EndProbeResolvesNameWhenStartTouchesPunctuator)
{
    std::vector<std::string> fileNames = {"IfxProbeSrc.ets", "IfxProbeConsumer.ets"};
    std::vector<std::string> fileContents = {R"(export function probeTarget(): void {})",
                                             R"(function use(): void {
    probeTarget();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[SOURCE_FILE_INDEX]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[CONSUMER_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    // Span starts on '(' of the call and ends inside the callee name
    const auto &consumer = fileContents[CONSUMER_FILE_INDEX];
    const auto nameStart = consumer.find("probeTarget");
    ASSERT_NE(nameStart, std::string::npos);
    const auto parenPos = consumer.find('(', nameStart);
    ASSERT_NE(parenPos, std::string::npos);
    auto fixes = GetFixesAtSpan(context, parenPos, (nameStart + 5) - parenPos);
    initializer.DestroyContext(context);

    const auto *fix = RequireSingleImportFix(fixes);
    ASSERT_NE(fix, nullptr);
    EXPECT_EQ(fix->description_, "Add import {probeTarget} from './IfxProbeSrc'");
}

// A span fully contained in non-identifier tokens (string literal content) resolves no name on
// any of the three probe positions, so no fix is offered.
TEST_F(ImportFixesTest2, NonIdentifierSpanReturnsNoFix)
{
    std::vector<std::string> fileNames = {"IfxStrConsumer.ets"};
    std::vector<std::string> fileContents = {
        R"(function use(): void {
    let msg = "ghostStr";
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto pos = fileContents[0].find("ghostStr");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = GetFixesAtSpan(context, pos, 8);
    initializer.DestroyContext(context);

    EXPECT_TRUE(CollectImportFixes(fixes).empty());
}

// The prefix-based fallback keeps only definitions whose exact name matches the unresolved
// symbol: a longer prefix match (ItemBox for query "It") is skipped without producing a fix.
TEST_F(ImportFixesTest2, ExactNameFilterSkipsLongerPrefixMatches)
{
    std::vector<std::string> fileNames = {"IfxPreA.ets", "IfxPreB.ets", "IfxPreConsumer.ets"};
    std::vector<std::string> fileContents = {R"(export class It {})", R"(export class ItemBox {})",
                                             R"(function use(): void {
    let i = new It();
})"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[0]);
    BuildSymbolIndex(indexInitializer, filePaths[1]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[2].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto pos = fileContents[2].find("It()");
    ASSERT_NE(pos, std::string::npos);
    auto fixes = GetFixesAtSpan(context, pos, 2);
    initializer.DestroyContext(context);

    const auto *fix = RequireSingleImportFix(fixes);
    ASSERT_NE(fix, nullptr);
    EXPECT_EQ(fix->description_, "Add import {It} from './IfxPreA'");
}

// An existing named-only import merges a newly imported default symbol into a combined
// `import DefWidget, { existFn } from '...'` statement.
TEST_F(ImportFixesTest2, MergesModuleDefaultIntoExistingNamedImport)
{
    std::vector<std::string> fileNames = {"IfxDmSrc.ets", "IfxDmConsumer.ets"};
    std::vector<std::string> fileContents = {
        R"(export default class DefWidget {}
export function existFn(): void {}
)",
        R"(import { existFn } from './IfxDmSrc';
function use(): void {
    let w = new DefWidget();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(filePaths.size(), fileNames.size());

    Initializer indexInitializer;
    BuildSymbolIndex(indexInitializer, filePaths[SOURCE_FILE_INDEX]);

    Initializer initializer;
    auto *context = initializer.CreateContext(filePaths[CONSUMER_FILE_INDEX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(context, nullptr);

    const auto &consumer = fileContents[CONSUMER_FILE_INDEX];
    const std::string_view defaultName = "DefWidget";
    const auto pos = consumer.find(defaultName);
    ASSERT_NE(pos, std::string::npos);
    auto fixes = GetFixesAtSpan(context, pos, defaultName.size());
    initializer.DestroyContext(context);

    const auto *fix = RequireSingleImportFix(fixes);
    ASSERT_NE(fix, nullptr);
    EXPECT_EQ(fix->description_, "Add import DefWidget from './IfxDmSrc'");
    ASSERT_EQ(fix->changes_.size(), 1U);
    ASSERT_EQ(fix->changes_[0].textChanges.size(), 1U);
    const auto &change = fix->changes_[0].textChanges[0];
    const auto importStart = consumer.find("import { existFn }");
    ASSERT_NE(importStart, std::string::npos);
    const auto importEnd = consumer.find(';', importStart) + 1;
    EXPECT_EQ(change.span.start, importStart);
    EXPECT_EQ(change.span.length, importEnd - importStart);
    EXPECT_EQ(change.newText, "import DefWidget, { existFn } from './IfxDmSrc';");
}

}  // namespace
