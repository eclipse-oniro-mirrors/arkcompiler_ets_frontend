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
#include <gtest/gtest.h>
#include <optional>

namespace {
using ark::es2panda::lsp::Initializer;

class LspQuickInfo2Tests : public LSPAPITests {};

std::optional<size_t> FindMarker(const std::string &source, const std::string &marker)
{
    auto pos = source.find(marker);
    if (pos == std::string::npos) {
        ADD_FAILURE() << "marker not found: " << marker;
        return std::nullopt;
    }
    return pos;
}

TEST_F(LspQuickInfo2Tests, HoverAnnotatedVariableDeclaration)
{
    const std::string source = "let annotated: number = 1;";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_var.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto marker = FindMarker(source, "annotated:");
    ASSERT_NE(marker, std::nullopt);
    auto info = lspApi->getQuickInfoAtPosition("qi2_var.ets", ctx, *marker);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "property");
    EXPECT_EQ(info.GetKindModifiers(), "static public");
    EXPECT_EQ(info.GetFileName(), "qi2_var.ets");
    EXPECT_EQ(info.GetTextSpan().start, *marker);
    EXPECT_EQ(info.GetTextSpan().length, std::string("annotated").size());
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("let", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("annotated", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "typeName");
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

TEST_F(LspQuickInfo2Tests, HoverVariableUsageShowsDeclaration)
{
    const std::string source = "let annotated: number = 1;\nlet usage = annotated;";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_var_usage.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto marker = FindMarker(source, "annotated;");
    ASSERT_NE(marker, std::nullopt);
    auto info = lspApi->getQuickInfoAtPosition("qi2_var_usage.ets", ctx, *marker);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "property");
    EXPECT_EQ(info.GetKindModifiers(), "static public");
    EXPECT_EQ(info.GetFileName(), "qi2_var_usage.ets");
    EXPECT_EQ(info.GetTextSpan().start, *marker);
    EXPECT_EQ(info.GetTextSpan().length, std::string("annotated").size());
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("let", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("annotated", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "typeName");
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

TEST_F(LspQuickInfo2Tests, HoverInferredTypeVariable)
{
    const std::string source = "let inferred = 2.5;";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_inferred.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto marker = FindMarker(source, "inferred =");
    ASSERT_NE(marker, std::nullopt);
    auto info = lspApi->getQuickInfoAtPosition("qi2_inferred.ets", ctx, *marker);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "property");
    EXPECT_EQ(info.GetKindModifiers(), "static public");
    EXPECT_EQ(info.GetFileName(), "qi2_inferred.ets");
    EXPECT_EQ(info.GetTextSpan().start, *marker);
    EXPECT_EQ(info.GetTextSpan().length, std::string("inferred").size());
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("let", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("inferred", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("Number", "typeName");
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

TEST_F(LspQuickInfo2Tests, HoverFunctionDeclaration)
{
    const std::string source = R"(function add(a: number, b: number): number {
    return a + b;
})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_func.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto marker = FindMarker(source, "add(a");
    ASSERT_NE(marker, std::nullopt);
    auto info = lspApi->getQuickInfoAtPosition("qi2_func.ets", ctx, *marker);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "method");
    EXPECT_EQ(info.GetKindModifiers(), "static public");
    EXPECT_EQ(info.GetFileName(), "qi2_func.ets");
    EXPECT_EQ(info.GetTextSpan().start, *marker);
    EXPECT_EQ(info.GetTextSpan().length, std::string("add").size());
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("function", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("add", "functionName");
    expected.emplace_back("(", "punctuation");
    expected.emplace_back("a", "functionParameter");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "typeParameter");
    expected.emplace_back(",", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("b", "functionParameter");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "typeParameter");
    expected.emplace_back(")", "punctuation");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "returnType");
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

TEST_F(LspQuickInfo2Tests, HoverFunctionCallSite)
{
    const std::string source = R"(function add(a: number, b: number): number {
    return a + b;
}
let total = add(1, 2);)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_func_call.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto marker = FindMarker(source, "add(1, 2)");
    ASSERT_NE(marker, std::nullopt);
    auto info = lspApi->getQuickInfoAtPosition("qi2_func_call.ets", ctx, *marker);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "method");
    EXPECT_EQ(info.GetKindModifiers(), "static public");
    EXPECT_EQ(info.GetFileName(), "qi2_func_call.ets");
    EXPECT_EQ(info.GetTextSpan().start, *marker);
    EXPECT_EQ(info.GetTextSpan().length, std::string("add").size());
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("function", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("add", "functionName");
    expected.emplace_back("(", "punctuation");
    expected.emplace_back("a", "functionParameter");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "typeParameter");
    expected.emplace_back(",", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("b", "functionParameter");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "typeParameter");
    expected.emplace_back(")", "punctuation");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "returnType");
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

std::string ClassMembersSource()
{
    return R"(class Parcel {
    public size: number = 0;
    public getSize(): number {
        return this.size;
    }
}
let parcel = new Parcel();
let memberSize = parcel.size;
let memberCall = parcel.getSize();)";
}

TEST_F(LspQuickInfo2Tests, HoverClassDeclarationAndProperty)
{
    const std::string source = ClassMembersSource();
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_class.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto mParcel = FindMarker(source, "Parcel {");
    auto mParcelCtor = FindMarker(source, "Parcel();");
    auto mSize = FindMarker(source, "size: number");
    ASSERT_NE(mParcel, std::nullopt);
    ASSERT_NE(mParcelCtor, std::nullopt);
    ASSERT_NE(mSize, std::nullopt);
    auto classDecl = lspApi->getQuickInfoAtPosition("qi2_class.ets", ctx, *mParcel);
    auto classUsage = lspApi->getQuickInfoAtPosition("qi2_class.ets", ctx, *mParcelCtor);
    auto propDecl = lspApi->getQuickInfoAtPosition("qi2_class.ets", ctx, *mSize);
    initializer.DestroyContext(ctx);

    std::vector<SymbolDisplayPart> expectedClass;
    expectedClass.emplace_back("class", "keyword");
    expectedClass.emplace_back(" ", "space");
    expectedClass.emplace_back("Parcel", "className");

    EXPECT_EQ(classDecl.GetKind(), "class");
    EXPECT_EQ(classDecl.GetKindModifiers(), "");
    EXPECT_EQ(classDecl.GetFileName(), "qi2_class.ets");
    EXPECT_EQ(classDecl.GetTextSpan().start, *mParcel);
    EXPECT_EQ(classDecl.GetTextSpan().length, std::string("Parcel").size());
    EXPECT_EQ(classDecl.GetDisplayParts(), expectedClass);

    EXPECT_EQ(classUsage.GetKind(), "class");
    EXPECT_EQ(classUsage.GetKindModifiers(), "");
    EXPECT_EQ(classUsage.GetTextSpan().start, *mParcelCtor);
    EXPECT_EQ(classUsage.GetTextSpan().length, std::string("Parcel").size());
    EXPECT_EQ(classUsage.GetDisplayParts(), expectedClass);

    std::vector<SymbolDisplayPart> expectedProp;
    expectedProp.emplace_back("Parcel", "className");
    expectedProp.emplace_back(".", "punctuation");
    expectedProp.emplace_back("size", "property");
    expectedProp.emplace_back(":", "punctuation");
    expectedProp.emplace_back(" ", "space");
    expectedProp.emplace_back("number", "typeName");

    EXPECT_EQ(propDecl.GetKind(), "property");
    EXPECT_EQ(propDecl.GetKindModifiers(), "public");
    EXPECT_EQ(propDecl.GetTextSpan().start, *mSize);
    EXPECT_EQ(propDecl.GetTextSpan().length, std::string("size").size());
    EXPECT_EQ(propDecl.GetDisplayParts(), expectedProp);
}

TEST_F(LspQuickInfo2Tests, HoverClassMethodMembers)
{
    const std::string source = ClassMembersSource();
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_class.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto mGetSize = FindMarker(source, "getSize():");
    auto mParcelGetSize = FindMarker(source, "parcel.getSize();");
    ASSERT_NE(mGetSize, std::nullopt);
    ASSERT_NE(mParcelGetSize, std::nullopt);
    auto methodDecl = lspApi->getQuickInfoAtPosition("qi2_class.ets", ctx, *mGetSize);
    auto methodUsage =
        lspApi->getQuickInfoAtPosition("qi2_class.ets", ctx, *mParcelGetSize + std::string("parcel.").size());
    initializer.DestroyContext(ctx);

    std::vector<SymbolDisplayPart> expectedMethod;
    expectedMethod.emplace_back("Parcel", "className");
    expectedMethod.emplace_back(".", "punctuation");
    expectedMethod.emplace_back("getSize", "functionName");
    expectedMethod.emplace_back("(", "punctuation");
    expectedMethod.emplace_back(")", "punctuation");
    expectedMethod.emplace_back(":", "punctuation");
    expectedMethod.emplace_back(" ", "space");
    expectedMethod.emplace_back("number", "returnType");

    EXPECT_EQ(methodDecl.GetKind(), "method");
    EXPECT_EQ(methodDecl.GetKindModifiers(), "public");
    EXPECT_EQ(methodDecl.GetTextSpan().start, *mGetSize);
    EXPECT_EQ(methodDecl.GetTextSpan().length, std::string("getSize").size());
    EXPECT_EQ(methodDecl.GetDisplayParts(), expectedMethod);

    EXPECT_EQ(methodUsage.GetKind(), "method");
    EXPECT_EQ(methodUsage.GetKindModifiers(), "public");
    EXPECT_EQ(methodUsage.GetTextSpan().start, *mParcelGetSize + std::string("parcel.").size());
    EXPECT_EQ(methodUsage.GetTextSpan().length, std::string("getSize").size());
    EXPECT_EQ(methodUsage.GetDisplayParts(), expectedMethod);
}

TEST_F(LspQuickInfo2Tests, HoverInterfaceDeclaration)
{
    const std::string source = R"(interface Shape {
    area: number;
})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_iface.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto marker = FindMarker(source, "Shape {");
    ASSERT_NE(marker, std::nullopt);
    auto info = lspApi->getQuickInfoAtPosition("qi2_iface.ets", ctx, *marker);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "interface");
    EXPECT_EQ(info.GetKindModifiers(), "static public");
    EXPECT_EQ(info.GetFileName(), "qi2_iface.ets");
    EXPECT_EQ(info.GetTextSpan().start, *marker);
    EXPECT_EQ(info.GetTextSpan().length, std::string("Shape").size());
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("interface", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("Shape", "className");
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

TEST_F(LspQuickInfo2Tests, HoverUnionTypeVariable)
{
    const std::string source = "let unionValue: string | number = 1;";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_union.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto marker = FindMarker(source, "unionValue:");
    ASSERT_NE(marker, std::nullopt);
    auto info = lspApi->getQuickInfoAtPosition("qi2_union.ets", ctx, *marker);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "property");
    EXPECT_EQ(info.GetKindModifiers(), "static public");
    EXPECT_EQ(info.GetFileName(), "qi2_union.ets");
    EXPECT_EQ(info.GetTextSpan().start, *marker);
    EXPECT_EQ(info.GetTextSpan().length, std::string("unionValue").size());
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("let", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("unionValue", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("string | number", "typeName");
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

TEST_F(LspQuickInfo2Tests, HoverFunctionTypeAliasVariable)
{
    const std::string source = R"(type Handler = (input: string, count: number) => boolean;
let fnValue: Handler;)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_fn_type.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto marker = FindMarker(source, "fnValue:");
    ASSERT_NE(marker, std::nullopt);
    auto info = lspApi->getQuickInfoAtPosition("qi2_fn_type.ets", ctx, *marker);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "property");
    EXPECT_EQ(info.GetKindModifiers(), "static public");
    EXPECT_EQ(info.GetFileName(), "qi2_fn_type.ets");
    EXPECT_EQ(info.GetTextSpan().start, *marker);
    EXPECT_EQ(info.GetTextSpan().length, std::string("fnValue").size());
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("let", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("fnValue", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("Handler", "typeName");
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

TEST_F(LspQuickInfo2Tests, HoverGenericFunctionDeclaration)
{
    const std::string source = R"(function wrap<T>(value: T): T {
    return value;
})";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_generic.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto marker = FindMarker(source, "wrap<T>");
    ASSERT_NE(marker, std::nullopt);
    auto info = lspApi->getQuickInfoAtPosition("qi2_generic.ets", ctx, *marker);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "method");
    EXPECT_EQ(info.GetKindModifiers(), "static public");
    EXPECT_EQ(info.GetFileName(), "qi2_generic.ets");
    EXPECT_EQ(info.GetTextSpan().start, *marker);
    EXPECT_EQ(info.GetTextSpan().length, std::string("wrap").size());
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("function", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("wrap", "functionName");
    expected.emplace_back("<", "punctuation");
    expected.emplace_back("T", "typeParameter");
    expected.emplace_back(">", "punctuation");
    expected.emplace_back("(", "punctuation");
    expected.emplace_back("value", "functionParameter");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("T", "typeParameter");
    expected.emplace_back(")", "punctuation");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("T", "returnType");
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

TEST_F(LspQuickInfo2Tests, HoverLiteralTypeVariables)
{
    const std::string source = "let strLit = \"abc\";\nlet numLit = 42;";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_literal.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto mStrLit = FindMarker(source, "strLit =");
    auto mNumLit = FindMarker(source, "numLit =");
    ASSERT_NE(mStrLit, std::nullopt);
    ASSERT_NE(mNumLit, std::nullopt);
    auto strInfo = lspApi->getQuickInfoAtPosition("qi2_literal.ets", ctx, *mStrLit);
    auto numInfo = lspApi->getQuickInfoAtPosition("qi2_literal.ets", ctx, *mNumLit);
    initializer.DestroyContext(ctx);

    std::vector<SymbolDisplayPart> expectedStr;
    expectedStr.emplace_back("let", "keyword");
    expectedStr.emplace_back(" ", "space");
    expectedStr.emplace_back("strLit", "property");
    expectedStr.emplace_back(":", "punctuation");
    expectedStr.emplace_back(" ", "space");
    expectedStr.emplace_back("String", "typeName");

    EXPECT_EQ(strInfo.GetKind(), "property");
    EXPECT_EQ(strInfo.GetKindModifiers(), "static public");
    EXPECT_EQ(strInfo.GetFileName(), "qi2_literal.ets");
    EXPECT_EQ(strInfo.GetTextSpan().start, *mStrLit);
    EXPECT_EQ(strInfo.GetTextSpan().length, std::string("strLit").size());
    EXPECT_EQ(strInfo.GetDisplayParts(), expectedStr);

    std::vector<SymbolDisplayPart> expectedNum;
    expectedNum.emplace_back("let", "keyword");
    expectedNum.emplace_back(" ", "space");
    expectedNum.emplace_back("numLit", "property");
    expectedNum.emplace_back(":", "punctuation");
    expectedNum.emplace_back(" ", "space");
    expectedNum.emplace_back("Number", "typeName");

    EXPECT_EQ(numInfo.GetKind(), "property");
    EXPECT_EQ(numInfo.GetKindModifiers(), "static public");
    EXPECT_EQ(numInfo.GetTextSpan().start, *mNumLit);
    EXPECT_EQ(numInfo.GetTextSpan().length, std::string("numLit").size());
    EXPECT_EQ(numInfo.GetDisplayParts(), expectedNum);
}

TEST_F(LspQuickInfo2Tests, HoverEnumMember)
{
    const std::string source = R"(enum Direction {
    Up = 1,
    Down = 2
}
let heading: Direction = Direction.Up;)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_enum.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto mUp = FindMarker(source, "Up = 1");
    auto mDirUp = FindMarker(source, "Direction.Up");
    ASSERT_NE(mUp, std::nullopt);
    ASSERT_NE(mDirUp, std::nullopt);
    auto memberDecl = lspApi->getQuickInfoAtPosition("qi2_enum.ets", ctx, *mUp);
    auto memberUsage = lspApi->getQuickInfoAtPosition("qi2_enum.ets", ctx, *mDirUp + std::string("Direction.").size());
    initializer.DestroyContext(ctx);

    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("Direction", "enumName");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("Up", "enumMember");
    expected.emplace_back(" ", "space");
    expected.emplace_back("=", "operator");
    expected.emplace_back(" ", "space");
    expected.emplace_back("1", "text");

    EXPECT_EQ(memberDecl.GetKind(), "enum member");
    EXPECT_EQ(memberDecl.GetKindModifiers(), "static public readonly");
    EXPECT_EQ(memberDecl.GetFileName(), "qi2_enum.ets");
    EXPECT_EQ(memberDecl.GetTextSpan().start, *mUp);
    EXPECT_EQ(memberDecl.GetTextSpan().length, std::string("Up").size());
    EXPECT_EQ(memberDecl.GetDisplayParts(), expected);

    EXPECT_EQ(memberUsage.GetKind(), "enum member");
    EXPECT_EQ(memberUsage.GetKindModifiers(), "static public readonly");
    EXPECT_EQ(memberUsage.GetTextSpan().start, *mDirUp + std::string("Direction.").size());
    EXPECT_EQ(memberUsage.GetTextSpan().length, std::string("Up").size());
    EXPECT_EQ(memberUsage.GetDisplayParts(), expected);
}

TEST_F(LspQuickInfo2Tests, HoverDeprecatedVariableShowsDeprecatedTag)
{
    const std::string source = R"(/** @deprecated use modernValue instead */
let legacyValue: number = 0;)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_deprecated.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto marker = FindMarker(source, "legacyValue:");
    ASSERT_NE(marker, std::nullopt);
    auto info = lspApi->getQuickInfoAtPosition("qi2_deprecated.ets", ctx, *marker);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "property");
    EXPECT_EQ(info.GetKindModifiers(), "static public");
    EXPECT_EQ(info.GetFileName(), "qi2_deprecated.ets");
    EXPECT_EQ(info.GetTextSpan().start, *marker);
    EXPECT_EQ(info.GetTextSpan().length, std::string("legacyValue").size());
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("let", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("legacyValue", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "typeName");
    EXPECT_EQ(info.GetDisplayParts(), expected);

    std::vector<DocTagInfo> expectedTags;
    expectedTags.emplace_back("deprecated", "use modernValue instead");
    EXPECT_EQ(info.GetTags(), expectedTags);
}

TEST_F(LspQuickInfo2Tests, HoverCommentStringOperatorReturnsEmpty)
{
    const std::string source = R"(// leading comment
let emptyProbe: number = 1 + 2;
let emptyStr = "hello";)";
    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext("qi2_empty.ets", ES2PANDA_STATE_CHECKED, source.c_str());
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto mLeading = FindMarker(source, "leading");
    auto mOp = FindMarker(source, "1 + 2");
    auto mHello = FindMarker(source, "hello");
    ASSERT_NE(mLeading, std::nullopt);
    ASSERT_NE(mOp, std::nullopt);
    ASSERT_NE(mHello, std::nullopt);
    auto commentInfo = lspApi->getQuickInfoAtPosition("qi2_empty.ets", ctx, *mLeading);
    auto operatorInfo = lspApi->getQuickInfoAtPosition("qi2_empty.ets", ctx, *mOp + 2);
    auto stringInfo = lspApi->getQuickInfoAtPosition("qi2_empty.ets", ctx, *mHello + 1);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(commentInfo, QuickInfo());
    EXPECT_EQ(operatorInfo, QuickInfo());
    EXPECT_EQ(stringInfo, QuickInfo());
}

TEST_F(LspQuickInfo2Tests, HoverImportAliasShowsOriginalDeclaration)
{
    std::vector<std::string> files = {"qi2_alias_export.ets", "qi2_alias_import.ets"};
    std::vector<std::string> texts = {R"(export function OriginalFunc(a: number): number {
    return a;
}
)",
                                      R"(import { OriginalFunc as RenamedFunc } from './qi2_alias_export';
let aliasResult = RenamedFunc(1);
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto aliasDecl = lspApi->getQuickInfoAtPosition(filePaths[1].c_str(), ctx, texts[1].find("RenamedFunc }"));
    auto aliasUsage = lspApi->getQuickInfoAtPosition(filePaths[1].c_str(), ctx, texts[1].find("RenamedFunc(1)"));
    initializer.DestroyContext(ctx);

    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("function", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("OriginalFunc", "functionName");
    expected.emplace_back("(", "punctuation");
    expected.emplace_back("a", "functionParameter");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "typeParameter");
    expected.emplace_back(")", "punctuation");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "returnType");

    EXPECT_EQ(aliasDecl.GetKind(), "method");
    EXPECT_EQ(aliasDecl.GetKindModifiers(), "static public export");
    EXPECT_EQ(aliasDecl.GetTextSpan().start, texts[1].find("RenamedFunc }"));
    EXPECT_EQ(aliasDecl.GetTextSpan().length, std::string("RenamedFunc").size());
    EXPECT_EQ(aliasDecl.GetDisplayParts(), expected);

    EXPECT_EQ(aliasUsage.GetKind(), "method");
    EXPECT_EQ(aliasUsage.GetKindModifiers(), "static public export");
    EXPECT_EQ(aliasUsage.GetTextSpan().start, texts[1].find("RenamedFunc(1)"));
    EXPECT_EQ(aliasUsage.GetTextSpan().length, std::string("RenamedFunc").size());
    EXPECT_EQ(aliasUsage.GetDisplayParts(), expected);
}

TEST_F(LspQuickInfo2Tests, HoverImportedInterfaceObjectLiteralAssignmentProperty)
{
    std::vector<std::string> files = {"qi2_config_lib.ets", "qi2_config_main.ets"};
    std::vector<std::string> texts = {R"(export interface Config {
    field: string;
})",
                                      R"(import { Config } from './qi2_config_lib';
let config: Config = { field: "v" };
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto pos = texts[1].find("field: \"v\"");
    // AST evidence: the contextual type of the object literal resolves to the
    // interface declaration inside the imported file, not to the
    // ETSImportDeclaration node itself.
    auto node = ark::es2panda::lsp::GetTokenForQuickInfo(ctx, pos);
    ASSERT_NE(node, nullptr);
    auto containingObjectNode = ark::es2panda::lsp::GetContainingObjectLiteralNode(node);
    ASSERT_NE(containingObjectNode, nullptr);
    auto contextualTypeNode = ark::es2panda::lsp::GetContextualTypeNode(containingObjectNode->Parent());
    ASSERT_NE(contextualTypeNode, nullptr);
    EXPECT_EQ(contextualTypeNode->Type(), ark::es2panda::ir::AstNodeType::TS_INTERFACE_DECLARATION);
    EXPECT_FALSE(contextualTypeNode->IsETSImportDeclaration());

    auto info = lspApi->getQuickInfoAtPosition(filePaths[1].c_str(), ctx, pos);
    initializer.DestroyContext(ctx);

    EXPECT_EQ(info.GetKind(), "get");
    EXPECT_EQ(info.GetKindModifiers(), "public abstract");
    EXPECT_EQ(info.GetFileName(), filePaths[1]);
    EXPECT_EQ(info.GetTextSpan().start, pos);
    EXPECT_EQ(info.GetTextSpan().length, std::string("field").size());
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("Config", "interface");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("field", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("string", "returnType");
    EXPECT_EQ(info.GetDisplayParts(), expected);
}

TEST_F(LspQuickInfo2Tests, HoverImportedClassObjectLiteralClassProperty)
{
    std::vector<std::string> files = {"qi2_cls_lib.ets", "qi2_cls_main.ets"};
    std::vector<std::string> texts = {R"(export class Config {
    field: string = "";
})",
                                      R"(import { Config } from './qi2_cls_lib';
class Holder {
    config: Config = { field: "v" };
}
let holder = new Holder();
)"};
    auto filePaths = CreateTempFile(files, texts);
    ASSERT_EQ(filePaths.size(), files.size());

    Initializer initializer = Initializer();
    auto *ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    auto pos = texts[1].find("field: \"v\"");
    // AST evidence: the contextual type of the class-property initializer
    // resolves to the class definition in the imported file, never to the
    // ETSImportDeclaration node. GetNodeFileName's IsETSImportDeclaration
    // branch is therefore unreachable through the public quick-info API for
    // imported class/interface types.
    auto node = ark::es2panda::lsp::GetTokenForQuickInfo(ctx, pos);
    ASSERT_NE(node, nullptr);
    auto containingObjectNode = ark::es2panda::lsp::GetContainingObjectLiteralNode(node);
    ASSERT_NE(containingObjectNode, nullptr);
    auto contextualTypeNode = ark::es2panda::lsp::GetContextualTypeNode(containingObjectNode->Parent());
    ASSERT_NE(contextualTypeNode, nullptr);
    EXPECT_EQ(contextualTypeNode->Type(), ark::es2panda::ir::AstNodeType::CLASS_DEFINITION);
    EXPECT_FALSE(contextualTypeNode->IsETSImportDeclaration());

    auto info = lspApi->getQuickInfoAtPosition(filePaths[1].c_str(), ctx, pos);
    initializer.DestroyContext(ctx);

    // Class members are CLASS_PROPERTY nodes, while
    // GetPropertyNodeFromContextualType only maps object-literal properties to
    // METHOD_DEFINITION members, so no display parts are produced here.
    EXPECT_EQ(info.GetKind(), "");
    EXPECT_EQ(info.GetKindModifiers(), "");
    EXPECT_EQ(info.GetFileName(), filePaths[1]);
    EXPECT_EQ(info.GetTextSpan().start, pos);
    EXPECT_EQ(info.GetTextSpan().length, std::string("field").size());
    EXPECT_TRUE(info.GetDisplayParts().empty());
}

}  // namespace
