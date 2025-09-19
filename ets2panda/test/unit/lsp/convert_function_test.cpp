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

#include <string>
#include <cstddef>
#include "ir/astNode.h"
#include <gtest/gtest.h>
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/refactors/convert_function.h"
#include "lsp/include/services/text_change/text_change_context.h"

namespace {
class LSPConvertFunction : public LSPAPITests {
public:
    static constexpr std::string_view TO_NAMED_FUNCTION_KIND = "refactor.rewrite.function.named";
};

TEST_F(LSPConvertFunction, ConvertFunction1Test)
{
    std::string testCode = R"(
    const x = () => 42;
const y = function foo() {
    const bar = () => this.x;
}
const z = 123;
)";

    auto tempFiles = CreateTempFile({"convert_function.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = context->parserProgram->Ast();
    ark::es2panda::ir::AstNode *nodeToTry = nullptr;
    ast->FindChild([&nodeToTry](ark::es2panda::ir::AstNode *node) {
        if (node->IsVariableDeclaration()) {
            const auto varDecl = node->AsVariableDeclaration();
            const auto decl = ark::es2panda::lsp::TryGetFunctionFromVariableDeclaration(varDecl);
            nodeToTry = decl;
            if (nodeToTry != nullptr && (nodeToTry->IsArrowFunctionExpression() || nodeToTry->IsFunctionExpression())) {
                return true;
            }
        }
        return false;
    });
    ASSERT_TRUE(nodeToTry != nullptr && (nodeToTry->IsArrowFunctionExpression() || nodeToTry->IsFunctionExpression()));
    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertFunction, ContainingThisTest)
{
    std::string testCode = R"(
    function foo() {
    const bar = () => this.x;
}
)";
    auto tempFiles = CreateTempFile({"containing_this.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());
    const size_t funPos = 26;
    const size_t thisPos = 38;
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    const auto func = ark::es2panda::lsp::GetFunctionInfo(ctx, funPos);
    ASSERT_EQ(func.func->Start().index, thisPos);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertFunction, GetVariableInfoTest)
{
    std::string testCode = R"(
   const foo = () => {
    console.log("hello");
};
)";
    auto tempFiles = CreateTempFile({"get_variable_info_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());
    const char *name = "foo";
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = context->parserProgram->Ast();
    auto decl = ast->FindChild([](auto *node) { return node->IsArrowFunctionExpression(); });
    ASSERT_TRUE(decl != nullptr && decl->IsArrowFunctionExpression());
    auto varInfo = ark::es2panda::lsp::GetVariableInfo(decl);
    ASSERT_TRUE(varInfo.has_value());
    ASSERT_TRUE(varInfo->name != nullptr);
    ASSERT_EQ(varInfo->name->AsIdentifier()->Name().Utf8(), name);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertFunction, ConvertArrowFunctionToNamedFunction)
{
    std::string source = R"(
        const add = (a: number, b: number) => a + b;
    )";

    auto tempFiles = CreateTempFile({"convert_arrow_func_to_named_function.ets"}, {source});
    ASSERT_FALSE(tempFiles.empty());
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = context->parserProgram->Ast();
    auto decl = ast->FindChild([](auto *node) { return node->IsArrowFunctionExpression(); });
    ASSERT_TRUE(decl != nullptr && decl->IsArrowFunctionExpression());
    ark::es2panda::lsp::RefactorContext refactorContext;
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesContext = {{}, formatContext, {}};
    refactorContext.textChangesContext = &textChangesContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_FUNCTION_KIND);
    refactorContext.span.pos = decl->Start().index;
    auto info = ark::es2panda::lsp::GetVariableInfo(decl);
    auto varInfo = ark::es2panda::lsp::GetEditInfoForConvertToNamedFunction(
        refactorContext, decl->AsArrowFunctionExpression(), info.value());
    ASSERT_FALSE(varInfo.empty());
    const auto &fileChange = varInfo.front();
    auto change = fileChange.textChanges[0].span.start;
    const size_t expectedPos = 9;  // position of 'add' in source code
    const std::string changeText = "function add(a: number, b: number) {\n  return ((a) + (b));\n}\n";
    ASSERT_EQ(fileChange.textChanges[0].newText, changeText);
    ASSERT_EQ(change, expectedPos);
    ASSERT_FALSE(fileChange.textChanges.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertFunction, ConvertNamedFunctionToArrowFunction)
{
    std::string source = "function A(a:number, b:number):boolean{ return true;}";
    auto tempFiles = CreateTempFile({"convert_named_to_arrow_function.ets"}, {source});
    ASSERT_FALSE(tempFiles.empty());
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = context->parserProgram->Ast();
    auto decl = ast->FindChild([](auto *node) { return node->IsFunctionDeclaration(); });
    ASSERT_TRUE(decl != nullptr && decl->IsFunctionDeclaration());
    ark::es2panda::lsp::RefactorContext refactorContext;
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesContext = {{}, formatContext, {}};
    refactorContext.textChangesContext = &textChangesContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_FUNCTION_KIND);
    refactorContext.span.pos = decl->Start().index;
    auto varInfo =
        ark::es2panda::lsp::GetEditInfoForConvertToArrowFunction(refactorContext, decl->AsFunctionDeclaration());
    ASSERT_FALSE(varInfo.empty());
    const auto &fileChange = varInfo.front();
    auto change = fileChange.textChanges[0].span.start;
    const std::string changeText = "((a: number, b: number) => {\n  return true;\n})";
    ASSERT_EQ(fileChange.textChanges[0].newText, changeText);
    ASSERT_EQ(change, decl->Start().index);
    ASSERT_FALSE(fileChange.textChanges.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPConvertFunction, ConvertFunctionDeclaration_ToArrowFunction)
{
    std::string source = "function A(a:number, b:number):boolean{ return true;}";
    auto tempFiles = CreateTempFile({"convert_arrow_func_to_named_function_2.ets"}, {source});
    ASSERT_FALSE(tempFiles.empty());
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = context->parserProgram->Ast();
    auto decl = ast->FindChild([](auto *node) { return node->IsFunctionDeclaration(); });
    ASSERT_TRUE(decl != nullptr && decl->IsFunctionDeclaration());
    ark::es2panda::lsp::RefactorContext refactorContext;
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesContext = {{}, formatContext, {}};
    refactorContext.textChangesContext = &textChangesContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_FUNCTION_KIND);
    refactorContext.span.pos = decl->Start().index;
    auto result = ark::es2panda::lsp::GetRefactorEditsToConvertFunctionExpressions(
        refactorContext, ark::es2panda::lsp::TO_ARROW_FUNCTION_ACTION.name);

    ASSERT_FALSE(result.GetFileTextChanges().empty());
    const std::string changeText = "((a: number, b: number) => {\n  return true;\n})";
    ASSERT_EQ(result.GetFileTextChanges().front().textChanges[0].newText, changeText);
}

TEST_F(LSPConvertFunction, ConvertArrowFunction_ToNamedFunction)
{
    std::string source = R"(
        const add = (a: number, b: number) => a + b;
    )";

    auto tempFiles = CreateTempFile({"convert_arrow_func_to_named_function_1.ets"}, {source});
    ASSERT_FALSE(tempFiles.empty());
    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto ast = context->parserProgram->Ast();
    auto decl = ast->FindChild([](auto *node) { return node->IsArrowFunctionExpression(); });
    ASSERT_TRUE(decl != nullptr && decl->IsArrowFunctionExpression());
    ark::es2panda::lsp::RefactorContext refactorContext;
    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesContext = {{}, formatContext, {}};
    refactorContext.textChangesContext = &textChangesContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(TO_NAMED_FUNCTION_KIND);
    refactorContext.span.pos = decl->Start().index;
    auto result = ark::es2panda::lsp::GetRefactorEditsToConvertFunctionExpressions(
        refactorContext, ark::es2panda::lsp::TO_NAMED_FUNCTION_ACTION.name);

    ASSERT_FALSE(result.GetFileTextChanges().empty());
    const std::string changeText = "function add(a: number, b: number) {\n  return ((a) + (b));\n}\n";
    ASSERT_EQ(result.GetFileTextChanges().front().textChanges[0].newText, changeText);
}

}  // namespace