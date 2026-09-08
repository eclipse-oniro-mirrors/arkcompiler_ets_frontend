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

#include "lsp/include/quick_info.h"
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {

using ark::es2panda::lsp::Initializer;

class LspQuickInfo3Tests : public LSPAPITests {};

bool DisplayHasText(const std::vector<SymbolDisplayPart> &parts, const std::string &text)
{
    for (const auto &part : parts) {
        if (part.GetText() == text) {
            return true;
        }
    }
    return false;
}

ark::es2panda::ir::AstNode *FindClassMethod(ark::es2panda::ir::AstNode *ast, bool requireConstructor,
                                            bool requireGetter, bool requireSetter)
{
    return ast->FindChild([&](ark::es2panda::ir::AstNode *n) {
        if (!n->IsMethodDefinition()) {
            return false;
        }
        auto *m = n->AsMethodDefinition();
        return m->IsConstructor() == requireConstructor && m->IsGetter() == requireGetter &&
               m->IsSetter() == requireSetter;
    });
}

// CreateDisplayForMethodDefinition with the "constructor" kind modifier routes
// to CreateDisplayForMethodDefinitionOfConstructor.
TEST_F(LspQuickInfo3Tests, CreateDisplayConstructor)
{
    const std::string src = R"(class CtorCls {
    x: number = 1;
    constructor(x: number) {
        this.x = x;
    }
})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi3_ctor.ets", ES2PANDA_STATE_CHECKED, src.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *pctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(pctx->GetChecker());
    auto *ast = pctx->parserProgram->Ast();
    auto *ctor = FindClassMethod(ast, true, false, false);
    ASSERT_NE(ctor, nullptr);
    auto parts = ark::es2panda::lsp::CreateDisplayForMethodDefinition(ctor, "constructor", checker);
    initializer.DestroyContext(ctx);

    ASSERT_FALSE(parts.empty());
    EXPECT_EQ(parts.front().GetText(), "constructor");
    EXPECT_TRUE(DisplayHasText(parts, "CtorCls"));
}

// CreateDisplayForMethodDefinition with the "getter" kind modifier routes to
// CreateDisplayForMethodDefinitionOfGetterOrSetter.
TEST_F(LspQuickInfo3Tests, CreateDisplayGetter)
{
    const std::string src = R"(class GetterCls {
    private _v: number = 1;
    get v(): number {
        return this._v;
    }
})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi3_getter.ets", ES2PANDA_STATE_CHECKED, src.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *pctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(pctx->GetChecker());
    auto *ast = pctx->parserProgram->Ast();
    auto *getter = FindClassMethod(ast, false, true, false);
    ASSERT_NE(getter, nullptr);
    auto parts = ark::es2panda::lsp::CreateDisplayForMethodDefinition(getter, "getter", checker);
    initializer.DestroyContext(ctx);

    ASSERT_FALSE(parts.empty());
    EXPECT_TRUE(DisplayHasText(parts, "getter"));
    EXPECT_TRUE(DisplayHasText(parts, "GetterCls"));
    EXPECT_TRUE(DisplayHasText(parts, "v"));
}

// CreateDisplayForMethodDefinition with the "setter" kind modifier routes to
// CreateDisplayForMethodDefinitionOfGetterOrSetter.
TEST_F(LspQuickInfo3Tests, CreateDisplaySetter)
{
    const std::string src = R"(class SetterCls {
    private _v: number = 1;
    set v(val: number) {
        this._v = val;
    }
})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi3_setter.ets", ES2PANDA_STATE_CHECKED, src.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto *pctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(pctx->GetChecker());
    auto *ast = pctx->parserProgram->Ast();
    auto *setter = FindClassMethod(ast, false, false, true);
    ASSERT_NE(setter, nullptr);
    auto parts = ark::es2panda::lsp::CreateDisplayForMethodDefinition(setter, "setter", checker);
    initializer.DestroyContext(ctx);

    ASSERT_FALSE(parts.empty());
    EXPECT_TRUE(DisplayHasText(parts, "setter"));
    EXPECT_TRUE(DisplayHasText(parts, "SetterCls"));
}

// CreateDisplayForClass on a struct declaration routes to
// GetNameFromETSStructDeclaration.
TEST_F(LspQuickInfo3Tests, CreateDisplayStruct)
{
    const std::string src = R"(struct Point3 {
    x: number = 0;
})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi3_struct.ets", ES2PANDA_STATE_PARSED, src.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *pctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = pctx->parserProgram->Ast();
    auto *structDecl = ast->FindChild([](ark::es2panda::ir::AstNode *n) { return n->IsETSStructDeclaration(); });
    ASSERT_NE(structDecl, nullptr);
    auto parts = ark::es2panda::lsp::CreateDisplayForClass(structDecl);
    initializer.DestroyContext(ctx);

    ASSERT_FALSE(parts.empty());
    EXPECT_EQ(parts.front().GetText(), "struct");
    EXPECT_TRUE(DisplayHasText(parts, "Point3"));
}

// Global const with a string literal value and no explicit type annotation
// routes to EscapeJsonString via GetNameForLiteralTypeNode.
TEST_F(LspQuickInfo3Tests, HoverConstStringLiteralValue)
{
    const std::string source = R"(const GREETING = "hello";)";
    const std::string fileName = "qi3_const_str.ets";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext(fileName.c_str(), ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    auto markerPos = source.find("GREETING");
    ASSERT_NE(markerPos, std::string::npos);
    LSPAPI const *lspApi = GetImpl();
    auto info = lspApi->getQuickInfoAtPosition(fileName.c_str(), ctx, markerPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "property");
    EXPECT_TRUE(DisplayHasText(info.GetDisplayParts(), "const"));
    EXPECT_TRUE(DisplayHasText(info.GetDisplayParts(), "GREETING"));
    // The string literal value is rendered through EscapeJsonString.
    EXPECT_TRUE(DisplayHasText(info.GetDisplayParts(), "\"hello\""));
}

// GetNameFromClassExpression and the CLASS_EXPRESSION arm of
// CreateDisplayForClass were removed as structurally dead: the only
// ir::ClassExpression construction site is the shared ParseClassExpression
// (expressionParser.cpp:1246), reached only from the shared
// ParsePrimaryExpression KEYW_CLASS arm (expressionParser.cpp:1301).
// ETSParser::ParsePrimaryExpression (ETSparserExpressions.cpp:431) overrides
// that entry without a KEYW_CLASS case, so ETS never allocates a
// ClassExpression. Expression-position "X.class" is rejected as a syntax
// error (ESY0316/ESY0224), and the recovered ETSClassLiteral form is reported
// as UNSUPPORTED_CLASS_LITERAL by the checker (ETSAnalyzer.cpp). This test
// guards the invariant that keeps those dead branches removed.
TEST_F(LspQuickInfo3Tests, EtsParserNeverProducesClassExpressionNodes)
{
    const std::string src = R"(class BaseQ {
    v: number = 1;
}
class DerivedQ extends BaseQ {
    constructor() {
        super();
    }
}
let derivedQ = new DerivedQ();)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi3_class_forms.ets", ES2PANDA_STATE_PARSED, src.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *pctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = pctx->parserProgram->Ast();

    // Sanity: declaration-form classes parse normally.
    ASSERT_NE(ast->FindChild([](ark::es2panda::ir::AstNode *n) { return n->IsClassDeclaration(); }), nullptr);
    // Dead-branch proof: no ir::ClassExpression anywhere in the parsed tree.
    EXPECT_EQ(ast->FindChild([](ark::es2panda::ir::AstNode *n) { return n->IsClassExpression(); }), nullptr);
    initializer.DestroyContext(ctx);
}

// CreateDisplayForImportDeclaration and both IMPORT_DECLARATION dispatch arms
// (GetQuickInfoAtPositionImpl's chain in quick_info.cpp and
// GetDisplayPartAndKind in completions_details.cpp) were removed as
// structurally dead: plain ir::ImportDeclaration is constructed only by the
// TS/JS/AS parsers (statementTSParser.cpp:374, statementParser.cpp:2052,
// ASparser.cpp:1747). The ETS parser builds ir::ETSImportDeclaration
// exclusively (ETSparser.cpp:1187), and LSP contexts are always created with
// "--extension ets" (bindings lsp_helper.ts). This test guards the invariant
// that keeps those arms removed.
TEST_F(LspQuickInfo3Tests, EtsImportsProduceOnlyEtsImportDeclarations)
{
    const std::string src = R"(import type { T4 } from './qi3_imp_lib';
import { B4 } from './qi3_imp_lib';
class UsesImports {
    v: number = 0;
})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi3_imports.ets", ES2PANDA_STATE_PARSED, src.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *pctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = pctx->parserProgram->Ast();

    // Both ETS import forms parse successfully...
    ASSERT_NE(ast->FindChild([](ark::es2panda::ir::AstNode *n) { return n->IsETSImportDeclaration(); }), nullptr);
    ASSERT_NE(ast->FindChild([](ark::es2panda::ir::AstNode *n) { return n->IsImportSpecifier(); }), nullptr);
    // ...but only as ETS_IMPORT_DECLARATION; no plain IMPORT_DECLARATION exists.
    EXPECT_EQ(ast->FindChild([](ark::es2panda::ir::AstNode *n) { return n->IsImportDeclaration(); }), nullptr);
    initializer.DestroyContext(ctx);
}

// GetNodeFileName and the object-literal filename override in
// GetQuickInfoAtPositionImpl were removed as structurally dead: the override
// was guarded by contextualTypeNode->IsETSImportDeclaration(), and the
// contextual node comes from Type::Variable()->Declaration()->Node(), where no
// production Decl ever binds an ETSImportDeclaration node: every ImportDecl
// construction binds an Identifier (ETSBinder.cpp:1212/1243/1261/1313),
// synthetic namespace members bind source decl/class-definition nodes
// (namespaceImportObject.cpp:135-139), and the module-object root variable
// binds the import identifier (helpers.cpp:3764-3765). Pinned tests
// getPropertySymbolFromContextualType1/2 show real targets are
// TS_INTERFACE_DECLARATION / CLASS_DEFINITION nodes. This test pins the
// observable contract through the public API: hovering an object-literal key
// whose contextual type is an imported interface keeps the hovered file name
// instead of replacing it with the import's resolved source.
TEST_F(LspQuickInfo3Tests, HoverObjectLiteralPropertyKeepsOriginalFileName)
{
    std::vector<std::string> files = {"qi3_gnf_lib.ets", "qi3_gnf_main.ets"};
    std::vector<std::string> texts = {R"(export interface GnFI {
    key6: string;
})",
                                      R"(import {GnFI} from './qi3_gnf_lib';
class GnFHolder {
    p6: GnFI = { key6: "v6" };
})"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_NE(ctx, nullptr);

    const auto markerPos = texts[1].find("key6");
    ASSERT_NE(markerPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition(filePaths[1].c_str(), ctx, markerPos);
    initializer.DestroyContext(ctx);

    // Removed dead GetNodeFileName override no longer fires: file name kept.
    EXPECT_EQ(info.GetFileName(), filePaths[1]);
    // Contextual-type flow still resolves the property through interface GnFI.
    EXPECT_TRUE(DisplayHasText(info.GetDisplayParts(), "GnFI"));
    EXPECT_TRUE(DisplayHasText(info.GetDisplayParts(), "key6"));
    // Span stays on the hovered object-literal key token in the main file.
    EXPECT_EQ(info.GetTextSpan().start, markerPos);
    EXPECT_EQ(info.GetTextSpan().length, std::string("key6").size());
}

}  // namespace
