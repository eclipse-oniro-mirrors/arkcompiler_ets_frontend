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

class QuickInfoDisplayMatrixTests : public LSPAPITests {};

bool DisplayHasText(const std::vector<SymbolDisplayPart> &parts, const std::string &text)
{
    for (const auto &part : parts) {
        if (part.GetText() == text) {
            return true;
        }
    }
    return false;
}

std::vector<SymbolDisplayPart> MakeParts(const std::vector<std::pair<std::string, std::string>> &texts)
{
    std::vector<SymbolDisplayPart> parts;
    parts.reserve(texts.size());
    for (const auto &item : texts) {
        parts.emplace_back(item.first, item.second);
    }
    return parts;
}

// Display-part index of the resolved module path inside an import hover:
// parts are ["module" keyword, " " space, <module path> className].
constexpr size_t MODULE_PATH_PART_IDX = 2;

std::vector<std::pair<std::string, std::string>> EnumDisplayTail(const char *enumName, const char *name,
                                                                 const char *value)
{
    return std::vector<std::pair<std::string, std::string>> {
        {enumName, "enumName"}, {".", "punctuation"}, {name, "enumMember"}, {" ", "space"},
        {"=", "operator"},      {" ", "space"},       {value, "text"}};
}

std::vector<std::pair<std::string, std::string>> EnumStringDisplayTail(const char *enumName, const char *name,
                                                                       const char *value)
{
    return std::vector<std::pair<std::string, std::string>> {
        {enumName, "enumName"}, {".", "punctuation"},  {name, "enumMember"}, {" ", "space"},       {"=", "operator"},
        {" ", "space"},         {"\"", "punctuation"}, {value, "text"},      {"\"", "punctuation"}};
}

std::vector<std::pair<std::string, std::string>> PropertyDisplayHead(const char *name, const char *type)
{
    return std::vector<std::pair<std::string, std::string>> {
        {"QidProps13", "className"}, {".", "punctuation"}, {name, "property"},
        {":", "punctuation"},        {" ", "space"},       {type, "typeName"}};
}

QuickInfo PropertyQuickInfo(LSPAPI const *lspApi, es2panda_Context *ctx, const std::string &source, const char *name)
{
    auto pos = source.find(name);
    EXPECT_NE(pos, std::string::npos);
    return lspApi->getQuickInfoAtPosition("qid_properties.ets", ctx, pos);
}

std::vector<SymbolDisplayPart> CtorSignatureParts()
{
    return MakeParts({{"QidSig14", "className"},
                      {".", "punctuation"},
                      {"constructor", "functionName"},
                      {"(", "punctuation"},
                      {"c14", "functionParameter"},
                      {":", "punctuation"},
                      {" ", "space"},
                      {"number", "typeParameter"},
                      {")", "punctuation"},
                      {":", "punctuation"},
                      {" ", "space"},
                      {"undefined", "returnType"}});
}

std::vector<SymbolDisplayPart> GenericSignatureParts()
{
    return MakeParts({{"QidSig14", "className"},
                      {".", "punctuation"},
                      {"generic14", "functionName"},
                      {"<", "punctuation"},
                      {"T", "typeParameter"},
                      {">", "punctuation"},
                      {"(", "punctuation"},
                      {"t14", "functionParameter"},
                      {":", "punctuation"},
                      {" ", "space"},
                      {"T", "typeParameter"},
                      {")", "punctuation"},
                      {":", "punctuation"},
                      {" ", "space"},
                      {"T", "returnType"}});
}

std::vector<SymbolDisplayPart> UnionRetSignatureParts()
{
    return MakeParts({{"QidSig14", "className"},
                      {".", "punctuation"},
                      {"unionRet14", "functionName"},
                      {"(", "punctuation"},
                      {"u14", "functionParameter"},
                      {":", "punctuation"},
                      {" ", "space"},
                      {"number", "typeParameter"},
                      {")", "punctuation"},
                      {":", "punctuation"},
                      {" ", "space"},
                      {"string", "returnType"},
                      {"|", "punctuation"},
                      {" ", "space"},
                      {"number", "returnType"}});
}

std::vector<SymbolDisplayPart> OptParamSignatureParts()
{
    return MakeParts({{"QidSig14", "className"},
                      {".", "punctuation"},
                      {"optParam14", "functionName"},
                      {"(", "punctuation"},
                      {"a14", "functionParameter"},
                      {":", "punctuation"},
                      {" ", "space"},
                      {"number", "typeParameter"},
                      {",", "punctuation"},
                      {" ", "space"},
                      {"b14", "functionParameter"},
                      {":", "punctuation"},
                      {" ", "space"},
                      {"number", "typeParameter"},
                      {")", "punctuation"},
                      {":", "punctuation"},
                      {" ", "space"},
                      {"number", "returnType"}});
}

std::vector<SymbolDisplayPart> NoParamSignatureParts()
{
    return MakeParts({{"QidSig14", "className"},
                      {".", "punctuation"},
                      {"noParam14", "functionName"},
                      {"(", "punctuation"},
                      {")", "punctuation"},
                      {":", "punctuation"},
                      {" ", "space"}});
}

std::vector<SymbolDisplayPart> ClsParamSignatureParts()
{
    return MakeParts({{"QidSig14", "className"},
                      {".", "punctuation"},
                      {"clsParam14", "functionName"},
                      {"(", "punctuation"},
                      {"p14", "functionParameter"},
                      {":", "punctuation"},
                      {" ", "space"},
                      {"QidOther14", "typeParameter"},
                      {")", "punctuation"},
                      {":", "punctuation"},
                      {" ", "space"},
                      {"QidOther14", "returnType"}});
}

// Collects the hover infos of every signature shape, releases the context and pins
// each rendered display against its expected part sequence.
void ExpectSignatureDisplayShapes(Initializer &initializer, es2panda_Context *ctx, const std::string &source)
{
    LSPAPI const *lspApi = GetImpl();

    auto ctorPos = source.find("constructor");
    auto genericPos = source.find("generic14");
    auto unionPos = source.find("unionRet14");
    auto optPos = source.find("optParam14");
    auto noParamPos = source.find("noParam14");
    auto clsParamPos = source.find("clsParam14");
    ASSERT_NE(ctorPos, std::string::npos);
    ASSERT_NE(genericPos, std::string::npos);
    ASSERT_NE(unionPos, std::string::npos);
    ASSERT_NE(optPos, std::string::npos);
    ASSERT_NE(noParamPos, std::string::npos);
    ASSERT_NE(clsParamPos, std::string::npos);

    auto ctorInfo = lspApi->getQuickInfoAtPosition("qid_signature.ets", ctx, ctorPos);
    auto genericInfo = lspApi->getQuickInfoAtPosition("qid_signature.ets", ctx, genericPos);
    auto unionInfo = lspApi->getQuickInfoAtPosition("qid_signature.ets", ctx, unionPos);
    auto optInfo = lspApi->getQuickInfoAtPosition("qid_signature.ets", ctx, optPos);
    auto noParamInfo = lspApi->getQuickInfoAtPosition("qid_signature.ets", ctx, noParamPos);
    auto clsParamInfo = lspApi->getQuickInfoAtPosition("qid_signature.ets", ctx, clsParamPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(ctorInfo.GetKindModifiers(), "public constructor");
    EXPECT_EQ(ctorInfo.GetDisplayParts(), CtorSignatureParts());
    EXPECT_EQ(genericInfo.GetDisplayParts(), GenericSignatureParts());
    EXPECT_EQ(unionInfo.GetDisplayParts(), UnionRetSignatureParts());
    EXPECT_EQ(optInfo.GetDisplayParts(), OptParamSignatureParts());
    // A void return annotation is not an ETS type reference, so the return-type arm
    // renders nothing and the display stops after the ": " punctuation pair.
    EXPECT_EQ(noParamInfo.GetDisplayParts(), NoParamSignatureParts());
    EXPECT_EQ(clsParamInfo.GetDisplayParts(), ClsParamSignatureParts());
}

template <typename Pred>
ark::es2panda::ir::AstNode *FindFirstNode(ark::es2panda::ir::AstNode *root, Pred pred)
{
    ark::es2panda::ir::AstNode *result = nullptr;
    root->FindChild([&](ark::es2panda::ir::AstNode *n) {
        if (result == nullptr && pred(n)) {
            result = n;
        }
        return false;
    });
    return result;
}

// PC-001: GetNameFromClassExpression and the CLASS_EXPRESSION arm of CreateDisplayForClass
// are unreachable through every context the LSP can create. ir::ClassExpression is allocated
// only by the shared ParserImpl::ParseClassExpression (expressionParser.cpp:1246), reachable
// only from the shared ParsePrimaryExpression KEYW_CLASS arm (expressionParser.cpp:1301);
// ETSParser::ParsePrimaryExpression (ETSparserExpressions.cpp:431) overrides that entry
// without a KEYW_CLASS case, and LSP contexts always parse with the ETS parser
// (public InitializeContext creates an ETSParser unconditionally) regardless of the
// file-name extension. This test pins that a .ts extension does not open a TS route: the
// class-expression source parses through ETS error recovery into a class declaration,
// leaves no ClassExpression node behind, and the hover sites stay empty.
TEST_F(QuickInfoDisplayMatrixTests, TsNamedClassExpressionSourceHasNoClassExpression)
{
    const std::string src = "let qce4 = class QceN4 { m14(): number { return 1; } };";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qid_class_expr.ts", ES2PANDA_STATE_PARSED, src.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *pctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = pctx->parserProgram->Ast();
    // No ClassExpression survives anywhere in the recovered tree: the source is
    // shaped by ETS error recovery instead of TS class-expression parsing.
    EXPECT_EQ(FindFirstNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsClassExpression(); }), nullptr);

    LSPAPI const *lspApi = GetImpl();
    auto classPos = src.find("class");
    auto varPos = src.find("qce4");
    ASSERT_NE(classPos, std::string::npos);
    ASSERT_NE(varPos, std::string::npos);
    // Hovering the recovered declaration keyword and the target variable both yield no
    // quick info, so the "(local class) " display cannot be reached by hover either.
    auto classInfo = lspApi->getQuickInfoAtPosition("qid_class_expr.ts", ctx, classPos);
    auto varInfo = lspApi->getQuickInfoAtPosition("qid_class_expr.ts", ctx, varPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(classInfo, QuickInfo());
    EXPECT_EQ(varInfo, QuickInfo());
}

// PC-002: CreateDisplayForImportDeclaration and its IMPORT_DECLARATION dispatch arm are
// unreachable through every context the LSP can create: plain ir::ImportDeclaration is
// allocated only by TSParser (statementTSParser.cpp:374), the shared JS path
// (statementParser.cpp:2052) and ASparser (ASparser.cpp:1747), while LSP contexts always
// parse with the ETS parser regardless of the file-name extension, and the ETS parser
// builds ir::ETSImportDeclaration exclusively (ETSparser.cpp:1171/1201/1204). This extends
// the pinned .ets proof (LspQuickInfo3Tests.EtsImportsProduceOnlyEtsImportDeclarations)
// to .ts-named files and pins the public hover display on such imports.
TEST_F(QuickInfoDisplayMatrixTests, TsNamedImportsProduceOnlyEtsImportDeclarations)
{
    std::vector<std::string> files = {"qid_imp_mod.ts", "qid_imp_main.ts"};
    std::vector<std::string> texts = {"export const qidMa4: number = 1;\nexport const qidMb4: number = 2;\n",
                                      R"(import { qidMa4, qidMb4 } from "./qid_imp_mod";
import qidDef4 from "./qid_imp_mod";
import * as qidNs4 from "./qid_imp_mod";
let qidUse4 = qidMa4 + qidMb4;)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *pctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = pctx->parserProgram->Ast();

    // All three import forms parse, but only as ETSImportDeclaration + TS specifier nodes.
    EXPECT_NE(FindFirstNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsETSImportDeclaration(); }), nullptr);
    EXPECT_NE(FindFirstNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsImportSpecifier(); }), nullptr);
    EXPECT_NE(FindFirstNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsImportDefaultSpecifier(); }), nullptr);
    EXPECT_NE(FindFirstNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsImportNamespaceSpecifier(); }),
              nullptr);
    EXPECT_EQ(FindFirstNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsImportDeclaration(); }), nullptr);

    // Hovering the module string of an ETS import resolves to the ETSImportDeclaration
    // node and renders the "module <resolved source>" display.
    LSPAPI const *lspApi = GetImpl();
    const auto modulePos = texts[1].find("./qid_imp_mod");
    ASSERT_NE(modulePos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition(filePaths[1].c_str(), ctx, modulePos + 1);
    initializer.DestroyContext(ctx);

    ASSERT_FALSE(info.GetDisplayParts().empty());
    EXPECT_EQ(info.GetDisplayParts().front().GetText(), "module");
    EXPECT_EQ(info.GetDisplayParts().front().GetKind(), "keyword");
    EXPECT_EQ(info.GetDisplayParts().at(1).GetText(), " ");
    EXPECT_EQ(info.GetDisplayParts().at(1).GetKind(), "space");
    ASSERT_GT(info.GetDisplayParts().size(), 2U);
    EXPECT_EQ(info.GetDisplayParts().at(MODULE_PATH_PART_IDX).GetKind(), "className");
    EXPECT_NE(info.GetDisplayParts().at(MODULE_PATH_PART_IDX).GetText().find("qid_imp_mod"), std::string::npos);
}

// PC-003: the object-literal contextual type still resolves through the imported
// interface when the literal feeds an assignment expression. The ETSImportDeclaration
// file-name override in GetQuickInfoAtPositionImpl cannot fire (no production decl
// ever binds an ETSImportDeclaration node), so the hovered file name is kept.
TEST_F(QuickInfoDisplayMatrixTests, ImportedContextualTypeFileName)
{
    std::vector<std::string> files = {"qid_gnf_lib.ets", "qid_gnf_main.ets"};
    std::vector<std::string> texts = {"export interface QidPt4 {\n    x4: number;\n}\n",
                                      R"(import {QidPt4} from './qid_gnf_lib';
let qidAssign4: QidPt4 = { x4: 0 };
qidAssign4 = { x4: 9 };)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    const auto markerPos = texts[1].rfind("x4");
    ASSERT_NE(markerPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition(filePaths[1].c_str(), ctx, markerPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetFileName(), filePaths[1]);
    EXPECT_TRUE(DisplayHasText(info.GetDisplayParts(), "QidPt4"));
    EXPECT_TRUE(DisplayHasText(info.GetDisplayParts(), "x4"));
    EXPECT_EQ(info.GetTextSpan().start, markerPos);
    EXPECT_EQ(info.GetTextSpan().length, std::string("x4").size());
}

// PC-011: enum member hover display for the initializers reachable through a CHECKED
// context: implicit zero, explicit numeric, constant-folded computed value, single-quote
// char (surfaced by the parser as a string literal) and a double-quoted string.
TEST_F(QuickInfoDisplayMatrixTests, EnumMemberInitializerMatrix)
{
    const std::string source = R"(enum QidNum4 {
    QRED4,
    QGREEN4 = 2,
    QCALC4 = 1 + 2
}
enum QidStr4 {
    QCHAR4 = 'c',
    QSTR4 = "s4"
}
let qpick4: QidNum4 = QidNum4.QGREEN4;)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qid_enum_members.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto redPos = source.find("QRED4");
    auto greenPos = source.find("QGREEN4");
    auto calcPos = source.find("QCALC4");
    auto charPos = source.find("QCHAR4");
    auto strPos = source.find("QSTR4");
    ASSERT_NE(redPos, std::string::npos);
    ASSERT_NE(greenPos, std::string::npos);
    ASSERT_NE(calcPos, std::string::npos);
    ASSERT_NE(charPos, std::string::npos);
    ASSERT_NE(strPos, std::string::npos);

    auto redInfo = lspApi->getQuickInfoAtPosition("qid_enum_members.ets", ctx, redPos);
    auto greenInfo = lspApi->getQuickInfoAtPosition("qid_enum_members.ets", ctx, greenPos);
    auto calcInfo = lspApi->getQuickInfoAtPosition("qid_enum_members.ets", ctx, calcPos);
    auto charInfo = lspApi->getQuickInfoAtPosition("qid_enum_members.ets", ctx, charPos);
    auto strInfo = lspApi->getQuickInfoAtPosition("qid_enum_members.ets", ctx, strPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(redInfo.GetDisplayParts(), MakeParts(EnumDisplayTail("QidNum4", "QRED4", "0")));
    EXPECT_EQ(greenInfo.GetDisplayParts(), MakeParts(EnumDisplayTail("QidNum4", "QGREEN4", "2")));
    EXPECT_EQ(charInfo.GetDisplayParts(), MakeParts(EnumStringDisplayTail("QidStr4", "QCHAR4", "c")));
    EXPECT_EQ(strInfo.GetDisplayParts(), MakeParts(EnumStringDisplayTail("QidStr4", "QSTR4", "s4")));
    // The checker folds the computed initializer into a number literal, so the
    // NUMBER_LITERAL arm renders its folded value.
    EXPECT_EQ(calcInfo.GetDisplayParts(), MakeParts(EnumDisplayTail("QidNum4", "QCALC4", "3")));
    EXPECT_EQ(greenInfo.GetKind(), "enum member");
    EXPECT_EQ(greenInfo.GetTextSpan().start, greenPos);
    EXPECT_EQ(greenInfo.GetTextSpan().length, std::string("QGREEN4").size());
}

// PC-011: boolean, null and undefined initializers have dedicated arms in
// CreateDisplayForEnumMember; they are exercised on a PARSED enum through the exported
// helper because the public hover route is checker-dependent. The bigint initializer
// keeps its BigIntLiteral arm but renders the normalized digits without the "n" suffix.
TEST_F(QuickInfoDisplayMatrixTests, EnumMemberExoticInitializerDirectDisplay)
{
    const std::string source =
        "enum QidEx4 {\n    QBOOL4 = true,\n    QNULL4 = null,\n    QUNDEF4 = undefined,\n"
        "    QBIG4 = 1n\n}";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qid_enum_exotic.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    auto *pctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    auto *ast = pctx->parserProgram->Ast();
    auto *enumDecl = FindFirstNode(ast, [](ark::es2panda::ir::AstNode *n) { return n->IsTSEnumDeclaration(); });
    ASSERT_NE(enumDecl, nullptr);

    auto *boolMember = ark::es2panda::lsp::GetEnumMemberByName(enumDecl, "QBOOL4");
    auto *nullMember = ark::es2panda::lsp::GetEnumMemberByName(enumDecl, "QNULL4");
    auto *undefMember = ark::es2panda::lsp::GetEnumMemberByName(enumDecl, "QUNDEF4");
    auto *bigMember = ark::es2panda::lsp::GetEnumMemberByName(enumDecl, "QBIG4");
    ASSERT_NE(boolMember, nullptr);
    ASSERT_NE(nullMember, nullptr);
    ASSERT_NE(undefMember, nullptr);
    ASSERT_NE(bigMember, nullptr);
    auto boolParts = ark::es2panda::lsp::CreateDisplayForEnumMember(boolMember);
    auto nullParts = ark::es2panda::lsp::CreateDisplayForEnumMember(nullMember);
    auto undefParts = ark::es2panda::lsp::CreateDisplayForEnumMember(undefMember);
    auto bigParts = ark::es2panda::lsp::CreateDisplayForEnumMember(bigMember);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(boolParts, MakeParts(EnumDisplayTail("QidEx4", "QBOOL4", "true")));
    EXPECT_EQ(nullParts, MakeParts(EnumDisplayTail("QidEx4", "QNULL4", "null")));
    EXPECT_EQ(undefParts, MakeParts(EnumDisplayTail("QidEx4", "QUNDEF4", "undefined")));
    EXPECT_EQ(bigParts, MakeParts(EnumDisplayTail("QidEx4", "QBIG4", "1")));
}

// PC-012: hover kindModifiers across modifier combinations. ModifiersToString emits
// flags in its fixed order (static, async, public, protected, private, ..., abstract,
// ..., override), so combined flags render in that order regardless of source order.
// Note the implicit "public" flag: even explicitly private members render without a
// "public" component, while unwritten access defaults to public.
TEST_F(QuickInfoDisplayMatrixTests, ModifierCombinationMatrix)
{
    const std::string source = R"(abstract class QidBase12 {
    po12(): void {}
}
abstract class QidAbs12 {
    protected abstract pab12(): number;
}
class QidMods12 extends QidBase12 {
    private static readonly psr12: number = 1;
    public static psm12(): void {}
    override po12(): void {}
    async pam12(): Promise<void> {
    }
}
let qm12: QidMods12 = new QidMods12();)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qid_modifiers.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto psrPos = source.find("psr12");
    auto pabPos = source.find("pab12");
    auto psmPos = source.find("psm12");
    auto poPos = source.find("override po12") + std::string("override ").size();
    auto pamPos = source.find("pam12");
    ASSERT_NE(psrPos, std::string::npos);
    ASSERT_NE(pabPos, std::string::npos);
    ASSERT_NE(psmPos, std::string::npos);
    ASSERT_NE(pamPos, std::string::npos);

    auto psrInfo = lspApi->getQuickInfoAtPosition("qid_modifiers.ets", ctx, psrPos);
    auto pabInfo = lspApi->getQuickInfoAtPosition("qid_modifiers.ets", ctx, pabPos);
    auto psmInfo = lspApi->getQuickInfoAtPosition("qid_modifiers.ets", ctx, psmPos);
    auto poInfo = lspApi->getQuickInfoAtPosition("qid_modifiers.ets", ctx, poPos);
    auto pamInfo = lspApi->getQuickInfoAtPosition("qid_modifiers.ets", ctx, pamPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(psrInfo.GetKindModifiers(), "static private readonly");
    EXPECT_EQ(psrInfo.GetKind(), "property");
    EXPECT_EQ(pabInfo.GetKindModifiers(), "protected abstract");
    EXPECT_EQ(pabInfo.GetKind(), "method");
    EXPECT_EQ(psmInfo.GetKindModifiers(), "static public");
    EXPECT_EQ(psmInfo.GetKind(), "method");
    EXPECT_EQ(poInfo.GetKindModifiers(), "public override");
    EXPECT_EQ(poInfo.GetKind(), "method");
    EXPECT_EQ(pamInfo.GetKindModifiers(), "async public");
    EXPECT_EQ(pamInfo.GetKind(), "method");
}

// PC-013: class property hover display matrix. The className-prefixed shape is
// [Class . name : type]; an optional property renders the lowered union with undefined,
// unannotated properties render the literal-name type and a new-class-instance value
// renders the constructed class name.
TEST_F(QuickInfoDisplayMatrixTests, ClassPropertyDisplayMatrix)
{
    const std::string source = R"(class QidOther13 {
    v13: number = 0;
}
class QidProps13 {
    opt13?: number;
    static stat13: number = 1;
    readonly ro13: number = 1;
    private priv13: number = 1;
    union13: string | number = "u";
    cls13: QidOther13 = new QidOther13();
    litNum13 = 7;
    litStr13 = "t";
    newExpr13 = new QidOther13();
}
let qp13: QidProps13 = new QidProps13();)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qid_properties.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto optInfo = PropertyQuickInfo(lspApi, ctx, source, "opt13");
    auto statInfo = PropertyQuickInfo(lspApi, ctx, source, "stat13");
    auto roInfo = PropertyQuickInfo(lspApi, ctx, source, "ro13");
    auto privInfo = PropertyQuickInfo(lspApi, ctx, source, "priv13");
    auto unionInfo = PropertyQuickInfo(lspApi, ctx, source, "union13");
    auto clsInfo = PropertyQuickInfo(lspApi, ctx, source, "cls13");
    auto litNumInfo = PropertyQuickInfo(lspApi, ctx, source, "litNum13");
    auto litStrInfo = PropertyQuickInfo(lspApi, ctx, source, "litStr13");
    auto newExprInfo = PropertyQuickInfo(lspApi, ctx, source, "newExpr13");
    initializer.DestroyContext(ctx);

    EXPECT_EQ(optInfo.GetDisplayParts(), MakeParts(PropertyDisplayHead("opt13", "number | undefined")));
    EXPECT_EQ(statInfo.GetDisplayParts(), MakeParts(PropertyDisplayHead("stat13", "number")));
    EXPECT_EQ(roInfo.GetDisplayParts(), MakeParts(PropertyDisplayHead("ro13", "number")));
    EXPECT_EQ(privInfo.GetDisplayParts(), MakeParts(PropertyDisplayHead("priv13", "number")));
    EXPECT_EQ(unionInfo.GetDisplayParts(), MakeParts(PropertyDisplayHead("union13", "string | number")));
    EXPECT_EQ(clsInfo.GetDisplayParts(), MakeParts(PropertyDisplayHead("cls13", "QidOther13")));
    EXPECT_EQ(litNumInfo.GetDisplayParts(), MakeParts(PropertyDisplayHead("litNum13", "Number")));
    EXPECT_EQ(litStrInfo.GetDisplayParts(), MakeParts(PropertyDisplayHead("litStr13", "String")));
    EXPECT_EQ(newExprInfo.GetDisplayParts(), MakeParts(PropertyDisplayHead("newExpr13", "QidOther13")));

    EXPECT_EQ(statInfo.GetKindModifiers(), "static public");
    EXPECT_EQ(roInfo.GetKindModifiers(), "public readonly");
    EXPECT_EQ(privInfo.GetKindModifiers(), "private");
}

// PC-014: signature rendering through AppendSignature -> CreateDisplayOfFunctionParams
// and CreateDisplayOfReturnType: generic type parameters, union return, optional
// parameter (rendered without a marker), class-typed parameter and the constructor
// hover. The constructor routes to the regular-method display because the public
// chain feeds "public constructor" as the kind modifier, and the checker-derived
// return type of a constructor renders "undefined".
TEST_F(QuickInfoDisplayMatrixTests, SignatureDisplayMatrix)
{
    const std::string source = R"(class QidOther14 {
    n14: number = 0;
}
class QidSig14 {
    v14: number = 0;
    constructor(c14: number) {
        this.v14 = c14;
    }
    generic14<T>(t14: T): T {
        return t14;
    }
    unionRet14(u14: number): string | number {
        return u14;
    }
    optParam14(a14: number, b14?: number): number {
        return a14;
    }
    noParam14(): void {}
    clsParam14(p14: QidOther14): QidOther14 {
        return p14;
    }
}
let qs14: QidSig14 = new QidSig14(1);)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qid_signature.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    ExpectSignatureDisplayShapes(initializer, ctx, source);
}

// PC-014: a namespace-qualified return type renders through the TSQualifiedName arm
// of CreateDisplayOfReturnType as the full dotted type name.
TEST_F(QuickInfoDisplayMatrixTests, QualifiedReturnTypeDisplay)
{
    const std::string source = R"(namespace QidNS15 {
    export class QInner15 {
        n15: number = 0;
    }
}
class QidUser15 {
    q15(): QidNS15.QInner15 {
        return new QidNS15.QInner15();
    }
}
let qu15: QidUser15 = new QidUser15();)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qid_qualified_return.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    auto methodPos = source.find("q15");
    ASSERT_NE(methodPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition("qid_qualified_return.ets", ctx, methodPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "method");
    EXPECT_EQ(info.GetTextSpan().start, methodPos);
    EXPECT_EQ(info.GetTextSpan().length, std::string("q15").size());
    EXPECT_EQ(info.GetDisplayParts(), MakeParts({{"QidUser15", "className"},
                                                 {".", "punctuation"},
                                                 {"q15", "functionName"},
                                                 {"(", "punctuation"},
                                                 {")", "punctuation"},
                                                 {":", "punctuation"},
                                                 {" ", "space"},
                                                 {"QidNS15.QInner15", "returnType"}}));
}

}  // namespace
