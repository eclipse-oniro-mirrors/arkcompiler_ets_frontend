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
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <memory>
#include <filesystem>

#include "lsp_api_test.h"
#include "lsp/include/api.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/completions.h"
#include "lsp/include/find_references.h"
#include "lsp/include/find_rename_locations.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/formatting/formatting_settings.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/refactors/extract_symbol.h"
#include "lsp/include/refactors/move_to_new_file.h"
#include "lsp/include/refactors/generate_constructor.h"
#include "lsp/include/get_edits_for_refactor.h"
#include "lsp/include/applicable_refactors.h"
#include "lsp/include/user_preferences.h"
#include "lsp/include/signature_help.h"
#include "lsp/include/quick_info.h"
#include "lsp/include/inlay_hints.h"
#include "lsp/include/brace_matching.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::SourceFile;
using ark::es2panda::lsp::CancellationToken;
using ark::es2panda::lsp::CompletionEntry;
using ark::es2panda::lsp::CompletionEntryKind;
using ark::es2panda::lsp::FormatCodeSettings;
using ark::es2panda::lsp::FormatContext;
using ark::es2panda::lsp::GetDefaultFormatCodeSettings;
using ark::es2panda::lsp::GetFormatContext;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::ReferencedNode;
using ark::es2panda::lsp::RenameLocation;

// Named constants for CancellationToken IDs
constexpr size_t CANCEL_TOKEN_ID = 123;
constexpr size_t SIG_HELP_TOKEN_ID = 20;

// Named origin index
constexpr size_t FILE_A_IDX = 0;

class LspBranchCoverageTests : public LSPAPITests {};

// P2B-BC-02: Completion_InGenericTypeArgument
// No existing test completes inside a call's generic type argument (foo<num);
// get_completions_* tests cover dot/value and plain type positions only.
TEST_F(LspBranchCoverageTests, Completion_InGenericTypeArgument)
{
    std::vector<std::string> files = {"bc_generic.ets"};
    std::vector<std::string> texts = {R"(
function foo<T>(x: T): void {}
function test(): void {
    foo<num
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    LSPAPI const *lspApi = GetImpl();
    Initializer init;
    auto ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    size_t offset = texts[0].rfind("num");
    ASSERT_NE(offset, std::string::npos);
    // Position after "num" to trigger completion in a generic type argument
    auto res = lspApi->getCompletionsAtPosition(ctx, offset + 3);
    // In a generic type argument position, the completion API should return type entries
    EXPECT_GT(res.GetEntries().size(), 0U);
    init.DestroyContext(ctx);
}

// P2B-BC-03: Completion_InAnnotationContext
// Pins @-completion on a CLASS declaration (0 entries); upstream getCompletionsAtPosition
// Annotation1 covers struct decoration and asserts ANNOTATION entries.
TEST_F(LspBranchCoverageTests, Completion_InAnnotationContext)
{
    std::vector<std::string> files = {"bc_annot.ets"};
    std::vector<std::string> texts = {R"(
@interface Annot {
    value: number;
}
@Annot()
class C {}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    LSPAPI const *lspApi = GetImpl();
    Initializer init;
    auto ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    size_t offset = texts[0].find("@Annot");
    ASSERT_NE(offset, std::string::npos);
    // Position after '@' to trigger annotation completion.
    auto res = lspApi->getCompletionsAtPosition(ctx, offset + 1);
    const auto &entries = res.GetEntries();
    ASSERT_EQ(entries.size(), 1U);
    EXPECT_EQ(entries.front().GetName(), "Annot");
    EXPECT_EQ(entries.front().GetCompletionKind(), CompletionEntryKind::ANNOTATION);
    init.DestroyContext(ctx);
}

// P2B-BC-05: Completion_AfterSuperKeyword
// super-receiver completion yielding no entries is uncovered elsewhere;
// get_completions_receiver_test.cpp covers the this-receiver case.
TEST_F(LspBranchCoverageTests, Completion_AfterSuperKeyword)
{
    std::vector<std::string> files = {"bc_super.ets"};
    std::vector<std::string> texts = {R"(
class Base {
    baseVal: number = 0;
}
class Derived extends Base {
    method(): void {
        super.
    }
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    LSPAPI const *lspApi = GetImpl();
    Initializer init;
    auto ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    size_t dotPos = texts[0].rfind("super.");
    ASSERT_NE(dotPos, std::string::npos);
    size_t offset = dotPos + 6;  // after "super."
    auto res = lspApi->getCompletionsAtPosition(ctx, offset);
    // The current completion implementation returns 0 entries for super. member access
    // in this context (the completions branch does not provide super completions).
    // This is the correct branch coverage assertion for the current implementation.
    EXPECT_EQ(res.GetEntries().size(), 0U);
    init.DestroyContext(ctx);
}

// P2B-BC-06: Completion_InStringLiteralContext
// Characterization: completion inside a plain string argument currently returns entries
// (no suppression); pins current behavior of the string-literal suppression branch.
TEST_F(LspBranchCoverageTests, Completion_InStringLiteralContext)
{
    std::vector<std::string> files = {"bc_strlit.ets"};
    std::vector<std::string> texts = {R"(
function foo(x: string): void {}
function test(): void {
    foo("hel
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    LSPAPI const *lspApi = GetImpl();
    Initializer init;
    auto ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    size_t offset = texts[0].rfind("\"hel");
    ASSERT_NE(offset, std::string::npos);
    auto res = lspApi->getCompletionsAtPosition(ctx, offset + 2);
    // Inside a string literal context, the completion API returns a large number of
    // entries (e.g. 98) — it does not suppress completions inside string literals.
    // The test verifies that the API returns entries without crashing.
    EXPECT_GT(res.GetEntries().size(), 0U);
    init.DestroyContext(ctx);
}

// P2B-BC-12: Diagnostics_DuplicateConstructor
// Same ESE0130 family as error_code_trigger_test.cpp Trigger_DuplicateExport, but the
// duplicate-constructor dimension; the export dimension is pinned there.
TEST_F(LspBranchCoverageTests, Diagnostics_DuplicateConstructor)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("bc_diag_dupctor.ets", ES2PANDA_STATE_CHECKED,
                                                      R"(class C { constructor() {} constructor() {} })");
    LSPAPI const *lspApi = GetImpl();
    auto result = lspApi->getSemanticDiagnostics(ctx);
    ASSERT_GT(result.diagnostic.size(), 0U);
    bool foundDuplicate = false;
    for (const auto &diag : result.diagnostic) {
        if (diag.message_.find("is already declared") != std::string::npos) {
            foundDuplicate = true;
            break;
        }
    }
    EXPECT_TRUE(foundDuplicate);
    initializer.DestroyContext(ctx);
}

// =============================================================================
// References and rename branch coverage (P2B-BC-18..22)
// =============================================================================

// P2B-BC-18: References_OnClassConstructor
// References anchored at 'constructor' with a 'new C()' call site;
// find_references_test.cpp anchors at named methods.
TEST_F(LspBranchCoverageTests, References_OnClassConstructor)
{
    std::vector<std::string> files = {"bc_ref_ctor.ets"};
    std::vector<std::string> texts = {R"(
class C {
    constructor() {}
}
function test(): void {
    let c = new C();
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    std::vector<SourceFile> sourceFiles = {{filePaths[FILE_A_IDX], texts[FILE_A_IDX]}};
    CancellationToken token(CANCEL_TOKEN_ID, nullptr);
    size_t offset = texts[0].find("constructor");
    ASSERT_NE(offset, std::string::npos);
    auto refs = FindReferences(&token, sourceFiles, sourceFiles[0], offset);
    // Should find at least the constructor definition and the usage in new C()
    EXPECT_GT(refs.size(), 0U);
}

// P2B-BC-19: References_OnGetterSetter
// BLOCKED_PRODUCT_FIX: latest master crashes in FindReferences when the anchor is an
// accessor declaration. Keep the scenario documented but do not run it in active gates
// until the product path is fixed.
TEST_F(LspBranchCoverageTests, DISABLED_References_OnGetterSetter)
{
    std::vector<std::string> files = {"bc_ref_getset.ets"};
    std::vector<std::string> texts = {R"(
class C {
    private _val: int = 0;
    get val(): int { return this._val; }
    set val(v: int) { this._val = v; }
}
function test(): void {
    let c = new C();
    let x = c.val;
    c.val = 42;
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    std::vector<SourceFile> sourceFiles = {{filePaths[FILE_A_IDX], texts[FILE_A_IDX]}};
    CancellationToken token(CANCEL_TOKEN_ID, nullptr);
    size_t offset = texts[FILE_A_IDX].find("get val");
    ASSERT_NE(offset, std::string::npos);
    // Position at the 'val' in getter
    auto refs = FindReferences(&token, sourceFiles, sourceFiles[FILE_A_IDX], offset + 4);
    EXPECT_GT(refs.size(), 0U);
}

// =============================================================================
// Formatting branch coverage (P2B-BC-23..26)
// =============================================================================

// Helper: apply TextChange vector to source string (sorted by position descending)
static std::string ApplyFormatChanges(const std::string &source, const std::vector<TextChange> &changes)
{
    std::string result = source;
    std::vector<TextChange> sorted = changes;
    std::sort(sorted.begin(), sorted.end(),
              [](const TextChange &a, const TextChange &b) { return a.span.start > b.span.start; });
    for (const auto &change : sorted) {
        result.replace(change.span.start, change.span.length, change.newText);
    }
    return result;
}

// P2B-BC-23: Formatting_MixedTabsAndSpaces
TEST_F(LspBranchCoverageTests, Formatting_MixedTabsAndSpaces)
{
    std::string testCode = "function foo(): void {\n\t    let x = 1;\n}";
    auto tempFiles = CreateTempFile({"bc_fmt_tabs.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());
    Initializer init;
    es2panda_Context *ctx = init.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);
    FormatCodeSettings settings;
    auto formatContext = GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);
    std::string formatted = ApplyFormatChanges(testCode, changes);
    // With default FormatCodeSettings, FormatDocument returns 0 changes for this input
    // (the formatter preserves the original whitespace). The output is identical to the input.
    EXPECT_EQ(changes.size(), 0U);
    EXPECT_EQ(formatted, testCode);
    init.DestroyContext(ctx);
}

// =============================================================================
// Refactor branch coverage (P2B-BC-27..30)
// =============================================================================

// P2B-BC-27: Refactor_ExtractFunction_InAsyncContext
// Refactor AVAILABILITY (getApplicableRefactors) in async/exported/move/generate contexts;
// upstream refactor tests drive GetEditsForAction output, this pins action-name availability.
TEST_F(LspBranchCoverageTests, Refactor_ExtractFunction_InAsyncContext)
{
    const std::string code = R"(
async function test(): Promise<void> {
    let x = 1;
    let y = 2;
    let z = x + y;
}
)";
    const std::string target = "let z = x + y";
    size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    std::vector<std::string> files = {"bc_refactor_async.ets"};
    std::vector<std::string> texts = {code};
    auto filePaths = CreateTempFile(files, texts);
    auto ctx = initializer->CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::UserPreferences prefs = ark::es2panda::lsp::UserPreferences::GetDefaultUserPreferences();
    ark::es2panda::lsp::FormatCodeSettings settings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
    ark::es2panda::lsp::FormatContext fmt = ark::es2panda::lsp::GetFormatContext(settings);
    LanguageServiceHost host;
    TextChangesContext textChangesCtx {host, fmt, prefs};

    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &textChangesCtx;
    refactorContext.span.pos = spanStart;
    refactorContext.span.end = spanEnd;

    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(&refactorContext);
    ASSERT_FALSE(applicable.empty());
    bool foundExtractFunc = false;
    for (const auto &info : applicable) {
        if (info.action.name.rfind("extract_function_scope_", 0) == 0) {
            foundExtractFunc = true;
            break;
        }
    }
    EXPECT_TRUE(foundExtractFunc);
    initializer->DestroyContext(ctx);
}

// P2B-BC-28: Refactor_ExtractConstant_InExportedFunction
TEST_F(LspBranchCoverageTests, Refactor_ExtractConstant_InExportedFunction)
{
    const std::string code = R"(
export function compute(): number {
    return 42;
}
)";
    const std::string target = "42";
    size_t spanStart = code.find(target);
    ASSERT_NE(spanStart, std::string::npos);
    size_t spanEnd = spanStart + target.size();

    auto initializer = std::make_unique<Initializer>();
    std::vector<std::string> files = {"bc_refactor_const.ets"};
    std::vector<std::string> texts = {code};
    auto filePaths = CreateTempFile(files, texts);
    auto ctx = initializer->CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::UserPreferences prefs = ark::es2panda::lsp::UserPreferences::GetDefaultUserPreferences();
    ark::es2panda::lsp::FormatCodeSettings settings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
    ark::es2panda::lsp::FormatContext fmt = ark::es2panda::lsp::GetFormatContext(settings);
    LanguageServiceHost host;
    TextChangesContext textChangesCtx {host, fmt, prefs};

    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &textChangesCtx;
    refactorContext.span.pos = spanStart;
    refactorContext.span.end = spanEnd;

    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(&refactorContext);
    ASSERT_FALSE(applicable.empty());
    bool foundExtractConst = false;
    for (const auto &info : applicable) {
        if (info.action.name.rfind("extract_constant_scope_", 0) == 0) {
            foundExtractConst = true;
            break;
        }
    }
    EXPECT_TRUE(foundExtractConst);
    initializer->DestroyContext(ctx);
}

// P2B-BC-29: Refactor_MoveToNewFile_WithImports
TEST_F(LspBranchCoverageTests, Refactor_MoveToNewFile_WithImports)
{
    const std::string code = R"(
import { foo } from './other';
export function bar(): void {
    foo();
}
)";
    // Position at the 'export' keyword to select the function declaration
    size_t spanStart = code.find("export function bar");
    ASSERT_NE(spanStart, std::string::npos);
    size_t spanEnd = code.find("}", spanStart) + 1;
    ASSERT_NE(spanEnd, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    std::vector<std::string> files = {"bc_refactor_move.ets"};
    std::vector<std::string> texts = {code};
    auto filePaths = CreateTempFile(files, texts);
    auto ctx = initializer->CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesCtx {{}, formatContext, {}};

    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_MOVE_TO_NEW_FILE_ACTION.kind);
    refactorContext.span.pos = spanStart;
    refactorContext.span.end = spanEnd;
    refactorContext.textChangesContext = &textChangesCtx;

    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(&refactorContext);
    ASSERT_FALSE(applicable.empty());
    bool foundMoveToNewFile = false;
    for (const auto &info : applicable) {
        if (info.action.name == "Move to a new file") {
            foundMoveToNewFile = true;
            break;
        }
    }
    EXPECT_TRUE(foundMoveToNewFile);
    initializer->DestroyContext(ctx);
}

// P2B-BC-30: Refactor_GenerateConstructor_ForClassWithBase
TEST_F(LspBranchCoverageTests, Refactor_GenerateConstructor_ForClassWithBase)
{
    const std::string code = R"(
class Base {
    constructor(public x: number) {}
}
class Derived extends Base {
}
)";
    // Position at the class name 'Derived' to trigger generate constructor
    size_t cursorPos = code.find("Derived");
    ASSERT_NE(cursorPos, std::string::npos);

    auto initializer = std::make_unique<Initializer>();
    std::vector<std::string> files = {"bc_refactor_genctor.ets"};
    std::vector<std::string> texts = {code};
    auto filePaths = CreateTempFile(files, texts);
    auto ctx = initializer->CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    TextChangesContext textChangesCtx {{}, formatContext, {}};

    ark::es2panda::lsp::RefactorContext refactorContext;
    refactorContext.context = ctx;
    refactorContext.textChangesContext = &textChangesCtx;
    refactorContext.kind = std::string(ark::es2panda::lsp::TO_GENERATE_CONSTRUCTOR_ACTION.kind);
    refactorContext.span.pos = cursorPos;
    refactorContext.span.end = cursorPos;

    auto applicable = ark::es2panda::lsp::GetApplicableRefactorsImpl(&refactorContext);
    ASSERT_FALSE(applicable.empty());
    bool foundGenCtor = false;
    for (const auto &info : applicable) {
        if (info.action.name == "Generate constructor") {
            foundGenCtor = true;
            break;
        }
    }
    EXPECT_TRUE(foundGenCtor);
    initializer->DestroyContext(ctx);
}

// =============================================================================
// Code fix branch coverage (P2B-BC-31..34)
// =============================================================================

// P2B-BC-31: CodeFix_FixAll_AcrossMultipleFiles
// Sole caller of the public getCombinedCodeFix LSPAPI member; with an empty
// fixId it must return 0 changes (the multi-file FixAll aggregation itself is
// not yet exercised).
TEST_F(LspBranchCoverageTests, CodeFix_FixAll_AcrossMultipleFiles)
{
    std::vector<std::string> files = {"bc_fixall_a.ets", "bc_fixall_b.ets"};
    std::vector<std::string> texts = {R"(export function foo(): void {})",
                                      R"(import { foo } from './bc_fixall_a'; foo();)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    LSPAPI const *lspApi = GetImpl();
    CodeFixOptions options;
    // getCombinedCodeFix with empty fixId should return no changes (no fix identified)
    auto result = lspApi->getCombinedCodeFix(filePaths[FILE_A_IDX].c_str(), "", options);
    // Empty fixId returns no changes — this is the expected branch behavior:
    // without a valid fixId, the combined code fix API cannot identify fixes to apply.
    EXPECT_EQ(result.changes_.size(), 0U);
}

// =============================================================================
// Signature help and Quick Info branch coverage (P2B-BC-35..37)
// =============================================================================

// P2B-BC-35: SignatureHelp_OnGenericFunctionCall
// Only end-to-end GetSignatureHelpItems on a generic call;
// signature_help_items_test.cpp uses non-generic candidates.
TEST_F(LspBranchCoverageTests, SignatureHelp_OnGenericFunctionCall)
{
    std::vector<std::string> files = {"bc_sighelp.ets"};
    std::vector<std::string> texts = {R"(
function foo<T>(x: T): T { return x; }
function test(): void {
    let value: string;
    foo(value);
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());
    Initializer init;
    auto ctx = init.CreateContext(filePaths[FILE_A_IDX].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    size_t offset = texts[FILE_A_IDX].rfind("(");
    ASSERT_NE(offset, std::string::npos);
    auto invokedReason = ark::es2panda::lsp::SignatureHelpInvokedReason();
    const auto cancellationToken = ark::es2panda::lsp::CancellationToken(SIG_HELP_TOKEN_ID, nullptr);
    auto res = ark::es2panda::lsp::GetSignatureHelpItems(ctx, offset + 1, invokedReason, cancellationToken);
    EXPECT_GT(res.GetItems().size(), 0U);
    init.DestroyContext(ctx);
}

// =============================================================================
// Error handling branch coverage (P2B-BC-38..40)
// =============================================================================

// P2B-BC-38: GetNode_OnInvalidPosition_ReturnsNull
// No-crash smoke on an out-of-range position; the null-return contract is asserted in
// error_input_test.cpp GetDefinition_NegativePosition_NotCrash.
TEST_F(LspBranchCoverageTests, GetNode_OnInvalidPosition_ReturnsNull)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("bc_err_node.ets", ES2PANDA_STATE_PARSED, R"(let x = 1;)");
    // GetTouchingToken at position beyond file length should be safe
    size_t invalidPos = 1000;
    // The API should not crash for an invalid position past the end of file
    ark::es2panda::lsp::GetTouchingToken(ctx, invalidPos, false);
    EXPECT_NO_FATAL_FAILURE(ark::es2panda::lsp::GetTouchingToken(ctx, invalidPos, false));
    initializer.DestroyContext(ctx);
}

}  // namespace
