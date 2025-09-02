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
#include "lsp/include/register_code_fix/forgotten_this_property_access.h"

namespace {
class ForgottenThisPropertyAccessTests : public LSPAPITests {};

TEST_F(ForgottenThisPropertyAccessTests, ForgottenThisPropertyAccessTests_GetInfo)
{
    const char *source = R"(class Person {
name = "Alice";
greet() {
console.log(name);
}
}
    )";
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *context =
        initializer.CreateContext("ForgottenThisPropertyAccessTests_GetInfo.ets", ES2PANDA_STATE_CHECKED, source);

    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);

    const auto &diagnostics =
        ctx->diagnosticEngine->GetDiagnosticStorage(ark::es2panda::util::DiagnosticType::SEMANTIC);

    for (const auto &diagnostic : diagnostics) {
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto offset = index.GetOffset(
            ark::es2panda::lexer::SourceLocation(diagnostic->Line(), diagnostic->Offset(), ctx->parserProgram));
        auto node = ark::es2panda::lsp::GetTouchingToken(context, offset, false);
        es2panda_AstNode *thisExpr = impl->CreateThisExpression(context);
        es2panda_AstNode *memberExpr =
            impl->CreateMemberExpression(context, thisExpr, reinterpret_cast<es2panda_AstNode *>(node),
                                         MEMBER_EXPRESSION_KIND_PROPERTY_ACCESS, false, false);
        impl->AstNodeSetParent(context, thisExpr, memberExpr);
        impl->AstNodeSetParent(context, reinterpret_cast<es2panda_AstNode *>(node), memberExpr);
        auto *memNode = reinterpret_cast<ark::es2panda::ir::AstNode *>(memberExpr);
        if (memNode == nullptr) {
            continue;
        }
        ASSERT_EQ(memNode->DumpEtsSrc(), "this.name");
    }

    initializer.DestroyContext(context);
}

}  // namespace