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

class LspQuickInfo4Tests : public LSPAPITests {};

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

// A variable annotated with a union of every numeric/boolean primitive keyword
// expands each constituent through PrimitiveTypeToName in the hover display.
// The initializer comes from a byte-typed function because a direct literal is
// ambiguous across the numeric union members (ESE101680).
TEST_F(LspQuickInfo4Tests, WidePrimitiveUnionVariableDisplay)
{
    const std::string source =
        R"(function wpuSrc4(): byte {
    return 0;
}
let wpu4: boolean | byte | short | int | long | float | double | char = wpuSrc4();)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_wide_union.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    auto markerPos = source.find("wpu4");
    ASSERT_NE(markerPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition("qi4_wide_union.ets", ctx, markerPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "property");
    EXPECT_EQ(info.GetKindModifiers(), "static public");
    EXPECT_EQ(info.GetFileName(), "qi4_wide_union.ets");
    EXPECT_EQ(info.GetTextSpan().start, markerPos);
    EXPECT_EQ(info.GetTextSpan().length, std::string("wpu4").size());
    std::vector<SymbolDisplayPart> expected =
        MakeParts({{"let", "keyword"},
                   {" ", "space"},
                   {"wpu4", "property"},
                   {":", "punctuation"},
                   {" ", "space"},
                   {"boolean | byte | short | int | long | float | double | char", "typeName"}});
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

// A union of string literal types keeps its constituent literal-type nodes, so
// GetNameForLiteralTypeNode renders both members through the ETSStringLiteralType arm.
TEST_F(LspQuickInfo4Tests, StringLiteralUnionVariableDisplay)
{
    const std::string source = R"(let slu4: "on4" | "off4" = "on4";)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_str_lit_union.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    auto markerPos = source.find("slu4");
    ASSERT_NE(markerPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition("qi4_str_lit_union.ets", ctx, markerPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "property");
    EXPECT_EQ(info.GetTextSpan().start, markerPos);
    EXPECT_EQ(info.GetTextSpan().length, std::string("slu4").size());
    std::vector<SymbolDisplayPart> expected = MakeParts({{"let", "keyword"},
                                                         {" ", "space"},
                                                         {"slu4", "property"},
                                                         {":", "punctuation"},
                                                         {" ", "space"},
                                                         {"String | String", "typeName"}});
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

// Const scalar initializers without a type annotation render their literal
// value names through GetNameForLiteralTypeNode with the kind-modifier flag.
TEST_F(LspQuickInfo4Tests, ConstScalarValuesShowKindModifierNames)
{
    const std::string source = R"(const cvb4 = true;
const cvn4 = null;
const cvu4 = undefined;
const cvt4 = `tpl4`;
const cnn4 = NaN;
const cbg4 = 1n;)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_scalar_consts.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto mBool = source.find("cvb4");
    auto mNull = source.find("cvn4");
    auto mUndef = source.find("cvu4");
    auto mTpl = source.find("cvt4");
    auto mNan = source.find("cnn4");
    auto mBig = source.find("cbg4");
    ASSERT_NE(mBool, std::string::npos);
    ASSERT_NE(mNull, std::string::npos);
    ASSERT_NE(mUndef, std::string::npos);
    ASSERT_NE(mTpl, std::string::npos);
    ASSERT_NE(mNan, std::string::npos);
    ASSERT_NE(mBig, std::string::npos);

    auto boolInfo = lspApi->getQuickInfoAtPosition("qi4_scalar_consts.ets", ctx, mBool);
    auto nullInfo = lspApi->getQuickInfoAtPosition("qi4_scalar_consts.ets", ctx, mNull);
    auto undefInfo = lspApi->getQuickInfoAtPosition("qi4_scalar_consts.ets", ctx, mUndef);
    auto tplInfo = lspApi->getQuickInfoAtPosition("qi4_scalar_consts.ets", ctx, mTpl);
    auto nanInfo = lspApi->getQuickInfoAtPosition("qi4_scalar_consts.ets", ctx, mNan);
    auto bigInfo = lspApi->getQuickInfoAtPosition("qi4_scalar_consts.ets", ctx, mBig);
    initializer.DestroyContext(ctx);

    auto expectedBool = MakeParts({{"const", "keyword"},
                                   {" ", "space"},
                                   {"cvb4", "property"},
                                   {":", "punctuation"},
                                   {" ", "space"},
                                   {"true", "typeName"}});
    auto expectedNull = MakeParts({{"const", "keyword"},
                                   {" ", "space"},
                                   {"cvn4", "property"},
                                   {":", "punctuation"},
                                   {" ", "space"},
                                   {"null", "typeName"}});

    EXPECT_EQ(boolInfo.GetDisplayParts(), expectedBool);
    EXPECT_EQ(nullInfo.GetDisplayParts(), expectedNull);
    // Neither undefined nor template-literal initializers keep a dedicated
    // literal arm: the checker lowers the template value to a string literal,
    // which EscapeJsonString renders with its quotes.
    EXPECT_TRUE(DisplayHasText(undefInfo.GetDisplayParts(), "undefined"));
    EXPECT_TRUE(DisplayHasText(tplInfo.GetDisplayParts(), "\"tpl4\""));
    // The NaN identifier is recognized and reported as Number, and a bigint
    // initializer keeps its BigIntLiteral arm ("Bigint").
    EXPECT_TRUE(DisplayHasText(nanInfo.GetDisplayParts(), "Number"));
    EXPECT_TRUE(DisplayHasText(bigInfo.GetDisplayParts(), "Bigint"));
}

// Every JSON escape branch of EscapeJsonString fires when a const string
// initializer contains quote, backslash, newline, carriage return and tab.
TEST_F(LspQuickInfo4Tests, JsonEscapeConstStringValue)
{
    const std::string source = R"(const cej4 = "a4\"b4\\c4\nd4\re4\tf4";)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_json_escape.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    auto markerPos = source.find("cej4");
    ASSERT_NE(markerPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition("qi4_json_escape.ets", ctx, markerPos);
    initializer.DestroyContext(ctx);

    EXPECT_TRUE(DisplayHasText(info.GetDisplayParts(), "const"));
    EXPECT_TRUE(DisplayHasText(info.GetDisplayParts(), "cej4"));
    // The decoded string value is rendered back through EscapeJsonString.
    EXPECT_TRUE(DisplayHasText(info.GetDisplayParts(), "\"a4\\\"b4\\\\c4\\nd4\\re4\\tf4\""));
}

// A JSDoc tag with no trailing whitespace or text takes the no-whitespace
// branch of TryStartTag and is flushed as a tag with empty text.
TEST_F(LspQuickInfo4Tests, BareDeprecatedTagWithoutTextIsCaptured)
{
    const std::string source = R"(/**
 * Legacy widget docs.
 * @deprecated
 */
function legacyFn4(): void {})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_bare_tag.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    auto markerPos = source.find("legacyFn4");
    ASSERT_NE(markerPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition("qi4_bare_tag.ets", ctx, markerPos);
    initializer.DestroyContext(ctx);

    std::vector<DocTagInfo> expectedTags;
    expectedTags.emplace_back("deprecated", "", 0);
    EXPECT_EQ(info.GetTags(), expectedTags);
    ASSERT_FALSE(info.GetDocument().empty());
    EXPECT_EQ(info.GetDocument().front().GetText(), "Legacy widget docs.");
    EXPECT_EQ(info.GetKind(), "method");
    EXPECT_EQ(info.GetTextSpan().start, markerPos);
    EXPECT_EQ(info.GetTextSpan().length, std::string("legacyFn4").size());
}

// A JSDoc body line whose normalized content is exactly "/" is dropped by the
// NormalizeJsdocLineView guard and acts as a paragraph separator.
TEST_F(LspQuickInfo4Tests, JsdocSlashOnlyLineActsAsSeparator)
{
    const std::string source = R"(/**
 * Widget docs.
 * /
 * More docs.
 */
function slashFn4(): void {})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_slash_line.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    auto markerPos = source.find("slashFn4");
    ASSERT_NE(markerPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition("qi4_slash_line.ets", ctx, markerPos);
    initializer.DestroyContext(ctx);

    ASSERT_FALSE(info.GetDocument().empty());
    EXPECT_EQ(info.GetDocument().front().GetText(), "Widget docs.\n\nMore docs.");
    EXPECT_TRUE(info.GetTags().empty());
}

// A plain /* */ comment above a declaration produces no hover documentation:
// the compiler only attaches /** JSDoc blocks, so ParseJsdocToDocumentationAndTags
// receives an empty raw document for plain block comments. This pins that
// contract; the hover itself stays fully functional.
TEST_F(LspQuickInfo4Tests, PlainBlockCommentYieldsNoDocumentation)
{
    const std::string source = R"(/* Plain block note. */
function pbcFn4(): void {})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_plain_block.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    auto markerPos = source.find("pbcFn4");
    ASSERT_NE(markerPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition("qi4_plain_block.ets", ctx, markerPos);
    initializer.DestroyContext(ctx);

    EXPECT_TRUE(info.GetDocument().empty());
    EXPECT_TRUE(info.GetTags().empty());
    EXPECT_EQ(info.GetKind(), "method");
    EXPECT_EQ(info.GetTextSpan().start, markerPos);
    EXPECT_EQ(info.GetTextSpan().length, std::string("pbcFn4").size());
}

// Hovering a type alias at its declaration site dispatches to
// CreateDisplayForTypeAlias through the public quick-info entry.
TEST_F(LspQuickInfo4Tests, AliasDeclSiteShowsExpandedAliasDisplay)
{
    const std::string source = R"(type tnum4 = number;
let tn4v: tnum4 = 1;)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_alias_decl.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    auto declPos = source.find("tnum4");
    ASSERT_NE(declPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition("qi4_alias_decl.ets", ctx, declPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetTextSpan().start, declPos);
    EXPECT_EQ(info.GetTextSpan().length, std::string("tnum4").size());
    std::vector<SymbolDisplayPart> expected = MakeParts({{"type", "keyword"},
                                                         {" ", "space"},
                                                         {"tnum4", "className"},
                                                         {" ", "space"},
                                                         {"=", "operator"},
                                                         {" ", "space"},
                                                         {"number", "typeName"}});
    EXPECT_EQ(info.GetDisplayParts(), expected);
}
// Hovering an enum name dispatches to the enum display both at its
// declaration site and at use sites (type annotation and qualified value).
TEST_F(LspQuickInfo4Tests, EnumDeclSiteShowsEnumDisplay)
{
    const std::string source = R"(enum Color4 {
    RED4,
    GREEN4
}
let color4: Color4 = Color4.RED4;)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_enum_decl.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto declPos = source.find("Color4");
    auto typeUsePos = source.find(": Color4");
    auto valueUsePos = source.find("= Color4");
    ASSERT_NE(declPos, std::string::npos);
    ASSERT_NE(typeUsePos, std::string::npos);
    ASSERT_NE(valueUsePos, std::string::npos);

    auto declInfo = lspApi->getQuickInfoAtPosition("qi4_enum_decl.ets", ctx, declPos);
    auto typeUseInfo = lspApi->getQuickInfoAtPosition("qi4_enum_decl.ets", ctx, typeUsePos + 2);
    auto valueUseInfo = lspApi->getQuickInfoAtPosition("qi4_enum_decl.ets", ctx, valueUsePos + 2);
    initializer.DestroyContext(ctx);

    // The ': ' / '= ' prefixes precede the enum name at both use sites.
    constexpr size_t usePrefixLength = 2;
    auto expected = MakeParts({{"enum", "keyword"}, {" ", "space"}, {"Color4", "enumName"}});
    EXPECT_EQ(declInfo.GetTextSpan().start, declPos);
    EXPECT_EQ(declInfo.GetTextSpan().length, std::string("Color4").size());
    EXPECT_EQ(declInfo.GetDisplayParts(), expected);

    // Use sites resolve to the same declaration display.
    EXPECT_EQ(typeUseInfo.GetDisplayParts(), expected);
    EXPECT_EQ(typeUseInfo.GetTextSpan().start, typeUsePos + usePrefixLength);
    EXPECT_EQ(valueUseInfo.GetDisplayParts(), expected);
    EXPECT_EQ(valueUseInfo.GetTextSpan().start, valueUsePos + usePrefixLength);

    // In a PARSED-only context the enum name does not yet resolve to any
    // quick-info target (pinned current behavior): the identifier-to-
    // declaration mapping used by hover only becomes available after checking,
    // where the enum has been transformed into its class definition form.
    auto *parsedCtx = initializer.CreateContext("qi4_enum_decl_parsed.ets", ES2PANDA_STATE_PARSED, source.c_str());
    ASSERT_EQ(ContextState(parsedCtx), ES2PANDA_STATE_PARSED);
    auto parsedInfo = lspApi->getQuickInfoAtPosition("qi4_enum_decl_parsed.ets", parsedCtx, declPos);
    initializer.DestroyContext(parsedCtx);
    EXPECT_EQ(parsedInfo, QuickInfo());
}

// A char-literal enum member initializer renders its character through the
// CHAR_LITERAL arm of CreateDisplayForEnumMember.
TEST_F(LspQuickInfo4Tests, EnumMemberCharLiteralInitDisplay)
{
    const std::string source = R"(enum Level4 {
    TAG4 = 'T'
}
let level4 = Level4.TAG4;)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_enum_char.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    auto memberPos = source.find("TAG4");
    ASSERT_NE(memberPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition("qi4_enum_char.ets", ctx, memberPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "enum member");
    EXPECT_EQ(info.GetTextSpan().start, memberPos);
    EXPECT_EQ(info.GetTextSpan().length, std::string("TAG4").size());
    // The parser surfaces the single-quoted char initializer as a string
    // literal node, so the value renders as a quoted text part.
    auto expected = MakeParts({{"Level4", "enumName"},
                               {".", "punctuation"},
                               {"TAG4", "enumMember"},
                               {" ", "space"},
                               {"=", "operator"},
                               {" ", "space"},
                               {"\"", "punctuation"},
                               {"T", "text"},
                               {"\"", "punctuation"}});
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

// An abstract method hover renders the class prefix, method name and its
// declared signature even though the body is empty.
TEST_F(LspQuickInfo4Tests, AbstractMethodHoverSkipsSignature)
{
    const std::string source = R"(abstract class ShapeBase4 {
    abstract area4(): number;
})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_abstract.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    auto methodPos = source.find("area4");
    ASSERT_NE(methodPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition("qi4_abstract.ets", ctx, methodPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "method");
    EXPECT_EQ(info.GetTextSpan().start, methodPos);
    EXPECT_EQ(info.GetTextSpan().length, std::string("area4").size());
    auto expected = MakeParts({{"ShapeBase4", "className"},
                               {".", "punctuation"},
                               {"area4", "functionName"},
                               {"(", "punctuation"},
                               {")", "punctuation"},
                               {":", "punctuation"},
                               {" ", "space"},
                               {"number", "returnType"}});
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

// Hovering a generic function's type-parameter declaration and its parameter
// name exercises the TS_TYPE_PARAMETER and ETS_PARAMETER_EXPRESSION dispatch arms.
TEST_F(LspQuickInfo4Tests, GenericTypeParamAndParamHover)
{
    const std::string source = R"(function gid4<T>(x4: T): T {
    return x4;
})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_generic_parts.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto typeParamPos = source.find("<T>");
    auto paramPos = source.find("(x4");
    auto paramUsePos = source.find("return x4");
    ASSERT_NE(typeParamPos, std::string::npos);
    ASSERT_NE(paramPos, std::string::npos);
    ASSERT_NE(paramUsePos, std::string::npos);
    auto typeParamInfo = lspApi->getQuickInfoAtPosition("qi4_generic_parts.ets", ctx, typeParamPos + 1);
    auto paramInfo = lspApi->getQuickInfoAtPosition("qi4_generic_parts.ets", ctx, paramPos + 1);
    auto paramUseInfo = lspApi->getQuickInfoAtPosition("qi4_generic_parts.ets", ctx, paramUsePos + 7);
    initializer.DestroyContext(ctx);

    // Hovering the type-parameter declaration resolves to its TSTypeParameter
    // node; the display spells out the owning function signature.
    EXPECT_EQ(typeParamInfo.GetTextSpan().start, typeParamPos + 1);
    EXPECT_EQ(typeParamInfo.GetTextSpan().length, std::size_t {1});
    auto expectedTp = MakeParts({{"T", "typeParameter"},
                                 {" ", "space"},
                                 {"in", "keyword"},
                                 {" ", "space"},
                                 {"gid4", "functionName"},
                                 {"<", "punctuation"},
                                 {"T", "typeParameter"},
                                 {">", "punctuation"},
                                 {"(", "punctuation"},
                                 {"x4", "functionParameter"},
                                 {":", "punctuation"},
                                 {" ", "space"},
                                 {"T", "typeParameter"},
                                 {")", "punctuation"},
                                 {":", "punctuation"},
                                 {" ", "space"},
                                 {"T", "returnType"}});
    EXPECT_EQ(typeParamInfo.GetDisplayParts(), expectedTp);

    // The parameter name at its declaration site yields no quick info (pinned
    // current behavior), while hovering the usage inside the body resolves to
    // the ETS_PARAMETER_EXPRESSION declaration node.
    EXPECT_EQ(paramInfo, QuickInfo());
    // 'return ' spans seven characters up to the x4 usage token inside the body.
    constexpr size_t returnKeywordPrefixLength = 7;
    EXPECT_EQ(paramUseInfo.GetKind(), "parameter");
    EXPECT_EQ(paramUseInfo.GetTextSpan().start, paramUsePos + returnKeywordPrefixLength);
    EXPECT_EQ(paramUseInfo.GetTextSpan().length, std::string("x4").size());
    auto expectedParam =
        MakeParts({{"x4", "functionParameter"}, {":", "punctuation"}, {" ", "space"}, {"T", "typeName"}});
    EXPECT_EQ(paramUseInfo.GetDisplayParts(), expectedParam);
}

// An interface method with its own type parameter renders the interface
// prefix, member name, type parameters and return type.
TEST_F(LspQuickInfo4Tests, GenericInterfaceMethodShowsTypeParams)
{
    const std::string source = R"(interface GIS4 {
    gidm4<T>(gx4: T): number;
})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_generic_iface.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    auto methodPos = source.find("gidm4");
    ASSERT_NE(methodPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition("qi4_generic_iface.ets", ctx, methodPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetTextSpan().start, methodPos);
    EXPECT_EQ(info.GetTextSpan().length, std::string("gidm4").size());
    auto expected = MakeParts({{"GIS4", "interface"},
                               {".", "punctuation"},
                               {"gidm4", "property"},
                               {"<", "punctuation"},
                               {"T", "typeParameter"},
                               {">", "punctuation"},
                               {":", "punctuation"},
                               {" ", "space"},
                               {"number", "returnType"}});
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

// Hovering an object-literal key whose contextual interface declares a method
// of the same name maps the hover onto the interface method definition.
TEST_F(LspQuickInfo4Tests, ContextualInterfaceMethodMatchViaObjectLiteral)
{
    const std::string source = R"(interface CIM4 {
    fld4: number;
    cim4(x4: number): number;
}
class CIMH4 {
    h4: CIM4 = { fld4: 1, cim4: (x4: number): number => { return 1; } };
})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_ctx_iface.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    auto litKeyPos = source.rfind("cim4");
    ASSERT_NE(litKeyPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition("qi4_ctx_iface.ets", ctx, litKeyPos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetTextSpan().start, litKeyPos);
    EXPECT_EQ(info.GetTextSpan().length, std::string("cim4").size());
    auto expected = MakeParts({{"CIM4", "interface"},
                               {".", "punctuation"},
                               {"cim4", "property"},
                               {":", "punctuation"},
                               {" ", "space"},
                               {"number", "returnType"}});
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

// Hovering inside a template-literal value of an object-literal property walks
// the TEMPLATE_ELEMENT grandparent path into the contextual-type flow and maps
// the hover onto the interface's synthesized accessor for that key.
TEST_F(LspQuickInfo4Tests, TemplateLiteralValueResolvesObjectLiteralKey)
{
    const std::string source = R"(interface CTI4 {
    tpl4: string;
}
class CTCH4 {
    tc4: CTI4 = { tpl4: `tv${1}` };
})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi4_tpl_value.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();
    auto contentPos = source.find("tv${");
    ASSERT_NE(contentPos, std::string::npos);
    auto info = lspApi->getQuickInfoAtPosition("qi4_tpl_value.ets", ctx, contentPos);
    initializer.DestroyContext(ctx);

    // The touched token is the template element whose span starts at the
    // backtick and covers the whole literal; the contextual interface resolves
    // the containing property to its accessor display.
    EXPECT_EQ(info.GetKind(), "get");
    EXPECT_EQ(info.GetFileName(), "qi4_tpl_value.ets");
    EXPECT_EQ(info.GetTextSpan().start, contentPos - 1);
    EXPECT_EQ(info.GetTextSpan().length, std::string("`tv${1}`").size());
    auto expected = MakeParts({{"CTI4", "interface"},
                               {".", "punctuation"},
                               {"tpl4", "property"},
                               {":", "punctuation"},
                               {" ", "space"},
                               {"string", "returnType"}});
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

// Contract guard: a null context passed straight to the Impl entry produces a
// default QuickInfo. The public wrapper dereferences the context before the
// Impl call, so this guard is only exercisable at the helper seam.
TEST_F(LspQuickInfo4Tests, NullContextImplReturnsEmptyQuickInfo)
{
    auto info = ark::es2panda::lsp::GetQuickInfoAtPositionImpl(nullptr, 0, "qi4_null_ctx.ets");
    EXPECT_EQ(info, QuickInfo());
}

}  // namespace
