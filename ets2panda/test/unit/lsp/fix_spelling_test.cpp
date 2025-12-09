/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gtest/gtest.h"
#include "lsp_api_test.h"
#include <cstddef>
#include <iostream>
#include "lsp/include/register_code_fix/fix_spelling.h"

namespace {
class FixSpellingTests : public LSPAPITests {};

TEST_F(FixSpellingTests, FixSpelling1)
{
    std::vector<std::string> files = {"Fix_Spelling1.ets", "Fix_Spelling2.ets"};
    std::vector<std::string> texts = {R"(export const spelling = 42;)",
                                      R"(import { speling } from './Fix_Spelling1';)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto context = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const auto &diagnostics =
        ctx->diagnosticEngine->GetDiagnosticStorage(ark::es2panda::util::DiagnosticType::SEMANTIC);

    for (const auto &diagnostic : diagnostics) {
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto offset = index.GetOffset(
            ark::es2panda::lexer::SourceLocation(diagnostic->Line(), diagnostic->Offset(), ctx->parserProgram));

        auto node = ark::es2panda::lsp::GetInfoSpelling(context, offset);
        ASSERT_EQ(node.GetFindClosestWord(), "spelling");
        ASSERT_NE(node.GetNode(), nullptr);
        ark::es2panda::lsp::FormatCodeSettings settings;
        auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
        TextChangesContext changeText {{}, formatContext, {}};
        ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
        ark::es2panda::lsp::DoChanges(tracker, context, node.GetNode(), node.GetFindClosestWord());
        auto changes = tracker.GetChanges();
        ASSERT_EQ(changes.size(), 1);
    }

    initializer.DestroyContext(context);
}

TEST_F(FixSpellingTests, FixSpelling3)
{
    const char *source1 = R"(
    let spelling = "123";
    console.log(speling);
    )";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *context = initializer.CreateContext("Fix_Spelling3.ets", ES2PANDA_STATE_CHECKED, source1);

    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);

    const auto &diagnostics =
        ctx->diagnosticEngine->GetDiagnosticStorage(ark::es2panda::util::DiagnosticType::SEMANTIC);

    for (const auto &diagnostic : diagnostics) {
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto offset = index.GetOffset(
            ark::es2panda::lexer::SourceLocation(diagnostic->Line(), diagnostic->Offset(), ctx->parserProgram));
        auto node = ark::es2panda::lsp::GetInfoSpelling(context, offset);
        ASSERT_EQ(node.GetFindClosestWord(), "spelling");
        ASSERT_NE(node.GetNode(), nullptr);
        ark::es2panda::lsp::FormatCodeSettings settings;
        auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
        TextChangesContext changeText {{}, formatContext, {}};
        ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
        ark::es2panda::lsp::DoChanges(tracker, context, node.GetNode(), node.GetFindClosestWord());
        auto changes = tracker.GetChanges();
        ASSERT_EQ(changes.size(), 1);
    }

    initializer.DestroyContext(context);
}
TEST_F(FixSpellingTests, FixSpelling4)
{
    const char *source1 = R"(
    const spelling = "hello";
speling;
    )";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *context = initializer.CreateContext("Fix_Spelling4.ets", ES2PANDA_STATE_CHECKED, source1);

    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);

    const auto &diagnostics =
        ctx->diagnosticEngine->GetDiagnosticStorage(ark::es2panda::util::DiagnosticType::SEMANTIC);

    for (const auto &diagnostic : diagnostics) {
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto offset = index.GetOffset(
            ark::es2panda::lexer::SourceLocation(diagnostic->Line(), diagnostic->Offset(), ctx->parserProgram));
        auto node = ark::es2panda::lsp::GetInfoSpelling(context, offset);
        ASSERT_EQ(node.GetFindClosestWord(), "spelling");
        ASSERT_NE(node.GetNode(), nullptr);
        ark::es2panda::lsp::FormatCodeSettings settings;
        auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
        TextChangesContext changeText {{}, formatContext, {}};
        ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
        ark::es2panda::lsp::DoChanges(tracker, context, node.GetNode(), node.GetFindClosestWord());
        auto changes = tracker.GetChanges();
        ASSERT_EQ(changes.size(), 1);
    }

    initializer.DestroyContext(context);
}
TEST_F(FixSpellingTests, FixSpelling5)
{
    const char *source1 = R"(
    namespace MyNamespace {
    export const x = 1;
}
MyNamspace.x;
    )";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *context = initializer.CreateContext("Fix_Spelling5.ets", ES2PANDA_STATE_CHECKED, source1);

    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);

    const auto &diagnostics =
        ctx->diagnosticEngine->GetDiagnosticStorage(ark::es2panda::util::DiagnosticType::SEMANTIC);

    for (const auto &diagnostic : diagnostics) {
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto offset = index.GetOffset(
            ark::es2panda::lexer::SourceLocation(diagnostic->Line(), diagnostic->Offset(), ctx->parserProgram));
        auto node = ark::es2panda::lsp::GetInfoSpelling(context, offset);
        ASSERT_EQ(node.GetFindClosestWord(), "MyNamespace");
        ASSERT_NE(node.GetNode(), nullptr);
        ark::es2panda::lsp::FormatCodeSettings settings;
        auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
        TextChangesContext changeText {{}, formatContext, {}};
        ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
        ark::es2panda::lsp::DoChanges(tracker, context, node.GetNode(), node.GetFindClosestWord());
        auto changes = tracker.GetChanges();
        ASSERT_EQ(changes.size(), 1);
    }

    initializer.DestroyContext(context);
}

}  // namespace