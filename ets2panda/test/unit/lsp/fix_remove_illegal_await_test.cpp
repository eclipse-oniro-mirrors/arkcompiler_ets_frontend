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

#include "generated/code_fix_register.h"
#include "lsp_api_test.h"
#include "util/diagnostic.h"

#include <algorithm>
#include <gtest/gtest.h>

#include "lsp/include/api.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/register_code_fix/fix_remove_illegal_await.h"

namespace {

using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::codefixes::FIX_REMOVE_ILLEGAL_AWAIT;

constexpr std::string_view EXPECTED_FIX_NAME = FIX_REMOVE_ILLEGAL_AWAIT.GetFixId();
constexpr std::string_view EXPECTED_FIX_DESCRIPTION = "Add async modifier to containing function";
constexpr auto ERROR_CODES = FIX_REMOVE_ILLEGAL_AWAIT.GetSupportedCodeNumbers();
// AWAIT_IN_ARROW_FUN_PARAM: DiagnosticType::SYNTAX * DIAGNOSTIC_CODE_MULTIPLIER + 46
constexpr int AWAIT_IN_ARROW_FUN_PARAM_CODE = 1046;
// AWAIT_IN_NON_ASYNC_DEPRECATED: DiagnosticType::SEMANTIC * DIAGNOSTIC_CODE_MULTIPLIER + 173979
constexpr int AWAIT_IN_NON_ASYNC_DEPRECATED_CODE = 175979;
constexpr int DEFAULT_THROTTLE = 20;
constexpr std::string_view MIXED_FUNCTION_KINDS_SOURCE = R"(
class A {
    constructor() {
        await Promise.resolve();
    }
    get value(): int {
        await Promise.resolve();
        return 1;
    }
    method(): void {
        await Promise.resolve();
    }
}
export function foo<T>(value: T): T {
    await Promise.resolve();
    return value;
}
async function outer(): Promise<void> {
    let inner = (): void => {
        await Promise.resolve();
    };
}
)";
constexpr std::string_view MIXED_FUNCTION_KINDS_EXPECTED = R"(
class A {
    constructor() {
        await Promise.resolve();
    }
    get value(): int {
        await Promise.resolve();
        return 1;
    }
    async method(): Promise<void> {
        await Promise.resolve();
    }
}
export async function foo<T>(value: T): Promise<T> {
    await Promise.resolve();
    return value;
}
async function outer(): Promise<void> {
    let inner = async (): Promise<void> => {
        await Promise.resolve();
    };
}
)";

class FixRemoveIllegalAwaitTests : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        auto pos = index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
        return pos;
    }

    static std::pair<size_t, size_t> LocationOfText(es2panda_Context *context, std::string_view text)
    {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto source = ctx->parserProgram->SourceCode();
        auto offset = source.find(text);
        EXPECT_NE(offset, std::string_view::npos);

        auto index = ark::es2panda::lexer::LineIndex(source);
        return index.GetLocation(offset);
    }

    static std::string ApplyTextChanges(std::string text, std::vector<TextChange> changes)
    {
        std::sort(changes.begin(), changes.end(),
                  [](const TextChange &left, const TextChange &right) { return left.span.start > right.span.start; });

        for (const auto &change : changes) {
            text.replace(change.span.start, change.span.length, change.newText);
        }

        return text;
    }

    static const Diagnostic *FindDiagnosticByMessage(const DiagnosticReferences &diagnostics,
                                                     const std::string &messagePart)
    {
        auto iter = std::find_if(diagnostics.diagnostic.begin(), diagnostics.diagnostic.end(),
                                 [&messagePart](const Diagnostic &diagnostic) {
                                     return diagnostic.message_.find(messagePart) != std::string::npos;
                                 });
        return iter == diagnostics.diagnostic.end() ? nullptr : &(*iter);
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

TEST_F(FixRemoveIllegalAwaitTests, TestIllegalAwaitDiagnosticStartsAtAwaitKeyword)
{
    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext("TestIllegalAwaitDiagnosticStartsAtAwaitKeyword.ets",
                                              ES2PANDA_STATE_CHECKED, "function foo(): void {\n  await mp;\n}\n");

    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences diagnostics = lspApi->getSemanticDiagnostics(context);
    const auto awaitLocation = LocationOfText(context, "await");
    const auto unresolvedReferenceLocation = LocationOfText(context, "mp");
    initializer.DestroyContext(context);

    const auto *awaitDiagnostic = FindDiagnosticByMessage(diagnostics, "Await in a non-async function");
    ASSERT_NE(awaitDiagnostic, nullptr);
    ASSERT_EQ(awaitDiagnostic->range_.start.line_, awaitLocation.first);
    ASSERT_EQ(awaitDiagnostic->range_.start.character_, awaitLocation.second);

    const auto *unresolvedDiagnostic = FindDiagnosticByMessage(diagnostics, "Unresolved reference mp");
    ASSERT_NE(unresolvedDiagnostic, nullptr);
    ASSERT_EQ(unresolvedDiagnostic->range_.start.line_, unresolvedReferenceLocation.first);
    ASSERT_EQ(unresolvedDiagnostic->range_.start.character_, unresolvedReferenceLocation.second);
}

// Test: await in non-async function should suggest making the containing function async
TEST_F(FixRemoveIllegalAwaitTests, TestFixRemoveIllegalAwait01)
{
    std::vector<std::string> fileNames = {"TestFixRemoveIllegalAwait01.ets"};
    std::vector<std::string> fileContents = {R"(
function foo(): void {
    let p = new Promise<void>(() => {});
    await p;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "await" keyword (line 4, col 5)
    const size_t start = LineColToPos(context, 4, 5);
    const size_t length = 5;
    const int expectedFixResultSize = 1;

    // Verify target error codes: AWAIT_IN_ARROW_FUN_PARAM(1046), AWAIT_IN_NON_ASYNC_DEPRECATED(175979)
    ASSERT_EQ(ERROR_CODES.size(), 2U);
    ASSERT_EQ(ERROR_CODES[0], AWAIT_IN_ARROW_FUN_PARAM_CODE);
    ASSERT_EQ(ERROR_CODES[1], AWAIT_IN_NON_ASYNC_DEPRECATED_CODE);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);
    ASSERT_EQ(fixResult.size(), expectedFixResultSize);

    const auto &result = fixResult[0];
    ASSERT_EQ(result.fixName_, EXPECTED_FIX_NAME);
    ASSERT_EQ(result.fixId_, EXPECTED_FIX_NAME);
    ASSERT_EQ(result.description_, EXPECTED_FIX_DESCRIPTION);
    ASSERT_EQ(result.changes_[0].fileName, filePaths[0]);
    ASSERT_EQ(result.changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], result.changes_[0].textChanges), R"(
async function foo(): Promise<void> {
    let p = new Promise<void>(() => {});
    await p;
}
)");

    initializer.DestroyContext(context);
}

// Test: await in non-async function with Promise should suggest making the containing function async
TEST_F(FixRemoveIllegalAwaitTests, TestFixRemoveIllegalAwait02)
{
    std::vector<std::string> fileNames = {"TestFixRemoveIllegalAwait02.ets"};
    std::vector<std::string> fileContents = {R"(
function bar(): void {
    await foo();
}
function foo(): Promise<void> { return Promise.resolve(); }
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    // Position at "await" keyword (line 3, col 5)
    const size_t start = LineColToPos(context, 3, 5);
    const size_t length = 5;

    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + length, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    const auto &result = fixResult[0];
    ASSERT_EQ(result.fixName_, EXPECTED_FIX_NAME);
    ASSERT_EQ(result.fixId_, EXPECTED_FIX_NAME);
    ASSERT_EQ(result.description_, EXPECTED_FIX_DESCRIPTION);
    ASSERT_EQ(result.changes_[0].fileName, filePaths[0]);
    ASSERT_EQ(result.changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], result.changes_[0].textChanges), R"(
async function bar(): Promise<void> {
    await foo();
}
function foo(): Promise<void> { return Promise.resolve(); }
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixMethodIllegalAwait)
{
    std::vector<std::string> fileNames = {"TestFixMethodIllegalAwait.ets"};
    std::vector<std::string> fileContents = {R"(
class A {
    foo(): void {
        let p = new Promise<void>(() => {});
        await p;
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
class A {
    async foo(): Promise<void> {
        let p = new Promise<void>(() => {});
        await p;
    }
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixStaticMethodWithModifierIllegalAwait)
{
    std::vector<std::string> fileNames = {"TestFixStaticMethodWithModifierIllegalAwait.ets"};
    std::vector<std::string> fileContents = {R"(
class A {
    public static foo(): void {
        await Promise.resolve();
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
class A {
    public static async foo(): Promise<void> {
        await Promise.resolve();
    }
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestNoFixForConstructorIllegalAwait)
{
    std::vector<std::string> fileNames = {"TestNoFixForConstructorIllegalAwait.ets"};
    std::vector<std::string> fileContents = {R"(
class A {
    constructor() {
        await Promise.resolve();
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_TRUE(fixResult.empty());

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixArrowFunctionIllegalAwait)
{
    std::vector<std::string> fileNames = {"TestFixArrowFunctionIllegalAwait.ets"};
    std::vector<std::string> fileContents = {R"(
let foo = (): void => {
    let p = new Promise<void>(() => {});
    await p;
};
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
let foo = async (): Promise<void> => {
    let p = new Promise<void>(() => {});
    await p;
};
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixFunctionWithFunctionKeywordInParameterInitializer)
{
    std::vector<std::string> fileNames = {"TestFixFunctionWithFunctionKeywordInParameterInitializer.ets"};
    std::vector<std::string> fileContents = {R"(
function foo(name: string = "function "): void {
    await Promise.resolve();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
async function foo(name: string = "function "): Promise<void> {
    await Promise.resolve();
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixFunctionWithCommentBeforeName)
{
    std::vector<std::string> fileNames = {"TestFixFunctionWithCommentBeforeName.ets"};
    std::vector<std::string> fileContents = {R"(
function /* keep */ foo(): void {
    await Promise.resolve();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
async function /* keep */ foo(): Promise<void> {
    await Promise.resolve();
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixExportFunction)
{
    std::vector<std::string> fileNames = {"TestFixExportFunction.ets"};
    std::vector<std::string> fileContents = {R"(
export function foo(): void {
    await Promise.resolve();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
export async function foo(): Promise<void> {
    await Promise.resolve();
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixExportDefaultFunction)
{
    std::vector<std::string> fileNames = {"TestFixExportDefaultFunction.ets"};
    std::vector<std::string> fileContents = {R"(
export default function foo(): void {
    await Promise.resolve();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
export default async function foo(): Promise<void> {
    await Promise.resolve();
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixDoesNotWrapPromiseReturnType)
{
    std::vector<std::string> fileNames = {"TestFixDoesNotWrapPromiseReturnType.ets"};
    std::vector<std::string> fileContents = {R"(
function foo(): Promise<void> {
    await Promise.resolve();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 1U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
async function foo(): Promise<void> {
    await Promise.resolve();
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixDoesNotWrapPromiseTypeAliasReturnType)
{
    std::vector<std::string> fileNames = {"TestFixDoesNotWrapPromiseTypeAliasReturnType.ets"};
    std::vector<std::string> fileContents = {R"(
type P<T> = Promise<T>;
function foo(): P<void> {
    await Promise.resolve();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 1U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
type P<T> = Promise<T>;
async function foo(): P<void> {
    await Promise.resolve();
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixGenericFunction)
{
    std::vector<std::string> fileNames = {"TestFixGenericFunction.ets"};
    std::vector<std::string> fileContents = {R"(
function foo<T>(value: T): T {
    await Promise.resolve();
    return value;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
async function foo<T>(value: T): Promise<T> {
    await Promise.resolve();
    return value;
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixNoExplicitReturnTypeOnlyAddsAsync)
{
    std::vector<std::string> fileNames = {"TestFixNoExplicitReturnTypeOnlyAddsAsync.ets"};
    std::vector<std::string> fileContents = {R"(
function foo() {
    await Promise.resolve();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 1U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
async function foo() {
    await Promise.resolve();
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixWrapsUnionReturnType)
{
    std::vector<std::string> fileNames = {"TestFixWrapsUnionReturnType.ets"};
    std::vector<std::string> fileContents = {R"(
function foo(): string | null {
    await Promise.resolve();
    return null;
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
async function foo(): Promise<string | null> {
    await Promise.resolve();
    return null;
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixWrapsComplexReturnType)
{
    std::vector<std::string> fileNames = {"TestFixWrapsComplexReturnType.ets"};
    std::vector<std::string> fileContents = {R"(
function foo(): Array<string> {
    await Promise.resolve();
    return new Array<string>();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
async function foo(): Promise<Array<string>> {
    await Promise.resolve();
    return new Array<string>();
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestNoFixForGetterIllegalAwait)
{
    std::vector<std::string> fileNames = {"TestNoFixForGetterIllegalAwait.ets"};
    std::vector<std::string> fileContents = {R"(
class A {
    get value(): int {
        await Promise.resolve();
        return 1;
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_TRUE(fixResult.empty());

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestNoFixForSetterIllegalAwait)
{
    std::vector<std::string> fileNames = {"TestNoFixForSetterIllegalAwait.ets"};
    std::vector<std::string> fileContents = {R"(
class A {
    set value(v: int) {
        await Promise.resolve();
    }
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_TRUE(fixResult.empty());

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixWhenCursorInsideAwaitKeyword)
{
    std::vector<std::string> fileNames = {"TestFixWhenCursorInsideAwaitKeyword.ets"};
    std::vector<std::string> fileContents = {R"(
function foo(): void {
    await Promise.resolve();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await") + 2U;
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 1, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
async function foo(): Promise<void> {
    await Promise.resolve();
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixNestedArrowInsideAsyncFunction)
{
    std::vector<std::string> fileNames = {"TestFixNestedArrowInsideAsyncFunction.ets"};
    std::vector<std::string> fileContents = {R"(
async function outer(): Promise<void> {
    let inner = (): void => {
        await Promise.resolve();
    };
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    const size_t start = fileContents[0].find("await");
    ASSERT_NE(start, std::string::npos);
    std::vector<int> errorCodes = {AWAIT_IN_NON_ASYNC_DEPRECATED_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto fixResult = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, start, start + 5, errorCodes, options);

    ASSERT_EQ(fixResult.size(), 1U);
    ASSERT_EQ(fixResult[0].changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], fixResult[0].changes_[0].textChanges), R"(
async function outer(): Promise<void> {
    let inner = async (): Promise<void> => {
        await Promise.resolve();
    };
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixAllAddsAsyncOncePerFunction)
{
    std::vector<std::string> fileNames = {"TestFixAllAddsAsyncOncePerFunction.ets"};
    std::vector<std::string> fileContents = {R"(
function foo(): void {
    let p = new Promise<void>(() => {});
    await p;
    await p;
}
function bar(): Promise<void> {
    await Promise.resolve();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    CombinedCodeActionsInfo combinedFixResult =
        ark::es2panda::lsp::GetCombinedCodeFixImpl(context, EXPECTED_FIX_NAME.data(), options);

    ASSERT_EQ(combinedFixResult.changes_.size(), 1U);
    ASSERT_EQ(combinedFixResult.changes_[0].fileName, filePaths[0]);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges.size(), 3U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], combinedFixResult.changes_[0].textChanges), R"(
async function foo(): Promise<void> {
    let p = new Promise<void>(() => {});
    await p;
    await p;
}
async function bar(): Promise<void> {
    await Promise.resolve();
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixAllSkipsConstructorAndFixesValidFunctions)
{
    std::vector<std::string> fileNames = {"TestFixAllSkipsConstructorAndFixesValidFunctions.ets"};
    std::vector<std::string> fileContents = {R"(
class A {
    constructor() {
        await Promise.resolve();
    }
}
function foo(): void {
    await Promise.resolve();
}
)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    CombinedCodeActionsInfo combinedFixResult =
        ark::es2panda::lsp::GetCombinedCodeFixImpl(context, EXPECTED_FIX_NAME.data(), options);

    ASSERT_EQ(combinedFixResult.changes_.size(), 1U);
    ASSERT_EQ(combinedFixResult.changes_[0].fileName, filePaths[0]);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges.size(), 2U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], combinedFixResult.changes_[0].textChanges), R"(
class A {
    constructor() {
        await Promise.resolve();
    }
}
async function foo(): Promise<void> {
    await Promise.resolve();
}
)");

    initializer.DestroyContext(context);
}

TEST_F(FixRemoveIllegalAwaitTests, TestFixAllMixedFunctionKinds)
{
    std::vector<std::string> fileNames = {"TestFixAllMixedFunctionKinds.ets"};
    std::vector<std::string> fileContents = {std::string(MIXED_FUNCTION_KINDS_SOURCE)};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    auto *context = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);

    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    CombinedCodeActionsInfo combinedFixResult =
        ark::es2panda::lsp::GetCombinedCodeFixImpl(context, EXPECTED_FIX_NAME.data(), options);

    ASSERT_EQ(combinedFixResult.changes_.size(), 1U);
    ASSERT_EQ(combinedFixResult.changes_[0].fileName, filePaths[0]);
    ASSERT_EQ(combinedFixResult.changes_[0].textChanges.size(), 6U);
    ASSERT_EQ(ApplyTextChanges(fileContents[0], combinedFixResult.changes_[0].textChanges),
              MIXED_FUNCTION_KINDS_EXPECTED);

    initializer.DestroyContext(context);
}

}  // namespace
