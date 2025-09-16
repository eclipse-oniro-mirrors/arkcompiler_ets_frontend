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

#include "lsp_api_test.h"

#include <gtest/gtest.h>

#include "ir/astNode.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "public/public.h"

namespace {

using ark::es2panda::lsp::Initializer;

TEST_F(LSPAPITests, DiagnosticConstructorAndField)
{
    int const errorCode = 404;
    int const defaultCharacter = 10;
    std::vector<DiagnosticTag> tags {};
    std::vector<DiagnosticRelatedInformation> relatedInfoList {};
    Diagnostic diagnostic(range_, tags, relatedInfoList, DiagnosticSeverity::Error, errorCode, message_);

    EXPECT_EQ(diagnostic.range_.start.line_, 1);
    EXPECT_EQ(diagnostic.range_.end.character_, defaultCharacter);
    EXPECT_EQ(diagnostic.message_, message_);
    EXPECT_EQ(diagnostic.severity_, DiagnosticSeverity::Error);
    EXPECT_EQ(std::get<int>(diagnostic.code_), errorCode);
}

TEST_F(LSPAPITests, DiagnosticCodeDescriptionOptional)
{
    CodeDescription codeDesc;
    codeDesc.href_ = "http://example.com/error/404";
    int const errorCode = 404;
    std::vector<DiagnosticTag> tags {};
    std::vector<DiagnosticRelatedInformation> relatedInfoList {};

    Diagnostic diagnostic(range_, tags, relatedInfoList, DiagnosticSeverity::Error, errorCode, message_, codeDesc);

    const auto &codeDescription = diagnostic.codeDescription_;
    EXPECT_EQ(codeDescription.href_, "http://example.com/error/404");
}

TEST_F(LSPAPITests, DiagnosticTagsAndRelatedInformation)
{
    std::vector<DiagnosticTag> tags {};
    tags.push_back(DiagnosticTag::Unnecessary);
    std::vector<DiagnosticRelatedInformation> relatedInfoList {};
    DiagnosticRelatedInformation relatedInfo;
    relatedInfo.location_ = Location {"www.test.uri", range_};
    relatedInfo.message_ = "Related information message";
    relatedInfoList.push_back(relatedInfo);
    int const errorCode = 200;
    CodeDescription des = {};

    Diagnostic diagnostic(range_, tags, relatedInfoList, DiagnosticSeverity::Information, errorCode, message_, des,
                          "default");

    const auto &diagnosticTags = diagnostic.tags_;
    EXPECT_EQ(diagnosticTags.size(), 1);
    EXPECT_EQ(diagnosticTags[0], DiagnosticTag::Unnecessary);

    const auto &relatedInformation = diagnostic.relatedInformation_;
    EXPECT_EQ(relatedInformation.size(), 1);
    EXPECT_EQ(relatedInformation[0].message_, "Related information message");
}

TEST_F(LSPAPITests, DiagnosticDataField)
{
    int const dataValue = 42;
    std::variant<int, std::string> data = dataValue;
    int const errorCode = 400;
    int const dataResult = 42;
    std::vector<DiagnosticTag> tags {};
    std::vector<DiagnosticRelatedInformation> relatedInfoList {};

    Diagnostic diagnostic(range_, tags, relatedInfoList, DiagnosticSeverity::Error, errorCode, message_, {}, {}, data);

    const auto &diagnosticData = diagnostic.data_;
    EXPECT_EQ(std::get<int>(diagnosticData), dataResult);
}

TEST_F(LSPAPITests, CreateDiagnosticForNode1)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("file1.ets", ES2PANDA_STATE_CHECKED, "function main() {}");
    auto astNode = GetAstFromContext<es2panda_AstNode>(ctx);
    int const dataValue = 42;
    std::variant<int, std::string> data = dataValue;
    int const errorCode = 400;
    std::string message = "Diagnostic";
    std::vector<DiagnosticTag> tags {};
    std::vector<DiagnosticRelatedInformation> relatedInfoList {};

    Diagnostic diagnostic(range_, tags, relatedInfoList, DiagnosticSeverity::Error, errorCode, message, {}, {}, data);
    FileDiagnostic result = ark::es2panda::lsp::CreateDiagnosticForNode(astNode, diagnostic, ctx);

    int const startLine = 1;
    int const endLine = 1;
    int const startChar = 1;
    int const endChar = 19;
    ASSERT_EQ(result.diagnostic.message_, "Diagnostic");
    ASSERT_EQ(result.diagnostic.range_.start.line_, startLine);
    ASSERT_EQ(result.diagnostic.range_.end.line_, endLine);
    ASSERT_EQ(result.diagnostic.range_.start.character_, startChar);
    ASSERT_EQ(result.diagnostic.range_.end.character_, endChar);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPAPITests, CreateDiagnosticForNode2)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("file1.ets", ES2PANDA_STATE_CHECKED, "function main() {}");
    auto astNode = GetAstFromContext<es2panda_AstNode>(ctx);
    int const dataValue = 42;
    std::variant<int, std::string> data = dataValue;
    int const errorCode = 400;
    std::string message = "Diagnostic {0}, for the {1}, and {2}";
    std::vector<std::string> args = {"Error1", "Error2", "Error3"};
    std::vector<DiagnosticTag> tags {};
    std::vector<DiagnosticRelatedInformation> relatedInfoList {};

    Diagnostic diagnostic(range_, tags, relatedInfoList, DiagnosticSeverity::Error, errorCode, message, {}, {}, data);
    FileDiagnostic result = ark::es2panda::lsp::CreateDiagnosticForNode(astNode, diagnostic, ctx, args);

    int const startLine = 1;
    int const endLine = 1;
    int const startChar = 1;
    int const endChar = 19;
    ASSERT_EQ(result.diagnostic.message_, "Diagnostic Error1, for the Error2, and Error3");
    ASSERT_EQ(result.diagnostic.range_.start.line_, startLine);
    ASSERT_EQ(result.diagnostic.range_.end.line_, endLine);
    ASSERT_EQ(result.diagnostic.range_.start.character_, startChar);
    ASSERT_EQ(result.diagnostic.range_.end.character_, endChar);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPAPITests, CreateDiagnosticForNode3)
{
    using ark::es2panda::ir::AstNode;
    using ark::es2panda::public_lib::Context;
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("file1.ets", ES2PANDA_STATE_CHECKED, "let a =     () => {\n  return 1;\n}");
    auto astNode = reinterpret_cast<AstNode *>(GetAstFromContext<ark::es2panda::ir::AstNode>(ctx));
    astNode = astNode->FindChild([](ark::es2panda::ir::AstNode *child) { return child->IsArrowFunctionExpression(); });
    int const dataValue = 42;
    std::variant<int, std::string> data = dataValue;
    int const errorCode = 400;
    std::string message = "Diagnostic {0}, for the {1}, and {2}";
    std::vector<std::string> args = {"Error1", "Error2", "Error3"};
    std::vector<DiagnosticTag> tags {};
    std::vector<DiagnosticRelatedInformation> relatedInfoList {};

    Diagnostic diagnostic(range_, tags, relatedInfoList, DiagnosticSeverity::Error, errorCode, message, {}, {}, data);
    FileDiagnostic result = ark::es2panda::lsp::CreateDiagnosticForNode(reinterpret_cast<es2panda_AstNode *>(astNode),
                                                                        diagnostic, ctx, args);

    int const startLine = 1;
    int const endLine = 3;
    int const startChar = 13;
    int const endChar = 2;
    ASSERT_EQ(result.diagnostic.message_, "Diagnostic Error1, for the Error2, and Error3");
    ASSERT_EQ(result.diagnostic.range_.start.line_, startLine);
    ASSERT_EQ(result.diagnostic.range_.end.line_, endLine);
    ASSERT_EQ(result.diagnostic.range_.start.character_, startChar);
    ASSERT_EQ(result.diagnostic.range_.end.character_, endChar);
    initializer.DestroyContext(ctx);
}

}  // namespace
