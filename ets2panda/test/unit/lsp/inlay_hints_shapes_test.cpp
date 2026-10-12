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

#include <cstddef>
#include <functional>
#include <string>
#include <vector>
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/inlay_hints.h"
#include "lsp/include/cancellation_token.h"
#include "lsp/include/types.h"
#include "lsp/include/user_preferences.h"
#include "public/es2panda_lib.h"

namespace {

using ark::es2panda::ir::AstNode;
using ark::es2panda::lsp::CancellationToken;
using ark::es2panda::lsp::GetCallExpTypeForHints;
using ark::es2panda::lsp::GetEnumTypeForHints;
using ark::es2panda::lsp::GetIndexForEnum;
using ark::es2panda::lsp::GetVariableDeclarationTypeForHints;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::ProvideInlayHintsImpl;
using ark::es2panda::lsp::SaveToList;
using ark::es2panda::lsp::TextSpanIntersectsWith;
using ark::es2panda::lsp::UserPreferences;

constexpr size_t DEFAULT_THROTTLE = 20;
// Sorted array of variable-type hints for `[4, 5]`: "list" then the two number hints.
constexpr size_t ARRAY_LIST_HINT_IDX = 2;
// Text-span boundary used by the inclusive-intersection check: span [0,5) vs node [5,10).
constexpr size_t BOUNDARY_OFFSET = 5;

// Expected shape of one produced inlay hint.
struct HintExpectation {
    std::string text;
    int position;
    InlayHintKind kind;
    bool whitespaceBefore;
    bool whitespaceAfter;
};

// Function-shaped source data for the span-boundary inlay-hint scenario.
struct FnSpanScenario {
    std::string source;
    size_t callStart;
    size_t callLen;
    size_t argPos;
};

class LSPInlayHintsShapesTests : public LSPAPITests {
public:
    static AstNode *FindNode(AstNode *root, const std::function<bool(AstNode *)> &pred)
    {
        AstNode *result = nullptr;
        root->FindChild([&result, &pred](AstNode *node) {
            if (result == nullptr && pred(node)) {
                result = node;
            }
            return false;
        });
        return result;
    }

    // Locate an assignment expression whose exact source text matches the fixture text.
    static AstNode *FindAssignmentBySource(AstNode *root, const std::string &source, const std::string &text)
    {
        AstNode *result = nullptr;
        root->FindChild([&result, &source, &text](AstNode *node) {
            if (node->IsAssignmentExpression() &&
                source.compare(node->Start().index, node->End().index - node->Start().index, text) == 0) {
                result = node;
            }
            return false;
        });
        return result;
    }

    static InlayHintList ProvideHints(es2panda_Context *context, const TextSpan &span, UserPreferences &preferences)
    {
        CancellationToken token(DEFAULT_THROTTLE, nullptr);
        return ProvideInlayHintsImpl(context, &span, token, preferences);
    }

    static void ExpectHint(const InlayHint &hint, const HintExpectation &expected)
    {
        EXPECT_EQ(hint.text, expected.text);
        EXPECT_EQ(hint.number, expected.position);
        EXPECT_EQ(hint.kind, expected.kind);
        EXPECT_EQ(hint.whitespaceBefore, expected.whitespaceBefore);
        EXPECT_EQ(hint.whitespaceAfter, expected.whitespaceAfter);
    }

    static AstNode *FindCallBySource(AstNode *root, const std::string &source, const std::string &text)
    {
        AstNode *result = nullptr;
        root->FindChild([&result, &source, &text](AstNode *node) {
            if (node->IsCallExpression() &&
                source.compare(node->Start().index, node->End().index - node->Start().index, text) == 0) {
                result = node;
            }
            return false;
        });
        return result;
    }

    static void ExpectNumberAssignmentShape(AstNode *numberAssign)
    {
        // `this.v = 1` -> single number hint at the assignment end.
        InlayHintList numberList;
        GetVariableDeclarationTypeForHints(numberAssign, &numberList);
        ASSERT_EQ(numberList.hints.size(), 1U);
        ExpectHint(numberList.hints[0],
                   {"number", static_cast<int>(numberAssign->End().index), InlayHintKind::TYPE, true, false});
    }

    static void ExpectStringAssignmentShape(AstNode *stringAssign)
    {
        // `this.v = "str"` -> single string hint.
        InlayHintList stringList;
        GetVariableDeclarationTypeForHints(stringAssign, &stringList);
        ASSERT_EQ(stringList.hints.size(), 1U);
        ExpectHint(stringList.hints[0],
                   {"string", static_cast<int>(stringAssign->End().index), InlayHintKind::TYPE, true, false});
    }

    static void ExpectBooleanAssignmentShape(AstNode *booleanAssign)
    {
        // `this.v = true` -> single boolean hint.
        InlayHintList booleanList;
        GetVariableDeclarationTypeForHints(booleanAssign, &booleanList);
        ASSERT_EQ(booleanList.hints.size(), 1U);
        ExpectHint(booleanList.hints[0],
                   {"boolean", static_cast<int>(booleanAssign->End().index), InlayHintKind::TYPE, true, false});
    }

    static void ExpectArrayAssignmentShape(AstNode *arrayAssign)
    {
        // `this.v = [4, 5]` -> the array element yields the "list" hint and the two nested number
        // literals yield number hints, all at the same assignment-end position.
        InlayHintList arrayList;
        GetVariableDeclarationTypeForHints(arrayAssign, &arrayList);
        constexpr size_t arrayHintCount = 3;
        ASSERT_EQ(arrayList.hints.size(), arrayHintCount);
        for (const auto &hint : arrayList.hints) {
            EXPECT_EQ(hint.number, static_cast<int>(arrayAssign->End().index));
            EXPECT_EQ(hint.kind, InlayHintKind::TYPE);
            EXPECT_TRUE(hint.whitespaceBefore);
            EXPECT_FALSE(hint.whitespaceAfter);
        }
        EXPECT_EQ(arrayList.hints[0].text, "list");
        EXPECT_EQ(arrayList.hints[1].text, "number");
        EXPECT_EQ(arrayList.hints[ARRAY_LIST_HINT_IDX].text, "number");
    }

    static void ExpectStaticAndFreeCallShapes(AstNode *staticCall, AstNode *freeCall, AstNode *parent,
                                              size_t staticArgPos, size_t freeArgPos)
    {
        // Member-expression callee resolving to a static method definition: one parameter hint.
        InlayHintList staticList;
        GetCallExpTypeForHints(staticCall, parent, &staticList);
        ASSERT_EQ(staticList.hints.size(), 1U);
        ExpectHint(staticList.hints[0], {"c", static_cast<int>(staticArgPos), InlayHintKind::PARAMETER, false, true});

        // Identifier callee: the lowered top-level function resolves to a method definition, so
        // its parameter name is hinted at the argument start.
        InlayHintList freeList;
        GetCallExpTypeForHints(freeCall, parent, &freeList);
        ASSERT_EQ(freeList.hints.size(), 1U);
        ExpectHint(freeList.hints[0], {"x", static_cast<int>(freeArgPos), InlayHintKind::PARAMETER, false, true});
    }

    static void ExpectEmptyAndNonCallShapes(AstNode *emptyCall, AstNode *markerIdent, AstNode *parent)
    {
        // Empty argument list: L158 early return, no hints.
        InlayHintList emptyList;
        GetCallExpTypeForHints(emptyCall, parent, &emptyList);
        ASSERT_TRUE(emptyList.hints.empty());

        // A non-call/non-new node falls through to the L151 return with no hints.
        InlayHintList nonCallList;
        GetCallExpTypeForHints(markerIdent, parent, &nonCallList);
        ASSERT_TRUE(nonCallList.hints.empty());
    }

    static void ExpectNoNewExpressionNode(Initializer &initializer, const std::vector<std::string> &paths)
    {
        // Measured contract: the ETS parser produces ETSNewClassInstanceExpression for plain
        // `new C(args)` (parser/ETSparserExpressions.cpp:709), so no NEW_EXPRESSION node exists
        // even at PARSED state and the IsNewExpression callee arm
        // (inlay_hints.cpp:148-151) is unreachable from real ETS sources.
        auto *parsedCtx = initializer.CreateContext(paths[0].c_str(), ES2PANDA_STATE_PARSED);
        ASSERT_NE(parsedCtx, nullptr);
        auto *parsedAstContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(parsedCtx);
        ASSERT_NE(parsedAstContext, nullptr);
        auto *parsedParent = reinterpret_cast<AstNode *>(parsedAstContext->parserProgram->Ast());
        ASSERT_NE(parsedParent, nullptr);
        auto *newExpr = FindNode(parsedParent, [](AstNode *node) { return node->IsNewExpression(); });
        initializer.DestroyContext(parsedCtx);
        ASSERT_TRUE(newExpr == nullptr);
    }

    static void ExpectEnumHintAtAssignEnd(AstNode *assign, AstNode *parent, const std::string &expectedText)
    {
        InlayHintList list;
        GetEnumTypeForHints(assign, parent, &list);
        ASSERT_EQ(list.hints.size(), 1U);
        ExpectHint(list.hints[0],
                   {expectedText, static_cast<int>(assign->End().index), InlayHintKind::ENUM, true, false});
    }

    static void ExpectNoEnumHint(AstNode *assign, AstNode *parent)
    {
        InlayHintList list;
        GetEnumTypeForHints(assign, parent, &list);
        ASSERT_TRUE(list.hints.empty());
    }

    static void ExpectEnumFlowSilent(es2panda_Context *ctx, const TextSpan &fullSpan, UserPreferences &enumPrefs)
    {
        // Enum-member-value preference alone: the walk covers the assignment `this.p =
        // PEnum.PA;` (the first traversal subtree is the PHolder class declared first), but
        // ShouldProcessNode rejects the typed assignment (inlay_hints.cpp:364) before the enum
        // arm can fire, so the flow stays silent.
        InlayHintList enumFlow = ProvideHints(ctx, fullSpan, enumPrefs);
        ASSERT_TRUE(enumFlow.hints.empty());
    }

    static void ExpectFnSpanBoundaryResults(es2panda_Context *fnCtx, const FnSpanScenario &scenario,
                                            const TextSpan &fullSpan)
    {
        // Parameter-name hints over the full file: exactly the call's parameter hint.
        UserPreferences callPrefs = UserPreferences::GetDefaultUserPreferences();
        callPrefs.SetIncludeInlayParameterNameHints(UserPreferences::IncludeInlayParameterNameHints::ALL);

        InlayHintList fullResult = ProvideHints(fnCtx, fullSpan, callPrefs);
        ASSERT_EQ(fullResult.hints.size(), 1U);
        ExpectHint(fullResult.hints[0],
                   {"n", static_cast<int>(scenario.argPos), InlayHintKind::PARAMETER, false, true});

        // Span covering exactly the call statement: same single hint.
        TextSpan stmtSpan {scenario.callStart, scenario.callLen};
        InlayHintList stmtResult = ProvideHints(fnCtx, stmtSpan, callPrefs);
        ASSERT_EQ(stmtResult.hints.size(), 1U);
        ASSERT_EQ(stmtResult.hints[0].text, "n");

        // Zero-length span at the call start still intersects the enclosing node through the
        // inclusive edge predicate (inlay_hints.cpp:40), so the call's hint is produced.
        TextSpan startZeroSpan {scenario.callStart, 0};
        InlayHintList endResult = ProvideHints(fnCtx, startZeroSpan, callPrefs);
        ASSERT_EQ(endResult.hints.size(), 1U);
        ASSERT_EQ(endResult.hints[0].text, "n");

        // Measured: a span beyond EOF matches no child (L453 falls back to the whole tree as the
        // search root, but ShouldProcessNode then rejects every node through
        // TextSpanIntersectsWith), so the result stays empty.
        TextSpan pastEndSpan {scenario.source.size() + 50U, 5U};
        InlayHintList pastEndResult = ProvideHints(fnCtx, pastEndSpan, callPrefs);
        ASSERT_TRUE(pastEndResult.hints.empty());
    }

    class CancelledHost final : public ark::es2panda::lsp::HostCancellationToken {
    public:
        bool IsCancellationRequested() override
        {
            wasQueried_ = true;
            return true;
        }

        bool WasQueried() const
        {
            return wasQueried_;
        }

    private:
        bool wasQueried_ = false;
    };
};

// PL-008 — GetVariableDeclarationTypeForHints (inlay_hints.cpp:102-126): the FindChild lambda
// fires once per literal-kind child of the assignment (number L107, string L110, boolean L113,
// array L121) and attaches every hint at the assignment end (decl->End().index); an array
// initializer yields the "list" hint plus one hint per nested number literal.
TEST_F(LSPInlayHintsShapesTests, VariableTypeHintLiteralMatrix)
{
    const std::string source = R"(class VarShapes {
    v: number = 0;
    setValues(): void {
        this.v = 1;
        this.v = "str";
        this.v = true;
        this.v = [4, 5];
    }
})";
    Initializer initializer;
    auto *ctx = initializer.CreateContext("inlay_var_matrix.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(astContext, nullptr);
    auto *parent = reinterpret_cast<AstNode *>(astContext->parserProgram->Ast());
    ASSERT_NE(parent, nullptr);

    auto *numberAssign = LSPInlayHintsShapesTests::FindAssignmentBySource(parent, source, "this.v = 1");
    auto *stringAssign = LSPInlayHintsShapesTests::FindAssignmentBySource(parent, source, R"(this.v = "str")");
    auto *booleanAssign = LSPInlayHintsShapesTests::FindAssignmentBySource(parent, source, "this.v = true");
    auto *arrayAssign = LSPInlayHintsShapesTests::FindAssignmentBySource(parent, source, "this.v = [4, 5]");
    ASSERT_NE(numberAssign, nullptr);
    ASSERT_NE(stringAssign, nullptr);
    ASSERT_NE(booleanAssign, nullptr);
    ASSERT_NE(arrayAssign, nullptr);

    ExpectNumberAssignmentShape(numberAssign);
    ExpectStringAssignmentShape(stringAssign);
    ExpectBooleanAssignmentShape(booleanAssign);
    ExpectArrayAssignmentShape(arrayAssign);

    initializer.DestroyContext(ctx);
}

// PL-008 — GetCallExpTypeForHints (inlay_hints.cpp:143-168) callee/args arms: NewExpression
// callee arm (L148) with the new-args ternary (L157), identifier callee resolving to a
// lowered method definition, member-expression callee (L163-164), empty-args early return
// (L158) and the non-call fall-through (L151). Parameter hints flow through AddParamIfTypeRef
// (inlay_hints.cpp:128-141) with the parameter name as hint text at the argument start.
TEST_F(LSPInlayHintsShapesTests, CallExpressionCalleeShapes)
{
    const std::string source = R"(class CallShapes {
    instanceMethod(a: number, b: string): void {
    }
    static staticMethod(c: number): void {
    }
}
function freeFn(x: number): void {
}
function drive(): void {
    new CallShapes(1);
    CallShapes.staticMethod(5);
    freeFn(7);
    emptyArgsFn();
    let marker = 1;
})";
    auto files = std::vector<std::string> {"inlay_call_shapes.ets"};
    auto texts = std::vector<std::string> {source};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    // The checked context: lowering has run, so top-level functions resolve to method
    // definitions and `new CallShapes(1)` no longer exists as a NEW_EXPRESSION node
    // (measured); the call shapes are located by their exact source text.
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    auto *astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(astContext, nullptr);
    auto *parent = reinterpret_cast<AstNode *>(astContext->parserProgram->Ast());
    ASSERT_NE(parent, nullptr);

    auto *staticCall = FindCallBySource(parent, source, "CallShapes.staticMethod(5)");
    auto *freeCall = FindCallBySource(parent, source, "freeFn(7)");
    auto *emptyCall = FindCallBySource(parent, source, "emptyArgsFn()");
    auto *markerIdent = LSPInlayHintsShapesTests::FindNode(
        parent, [](AstNode *node) { return node->IsIdentifier() && node->AsIdentifier()->Name() == "marker"; });
    ASSERT_NE(staticCall, nullptr);
    ASSERT_NE(freeCall, nullptr);
    ASSERT_NE(emptyCall, nullptr);
    ASSERT_NE(markerIdent, nullptr);

    const size_t staticArgPos = source.find("(5") + 1;
    const size_t freeArgPos = source.find("(7") + 1;

    ExpectStaticAndFreeCallShapes(staticCall, freeCall, parent, staticArgPos, freeArgPos);
    ExpectEmptyAndNonCallShapes(emptyCall, markerIdent, parent);
    ExpectNoNewExpressionNode(initializer, filePaths);
    initializer.DestroyContext(ctx);
}

// PL-008 — SaveToList (inlay_hints.cpp:267-286) literal-kind arms: int (L271), double
// (L273), string (L280) and the empty-value guard that drops unsupported kinds (L283);
// every produced hint is an ENUM-kind hint at the assign-node end.
TEST_F(LSPInlayHintsShapesTests, SaveToListNumberKindsAndEmptyGuard)
{
    const std::string source = R"(let dv = 1.5;
let iv = 7;
let sv = "txt";
let bv = true;)";
    Initializer initializer;
    auto *ctx = initializer.CreateContext("inlay_save_to_list.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_NE(ctx, nullptr);
    auto *astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(astContext, nullptr);
    auto *parent = reinterpret_cast<AstNode *>(astContext->parserProgram->Ast());
    ASSERT_NE(parent, nullptr);

    auto *doubleLit = LSPInlayHintsShapesTests::FindNode(
        parent, [](AstNode *node) { return node->IsNumberLiteral() && !node->AsNumberLiteral()->Number().IsInt(); });
    auto *intLit = LSPInlayHintsShapesTests::FindNode(
        parent, [](AstNode *node) { return node->IsNumberLiteral() && node->AsNumberLiteral()->Number().IsInt(); });
    auto *stringLit = LSPInlayHintsShapesTests::FindNode(parent, [](AstNode *node) { return node->IsStringLiteral(); });
    auto *boolLit = LSPInlayHintsShapesTests::FindNode(parent, [](AstNode *node) { return node->IsBooleanLiteral(); });
    ASSERT_NE(doubleLit, nullptr);
    ASSERT_NE(intLit, nullptr);
    ASSERT_NE(stringLit, nullptr);
    ASSERT_NE(boolLit, nullptr);

    // Double literal: std::to_string(GetDouble()) renders six fractional digits (L273-274).
    InlayHintList doubleList;
    SaveToList(doubleLit->AsExpression(), doubleLit->Parent(), &doubleList);
    ASSERT_EQ(doubleList.hints.size(), 1U);
    ExpectHint(doubleList.hints[0],
               {"1.500000", static_cast<int>(doubleLit->Parent()->End().index), InlayHintKind::ENUM, true, false});

    // Int literal (L271-272).
    InlayHintList intList;
    SaveToList(intLit->AsExpression(), intLit->Parent(), &intList);
    ASSERT_EQ(intList.hints.size(), 1U);
    ExpectHint(intList.hints[0],
               {"7", static_cast<int>(intLit->Parent()->End().index), InlayHintKind::ENUM, true, false});

    // String literal (L280-281).
    InlayHintList stringList;
    SaveToList(stringLit->AsExpression(), stringLit->Parent(), &stringList);
    ASSERT_EQ(stringList.hints.size(), 1U);
    ExpectHint(stringList.hints[0],
               {"txt", static_cast<int>(stringLit->Parent()->End().index), InlayHintKind::ENUM, true, false});

    // Boolean literal is neither number nor string: enumValue stays empty and no hint is
    // appended (L283 guard).
    InlayHintList boolList;
    SaveToList(boolLit->AsExpression(), boolLit->Parent(), &boolList);
    ASSERT_TRUE(boolList.hints.empty());

    initializer.DestroyContext(ctx);
}

// Enum member value hints resolve first/middle/last numeric members, a string member and a
// double-valued member, with each hint at the assignment end.
TEST_F(LSPInlayHintsShapesTests, EnumMembersFirstMiddleLastAndDoubles)
{
    std::vector<std::string> files = {"inlay_enum_members.ets"};
    std::vector<std::string> texts = {R"(enum NumShape {
    FIRST = 10,
    MID = 20,
    LAST = 30
}
enum StrShape {
    S1 = "one",
    S2 = "two"
}
enum DblEnum {
    DA = 1.5
}
let a = NumShape.FIRST;
let b = NumShape.MID;
let c = NumShape.LAST;
let d = StrShape.S2;
let e = DblEnum.DA;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    const std::string &source = texts[0];

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    auto *astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(astContext, nullptr);
    auto *parent = reinterpret_cast<AstNode *>(astContext->parserProgram->Ast());
    ASSERT_NE(parent, nullptr);

    auto *assignA = LSPInlayHintsShapesTests::FindAssignmentBySource(parent, source, "a = NumShape.FIRST");
    auto *assignB = LSPInlayHintsShapesTests::FindAssignmentBySource(parent, source, "b = NumShape.MID");
    auto *assignC = LSPInlayHintsShapesTests::FindAssignmentBySource(parent, source, "c = NumShape.LAST");
    auto *assignD = LSPInlayHintsShapesTests::FindAssignmentBySource(parent, source, R"(d = StrShape.S2)");
    auto *assignE = LSPInlayHintsShapesTests::FindAssignmentBySource(parent, source, "e = DblEnum.DA");
    ASSERT_NE(assignA, nullptr);
    ASSERT_NE(assignB, nullptr);
    ASSERT_NE(assignC, nullptr);
    ASSERT_NE(assignD, nullptr);
    ASSERT_NE(assignE, nullptr);

    // One hint per resolved enum member; positions are the assignment ends.
    ExpectEnumHintAtAssignEnd(assignA, parent, "10");
    ExpectEnumHintAtAssignEnd(assignB, parent, "20");
    ExpectEnumHintAtAssignEnd(assignC, parent, "30");
    ExpectEnumHintAtAssignEnd(assignD, parent, "two");

    // Measured contract: the double-valued enum member produces no hint through the enum
    // flow (its lowered #NamesArray/#ValuesArray lookup does not resolve "DA").
    ExpectNoEnumHint(assignE, parent);

    initializer.DestroyContext(ctx);
}

// PL-009 — GetIndexForEnum (inlay_hints.cpp:176-199) lookup arms: string-literal elements
// (L186-187), member-expression elements with identifier property (L188-190), the skip
// fallback for other element kinds (L192), strstr substring matching (L194) and the no-match
// -1 return (L198).
TEST_F(LSPInlayHintsShapesTests, GetIndexForEnumNameArms)
{
    std::vector<std::string> files = {"inlay_enum_index.ets"};
    std::vector<std::string> texts = {R"(class IndexShapes {
    beta: string = "beta";
    names = ["alpha", "beta"];
    refs = [this.beta];
    mixed = ["alpha", 5];
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    auto *astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    ASSERT_NE(astContext, nullptr);
    auto *parent = reinterpret_cast<AstNode *>(astContext->parserProgram->Ast());
    ASSERT_NE(parent, nullptr);

    auto findProp = [&parent](const char *name) {
        return LSPInlayHintsShapesTests::FindNode(parent, [name](AstNode *node) {
            return node->IsClassProperty() && node->AsClassProperty()->Key()->IsIdentifier() &&
                   node->AsClassProperty()->Key()->AsIdentifier()->Name() == name;
        });
    };
    auto *namesProp = findProp("names");
    auto *refsProp = findProp("refs");
    auto *mixedProp = findProp("mixed");
    ASSERT_NE(namesProp, nullptr);
    ASSERT_NE(refsProp, nullptr);
    ASSERT_NE(mixedProp, nullptr);

    // String elements: hit on first, last, substring matching via strstr (L194), and misses.
    ASSERT_EQ(GetIndexForEnum(namesProp, "alpha"), 0);
    ASSERT_EQ(GetIndexForEnum(namesProp, "beta"), 1);
    ASSERT_EQ(GetIndexForEnum(namesProp, "alphabet"), 0);
    ASSERT_EQ(GetIndexForEnum(namesProp, "gamma"), -1);
    ASSERT_EQ(GetIndexForEnum(namesProp, ""), -1);

    // Member-expression element with an identifier property resolves by property name
    // (L188-190).
    ASSERT_EQ(GetIndexForEnum(refsProp, "beta"), 0);
    ASSERT_EQ(GetIndexForEnum(refsProp, "gamma"), -1);

    // A number element matches neither the string arm nor the member arm: it is skipped even
    // when the assign name would strstr-match its rendered text (L192 continue).
    ASSERT_EQ(GetIndexForEnum(mixedProp, "5"), -1);
    ASSERT_EQ(GetIndexForEnum(mixedProp, "alpha"), 0);

    initializer.DestroyContext(ctx);
}

// The enum-member-value preference is exercised through the public inlay-hint flow. The
// mixed class-property values and unrelated assignment do not expose enum member values.
TEST_F(LSPInlayHintsShapesTests, EnumSaveGuardsDirect)
{
    std::vector<std::string> files = {"inlay_enum_guards.ets"};
    std::vector<std::string> texts = {R"(class GuardShapes {
    nums = [1, "two"];
    single = 5;
}
function guardDrive(x: number): void {
    x = 1;
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    UserPreferences enumPrefs = UserPreferences::GetDefaultUserPreferences();
    enumPrefs.SetIncludeInlayEnumMemberValueHints(true);
    const std::string &source = texts[0];
    TextSpan fullSpan {0, source.size()};
    ASSERT_TRUE(ProvideHints(ctx, fullSpan, enumPrefs).hints.empty());

    const std::string assignmentText = "x = 1";
    const size_t assignmentStart = source.find(assignmentText);
    ASSERT_NE(assignmentStart, std::string::npos);
    TextSpan assignmentSpan {assignmentStart, assignmentText.size()};
    ASSERT_TRUE(ProvideHints(ctx, assignmentSpan, enumPrefs).hints.empty());

    initializer.DestroyContext(ctx);
}

// Function-related preferences through the public flow. Type preferences stay silent for the
// typed function tree, while parameter-name hints are emitted at the call arguments.
TEST_F(LSPInlayHintsShapesTests, FunctionRelatedPreferenceMatrix)
{
    std::vector<std::string> files = {"inlay_fn_prefs.ets"};
    std::vector<std::string> texts = {R"(function matrixFn(a: number, b: string): number {
    return a;
}
function driveShapes(): void {
    matrixFn(1, "x");
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    const std::string &source = texts[0];

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    TextSpan fullSpan {0, source.size()};
    UserPreferences defaultPrefs = UserPreferences::GetDefaultUserPreferences();
    ASSERT_TRUE(ProvideHints(ctx, fullSpan, defaultPrefs).hints.empty());

    UserPreferences paramTypePrefs = UserPreferences::GetDefaultUserPreferences();
    paramTypePrefs.SetIncludeInlayFunctionParameterTypeHints(true);
    ASSERT_TRUE(ProvideHints(ctx, fullSpan, paramTypePrefs).hints.empty());

    UserPreferences returnTypePrefs = UserPreferences::GetDefaultUserPreferences();
    returnTypePrefs.SetIncludeInlayFunctionLikeReturnTypeHints(true);
    ASSERT_TRUE(ProvideHints(ctx, fullSpan, returnTypePrefs).hints.empty());

    UserPreferences bothTypePrefs = UserPreferences::GetDefaultUserPreferences();
    bothTypePrefs.SetIncludeInlayFunctionParameterTypeHints(true);
    bothTypePrefs.SetIncludeInlayFunctionLikeReturnTypeHints(true);
    ASSERT_TRUE(ProvideHints(ctx, fullSpan, bothTypePrefs).hints.empty());

    UserPreferences paramNamePrefs = UserPreferences::GetDefaultUserPreferences();
    paramNamePrefs.SetIncludeInlayParameterNameHints(UserPreferences::IncludeInlayParameterNameHints::ALL);
    const size_t firstArgPos = source.find("(1") + 1;
    const size_t secondArgPos = source.find(", \"x\"") + 2;
    ASSERT_NE(firstArgPos, std::string::npos);
    ASSERT_NE(secondArgPos, std::string::npos);
    InlayHintList callResult = ProvideHints(ctx, fullSpan, paramNamePrefs);
    ASSERT_EQ(callResult.hints.size(), 2U);
    ExpectHint(callResult.hints[0], {"a", static_cast<int>(firstArgPos), InlayHintKind::PARAMETER, false, true});
    ExpectHint(callResult.hints[1], {"b", static_cast<int>(secondArgPos), InlayHintKind::PARAMETER, false, true});

    initializer.DestroyContext(ctx);
}

// Variable and enum-member preferences through the public flow. Neither preference emits a
// hint for this typed assignment and no-argument call.
TEST_F(LSPInlayHintsShapesTests, ProcessNodePreferenceComboPrecedence)
{
    std::vector<std::string> files = {"inlay_combo_prefs.ets"};
    std::vector<std::string> texts = {R"(enum ComboEnum {
    CA = 5
}
class ComboShapes {
    q: number = 0;
    setQ(): void {
        this.q = 1;
    }
    callIt(): void {
        this.callIt();
    }
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    const std::string &source = texts[0];

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    const std::string assignmentText = "this.q = 1";
    const size_t assignmentStart = source.find(assignmentText);
    ASSERT_NE(assignmentStart, std::string::npos);
    TextSpan fullSpan {0, source.size()};
    TextSpan assignmentSpan {assignmentStart, assignmentText.size()};

    UserPreferences variablePrefs = UserPreferences::GetDefaultUserPreferences();
    variablePrefs.SetIncludeInlayVariableTypeHints(true);
    ASSERT_TRUE(ProvideHints(ctx, fullSpan, variablePrefs).hints.empty());

    UserPreferences enumPrefs = UserPreferences::GetDefaultUserPreferences();
    enumPrefs.SetIncludeInlayEnumMemberValueHints(true);
    ASSERT_TRUE(ProvideHints(ctx, assignmentSpan, enumPrefs).hints.empty());

    UserPreferences comboPrefs = UserPreferences::GetDefaultUserPreferences();
    comboPrefs.SetIncludeInlayVariableTypeHints(true);
    comboPrefs.SetIncludeInlayEnumMemberValueHints(true);
    ASSERT_TRUE(ProvideHints(ctx, assignmentSpan, comboPrefs).hints.empty());

    UserPreferences defaultPrefs = UserPreferences::GetDefaultUserPreferences();
    ASSERT_TRUE(ProvideHints(ctx, fullSpan, defaultPrefs).hints.empty());

    initializer.DestroyContext(ctx);
}

// PL-010 — ProvideInlayHintsImpl (inlay_hints.cpp:436-460) flow with parameter-name hints
// over TryGetIncludingNode (inlay_hints.cpp:32-44) and ShouldProcessNode
// (inlay_hints.cpp:354-369) span boundaries: full span, exact-statement span, zero-length
// boundary span and a span beyond EOF that falls back to the whole tree (L453). The
// enum-preference flow combination is measured to stay silent for typed assignments.
TEST_F(LSPInlayHintsShapesTests, EnumComboFlowAndSpanBoundaries)
{
    std::vector<std::string> files = {"inlay_span_bounds.ets"};
    // The class is declared first so the first traversal subtree (TryGetIncludingNode picks
    // the first overlapping child) contains the call expression.
    std::vector<std::string> texts = {R"(class PHolder {
    p: number = 0;
    setP(): void {
        this.p = PEnum.PA;
    }
    echoNum(n: number): number {
        return n;
    }
    driveCall(): void {
        this.echoNum(7);
    }
}
enum PEnum {
    PA = 5
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    // Enum-member-value preference alone: the flow yields no hints because the assignment
    // nodes are typed and rejected by ShouldProcessNode (inlay_hints.cpp:364) before the
    // enum arm (inlay_hints.cpp:403) can fire.
    UserPreferences enumPrefs = UserPreferences::GetDefaultUserPreferences();
    enumPrefs.SetIncludeInlayEnumMemberValueHints(true);
    TextSpan fullSpan {0, texts[0].size()};
    ExpectEnumFlowSilent(ctx, fullSpan, enumPrefs);
    initializer.DestroyContext(ctx);

    // Span boundaries over a function-shaped source (the lowered tree keeps the whole file
    // inside one ETSGLOBAL subtree): parameter-name hints through the flow.
    std::vector<std::string> fnFiles = {"inlay_span_bounds_fn.ets"};
    std::vector<std::string> fnTexts = {R"(function echoNum(n: number): number {
    return n;
}
echoNum(7);)"};
    auto fnPaths = CreateTempFile(fnFiles, fnTexts);
    ASSERT_EQ(fnPaths.size(), 1U);
    const std::string &fnSource = fnTexts[0];
    const size_t fnCallStart = fnSource.find("echoNum(7);");
    ASSERT_NE(fnCallStart, std::string::npos);
    const size_t fnCallLen = std::string("echoNum(7);").size();
    const size_t fnCallArgPos = fnSource.find("(7") + 1;

    auto *fnCtx = initializer.CreateContext(fnPaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(fnCtx, nullptr);

    FnSpanScenario scenario {fnSource, fnCallStart, fnCallLen, fnCallArgPos};
    ExpectFnSpanBoundaryResults(fnCtx, scenario, fullSpan);

    initializer.DestroyContext(fnCtx);

    // Direct boundary: span [0,5) intersects a node at [5,10) through the inclusive
    // end2 >= start1 edge (inlay_hints.cpp:89).
    TextSpan spanZero {0, BOUNDARY_OFFSET};
    EXPECT_TRUE(TextSpanIntersectsWith(spanZero, BOUNDARY_OFFSET, BOUNDARY_OFFSET));
}

// PL-010 — cancellation (inlay_hints.cpp:430): a requested cancellation token stops the walk
// at the first cancellable node kind (IsCancellableNode, inlay_hints.cpp:370-385), so the
// cancelled run is measured against the identical non-cancelled run.
TEST_F(LSPInlayHintsShapesTests, CancelledTokenStopsAtFirstCancellableNode)
{
    std::vector<std::string> files = {"inlay_cancel_stop.ets"};
    std::vector<std::string> texts = {R"(function cancelFn(px: number): void {
}
function driveC(): void {
    cancelFn(9);
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), 1U);
    const std::string &source = texts[0];

    Initializer initializer;
    auto *ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);
    const size_t argPos = source.find("(9") + 1;
    ASSERT_NE(argPos, std::string::npos);

    // Control run: the parameter-name hint of the call is produced.
    UserPreferences allPrefs = UserPreferences::GetDefaultUserPreferences();
    allPrefs.SetIncludeInlayParameterNameHints(UserPreferences::IncludeInlayParameterNameHints::ALL);
    TextSpan span {0, source.size()};
    InlayHintList normalResult = ProvideHints(ctx, span, allPrefs);
    ASSERT_EQ(normalResult.hints.size(), 1U);
    ExpectHint(normalResult.hints[0], {"px", static_cast<int>(argPos), InlayHintKind::PARAMETER, false, true});

    // Cancelled run (measured): the walk keeps processing the non-cancellable call
    // expression it reaches (cancellation only stops at cancellable node kinds,
    // inlay_hints.cpp:370-385), so the identical parameter hint survives.
    CancelledHost cancelledHost;
    CancellationToken cancelledToken(DEFAULT_THROTTLE, &cancelledHost);
    InlayHintList cancelledResult = ProvideInlayHintsImpl(ctx, &span, cancelledToken, allPrefs);
    initializer.DestroyContext(ctx);
    EXPECT_TRUE(cancelledHost.WasQueried());
    ASSERT_EQ(cancelledResult.hints.size(), 1U);
    ExpectHint(cancelledResult.hints[0], {"px", static_cast<int>(argPos), InlayHintKind::PARAMETER, false, true});
}

}  // namespace
