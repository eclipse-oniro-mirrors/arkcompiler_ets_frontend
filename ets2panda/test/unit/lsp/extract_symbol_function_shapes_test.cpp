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
#include <memory>
#include <string>
#include <vector>
#include "lsp/include/refactors/extract_symbol.h"
#include "lsp/include/refactors/refactor_types.h"
#include "lsp/include/get_edits_for_refactor.h"
#include "lsp/include/types.h"
#include "lsp/include/formatting/formatting.h"
#include "lsp/include/user_preferences.h"
#include "lsp/include/internal_api.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"
#include "public/public.h"

namespace {

using ark::es2panda::lsp::ApplicableRefactorInfo;
using ark::es2panda::lsp::FormatCodeSettings;
using ark::es2panda::lsp::FormatContext;
using ark::es2panda::lsp::GetDefaultFormatCodeSettings;
using ark::es2panda::lsp::GetFormatContext;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::RefactorContext;
using ark::es2panda::lsp::RefactorEditInfo;
using ark::es2panda::lsp::UserPreferences;

std::string ApplyEdits(const std::string &original, const std::vector<::TextChange> &edits)
{
    std::vector<const ::TextChange *> ordered;
    ordered.reserve(edits.size());
    for (const auto &change : edits) {
        ordered.push_back(&change);
    }
    std::stable_sort(ordered.begin(), ordered.end(), [](const ::TextChange *lhs, const ::TextChange *rhs) {
        return lhs->span.start < rhs->span.start;
    });

    std::string result;
    result.reserve(original.size());
    size_t cursor = 0;
    for (const auto *change : ordered) {
        size_t start = std::min(change->span.start, original.size());
        if (start < cursor) {
            start = cursor;
        }
        size_t end = std::min(start + change->span.length, original.size());
        end = std::max(end, cursor);
        if (cursor < start) {
            result.append(original, cursor, start - cursor);
        }
        result.append(change->newText);
        cursor = end;
    }
    if (cursor < original.size()) {
        result.append(original, cursor, original.size() - cursor);
    }
    return result;
}

// Inputs for one extract-function scenario. Scenario markers describe stable semantics instead
// of exact editor offsets or complete formatted output.
struct FunctionExtractionEdits {
    std::string code;
    std::string target;
    std::string actionName;
    std::vector<std::string> semanticMarkers;
    std::string fileName;
};

class ExtractSymbolFunctionShapesTests : public LSPAPITests {
public:
    struct OwnedRefactorContext {
        Initializer initializer;
        es2panda_Context *compilerContext = nullptr;
        ::TextChangesContext textChangesContext;
        std::unique_ptr<RefactorContext> refactorContext;

        OwnedRefactorContext(FormatContext &formatContext, const UserPreferences &preferences)
            : textChangesContext {::LanguageServiceHost {}, formatContext, preferences}
        {
        }

        ~OwnedRefactorContext()
        {
            if (compilerContext != nullptr) {
                initializer.DestroyContext(compilerContext);
            }
        }
    };

    std::unique_ptr<OwnedRefactorContext> MakeContext(const std::string &fileName, const std::string &code,
                                                      size_t start, size_t end)
    {
        auto filePaths = CreateTempFile({fileName}, {code});
        UserPreferences prefs = UserPreferences::GetDefaultUserPreferences();
        FormatCodeSettings settings = GetDefaultFormatCodeSettings("\n");
        FormatContext fmt = GetFormatContext(settings);
        auto owned = std::make_unique<OwnedRefactorContext>(fmt, prefs);
        owned->compilerContext = owned->initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
        if (owned->compilerContext == nullptr) {
            return nullptr;
        }

        owned->refactorContext = std::make_unique<RefactorContext>();
        owned->refactorContext->context = owned->compilerContext;
        owned->refactorContext->textChangesContext = &owned->textChangesContext;
        owned->refactorContext->span.pos = start;
        owned->refactorContext->span.end = end;
        return owned;
    }

    static bool HasAction(const std::vector<ApplicableRefactorInfo> &applicable, const std::string &actionName)
    {
        return std::any_of(applicable.begin(), applicable.end(),
                           [&](const ApplicableRefactorInfo &info) { return info.action.name == actionName; });
    }

    static bool LocationInsideToken(const std::string &text, const std::string &token, size_t location)
    {
        size_t position = text.find(token);
        while (position != std::string::npos) {
            if (location >= position && location < position + token.size()) {
                return true;
            }
            position = text.find(token, position + token.size());
        }
        return false;
    }

    static void ExpectContains(const std::string &text, const std::string &marker, const std::string &context)
    {
        EXPECT_NE(text.find(marker), std::string::npos) << context << "\nactual: " << text;
    }

    void ExpectFunctionExtractionEdits(const FunctionExtractionEdits &scenario)
    {
        const size_t start = scenario.code.find(scenario.target);
        ASSERT_NE(start, std::string::npos);
        auto owned = MakeContext(scenario.fileName, scenario.code, start, start + scenario.target.size());
        ASSERT_NE(owned, nullptr);
        ASSERT_NE(owned->compilerContext, nullptr);

        auto *refactorContext = owned->refactorContext.get();
        auto applicable = GetApplicableRefactorsImpl(refactorContext);
        ASSERT_TRUE(HasAction(applicable, scenario.actionName));

        const std::string refactorName = std::string(ark::es2panda::lsp::refactor_name::EXTRACT_FUNCTION_ACTION_NAME);
        auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(*refactorContext, refactorName, scenario.actionName);
        ASSERT_NE(edits, nullptr);
        ASSERT_FALSE(edits->GetFileTextChanges().empty());
        const auto &fileEdit = edits->GetFileTextChanges().front();
        ASSERT_FALSE(fileEdit.textChanges.empty());

        const std::string applied = ApplyEdits(scenario.code, fileEdit.textChanges);
        const bool isMethodExtraction = applied.find("private newMethod(") != std::string::npos;
        const std::string helperMarker = isMethodExtraction ? "private newMethod" : "function newFunction";
        const std::string callMarker = isMethodExtraction ? "this.newMethod" : "newFunction";
        ExpectContains(applied, helperMarker, scenario.fileName);
        ExpectContains(applied, callMarker, scenario.fileName);
        for (const auto &marker : scenario.semanticMarkers) {
            ExpectContains(applied, marker, scenario.fileName);
        }

        ASSERT_TRUE(edits->GetRenameLocation().has_value());
        const size_t renameLocation = edits->GetRenameLocation().value();
        ASSERT_LT(renameLocation, applied.size());
        const bool insideGeneratedName = LocationInsideToken(applied, callMarker, renameLocation) ||
                                         LocationInsideToken(applied, helperMarker, renameLocation);
        const std::string message =
            "rename location " + std::to_string(renameLocation) + " is outside generated name in:\n" + applied;
        ASSERT_TRUE(insideGeneratedName) << message;
    }
};

// A multiline arrow expression selected as a call argument keeps the expression in a generated
// function and replaces the original argument with a call.
TEST_F(ExtractSymbolFunctionShapesTests, MultilineRewriteArrowExpressionCallArgument)
{
    const std::string code = R"(
function supply(v: () => int): int {
    return v();
}
function host(): int {
    let r = supply(() => 2 +
        3);
    return r;
}
)";
    ExpectFunctionExtractionEdits(
        {code, "() => 2 +\n        3", "extract_function_scope_2", {"return () => 2 +"}, "MultilineArrow.ets"});
}

// A multiline statement selection is represented as a generated function and a call at the
// original statement site.
TEST_F(ExtractSymbolFunctionShapesTests, MultilineStatementSelectionNotRewritten)
{
    const std::string code = R"(
function f(a: int): int {
    let v = a +
        100;
    return v;
}
)";
    ExpectFunctionExtractionEdits(
        {code, "let v = a +\n        100;", "extract_function_scope_2", {"return a +"}, "MultilineStmt.ets"});
}

// A multiline top-level selection still exposes a global extract-function action and produces a
// stable helper-call transformation.
TEST_F(ExtractSymbolFunctionShapesTests, EmptyRefactorFallbackTopLevelMultiline)
{
    const std::string code = "let a = 1;\nlet b = 2;\nfunction use(): int { return a + b; }\n";
    ExpectFunctionExtractionEdits(
        {code, "let a = 1;\nlet b = 2;", "extract_function_scope_2", {"let a = 1;", "let b = 2;"}, "TopLevel.ets"});
}

// A text selection containing a complete declaration plus an extra semicolon exercises the
// declaration-text fallback without binding its historical edit list.
TEST_F(ExtractSymbolFunctionShapesTests, EmptyRefactorFallbackDeclarationTextSelection)
{
    const std::string code = "let q = 5;;\nfunction use(): int { return q; }\n";
    ExpectFunctionExtractionEdits({code, "let q = 5;;", "extract_function_scope_2", {"return 5;"}, "DeclText.ets"});
}

// Multiline numeric declarations preserve their initializer expression in the generated helper.
TEST_F(ExtractSymbolFunctionShapesTests, TopLevelLeadingDeclMultilineNumeric)
{
    const std::string code = "let v =\n    9;\nfunction use(): int { return 0; }\n";
    ExpectFunctionExtractionEdits(
        {code, "let v =\n    9;", "extract_function_scope_2", {"return v;"}, "LeadingNumeric.ets"});
}

// A declared annotation remains visible as stable semantic information in the generated helper.
TEST_F(ExtractSymbolFunctionShapesTests, TopLevelLeadingDeclMultilineConstAnnotated)
{
    const std::string code = "const k: int =\n    12;\nfunction use(): int { return 0; }\n";
    ExpectFunctionExtractionEdits({code,
                                   "const k: int =\n    12;",
                                   "extract_function_scope_2",
                                   {"const k: int =", "return k;"},
                                   "LeadingConst.ets"});
}

// A leading block comment remains in the source while extraction inserts a callable helper.
TEST_F(ExtractSymbolFunctionShapesTests, TopLevelLeadingDeclAfterBlockComment)
{
    const std::string code = "/* lead */ let k =\n    8;\nfunction use(): int { return 0; }\n";
    ExpectFunctionExtractionEdits(
        {code, "let k =\n    8;", "extract_function_scope_2", {"/* lead */", "return k;"}, "LeadingComment.ets"});
}

// Extracting a return statement produces a helper whose result is returned by a call.
TEST_F(ExtractSymbolFunctionShapesTests, CallReplacementPlainReturnReturnCallResult)
{
    const std::string code = R"(
function pick(a: int): int {
    return a * 2;
}
function use(): int {
    return pick(3);
}
)";
    ExpectFunctionExtractionEdits(
        {code, "return pick(3);", "extract_function_scope_2", {"return pick(3);"}, "CallPlain.ets"});
}

// The await expression is retained inside an async helper and called from the original return.
TEST_F(ExtractSymbolFunctionShapesTests, CallReplacementAwaitReturnPrefix)
{
    const std::string code = R"(
async function query(): Promise<int> {
    return 5;
}
async function load(): Promise<int> {
    let p = query();
    return await p;
}
)";
    ExpectFunctionExtractionEdits(
        {code, "return await p;", "extract_function_scope_2", {"return await p;"}, "CallAwait.ets"});
}

// Extraction inside a generic function preserves explicit type arguments on the generated call.
TEST_F(ExtractSymbolFunctionShapesTests, CallReplacementGenericTypeArgs)
{
    const std::string code = R"(
function first<T>(a: T): T {
    let copy = a;
    return copy;
}
function use(): int {
    return first<int>(5);
}
)";
    ExpectFunctionExtractionEdits(
        {code, "let copy = a;", "extract_function_scope_2", {"function newFunction<T>("}, "CallGeneric.ets"});
}

// Class-scope extraction generates a private method and calls it through this.
TEST_F(ExtractSymbolFunctionShapesTests, CallReplacementClassPrivateThisCall)
{
    const std::string code = R"(
class C {
    scale(v: int): int {
        return v * 2;
    }

    compute(v: int): int {
        return this.scale(v) * 2;
    }
}
)";
    ExpectFunctionExtractionEdits(
        {code, "return this.scale(v) * 2;", "extract_function_scope_1", {"return this.scale(v) * 2;"}, "CallThis.ets"});
}

// For an expression extraction, the rename anchor points at a generated newFunction token rather
// than relying on a product-internal byte offset.
TEST_F(ExtractSymbolFunctionShapesTests, CallRenameLocExpressionExtraction)
{
    const std::string code = R"(
function pick(a: int): int {
    return a * 2;
}
function use(): int {
    return pick(3);
}
)";
    ExpectFunctionExtractionEdits({code, "pick(3)", "extract_function_scope_2", {}, "CallRenameLoc.ets"});
}

// For a statement extraction, prior helper insertion may shift the rename location; it must still
// remain within the generated function name in applied text.
TEST_F(ExtractSymbolFunctionShapesTests, CallRenameLocAssignedStatement)
{
    const std::string code = R"(
function demo(): int {
    let acc = 0;
    let factor = 2;
    acc = acc + 5;
    acc = acc * factor;
    return acc;
}
)";
    ExpectFunctionExtractionEdits({code,
                                   "acc = acc + 5;\n    acc = acc * factor;",
                                   "extract_function_scope_2",
                                   {"return acc + 5;"},
                                   "CallRenameLocStmt.ets"});
}

// Primitive literal declarations expose their inferred return annotation as a semantic marker.
TEST_F(ExtractSymbolFunctionShapesTests, InferReturnPrimitiveLiteralKinds)
{
    struct LiteralCase {
        const char *decl;
        const char *annotation;
    };
    const std::vector<LiteralCase> cases = {
        {"let n =\n    7;", ": int"},
        {"let s =\n    \"abc\";", ": String"},
        {"let b =\n    true;", ": boolean"},
    };
    for (const auto &literalCase : cases) {
        std::string code = std::string(literalCase.decl) + "\nfunction use(): int { return 0; }\n";
        ExpectFunctionExtractionEdits({code,
                                       literalCase.decl,
                                       "extract_function_scope_2",
                                       {std::string("function newFunction()") + literalCase.annotation + " {"},
                                       "InferLiteralPrim.ets"});
    }
}

// Literal kinds without a stable primitive annotation still generate a callable helper while
// preserving the original literal expression.
TEST_F(ExtractSymbolFunctionShapesTests, InferReturnCompositeLiteralKinds)
{
    const std::vector<std::string> declarations = {
        "let c =\n    'x';",
        "let z =\n    null;",
        "let arr =\n    [1, 2, 3];",
    };
    for (const auto &declaration : declarations) {
        std::string code = declaration + "\nfunction use(): int { return 0; }\n";
        ExpectFunctionExtractionEdits(
            {code, declaration, "extract_function_scope_2", {"function newFunction() {"}, "InferLiteralComp.ets"});
    }
}

// A whole annotated arrow initializer is preserved in a helper with an empty wrapper signature.
TEST_F(ExtractSymbolFunctionShapesTests, ParamsArrowWholeInitializerSingle)
{
    const std::string code = R"(
function host(): int {
    let inc = (v: int): int => v + 1;
    return inc(1);
}
)";
    ExpectFunctionExtractionEdits({code,
                                   "(v: int): int => v + 1",
                                   "extract_function_scope_2",
                                   {"function newFunction() {", "(v: int): int => v + 1"},
                                   "ParamsSingle.ets"});
}

// Multiple arrow parameters remain on the preserved arrow expression rather than its wrapper.
TEST_F(ExtractSymbolFunctionShapesTests, ParamsArrowWholeInitializerTwoAnnotated)
{
    const std::string code = R"(
function host(): int {
    let add = (a: int, b: int): int => a + b;
    return add(1, 2);
}
)";
    ExpectFunctionExtractionEdits({code,
                                   "(a: int, b: int): int => a + b",
                                   "extract_function_scope_2",
                                   {"function newFunction() {", "(a: int, b: int): int => a + b"},
                                   "ParamsTwo.ets"});
}

// A literal-only expression generates a helper without source parameters.
TEST_F(ExtractSymbolFunctionShapesTests, ParamsNoParamsLiteralExpression)
{
    const std::string code = R"(
function host(): int {
    let v = 2 +
        3;
    return v;
}
)";
    ExpectFunctionExtractionEdits({code,
                                   "2 +\n        3",
                                   "extract_function_scope_2",
                                   {"function newFunction(", "return 2 +"},
                                   "ParamsNone.ets"});
}
}  // namespace
