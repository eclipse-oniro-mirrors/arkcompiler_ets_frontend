/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "lsp/include/internal_api.h"
#include "lsp/include/classifier.h"
#include "lsp_api_test.h"
#include "public/es2panda_lib.h"

using ark::es2panda::lsp::Initializer;

class LspClassificationTests : public LSPAPITests {};

TEST_F(LspClassificationTests, GetSyntacticClassifications1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("class-name.ets", ES2PANDA_STATE_PARSED, R"(class A {};)");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    size_t const start = 6;
    size_t const length = 1;
    auto result = ark::es2panda::lsp::GetSyntacticClassifications(ctx, start, length);
    const size_t expectedCount = 1;
    std::string expectedName = "class name";
    ASSERT_EQ(result.size(), expectedCount);
    ASSERT_EQ(result.at(0)->start, start);
    ASSERT_EQ(result.at(0)->length, length);
    ASSERT_EQ(result.at(0)->name, expectedName);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSyntacticClassifications2)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("enum-name.ets", ES2PANDA_STATE_PARSED, R"(enum Color {Red, Blue, Green};)");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    size_t const start = 5;
    size_t const length = 5;
    auto result = ark::es2panda::lsp::GetSyntacticClassifications(ctx, start, length);
    const size_t expectedCount = 1;
    std::string expectedName = "enum name";
    ASSERT_EQ(result.size(), expectedCount);
    ASSERT_EQ(result.at(0)->start, start);
    ASSERT_EQ(result.at(0)->length, length);
    ASSERT_EQ(result.at(0)->name, expectedName);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSyntacticClassifications3)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("interface-name.ets", ES2PANDA_STATE_PARSED, R"(interface I {};)");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    size_t const start = 10;
    size_t const length = 1;
    auto result = ark::es2panda::lsp::GetSyntacticClassifications(ctx, start, length);
    const size_t expectedCount = 1;
    std::string expectedName = "interface name";
    ASSERT_EQ(result.size(), expectedCount);
    ASSERT_EQ(result.at(0)->start, start);
    ASSERT_EQ(result.at(0)->length, length);
    ASSERT_EQ(result.at(0)->name, expectedName);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSyntacticClassifications4)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("type-parameter-name.ets", ES2PANDA_STATE_PARSED, R"(class Foo<T> {};)");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    size_t const start = 10;
    size_t const length = 1;
    auto result = ark::es2panda::lsp::GetSyntacticClassifications(ctx, start, length);
    const size_t expectedCount = 1;
    std::string expectedName = "type parameter name";
    ASSERT_EQ(result.size(), expectedCount);
    ASSERT_EQ(result.at(0)->start, start);
    ASSERT_EQ(result.at(0)->length, length);
    ASSERT_EQ(result.at(0)->name, expectedName);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSyntacticClassifications5)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("type-alias-name.ets", ES2PANDA_STATE_PARSED, R"(type tmp = Long|null;)");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    size_t const start = 5;
    size_t const length = 3;
    auto result = ark::es2panda::lsp::GetSyntacticClassifications(ctx, start, length);
    const size_t expectedCount = 1;
    std::string expectedName = "type alias name";
    ASSERT_EQ(result.size(), expectedCount);
    ASSERT_EQ(result.at(0)->start, start);
    ASSERT_EQ(result.at(0)->length, length);
    ASSERT_EQ(result.at(0)->name, expectedName);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSyntacticClassifications6)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("parameter-name.ets", ES2PANDA_STATE_PARSED, R"(function A(a:number) {};)");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    size_t const start = 11;
    size_t const length = 1;
    auto result = ark::es2panda::lsp::GetSyntacticClassifications(ctx, start, length);
    const size_t expectedCount = 1;
    std::string expectedName = "parameter name";
    ASSERT_EQ(result.size(), expectedCount);
    ASSERT_EQ(result.at(0)->start, start);
    ASSERT_EQ(result.at(0)->length, length);
    ASSERT_EQ(result.at(0)->name, expectedName);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSyntacticClassifications7)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("number-type.ets", ES2PANDA_STATE_PARSED, R"(let num = 1;)");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    size_t const start = 10;
    size_t const length = 1;
    auto result = ark::es2panda::lsp::GetSyntacticClassifications(ctx, start, length);
    const size_t expectedCount = 1;
    std::string expectedName = "number";
    ASSERT_EQ(result.size(), expectedCount);
    ASSERT_EQ(result.at(0)->start, start);
    ASSERT_EQ(result.at(0)->length, length);
    ASSERT_EQ(result.at(0)->name, expectedName);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSyntacticClassifications8)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("string-type.ets", ES2PANDA_STATE_PARSED, R"(let str = "123";)");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    size_t const start = 10;
    size_t const length = 5;
    auto result = ark::es2panda::lsp::GetSyntacticClassifications(ctx, start, length);
    const size_t expectedCount = 1;
    std::string expectedName = "string";
    ASSERT_EQ(result.size(), expectedCount);
    ASSERT_EQ(result.at(0)->start, start);
    ASSERT_EQ(result.at(0)->length, length);
    ASSERT_EQ(result.at(0)->name, expectedName);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSyntacticClassifications9)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("boolean-type.ets", ES2PANDA_STATE_PARSED, R"(let a = true;
let b = false;)");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    size_t const start = 0;
    size_t const length = 28;
    auto result = ark::es2panda::lsp::GetSyntacticClassifications(ctx, start, length);
    // NOLINTBEGIN(readability-magic-numbers)
    std::vector<std::tuple<size_t, size_t, std::string>> expectedResult = {
        {0, 3, "keyword"},  {4, 1, "identifier"},  {6, 1, "punctuation"},  {8, 4, "boolean"},  {12, 1, "punctuation"},
        {14, 3, "keyword"}, {18, 1, "identifier"}, {20, 1, "punctuation"}, {22, 5, "boolean"}, {27, 1, "punctuation"}};
    // NOLINTEND(readability-magic-numbers)
    ASSERT_EQ(result.size(), expectedResult.size());

    for (size_t i = 0; i < result.size(); i++) {
        auto expectedStart = std::get<0>(expectedResult.at(i));
        auto expectedLength = std::get<1>(expectedResult.at(i));
        auto expectedName = std::get<2>(expectedResult.at(i));
        ASSERT_EQ(result.at(i)->start, expectedStart);
        ASSERT_EQ(result.at(i)->length, expectedLength);
        ASSERT_EQ(result.at(i)->name, expectedName);
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSyntacticClassifications10)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("null-type.ets", ES2PANDA_STATE_PARSED, R"(type tmp = null;)");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);

    size_t const start = 0;
    size_t const length = 28;
    auto result = ark::es2panda::lsp::GetSyntacticClassifications(ctx, start, length);
    // NOLINTBEGIN(readability-magic-numbers)
    std::vector<std::tuple<size_t, size_t, std::string>> expectedResult = {
        {0, 4, "keyword"}, {5, 3, "type alias name"}, {9, 1, "punctuation"}, {11, 4, "null"}, {15, 1, "punctuation"}};
    // NOLINTEND(readability-magic-numbers)
    ASSERT_EQ(result.size(), expectedResult.size());

    for (size_t i = 0; i < result.size(); i++) {
        auto expectedStart = std::get<0>(expectedResult.at(i));
        auto expectedLength = std::get<1>(expectedResult.at(i));
        auto expectedName = std::get<2>(expectedResult.at(i));
        ASSERT_EQ(result.at(i)->start, expectedStart);
        ASSERT_EQ(result.at(i)->length, expectedLength);
        ASSERT_EQ(result.at(i)->name, expectedName);
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSyntacticClassifications11)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(
        "punctuator-type1.ets", ES2PANDA_STATE_PARSED,
        "let a = 1;\nlet b = 2;\nclass C {foo(){}};\nlet c = new C();\na + b;\na & b;\na += b;\na |= b;\na &= b;\na < "
        "b;\nc?.foo;\na - b;\na | b;\na -= b;\na ^= b;\na && b;\na > b;\n!a;\na * b;\na ^ b;\na *= b;\na <<= b;\na || "
        "b;\n");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    size_t const start = 0;
    size_t const length = 428;
    auto result = ark::es2panda::lsp::GetSyntacticClassifications(ctx, start, length);
    // NOLINTBEGIN(readability-magic-numbers)
    std::vector<std::tuple<size_t, size_t, std::string>> expectedResult = {
        {0, 3, "keyword"},       {4, 1, "identifier"},    {6, 1, "punctuation"},   {8, 1, "number"},
        {9, 1, "punctuation"},   {11, 3, "keyword"},      {15, 1, "identifier"},   {17, 1, "punctuation"},
        {19, 1, "number"},       {20, 1, "punctuation"},  {22, 5, "keyword"},      {28, 1, "class name"},
        {30, 1, "punctuation"},  {31, 3, "identifier"},   {34, 1, "punctuation"},  {35, 1, "punctuation"},
        {36, 1, "punctuation"},  {37, 1, "punctuation"},  {38, 1, "punctuation"},  {39, 1, "punctuation"},
        {41, 3, "keyword"},      {45, 1, "identifier"},   {47, 1, "punctuation"},  {49, 3, "keyword"},
        {53, 1, "identifier"},   {54, 1, "punctuation"},  {55, 1, "punctuation"},  {56, 1, "punctuation"},
        {58, 1, "identifier"},   {60, 1, "punctuation"},  {62, 1, "identifier"},   {63, 1, "punctuation"},
        {65, 1, "identifier"},   {67, 1, "punctuation"},  {69, 1, "identifier"},   {70, 1, "punctuation"},
        {72, 1, "identifier"},   {74, 2, "punctuation"},  {77, 1, "identifier"},   {78, 1, "punctuation"},
        {80, 1, "identifier"},   {82, 2, "punctuation"},  {85, 1, "identifier"},   {86, 1, "punctuation"},
        {88, 1, "identifier"},   {90, 2, "punctuation"},  {93, 1, "identifier"},   {94, 1, "punctuation"},
        {96, 1, "identifier"},   {98, 1, "punctuation"},  {100, 1, "identifier"},  {101, 1, "punctuation"},
        {103, 1, "identifier"},  {104, 2, "punctuation"}, {106, 3, "identifier"},  {109, 1, "punctuation"},
        {111, 1, "identifier"},  {113, 1, "punctuation"}, {115, 1, "identifier"},  {116, 1, "punctuation"},
        {118, 1, "identifier"},  {120, 1, "punctuation"}, {122, 1, "identifier"},  {123, 1, "punctuation"},
        {125, 1, "identifier"},  {127, 2, "punctuation"}, {130, 1, "identifier"},  {131, 1, "punctuation"},
        {133, 1, "identifier"},  {135, 2, "punctuation"}, {138, 1, "identifier"},  {139, 1, "punctuation"},
        {141, 1, "identifier"},  {143, 2, "punctuation"}, {146, 1, "identifier"},  {147, 1, "punctuation"},
        {149, 1, "identifier"},  {151, 1, "punctuation"}, {153, 1, "identifier"},  {154, 1, "punctuation"},
        {156, 1, "punctuation"}, {157, 1, "identifier"},  {158, 1, "punctuation"}, {160, 1, "identifier"},
        {162, 1, "punctuation"}, {164, 1, "identifier"},  {165, 1, "punctuation"}, {167, 1, "identifier"},
        {169, 1, "punctuation"}, {171, 1, "identifier"},  {172, 1, "punctuation"}, {174, 1, "identifier"},
        {176, 2, "punctuation"}, {179, 1, "identifier"},  {180, 1, "punctuation"}, {182, 1, "identifier"},
        {184, 3, "punctuation"}, {188, 1, "identifier"},  {189, 1, "punctuation"}, {191, 1, "identifier"},
        {193, 2, "punctuation"}, {196, 1, "identifier"},  {197, 1, "punctuation"}};
    // NOLINTEND(readability-magic-numbers)
    ASSERT_EQ(result.size(), expectedResult.size());
    for (size_t i = 0; i < result.size(); i++) {
        auto expectedStart = std::get<0>(expectedResult.at(i));
        auto expectedLength = std::get<1>(expectedResult.at(i));
        auto expectedName = std::get<2>(expectedResult.at(i));
        ASSERT_EQ(result.at(i)->start, expectedStart);
        ASSERT_EQ(result.at(i)->length, expectedLength);
        ASSERT_EQ(result.at(i)->name, expectedName);
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSyntacticClassifications12)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext(
        "punctuator-type2.ets", ES2PANDA_STATE_PARSED,
        "a === b;\na <= b;\na / b;\na >> b;\na /= b;\na >>= b;\na++;\na == b;\na >= b;\na % b;\na << b;\na %= "
        "b;\na >>>= b;\na--;\na = b;\nlet var1: [number[]];\n(a);\nlet d:[];\n{a};\nc??b;\na != b;\na !== b;\n");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_PARSED);
    size_t const start = 0;
    size_t const length = 428;
    auto result = ark::es2panda::lsp::GetSyntacticClassifications(ctx, start, length);
    // NOLINTBEGIN(readability-magic-numbers)
    std::vector<std::tuple<size_t, size_t, std::string>> expectedResult = {
        {0, 1, "identifier"},    {2, 3, "punctuation"},   {6, 1, "identifier"},    {7, 1, "punctuation"},
        {9, 1, "identifier"},    {11, 2, "punctuation"},  {14, 1, "identifier"},   {15, 1, "punctuation"},
        {17, 1, "identifier"},   {19, 1, "punctuation"},  {21, 1, "identifier"},   {22, 1, "punctuation"},
        {24, 1, "identifier"},   {26, 2, "punctuation"},  {29, 1, "identifier"},   {30, 1, "punctuation"},
        {32, 1, "identifier"},   {34, 2, "punctuation"},  {37, 1, "identifier"},   {38, 1, "punctuation"},
        {40, 1, "identifier"},   {42, 3, "punctuation"},  {46, 1, "identifier"},   {47, 1, "punctuation"},
        {49, 1, "identifier"},   {50, 2, "punctuation"},  {52, 1, "punctuation"},  {54, 1, "identifier"},
        {56, 2, "punctuation"},  {59, 1, "identifier"},   {60, 1, "punctuation"},  {62, 1, "identifier"},
        {64, 2, "punctuation"},  {67, 1, "identifier"},   {68, 1, "punctuation"},  {70, 1, "identifier"},
        {72, 1, "punctuation"},  {74, 1, "identifier"},   {75, 1, "punctuation"},  {77, 1, "identifier"},
        {79, 2, "punctuation"},  {82, 1, "identifier"},   {83, 1, "punctuation"},  {85, 1, "identifier"},
        {87, 2, "punctuation"},  {90, 1, "identifier"},   {91, 1, "punctuation"},  {93, 1, "identifier"},
        {95, 4, "punctuation"},  {100, 1, "identifier"},  {101, 1, "punctuation"}, {103, 1, "identifier"},
        {104, 2, "punctuation"}, {106, 1, "punctuation"}, {108, 1, "identifier"},  {110, 1, "punctuation"},
        {112, 1, "identifier"},  {113, 1, "punctuation"}, {115, 3, "keyword"},     {119, 4, "identifier"},
        {123, 1, "punctuation"}, {125, 1, "punctuation"}, {126, 6, "keyword"},     {132, 1, "punctuation"},
        {133, 1, "punctuation"}, {134, 1, "punctuation"}, {135, 1, "punctuation"}, {137, 1, "punctuation"},
        {138, 1, "identifier"},  {139, 1, "punctuation"}, {140, 1, "punctuation"}, {142, 3, "keyword"},
        {146, 1, "identifier"},  {147, 1, "punctuation"}, {148, 1, "punctuation"}, {149, 1, "punctuation"},
        {150, 1, "punctuation"}, {152, 1, "punctuation"}, {153, 1, "identifier"},  {154, 1, "punctuation"},
        {155, 1, "punctuation"}, {157, 1, "identifier"},  {158, 2, "punctuation"}, {160, 1, "identifier"},
        {161, 1, "punctuation"}, {163, 1, "identifier"},  {165, 2, "punctuation"}, {168, 1, "identifier"},
        {169, 1, "punctuation"}, {171, 1, "identifier"},  {173, 3, "punctuation"}, {177, 1, "identifier"},
        {178, 1, "punctuation"}};
    // NOLINTEND(readability-magic-numbers)
    ASSERT_EQ(result.size(), expectedResult.size());
    for (size_t i = 0; i < result.size(); i++) {
        auto expectedStart = std::get<0>(expectedResult.at(i));
        auto expectedLength = std::get<1>(expectedResult.at(i));
        auto expectedName = std::get<2>(expectedResult.at(i));
        ASSERT_EQ(result.at(i)->start, expectedStart);
        ASSERT_EQ(result.at(i)->length, expectedLength);
        ASSERT_EQ(result.at(i)->name, expectedName);
    }
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSemanticClassifications1)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("class-name.ets", ES2PANDA_STATE_CHECKED, "class Foo{};\nlet a:Foo = new Foo();");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const start = 19;
    size_t const length = 3;
    auto result = ark::es2panda::lsp::GetSemanticClassifications(ctx, start, length);

    const size_t expectedCount = 1;
    std::string expectedName = "class name";
    ASSERT_EQ(result.size(), expectedCount);
    ASSERT_EQ(result.at(0)->start, start);
    ASSERT_EQ(result.at(0)->length, length);
    ASSERT_EQ(result.at(0)->name, expectedName);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSemanticClassifications2)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("class-name.ets", ES2PANDA_STATE_CHECKED, "class Foo{};\nlet a:Foo = new Foo();");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const start = 29;
    size_t const length = 3;
    auto result = ark::es2panda::lsp::GetSemanticClassifications(ctx, start, length);

    const size_t expectedCount = 1;
    std::string expectedName = "class name";
    ASSERT_EQ(result.size(), expectedCount);
    ASSERT_EQ(result.at(0)->start, start);
    ASSERT_EQ(result.at(0)->length, length);
    ASSERT_EQ(result.at(0)->name, expectedName);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSemanticClassifications3)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx = initializer.CreateContext("enum-name.ets", ES2PANDA_STATE_CHECKED,
                                                      "enum Color {Red, Blue, Green};\nlet a:Color");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const start = 37;
    size_t const length = 5;
    auto result = ark::es2panda::lsp::GetSemanticClassifications(ctx, start, length);
    const size_t expectedCount = 1;
    std::string expectedName = "enum name";
    ASSERT_EQ(result.size(), expectedCount);
    ASSERT_EQ(result.at(0)->start, start);
    ASSERT_EQ(result.at(0)->length, length);
    ASSERT_EQ(result.at(0)->name, expectedName);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSemanticClassifications4)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("interface-name.ets", ES2PANDA_STATE_CHECKED, "interface I {};\nlet a:I");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const start = 22;
    size_t const length = 1;
    auto result = ark::es2panda::lsp::GetSemanticClassifications(ctx, start, length);
    const size_t expectedCount = 1;
    std::string expectedName = "interface name";
    ASSERT_EQ(result.size(), expectedCount);
    ASSERT_EQ(result.at(0)->start, start);
    ASSERT_EQ(result.at(0)->length, length);
    ASSERT_EQ(result.at(0)->name, expectedName);
    initializer.DestroyContext(ctx);
}

TEST_F(LspClassificationTests, GetSemanticClassifications5)
{
    Initializer initializer = Initializer();
    es2panda_Context *ctx =
        initializer.CreateContext("type-alias-name.ets", ES2PANDA_STATE_CHECKED, "type tmp = Long|null;\nlet a:tmp");
    ASSERT_EQ(ContextState(ctx), ES2PANDA_STATE_CHECKED);

    size_t const start = 28;
    size_t const length = 3;
    auto result = ark::es2panda::lsp::GetSemanticClassifications(ctx, start, length);
    const size_t expectedCount = 1;
    std::string expectedName = "type alias name";
    ASSERT_EQ(result.size(), expectedCount);
    ASSERT_EQ(result.at(0)->start, start);
    ASSERT_EQ(result.at(0)->length, length);
    ASSERT_EQ(result.at(0)->name, expectedName);
    initializer.DestroyContext(ctx);
}
