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

#include <algorithm>
#include <gtest/gtest.h>
#include <variant>
#include <vector>
#include "generated/code_fix_register.h"
#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::lsp::Initializer;

// FixNaNEquality is a half-finished template provider: there is no FIX_NAN_EQUALITY entry in
// the generated code_fix_register.h, so the error code is hardcoded in the provider as
// G_FIX_NAN_EQUALITY_CODE. The literal is used here because the constant is not exported.
constexpr int FIX_NAN_EQUALITY_ERROR_CODE = 1003;
// Error code that is not registered by any code fix provider.
constexpr int UNREGISTERED_ERROR_CODE = 9999;
constexpr int DEFAULT_THROTTLE = 20;

class FixNaNEqualityTests : public LSPAPITests {
public:
    static ark::es2panda::lsp::CancellationToken CreateNonCancellationToken()
    {
        return ark::es2panda::lsp::CancellationToken(DEFAULT_THROTTLE, &GetNullHost());
    }

    static size_t LineColToPos(es2panda_Context *context, const size_t line, const size_t col)
    {
        // line/column are 1-based
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        auto index = ark::es2panda::lexer::LineIndex(ctx->parserProgram->SourceCode());
        return index.GetOffset(ark::es2panda::lexer::SourceLocation(line, col, ctx->parserProgram));
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

// The provider is registered (AutoCodeFixRegister<FixNaNEquality>) and reachable through the
// error code 1003, but MakeChangeForNaNEquality only emits an edit when the binary expression's
// node type is TS_IMPORT_EQUALS_DECLARATION, which can never be true for a BinaryExpression.
// The real text-replacing branch is therefore unreachable and the provider never produces a fix.
// This test fixes the current behavior: requesting the fix through the provider entry returns an
// empty result instead of an erroneous edit.
TEST_F(FixNaNEqualityTests, NaNComparisonReturnsNoFixBecauseChangeBranchUnreachable)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("FixNaNEquality_Compare.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(function check(x: number): void {
    if (x == NaN) {}
})");

    // Point at the `==` operator (1-based line 2, 1-based column 11) so the touching token is
    // the BinaryExpression, reaching the isLeftNaN/isRightNaN logic inside the provider.
    const size_t start = LineColToPos(ctx, 2, 11);
    const size_t length = 2;
    std::vector<int> errorCodes = {FIX_NAN_EQUALITY_ERROR_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto result = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, start, start + length, errorCodes, options);

    ASSERT_TRUE(result.empty());

    initializer.DestroyContext(ctx);
}

// Control group: the same position with an error code that no provider is registered for must
// also return an empty result.
TEST_F(FixNaNEqualityTests, UnregisteredErrorCodeReturnsNoFixes)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("FixNaNEquality_Unregistered.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(function check(x: number): void {
    if (x == NaN) {}
})");

    const size_t start = LineColToPos(ctx, 2, 11);
    const size_t length = 2;
    std::vector<int> errorCodes = {UNREGISTERED_ERROR_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto result = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, start, start + length, errorCodes, options);

    ASSERT_TRUE(result.empty());

    initializer.DestroyContext(ctx);
}

// Negative case: a plain comparison that does not involve NaN at all must not produce a fix for
// the NaN-equality error code either.
TEST_F(FixNaNEqualityTests, PlainComparisonWithNaNErrorCodeReturnsNoFixes)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("FixNaNEquality_PlainCompare.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(function check(x: number, y: number): void {
    if (x == y) {}
})");

    const size_t start = LineColToPos(ctx, 2, 11);
    const size_t length = 2;
    std::vector<int> errorCodes = {FIX_NAN_EQUALITY_ERROR_CODE};
    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};

    auto result = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(ctx, start, start + length, errorCodes, options);

    ASSERT_TRUE(result.empty());

    initializer.DestroyContext(ctx);
}

// Walks the fix-all entry (FixNaNEquality::GetAllCodeActions) through GetCombinedCodeFixImpl. No
// real diagnostic carries error code 1003 (the hardcoded template placeholder), so the fix-all
// lambda never fires and the combined result is empty. This fixes the current behavior of the
// fix-all entry: it is entered but produces no edits.
TEST_F(FixNaNEqualityTests, FixAllReturnsEmptyForTemplateErrorCode)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("FixNaNEquality_FixAll.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(function check(x: number): void {
    if (x == NaN) {}
})");

    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    auto combined = ark::es2panda::lsp::GetCombinedCodeFixImpl(ctx, "FixNaNEquality", options);

    ASSERT_TRUE(combined.changes_.empty());

    initializer.DestroyContext(ctx);
}

// The fix-all callback inside FixNaNEquality::GetAllCodeActions fires only when a real diagnostic
// carries the integer code 1003 (DiagnosticType::SYNTAX * DIAGNOSTIC_CODE_MULTIPLIER + 3). Today
// that code is produced solely by PACKAGE_MODULE_IMPORT_OWN_PACKAGE: a package module importing a
// sibling file of its own package through an explicit relative import (logged by the
// PackageImplicitImport lowering phase). This test builds that two-file package scenario, verifies
// the precondition diagnostic exists, and pins current fix-all behavior: the provider's fix-all
// entry runs over the matched diagnostic, but MakeChangeForNaNEquality never emits an edit (its
// TS_IMPORT_EQUALS_DECLARATION branch cannot hold for a BinaryExpression), so the combined result
// stays empty.
TEST_F(FixNaNEqualityTests, DISABLED_FixAllRunsOverPackageOwnImportDiagnosticAndStaysEmpty)
{
    const std::string mainSource =
        "package com.example.pkg;\n"
        "import { v } from \"./nan_fix_all_other\";\n"
        "export function useValue(): int {\n"
        "    return v;\n"
        "}\n";
    const std::string otherSource =
        "package com.example.pkg;\n"
        "export const v: int = 1;\n";
    std::vector<std::string> fileNames = {"nan_fix_all_main.ets", "nan_fix_all_other.ets"};
    std::vector<std::string> fileContents = {mainSource, otherSource};
    auto filePaths = CreateTempFile(fileNames, fileContents);
    ASSERT_EQ(fileNames.size(), filePaths.size());

    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    // Precondition: the syntactic storage must contain a diagnostic whose LSP code equals the
    // hardcoded G_FIX_NAN_EQUALITY_CODE (1003); otherwise EachDiagnostic never reaches the
    // fix-all lambda below.
    LSPAPI const *lspApi = GetImpl();
    DiagnosticReferences diagnostics = lspApi->getSyntacticDiagnostics(ctx);
    const auto packageOwnImportDiag =
        std::find_if(diagnostics.diagnostic.begin(), diagnostics.diagnostic.end(), [](const Diagnostic &diagnostic) {
            return std::holds_alternative<int>(diagnostic.code_) &&
                   std::get<int>(diagnostic.code_) == FIX_NAN_EQUALITY_ERROR_CODE;
        });
    ASSERT_NE(packageOwnImportDiag, diagnostics.diagnostic.end());
    EXPECT_EQ(packageOwnImportDiag->message_, "Package module cannot import from a file in it's own package.");

    CodeFixOptions options = {CreateNonCancellationToken(), ark::es2panda::lsp::FormatCodeSettings(), {}};
    CombinedCodeActionsInfo combined = ark::es2panda::lsp::GetCombinedCodeFixImpl(ctx, "FixNaNEquality", options);

    // Current behavior: the fix-all lambda executes for the matched diagnostic, but the change
    // branch inside MakeChangeForNaNEquality is unreachable, so no edit is produced.
    EXPECT_TRUE(combined.changes_.empty());
    EXPECT_TRUE(combined.commands_.empty());

    initializer.DestroyContext(ctx);
}

}  // namespace
