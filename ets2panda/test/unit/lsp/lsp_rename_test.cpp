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

#include "lsp/include/rename.h"
#include <gtest/gtest.h>
#include <cstddef>
#include <cstdio>
#include <optional>
#include <string>
#include "lexer/token/letters.h"
#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include "util/path.h"

using ark::es2panda::lsp::Initializer;

class LspRenameInfoTests : public LSPAPITests {
public:
    std::string GetPandalibPath(const ark::es2panda::ir::AstNode *consoleDecl)
    {
        auto consoleFilePath = std::string(consoleDecl->Range().start.Program()->SourceFile().GetAbsolutePath().Utf8());
        size_t lastDelimPos = consoleFilePath.find_last_of(ark::es2panda::util::PATH_DELIMITER);
        std::string consoleDir =
            (lastDelimPos != std::string::npos) ? consoleFilePath.substr(0, lastDelimPos) : consoleFilePath;
        std::string pandaLibPath = consoleDir;
        size_t pos = 0;
        const int threeLevelsUp = 3;
        for (int i = 0; i < threeLevelsUp; ++i) {
            pos = pandaLibPath.find_last_of(ark::es2panda::util::PATH_DELIMITER);
            if (pos != std::string::npos) {
                pandaLibPath = pandaLibPath.substr(0, pos);
            }
        }
        return pandaLibPath;
    }
};

TEST_F(LspRenameInfoTests, RenameInfoCreateTriggerSpanForNodeNumberLiteral)
{
    Initializer initializer = Initializer();

    es2panda_Context *ctx = initializer.CreateContext("trigger-span-literal.ets", ES2PANDA_STATE_CHECKED,
                                                      "let number_literal: number = 1234\n");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto targetNode = ast->FindChild([](ark::es2panda::ir::AstNode *node) {
        return node->IsNumberLiteral() && node->AsNumberLiteral()->Str() == "1234";
    });
    ASSERT_NE(targetNode, nullptr);

    auto result = ark::es2panda::lsp::CreateTriggerSpanForNode(targetNode);
    size_t const expectedStart = 29;
    size_t const expectedLength = 4;
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoCreateTriggerSpanForNodeStringLiteral)
{
    Initializer initializer = Initializer();

    es2panda_Context *ctx = initializer.CreateContext("trigger-span-string-literal.ets", ES2PANDA_STATE_CHECKED,
                                                      "let string_literal: string = \"hello\";\n");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto targetNode = ast->FindChild([](ark::es2panda::ir::AstNode *node) {
        return node->IsStringLiteral() && node->AsStringLiteral()->Str() == "hello";
    });
    ASSERT_NE(targetNode, nullptr);

    auto result = ark::es2panda::lsp::CreateTriggerSpanForNode(targetNode);
    size_t const expectedStart = 30;
    size_t const expectedLength = 5;
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoCreateTriggerSpanForNodeEmptyStringLiteral)
{
    Initializer initializer = Initializer();

    es2panda_Context *ctx =
        initializer.CreateContext("trigger-span-empty-string.ets", ES2PANDA_STATE_CHECKED, "let emptyStr = \"\";");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto targetNode = ast->FindChild([](ark::es2panda::ir::AstNode *node) {
        return node->IsStringLiteral() && node->AsStringLiteral()->Str() == "";
    });
    ASSERT_NE(targetNode, nullptr);

    auto result = ark::es2panda::lsp::CreateTriggerSpanForNode(targetNode);
    size_t const expectedStart = 16;
    size_t const expectedLength = 0;
    ASSERT_EQ(result.start, expectedStart);
    ASSERT_EQ(result.length, expectedLength);
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoNodeReturnTrueIfNodeIsNull)
{
    ark::es2panda::ir::AstNode *nullNode = nullptr;
    bool result = ark::es2panda::lsp::NodeIsMissing(nullNode);
    EXPECT_TRUE(result);
}

TEST_F(LspRenameInfoTests, RenameInfoNodeReturnTrueIfStartEqualEnd)
{
    Initializer initializer = Initializer();

    es2panda_Context *ctx = initializer.CreateContext("StartEqualEnd.ets", ES2PANDA_STATE_CHECKED, "");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto program = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram;

    size_t const expectedIndex = 10;
    size_t const expectedLength = 20;
    auto srcPositionStart = ark::es2panda::lexer::SourcePosition(expectedIndex, expectedLength, program);
    auto srcPositionEnd = ark::es2panda::lexer::SourcePosition(expectedIndex, expectedLength, program);
    auto srcRange = ark::es2panda::lexer::SourceRange(srcPositionStart, srcPositionEnd);

    ast->SetRange(srcRange);
    bool result = ark::es2panda::lsp::NodeIsMissing(ast);
    EXPECT_TRUE(result);
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoNodeReturnFalseIfStartNotEqualEnd)
{
    Initializer initializer = Initializer();

    es2panda_Context *ctx = initializer.CreateContext("StartNotEqualEnd.ets", ES2PANDA_STATE_CHECKED, "");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto program = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram;
    size_t const expectedStartIndex = 10;
    size_t const expectedEndIndex = 15;
    size_t const expectedLength = 20;
    auto srcPositionStart = ark::es2panda::lexer::SourcePosition(expectedStartIndex, expectedLength, program);
    auto srcPositionEnd = ark::es2panda::lexer::SourcePosition(expectedEndIndex, expectedLength, program);
    auto srcRange = ark::es2panda::lexer::SourceRange(srcPositionStart, srcPositionEnd);

    ast->SetRange(srcRange);
    bool result = ark::es2panda::lsp::NodeIsMissing(ast);
    EXPECT_FALSE(result);
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoNodeGetSourceTextOfNodeFromSourceFile)
{
    Initializer initializer = Initializer();

    auto sourceView = ark::es2panda::util::StringView("function A(a:number, b:number) {\n  return a + b;\n}\nA(1, 2);");
    es2panda_Context *ctx = initializer.CreateContext("GetTextOfNodeFromSourceFile.ets", ES2PANDA_STATE_CHECKED,
                                                      "function A(a:number, b:number) {\n  return a + b;\n}\nA(1, 2);");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto program = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram;
    size_t const startIndex = 9;
    size_t const startLength = 5;
    size_t const endIndex = 10;
    size_t const endLength = 5;
    auto srcPositionStart = ark::es2panda::lexer::SourcePosition(startIndex, startLength, program);
    auto srcPositionEnd = ark::es2panda::lexer::SourcePosition(endIndex, endLength, program);
    auto srcRange = ark::es2panda::lexer::SourceRange(srcPositionStart, srcPositionEnd);
    ast->SetRange(srcRange);

    std::string result = ark::es2panda::lsp::GetSourceTextOfNodeFromSourceFile(sourceView, ast);
    ASSERT_EQ(result, "A");
    ASSERT_NE(result, "B");
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoNodeGetSourceTextOfNodeFromSourceFileNullPtr)
{
    auto sourceView = ark::es2panda::util::StringView("function A(a:number, b:number) {\n  return a + b;\n}\nA(1, 2);");
    ark::es2panda::ir::AstNode *ast = nullptr;
    std::string result = ark::es2panda::lsp::GetSourceTextOfNodeFromSourceFile(sourceView, ast);
    ASSERT_NE(result, "A");
}

TEST_F(LspRenameInfoTests, RenameInfoNodeIsAncestorType)
{
    ASSERT_EQ(ark::es2panda::lsp::IsValidAncestorType(ark::es2panda::ir::AstNodeType::TS_ANY_KEYWORD), true);
    ASSERT_EQ(ark::es2panda::lsp::IsValidAncestorType(ark::es2panda::ir::AstNodeType::TS_UNKNOWN_KEYWORD), true);
    ASSERT_EQ(ark::es2panda::lsp::IsValidAncestorType(ark::es2panda::ir::AstNodeType::TS_NUMBER_KEYWORD), true);
    ASSERT_EQ(ark::es2panda::lsp::IsValidAncestorType(ark::es2panda::ir::AstNodeType::TS_BIGINT_KEYWORD), true);
    ASSERT_EQ(ark::es2panda::lsp::IsValidAncestorType(ark::es2panda::ir::AstNodeType::TS_OBJECT_KEYWORD), true);
    ASSERT_EQ(ark::es2panda::lsp::IsValidAncestorType(ark::es2panda::ir::AstNodeType::TS_BOOLEAN_KEYWORD), true);
    ASSERT_EQ(ark::es2panda::lsp::IsValidAncestorType(ark::es2panda::ir::AstNodeType::TS_STRING_KEYWORD), true);
    ASSERT_EQ(ark::es2panda::lsp::IsValidAncestorType(ark::es2panda::ir::AstNodeType::TS_VOID_KEYWORD), true);
    ASSERT_EQ(ark::es2panda::lsp::IsValidAncestorType(ark::es2panda::ir::AstNodeType::TS_UNDEFINED_KEYWORD), true);
    ASSERT_EQ(ark::es2panda::lsp::IsValidAncestorType(ark::es2panda::ir::AstNodeType::TS_NEVER_KEYWORD), true);
}

TEST_F(LspRenameInfoTests, RenameInfoNodeIsNotAncestorType)
{
    ASSERT_NE(ark::es2panda::lsp::IsValidAncestorType(ark::es2panda::ir::AstNodeType::TS_ARRAY_TYPE), true);
    ASSERT_NE(ark::es2panda::lsp::IsValidAncestorType(ark::es2panda::ir::AstNodeType::TS_FUNCTION_TYPE), true);

    size_t const invalidNum = 999;
    auto invalidType = static_cast<ark::es2panda::ir::AstNodeType>(invalidNum);
    ASSERT_NE(ark::es2panda::lsp::IsValidAncestorType(invalidType), true);
}

TEST_F(LspRenameInfoTests, RenameInfoNodeIsQuoteOrBacktick)
{
    int charCode = ark::es2panda::lexer::LEX_CHAR_DOUBLE_QUOTE;
    ASSERT_TRUE(ark::es2panda::lsp::IsQuoteOrBacktick(charCode));

    charCode = ark::es2panda::lexer::LEX_CHAR_0;
    ASSERT_FALSE(ark::es2panda::lsp::IsQuoteOrBacktick(charCode));
}

TEST_F(LspRenameInfoTests, RenameInfoIsStringOrNumericLiteralLike)
{
    Initializer initializer = Initializer();

    es2panda_Context *ctx = initializer.CreateContext("trigger-span-string-literal.ets", ES2PANDA_STATE_CHECKED,
                                                      "let string_literal: string = \"hello\";\n");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto targetNode = ast->FindChild([](ark::es2panda::ir::AstNode *node) {
        return node->IsStringLiteral() && node->AsStringLiteral()->Str() == "hello";
    });

    ASSERT_TRUE(ark::es2panda::lsp::IsStringOrNumericLiteralLike(targetNode));
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoStripQuotes)
{
    std::string hello = "'hello'";
    ASSERT_EQ(ark::es2panda::lsp::StripQuotes(hello), "hello");
}

TEST_F(LspRenameInfoTests, RenameInfoGetNodeKindForRenameInfo)
{
    Initializer initializer = Initializer();

    es2panda_Context *ctx = initializer.CreateContext("token-pos-identifier.ets", ES2PANDA_STATE_CHECKED,
                                                      "function A(a:number, b:number) {\n  return a + b;\n}\nA(1, 2);");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    ASSERT_EQ(ark::es2panda::lsp::GetNodeKindForRenameInfo(ast), "");
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoGetTextOfNode)
{
    Initializer initializer = Initializer();

    std::vector<std::string> files = {"reanme_export1.ets"};
    std::vector<std::string> texts = {
        R"(export function A(a:number, b:number): number {return a + b;}
        export function B(a:number, b:number): number {return a + b;})"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    char const *fileName = filePaths[0].c_str();
    auto ctx = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);

    size_t const startIndex = 16;
    size_t const startLength = 5;
    size_t const endIndex = 17;
    size_t const endLength = 5;

    auto program = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram;
    auto srcPositionStart = ark::es2panda::lexer::SourcePosition(startIndex, startLength, program);
    auto srcPositionEnd = ark::es2panda::lexer::SourcePosition(endIndex, endLength, program);
    auto srcRange = ark::es2panda::lexer::SourceRange(srcPositionStart, srcPositionEnd);
    ast->SetRange(srcRange);

    ASSERT_EQ(ark::es2panda::lsp::GetTextOfNode(ast, program), "A");
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoGetRenameInfoError)
{
    ark::es2panda::lsp::RenameInfoFailure renameFailureFromFunction =
        ark::es2panda::lsp::GetRenameInfoError("errorMessage");
    ark::es2panda::lsp::RenameInfoFailure renameFailure = ark::es2panda::lsp::RenameInfoFailure(false, "errorMessage");
    ASSERT_EQ(renameFailure.GetLocalizedErrorMessage(), renameFailureFromFunction.GetLocalizedErrorMessage());
    ASSERT_EQ(renameFailure.GetCanRenameFailure(), renameFailureFromFunction.GetCanRenameFailure());
}

TEST_F(LspRenameInfoTests, RenameInfoGetRenameInfoSuccess)
{
    Initializer initializer = Initializer();

    es2panda_Context *ctx = initializer.CreateContext("token-pos-identifier.ets", ES2PANDA_STATE_CHECKED,
                                                      "function A(a:number, b:number) {\n  return a + b;\n}\nA(1, 2);");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);

    ark::es2panda::lsp::RenameInfoSuccess renameSuccessFromFunction =
        ark::es2panda::lsp::GetRenameInfoSuccess("displayName", "fullDisplayName", "", "", ast);

    ark::es2panda::lsp::RenameInfoSuccess renameSuccess =
        ark::es2panda::lsp::RenameInfoSuccess(true, "", "", "displayName", "fullDisplayName", "", TextSpan(1, 2));

    ASSERT_EQ(renameSuccess.GetDisplayName(), renameSuccessFromFunction.GetDisplayName());
    ASSERT_EQ(renameSuccess.GetFullDisplayName(), renameSuccessFromFunction.GetFullDisplayName());
    ASSERT_EQ(renameSuccess.GetKind(), renameSuccessFromFunction.GetKind());
    ASSERT_EQ(renameSuccess.GetKindModifiers(), renameSuccessFromFunction.GetKindModifiers());
    ASSERT_EQ(renameSuccess.GetCanRenameSuccess(), renameSuccessFromFunction.GetCanRenameSuccess());

    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoTryGetImportFromModuleSpecifier)
{
    Initializer initializer = Initializer();

    std::vector<std::string> files = {"rename-module.ets"};
    std::vector<std::string> texts = {R"(import { PI } from "std/math/consts";
    console.log(PI);)"};

    auto filePaths = CreateTempFile(files, texts);
    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const startIndex = 0;
    char const *referenceFileName = filePaths[startIndex].c_str();
    auto ctx = initializer.CreateContext(referenceFileName, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    ark::es2panda::ir::AstNode *ast = nullptr;
    ASSERT_EQ(ark::es2panda::lsp::TryGetImportFromModuleSpecifier(ast), nullptr);

    ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto targetNode = ast->FindChild([](ark::es2panda::ir::AstNode *node) {
        return node->Type() == ark::es2panda::ir::AstNodeType::IMPORT_SPECIFIER;
    });
    ASSERT_EQ(ark::es2panda::lsp::TryGetImportFromModuleSpecifier(targetNode), targetNode->Parent());

    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoGetRenameInfoForModuleCaseModule1)
{
    Initializer initializer = Initializer();

    std::vector<std::string> files = {"rename-module.ets"};
    std::vector<std::string> texts = {R"(const s = "std/math/index";)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedSize = 1;
    ASSERT_EQ(filePaths.size(), expectedSize);

    size_t const startIndex = 0;
    char const *filePath = filePaths[startIndex].c_str();
    es2panda_Context *ctx = initializer.CreateContext(filePath, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto targetNode = ast->FindChild([](ark::es2panda::ir::AstNode *node) {
        return node->IsStringLiteral() && node->AsStringLiteral()->ToString() == "std/math/index";
    });
    ASSERT_NE(targetNode, nullptr);

    auto renameInfo = ark::es2panda::lsp::GetRenameInfoForModule(
        targetNode, reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram);
    ASSERT_EQ(renameInfo.value().GetDisplayName(), targetNode->AsStringLiteral()->ToString());
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoGetRenameInfoForModuleCaseModule2)
{
    Initializer initializer = Initializer();

    std::vector<std::string> files = {"rename-module.ets"};
    std::vector<std::string> texts = {R"(const s = "std/math.ets";)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedSize = 1;
    ASSERT_EQ(filePaths.size(), expectedSize);

    size_t const startIndex = 0;
    char const *filePath = filePaths[startIndex].c_str();
    es2panda_Context *ctx = initializer.CreateContext(filePath, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto targetNode = ast->FindChild([](ark::es2panda::ir::AstNode *node) {
        return node->IsStringLiteral() && node->AsStringLiteral()->ToString() == "std/math.ets";
    });
    ASSERT_NE(targetNode, nullptr);

    auto renameInfo = ark::es2panda::lsp::GetRenameInfoForModule(
        targetNode, reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram);
    ASSERT_EQ(renameInfo.value().GetDisplayName(), targetNode->AsStringLiteral()->ToString());
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoGetRenameInfoForModuleCaseDirectory)
{
    Initializer initializer = Initializer();

    std::vector<std::string> files = {"rename-directory.ets"};
    std::vector<std::string> texts = {R"(const s = "std/math/index.ets";)"};
    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedSize = 1;
    ASSERT_EQ(filePaths.size(), expectedSize);

    size_t const startIndex = 0;
    char const *filePath = filePaths[startIndex].c_str();
    es2panda_Context *ctx = initializer.CreateContext(filePath, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto targetNode = ast->FindChild([](ark::es2panda::ir::AstNode *node) {
        return node->IsStringLiteral() && node->AsStringLiteral()->ToString() == "std/math/index.ets";
    });
    ASSERT_NE(targetNode, nullptr);

    auto renameInfo = ark::es2panda::lsp::GetRenameInfoForModule(
        targetNode, reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram);
    ASSERT_EQ(renameInfo.value().GetDisplayName(), "std/math");
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoGetRenameInfoForNode1)
{
    Initializer initializer = Initializer();

    std::vector<std::string> files = {"rename.ets"};
    std::vector<std::string> texts = {R"(import { PI } from "std/math/consts"; console.log(PI);)"};

    auto filePaths = CreateTempFile(files, texts);
    size_t const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    size_t const startIndex = 0;
    char const *referenceFileName = filePaths[startIndex].c_str();
    auto ctx = initializer.CreateContext(referenceFileName, ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto consoleNode = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == "console";
    });
    auto consoleDecl = consoleNode->Variable()->Declaration()->Node();
    std::string pandaLibPath = GetPandalibPath(consoleDecl);
    auto checker = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->GetChecker()->AsETSChecker();
    auto program = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx)->parserProgram;

    auto targetNode = ast->FindChild([](ark::es2panda::ir::AstNode *node) {
        return node->IsIdentifier() && node->AsIdentifier()->ToString() == "PI";
    });
    ASSERT_EQ(ark::es2panda::lsp::GetRenameInfoForNode(targetNode, checker, program, pandaLibPath), std::nullopt);
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoNodeIsEligibleForRenameStringLiteral)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("trigger-span-string-literal.ets", ES2PANDA_STATE_CHECKED,
                                                      "let string_literal: string = \"hello\";\n");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto targetNode = ast->FindChild([](ark::es2panda::ir::AstNode *node) {
        return node->IsStringLiteral() && node->AsStringLiteral()->Str() == "hello";
    });

    ASSERT_NE(targetNode, nullptr);
    ASSERT_TRUE(ark::es2panda::lsp::NodeIsEligibleForRename(targetNode));
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, RenameInfoNodeIsEligibleForRenameNumberLiteralEnumMember)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("trigger-span-literal.ets", ES2PANDA_STATE_CHECKED,
                                                      "enum MyEnum { FixedValue = 42 }");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto enumClass = ast->FindChild([](ark::es2panda::ir::AstNode *child) {
        return child->IsClassDefinition() && child->AsClassDefinition()->Ident()->Name() == "MyEnum";
    });
    auto enumDecl = enumClass->AsClassDefinition()->OrigEnumDecl()->AsTSEnumDeclaration();
    auto targetNode = enumDecl->FindChild([](ark::es2panda::ir::AstNode *node) {
        return node->IsNumberLiteral() && node->AsNumberLiteral()->Str() == "42";
    });

    ASSERT_NE(targetNode, nullptr);
    ASSERT_FALSE(ark::es2panda::lsp::NodeIsEligibleForRename(targetNode));
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, GetRenameInfo1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("GetRenameInfo1.ets", ES2PANDA_STATE_CHECKED,
                                                      "let aaa = 123;let bbb = aaa + 123;console.log(\"456\")");
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto consoleNode = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == "console";
    });
    auto consoleDecl = consoleNode->Variable()->Declaration()->Node();
    std::string pandaLibPath = GetPandalibPath(consoleDecl);
    LSPAPI const *lspApi = GetImpl();
    const size_t varIndex = 6;
    auto varResult = lspApi->getRenameInfo(ctx, varIndex, const_cast<char *>(pandaLibPath.c_str()));
    ASSERT(std::holds_alternative<ark::es2panda::lsp::RenameInfoSuccess>(varResult));
    auto varRenameInfo = std::get<ark::es2panda::lsp::RenameInfoSuccess>(varResult);
    const size_t expectedStart = 4;
    const size_t expectedLength = 3;
    ASSERT_EQ(varRenameInfo.GetCanRenameSuccess(), true);
    ASSERT_EQ(varRenameInfo.GetKind(), "property");
    ASSERT_EQ(varRenameInfo.GetDisplayName(), "aaa");
    ASSERT_EQ(varRenameInfo.GetFullDisplayName(), "aaa");
    ASSERT_EQ(varRenameInfo.GetTriggerSpan().start, expectedStart);
    ASSERT_EQ(varRenameInfo.GetTriggerSpan().length, expectedLength);
    const size_t consoleIndex = 38;
    auto consoleResult = lspApi->getRenameInfo(ctx, consoleIndex, const_cast<char *>(pandaLibPath.c_str()));
    ASSERT(std::holds_alternative<ark::es2panda::lsp::RenameInfoFailure>(consoleResult));
    auto consoleRenameInfo = std::get<ark::es2panda::lsp::RenameInfoFailure>(consoleResult);
    ASSERT_EQ(consoleRenameInfo.GetCanRenameFailure(), false);
    ASSERT_EQ(consoleRenameInfo.GetLocalizedErrorMessage(), "You cannot rename this element");
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, GetInterfaceRenameInfo1)
{
    const std::string fileContent = R"(
console.log("Hello World");
interface Person {
  name: string;
  age: number;
}

let p1: Person = {name:"John", age:123};

console.log(p1.name);
console.log(p1.age);
)";
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("GetInterfaceRenameInfo1.ets", ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto consoleNode = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == "console";
    });
    auto consoleDecl = consoleNode->Variable()->Declaration()->Node();
    std::string pandaLibPath = GetPandalibPath(consoleDecl);
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getRenameInfo(ctx, fileContent.find("name)"), const_cast<char *>(pandaLibPath.c_str()));
    ASSERT(std::holds_alternative<ark::es2panda::lsp::RenameInfoSuccess>(result));
    initializer.DestroyContext(ctx);
}

TEST_F(LspRenameInfoTests, GetConstVariableRenameInfo)
{
    const std::string fileContent = R"(
const demo1 = 1;
class test {
    a =  demo1;
}
console.log(demo1);
)";
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("GetConstVariableRenameInfo.ets", ES2PANDA_STATE_CHECKED, fileContent.c_str());
    auto ast = GetAstFromContext<ark::es2panda::ir::AstNode>(ctx);
    auto consoleNode = ast->FindChild([](ark::es2panda::ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == "console";
    });
    auto consoleDecl = consoleNode->Variable()->Declaration()->Node();
    std::string pandaLibPath = GetPandalibPath(consoleDecl);
    LSPAPI const *lspApi = GetImpl();

    auto result = lspApi->getRenameInfo(ctx, fileContent.find("demo1;"), const_cast<char *>(pandaLibPath.c_str()));
    ASSERT(std::holds_alternative<ark::es2panda::lsp::RenameInfoSuccess>(result));
    initializer.DestroyContext(ctx);
}