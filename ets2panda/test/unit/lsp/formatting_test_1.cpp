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

class LSPFormattingTests1 : public LSPAPITests {};

TEST_F(LSPFormattingTests1, GetFormatContextTest)
{
    ark::es2panda::lsp::FormatCodeSettings settings;

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    EXPECT_NE(&formatContext, nullptr);
}

TEST_F(LSPFormattingTests1, FormatDocument_DeleteSpaceBeforeRightBrace_WhenSettingFalse)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function foo() {
let obj = { a: 1, b: 2 };
}
)";

    auto tempFiles = CreateTempFile({"delete_space_before_right_brace.ets"}, {testCode});
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

TEST_F(LSPFormattingTests1, FormatDocument_InsertSpaceInsideEmptyBraces_WhenSettingFalse)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function emptyFn() {}
let emptyObj = {};
class A {
method() {}
}
)";

    auto tempFiles = CreateTempFile({"insert_space_inside_empty_braces.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingEmptyBraces(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocument_DeleteSpaceInsideEmptyBraces_WhenSettingFalse)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function emptyFn() { }
let emptyObj = { };
class A {
method() { }
}
)";

    auto tempFiles = CreateTempFile({"delete_space_inside_empty_braces.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingEmptyBraces(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocument_InsertSpaceAfterSemicolonInFor_WhenSettingTrue)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
for (let i = 0;i < 10;i++) {
console.log(i);
}
)";

    auto tempFiles = CreateTempFile({"insert_space_after_semicolon.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterSemicolonInForStatements(true);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocument_DeleteSpaceAfterSemicolonInFor_WhenSettingFalse)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
for (let i = 0; i < 10; i++) {
console.log(i);
}
)";

    auto tempFiles = CreateTempFile({"delete_space_after_semicolon.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterSemicolonInForStatements(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocument_InsertSpaceBeforeFunctionParenthesis_WhenSettingTrue)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function foo() {
return 42;
}
)";

    auto tempFiles = CreateTempFile({"insert_space_before_fn_paren.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceBeforeFunctionParenthesis(true);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocument_DeleteSpaceBeforeFunctionParenthesis_WhenSettingFalse)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function foo () {
return 42;
}
)";

    auto tempFiles = CreateTempFile({"delete_space_before_fn_paren.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceBeforeFunctionParenthesis(false);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocument_InsertNewlineBeforeOpenBraceInControlBlock_WhenSettingTrue)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
if (x > 0) {
doSomething();
}
)";

    auto tempFiles = CreateTempFile({"insert_newline_before_brace.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetPlaceOpenBraceOnNewLineForControlBlocks(true);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocument_InsertNewlineBeforeOpenBraceInFunctionDecl_WhenSettingTrue)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function foo(x: number) {
return x + 1;
}
)";

    auto tempFiles = CreateTempFile({"insert_newline_before_brace_function.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetPlaceOpenBraceOnNewLineForFunctions(true);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocument_InsertNewlineBeforeOpenBraceInTypeScriptDecl_WhenSettingTrue)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
interface IExample {
prop: string;
}

class Example {
method(): void {}
}

namespace NS {
export let x = 1;
}
)";

    auto tempFiles = CreateTempFile({"insert_newline_before_brace_ts_decl.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer;
    es2panda_Context *ctx = initializer.CreateContext(tempFiles[0].c_str(), ES2PANDA_STATE_PARSED);
    ASSERT_NE(ctx, nullptr);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetPlaceOpenBraceOnNewLineForFunctions(true);

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());

    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentColonSpacingTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
function typeTest(param:string,value:number):boolean{
let result:boolean=false;
return result;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_colon_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentBinaryOperatorTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
function mathTest(a:number,b:number):number{
let c:number=3;
let d:number=4;
let result:number=a+b*c-d;
return result;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_binary_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentFunctionSpacingTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function normalFunction(param:string):number{return 5;}
function getTest():string{return"test";}
function setTest(value:string):void{}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_function_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentBraceSpacingTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function braceTest():number{
// CC-OFFNXT(G.FMT.16-CPP) test logic
if(true){
let result:number=10;
return result;
// CC-OFFNXT(G.FMT.16-CPP) test logic
}else{
return 5;
}
}
// CC-OFFNXT(G.FMT.16-CPP) test logic
class TestClass{
private value:number;
constructor(){this.value=10;}
method():number{return this.value;}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_brace_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentUnaryOperatorTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function unaryTest(val:number):number{
let a:number=++val;
let b:number=val++;
let c:number=--val;
let d:number=val--;
return a+b+c+d;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_unary_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentArrowFunctionTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
let arrow1=(x:number)=>x*2;
// CC-OFFNXT(G.FMT.16-CPP) test logic
let arrow2=(a:number,b:number)=>{return a+b;};
let arrow3=(param:number)=>param+5;
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_arrow_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentKeywordSpacingTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function keywordTest():number{
let var1:number=10;
const var2:number=20;
return var1+var2;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_keyword_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentGenericTypeTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
class GenericTest<T,U>{
method<V>(param:T):U{
// CC-OFFNXT(G.FMT.16-CPP) test logic
let array:Array<string>=[];
// CC-OFFNXT(G.FMT.16-CPP) test logic
let map:Map<string,number>=new Map<string,number>();
return param as U;
}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_generic_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentImportExportTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
class TestClass{
static staticMethod():void{}
public publicMethod():void{}
private privateMethod():void{}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_import_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentDotAndOptionalTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function dotTest():string{
let numVal:number=123.456;
return"test";
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_dot_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentControlFlowTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
function controlTest(condition:boolean):void{     
// CC-OFFNXT(G.FMT.16-CPP) test logic
if(condition){
let x:number=1;
// CC-OFFNXT(G.FMT.16-CPP) test logic
}else{
let y:number=2;
}
// CC-OFFNXT(G.FMT.16-CPP) test logic
while(condition){
condition=false;
}
// CC-OFFNXT(G.FMT.16-CPP) test logic
for(let i:number=0;i<10;i++){
let z:number=i;
}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_control_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentSpreadOperatorTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function spreadTest():number{
let a:number=1;
let b:number=2;
let c:number=3;
return a+b+c;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_spread_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentNonNullAssertionTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function assertionTest(value:string):number{
let result:number=5;
return result;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_assertion_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentConstructorTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
class ConstructorTest{
private value:number;
public name:string;
// CC-OFFNXT(G.FMT.16-CPP) test logic
constructor(value:number,name:string){
this.value=value;
this.name=name;
}

getValue():number{return this.value;}
setValue(val:number):void{this.value=val;}
// CC-OFFNXT(G.FMT.16-CPP) test logic
static create():ConstructorTest{
// CC-OFFNXT(G.FMT.16-CPP) test logic
let instance:ConstructorTest=new ConstructorTest(10,"test");
return instance;
}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_constructor_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentInterfaceTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
interface TestInterface{
name:string;
method():void;
}
// CC-OFFNXT(G.FMT.16-CPP) test logic
enum TestEnum{FIRST,SECOND,THIRD}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_interface_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentCommaSpacingTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
function commaTest(a:number,b:string,c:boolean):number{
return a;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_comma_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests1, FormatDocumentSemicolonTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function semicolonTest():void{
let a:number=10;let b:number=20;
// CC-OFFNXT(G.FMT.16-CPP) test logic
if(true)return;
// CC-OFFNXT(G.FMT.16-CPP) test logic
for(let i:number=0;i<10;i++){
let x:number=i;
}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_semicolon_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_FALSE(changes.empty());
    initializer.DestroyContext(ctx);
}

}  // namespace