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

#include "lsp/include/script_element_kind.h"
#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"
#include <gtest/gtest.h>

using ark::es2panda::lsp::CompletionEntryKind;
using ark::es2panda::lsp::Initializer;

namespace {

class LspScriptElementKindTests : public LSPAPITests {};

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_1)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);

    Initializer initializer = Initializer();
    es2panda_Context *context = initializer.CreateContext(
        "script_element_kind_1.ets", ES2PANDA_STATE_CHECKED,
        "let number_literal: number = 1234;\nlet string_literal: string = \"hello\";\nconst str_property = "
        "\"foo\";\n");

    ASSERT_EQ(ContextState(context), ES2PANDA_STATE_CHECKED);
    size_t const numberLiteralOffset = 31;  // 31: position of '3' in '1234'
    size_t const stringLiteralOffset = 96;  // 96: position of first 'o' in 'foo'

    auto result = lspApi->getAliasScriptElementKind(context, numberLiteralOffset);
    ASSERT_EQ(result, CompletionEntryKind::TEXT);  // Literal is VALUE
    result = lspApi->getAliasScriptElementKind(context, stringLiteralOffset);
    ASSERT_EQ(result, CompletionEntryKind::TEXT);
    initializer.DestroyContext(context);
}

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_2)
{
    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();
    es2panda_Context *context =
        initializer.CreateContext("script_element_kind_2.ets", ES2PANDA_STATE_CHECKED,
                                  "    \nfunction f() {\n    let a = 123;\n}\nconst s = \"hello\";\n");

    size_t const startOfFile = 0;            // 0: position of start of file, first space
    size_t const firstleftCurlyBrance = 18;  // 18：position of left curly brance after f()
    size_t const numberLiteralOffset = 33;   // 33: position of '2' in '123'
    size_t const stringLiteralOffset = 50;   // 50: position of 'h' in 'hello'

    ASSERT_EQ(lspApi->getAliasScriptElementKind(context, startOfFile), CompletionEntryKind::TEXT);
    ASSERT_EQ(lspApi->getAliasScriptElementKind(context, firstleftCurlyBrance), CompletionEntryKind::TEXT);
    ASSERT_EQ(lspApi->getAliasScriptElementKind(context, numberLiteralOffset), CompletionEntryKind::TEXT);
    ASSERT_EQ(lspApi->getAliasScriptElementKind(context, stringLiteralOffset), CompletionEntryKind::TEXT);

    initializer.DestroyContext(context);
}

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_3)
{
    LSPAPI const *lspApi = GetImpl();
    Initializer initializer = Initializer();

    const char *statement = R"(let empty: null = null;
let notAssigned: undefined = undefined;)";
    es2panda_Context *context =
        initializer.CreateContext("script_element_kind_3.ets", ES2PANDA_STATE_CHECKED, statement);

    size_t const nullLiteral = 19;       // 19：position of the second null.
    size_t const etsNullType = 25;       // 25: position of the second let.
    size_t const undefinedLiteral = 54;  // 54: position of the second undefined.

    ASSERT_EQ(lspApi->getAliasScriptElementKind(context, nullLiteral), CompletionEntryKind::TEXT);
    ASSERT_EQ(lspApi->getAliasScriptElementKind(context, etsNullType), CompletionEntryKind::TEXT);
    ASSERT_EQ(lspApi->getAliasScriptElementKind(context, undefinedLiteral), CompletionEntryKind::TEXT);

    initializer.DestroyContext(context);
}

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_4)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(class Person {
 private body_ : string = '1';
}

type alias/*1*/ = Person;)";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("aliasScriptElementKind_4.ets", ES2PANDA_STATE_CHECKED, text.c_str());

    auto result = lspApi->getAliasScriptElementKind(context, pos);
    EXPECT_EQ(result, CompletionEntryKind::CLASS);
    initializer.DestroyContext(context);
}

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_5)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(
interface MockInterface {
  parameter1: number;
  parameter2: string;
  parameter3: boolean;
}

type alias/*1*/ = MockInterface;)";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("aliasScriptElementKind_5.ets", ES2PANDA_STATE_CHECKED, text.c_str());

    auto result = lspApi->getAliasScriptElementKind(context, pos);
    EXPECT_EQ(result, CompletionEntryKind::INTERFACE);
    initializer.DestroyContext(context);
}

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_6)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(
@Entry
@Component
struct Index {
  @State message: string = 'Hello World';

  build() {
    RelativeContainer() {
      Text(this.message)
        .id('HelloWorld')
        .fontSize($r('app.float.page_text_font_size'))
        .fontWeight(FontWeight.Bold)
        .alignRules({
          center: { anchor: '__container__', align: VerticalAlign.Center },
          middle: { anchor: '__container__', align: HorizontalAlign.Center }
        })
        .onClick(() => {
          this.message = 'Welcome';
        })
    }
    .height('100%')
    .width('100%')
  }
}

type alias/*1*/ = Index;)";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("aliasScriptElementKind_6.ets", ES2PANDA_STATE_CHECKED, text.c_str());

    auto result = lspApi->getAliasScriptElementKind(context, pos);
    EXPECT_EQ(result, CompletionEntryKind::CLASS);
    initializer.DestroyContext(context);
}

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_7)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(type alias/*1*/ = '2';)";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("aliasScriptElementKind_7.ets", ES2PANDA_STATE_CHECKED, text.c_str());

    auto result = lspApi->getAliasScriptElementKind(context, pos);
    EXPECT_EQ(result, CompletionEntryKind::VALUE);
    initializer.DestroyContext(context);
}

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_8)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(type alias/*1*/ = undefinedType11;)";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("aliasScriptElementKind_8.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto result = lspApi->getAliasScriptElementKind(context, pos);
    EXPECT_EQ(result, CompletionEntryKind::ALIAS_TYPE);
    initializer.DestroyContext(context);
}

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_9)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(type TestUnion/*1*/ = string | number;)";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("aliasScriptElementKind_9.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto result = lspApi->getAliasScriptElementKind(context, pos);
    EXPECT_EQ(result, CompletionEntryKind::TYPE_PARAMETER);
    initializer.DestroyContext(context);
}

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_10)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(type Callback/*1*/ = (result: string) => void;)";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("aliasScriptElementKind_10.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto result = lspApi->getAliasScriptElementKind(context, pos);
    EXPECT_EQ(result, CompletionEntryKind::FUNCTION);
    initializer.DestroyContext(context);
}

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_11)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(type Callback/*1*/ = () => {};)";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("aliasScriptElementKind_11.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto result = lspApi->getAliasScriptElementKind(context, pos);
    EXPECT_EQ(result, CompletionEntryKind::FUNCTION);
    initializer.DestroyContext(context);
}

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_12)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);
    const std::string text = R"(class Person {
 private body_ : string = '1';
 private body_ : string = 'ad';
}
type alias/*1*/ = Person;)";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("aliasScriptElementKind_12.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto result = lspApi->getAliasScriptElementKind(context, pos);
    EXPECT_EQ(result, CompletionEntryKind::CLASS);
    initializer.DestroyContext(context);
}

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_13)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);

    const std::string text = R"(type TestA<T>/*1*/ = Array<TestA<T>>;)";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("aliasScriptElementKind_13.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto result = lspApi->getAliasScriptElementKind(context, pos);
    EXPECT_EQ(result, CompletionEntryKind::CLASS);
    initializer.DestroyContext(context);
}

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_14)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);

    const std::string text = R"(type TestB<T>/*1*/ = string | Array<TestB<T>>;)";

    auto pos = text.find("/*1*/");
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("aliasScriptElementKind_14.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto result = lspApi->getAliasScriptElementKind(context, pos);
    EXPECT_EQ(result, CompletionEntryKind::TYPE_PARAMETER);
    initializer.DestroyContext(context);
}

TEST_F(LspScriptElementKindTests, GetAliasScriptElementKind_15)
{
    LSPAPI const *lspApi = GetImpl();
    ASSERT_TRUE(lspApi != nullptr);

    const std::string text = R"(
    //中文测试
    type TestB<T>/*1*/ = string | Array<TestB<T>>;)";

    size_t pos = 29;
    ASSERT_NE(pos, std::string::npos);
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext("aliasScriptElementKind_15.ets", ES2PANDA_STATE_CHECKED, text.c_str());
    auto result = lspApi->getAliasScriptElementKind(context, pos);
    EXPECT_EQ(result, CompletionEntryKind::TYPE_PARAMETER);
    initializer.DestroyContext(context);
}

}  // namespace
