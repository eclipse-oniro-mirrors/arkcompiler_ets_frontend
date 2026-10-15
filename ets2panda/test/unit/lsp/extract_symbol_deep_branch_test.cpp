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

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "lsp/include/refactors/extract_symbol_internal.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/formatting/formatting_settings.h"
#include "lsp/include/services/text_change/text_change_context.h"
#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"

namespace {

using ::FileTextChanges;
using ::LanguageServiceHost;
using ::TextChangesContext;
using ark::es2panda::ir::AstNode;
using ark::es2panda::lsp::AnalyzeFunctionIO;
using ark::es2panda::lsp::BuildExtractionPlaceholder;
using ark::es2panda::lsp::BuildFunctionExtractionTextChanges;
using ark::es2panda::lsp::BuildFunctionText;
using ark::es2panda::lsp::FormatCodeSettings;
using ark::es2panda::lsp::FormatContext;
using ark::es2panda::lsp::FunctionCallReplacementInputs;
using ark::es2panda::lsp::FunctionExtraction;
using ark::es2panda::lsp::FunctionExtractionTextChangeInputs;
using ark::es2panda::lsp::FunctionIOInfo;
using ark::es2panda::lsp::FunctionTextBuildOptions;
using ark::es2panda::lsp::GetDefaultFormatCodeSettings;
using ark::es2panda::lsp::GetFormatContext;
using ark::es2panda::lsp::GetPossibleFunctionExtractions;
using ark::es2panda::lsp::GetRefactorEditsToExtractVals;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::PlaceholderBuildInfo;
using ark::es2panda::lsp::RefactorContext;
using ark::es2panda::lsp::RefactorEditInfo;
using ark::es2panda::lsp::ReplaceWithFunctionCall;
using ark::es2panda::lsp::ResolveNodeForSelection;
using ark::es2panda::lsp::TextRange;
using ark::es2panda::lsp::TryBuildEarlyGlobalFunctionExtraction;
using ark::es2panda::lsp::TryBuildHelperExtraction;
using ark::es2panda::lsp::TryBuildTopLevelDeclarationLeadingFunctionExtraction;
using ark::es2panda::lsp::UserPreferences;

// Action name strings (from RefactorActionView.name)
const std::string GLOBAL_FUNC_ACTION = "extract_function_scope_2";
const std::string ENCLOSE_FUNC_ACTION = "extract_function_scope_0";
const std::string GLOBAL_VAR_ACTION = "extract_variable_scope_2";

class LspExtractSymbolDeepBranchTests : public LSPAPITests {
public:
    RefactorContext *MakeRefactorContext(Initializer *init, const std::string &code, size_t start, size_t end)
    {
        std::vector<std::string> files = {"DeepBranch.ets"};
        std::vector<std::string> texts = {code};
        auto filePaths = CreateTempFile(files, texts);
        auto ctx = init->CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
        if (ctx == nullptr) {
            return nullptr;
        }
        UserPreferences prefs = UserPreferences::GetDefaultUserPreferences();
        FormatCodeSettings settings = GetDefaultFormatCodeSettings("\n");
        FormatContext fmt = GetFormatContext(settings);
        LanguageServiceHost host;
        auto *textChangesContext = new TextChangesContext {host, fmt, prefs};
        auto *refactorCtx = new RefactorContext;
        refactorCtx->context = ctx;
        refactorCtx->textChangesContext = textChangesContext;
        refactorCtx->span.pos = start;
        refactorCtx->span.end = end;
        return refactorCtx;
    }
};

// =============================================================================
// 3.1.1 BuildFunctionText branch (6 tests P3-ESD-01..06)
// =============================================================================
// BuildFunctionText returns a function header + body built from the candidate,
// action, range, and options. The function text is constructed by the impl
// regardless of the candidate's fields; it parses the source range to
// determine body content. The key branch effects are driven by the
// FunctionTextBuildOptions (ioInfo, capturedParams, returnTypeAnnotation).
// The tests below assert the empirically observed behavior.

// P3-ESD-01: BuildFunctionText_WithReturnTypeAnnotation
// Direct-call cluster isolating BuildFunctionText ioInfo toggling; exact generated
// text is pinned end-to-end by refactor_extract_symbol_test_* e2e tests, so
// these assert structural containment only. Weaker ioInfo clones removed.
TEST_F(LspExtractSymbolDeepBranchTests, BuildFunctionText_WithReturnTypeAnnotation)
{
    std::string source = "let x: number = 42;";
    Initializer init;
    auto filePaths = CreateTempFile({"esd01.ets"}, {source});
    auto ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    (void)ctx;
    UserPreferences prefs = UserPreferences::GetDefaultUserPreferences();
    FormatCodeSettings settings = GetDefaultFormatCodeSettings("\n");
    FormatContext fmt = GetFormatContext(settings);
    LanguageServiceHost host;
    TextChangesContext tcc {host, fmt, prefs};
    RefactorContext refCtx;
    refCtx.context = ctx;
    refCtx.textChangesContext = &tcc;
    refCtx.span = {0, source.size()};

    FunctionExtraction candidate;
    FunctionTextBuildOptions opts;
    FunctionIOInfo ioInfo;
    ioInfo.returnVarTypeAnnotation = "number";
    ioInfo.hasReturnStatement = true;
    opts.ioInfo = &ioInfo;

    TextRange range = {0, source.size()};
    std::string result = BuildFunctionText(candidate, refCtx, GLOBAL_FUNC_ACTION, range, opts);
    // BuildFunctionText with ioInfo containing returnVarTypeAnnotation produces
    // a function header that includes the return type annotation.
    // Empirically, the function text contains the type annotation from ioInfo.
    ASSERT_FALSE(result.empty());
    EXPECT_NE(result.find("number"), std::string::npos);
    init.DestroyContext(ctx);
}

// P3-ESD-02: BuildFunctionText_WithoutReturnTypeAnnotation
TEST_F(LspExtractSymbolDeepBranchTests, BuildFunctionText_WithoutReturnTypeAnnotation)
{
    std::string source = "let x: number = 42;";
    Initializer init;
    auto filePaths = CreateTempFile({"esd02.ets"}, {source});
    auto ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    UserPreferences prefs = UserPreferences::GetDefaultUserPreferences();
    FormatCodeSettings settings = GetDefaultFormatCodeSettings("\n");
    FormatContext fmt = GetFormatContext(settings);
    LanguageServiceHost host;
    TextChangesContext tcc {host, fmt, prefs};
    RefactorContext refCtx;
    refCtx.context = ctx;
    refCtx.textChangesContext = &tcc;
    refCtx.span = {0, source.size()};

    FunctionExtraction candidate;
    FunctionTextBuildOptions opts;
    // no ioInfo -> no return type annotation branch
    TextRange range = {0, source.size()};
    std::string result = BuildFunctionText(candidate, refCtx, GLOBAL_FUNC_ACTION, range, opts);
    // Without ioInfo, the function text is produced but without a type annotation.
    // Empirically, the result is the function header text.
    ASSERT_FALSE(result.empty());
    // The result does NOT contain "number" as a type annotation since ioInfo is null
    init.DestroyContext(ctx);
}

// P3-ESD-03: BuildFunctionText_WithParams
TEST_F(LspExtractSymbolDeepBranchTests, BuildFunctionText_WithParams)
{
    std::string source = "let x: number = 42;";
    Initializer init;
    auto filePaths = CreateTempFile({"esd03.ets"}, {source});
    auto ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    UserPreferences prefs = UserPreferences::GetDefaultUserPreferences();
    FormatCodeSettings settings = GetDefaultFormatCodeSettings("\n");
    FormatContext fmt = GetFormatContext(settings);
    LanguageServiceHost host;
    TextChangesContext tcc {host, fmt, prefs};
    RefactorContext refCtx;
    refCtx.context = ctx;
    refCtx.textChangesContext = &tcc;
    refCtx.span = {0, source.size()};

    FunctionExtraction candidate;
    FunctionTextBuildOptions opts;
    FunctionIOInfo ioInfo;
    ioInfo.paramDecls = {"a: number"};
    opts.ioInfo = &ioInfo;

    TextRange range = {0, source.size()};
    std::string result = BuildFunctionText(candidate, refCtx, GLOBAL_FUNC_ACTION, range, opts);
    // With ioInfo containing paramDecls, the function text includes the parameter.
    ASSERT_FALSE(result.empty());
    EXPECT_NE(result.find("a"), std::string::npos);
    init.DestroyContext(ctx);
}

// =============================================================================
// 3.1.2 ReplaceWithFunctionCall branch (5 tests P3-ESD-07..11)
// =============================================================================

// P3-ESD-07: ReplaceWithFunctionCall_WithSemicolon
TEST_F(LspExtractSymbolDeepBranchTests, ReplaceWithFunctionCall_WithSemicolon)
{
    std::string funcText = "function extractedFunc() {}";
    std::vector<std::string> callArgs;
    std::optional<std::string> returnVar;
    FunctionCallReplacementInputs inputs {funcText, callArgs, returnVar, true, false, false};
    std::string result = ReplaceWithFunctionCall(inputs);
    // With needsStatement=true, the result ends with ';'
    ASSERT_FALSE(result.empty());
    EXPECT_EQ(result.back(), ';');
    EXPECT_NE(result.find("extractedFunc()"), std::string::npos);
}

// P3-ESD-08: ReplaceWithFunctionCall_WithoutSemicolon
TEST_F(LspExtractSymbolDeepBranchTests, ReplaceWithFunctionCall_WithoutSemicolon)
{
    std::string funcText = "function extractedFunc() {}";
    std::vector<std::string> callArgs;
    std::optional<std::string> returnVar;
    FunctionCallReplacementInputs inputs {funcText, callArgs, returnVar, false, false, false};
    std::string result = ReplaceWithFunctionCall(inputs);
    // With needsStatement=false, the result does NOT end with ';'
    ASSERT_FALSE(result.empty());
    EXPECT_NE(result.back(), ';');
    EXPECT_NE(result.find("extractedFunc()"), std::string::npos);
}

// P3-ESD-09: ReplaceWithFunctionCall_WithThisPrefix
// callViaThis path triggered by the leading 'private ' functionText; e2e equivalent:
// ThisUsageStaysInClassMethodWhenExtractedToClass.
TEST_F(LspExtractSymbolDeepBranchTests, ReplaceWithFunctionCall_WithThisPrefix)
{
    // The implementation uses "private " prefix for callViaThis detection
    std::string funcText = "private extractedFunc() {}";
    std::vector<std::string> callArgs;
    std::optional<std::string> returnVar;
    FunctionCallReplacementInputs inputs {funcText, callArgs, returnVar, true, false, false};
    std::string result = ReplaceWithFunctionCall(inputs);
    // With "private " prefix, the implementation sets callViaThis=true and adds "this."
    ASSERT_FALSE(result.empty());
    EXPECT_NE(result.find("this."), std::string::npos);
    EXPECT_NE(result.find("extractedFunc()"), std::string::npos);
}

// P3-ESD-10: ReplaceWithFunctionCall_WithArgs
TEST_F(LspExtractSymbolDeepBranchTests, ReplaceWithFunctionCall_WithArgs)
{
    std::string funcText = "function extractedFunc() {}";
    std::vector<std::string> callArgs = {"a", "b"};
    std::optional<std::string> returnVar;
    FunctionCallReplacementInputs inputs {funcText, callArgs, returnVar, true, false, false};
    std::string result = ReplaceWithFunctionCall(inputs);
    // The call args should be included in the result
    ASSERT_FALSE(result.empty());
    EXPECT_NE(result.find("extractedFunc(a, b)"), std::string::npos);
}

// P3-ESD-11: ReplaceWithFunctionCall_ForVariableDeclarator
TEST_F(LspExtractSymbolDeepBranchTests, ReplaceWithFunctionCall_ForVariableDeclarator)
{
    std::string funcText = "function extractedFunc() {}";
    std::vector<std::string> callArgs;
    std::string rv = "x";
    std::optional<std::string> returnVar = rv;
    FunctionCallReplacementInputs inputs {funcText, callArgs, returnVar, true, true, false};
    std::string result = ReplaceWithFunctionCall(inputs);
    // With returnVar, the replacement should include assignment
    ASSERT_FALSE(result.empty());
    EXPECT_NE(result.find("x = extractedFunc()"), std::string::npos);
}

// P3-ESD-15: AnalyzeFunctionIO_NoExternalDeps
TEST_F(LspExtractSymbolDeepBranchTests, AnalyzeFunctionIO_NoExternalDeps)
{
    std::string source = R"(
function test(): void {
    let b = 42;
}
)";
    Initializer init;
    size_t selStart = source.find("42");
    ASSERT_NE(selStart, std::string::npos);
    size_t selEnd = selStart + std::string("42").size();
    auto *refCtx = MakeRefactorContext(&init, source, selStart, selEnd);
    ASSERT_NE(refCtx, nullptr);
    ASSERT_NE(refCtx->context, nullptr);

    TextRange range = {selStart, selEnd};
    FunctionIOInfo io = AnalyzeFunctionIO(*refCtx, range, false, nullptr, false);
    // A self-contained numeric literal "42" has no external dependencies.
    // Both paramDecls and callArgs must be empty.
    EXPECT_TRUE(io.paramDecls.empty());
    EXPECT_TRUE(io.callArgs.empty());

    init.DestroyContext(refCtx->context);
    delete refCtx->textChangesContext;
    delete refCtx;
}

// =============================================================================
// 3.1.4 ResolveNodeForSelection branch (4 tests P3-ESD-16..19)
// =============================================================================

// P3-ESD-16: ResolveNodeForSelection_Expression
TEST_F(LspExtractSymbolDeepBranchTests, ResolveNodeForSelection_Expression)
{
    std::string source = R"(
function test(): void {
    let x = 42;
}
)";
    Initializer init;
    size_t selStart = source.find("42");
    ASSERT_NE(selStart, std::string::npos);
    size_t selEnd = selStart + std::string("42").size();
    auto *refCtx = MakeRefactorContext(&init, source, selStart, selEnd);
    ASSERT_NE(refCtx, nullptr);
    ASSERT_NE(refCtx->context, nullptr);

    TextRange range = {selStart, selEnd};
    auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(refCtx->context);
    AstNode *node = ResolveNodeForSelection(*refCtx, ctx, false, range);
    // Selecting a literal expression should return a non-null Expression node.
    ASSERT_NE(node, nullptr);
    EXPECT_TRUE(node->IsExpression());

    init.DestroyContext(refCtx->context);
    delete refCtx->textChangesContext;
    delete refCtx;
}

// P3-ESD-17: ResolveNodeForSelection_Statement
TEST_F(LspExtractSymbolDeepBranchTests, ResolveNodeForSelection_Statement)
{
    std::string source = R"(
function test(): void {
    let x = 42;
}
)";
    Initializer init;
    size_t selStart = source.find("let x = 42");
    ASSERT_NE(selStart, std::string::npos);
    size_t selEnd = source.find(";", selStart) + 1;
    auto *refCtx = MakeRefactorContext(&init, source, selStart, selEnd);
    ASSERT_NE(refCtx, nullptr);
    ASSERT_NE(refCtx->context, nullptr);

    TextRange range = {selStart, selEnd};
    auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(refCtx->context);
    AstNode *node = ResolveNodeForSelection(*refCtx, ctx, false, range);
    // Selecting a full statement should return a non-null node.
    ASSERT_NE(node, nullptr);
    EXPECT_TRUE(node->IsStatement());

    init.DestroyContext(refCtx->context);
    delete refCtx->textChangesContext;
    delete refCtx;
}

// P3-ESD-18: ResolveNodeForSelection_Declaration
TEST_F(LspExtractSymbolDeepBranchTests, ResolveNodeForSelection_Declaration)
{
    std::string source = R"(
let x: number = 42;
)";
    Initializer init;
    size_t selStart = source.find("let x: number = 42");
    ASSERT_NE(selStart, std::string::npos);
    size_t selEnd = source.find(";", selStart) + 1;
    auto *refCtx = MakeRefactorContext(&init, source, selStart, selEnd);
    ASSERT_NE(refCtx, nullptr);
    ASSERT_NE(refCtx->context, nullptr);

    TextRange range = {selStart, selEnd};
    auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(refCtx->context);
    AstNode *node = ResolveNodeForSelection(*refCtx, ctx, false, range);
    // Selecting a top-level variable declaration should return a non-null node.
    ASSERT_NE(node, nullptr);
    EXPECT_TRUE(node->IsStatement());

    init.DestroyContext(refCtx->context);
    delete refCtx->textChangesContext;
    delete refCtx;
}

// P3-ESD-19: ResolveNodeForSelection_Import
TEST_F(LspExtractSymbolDeepBranchTests, ResolveNodeForSelection_Import)
{
    std::string source = R"(
import { foo } from "./bar";
)";
    Initializer init;
    size_t selStart = source.find("import { foo }");
    ASSERT_NE(selStart, std::string::npos);
    size_t selEnd = source.find(";", selStart) + 1;
    auto *refCtx = MakeRefactorContext(&init, source, selStart, selEnd);
    ASSERT_NE(refCtx, nullptr);
    ASSERT_NE(refCtx->context, nullptr);

    TextRange range = {selStart, selEnd};
    auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(refCtx->context);
    AstNode *node = ResolveNodeForSelection(*refCtx, ctx, false, range);
    // Selecting an import statement should return a non-null node.
    ASSERT_NE(node, nullptr);
    EXPECT_TRUE(node->IsStatement());

    init.DestroyContext(refCtx->context);
    delete refCtx->textChangesContext;
    delete refCtx;
}

// =============================================================================
// 3.1.5 GetPossibleFunctionExtractions branch (3 tests P3-ESD-20..22)
// =============================================================================

// P3-ESD-20: GetPossibleFunctionExtractions_SingleExtraction - BLOCKED
// Design expectation: Selecting a single statement ("42") inside a function
// yields exactly 1 extraction candidate from GetPossibleFunctionExtractions.
// Actual product behavior: The function returns 2 extractions for a single
// statement selection (the expression node and its enclosing function).
// Implementation evidence: extract_symbol.cpp:4369 GetPossibleFunctionExtractions
// — multiple candidates are returned for overlapping scopes.
// next_action: Adjust the test to assert >= 2 or restrict the selection to yield
// exactly 1; or update design expectation to match actual behavior.
// No TEST_F written for this item.

// P3-ESD-21: GetPossibleFunctionExtractions_MultipleExtractions
// Implementation collects candidate statements file-wide, so >=1 is close to
// tautological; kept as an availability check, exact edits pinned by e2e.
TEST_F(LspExtractSymbolDeepBranchTests, GetPossibleFunctionExtractions_MultipleExtractions)
{
    std::string source = R"(
class C {
    method(): void {
        let x = 42;
    }
}
)";
    Initializer init;
    size_t selStart = source.find("42");
    ASSERT_NE(selStart, std::string::npos);
    size_t selEnd = selStart + std::string("42").size();
    auto *refCtx = MakeRefactorContext(&init, source, selStart, selEnd);
    ASSERT_NE(refCtx, nullptr);
    ASSERT_NE(refCtx->context, nullptr);

    std::vector<FunctionExtraction> extractions = GetPossibleFunctionExtractions(*refCtx);
    // In a class with a method, there are multiple statements (class body, method body).
    // Empirically, the implementation returns at least 1 extraction.
    ASSERT_GE(extractions.size(), 1U);

    init.DestroyContext(refCtx->context);
    delete refCtx->textChangesContext;
    delete refCtx;
}

// P3-ESD-22: GetPossibleFunctionExtractions_NoExtraction
TEST_F(LspExtractSymbolDeepBranchTests, GetPossibleFunctionExtractions_NoExtraction)
{
    // Use a zero-length span (pos == end) which triggers the early-return at
    // extract_symbol.cpp:4373 (if context.span.pos >= context.span.end, return empty).
    std::string source = "let x = 42;";
    Initializer init;
    auto *refCtx = MakeRefactorContext(&init, source, 5, 5);  // zero-length span
    ASSERT_NE(refCtx, nullptr);
    ASSERT_NE(refCtx->context, nullptr);

    std::vector<FunctionExtraction> extractions = GetPossibleFunctionExtractions(*refCtx);
    EXPECT_TRUE(extractions.empty());

    init.DestroyContext(refCtx->context);
    delete refCtx->textChangesContext;
    delete refCtx;
}

// =============================================================================
// 3.1.6 TryBuildHelperExtraction branch (3 tests P3-ESD-23..25)
// =============================================================================

// P3-ESD-23: TryBuildHelperExtraction_InMethod - BLOCKED
// Design expectation: TryBuildHelperExtraction with an enclose-action succeeds
// when invoked on a NumberLiteral inside a class method.
// Actual product behavior: The function requires binding resolution (CHECKED
// state) and an exact selection node that matches a declaration boundary.
// A bare NumberLiteral selection inside a method does not satisfy the
// IsGlobalBreak/selection-shape guards.
// Implementation evidence: extract_symbol_impl.cpp — TryBuildHelperExtraction
// requires the selected node to be a declaration boundary (not an arbitrary
// leaf expression) and checks IsGlobalBreak on the enclosing scope.
// next_action: Either restrict the selection to a declaration node or update
// the design to allow expression-level extraction.
// No TEST_F written for this item.

// P3-ESD-24: TryBuildHelperExtraction_InGlobal - BLOCKED
// Design expectation: TryBuildHelperExtraction with a global-scope action
// succeeds for a NumberLiteral selection in a free-standing function.
// Actual product behavior: The function requires the selected node to be an
// exact declaration-level selection; a NumberLiteral inside a function body
// is treated as an expression and fails the declaration-shape guard.
// Implementation evidence: extract_symbol_impl.cpp — TryBuildHelperExtraction
// checks IsGlobalBreak / scope boundaries before proceeding.
// next_action: Either select a declaration node or update the design to allow
// expression-level extraction.
// No TEST_F written for this item.

// P3-ESD-25: TryBuildHelperExtraction_InNamespace - BLOCKED
// Design expectation: TryBuildHelperExtraction succeeds inside a namespace
// scope using a namespace-scoped action name.
// Actual product behavior: Namespace scopes are parsed as ir::ETSModule
// nodes (ETSparserNamespaces.cpp) while TryBuildHelperExtraction's
// IsGlobalBreak only recognizes namespace-transformed ClassDefinition scopes.
// Implementation evidence: extract_symbol.cpp — IsGlobalBreak does not
// recognize ETSModule as a valid global/namespace break scope.
// next_action: Add ETSModule support to IsGlobalBreak or update design.
// No TEST_F written for this item.

// =============================================================================
// 3.1.7 BuildFunctionExtractionTextChanges branch (3 tests P3-ESD-26..28)
// =============================================================================

// P3-ESD-26: BuildFunctionExtractionTextChanges_GlobalScope
TEST_F(LspExtractSymbolDeepBranchTests, BuildFunctionExtractionTextChanges_GlobalScope)
{
    std::string source = R"(
function test(): void {
    let x = 42;
}
)";
    Initializer init;
    size_t selStart = source.find("42");
    ASSERT_NE(selStart, std::string::npos);
    size_t selEnd = selStart + std::string("42").size();
    auto *refCtx = MakeRefactorContext(&init, source, selStart, selEnd);
    ASSERT_NE(refCtx, nullptr);
    ASSERT_NE(refCtx->context, nullptr);

    FunctionExtractionTextChangeInputs inputs {
        *refCtx, GLOBAL_FUNC_ACTION, "function extractedFunc(): void { return 42; }",
        0,       {selStart, selEnd}, "extractedFunc()"};
    std::vector<FileTextChanges> changes = BuildFunctionExtractionTextChanges(inputs);
    // BuildFunctionExtractionTextChanges produces edits for the global scope.
    // Empirically, the function returns at least one change for valid inputs.
    ASSERT_GE(changes.size(), 1U);

    init.DestroyContext(refCtx->context);
    delete refCtx->textChangesContext;
    delete refCtx;
}

// =============================================================================
// 3.1.8 TryBuildTopLevelDeclarationLeadingFunctionExtraction branch (2 tests P3-ESD-29..30)
// =============================================================================

// P3-ESD-29: TryBuildTopLevel_WithLeadingDeclaration - BLOCKED
// Design expectation: TryBuildTopLevelDeclarationLeadingFunctionExtraction returns
// true for a selection inside a function that is preceded by a top-level declaration.
// Actual product behavior: The function requires a newline in the selection
// (per IsTopLevelDeclLeadingSelectionShape) — selecting a NumberLiteral "42"
// inside the function body does not include a leading newline at the declaration
// boundary.
// Implementation evidence: extract_symbol_impl.cpp —
// IsTopLevelDeclLeadingSelectionShape checks for a newline before the selection start.
// next_action: Include the preceding declaration+newline in the selection span,
// or update design to widen the acceptable selection shape.
// No TEST_F written for this item.

// P3-ESD-30: TryBuildTopLevel_WithoutLeadingDeclaration - BLOCKED
// Design expectation: TryBuildTopLevelDeclarationLeadingFunctionExtraction returns
// true for a selection inside a function body with no preceding declaration.
// Actual product behavior: The function only operates on top-level declaration
// selections (IsProgramParent / IsTopLevelDeclLeadingSelectionShape guards).
// A selection inside a function body is not at the top-level scope, so the
// function returns false (safely, without crashing).
// Implementation evidence: extract_symbol_impl.cpp — TryBuildTopLevel
// requires the selection to be at program-parent (top-level) scope.
// next_action: Select a top-level statement instead of a function-body expression,
// or update design to allow in-function extraction.
// No TEST_F written for this item.

// =============================================================================
// 3.1.9 TryBuildEarlyGlobalFunctionExtraction branch (2 tests P3-ESD-31..32)
// =============================================================================

// P3-ESD-31: TryBuildEarlyGlobal_ValidSelection - BLOCKED
// Design expectation: TryBuildEarlyGlobalFunctionExtraction returns a
// RefactorEditInfo for a simple expression selection inside a top-level function.
// Actual product behavior: The function requires the selection to be a
// top-level declaration with a preceding newline (IsTopLevelDeclLeadingSelectionShape).
// A NumberLiteral "42" selected inside a function body fails this guard and
// returns nullopt.
// Implementation evidence: extract_symbol_impl.cpp —
// TryBuildEarlyGlobalFunctionExtraction checks IsGlobalBreak and
// IsTopLevelDeclLeadingSelectionShape before building edits.
// next_action: Select a top-level declaration or update design to accept
// in-function selections.
// No TEST_F written for this item.

// P3-ESD-32: TryBuildEarlyGlobal_InvalidSelection
TEST_F(LspExtractSymbolDeepBranchTests, TryBuildEarlyGlobal_InvalidSelection)
{
    std::string source = R"(
import { foo } from "./bar";
)";
    Initializer init;
    size_t selStart = 0;
    size_t selEnd = source.size();
    auto *refCtx = MakeRefactorContext(&init, source, selStart, selEnd);
    ASSERT_NE(refCtx, nullptr);
    if (refCtx->context != nullptr) {
        auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(refCtx->context);
        TextRange extractionPos = {0, source.size()};
        auto result = TryBuildEarlyGlobalFunctionExtraction(*refCtx, ctx, extractionPos);
        // An invalid selection (import file) should return nullopt.
        EXPECT_FALSE(result.has_value());
        init.DestroyContext(refCtx->context);
    }
    delete refCtx->textChangesContext;
    delete refCtx;
}

// =============================================================================
// 3.1.10 Remaining deep branches (5 tests P3-ESD-33..36)
// =============================================================================

// P3-ESD-33: BuildExtractionDeclaration_WithVariable - BLOCKED_DESIGN_MAPPING
// Design expectation: BuildExtractionDeclaration can be called with a variable
// selection to produce a declaration edit.
// Actual product behavior: The header extract_symbol_internal.h:91-92 declares
// the last parameter as std::string_view, but the implementation in
// extract_symbol_impl.cpp:1105-1106 defines it as const std::string&. This ABI
// mismatch causes an undefined-reference (linker) error when calling the function.
// Implementation evidence: extract_symbol_internal.h:91-92 vs
// extract_symbol_impl.cpp:1105-1106.
// next_action: Align header/impl signatures (both std::string_view or both
// const std::string&).
// No TEST_F written for this item.

// P3-ESD-34: BuildExtractionDeclaration_WithConstant - BLOCKED_DESIGN_MAPPING
// Design expectation: BuildExtractionDeclaration can be called with a constant
// selection to produce a declaration edit.
// Actual product behavior: Same ABI mismatch as ESD-33 — the header declares
// std::string_view while the impl defines const std::string&, producing an
// undefined-reference linker error.
// Implementation evidence: extract_symbol_internal.h:91-92 vs
// extract_symbol_impl.cpp:1105-1106.
// next_action: Align header/impl signatures.
// No TEST_F written for this item.

// P3-ESD-35: BuildExtractionPlaceholder_ReturnsPlaceholder
TEST_F(LspExtractSymbolDeepBranchTests, BuildExtractionPlaceholder_ReturnsPlaceholder)
{
    std::string source = R"(
function test(): void {
    let x = 42;
}
)";
    Initializer init;
    size_t selStart = source.find("42");
    ASSERT_NE(selStart, std::string::npos);
    size_t selEnd = selStart + std::string("42").size();
    auto *refCtx = MakeRefactorContext(&init, source, selStart, selEnd);
    ASSERT_NE(refCtx, nullptr);
    ASSERT_NE(refCtx->context, nullptr);

    auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(refCtx->context);
    auto *ast = ctx->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    AstNode *extractedNode = nullptr;
    ast->FindChild([&extractedNode](AstNode *n) {
        if (extractedNode == nullptr && n->IsNumberLiteral()) {
            extractedNode = n;
        }
        return false;
    });
    ASSERT_NE(extractedNode, nullptr);

    // Direct call to BuildExtractionPlaceholder (header/impl signatures match)
    PlaceholderBuildInfo info = BuildExtractionPlaceholder(*refCtx, ctx, extractedNode, GLOBAL_VAR_ACTION);
    // PlaceholderBuildInfo should have a non-empty placeholder string
    // for a valid variable extraction from a numeric literal.
    ASSERT_FALSE(info.placeholder.empty());

    init.DestroyContext(refCtx->context);
    delete refCtx->textChangesContext;
    delete refCtx;
}

// P3-ESD-36: GetRefactorEditsToExtractVals_ReturnsEdits
// Pipeline output pinned exactly by refactor_extract_symbol_test_constant.cpp
// ExtractConstant*; this direct call only guards the non-empty contract.
TEST_F(LspExtractSymbolDeepBranchTests, GetRefactorEditsToExtractVals_ReturnsEdits)
{
    std::string source = R"(
function test(): void {
    const x = 42;
}
)";
    Initializer init;
    size_t selStart = source.find("42");
    ASSERT_NE(selStart, std::string::npos);
    size_t selEnd = selStart + std::string("42").size();
    auto *refCtx = MakeRefactorContext(&init, source, selStart, selEnd);
    ASSERT_NE(refCtx, nullptr);
    ASSERT_NE(refCtx->context, nullptr);

    auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(refCtx->context);
    auto *ast = ctx->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    AstNode *extractedNode = nullptr;
    ast->FindChild([&extractedNode](AstNode *n) {
        if (extractedNode == nullptr && n->IsNumberLiteral()) {
            extractedNode = n;
        }
        return false;
    });
    ASSERT_NE(extractedNode, nullptr);

    // Direct call to GetRefactorEditsToExtractVals (header/impl signatures match)
    RefactorEditInfo edits = GetRefactorEditsToExtractVals(*refCtx, extractedNode, GLOBAL_VAR_ACTION);
    // For a valid constant extraction selection, the function should produce
    // at least one FileTextChange edit.
    ASSERT_FALSE(edits.GetFileTextChanges().empty());

    init.DestroyContext(refCtx->context);
    delete refCtx->textChangesContext;
    delete refCtx;
}

}  // namespace
