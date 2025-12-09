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

#include "lsp/include/code_fix_provider.h"
#include "gtest/gtest.h"
#include "lsp_api_test.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/register_code_fix/convert_const_to_let.h"
#include "generated/code_fix_register.h"

namespace {
constexpr int DEFAULT_THROTTLE = 20;
constexpr auto ERROR_CODES = ark::es2panda::lsp::codefixes::FIX_CONVERT_CONST_TO_LET.GetSupportedCodeNumbers();
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Convert const to let";
constexpr std::string_view EXPECTED_TEXT_CHANGE_NEW_TEXT = "let";
constexpr auto EXPECTED_FIX_NAME = ark::es2panda::lsp::codefixes::FIX_CONVERT_CONST_TO_LET.GetFixId();
constexpr int ERROR_CODE_EXAMPLE = 900;    // Example error code for testing
constexpr int ERROR_CODE_EXAMPLE_2 = 901;  // Another example error code for testing
constexpr unsigned int END_RANGE_EXAMPLE = 10;

class DummyCodeFixRegistration : public ark::es2panda::lsp::CodeFixRegistration {
public:
    std::vector<ark::es2panda::lsp::CodeFixAction> GetCodeActions(
        [[maybe_unused]] const ark::es2panda::lsp::CodeFixContext &context) override
    {
        return {};
    }

    ark::es2panda::lsp::CombinedCodeActions GetAllCodeActions(
        [[maybe_unused]] const ark::es2panda::lsp::CodeFixAllContext &context) override
    {
        return {};
    }
};

class CodeFixProviderTest : public LSPAPITests {
protected:
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
    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto pos = index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
        return pos;
    }
    static void ValidateCodeFixActionInfo(const CodeFixActionInfo &info, const size_t expectedTextChangeStart,
                                          const size_t expectedTextChangeLength, const std::string &expectedFileName)
    {
        ASSERT_EQ(info.fixName_, EXPECTED_FIX_NAME);
        ASSERT_EQ(info.fixId_, EXPECTED_FIX_NAME);
        ASSERT_EQ(info.description_, EXPECTED_FIX_DESCRIPTION);
        ASSERT_EQ(info.changes_[0].fileName, expectedFileName);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.start, expectedTextChangeStart);
        ASSERT_EQ(info.changes_[0].textChanges[0].span.length, expectedTextChangeLength);
        ASSERT_EQ(info.changes_[0].textChanges[0].newText, EXPECTED_TEXT_CHANGE_NEW_TEXT);
    }
};

TEST_F(CodeFixProviderTest, CreatesCodeFixActionWithoutFixAll)
{
    std::string fixName = "testFix";
    std::vector<FileTextChanges> changes = {
        ark::es2panda::lsp::CodeFixProvider::Instance().CreateFileTextChanges("CodeFixProviderTest.ets", {})};

    ark::es2panda::lsp::codefixes::DiagnosticCode diag(ark::es2panda::util::DiagnosticType::SEMANTIC,
                                                       ERROR_CODE_EXAMPLE, "Test message without arguments");

    auto action =
        ark::es2panda::lsp::CodeFixProvider::Instance().CreateCodeFixActionWithoutFixAll(fixName, changes, diag);

    EXPECT_EQ(action.fixName, fixName);
    EXPECT_EQ(action.fixAllDescription, "");
    EXPECT_EQ(action.fixId, "");
    EXPECT_EQ(action.description, "Test message without arguments");
    EXPECT_EQ(action.changes.size(), 1);
    EXPECT_EQ(action.changes[0].fileName, "CodeFixProviderTest.ets");
}

TEST_F(CodeFixProviderTest, CreatesCodeFixActionWithFixAll)
{
    std::string fixName = "fixWithAll";
    std::string fixId = "fix-all-id";
    ark::es2panda::lsp::codefixes::DiagnosticCode diag(ark::es2panda::util::DiagnosticType::SEMANTIC,
                                                       ERROR_CODE_EXAMPLE, "Message");

    std::vector<FileTextChanges> changes = {
        ark::es2panda::lsp::CodeFixProvider::Instance().CreateFileTextChanges("CodeFixProviderFile.ets", {})};

    ark::es2panda::lsp::CodeActionCommand cmd;
    cmd.type = "commandName";
    std::vector<ark::es2panda::lsp::CodeActionCommand> commands = {cmd};

    auto action =
        ark::es2panda::lsp::CodeFixProvider::Instance().CreateCodeFixAction(fixName, changes, diag, fixId, commands);

    EXPECT_EQ(action.fixName, fixName);
    EXPECT_EQ(action.fixId, fixId);
    EXPECT_EQ(action.description, "Message");
    EXPECT_EQ(action.fixAllDescription, "Fix all: Message");
    EXPECT_EQ(action.changes.size(), 1);
    EXPECT_EQ(action.changes[0].fileName, "CodeFixProviderFile.ets");
    EXPECT_FALSE(action.commands.empty());
    EXPECT_EQ(action.commands[0].type, "commandName");
}

TEST_F(CodeFixProviderTest, DiagnosticToStringHandlesMessage)
{
    ark::es2panda::lsp::codefixes::DiagnosticCode diagNoArgs(ark::es2panda::util::DiagnosticType::SEMANTIC,
                                                             ERROR_CODE_EXAMPLE, "No args message");
    EXPECT_EQ(ark::es2panda::lsp::CodeFixProvider::Instance().DiagnosticToString(diagNoArgs), "No args message");
    ark::es2panda::lsp::codefixes::DiagnosticCode diagWithPlaceholder(ark::es2panda::util::DiagnosticType::SEMANTIC,
                                                                      ERROR_CODE_EXAMPLE_2, "With {0} placeholder");
    // Even though formatting is not implemented yet, it should return the raw message
    EXPECT_EQ(ark::es2panda::lsp::CodeFixProvider::Instance().DiagnosticToString(diagWithPlaceholder),
              "With {0} placeholder");
}

TEST_F(CodeFixProviderTest, GetSupportedErrorCodesReturnsNonEmpty)
{
    auto supported = ark::es2panda::lsp::CodeFixProvider::Instance().GetSupportedErrorCodes();
    EXPECT_FALSE(supported.empty());
}

TEST_F(CodeFixProviderTest, ShouldIncludeFixAllBehavior)
{
    DummyCodeFixRegistration dummyReg;
    dummyReg.SetErrorCodes({ERROR_CODE_EXAMPLE});

    // Common dummy data
    Range dummyRange {{0, 0}, {0, END_RANGE_EXAMPLE}};
    std::vector<DiagnosticTag> tags;
    std::vector<DiagnosticRelatedInformation> relatedInfo;
    DiagnosticSeverity severity = DiagnosticSeverity::Error;

    Diagnostic diag1(dummyRange, tags, relatedInfo, severity, ERROR_CODE_EXAMPLE, "Error 900");
    Diagnostic diag2(dummyRange, tags, relatedInfo, severity, ERROR_CODE_EXAMPLE_2, "Error 901");

    std::vector<Diagnostic> single = {diag1};
    std::vector<Diagnostic> multiple = {diag1, diag2, diag1};

    EXPECT_FALSE(ark::es2panda::lsp::CodeFixProvider::Instance().ShouldIncludeFixAll(dummyReg, single));
    EXPECT_TRUE(ark::es2panda::lsp::CodeFixProvider::Instance().ShouldIncludeFixAll(dummyReg, multiple));
}

TEST_F(CodeFixProviderTest, TestFixExample)
{
    std::vector<std::string> fileNames = {"TestFixExample.ets"};
    std::vector<std::string> fileContents = {R"(
const a:Int = 0;
a = 1;
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    ASSERT_EQ(fileNames.size(), filePaths.size());
    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = LineColToPos(context, 3, 1);
    const size_t length = 1;
    const size_t expectedTextChangeStart = 1;
    const size_t expectedTextChangeLength = 5;
    const int expectedFixResultSize = 2;
    const int expectedcombinedFixResultSize = 1;
    const int expectedCombinedTextChangesSize = 1;

    std::vector<int> errorCodes(ERROR_CODES.begin(), ERROR_CODES.end());
    ark::es2panda::lsp::CancellationToken cancelationToken(DEFAULT_THROTTLE, &GetNullHost());
    CodeFixOptions emptyOptions = {cancelationToken, ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, emptyOptions);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    ValidateCodeFixActionInfo(fixResult[0], expectedTextChangeStart, expectedTextChangeLength, filePaths[0]);

    CombinedCodeActionsInfo combinedFixResult = ark::es2panda::lsp::GetCombinedCodeFixImpl(
        context, std::string(ark::es2panda::lsp::codefixes::FIX_CONVERT_CONST_TO_LET.GetFixId()), emptyOptions);
    ASSERT_EQ(combinedFixResult.changes_.size(), expectedcombinedFixResultSize);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges.size(), expectedCombinedTextChangesSize);
    ASSERT_EQ(combinedFixResult.changes_[0].fileName, filePaths[0]);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[0].span.start, expectedTextChangeStart);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[0].span.length, expectedTextChangeLength);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges[0].newText, EXPECTED_TEXT_CHANGE_NEW_TEXT.data());
    initializer.DestroyContext(context);
}

}  // namespace