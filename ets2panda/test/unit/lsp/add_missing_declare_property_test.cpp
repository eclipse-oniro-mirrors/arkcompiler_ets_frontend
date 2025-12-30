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

#include "lsp/include/register_code_fix/add_missing_declare_property.h"
#include <vector>
#include "gtest/gtest.h"
#include "lsp_api_test.h"

namespace {
class AddMissingDeclareProperty : public LSPAPITests {};

TEST_F(AddMissingDeclareProperty, AddMissingDeclareProperty1)
{
    std::vector<std::string> files = {"Add_missing_declare_property.ets"};
    std::vector<std::string> texts = {R"(
        function foo(): void;
        class MyClass {
            myProperty: string;
            constructor() {
                this.myProperty = "Hello";
            }
        }
)"};
    auto filePaths = CreateTempFile(files, texts);

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const auto &diagnostics =
        ctx->diagnosticEngine->GetDiagnosticStorage(ark::es2panda::util::DiagnosticType::SEMANTIC);
    std::vector<ark::es2panda::ir::AstNode *> fixedNodes;
    for (const auto &diagnostic : diagnostics) {
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto offset = index.GetOffset(
            ark::es2panda::lexer::SourceLocation(diagnostic->Line(), diagnostic->Offset(), ctx->parserProgram));
        ark::es2panda::lsp::FormatCodeSettings settings;
        auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
        TextChangesContext changeText {{}, formatContext, {}};
        ark::es2panda::lsp::ChangeTracker tracker = ark::es2panda::lsp::ChangeTracker::FromContext(changeText);
        ark::es2panda::lsp::MakeChangeAddMissing(tracker, context, offset, fixedNodes);
        auto changes = tracker.GetChanges();
        ASSERT_EQ(changes.size(), 1);
        auto &fileChange = changes[0];
        ASSERT_EQ(fileChange.textChanges.size(), 1);
        auto &textChange = fileChange.textChanges[0];
        EXPECT_EQ(textChange.newText, "declare ");
        auto startPos = textChange.span.start;
        EXPECT_EQ(startPos, diagnostic->Offset());
    }
    initializer.DestroyContext(context);
}
}  // namespace