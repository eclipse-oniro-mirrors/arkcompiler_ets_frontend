/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <memory>

namespace {
using ark::es2panda::lsp::Initializer;

class LspQuickInfoTests : public LSPAPITests {};

std::string GetStringOfDisplayParts(const std::vector<SymbolDisplayPart> &displays)
{
    std::string result;
    for (const auto &display : displays) {
        result += display.GetText();
    }
    return result;
}

void AssertQuickInfo(const QuickInfo &expectedQuickInfo, const QuickInfo &actualQuickInfo)
{
    ASSERT_EQ(expectedQuickInfo.GetDisplayParts(), actualQuickInfo.GetDisplayParts())
        << "DisplayParts expect \"" << GetStringOfDisplayParts(expectedQuickInfo.GetDisplayParts()) << "\", but got \""
        << GetStringOfDisplayParts(actualQuickInfo.GetDisplayParts()) << "\"";
    ASSERT_EQ(expectedQuickInfo.GetTextSpan(), actualQuickInfo.GetTextSpan())
        << "TextSpan expect"
        << "(" << expectedQuickInfo.GetTextSpan().start << ", " << expectedQuickInfo.GetTextSpan().length
        << ") but got (" << actualQuickInfo.GetTextSpan().start << ", " << actualQuickInfo.GetTextSpan().length << ")";
    ASSERT_EQ(expectedQuickInfo.GetKind(), actualQuickInfo.GetKind()) << "Kind Assertion Failed";
    ASSERT_EQ(expectedQuickInfo.GetKindModifiers(), actualQuickInfo.GetKindModifiers())
        << "KindModifiers Assertion Failed";
    ASSERT_EQ(expectedQuickInfo.GetFileName(), actualQuickInfo.GetFileName()) << "FileName Assertion Failed";
    ASSERT_EQ(expectedQuickInfo.GetDocument(), actualQuickInfo.GetDocument()) << "Document Assertion Failed";
    ASSERT_EQ(expectedQuickInfo.GetTags(), actualQuickInfo.GetTags()) << "Tags Assertion Failed";
}

const std::vector<std::string> &GetFileContentsOfInterface()
{
    static const std::vector<std::string> CONTENTS = {
        R"('use static'
        interface InterfaceTest {
            a: number;
        })"};
    return CONTENTS;
}

QuickInfo ExpectResultInterface()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetFileContentsOfInterface.ets";
    const std::string kind = "get";
    size_t const start = 59;
    size_t const length = 1;
    TextSpan span(start, length);
    const std::string kindModifiers = "public abstract";
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("InterfaceTest", "interface");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("a", "property");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "returnType");
    return QuickInfo(kind, kindModifiers, span, expected, document, tags, expectedFileName);
}

TEST_F(LspQuickInfoTests, GetFileContentsOfInterface)
{
    std::vector<std::string> fileNames = {"GetFileContentsOfInterface.ets"};

    auto filePaths = CreateTempFile(fileNames, GetFileContentsOfInterface());
    ASSERT_TRUE(filePaths.size() == GetFileContentsOfInterface().size());

    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    size_t const offset = 59;
    auto quickInfo = lspApi->getQuickInfoAtPosition("GetFileContentsOfInterface.ets", ctx, offset);
    auto expectedQuickInfo = ExpectResultInterface();
    AssertQuickInfo(expectedQuickInfo, quickInfo);

    initializer.DestroyContext(ctx);
}

const std::vector<std::string> &GetFileContentsOfKeywordsClass()
{
    static const std::vector<std::string> CONTENTS = {
        R"('use static'
        class A {}
        )"};
    return CONTENTS;
}

QuickInfo ExpectResultKeywordsClass()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetFileContentsOfKeywordsClass.ets";
    const std::string kind = "class";
    size_t const start = 21;
    size_t const length = 19;
    TextSpan span(start, length);
    const std::string kindModifiers = "static public";
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("class", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("A", "className");
    return QuickInfo(kind, kindModifiers, span, expected, document, tags, expectedFileName);
}

TEST_F(LspQuickInfoTests, GetFileContentsOfKeywordsClass)
{
    std::vector<std::string> fileNames = {"GetFileContentsOfKeywordsClass.ets"};

    auto filePaths = CreateTempFile(fileNames, GetFileContentsOfKeywordsClass());
    ASSERT_TRUE(filePaths.size() == GetFileContentsOfKeywordsClass().size());

    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    size_t const offset = 21;
    auto quickInfo = lspApi->getQuickInfoAtPosition("GetFileContentsOfKeywordsClass.ets", ctx, offset);
    auto expectedQuickInfo = ExpectResultKeywordsClass();
    AssertQuickInfo(expectedQuickInfo, quickInfo);

    initializer.DestroyContext(ctx);
}

const std::vector<std::string> &GetFileContentsOfKeywordsEnum()
{
    static const std::vector<std::string> CONTENTS = {
        R"('use static'
        enum A {}
        )"};
    return CONTENTS;
}

QuickInfo ExpectResultKeywordsEnum()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetFileContentsOfKeywordsEnum.ets";
    const std::string kind = "enum";
    size_t const start = 21;
    size_t const length = 9;
    TextSpan span(start, length);
    const std::string kindModifiers = "";
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("enum", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("A", "enumName");
    return QuickInfo(kind, kindModifiers, span, expected, document, tags, expectedFileName);
}

TEST_F(LspQuickInfoTests, GetFileContentsOfKeywordsEnum)
{
    std::vector<std::string> fileNames = {"GetFileContentsOfKeywordsEnum.ets"};

    auto filePaths = CreateTempFile(fileNames, GetFileContentsOfKeywordsEnum());
    ASSERT_TRUE(filePaths.size() == GetFileContentsOfKeywordsEnum().size());

    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    size_t const offset = 21;
    auto quickInfo = lspApi->getQuickInfoAtPosition("GetFileContentsOfKeywordsEnum.ets", ctx, offset);
    auto expectedQuickInfo = ExpectResultKeywordsEnum();
    AssertQuickInfo(expectedQuickInfo, quickInfo);

    initializer.DestroyContext(ctx);
}

const std::vector<std::string> &GetFileContentsOfEnum()
{
    static const std::vector<std::string> CONTENTS = {
        R"(
enum MyEnumStrings { A = 'hello' };

enum MyEnumNum {
    a = 1
}

enum MyDefaultEnum {
    a,
    b
}

enum MyErrorEnum {
    a,
    b = 2,
    c = 'cc'
}

MyDefaultEnum.a
)"};
    return CONTENTS;
}

QuickInfo ExpectResultEnum1()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string kind1 = "enum";
    size_t const start1 = 6;
    size_t const length1 = 13;
    TextSpan span1(start1, length1);
    const std::string kindModifiers1 = "final";
    const std::string expectedFileName = "GetQuickInfoAtPositionEnum.ets";
    std::vector<SymbolDisplayPart> expected1;
    expected1.emplace_back("enum", "keyword");
    expected1.emplace_back(" ", "space");
    expected1.emplace_back("MyEnumStrings", "enumName");
    return QuickInfo(kind1, kindModifiers1, span1, expected1, document, tags, expectedFileName);
}

QuickInfo ExpectResultEnum2()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionEnum.ets";
    const std::string kind2 = "enum member";
    size_t const start2 = 22;
    size_t const length2 = 1;
    TextSpan span2(start2, length2);
    const std::string kindModifiers2 = "static public readonly";
    std::vector<SymbolDisplayPart> expected2;
    expected2.emplace_back("MyEnumStrings", "enumName");
    expected2.emplace_back(".", "punctuation");
    expected2.emplace_back("A", "enumMember");
    expected2.emplace_back(" ", "space");
    expected2.emplace_back("=", "operator");
    expected2.emplace_back(" ", "space");
    expected2.emplace_back("\"", "punctuation");
    expected2.emplace_back("hello", "text");
    expected2.emplace_back("\"", "punctuation");
    return QuickInfo(kind2, kindModifiers2, span2, expected2, document, tags, expectedFileName);
}

QuickInfo ExpectResultEnum3()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionEnum.ets";
    const std::string kind = "enum member";
    size_t const start = 59;
    size_t const length = 1;
    TextSpan span(start, length);
    const std::string kindModifiers = "static public readonly";
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("MyEnumNum", "enumName");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("a", "enumMember");
    expected.emplace_back(" ", "space");
    expected.emplace_back("=", "operator");
    expected.emplace_back(" ", "space");
    expected.emplace_back("1", "text");
    return QuickInfo(kind, kindModifiers, span, expected, document, tags, expectedFileName);
}

QuickInfo ExpectResultEnum4()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionEnum.ets";
    const std::string kind2 = "enum member";
    size_t const start2 = 93;
    size_t const length2 = 1;
    TextSpan span2(start2, length2);
    const std::string kindModifiers2 = "static public readonly";
    std::vector<SymbolDisplayPart> expected2;
    expected2.emplace_back("MyDefaultEnum", "enumName");
    expected2.emplace_back(".", "punctuation");
    expected2.emplace_back("a", "enumMember");
    expected2.emplace_back(" ", "space");
    expected2.emplace_back("=", "operator");
    expected2.emplace_back(" ", "space");
    expected2.emplace_back("0", "text");
    return QuickInfo(kind2, kindModifiers2, span2, expected2, document, tags, expectedFileName);
}

QuickInfo ExpectResultEnum5()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionEnum.ets";
    const std::string kind2 = "enum member";
    size_t const start2 = 100;
    size_t const length2 = 1;
    TextSpan span2(start2, length2);
    const std::string kindModifiers2 = "static public readonly";
    std::vector<SymbolDisplayPart> expected2;
    expected2.emplace_back("MyDefaultEnum", "enumName");
    expected2.emplace_back(".", "punctuation");
    expected2.emplace_back("b", "enumMember");
    expected2.emplace_back(" ", "space");
    expected2.emplace_back("=", "operator");
    expected2.emplace_back(" ", "space");
    expected2.emplace_back("1", "text");
    return QuickInfo(kind2, kindModifiers2, span2, expected2, document, tags, expectedFileName);
}

QuickInfo ExpectResultEnum6()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionEnum.ets";
    const std::string kind2 = "enum member";
    size_t const start2 = 128;
    size_t const length2 = 1;
    TextSpan span2(start2, length2);
    const std::string kindModifiers2;
    std::vector<SymbolDisplayPart> expected2;
    expected2.emplace_back("MyErrorEnum", "enumName");
    expected2.emplace_back(".", "punctuation");
    expected2.emplace_back("a", "enumMember");
    expected2.emplace_back(" ", "space");
    expected2.emplace_back("=", "operator");
    expected2.emplace_back(" ", "space");
    expected2.emplace_back("0", "text");
    return QuickInfo(kind2, kindModifiers2, span2, expected2, document, tags, expectedFileName);
}

QuickInfo ExpectResultEnum7()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionEnum.ets";
    const std::string kind2 = "enum member";
    size_t const start2 = 135;
    size_t const length2 = 1;
    TextSpan span2(start2, length2);
    const std::string kindModifiers2;
    std::vector<SymbolDisplayPart> expected2;
    expected2.emplace_back("MyErrorEnum", "enumName");
    expected2.emplace_back(".", "punctuation");
    expected2.emplace_back("b", "enumMember");
    expected2.emplace_back(" ", "space");
    expected2.emplace_back("=", "operator");
    expected2.emplace_back(" ", "space");
    expected2.emplace_back("2", "text");
    return QuickInfo(kind2, kindModifiers2, span2, expected2, document, tags, expectedFileName);
}

QuickInfo ExpectResultEnum8()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionEnum.ets";
    const std::string kind2 = "enum member";
    size_t const start2 = 146;
    size_t const length2 = 1;
    TextSpan span2(start2, length2);
    const std::string kindModifiers2;
    std::vector<SymbolDisplayPart> expected2;
    expected2.emplace_back("MyErrorEnum", "enumName");
    expected2.emplace_back(".", "punctuation");
    expected2.emplace_back("c", "enumMember");
    expected2.emplace_back(" ", "space");
    expected2.emplace_back("=", "operator");
    expected2.emplace_back(" ", "space");
    expected2.emplace_back("\"", "punctuation");
    expected2.emplace_back("cc", "text");
    expected2.emplace_back("\"", "punctuation");
    return QuickInfo(kind2, kindModifiers2, span2, expected2, document, tags, expectedFileName);
}

QuickInfo ExpectResultEnum9()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionEnum.ets";
    const std::string kind2 = "enum member";
    size_t const start2 = 172;
    size_t const length2 = 1;
    TextSpan span2(start2, length2);
    const std::string kindModifiers2 = "static public readonly";
    std::vector<SymbolDisplayPart> expected2;
    expected2.emplace_back("MyDefaultEnum", "enumName");
    expected2.emplace_back(".", "punctuation");
    expected2.emplace_back("a", "enumMember");
    expected2.emplace_back(" ", "space");
    expected2.emplace_back("=", "operator");
    expected2.emplace_back(" ", "space");
    expected2.emplace_back("0", "text");
    return QuickInfo(kind2, kindModifiers2, span2, expected2, document, tags, expectedFileName);
}

TEST_F(LspQuickInfoTests, GetQuickInfoAtPositionEnum)
{
    std::vector<std::string> fileNames = {"GetQuickInfoAtPositionEnum.ets"};

    auto filePaths = CreateTempFile(fileNames, GetFileContentsOfEnum());
    ASSERT_TRUE(filePaths.size() == GetFileContentsOfEnum().size());

    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    size_t const offset1 = 12;  // MyEnumStrings
    auto quickInfo1 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionEnum.ets", ctx, offset1);
    auto expectedQuickInfo1 = ExpectResultEnum1();
    AssertQuickInfo(expectedQuickInfo1, quickInfo1);

    size_t const offset2 = 22;  // MyEnumStrings.A
    auto quickInfo2 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionEnum.ets", ctx, offset2);
    auto expectedQuickInfo2 = ExpectResultEnum2();
    AssertQuickInfo(expectedQuickInfo2, quickInfo2);

    size_t const offset3 = 59;  // MyEnumNum.a
    auto quickInfo3 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionEnum.ets", ctx, offset3);
    auto expectedQuickInfo3 = ExpectResultEnum3();
    AssertQuickInfo(expectedQuickInfo3, quickInfo3);

    size_t const offset4 = 93;  // MyDefaultEnum.a
    auto quickInfo4 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionEnum.ets", ctx, offset4);
    auto expectedQuickInfo4 = ExpectResultEnum4();
    AssertQuickInfo(expectedQuickInfo4, quickInfo4);

    size_t const offset5 = 100;  // MyDefaultEnum.b
    auto quickInfo5 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionEnum.ets", ctx, offset5);
    auto expectedQuickInfo5 = ExpectResultEnum5();
    AssertQuickInfo(expectedQuickInfo5, quickInfo5);

    size_t const offset6 = 128;  // MyErrorEnum.a
    auto quickInfo6 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionEnum.ets", ctx, offset6);
    auto expectedQuickInfo6 = ExpectResultEnum6();
    AssertQuickInfo(expectedQuickInfo6, quickInfo6);

    size_t const offset7 = 135;  // MyErrorEnum.b
    auto quickInfo7 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionEnum.ets", ctx, offset7);
    auto expectedQuickInfo7 = ExpectResultEnum7();
    AssertQuickInfo(expectedQuickInfo7, quickInfo7);

    size_t const offset8 = 146;  // MyErrorEnum.c
    auto quickInfo8 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionEnum.ets", ctx, offset8);
    auto expectedQuickInfo8 = ExpectResultEnum8();
    AssertQuickInfo(expectedQuickInfo8, quickInfo8);

    size_t const offset9 = 172;  // MyDefaultEnum.a
    auto quickInfo9 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionEnum.ets", ctx, offset9);
    auto expectedQuickInfo9 = ExpectResultEnum9();
    AssertQuickInfo(expectedQuickInfo9, quickInfo9);

    initializer.DestroyContext(ctx);
}

const std::vector<std::string> &GetFileContentsOfImport()
{
    static const std::vector<std::string> CONTENTS = {
        R"(
export function Text(a: number){return '1'}
export class A {
    a: number = 1
}
)",
        R"(
import { Text, A } from './GetQuickInfoAtPositionImportText'
Text(1)
)"};
    return CONTENTS;
}

QuickInfo ExpectResultImport1()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionImport.ets";
    const std::string kind2 = "method";
    size_t const start2 = 10;
    size_t const length2 = 4;
    TextSpan span2(start2, length2);
    const std::string kindModifiers2 = "static public export";
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("function", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("Text", "functionName");
    expected.emplace_back("(", "punctuation");
    expected.emplace_back("a", "functionParameter");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("number", "typeParameter");
    expected.emplace_back(")", "punctuation");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("string", "returnType");
    return QuickInfo(kind2, kindModifiers2, span2, expected, document, tags, expectedFileName);
}

QuickInfo ExpectResultImport2()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionImport.ets";
    const std::string kind2 = "class";
    size_t const start2 = 16;
    size_t const length2 = 1;
    TextSpan span2(start2, length2);
    const std::string kindModifiers2;
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("class", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("A", "className");
    return QuickInfo(kind2, kindModifiers2, span2, expected, document, tags, expectedFileName);
}

TEST_F(LspQuickInfoTests, GetQuickInfoAtPositionImport)
{
    std::vector<std::string> fileNames = {"GetQuickInfoAtPositionImportText.ets", "GetQuickInfoAtPositionImport.ets"};

    auto filePaths = CreateTempFile(fileNames, GetFileContentsOfImport());
    ASSERT_TRUE(filePaths.size() == GetFileContentsOfImport().size());

    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    size_t const offset1 = 10;  // import { Text
    auto quickInfo1 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionImport.ets", ctx, offset1);
    auto expectedQuickInfo1 = ExpectResultImport1();
    AssertQuickInfo(expectedQuickInfo1, quickInfo1);

    size_t const offset2 = 16;  // , A
    auto quickInfo2 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionImport.ets", ctx, offset2);
    auto expectedQuickInfo2 = ExpectResultImport2();
    AssertQuickInfo(expectedQuickInfo2, quickInfo2);

    initializer.DestroyContext(ctx);
}

const std::vector<std::string> &GetFileContentsOfImport1()
{
    static const std::vector<std::string> CONTENTS = {
        R"('use static'
        export @interface B {})",
        R"('use static'
        import { B } from "./GetQuickInfoAtPositionImportText1")"};
    return CONTENTS;
}

QuickInfo ExpectResultImport3()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionImport1.ets";
    const std::string kind = "annotation";
    size_t const start = 30;
    size_t const length = 1;
    TextSpan span(start, length);
    const std::string kindModifiers = "static public abstract export annotation_declaration";
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("@", "punctuation");
    expected.emplace_back("interface", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("GetQuickInfoAtPositionImportText1.B", "className");
    expected.emplace_back(" ", "space");
    expected.emplace_back("{", "punctuation");
    expected.emplace_back("}", "punctuation");
    return QuickInfo(kind, kindModifiers, span, expected, document, tags, expectedFileName);
}

QuickInfo ExpectResultImport4()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionImport1.ets";
    const std::string kind = "import";
    size_t const start = 39;
    size_t const length = 37;
    TextSpan span(start, length);
    const std::string kindModifiers = "";
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("module", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("/tmp/GetQuickInfoAtPositionImportText1.ets", "className");
    return QuickInfo(kind, kindModifiers, span, expected, document, tags, expectedFileName);
}

TEST_F(LspQuickInfoTests, GetQuickInfoAtPositionImport1)
{
    std::vector<std::string> fileNames = {"GetQuickInfoAtPositionImportText1.ets", "GetQuickInfoAtPositionImport1.ets"};

    auto filePaths = CreateTempFile(fileNames, GetFileContentsOfImport1());
    ASSERT_TRUE(filePaths.size() == GetFileContentsOfImport1().size());

    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[1].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    size_t const offset1 = 30;
    auto quickInfo1 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionImport1.ets", ctx, offset1);

    QuickInfo expectedQuickInfo3 = ExpectResultImport3();

    AssertQuickInfo(expectedQuickInfo3, quickInfo1);

    size_t const offset2 = 39;
    auto quickInfo2 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionImport1.ets", ctx, offset2);

    QuickInfo expectedQuickInfo4 = ExpectResultImport4();

    AssertQuickInfo(expectedQuickInfo4, quickInfo2);

    initializer.DestroyContext(ctx);
}

const std::vector<std::string> &GetFileContentsOfClass()
{
    static const std::vector<std::string> CONTENTS = {
        R"(
struct A {
    a: number = 1
}
namespace S {}
)"};
    return CONTENTS;
}

QuickInfo ExpectResultClass1()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionClass.ets";
    const std::string kind2 = "struct";
    size_t const start2 = 8;
    size_t const length2 = 1;
    TextSpan span2(start2, length2);
    const std::string kindModifiers2;
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("struct", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("A", "structName");
    return QuickInfo(kind2, kindModifiers2, span2, expected, document, tags, expectedFileName);
}

QuickInfo ExpectResultClass2()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionClass.ets";
    const std::string kind2 = "namespace";
    size_t const start2 = 42;
    size_t const length2 = 1;
    TextSpan span2(start2, length2);
    const std::string kindModifiers2 = "abstract";
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("namespace", "keyword");
    expected.emplace_back(" ", "space");
    expected.emplace_back("S", "namespace");
    return QuickInfo(kind2, kindModifiers2, span2, expected, document, tags, expectedFileName);
}

TEST_F(LspQuickInfoTests, GetQuickInfoAtPositionClass)
{
    std::vector<std::string> fileNames = {"GetQuickInfoAtPositionClass.ets"};

    auto filePaths = CreateTempFile(fileNames, GetFileContentsOfClass());
    ASSERT_TRUE(filePaths.size() == GetFileContentsOfClass().size());

    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    size_t const offset1 = 8;  // struct A
    auto quickInfo1 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionClass.ets", ctx, offset1);
    auto expectedQuickInfo1 = ExpectResultClass1();
    AssertQuickInfo(expectedQuickInfo1, quickInfo1);

    auto context = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    const ark::es2panda::ir::AstNode *ast = context->parserProgram->Ast();
    auto *structDefNode = ast->FindChild(
        [](auto *node) { return node->IsClassDefinition() && node->Parent()->IsETSStructDeclaration(); });
    structDefNode->AsClassDefinition()->SetFromStructModifier();
    auto isFromStruct = structDefNode->AsClassDefinition()->IsFromStruct();
    ASSERT_EQ(isFromStruct, true);
    auto quickInfo3 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionClass.ets", ctx, offset1);
    AssertQuickInfo(expectedQuickInfo1, quickInfo3);

    size_t const offset2 = 42;  // namespace S
    auto quickInfo2 = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionClass.ets", ctx, offset2);
    auto expectedQuickInfo2 = ExpectResultClass2();
    AssertQuickInfo(expectedQuickInfo2, quickInfo2);

    initializer.DestroyContext(ctx);
}

const std::vector<std::string> &GetFileContentsOfJSON()
{
    static const std::vector<std::string> CONTENTS = {R"(let obj = JSON.parseJsonElement("");)"};
    return CONTENTS;
}

QuickInfo ExpectResultJSONParse()
{
    std::vector<DocTagInfo> tags {};
    std::vector<SymbolDisplayPart> document {};
    const std::string expectedFileName = "GetQuickInfoAtPositionJSON.ets";
    const std::string kind = "method";
    size_t const start = 15;
    size_t const length = 16;
    TextSpan span(start, length);
    const std::string kindModifiers = "static public declare";
    std::vector<SymbolDisplayPart> expected;
    expected.emplace_back("JSON", "className");
    expected.emplace_back(".", "punctuation");
    expected.emplace_back("parseJsonElement", "functionName");
    expected.emplace_back("(", "punctuation");
    expected.emplace_back("text", "functionParameter");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("string", "typeParameter");
    expected.emplace_back(")", "punctuation");
    expected.emplace_back(":", "punctuation");
    expected.emplace_back(" ", "space");
    expected.emplace_back("jsonx.JsonElement", "returnType");
    return QuickInfo(kind, kindModifiers, span, expected, document, tags, expectedFileName);
}

TEST_F(LspQuickInfoTests, GetQuickInfoAtPositionJSON)
{
    std::vector<std::string> fileNames = {"GetQuickInfoAtPositionJSON.ets"};

    auto filePaths = CreateTempFile(fileNames, GetFileContentsOfJSON());
    ASSERT_TRUE(filePaths.size() == GetFileContentsOfJSON().size());

    Initializer initializer;
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    LSPAPI const *lspApi = GetImpl();

    size_t const offset = 20;  // parseJsonElement
    auto quickInfo = lspApi->getQuickInfoAtPosition("GetQuickInfoAtPositionJSON.ets", ctx, offset);
    auto expectedQuickInfo = ExpectResultJSONParse();
    AssertQuickInfo(expectedQuickInfo, quickInfo);

    initializer.DestroyContext(ctx);
}
}  // namespace