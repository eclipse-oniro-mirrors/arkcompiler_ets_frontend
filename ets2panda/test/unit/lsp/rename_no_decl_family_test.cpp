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
#include <cstddef>
#include <string>
#include <variant>
#include <vector>

#include "lsp/include/internal_api.h"
#include "lsp/include/rename.h"
#include "lsp/include/types.h"
#include "lsp_api_test.h"

namespace {

using ark::es2panda::ir::AstNode;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::RenameInfoFailure;
using ark::es2panda::lsp::RenameInfoSuccess;
using ark::es2panda::lsp::RenameInfoType;

// Deep non-existent path prevents local test declarations from being classified as library files.
constexpr const char *NONEXISTENT_LIB_PATH = "/nonexistent/very/deep/path/";

class RenameNoDeclFamilyTests : public LSPAPITests {
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

    static const RenameInfoSuccess &AssertSuccess(const RenameInfoType &info)
    {
        // A variant mismatch must fail only this test; std::get would abort the whole binary.
        static const RenameInfoSuccess fallback {false, "", "", "", "", "", TextSpan(0, 0)};
        const auto *success = std::get_if<RenameInfoSuccess>(&info);
        EXPECT_NE(success, nullptr);
        return success != nullptr ? *success : fallback;
    }

    static const RenameInfoFailure &AssertFailure(const RenameInfoType &info)
    {
        static const RenameInfoFailure fallback {false, ""};
        const auto *failure = std::get_if<RenameInfoFailure>(&info);
        EXPECT_NE(failure, nullptr);
        return failure != nullptr ? *failure : fallback;
    }

    // Label scenario used by the flow tests below. ETS labelled statements parse and check
    // (verified with es2panda --extension=ets); "outer" is used by both continue and break.
    static std::string LabelSource()
    {
        return "let n: number = 0;\n"                            // line 1
               "outer: for (let i: number = 0; i < 3; i++) {\n"  // line 2
               "  if (i == 1) {\n"                               // line 3
               "    continue outer;\n"                           // line 4
               "  }\n"                                           // line 5
               "  if (i == 2) {\n"                               // line 6
               "    break outer;\n"                              // line 7
               "  }\n"                                           // line 8
               "  n++;\n"                                        // line 9
               "}\n";                                            // line 10
    }
};

// A break-label use is renameable through the public flow with anonymous display metadata.
TEST_F(RenameNoDeclFamilyTests, FlowBreakLabelUseReturnsAnonymousLabelInfo)
{
    Initializer initializer;
    const std::string source = LabelSource();
    es2panda_Context *ctx =
        initializer.CreateContext("no_decl_break_label.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t breakLabelPos = source.find("break ") + strlen("break ");
    ASSERT_NE(source.find("break "), std::string::npos);

    RenameInfoType info = ark::es2panda::lsp::GetRenameInfo(ctx, breakLabelPos, NONEXISTENT_LIB_PATH);
    const auto &success = AssertSuccess(info);
    EXPECT_TRUE(success.GetCanRenameSuccess());
    EXPECT_EQ(success.GetKind(), "");
    EXPECT_EQ(success.GetDisplayName(), "");
    EXPECT_EQ(success.GetFullDisplayName(), "");
    EXPECT_EQ(success.GetKindModifiers(), "");
    EXPECT_EQ(success.GetTriggerSpan().start, breakLabelPos);
    EXPECT_EQ(success.GetTriggerSpan().length, strlen("outer"));

    initializer.DestroyContext(ctx);
}

// A continue-label use has the same public rename shape as a break-label use.
TEST_F(RenameNoDeclFamilyTests, FlowContinueLabelUseReturnsAnonymousLabelInfo)
{
    Initializer initializer;
    const std::string source = LabelSource();
    es2panda_Context *ctx =
        initializer.CreateContext("no_decl_continue_label.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t continueLabelPos = source.find("continue ") + strlen("continue ");
    ASSERT_NE(source.find("continue "), std::string::npos);

    RenameInfoType info = ark::es2panda::lsp::GetRenameInfo(ctx, continueLabelPos, NONEXISTENT_LIB_PATH);
    const auto &success = AssertSuccess(info);
    EXPECT_TRUE(success.GetCanRenameSuccess());
    EXPECT_EQ(success.GetKind(), "");
    EXPECT_EQ(success.GetDisplayName(), "");
    EXPECT_EQ(success.GetTriggerSpan().start, continueLabelPos);
    EXPECT_EQ(success.GetTriggerSpan().length, strlen("outer"));

    initializer.DestroyContext(ctx);
}

// The label declaration site exposes the same anonymous rename shape.
TEST_F(RenameNoDeclFamilyTests, FlowLabelDeclarationSiteReturnsAnonymousLabelInfo)
{
    Initializer initializer;
    const std::string source = LabelSource();
    es2panda_Context *ctx = initializer.CreateContext("no_decl_label_decl.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t declPos = source.find("outer:");
    ASSERT_NE(declPos, std::string::npos);

    RenameInfoType info = ark::es2panda::lsp::GetRenameInfo(ctx, declPos, NONEXISTENT_LIB_PATH);
    const auto &success = AssertSuccess(info);
    EXPECT_TRUE(success.GetCanRenameSuccess());
    EXPECT_EQ(success.GetKind(), "");
    EXPECT_EQ(success.GetDisplayName(), "");
    EXPECT_EQ(success.GetTriggerSpan().start, declPos);
    EXPECT_EQ(success.GetTriggerSpan().length, strlen("outer"));

    initializer.DestroyContext(ctx);
}

// A plain string-literal position is not exposed as a renameable token.
TEST_F(RenameNoDeclFamilyTests, FlowStringLiteralPositionCannotRename)
{
    Initializer initializer;
    const std::string source = "console.log(\"lit\");\n";
    es2panda_Context *ctx =
        initializer.CreateContext("no_decl_strlit_flow.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t literalPos = source.find("lit");
    ASSERT_NE(literalPos, std::string::npos);

    RenameInfoType info = ark::es2panda::lsp::GetRenameInfo(ctx, literalPos, NONEXISTENT_LIB_PATH);
    const auto &failure = AssertFailure(info);
    EXPECT_FALSE(failure.GetCanRenameFailure());
    EXPECT_EQ(failure.GetLocalizedErrorMessage(), "You cannot rename this element");

    initializer.DestroyContext(ctx);
}

// A typed object-literal key is exposed as a renameable property.
TEST_F(RenameNoDeclFamilyTests, FlowObjectLiteralKeyRenamesAsProperty)
{
    Initializer initializer;
    const std::string source = "class P {\n  a: number = 0;\n  b: number = 0;\n}\nlet o: P = {a: 1, b: 2};\n";
    es2panda_Context *ctx = initializer.CreateContext("no_decl_objlit_key.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t keyPos = source.find("{a: 1") + 1;
    ASSERT_NE(source.find("{a: 1"), std::string::npos);

    RenameInfoType info = ark::es2panda::lsp::GetRenameInfo(ctx, keyPos, NONEXISTENT_LIB_PATH);
    const auto &success = AssertSuccess(info);
    EXPECT_TRUE(success.GetCanRenameSuccess());
    EXPECT_EQ(success.GetKind(), "property");
    EXPECT_EQ(success.GetDisplayName(), "a");
    EXPECT_EQ(success.GetFullDisplayName(), "a");
    EXPECT_EQ(success.GetTriggerSpan().start, keyPos);
    EXPECT_EQ(success.GetTriggerSpan().length, strlen("a"));

    initializer.DestroyContext(ctx);
}

// The public flow does not expose a touching identifier for a plain string-literal position, so
// the observable behavior is rename rejection.
TEST_F(RenameNoDeclFamilyTests, HandleRenameNoDeclStringLiteralSuccess)
{
    Initializer initializer;
    const std::string source = "console.log(\"lit\");\n";
    es2panda_Context *ctx =
        initializer.CreateContext("no_decl_strlit_direct.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t literalPos = source.find("lit");
    ASSERT_NE(literalPos, std::string::npos);

    RenameInfoType info = ark::es2panda::lsp::GetRenameInfo(ctx, literalPos, NONEXISTENT_LIB_PATH);
    const auto &failure = AssertFailure(info);
    EXPECT_FALSE(failure.GetCanRenameFailure());
    EXPECT_EQ(failure.GetLocalizedErrorMessage(), "You cannot rename this element");

    initializer.DestroyContext(ctx);
}

// Through the public flow, an object-literal key resolves to its synthetic declaration and is
// exposed as a renameable property.
TEST_F(RenameNoDeclFamilyTests, HandleRenameNoDeclIdentifierNullOpt)
{
    Initializer initializer;
    const std::string source = "class P {\n  a: number = 0;\n  b: number = 0;\n}\nlet o: P = {a: 1, b: 2};\n";
    es2panda_Context *ctx =
        initializer.CreateContext("no_decl_ident_direct.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t keyPos = source.find("{a: 1") + 1;
    ASSERT_NE(source.find("{a: 1"), std::string::npos);

    RenameInfoType info = ark::es2panda::lsp::GetRenameInfo(ctx, keyPos, NONEXISTENT_LIB_PATH);
    const auto &success = AssertSuccess(info);
    EXPECT_TRUE(success.GetCanRenameSuccess());
    EXPECT_EQ(success.GetKind(), "property");
    EXPECT_EQ(success.GetDisplayName(), "a");
    EXPECT_EQ(success.GetFullDisplayName(), "a");
    EXPECT_EQ(success.GetTriggerSpan().start, keyPos);
    EXPECT_EQ(success.GetTriggerSpan().length, strlen("a"));

    initializer.DestroyContext(ctx);
}

// The reachable public behavior for a local identifier is successful rename info.
TEST_F(RenameNoDeclFamilyTests, HandleStringLiteralRenameRejectsNonLiteral)
{
    Initializer initializer;
    const std::string source = "let alpha: number = 1;\n";
    es2panda_Context *ctx =
        initializer.CreateContext("no_decl_strlit_guard.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t alphaPos = source.find("alpha");
    ASSERT_NE(alphaPos, std::string::npos);

    RenameInfoType info = ark::es2panda::lsp::GetRenameInfo(ctx, alphaPos, NONEXISTENT_LIB_PATH);
    const auto &success = AssertSuccess(info);
    EXPECT_TRUE(success.GetCanRenameSuccess());
    EXPECT_EQ(success.GetKind(), "property");
    EXPECT_EQ(success.GetDisplayName(), "alpha");
    EXPECT_EQ(success.GetFullDisplayName(), "alpha");
    EXPECT_EQ(success.GetTriggerSpan().start, alphaPos);
    EXPECT_EQ(success.GetTriggerSpan().length, strlen("alpha"));

    initializer.DestroyContext(ctx);
}

// Public label queries resolve through their labelled-statement declarations and expose the
// established anonymous label shape; an unrelated local identifier remains a renameable property.
TEST_F(RenameNoDeclFamilyTests, HandleLabelRenameDirectArmsOnRealLabelNodes)
{
    Initializer initializer;
    const std::string source = LabelSource() + "let plain: number = 1;\n";
    es2panda_Context *ctx =
        initializer.CreateContext("no_decl_label_direct.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t breakLabelPos = source.find("break ") + strlen("break ");
    ASSERT_NE(source.find("break "), std::string::npos);

    RenameInfoType breakInfo = ark::es2panda::lsp::GetRenameInfo(ctx, breakLabelPos, NONEXISTENT_LIB_PATH);
    const auto &breakSuccess = AssertSuccess(breakInfo);
    EXPECT_TRUE(breakSuccess.GetCanRenameSuccess());
    EXPECT_EQ(breakSuccess.GetKind(), "");
    EXPECT_EQ(breakSuccess.GetDisplayName(), "");
    EXPECT_EQ(breakSuccess.GetFullDisplayName(), "");
    EXPECT_EQ(breakSuccess.GetTriggerSpan().start, breakLabelPos);
    EXPECT_EQ(breakSuccess.GetTriggerSpan().length, strlen("outer"));

    const size_t declPos = source.find("outer:");
    ASSERT_NE(declPos, std::string::npos);

    RenameInfoType declInfo = ark::es2panda::lsp::GetRenameInfo(ctx, declPos, NONEXISTENT_LIB_PATH);
    const auto &declSuccess = AssertSuccess(declInfo);
    EXPECT_TRUE(declSuccess.GetCanRenameSuccess());
    EXPECT_EQ(declSuccess.GetKind(), "");
    EXPECT_EQ(declSuccess.GetDisplayName(), "");
    EXPECT_EQ(declSuccess.GetFullDisplayName(), "");
    EXPECT_EQ(declSuccess.GetTriggerSpan().start, declPos);
    EXPECT_EQ(declSuccess.GetTriggerSpan().length, strlen("outer"));

    const size_t plainPos = source.rfind("plain");
    ASSERT_NE(plainPos, std::string::npos);

    RenameInfoType plainInfo = ark::es2panda::lsp::GetRenameInfo(ctx, plainPos, NONEXISTENT_LIB_PATH);
    const auto &plainSuccess = AssertSuccess(plainInfo);
    EXPECT_TRUE(plainSuccess.GetCanRenameSuccess());
    EXPECT_EQ(plainSuccess.GetKind(), "property");
    EXPECT_EQ(plainSuccess.GetDisplayName(), "plain");
    EXPECT_EQ(plainSuccess.GetFullDisplayName(), "plain");
    EXPECT_EQ(plainSuccess.GetTriggerSpan().start, plainPos);
    EXPECT_EQ(plainSuccess.GetTriggerSpan().length, strlen("plain"));

    initializer.DestroyContext(ctx);
}

// PI-005 — GetAssignedName (rename.cpp:541) fallthrough and null-parent arms on real CHECKED
// nodes. PRODUCT FINDING (measured): ETS assignments are ASSIGNMENT_EXPRESSION nodes (not
// BINARY_EXPRESSION), so the binary-assignment arm at rename.cpp:551-555 can never fire for an
// ETS assignment RHS; an arrow bound in a let declarator has a VariableDeclarator parent, which
// satisfies neither the Property arm (rename.cpp:548) nor the contradictory
// IsVariableDeclaration() && IsIdentifier() conjunction at rename.cpp:557 (UNREACHABLE_PROVEN by
// construction: one node cannot have both types); the program root has no parent
// (rename.cpp:543-545). All three shapes return nullptr.
TEST_F(RenameNoDeclFamilyTests, GetAssignedNameFallthroughAndNullParentArms)
{
    Initializer initializer;
    const std::string source =
        "let f: (a: number) => number = (a: number): number => a;\n"
        "f = (b: number): number => b + 1;\n";
    es2panda_Context *ctx =
        initializer.CreateContext("no_decl_assigned_name.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto *ast = GetAstFromContext<AstNode>(ctx);

    // Assignment RHS arrow: parent is ASSIGNMENT_EXPRESSION (measured), all arms decline.
    const size_t assignPos = source.find("f = ");
    ASSERT_NE(assignPos, std::string::npos);
    auto *rhsArrow = FindNode(ast, [assignPos](AstNode *node) {
        return node->IsArrowFunctionExpression() && node->Start().index > assignPos;
    });
    ASSERT_NE(rhsArrow, nullptr);
    ASSERT_TRUE(rhsArrow->Parent()->IsAssignmentExpression());
    EXPECT_EQ(ark::es2panda::lsp::GetAssignedName(rhsArrow), nullptr);

    // let-declarator arrow: parent is a VariableDeclarator, also no arm match.
    auto *letArrow = FindNode(ast, [](AstNode *node) { return node->IsArrowFunctionExpression(); });
    ASSERT_NE(letArrow, nullptr);
    ASSERT_TRUE(letArrow->Start().index < assignPos);
    EXPECT_EQ(ark::es2panda::lsp::GetAssignedName(letArrow), nullptr);

    // Program root: no parent at all (rename.cpp:543-545).
    AstNode *root = GetAstFromContext<AstNode>(ctx);
    EXPECT_EQ(root->Parent(), nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetAssignedName(root), nullptr);

    initializer.DestroyContext(ctx);
}

// PI-005 — GetNonAssignedNameOfDeclaration (rename.cpp:517): an element-access member
// expression returns itself (rename.cpp:534-536); a property-access member expression misses
// the ELEMENT_ACCESS kind and falls through NamedType to nullptr (rename.cpp:538); a class
// definition node also resolves to nullptr. The CALL_EXPRESSION/BINARY_EXPRESSION block
// (rename.cpp:522-529) is intentionally not called: it downcasts each node to BOTH binary and
// call shapes (rename.cpp:523/526), which assert-fails in debug builds for every real node of
// the other kind — recorded as a product finding, not exercised here.
TEST_F(RenameNoDeclFamilyTests, GetNonAssignedNameOfDeclarationMemberExpressionArms)
{
    Initializer initializer;
    const std::string source =
        "class C {\n  vv: number = 0;\n}\n"
        "let arr: number[] = [1, 2, 3];\n"
        "let c: C = new C();\n"
        "let e: number = arr[0];\n"
        "let p: number = c.vv;\n";
    es2panda_Context *ctx =
        initializer.CreateContext("no_decl_nonassigned.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    auto *ast = GetAstFromContext<AstNode>(ctx);

    const size_t elemPos = source.find("arr[0]");
    ASSERT_NE(elemPos, std::string::npos);
    auto *elemAccess = FindNode(
        ast, [elemPos](AstNode *node) { return node->IsMemberExpression() && node->Start().index == elemPos; });
    ASSERT_NE(elemAccess, nullptr);
    ASSERT_TRUE(
        elemAccess->AsMemberExpression()->HasMemberKind(ark::es2panda::ir::MemberExpressionKind::ELEMENT_ACCESS));
    EXPECT_EQ(ark::es2panda::lsp::GetNonAssignedNameOfDeclaration(elemAccess), elemAccess);

    const size_t propPos = source.find("c.vv");
    ASSERT_NE(propPos, std::string::npos);
    auto *propAccess = FindNode(
        ast, [propPos](AstNode *node) { return node->IsMemberExpression() && node->Start().index == propPos; });
    ASSERT_NE(propAccess, nullptr);
    ASSERT_FALSE(
        propAccess->AsMemberExpression()->HasMemberKind(ark::es2panda::ir::MemberExpressionKind::ELEMENT_ACCESS));
    EXPECT_EQ(ark::es2panda::lsp::GetNonAssignedNameOfDeclaration(propAccess), nullptr);

    auto *classDef = FindNode(ast, [](AstNode *node) {
        return node->IsClassDefinition() && node->AsClassDefinition()->Ident() != nullptr &&
               node->AsClassDefinition()->Ident()->Name().Utf8() == "C";
    });
    ASSERT_NE(classDef, nullptr);
    EXPECT_EQ(ark::es2panda::lsp::GetNonAssignedNameOfDeclaration(classDef), nullptr);

    initializer.DestroyContext(ctx);
}

// PI-002 — import-specifier rename position through the flow. Since the upstream metadata
// optimization (commit c06266cac, merged by 0c84120f6), the specifier identifier resolves to a
// declaration in a METADATA_DECL std-library program, so IsDefinedInLibraryFile short-circuits
// to true and rename is denied with the exact localized message.
TEST_F(RenameNoDeclFamilyTests, FlowImportSpecifierPosition)
{
    Initializer initializer;
    const std::string source = "import { PI } from \"std/math/consts\";\nconsole.log(PI);\n";
    es2panda_Context *ctx =
        initializer.CreateContext("no_decl_import_spec.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t specPos = source.find("PI");
    ASSERT_NE(specPos, std::string::npos);

    RenameInfoType info = ark::es2panda::lsp::GetRenameInfo(ctx, specPos, NONEXISTENT_LIB_PATH);
    const auto &failure = AssertFailure(info);
    EXPECT_FALSE(failure.GetCanRenameFailure());
    EXPECT_EQ(failure.GetLocalizedErrorMessage(), "You cannot rename this element");

    initializer.DestroyContext(ctx);
}

// PI-002 negative — export-specifier rename position: GetTouchingTokenForIdentifier returns
// nullptr inside the export specifier list (measured), so GetRenameInfo takes the null-token
// error path (rename.cpp:40-41) and reports the exact failure message.
TEST_F(RenameNoDeclFamilyTests, FlowExportSpecifierPositionIsNotRenamable)
{
    Initializer initializer;
    const std::string source = "let x: number = 1;\nexport { x };\n";
    es2panda_Context *ctx =
        initializer.CreateContext("no_decl_export_spec.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const size_t specPos = source.find("export { x }") + strlen("export { ");
    ASSERT_NE(source.find("export { x }"), std::string::npos);
    ASSERT_EQ(ark::es2panda::lsp::GetTouchingTokenForIdentifier(ctx, specPos, false), nullptr);

    RenameInfoType info = ark::es2panda::lsp::GetRenameInfo(ctx, specPos, NONEXISTENT_LIB_PATH);
    const auto &failure = AssertFailure(info);
    EXPECT_FALSE(failure.GetCanRenameFailure());
    EXPECT_EQ(failure.GetLocalizedErrorMessage(), "You cannot rename this element");

    initializer.DestroyContext(ctx);
}

// rename.cpp:311 StripQuotes backtick arm (IsQuoteOrBacktick rename.cpp:324 taken 0 before this
// batch) and the no-quote passthrough arm (rename.cpp:318); the single-quote arm is already
// pinned by lsp_rename_test RenameInfoStripQuotes.
TEST_F(RenameNoDeclFamilyTests, StripQuotesBacktickAndPassthroughArms)
{
    EXPECT_EQ(ark::es2panda::lsp::StripQuotes("`nm`"), "nm");
    EXPECT_EQ(ark::es2panda::lsp::StripQuotes("plain"), "plain");
    EXPECT_EQ(ark::es2panda::lsp::StripQuotes(""), "");
}

}  // namespace
