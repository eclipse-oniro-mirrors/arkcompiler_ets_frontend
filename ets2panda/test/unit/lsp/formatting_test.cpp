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

#include "lsp/include/formatting/formatting.h"
#include "lsp/include/formatting/formatting_settings.h"
#include "lsp_api_test.h"
#include <gtest/gtest.h>

namespace {

class LSPFormattingTests : public LSPAPITests {};

TEST_F(LSPFormattingTests, GetFormatContextTest)
{
    ark::es2panda::lsp::FormatCodeSettings settings;

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    EXPECT_NE(&formatContext, nullptr);
}

TEST_F(LSPFormattingTests, FormatDocumentQuestionMarkTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function conditionalTest(value:number):number{
return value>0?value:-value;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_question_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_ControlBlockBraceSpacingTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function test(x: number) {
if (x)
{
x++;
}
}
)";

    auto tempFiles = CreateTempFile({"control_block_spacing.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetPlaceOpenBraceOnNewLineForControlBlocks(false);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);
    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_FunctionBraceSpacingTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function add(x: number, y: number)
{
return x + y;
}
)";

    auto tempFiles = CreateTempFile({"function_brace_spacing.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetPlaceOpenBraceOnNewLineForFunctions(false);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);
    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_TypeScriptDeclBraceSpacingTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
class MyClass
{
name: string;
}

interface MyInterface
{
age: number;
}
)";

    auto tempFiles = CreateTempFile({"ts_decl_brace_spacing.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetPlaceOpenBraceOnNewLineForFunctions(false);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);
    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_RemoveSpaceAfterOpenBraceTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function x() {  return 1; }
)";

    auto tempFiles = CreateTempFile({"remove_space_after_brace.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);
    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_InsertSpaceAfterParensAndKeywordsTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
function x(x: int) {
// CC-OFFNXT(G.FMT.16-CPP) test logic
if (x){
x++;
}
// CC-OFFNXT(G.FMT.16-CPP) test logic
do{
x--;
} while (x > 0);

switch (x) {
case 1:{
break;
}
default:{
break;
}
}
}
)";

    auto tempFiles = CreateTempFile({"paren_keyword_spacing.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);
    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_DeleteSpaceAfterConstructorTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
class A {
constructor () {}
}
)";

    auto tempFiles = CreateTempFile({"delete_space_after_ctor.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterConstructor(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_DeleteSpaceAfterConstructor_WhenOptionFalse)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
class A {
constructor () {}
}
)";

    auto tempFiles = CreateTempFile({"delete_space_ctor_opt_false.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterConstructor(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_InsertSpaceAfterComma_WhenInsertSpaceAfterConstructorIsTrue)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
class A {
// CC-OFFNXT(G.FMT.16-CPP) test logic
constructor(a: number,b: number) {}
}
)";

    auto tempFiles = CreateTempFile({"insert_space_after_comma_true.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterConstructor(true);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_InsertSpaceAfterCommaDelimiterTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
let array = [1,2,3];
// CC-OFFNXT(G.FMT.16-CPP) test logic
let fn = (a: number,b: number) => {};
)";

    auto tempFiles = CreateTempFile({"comma_delimiter_true.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterCommaDelimiter(true);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_DeleteSpaceAfterCommaDelimiterTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
let arr = [1, 2, 3];
let fn = (a: number, b: number) => {};
)";

    auto tempFiles = CreateTempFile({"delete_comma_space_false.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterCommaDelimiter(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_InsertSpaceAfterFunctionKeywordForAnonymousFunctionsTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
let x = function(){ return 1; };
let y = function*(){ yield 2; };
)";

    auto tempFiles = CreateTempFile({"anon_function_space.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterFunctionKeywordForAnonymousFunctions(true);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_DeleteSpaceAfterFunctionKeywordForAnonymousFunctionsTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
let x = function (){ return 1; };
let y = function* (){ yield 2; };
)";

    auto tempFiles = CreateTempFile({"anon_function_remove_space.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterFunctionKeywordForAnonymousFunctions(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_InsertSpaceAfterControlFlowKeywordsTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
if(true){ doSomething(); }
// CC-OFFNXT(G.FMT.16-CPP) test logic
while(true){ run(); }
// CC-OFFNXT(G.FMT.16-CPP) test logic
for(let i = 0;i<10;++i){ process(); }
// CC-OFFNXT(G.FMT.16-CPP) test logic
switch(value){ case 1: break; }
)";

    auto tempFiles = CreateTempFile({"control_keyword_insert_space.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterKeywordsInControlFlowStatements(true);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_DeleteSpaceAfterControlFlowKeywordsTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
if (true) { doSomething(); }
while (true) { run(); }
for (let i = 0; i < 10; ++i) { process(); }
switch (value) { case 1: break; }
)";

    auto tempFiles = CreateTempFile({"control_keyword_delete_space.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterKeywordsInControlFlowStatements(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_InsertSpaceBeforeClosingParen_ControlDecl)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
if (a){
return;
}
// CC-OFFNXT(G.FMT.16-CPP) test logic
while(x){
continue;
}
)";

    auto tempFiles = CreateTempFile({"space_before_closing_paren.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_InsertSpaceBeforeRightParenthesis_WhenSettingFalse_AndInControlFlow)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
if (x) {
let y = 5;
}
)";

    auto tempFiles = CreateTempFile({"insert_space_before_right_paren.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_InsertSpaceBetweenDoubleLeftParentheses_WhenSettingFalse)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
let fn = ((x)) => x;
)";

    auto tempFiles = CreateTempFile({"double_left_paren_insert_space.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_DeleteSpaceBeforeRightParenthesis_WhenSettingFalse_AndInControlDecl)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
if (a > b ) {
doSomething();
}
)";

    auto tempFiles = CreateTempFile({"control_decl_right_paren_space.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_InsertSpaceAfterOpeningBracket_WhenSettingTrue)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
let x = [ 1, 2];
)";

    auto tempFiles = CreateTempFile({"insert_space_after_bracket.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets(true);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_InsertSpaceBeforeClosingBracketInControlStructure_WhenSettingTrue)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
if ([1, 2].length > 0) {
doSomething();
}
)";

    auto tempFiles = CreateTempFile({"insert_space_before_bracket.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets(true);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_DeleteSpaceAfterOpeningBracket_WhenSettingFalse)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
let a = [1, 2];
)";

    auto tempFiles = CreateTempFile({"delete_space_after_bracket.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_DeleteSpaceBeforeClosingBracket_WhenSettingFalse_AndControlContext)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
if ([1, 2 ]) {
doSomething();
}
)";

    auto tempFiles = CreateTempFile({"delete_space_before_bracket.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_InsertSpaceBeforeRightParenthesis_InControlContext_WhenSettingFalse)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
if (x > 0){
doSomething();
}
)";

    auto tempFiles = CreateTempFile({"insert_space_before_right_paren_control.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_DeleteSpaceBeforeRightParenthesisInIfCondition_WhenSettingFalse)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
if (x > 0 ) {
print(x);
}
)";

    auto tempFiles = CreateTempFile({"delete_space_before_right_paren_if.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}
TEST_F(LSPFormattingTests, FormatDocument_InsertSpaceAfterLeftSquareBracket_WhenSettingTrue)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
let arr = [1, 2, 3];
)";

    auto tempFiles = CreateTempFile({"insert_space_after_left_bracket.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets(true);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_InsertSpaceBeforeRightSquareBracket_WhenSettingTrueAndInControlDecl)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
if (arr[0]) {
print(arr[1]);
}
)";

    auto tempFiles = CreateTempFile({"insert_space_before_right_bracket_control.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets(true);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_DeleteSpaceAfterLeftSquareBracket_WhenSettingFalse)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
let arr = [1, 2, 3];
let x = arr[ 0];
)";

    auto tempFiles = CreateTempFile({"delete_space_after_left_bracket.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_DeleteSpaceBeforeRightSquareBracket_WhenSettingFalse_InsideControlDecl)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
if (arr[0 ]) {
print("ok");
}
)";

    auto tempFiles = CreateTempFile({"delete_space_before_right_bracket.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}
TEST_F(LSPFormattingTests, InsertSpaceAfterLeftSquareBracket_AndBraceWrappedContext)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
let list = [{ a: 1 }, { b: 2 }];
)";

    auto tempFiles = CreateTempFile({"insert_space_after_left_bracket_brace_context.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBraces(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, WhenBracesSettingFalse_AndBraceWrappedContext)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
let list = [{ a: 1}];
)";

    auto tempFiles = CreateTempFile({"insert_space_before_right_bracket_brace_context.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBraces(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_DeleteSpaceBetweenBraces_WhenSettingFalse_AndObjectContext)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
let obj = { };
)";

    auto tempFiles = CreateTempFile({"delete_space_between_braces_object_context.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBraces(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests, FormatDocument_DeleteSpaceAfterLeftBrace_WhenSettingFalse)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function foo() {
let obj = { a: 1, b: 2 };
}
)";

    auto tempFiles = CreateTempFile({"delete_space_after_left_brace.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingNonemptyBraces(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

}  // namespace