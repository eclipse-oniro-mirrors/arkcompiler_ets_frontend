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

// P1-D batch 1 — Signature Help arm coverage (ledger rows PH-001..PH-006).
// PH-007/PH-008/PH-009 have no tests by design (BLOCKED_PRODUCT_FIX, ledger record-keeping only):
//  - PH-007: GetArgumentCount true-arms (signature_help.cpp:241-253) are dead because both
//    production call sites (signature_help.cpp:290 and :354) pass a hard-coded false for the
//    ignoreTrailingComma input; "trailing comma" is a misnomer, no comma logic exists in the file.
//  - PH-008: GetHighestBinary (signature_help.cpp:193-199) has no live caller (self-recursion only,
//    plus a const/non-const header-source mismatch at include/signature_help.h:290 that breaks
//    direct linking, see signature_help_internals_test.cpp P1B-SH-04).
//  - PH-009: GetChildListThatStartsWithOpenerToken (signature_help.cpp:225-238) is declaration-only
//    dead code (include/signature_help.h:303, zero callers repo-wide).
//
// Unreachable-with-evidence chains recorded for this batch (no test can reach them from a real
// ETS context, and this file must not fabricate internal state):
//  - PH-001 OPTIONAL "?" arc (signature_help_items.cpp:127-129): varbinder::VariableFlags::OPTIONAL
//    is only set on parameter variables by the TS route (parser/TSparser.cpp:1685,
//    checker/ts/function.cpp:131/186/224). The ETS parser records optionality at AST level only
//    (parser/ETSparser.cpp:1770) and never sets the variable flag, and the LSP plugin API can only
//    create ETS contexts (public/es2panda_lib.cpp:495 "Plugin API supports only ETS"). Therefore
//    the "?" marker can never render in any LSP-reachable signature help output.
//  - PH-004 flow-level TypeInfo arm (signature_help.cpp:183-185): GetCandidateOrTypeInfo returns
//    TypeInfo only for TypeArgsInvocation, and TypeArgsInvocation has zero producers in lsp/
//    (variant declared include/create_type_help_items.h:38-49; consumed only at
//    signature_help.cpp:107; the only invocation constructors in the flow are CallInvocation at
//    signature_help.cpp:261/272/357 and ContextualInvocation at :307-313/:372-374). The TYPEENUM
//    arm itself is covered by direct construction (GetCandidateOrTypeInfoTypeArgsYieldsTypeInfo).

#include <gtest/gtest.h>

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "lsp/include/internal_api.h"
#include "lsp/include/signature_help.h"
#include "lsp/include/signature_help_items.h"
#include "lsp_api_test.h"
#include "checker/types/signature.h"
#include "ir/astNode.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::ir::AstNode;
using ark::es2panda::lsp::ArgumentListInfo;
using ark::es2panda::lsp::CallInvocation;
using ark::es2panda::lsp::CancellationToken;
using ark::es2panda::lsp::CandidateInfo;
using ark::es2panda::lsp::CandidateOrTypeKind;
using ark::es2panda::lsp::ContextualInvocation;
using ark::es2panda::lsp::CreateSignatureHelpItems;
using ark::es2panda::lsp::GetArgumentIndexForTemplatePiece;
using ark::es2panda::lsp::GetArgumentOrParameterListAndIndex;
using ark::es2panda::lsp::GetCandidateOrTypeInfo;
using ark::es2panda::lsp::GetImmediatelyContainingArgumentInfo;
using ark::es2panda::lsp::GetSignatureHelpItems;
using ark::es2panda::lsp::HostCancellationToken;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::Invocation;
using ark::es2panda::lsp::InvocationKind;
using ark::es2panda::lsp::SignatureHelpCharacterTypedReason;
using ark::es2panda::lsp::SignatureHelpInvokedReason;
using ark::es2panda::lsp::TryGetParameterInfo;
using ark::es2panda::lsp::TypeArgsInvocation;

constexpr size_t SHAPES_DEFAULT_TIME = 20;
constexpr size_t SHAPES_CONTEXTUAL_INVOCATION_INDEX = 2;
// Query position used for the null-node / null-parent boundary arcs of
// GetImmediatelyContainingArgumentInfo.
constexpr size_t SHAPES_BOUNDARY_POS = 5;

class AlwaysCancelledHost : public HostCancellationToken {
public:
    bool IsCancellationRequested() override
    {
        return true;
    }
};

class SignatureHelpShapesTests : public LSPAPITests {
protected:
    static AstNode *FindNode(AstNode *root, const std::function<bool(AstNode *)> &pred)
    {
        AstNode *result = nullptr;
        root->FindChild([&result, &pred](AstNode *n) {
            if (result == nullptr && pred(n)) {
                result = n;
            }
            return false;
        });
        return result;
    }

    static AstNode *FindCallByCalleeName(AstNode *root, const std::string &calleeName)
    {
        AstNode *result = nullptr;
        root->FindChild([&result, &calleeName](AstNode *n) {
            if (result == nullptr && n->IsCallExpression() &&
                n->AsCallExpression()->Callee()->AsIdentifier()->Name().Utf8() == calleeName) {
                result = n;
            }
            return false;
        });
        return result;
    }

    static void ExpectEmptyAndMismatchGuards(ark::es2panda::checker::Signature *fooSig,
                                             ark::es2panda::checker::Signature *barSig, const ArgumentListInfo &info)
    {
        // Empty candidate list: early return before any item is built (signature_help_items.cpp:41-42).
        std::vector<ark::es2panda::checker::Signature *> emptyList;
        auto fromEmpty = CreateSignatureHelpItems(emptyList, fooSig, std::make_optional(info));
        EXPECT_TRUE(fromEmpty.GetItems().empty());

        // Resolved signature missing from the candidate list: selectedItemIndex stays -1 -> early
        // return (signature_help_items.cpp:35-42).
        std::vector<ark::es2panda::checker::Signature *> mismatched {fooSig};
        auto fromMismatch = CreateSignatureHelpItems(mismatched, barSig, std::make_optional(info));
        EXPECT_TRUE(fromMismatch.GetItems().empty());
    }

    static void ExpectDeclaredTemplateItemShape(SignatureHelpItem &declItem)
    {
        // Item 0: declared type parameters <T, U> and parameter types T/U.
        auto expectedDeclPrefix = std::vector<SymbolDisplayPart> {
            SymbolDisplayPart {"<", "punctuation"}, SymbolDisplayPart {"T", "typeName"},
            SymbolDisplayPart {",", "punctuation"}, SymbolDisplayPart {"U", "typeName"},
            SymbolDisplayPart {">", "punctuation"}, SymbolDisplayPart {"(", "punctuation"},
        };
        EXPECT_EQ(declItem.GetPrefixDisplayParts(), expectedDeclPrefix);
        auto expectedDeclSeparator = std::vector<SymbolDisplayPart> {SymbolDisplayPart {",", "punctuation"}};
        EXPECT_EQ(declItem.GetSeparatorDisplayParts(), expectedDeclSeparator);
        ASSERT_EQ(declItem.GetParameters().size(), 2U);
        EXPECT_EQ(declItem.GetParameters()[0].GetName(), "a");
        auto expectedDeclA = std::vector<SymbolDisplayPart> {
            SymbolDisplayPart {"a", "paramName"},
            SymbolDisplayPart {":", "punctuation"},
            SymbolDisplayPart {"T", "typeName"},
            SymbolDisplayPart {",", "punctuation"},
        };
        EXPECT_EQ(declItem.GetParameters()[0].GetDisplayParts(), expectedDeclA);
        auto expectedDeclB = std::vector<SymbolDisplayPart> {
            SymbolDisplayPart {"b", "paramName"},
            SymbolDisplayPart {":", "punctuation"},
            SymbolDisplayPart {"U", "typeName"},
        };
        EXPECT_EQ(declItem.GetParameters()[1].GetDisplayParts(), expectedDeclB);
    }

    static void ExpectSubstitutedTemplateItemsShape(SignatureHelpItems &res)
    {
        // Items 1..2: substituted call signatures <Double, String>.
        for (size_t i = 1; i < res.GetItems().size(); ++i) {
            auto &callItem = res.GetItem(i);
            auto expectedCallPrefix = std::vector<SymbolDisplayPart> {
                SymbolDisplayPart {"<", "punctuation"}, SymbolDisplayPart {"Double", "typeName"},
                SymbolDisplayPart {",", "punctuation"}, SymbolDisplayPart {"String", "typeName"},
                SymbolDisplayPart {">", "punctuation"}, SymbolDisplayPart {"(", "punctuation"},
            };
            EXPECT_EQ(callItem.GetPrefixDisplayParts(), expectedCallPrefix);
            ASSERT_EQ(callItem.GetParameters().size(), 2U);
            auto expectedCallA = std::vector<SymbolDisplayPart> {
                SymbolDisplayPart {"a", "paramName"},
                SymbolDisplayPart {":", "punctuation"},
                SymbolDisplayPart {"Double", "typeName"},
                SymbolDisplayPart {",", "punctuation"},
            };
            EXPECT_EQ(callItem.GetParameters()[0].GetDisplayParts(), expectedCallA);
            auto expectedCallB = std::vector<SymbolDisplayPart> {
                SymbolDisplayPart {"b", "paramName"},
                SymbolDisplayPart {":", "punctuation"},
                SymbolDisplayPart {"String", "typeName"},
            };
            EXPECT_EQ(callItem.GetParameters()[1].GetDisplayParts(), expectedCallB);
        }
    }
};

// PH-001: signature_help_items.cpp:143-164 — restVar tail rendering of SetSignatureHelpParameter
// (",", "...", name, ":", type) reached end-to-end through signature_help.cpp:151-192.
// Measured candidate shape (same as in the exemplar suite): a call to a top-level ETS function
// resolves to 3 candidates (the lowered METHOD_DEFINITION "g" plus the duplicated CallExpression
// nodes), all rendering identically; the resolved call signature matches at index 0.
TEST_F(SignatureHelpShapesTests, RestParameterTailRenderedFromCallSite)
{
    std::vector<std::string> files = {"shapes_rest_tail.ets"};
    // offsets: g(1, 2) call span = {60, 7}; query position 62 sits inside argument `1`.
    std::vector<std::string> texts = {R"(function g(...args: number[]): void {
    return;
}
let r = g(1, 2);
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t position = texts[0].rfind("(") + 1;
    ASSERT_EQ(position, 62U);
    auto invokedReason = SignatureHelpInvokedReason();
    auto cancellationToken = CancellationToken(SHAPES_DEFAULT_TIME, nullptr);
    auto res = GetSignatureHelpItems(ctx, position, invokedReason, cancellationToken);

    ASSERT_EQ(res.GetItems().size(), 3U);
    EXPECT_EQ(res.GetApplicableSpan().start, 60U);
    EXPECT_EQ(res.GetApplicableSpan().length, 7U);
    EXPECT_EQ(res.GetSelectedItemIndex(), 0U);
    // ArgCount() is 0 (restVar is kept out of params), so the min at signature_help_items.cpp:59
    // degenerates and the argument index stays at the call start index 60.
    EXPECT_EQ(res.GetArgumentIndex(), 60U);
    EXPECT_EQ(res.GetArgumentCount(), 0U);  // signature_help.cpp:354 GetArgumentCount counts 0 members

    for (size_t i = 0; i < res.GetItems().size(); ++i) {
        auto &item = res.GetItem(i);
        auto expectedPrefix = std::vector<SymbolDisplayPart> {SymbolDisplayPart {"(", "punctuation"}};
        auto expectedSuffix = std::vector<SymbolDisplayPart> {
            SymbolDisplayPart {")", "punctuation"},
            SymbolDisplayPart {":", "punctuation"},
            SymbolDisplayPart {"undefined", "typeName"},
        };
        EXPECT_EQ(item.GetPrefixDisplayParts(), expectedPrefix);
        EXPECT_EQ(item.GetSuffixDisplayParts(), expectedSuffix);
        EXPECT_TRUE(item.GetSeparatorDisplayParts().empty());

        ASSERT_EQ(item.GetParameters().size(), 1U);
        // Measured: the restVar block (signature_help_items.cpp:147-164) never calls SetName, so
        // the parameter name stays empty while the display parts still carry the declared name.
        EXPECT_EQ(item.GetParameters()[0].GetName(), "");
        auto expectedRestParts = std::vector<SymbolDisplayPart> {
            SymbolDisplayPart {"...", "punctuation"},
            SymbolDisplayPart {"args", "paramName"},
            SymbolDisplayPart {":", "punctuation"},
            SymbolDisplayPart {"Array<Double>", "typeName"},
        };
        EXPECT_EQ(item.GetParameters()[0].GetDisplayParts(), expectedRestParts);
    }
    initializer.DestroyContext(ctx);
}

// PH-003: signature_help_items.cpp:56-60 — rest-parameter argumentIndex clamp when
// HasRestParameter() && ArgCount() > 1, driven both directly and through the public flow.
TEST_F(SignatureHelpShapesTests, RestParameterClampArgIndexWithLeadingParams)
{
    std::vector<std::string> files = {"shapes_rest_clamp.ets"};
    // offsets: h(1, 2, 3) call span = {86, 10}; query position 88 sits inside argument `1`.
    std::vector<std::string> texts = {R"(function h(a: number, b: number, ...rest: number[]): number {
    return 0;
}
let r = h(1, 2, 3);
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *callExpr = FindNode(ast, [](AstNode *n) { return n->IsCallExpression(); });
    ASSERT_NE(callExpr, nullptr);
    ASSERT_TRUE(callExpr->IsCallExpression());
    auto *sig = callExpr->AsCallExpression()->Signature();
    ASSERT_NE(sig, nullptr);
    ASSERT_TRUE(sig->HasRestParameter());
    ASSERT_EQ(sig->ArgCount(), 2U);  // leading params only; restVar kept separate

    // Direct drive: oversized argumentIndex 5 must clamp to ArgCount() = 2 (signature_help_items.cpp:56-57).
    ArgumentListInfo info;
    info.SetArgumentIndex(5U);
    info.SetArgumentCount(3U);
    std::vector<ark::es2panda::checker::Signature *> signatures {sig};
    auto direct = CreateSignatureHelpItems(signatures, sig, std::make_optional(info));
    ASSERT_EQ(direct.GetItems().size(), 1U);
    EXPECT_EQ(direct.GetSelectedItemIndex(), 0U);
    EXPECT_EQ(direct.GetArgumentIndex(), 2U);
    EXPECT_EQ(direct.GetArgumentCount(), 3U);

    // Flow drive: argumentIndex enters as the call start (86, signature_help.cpp:292) and is
    // clamped down to 2 by the same arm. The call resolves to 3 identical candidates (see
    // RestParameterTailRenderedFromCallSite); the match lands at index 0.
    auto invokedReason = SignatureHelpInvokedReason();
    auto cancellationToken = CancellationToken(SHAPES_DEFAULT_TIME, nullptr);
    auto res = GetSignatureHelpItems(ctx, texts[0].rfind("(") + 1, invokedReason, cancellationToken);
    ASSERT_EQ(res.GetItems().size(), 3U);
    EXPECT_EQ(res.GetApplicableSpan().start, 86U);
    EXPECT_EQ(res.GetApplicableSpan().length, 10U);
    EXPECT_EQ(res.GetSelectedItemIndex(), 0U);
    EXPECT_EQ(res.GetArgumentIndex(), 2U);
    EXPECT_EQ(res.GetArgumentCount(), 0U);
    initializer.DestroyContext(ctx);
}

// PH-003: signature_help_items.cpp:33-43 — dedup loop and the early-return guards
// (empty candidate list; resolved signature ToString not present among candidates).
TEST_F(SignatureHelpShapesTests, CreateSignatureHelpItemsDedupAndEmptyGuards)
{
    std::vector<std::string> files = {"shapes_dedup.ets"};
    std::vector<std::string> texts = {R"(function foo(x: number): void {
    return;
}
function bar(y: number): number {
    return y;
}
let a = foo(1);
let b = bar(2);
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);

    auto *fooCall = FindCallByCalleeName(ast, "foo");
    auto *barCall = FindCallByCalleeName(ast, "bar");
    ASSERT_NE(fooCall, nullptr);
    ASSERT_NE(barCall, nullptr);
    auto *fooSig = fooCall->AsCallExpression()->Signature();
    auto *barSig = barCall->AsCallExpression()->Signature();
    ASSERT_NE(fooSig, nullptr);
    ASSERT_NE(barSig, nullptr);
    // Precondition pin: the two signatures must compare unequal so the dedup loop really scans.
    EXPECT_NE(fooSig->ToString(), barSig->ToString());

    ArgumentListInfo info;
    info.SetArgumentIndex(0U);
    info.SetArgumentCount(1U);

    ExpectEmptyAndMismatchGuards(fooSig, barSig, info);

    // Duplicate-overload source with the match at index 1: the loop increments itemsSeen past the
    // first entry and selects the first ToString match (signature_help_items.cpp:33-40, 51).
    std::vector<ark::es2panda::checker::Signature *> duplicated {barSig, fooSig, fooSig};
    auto fromDup = CreateSignatureHelpItems(duplicated, fooSig, std::make_optional(info));
    ASSERT_EQ(fromDup.GetItems().size(), 3U);
    EXPECT_EQ(fromDup.GetSelectedItemIndex(), 1U);
    EXPECT_EQ(fromDup.GetArgumentIndex(), 0U);
    initializer.DestroyContext(ctx);
}

// PH-002: signature_help_items.cpp:104-109 — the "=>" else-arm renders for signatures without a
// ScriptFunction (function-type call through an arrow signature). Measured: the call to a
// function-type variable resolves to the 2 duplicated CallExpression candidates, both rendering
// the arrow suffix; selected match at index 0.
TEST_F(SignatureHelpShapesTests, FunctionTypeCallRendersArrowSuffix)
{
    std::vector<std::string> files = {"shapes_arrow_suffix.ets"};
    // offsets: g(1) call span = {81, 4}; query position 83 sits inside argument `1`.
    std::vector<std::string> texts = {R"(let g: (a: number) => number = (a: number): number => {
    return a;
};
let r = g(1);
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t position = texts[0].rfind("(") + 1;
    ASSERT_EQ(position, 83U);
    auto invokedReason = SignatureHelpInvokedReason();
    auto cancellationToken = CancellationToken(SHAPES_DEFAULT_TIME, nullptr);
    auto res = GetSignatureHelpItems(ctx, position, invokedReason, cancellationToken);

    ASSERT_EQ(res.GetItems().size(), 2U);
    EXPECT_EQ(res.GetApplicableSpan().start, 81U);
    EXPECT_EQ(res.GetApplicableSpan().length, 4U);
    EXPECT_EQ(res.GetSelectedItemIndex(), 0U);
    EXPECT_EQ(res.GetArgumentIndex(), 0U);  // signature_help_items.cpp:59 min(81, 1-1)
    EXPECT_EQ(res.GetArgumentCount(), 0U);

    for (size_t i = 0; i < res.GetItems().size(); ++i) {
        auto &item = res.GetItem(i);
        // signature_help_items.cpp:104-108: no ScriptFunction and no METHOD owner var => "=>" suffix.
        auto expectedSuffix = std::vector<SymbolDisplayPart> {
            SymbolDisplayPart {")", "punctuation"},
            SymbolDisplayPart {"=>", "punctuation"},
            SymbolDisplayPart {"Double", "typeName"},
        };
        EXPECT_EQ(item.GetSuffixDisplayParts(), expectedSuffix);
        ASSERT_EQ(item.GetParameters().size(), 1U);
        EXPECT_EQ(item.GetParameters()[0].GetName(), "a");
        auto expectedParts = std::vector<SymbolDisplayPart> {
            SymbolDisplayPart {"a", "paramName"},
            SymbolDisplayPart {":", "punctuation"},
            SymbolDisplayPart {"Double", "typeName"},
        };
        EXPECT_EQ(item.GetParameters()[0].GetDisplayParts(), expectedParts);
    }
    initializer.DestroyContext(ctx);
}

// PH-002 counterpart + PH-006 member-call shape: signature_help_items.cpp:104-106 ":" arm and
// signature_help.cpp:343-361 MemberExpression parent hoist (member callee counted as argument 1).
TEST_F(SignatureHelpShapesTests, MemberCallSignaturePinsColonSuffixAndArgCount)
{
    std::vector<std::string> files = {"shapes_member_call.ets"};
    // offsets: p.set(9) call span = {62, 8}; query position 68 sits inside argument `9`.
    std::vector<std::string> texts = {R"(class P {
    set(v: number): void {
    }
}
let p = new P();
p.set(9);
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t position = texts[0].rfind("(") + 1;
    ASSERT_EQ(position, 68U);
    auto invokedReason = SignatureHelpInvokedReason();
    auto cancellationToken = CancellationToken(SHAPES_DEFAULT_TIME, nullptr);
    auto res = GetSignatureHelpItems(ctx, position, invokedReason, cancellationToken);

    ASSERT_EQ(res.GetItems().size(), 1U);
    EXPECT_EQ(res.GetApplicableSpan().start, 62U);
    EXPECT_EQ(res.GetApplicableSpan().length, 8U);
    EXPECT_EQ(res.GetSelectedItemIndex(), 0U);
    EXPECT_EQ(res.GetArgumentIndex(), 0U);  // signature_help_items.cpp:59 min(callStart, 1-1)
    EXPECT_EQ(res.GetArgumentCount(), 1U);  // signature_help.cpp:354 counts the `p.set` MemberExpression child

    auto &item = res.GetItem(0);
    auto expectedSuffix = std::vector<SymbolDisplayPart> {
        SymbolDisplayPart {")", "punctuation"},
        SymbolDisplayPart {":", "punctuation"},
        SymbolDisplayPart {"undefined", "typeName"},
    };
    EXPECT_EQ(item.GetSuffixDisplayParts(), expectedSuffix);
    ASSERT_EQ(item.GetParameters().size(), 1U);
    EXPECT_EQ(item.GetParameters()[0].GetName(), "v");
    auto expectedParts = std::vector<SymbolDisplayPart> {
        SymbolDisplayPart {"v", "paramName"},
        SymbolDisplayPart {":", "punctuation"},
        SymbolDisplayPart {"Double", "typeName"},
    };
    EXPECT_EQ(item.GetParameters()[0].GetDisplayParts(), expectedParts);
    initializer.DestroyContext(ctx);
}

// PH-004: signature_help.cpp:107-111 — TypeArgsInvocation routes to TypeInfo/TYPEENUM; plus the
// missing-info nullopt guard at signature_help.cpp:87-89. The flow cannot produce TypeArgsInvocation
// (see file-top evidence), so the seam is driven by direct construction.
TEST_F(SignatureHelpShapesTests, GetCandidateOrTypeInfoTypeArgsYieldsTypeInfo)
{
    std::vector<std::string> files = {"shapes_typeargs.ets"};
    std::vector<std::string> texts = {R"(function tf<T, U>(a: T, b: U): number {
    return 0;
}
let r = tf<number, string>(1, "x");
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *ident = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name() == "tf"; });
    ASSERT_NE(ident, nullptr);

    // signature_help.cpp:87-89: missing ArgumentListInfo short-circuits to nullopt.
    EXPECT_EQ(GetCandidateOrTypeInfo(std::nullopt, ast, false), std::nullopt);

    ArgumentListInfo info;
    info.SetInvocation(Invocation(TypeArgsInvocation {InvocationKind::TYPE_ARGS, ident}));
    auto result = GetCandidateOrTypeInfo(info, ast, false);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<ark::es2panda::lsp::TypeInfo>(*result));
    const auto &typeInfo = std::get<ark::es2panda::lsp::TypeInfo>(*result);
    EXPECT_EQ(typeInfo.GetKind(), CandidateOrTypeKind::TYPEENUM);
    EXPECT_EQ(typeInfo.GetSymbol(), ident);
    initializer.DestroyContext(ctx);
}

// PH-004: signature_help.cpp:112-122 — ContextualInvocation with a METHOD_DEFINITION node pushes
// its function signature; a non-method ContextualInvocation node falls through to nullopt (L122).
TEST_F(SignatureHelpShapesTests, GetCandidateOrTypeInfoContextualArms)
{
    std::vector<std::string> files = {"shapes_contextual.ets"};
    std::vector<std::string> texts = {R"(class C {
    foo(a: number): void {
    }
}
let c = new C();
let z = c.foo(1);
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *methodDef = FindNode(ast, [](AstNode *n) { return n->IsMethodDefinition(); });
    ASSERT_NE(methodDef, nullptr);
    auto *fooSig = methodDef->AsMethodDefinition()->Function()->Signature();
    ASSERT_NE(fooSig, nullptr);
    auto *callExpr = FindNode(ast, [](AstNode *n) { return n->IsCallExpression(); });
    ASSERT_NE(callExpr, nullptr);

    // TryGetParameterInfo (signature_help.cpp:295-319) wraps the method definition into a
    // ContextualInvocation argument info; feed it straight into the candidate resolution.
    auto paramInfo = TryGetParameterInfo(methodDef);
    ASSERT_TRUE(paramInfo.has_value());
    EXPECT_EQ(paramInfo->GetInvocation().index(), SHAPES_CONTEXTUAL_INVOCATION_INDEX);
    auto result = GetCandidateOrTypeInfo(paramInfo, ast, false);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<CandidateInfo>(*result));
    auto candidateInfo = std::get<CandidateInfo>(*result);
    EXPECT_EQ(candidateInfo.GetKind(), CandidateOrTypeKind::CANDIDATE);
    ASSERT_EQ(candidateInfo.GetSignatures().size(), 1U);
    EXPECT_EQ(candidateInfo.GetSignatures()[0], fooSig);
    EXPECT_EQ(candidateInfo.GetResolvedSignature(), fooSig);

    // ContextualInvocation whose node is NOT a MethodDefinition: signature_help.cpp:115 evaluates
    // false and control reaches the trailing `return std::nullopt` at signature_help.cpp:122.
    ArgumentListInfo wrong;
    wrong.SetInvocation(Invocation(ContextualInvocation {InvocationKind::CONTEXTUAL, fooSig, callExpr}));
    EXPECT_EQ(GetCandidateOrTypeInfo(wrong, ast, false), std::nullopt);
    initializer.DestroyContext(ctx);
}

// PH-004: signature_help.cpp:90-94 — onlyUseSyntacticOwners filter: parameter-backed
// CallInvocation nodes are rejected, call-expression-backed ones pass through.
TEST_F(SignatureHelpShapesTests, GetCandidateOrTypeInfoSyntacticOwnerFilter)
{
    std::vector<std::string> files = {"shapes_syntactic.ets"};
    std::vector<std::string> texts = {R"(function s(a: number, b: number): number {
    return a + b;
}
let q = s(1, 2);
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *callExpr = FindNode(ast, [](AstNode *n) { return n->IsCallExpression(); });
    ASSERT_NE(callExpr, nullptr);

    std::vector<ArgumentListInfo> list;
    GetArgumentOrParameterListAndIndex(callExpr, list);
    ASSERT_EQ(list.size(), 2U);
    // Each list entry carries a CallInvocation whose node is the argument AST node.
    const auto *callInvocation = std::get_if<CallInvocation>(&list[0].GetInvocation());
    ASSERT_NE(callInvocation, nullptr);
    auto *argNode = callInvocation->callExpressionNode;
    ASSERT_NE(argNode, nullptr);
    ASSERT_TRUE(argNode->IsNumberLiteral());

    // onlyUseSyntacticOwners=true rejects the parameter-backed node (signature_help.cpp:92-94).
    EXPECT_EQ(GetCandidateOrTypeInfo(list[0], ast, true), std::nullopt);

    // The same filter lets a real call-expression node through and still returns candidates.
    ArgumentListInfo callBacked;
    callBacked.SetInvocation(Invocation(CallInvocation {InvocationKind::CALL, callExpr}));
    auto result = GetCandidateOrTypeInfo(callBacked, ast, true);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<CandidateInfo>(*result));
    auto candidateInfo = std::get<CandidateInfo>(*result);
    EXPECT_FALSE(candidateInfo.GetSignatures().empty());
    EXPECT_EQ(candidateInfo.GetResolvedSignature(), callExpr->AsCallExpression()->Signature());
    initializer.DestroyContext(ctx);
}

// PH-005: signature_help.cpp:363-378 — METHOD_DEFINITION-parent arm. A query inside the method
// name produces a ContextualInvocation argument info and a full signature-help result with the
// method's own signature as the only candidate (hence exactly one item).
TEST_F(SignatureHelpShapesTests, SignatureHelpInsideMethodNameTakesMethodDefinitionArm)
{
    std::vector<std::string> files = {"shapes_method_name.ets"};
    // offsets: identifier `foo` spans {14, 3}; the MethodDefinition node spans {14, 28};
    // query position 15 sits inside the name.
    std::vector<std::string> texts = {R"(class C {
    foo(a: number): void {
    }
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *methodDef = FindNode(ast, [](AstNode *n) {
        return n->IsMethodDefinition() && n->AsMethodDefinition()->Id() != nullptr &&
               n->AsMethodDefinition()->Id()->AsIdentifier()->Name() == "foo";
    });
    ASSERT_NE(methodDef, nullptr);
    ASSERT_EQ(methodDef->Start().index, 14U);

    const size_t position = texts[0].find("foo") + 1;
    ASSERT_EQ(position, 15U);
    auto invokedReason = SignatureHelpInvokedReason();
    auto cancellationToken = CancellationToken(SHAPES_DEFAULT_TIME, nullptr);
    auto res = GetSignatureHelpItems(ctx, position, invokedReason, cancellationToken);

    ASSERT_EQ(res.GetItems().size(), 1U);
    // Applicable span is the whole MethodDefinition node (signature_help.cpp:364-375).
    EXPECT_EQ(res.GetApplicableSpan().start, 14U);
    EXPECT_EQ(res.GetApplicableSpan().length, 28U);
    EXPECT_EQ(res.GetSelectedItemIndex(), 0U);
    EXPECT_EQ(res.GetArgumentIndex(), 0U);  // signature_help_items.cpp:59 min(methodStart, ArgCount-1=0)
    EXPECT_EQ(res.GetArgumentCount(), 0U);  // signature_help.cpp:282-293 GetArgumentCount(methodDef, false)

    auto &item = res.GetItem(0);
    auto expectedSuffix = std::vector<SymbolDisplayPart> {
        SymbolDisplayPart {")", "punctuation"},
        SymbolDisplayPart {":", "punctuation"},
        SymbolDisplayPart {"undefined", "typeName"},
    };
    EXPECT_EQ(item.GetSuffixDisplayParts(), expectedSuffix);
    ASSERT_EQ(item.GetParameters().size(), 1U);
    EXPECT_EQ(item.GetParameters()[0].GetName(), "a");
    auto expectedParts = std::vector<SymbolDisplayPart> {
        SymbolDisplayPart {"a", "paramName"},
        SymbolDisplayPart {":", "punctuation"},
        SymbolDisplayPart {"Double", "typeName"},
    };
    EXPECT_EQ(item.GetParameters()[0].GetDisplayParts(), expectedParts);
    initializer.DestroyContext(ctx);
}

// PH-005: signature_help.cpp:336-341 — boundary arcs of GetImmediatelyContainingArgumentInfo:
// position==0, null node, and null parent (Program root) all return nullopt.
TEST_F(SignatureHelpShapesTests, ContainingArgumentInfoBoundaryArcs)
{
    std::vector<std::string> files = {"shapes_boundaries.ets"};
    std::vector<std::string> texts = {R"(class C {
    foo(a: number): void {
    }
}
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *ident = FindNode(ast, [](AstNode *n) { return n->IsIdentifier() && n->AsIdentifier()->Name() == "foo"; });
    ASSERT_NE(ident, nullptr);

    // signature_help.cpp:336 position==0 arc (node valid).
    EXPECT_EQ(GetImmediatelyContainingArgumentInfo(ident, 0), std::nullopt);
    // signature_help.cpp:336 node==nullptr arc.
    EXPECT_EQ(GetImmediatelyContainingArgumentInfo(nullptr, SHAPES_BOUNDARY_POS), std::nullopt);
    // signature_help.cpp:340-341 parent==nullptr arc (Program root has no parent).
    EXPECT_EQ(GetImmediatelyContainingArgumentInfo(ast, SHAPES_BOUNDARY_POS), std::nullopt);
    initializer.DestroyContext(ctx);
}

// PH-006: signature_help.cpp:151-192 entry matrix — explicit type-argument call shape.
// Measured: item[0] renders the DECLARATION signature (unsubstituted <T,U> prefix and a: T/b: U
// parameter types) while items[1..2] render the substituted call signatures (<Double,String>), so
// the resolved call signature's ToString matches at index 1 — the dedup scan (signature_help_items.cpp:33-40)
// skips item[0] through the flow. GetArgumentCount at signature_help.cpp:354 yields 0 because the
// call-site type arguments are TSTypeReference nodes, not TSTypeParameter (the L246-247 increment
// arm only counts type-parameter DECLARATIONS reachable under the call node).
TEST_F(SignatureHelpShapesTests, TemplateCallRendersSubstitutedAndDeclaredSignatures)
{
    std::vector<std::string> files = {"shapes_template.ets"};
    // offsets: tf<number, string>(1, "x") call span = {64, 26}; query position 83 sits inside `1`.
    std::vector<std::string> texts = {R"(function tf<T, U>(a: T, b: U): number {
    return 0;
}
let r = tf<number, string>(1, "x");
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t position = texts[0].rfind("(") + 1;
    ASSERT_EQ(position, 83U);
    auto invokedReason = SignatureHelpInvokedReason();
    auto cancellationToken = CancellationToken(SHAPES_DEFAULT_TIME, nullptr);
    auto res = GetSignatureHelpItems(ctx, position, invokedReason, cancellationToken);

    ASSERT_EQ(res.GetItems().size(), 3U);
    EXPECT_EQ(res.GetApplicableSpan().start, 64U);
    EXPECT_EQ(res.GetApplicableSpan().length, 26U);
    EXPECT_EQ(res.GetSelectedItemIndex(), 1U);
    EXPECT_EQ(res.GetArgumentIndex(), 1U);  // signature_help_items.cpp:59 min(64, 2-1)
    EXPECT_EQ(res.GetArgumentCount(), 0U);

    ExpectDeclaredTemplateItemShape(res.GetItem(0));
    ExpectSubstitutedTemplateItemsShape(res);
    initializer.DestroyContext(ctx);
}

// PH-006: negative shape pin — new-expression call sites produce no items because
// GetArgumentOrParameterListAndIndex (signature_help.cpp:255-281) only lists METHOD_DEFINITION and
// CALL_EXPRESSION nodes, so the NEW_EXPRESSION arm at signature_help.cpp:343-352 always finds an
// empty list and the flow exits with an empty result. Product finding, pinned as current contract.
TEST_F(SignatureHelpShapesTests, NewExpressionCallSiteReturnsEmpty)
{
    std::vector<std::string> files = {"shapes_new_expr.ets"};
    // query position sits inside the constructor argument of `new N(1)`.
    std::vector<std::string> texts = {R"(class N {
    constructor(v: number) {
    }
}
let n = new N(1);
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto invokedReason = SignatureHelpInvokedReason();
    auto cancellationToken = CancellationToken(SHAPES_DEFAULT_TIME, nullptr);
    auto res = GetSignatureHelpItems(ctx, texts[0].rfind("(") + 1, invokedReason, cancellationToken);
    EXPECT_TRUE(res.GetItems().empty());
    EXPECT_EQ(res.GetApplicableSpan().start, 0U);
    EXPECT_EQ(res.GetApplicableSpan().length, 0U);
    initializer.DestroyContext(ctx);
}

// PH-006: nested call shape — the innermost containing call wins at both the inner argument
// position and a position inside the inner callee identifier (whose parent is still the inner
// CallExpression). The call to the top-level `inner` resolves to 3 identical candidates.
TEST_F(SignatureHelpShapesTests, NestedCallResolvesInnermostCall)
{
    std::vector<std::string> files = {"shapes_nested.ets"};
    // offsets: outer(inner(1)); `inner(1)` call span = {118, 8}; identifier `inner` spans {118, 5}.
    std::vector<std::string> texts = {R"(function inner(x: number): number {
    return x;
}
function outer(y: number): number {
    return y;
}
let n = outer(inner(1));
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto invokedReason = SignatureHelpInvokedReason();
    auto cancellationToken = CancellationToken(SHAPES_DEFAULT_TIME, nullptr);

    // Position inside the inner argument `1`.
    auto resInnerArg = GetSignatureHelpItems(ctx, 124, invokedReason, cancellationToken);
    ASSERT_EQ(resInnerArg.GetItems().size(), 3U);
    EXPECT_EQ(resInnerArg.GetApplicableSpan().start, 118U);
    EXPECT_EQ(resInnerArg.GetApplicableSpan().length, 8U);
    EXPECT_EQ(resInnerArg.GetSelectedItemIndex(), 0U);
    EXPECT_EQ(resInnerArg.GetArgumentIndex(), 0U);  // signature_help_items.cpp:59 min(118, 1-1)
    for (size_t i = 0; i < resInnerArg.GetItems().size(); ++i) {
        auto &item = resInnerArg.GetItem(i);
        ASSERT_EQ(item.GetParameters().size(), 1U);
        EXPECT_EQ(item.GetParameters()[0].GetName(), "x");
        auto expectedSuffix = std::vector<SymbolDisplayPart> {
            SymbolDisplayPart {")", "punctuation"},
            SymbolDisplayPart {":", "punctuation"},
            SymbolDisplayPart {"Double", "typeName"},
        };
        EXPECT_EQ(item.GetSuffixDisplayParts(), expectedSuffix);
    }

    // Position inside the inner callee identifier `inner` (outer's argument slot): the touching
    // token's parent is still the inner CallExpression, so the same inner signature is returned.
    auto resInnerCallee = GetSignatureHelpItems(ctx, 119, invokedReason, cancellationToken);
    ASSERT_EQ(resInnerCallee.GetItems().size(), 3U);
    EXPECT_EQ(resInnerCallee.GetApplicableSpan().start, 118U);
    EXPECT_EQ(resInnerCallee.GetApplicableSpan().length, 8U);
    ASSERT_EQ(resInnerCallee.GetItem(0).GetParameters().size(), 1U);
    EXPECT_EQ(resInnerCallee.GetItem(0).GetParameters()[0].GetName(), "x");
    initializer.DestroyContext(ctx);
}

// PH-006: trigger-matrix empty-result pins on one source/context — signature_help.cpp:156-157
// (no touching token at position 0), :169-171 (non-invocation position), :163-166 (characterTyped
// reason short-circuits), and :172-174 (cancelled token).
TEST_F(SignatureHelpShapesTests, TriggerMatrixEmptyResultPins)
{
    // offsets: `let q = s(1);` starts at 48; `s(1)` call span = {56, 4}; position 58 sits inside
    // argument `1`; position 54 sits on the `=` token whose parent is not an invocation.
    std::vector<std::string> files = {"shapes_trigger_matrix.ets"};
    std::vector<std::string> texts = {R"(function s(a: number): number {
    return a;
}
let q = s(1);
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto invokedReason = SignatureHelpInvokedReason();
    auto cancellationToken = CancellationToken(SHAPES_DEFAULT_TIME, nullptr);

    // Position 0 has no token on its left: FindTokenOnLeftOfPosition yields nullptr and the flow
    // exits at signature_help.cpp:156-157.
    auto resAtZero = GetSignatureHelpItems(ctx, 0, invokedReason, cancellationToken);
    EXPECT_TRUE(resAtZero.GetItems().empty());

    // A non-invocation position (`=` of the assignment) leaves GetImmediatelyContainingArgumentInfo
    // without a matching parent type: signature_help.cpp:169-171 empty exit.
    auto resNonCall = GetSignatureHelpItems(ctx, 54, invokedReason, cancellationToken);
    EXPECT_TRUE(resNonCall.GetItems().empty());

    // characterTyped reason sets onlyUseSyntacticOwners and the flow short-circuits at
    // signature_help.cpp:163-166 before any candidate resolution.
    auto typedReason = SignatureHelpCharacterTypedReason();
    auto resTyped = GetSignatureHelpItems(ctx, 58, typedReason, cancellationToken);
    EXPECT_TRUE(resTyped.GetItems().empty());

    // Cancelled token: signature_help.cpp:172-174 exits after the argument info was resolved.
    AlwaysCancelledHost cancelledHost;
    auto cancelledToken = CancellationToken(SHAPES_DEFAULT_TIME, &cancelledHost);
    auto resCancelled = GetSignatureHelpItems(ctx, 58, invokedReason, cancelledToken);
    EXPECT_TRUE(resCancelled.GetItems().empty());
    initializer.DestroyContext(ctx);
}

// PH-006: signature_help.cpp:321-332 — GetArgumentIndexForTemplatePiece arm matrix (direct seam;
// the flow never calls it, matching the branch map). TEMPLATE_LITERAL in-range => 0, out-of-range
// => spanIndex+2, non-template node => spanIndex+1.
TEST_F(SignatureHelpShapesTests, TemplatePieceArgumentIndexArms)
{
    // Template literal with an expression so the checker reaches CHECKED state; a declared
    // variable keeps the AST well-formed (ETS parser maps back-tick to ParseTemplateLiteral).
    std::vector<std::string> files = {"shapes_template_piece.ets"};
    std::vector<std::string> texts = {R"(let name = "world";
let x = `hello ${name} world`;
)"};
    auto filePaths = CreateTempFile(files, texts);
    Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    AstNode *ast = context->parserProgram->Ast();
    ASSERT_NE(ast, nullptr);
    auto *templateLit = FindNode(ast, [](AstNode *n) { return n->IsTemplateLiteral(); });
    ASSERT_NE(templateLit, nullptr) << "Expected a TemplateLiteral node in the CHECKED AST";

    const size_t spanIndex = 5;
    const size_t outOfRangeOffset = 2;
    // signature_help.cpp:325-328: position strictly inside the template span returns 0.
    EXPECT_EQ(GetArgumentIndexForTemplatePiece(spanIndex, templateLit, templateLit->Start().index + 1), 0U);
    // signature_help.cpp:329: position outside the template span returns spanIndex+2.
    EXPECT_EQ(GetArgumentIndexForTemplatePiece(spanIndex, templateLit, templateLit->End().index + 1),
              spanIndex + outOfRangeOffset);

    // signature_help.cpp:331: a non-template node falls through to spanIndex+1.
    auto *stringLit = FindNode(ast, [](AstNode *n) { return n->IsStringLiteral(); });
    ASSERT_NE(stringLit, nullptr);
    const size_t nonTemplateOffset = 1;
    EXPECT_EQ(GetArgumentIndexForTemplatePiece(spanIndex, stringLit, stringLit->Start().index + 1),
              spanIndex + nonTemplateOffset);
    initializer.DestroyContext(ctx);
}

}  // namespace
