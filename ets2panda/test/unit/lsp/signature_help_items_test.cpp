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

#include "lsp/include/signature_help_items.h"
#include "lsp/include/create_type_help_items.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>
#include "lsp/include/internal_api.h"
#include "test/unit/lsp/lsp_api_test.h"
#include "checker/types/signature.h"
#include "checker/checker.h"
#include "lexer/token/sourceLocation.h"
#include "public/es2panda_lib.h"
#include "ir/astNode.h"
#include "ir/expressions/functionExpression.h"
#include "ir/statements/functionDeclaration.h"
#include <iostream>
#include <optional>
#include <ostream>
#include <vector>
#include "lsp/include/signature_help.h"

namespace {

using ark::es2panda::lsp::Initializer;
class LSPSignatureHelpItemsTests : public LSPAPITests {};

ark::es2panda::ir::AstNode *FindTokenOnLeftOfPosition(es2panda_Context *context, size_t position)
{
    auto const tokenAtPosition = ark::es2panda::lsp::GetTouchingToken(context, position, false);
    if (tokenAtPosition->Start().index < position && tokenAtPosition->End().index > position) {
        return tokenAtPosition;
    }
    const auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    return ark::es2panda::lsp::FindPrecedingToken(position, ctx->parserProgram->Ast(), ctx->allocator);
}

TEST_F(LSPSignatureHelpItemsTests, StdLibMapGet)
{
    std::vector<std::string> files = {"getSignatureHelpItemsTest_map.ets"};
    std::vector<std::string> texts = {R"(let map = new Map<string, number>();
map.set("a", 1);
let a = map.get("a");
)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const offset = 68;
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    const size_t defaultTime = 20;
    auto invokedReason = ark::es2panda::lsp::SignatureHelpInvokedReason();
    auto cancellationToken = ark::es2panda::lsp::CancellationToken(defaultTime, nullptr);
    auto res = ark::es2panda::lsp::GetSignatureHelpItems(ctx, offset, invokedReason, cancellationToken);
    size_t const expectedSize = 2;
    size_t const expectedStart = 62;
    size_t const expectedLength = 12;
    size_t const expectedArgumentCount = 1;
    ASSERT_EQ(res.GetItems().size(), expectedSize);
    ASSERT_EQ(res.GetApplicableSpan().start, expectedStart);
    ASSERT_EQ(res.GetApplicableSpan().length, expectedLength);
    ASSERT_EQ(res.GetArgumentCount(), expectedArgumentCount);
    auto &item = res.GetItem(0);
    auto &prefix = item.GetPrefixDisplayParts();
    auto &suffix = item.GetSuffixDisplayParts();
    auto &parameters = item.GetParameters()[0].GetDisplayParts();
    auto expectedPrefix = std::vector<SymbolDisplayPart> {SymbolDisplayPart {"(", "punctuation"}};
    auto expectedSuffix = std::vector<SymbolDisplayPart> {
        SymbolDisplayPart {")", "punctuation"},
        SymbolDisplayPart {":", "punctuation"},
        SymbolDisplayPart {"Double|undefined", "typeName"},
    };
    auto expectedParameters = std::vector<SymbolDisplayPart> {
        SymbolDisplayPart {"key", "paramName"},
        SymbolDisplayPart {":", "punctuation"},
        SymbolDisplayPart {"String", "typeName"},
    };
    ASSERT_EQ(prefix, expectedPrefix);
    ASSERT_EQ(suffix, expectedSuffix);
    ASSERT_EQ(parameters, expectedParameters);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPSignatureHelpItemsTests, GetSignatureHelpItemsTest)
{
    const auto fileName = "getSignatureHelpItemsTest.ets";
    const auto fileText = R"(
    function test(a: number, b: string): void {
        console.log(a);
    }
    test(1, "test");
)";
    const size_t index0 = 0;
    const size_t index1 = 1;
    const size_t index2 = 2;
    const size_t position = 10;
    const size_t argumentIndex1 = 19;
    const size_t argumentIndex2 = 30;
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {fileText};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext(files.at(index0).c_str(), ES2PANDA_STATE_CHECKED, texts.at(index0).c_str());
    auto startingToken = FindTokenOnLeftOfPosition(ctx, position);
    ASSERT_TRUE(startingToken != nullptr);
    std::vector<ark::es2panda::lsp::ArgumentListInfo> argumentInfo;
    GetArgumentOrParameterListAndIndex(startingToken, argumentInfo);

    EXPECT_EQ(argumentInfo.size(), index2);
    EXPECT_EQ(argumentInfo[index0].GetArgumentIndex(), argumentIndex1);
    EXPECT_EQ(argumentInfo[index1].GetArgumentIndex(), argumentIndex2);
    initializer.DestroyContext(ctx);
}
TEST_F(LSPSignatureHelpItemsTests, GetResolvedSignatureForSignatureHelp)
{
    const auto fileName = "testSignature.ets";
    const auto fileText = R"(
function add(x: number, y: number): number {
return x + y;
}
let result = add(1, 2);
    )";
    const size_t index0 = 0;
    const size_t index2 = 3;
    const size_t position = 77;
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {fileText};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(files.at(index0).c_str(), ES2PANDA_STATE_CHECKED, fileText);

    auto callToken = FindTokenOnLeftOfPosition(ctx, position);
    const auto callExpr = callToken->Parent();
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto astNode = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());
    ASSERT_NE(callExpr, nullptr);
    ASSERT_NE(callExpr, nullptr);
    ASSERT_TRUE(callExpr->IsCallExpression());
    std::vector<ark::es2panda::checker::Signature *> candidates;
    auto *sig = ark::es2panda::lsp::GetResolvedSignatureForSignatureHelp(callExpr, astNode, candidates);

    ASSERT_NE(sig, nullptr);

    ASSERT_EQ(candidates.size(), index2);

    initializer.DestroyContext(ctx);
}
TEST_F(LSPSignatureHelpItemsTests, GetCandidateOrTypeInfo)
{
    const auto fileName = "candidateOrTypeInfo.ets";
    const auto fileText = R"(
function multiply(a: number, b: number): number {
return a * b;
}
let result = multiply(10, 20);
    )";
    const size_t index0 = 0;
    const size_t position = 82;

    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {fileText};
    auto filePaths = CreateTempFile(files, texts);

    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(files.at(index0).c_str(), ES2PANDA_STATE_CHECKED, fileText);

    auto callToken = ark::es2panda::lsp::FindTokenOnLeftOfPosition(ctx, position);
    ASSERT_NE(callToken, nullptr);
    const auto callee = callToken->Parent();
    std::vector<ark::es2panda::lsp::ArgumentListInfo> argumentInfoVec;
    ark::es2panda::lsp::GetArgumentOrParameterListAndIndex(callee, argumentInfoVec);
    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto astNode = reinterpret_cast<ark::es2panda::ir::AstNode *>(context->parserProgram->Ast());

    ASSERT_FALSE(argumentInfoVec.empty());
    ark::es2panda::lsp::ArgumentListInfo info = argumentInfoVec[index0];
    auto result = ark::es2panda::lsp::GetCandidateOrTypeInfo(info, astNode, false);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<ark::es2panda::lsp::CandidateInfo>(*result));
    auto candidateInfo = std::get<ark::es2panda::lsp::CandidateInfo>(*result);
    EXPECT_EQ(candidateInfo.GetKind(), ark::es2panda::lsp::CandidateOrTypeKind::CANDIDATE);
    EXPECT_FALSE(candidateInfo.GetSignatures().empty());
    EXPECT_NE(candidateInfo.GetResolvedSignature(), nullptr);

    initializer.DestroyContext(ctx);
}

TEST_F(LSPSignatureHelpItemsTests, CreateSignatureHelpItemTest)
{
    const auto fileName = "createSignatureHelpItemTest.ets";
    const auto fileText = R"(
 function testFunction<T, U>(param1: T, param2: U): number {
 return 0;
 }
 testFunction<number, string>(130, "test");
 )";
    const size_t index0 = 0;
    const size_t index1 = 1;
    const size_t position = 83;
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {fileText};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(files.at(index0).c_str(), ES2PANDA_STATE_CHECKED, fileText);
    auto callToken = FindTokenOnLeftOfPosition(ctx, position);
    const auto callNode = callToken->Parent();
    ASSERT_NE(callNode, nullptr);

    ASSERT_TRUE(callNode->IsCallExpression());
    ark::es2panda::ir::CallExpression *callExpr = nullptr;
    if (callNode->IsCallExpression()) {
        callExpr = callNode->AsCallExpression();
    }
    ark::es2panda::checker::Signature *signature = callExpr->Signature();
    SignatureHelpItem item = ark::es2panda::lsp::CreateSignatureHelpItem(*signature);
    bool found1 = false;
    bool found2 = false;
    EXPECT_FALSE(item.GetPrefixDisplayParts().empty());
    for (const auto &displayPart : item.GetPrefixDisplayParts()) {
        if (displayPart.GetText() == "<") {
            found1 = true;
        } else if (displayPart.GetText() == ">") {
            found2 = true;
        }
    }
    EXPECT_EQ(found1, true);
    EXPECT_EQ(found2, true);
    EXPECT_FALSE(item.GetSeparatorDisplayParts().empty());
    EXPECT_EQ(item.GetSeparatorDisplayParts().front().GetText(), ",");
    EXPECT_EQ(item.GetParameters().size(), 2U);
    EXPECT_EQ(item.GetParameters()[index0].GetName(), "param1");
    EXPECT_EQ(item.GetParameters()[index1].GetName(), "param2");
    item.Clear();
    initializer.DestroyContext(ctx);
}

TEST_F(LSPSignatureHelpItemsTests, CreateSignatureHelpItemParamsTest)
{
    const auto fileName = "CreateSignatureHelpItemParamsTest.ets";
    const auto fileText = R"(
 function testFunction<T, U>(param1: T, param2: U): number {
 return 0;
 }
 testFunction<number, string>(130, "test");
 )";
    const size_t index0 = 0;
    const size_t position = 83;
    const size_t position1 = 13;
    std::vector<std::string> files = {fileName};
    std::vector<std::string> texts = {fileText};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(files.at(index0).c_str(), ES2PANDA_STATE_CHECKED, fileText);
    auto callToken = FindTokenOnLeftOfPosition(ctx, position);
    const auto callNode = callToken->Parent();
    ASSERT_NE(callNode, nullptr);
    ASSERT_TRUE(callNode->IsCallExpression());
    ark::es2panda::ir::CallExpression *callExpr = nullptr;
    if (callNode->IsCallExpression()) {
        callExpr = callNode->AsCallExpression();
    }
    auto funcNode = FindTokenOnLeftOfPosition(ctx, position1);
    funcNode = funcNode->Parent();
    ASSERT_NE(funcNode, nullptr);
    ASSERT_TRUE(funcNode->IsMethodDefinition());
    ark::es2panda::ir::MethodDefinition *funcDecl = nullptr;
    if (funcNode->IsMethodDefinition()) {
        funcDecl = funcNode->AsMethodDefinition();
    }
    std::vector<ark::es2panda::checker::Signature *> signatures;
    signatures.push_back(callExpr->Signature());
    signatures.push_back(funcDecl->Function()->Signature());
    ark::es2panda::lsp::ArgumentListInfo argumentListInfo;
    const size_t argumentCount = 2;
    const size_t argumentIndex = 1;
    argumentListInfo.SetArgumentCount(argumentCount);
    argumentListInfo.SetArgumentIndex(argumentIndex);
    auto signatureHelpItems =
        ark::es2panda::lsp::CreateSignatureHelpItems(signatures, callExpr->Signature(), argumentListInfo);
    EXPECT_EQ(signatureHelpItems.GetSelectedItemIndex(), index0);
    signatureHelpItems.Clear();
    initializer.DestroyContext(ctx);
}

}  // namespace