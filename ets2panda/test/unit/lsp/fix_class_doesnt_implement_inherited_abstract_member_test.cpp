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
#include "lsp/include/internal_api.h"
#include "lsp/include/types.h"
#include "lsp/include/register_code_fix/fix_class_doesnt_implement_inherited_abstract_member.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "lsp/include/code_fixes/code_fix_types.h"

namespace {

using ark::es2panda::lsp::Initializer;

class LspFixAbstractMemberTests : public LSPAPITests {
public:
    std::vector<ark::es2panda::lsp::CodeFixAction> MockGetCodeActions(es2panda_Context *context, TextSpan span)
    {
        std::vector<ark::es2panda::lsp::CodeFixAction> returnedActions;
        if (span.length == 0) {
            return returnedActions;
        }
        std::vector<TextChange> textChanges;
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        const auto &diagnostics =
            ctx->diagnosticEngine->GetDiagnosticStorage(ark::es2panda::util::DiagnosticType::SEMANTIC);
        ark::es2panda::lsp::FixClassNotImplementingInheritedMembers handle;
        for (const auto &diagnostic : diagnostics) {
            auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
            auto offset = index.GetOffset(
                ark::es2panda::lexer::SourceLocation(diagnostic->Line(), diagnostic->Offset(), ctx->parserProgram));
            size_t end = span.start + span.length;
            if (offset < span.start || offset >= end) {
                break;
            }
            textChanges.push_back(handle.MakeTextChange(context, offset));
        }
        ark::es2panda::lsp::CodeFixAction codeAction;
        codeAction.changes.emplace_back(std::string(ctx->parserProgram->SourceFilePath()), textChanges);
        codeAction.fixId = "FixClassNotImplementingInheritedMembers";
        returnedActions.push_back(codeAction);
        return returnedActions;
    }

    std::vector<ark::es2panda::lsp::CodeFixAction> MockGetAllCodeActions(es2panda_Context *context)
    {
        std::vector<ark::es2panda::lsp::CodeFixAction> returnedActions;
        std::vector<TextChange> textChanges;
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        const auto &diagnostics =
            ctx->diagnosticEngine->GetDiagnosticStorage(ark::es2panda::util::DiagnosticType::SEMANTIC);
        ark::es2panda::lsp::FixClassNotImplementingInheritedMembers handle;
        for (const auto &diagnostic : diagnostics) {
            auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
            auto offset = index.GetOffset(
                ark::es2panda::lexer::SourceLocation(diagnostic->Line(), diagnostic->Offset(), ctx->parserProgram));
            textChanges.push_back(handle.MakeTextChange(context, offset));
        }
        ark::es2panda::lsp::CodeFixAction codeAction;
        codeAction.changes.emplace_back(std::string(ctx->parserProgram->SourceFilePath()), textChanges);
        codeAction.fixId = "FixClassNotImplementingInheritedMembers";
        returnedActions.push_back(codeAction);
        return returnedActions;
    }
};

TEST_F(LspFixAbstractMemberTests, getCodeActionsToFixClassNotImplementingInheritedMembers1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("LspFixAbstractMemberTests_001.ets", ES2PANDA_STATE_CHECKED, R"(abstract class A {
  abstract foo(a:number, b:number): number;
  abstract foo1(a:number, b:number);
}

class B extends A {
  foo(a: number, b: number): number {}
})");
    const size_t start = 109;
    ark::es2panda::lsp::FixClassNotImplementingInheritedMembers handle;
    auto result = handle.MakeTextChange(ctx, start);
    std::string expectedNewText = "  foo1(a: number, b: number) {}\n";
    const size_t expectedStart = 122;
    const size_t expectedLength = 0;
    ASSERT_EQ(result.newText, expectedNewText);
    ASSERT_EQ(result.span.start, expectedStart);
    ASSERT_EQ(result.span.length, expectedLength);
    initializer.DestroyContext(ctx);
}

TEST_F(LspFixAbstractMemberTests, getCodeActionsToFixClassNotImplementingInheritedMembers2)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("LspFixAbstractMemberTests_002.ets", ES2PANDA_STATE_CHECKED, R"(abstract class A {
  abstract foo(a:number, b:number): number;
  abstract foo1(a:number, b:number);
}

class B extends A {
};
class C extends A {
})");
    const size_t start = 109;
    const size_t length = 20;
    auto result = MockGetCodeActions(ctx, {start, length});
    std::string expectedNewText = "  foo(a: number, b: number): number {}\n  foo1(a: number, b: number) {}\n";
    std::string expectedFileName = "LspFixAbstractMemberTests_002.ets";
    const size_t expectedStart = 122;
    const size_t expectedLength = 0;
    ASSERT_EQ(result[0].changes[0].fileName, expectedFileName);
    ASSERT_EQ(result[0].changes[0].textChanges[0].newText, expectedNewText);
    ASSERT_EQ(result[0].changes[0].textChanges[0].span.start, expectedStart);
    ASSERT_EQ(result[0].changes[0].textChanges[0].span.length, expectedLength);
    initializer.DestroyContext(ctx);
}

TEST_F(LspFixAbstractMemberTests, getCodeActionsToFixClassNotImplementingInheritedMembers3)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("LspFixAbstractMemberTests_003.ets", ES2PANDA_STATE_CHECKED, R"(abstract class A {
  abstract foo(a:number, b:number): number;
  abstract foo1(a:number, b:number);
}

class B extends A {
};
class C extends A {
})");
    auto result = MockGetAllCodeActions(ctx);
    std::string expectedNewText = "  foo(a: number, b: number): number {}\n  foo1(a: number, b: number) {}\n";
    std::string expectedFileName = "LspFixAbstractMemberTests_003.ets";
    const size_t expectedStart = 122;
    const size_t expectedStart2 = 145;
    const size_t expectedLength = 0;
    ASSERT_EQ(result[0].changes[0].fileName, expectedFileName);
    ASSERT_EQ(result[0].changes[0].textChanges[0].newText, expectedNewText);
    ASSERT_EQ(result[0].changes[0].textChanges[0].span.start, expectedStart);
    ASSERT_EQ(result[0].changes[0].textChanges[0].span.length, expectedLength);
    ASSERT_EQ(result[0].changes[0].fileName, expectedFileName);
    ASSERT_EQ(result[0].changes[0].textChanges[1].newText, expectedNewText);
    ASSERT_EQ(result[0].changes[0].textChanges[1].span.start, expectedStart2);
    ASSERT_EQ(result[0].changes[0].textChanges[1].span.length, expectedLength);
    initializer.DestroyContext(ctx);
}
}  // namespace