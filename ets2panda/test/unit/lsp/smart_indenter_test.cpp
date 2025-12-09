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

#include <gtest/gtest.h>
#include <string>
#include "lsp/include/formatting/smart_indenter.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"
#include "lsp_api_test.h"

namespace {
using ark::es2panda::lsp::FormatCodeSettings;
using ark::es2panda::lsp::GetIndentation;
using ark::es2panda::lsp::IndentStyle;
using ark::es2panda::lsp::Initializer;
using ark::es2panda::lsp::ONE;
using ark::es2panda::lsp::ZERO;

class SmartIndenterTests : public LSPAPITests {
public:
    struct TestResult {
        bool isValid;
        size_t indentation;
    };

    static size_t GetExpectedBaseIndent()
    {
        FormatCodeSettings defaultSettings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
        return defaultSettings.GetBaseIndentSize();
    }

    TestResult RunIndentationTest(const std::string &code, size_t position,
                                  const FormatCodeSettings &settings = FormatCodeSettings(),
                                  const std::string &fileName = "smart_indenter_test.ets")
    {
        std::vector<std::string> files = {fileName};
        std::vector<std::string> texts = {code};
        auto filePaths = CreateTempFile(files, texts);
        if (filePaths.empty()) {
            return {false, ZERO};
        }

        Initializer init;
        es2panda_Context *ctx = init.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_PARSED);
        if (ctx == nullptr) {
            return {false, ZERO};
        }

        FormatCodeSettings testSettings = settings;
        if (testSettings.GetIndentSize() == ZERO) {
            testSettings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
        }

        auto result = GetIndentation(ctx, position, testSettings);
        init.DestroyContext(ctx);

        return {result.isValid, result.indentation};
    }
};

TEST_F(SmartIndenterTests, EmptyFileIndentation)
{
    const std::string code;
    auto result = RunIndentationTest(code, ZERO, FormatCodeSettings(), "smart_indenter_empty.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());
}

TEST_F(SmartIndenterTests, StringLiteralNoIndentation)
{
    const std::string code = R"(const str = "hello
world";
)";
    constexpr std::string_view HELLO_MARKER = "hello";
    size_t position = code.find(HELLO_MARKER.data()) + HELLO_MARKER.length();
    auto result = RunIndentationTest(code, position, FormatCodeSettings(), "smart_indenter_string.ets");
    EXPECT_FALSE(result.isValid);
    EXPECT_EQ(result.indentation, ZERO);
}

TEST_F(SmartIndenterTests, IndentStyleNone)
{
    const std::string code = "function test() {\n}";
    FormatCodeSettings settings = ark::es2panda::lsp::GetDefaultFormatCodeSettings("\n");
    settings.SetIndentStyle(IndentStyle::NONE);

    auto result = RunIndentationTest(code, code.find('{') + ONE, settings, "smart_indenter_none.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, ZERO);
}

TEST_F(SmartIndenterTests, FixBrokenIfStatementIndentation)
{
    const std::string code = R"(function calculate(x: number): number {
if (x > 0) {
return x * 2;
} else {
return x * -1;
}
}
)";
    constexpr std::string_view IF_MARKER = "if (x > 0) {";
    size_t positionAfterIf = code.find(IF_MARKER.data()) + IF_MARKER.length();
    auto result = RunIndentationTest(code, positionAfterIf, FormatCodeSettings(), "smart_indenter_fix_if.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());

    constexpr std::string_view ELSE_MARKER = "} else {";
    size_t positionAfterElse = code.find(ELSE_MARKER.data()) + ELSE_MARKER.length();
    result = RunIndentationTest(code, positionAfterElse, FormatCodeSettings(), "smart_indenter_fix_else.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());
}

TEST_F(SmartIndenterTests, FixBrokenSwitchStatementIndentation)
{
    const std::string code = R"(function processValue(val: number): string {
switch (val) {
case 1:
return "one";
case 2:
return "two";
default:
return "other";
}
}
)";
    constexpr std::string_view SWITCH_MARKER = "switch (val) {";
    size_t positionAfterSwitch = code.find(SWITCH_MARKER.data()) + SWITCH_MARKER.length();
    auto result = RunIndentationTest(code, positionAfterSwitch, FormatCodeSettings(), "smart_indenter_fix_switch.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());
}

TEST_F(SmartIndenterTests, FixBrokenForLoopIndentation)
{
    const std::string code = R"(function sumArray(arr: number[]): number {
let sum = 0;
for (let i = 0; i < arr.length; i++) {
sum += arr[i];
}
return sum;
}
)";
    constexpr std::string_view FOR_MARKER = "for (let i = 0; i < arr.length; i++) {";
    size_t positionInsideFor = code.find(FOR_MARKER.data()) + FOR_MARKER.length();
    auto result = RunIndentationTest(code, positionInsideFor, FormatCodeSettings(), "smart_indenter_fix_for.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());
}

TEST_F(SmartIndenterTests, FixBrokenWhileLoopIndentation)
{
    const std::string code = R"(function countdown(n: number): void {
while (n > 0) {
console.log(n);
n--;
}
}
)";
    constexpr std::string_view WHILE_MARKER = "while (n > 0) {";
    size_t positionInsideWhile = code.find(WHILE_MARKER.data()) + WHILE_MARKER.length();
    auto result = RunIndentationTest(code, positionInsideWhile, FormatCodeSettings(), "smart_indenter_fix_while.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());
}

TEST_F(SmartIndenterTests, FixBrokenNestedBlocksIndentation)
{
    const std::string code = R"(function complexLogic(x: number): number {
if (x > 0) {
for (let i = 0; i < x; i++) {
if (i % 2 == 0) {
x += i;
}
}
}
return x;
}
)";
    constexpr std::string_view INNER_IF_MARKER = "if (i % 2 == 0) {";
    size_t positionInnerIf = code.find(INNER_IF_MARKER.data()) + INNER_IF_MARKER.length();
    auto result = RunIndentationTest(code, positionInnerIf, FormatCodeSettings(), "smart_indenter_fix_nested.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());
}

TEST_F(SmartIndenterTests, FixBrokenClassIndentation)
{
    const std::string code = R"(class Calculator {
private value: number;

constructor(initial: number) {
this.value = initial;
}

add(x: number): void {
this.value += x;
}

getValue(): number {
return this.value;
}
}
)";
    constexpr std::string_view CONSTRUCTOR_MARKER = "constructor(initial: number) {";
    size_t positionAfterConstructor = code.find(CONSTRUCTOR_MARKER.data()) + CONSTRUCTOR_MARKER.length();
    auto result =
        RunIndentationTest(code, positionAfterConstructor, FormatCodeSettings(), "smart_indenter_fix_class1.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());

    constexpr std::string_view ADD_MARKER = "add(x: number): void {";
    size_t positionAfterAdd = code.find(ADD_MARKER.data()) + ADD_MARKER.length();
    result = RunIndentationTest(code, positionAfterAdd, FormatCodeSettings(), "smart_indenter_fix_class2.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());
}

TEST_F(SmartIndenterTests, FixBrokenTryCatchIndentation)
{
    const std::string code = R"(function parseData(data: string): number {
try {
let result = parseInt(data);
return result;
} catch (e) {
console.log(e);
return 0;
}
}
)";
    constexpr std::string_view TRY_MARKER = "try {";
    size_t positionInsideTry = code.find(TRY_MARKER.data()) + TRY_MARKER.length();
    auto result = RunIndentationTest(code, positionInsideTry, FormatCodeSettings(), "smart_indenter_fix_try.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());

    constexpr std::string_view CATCH_MARKER = "} catch (e) {";
    size_t positionInsideCatch = code.find(CATCH_MARKER.data()) + CATCH_MARKER.length();
    result = RunIndentationTest(code, positionInsideCatch, FormatCodeSettings(), "smart_indenter_fix_catch.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());
}

TEST_F(SmartIndenterTests, FixBrokenObjectMethodsIndentation)
{
    const std::string code = R"(const service = {
name: "DataService",
fetchData(): void {
console.log("Fetching data");
},
processData(data: string): void {
console.log(data);
}
};
)";
    constexpr std::string_view FETCH_MARKER = "fetchData(): void {";
    size_t positionInsideFetch = code.find(FETCH_MARKER.data()) + FETCH_MARKER.length();
    auto result = RunIndentationTest(code, positionInsideFetch, FormatCodeSettings(), "smart_indenter_fix_obj1.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());
}

TEST_F(SmartIndenterTests, FixBrokenEnumIndentation)
{
    const std::string code = R"(enum Status {
Active,
Inactive,
Pending
}
)";
    size_t positionInActive = code.find("Pending");
    auto result = RunIndentationTest(code, positionInActive + ONE, FormatCodeSettings(), "smart_indenter_fix_enum.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());
}

TEST_F(SmartIndenterTests, FixBrokenArrowFunctionIndentation)
{
    const std::string code = R"(const calculate = (x: number, y: number) => {
if (x > y) {
return x - y;
} else {
return y - x;
}
};
)";
    constexpr std::string_view ARROW_MARKER = "if (x > y) {";
    size_t positionInsideArrow = code.find(ARROW_MARKER.data()) + ARROW_MARKER.length();
    auto result = RunIndentationTest(code, positionInsideArrow, FormatCodeSettings(), "smart_indenter_fix_arrow.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());
}

TEST_F(SmartIndenterTests, FixBrokenArrayMethodIndentation)
{
    const std::string code = R"(function processItems(items: number[]): number[] {
return items.filter(x => {
return x > 0;
}).map(x => {
return x * 2;
});
}
)";
    constexpr std::string_view FILTER_MARKER = "return items.filter(x => {";
    size_t positionInsideFilter = code.find(FILTER_MARKER.data()) + FILTER_MARKER.length();
    auto result = RunIndentationTest(code, positionInsideFilter, FormatCodeSettings(), "smart_indenter_fix_array.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());
}

TEST_F(SmartIndenterTests, BasicFunctionIndentation)
{
    const std::string code = R"(function test() {
}
)";
    auto result = RunIndentationTest(code, code.find('{') + ONE, FormatCodeSettings(), "smart_indenter_basic.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());
}

TEST_F(SmartIndenterTests, ObjectLiteralIndentation)
{
    const std::string code = R"(const obj = {
}
)";
    auto result = RunIndentationTest(code, code.find('{') + ONE, FormatCodeSettings(), "smart_indenter_object.ets");
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.indentation, GetExpectedBaseIndent());
}

}  // namespace
