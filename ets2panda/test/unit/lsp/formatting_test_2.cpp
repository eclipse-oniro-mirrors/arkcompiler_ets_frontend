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

class LSPFormattingTests2 : public LSPAPITests {};

std::string ApplyChanges(const std::string &source, const std::vector<TextChange> &changes)
{
    std::string result = source;
    std::vector<TextChange> sortedChanges = changes;
    std::sort(sortedChanges.begin(), sortedChanges.end(),
              [](const TextChange &a, const TextChange &b) { return a.span.start > b.span.start; });
    for (const auto &change : sortedChanges) {
        result.replace(change.span.start, change.span.length, change.newText);
    }
    return result;
}

TEST_F(LSPFormattingTests2, GetFormatContextTest)
{
    ark::es2panda::lsp::FormatCodeSettings settings;

    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);
    EXPECT_NE(&formatContext, nullptr);
}

TEST_F(LSPFormattingTests2, FormatDocumentModuleTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
namespace TestNamespace{
interface NamespaceInterface{}
const value:number=10;
function moduleFunction():void{}
// CC-OFFNXT(G.FMT.16-CPP) test logic
class ModuleClass{}
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
namespace TestNamespace{
interface NamespaceInterface { }
const value: number = 10;
function moduleFunction(): void { }
// CC-OFFNXT(G.FMT.16-CPP) test logic
class ModuleClass { }
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_module_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterOpeningAndBeforeClosingEmptyBraces(true);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentParenthesesTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function parenthesesTest():number{
let a:number=10;
let b:number=20;
let c:number=30;
let d:number=40;
let condition1:boolean=true;
let condition2:boolean=false;
let condition3:boolean=true;
let condition4:boolean=false;
let result:number=(a+b)*(c-d);
// CC-OFFNXT(G.FMT.16-CPP) test logic
if((condition1&&condition2)||(condition3||condition4)){
let x:number=1;
}
return result;
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
function parenthesesTest(): number {
let a: number = 10;
let b: number = 20;
let c: number = 30;
let d: number = 40;
let condition1: boolean = true;
let condition2: boolean = false;
let condition3: boolean = true;
let condition4: boolean = false;
let result: number = (a + b) * (c - d);
// CC-OFFNXT(G.FMT.16-CPP) test logic
if ((condition1 && condition2) || (condition3 || condition4)) {
let x: number = 1;
}
return result;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_parentheses_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentReturnSpacingTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function returnTest():number{
return 42;
}
function returnVoidTest():void{
return;
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
function returnTest(): number {
return 42;
}
function returnVoidTest(): void {
return;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_return_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentLetConstSpacingTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function varTest():number{
let value1:number=10;
const value2:number=20;
return value1+value2;
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
function varTest(): number {
let value1: number = 10;
const value2: number = 20;
return value1 + value2;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_letconst_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentBinaryKeywordTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function binaryKeywordTest(obj:object):boolean{
let result:boolean=obj instanceof Object;
return result;
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
function binaryKeywordTest(obj: object): boolean {
let result: boolean = obj instanceof Object;
return result;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_binarykeyword_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentDeleteOperatorTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function deleteTest():void{
let obj:object={};
delete obj;
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
function deleteTest(): void {
let obj: object = {};
delete obj;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_delete_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentTypeofOperatorTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function typeofTest(value:object):string{
let type:string=typeof value;
return type;
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
function typeofTest(value: object): string {
let type: string = typeof value;
return type;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_typeof_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentNewOperatorTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
class TestClass{
constructor(){}
}
function newTest():TestClass{
let instance:TestClass=new TestClass();
return instance;
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
class TestClass {
constructor() {}
}
function newTest(): TestClass {
let instance: TestClass = new TestClass();
return instance;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_new_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentStaticKeywordTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
class StaticTest{
static value:number=10;
static method():number{
return StaticTest.value;
}
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
class StaticTest {
static value: number = 10;
static method(): number {
return StaticTest.value;
}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_static_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentAccessModifierTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
class AccessTest{
public publicField:number;
private privateField:number;
protected protectedField:number;
public constructor(){
this.publicField=1;
this.privateField=2;
this.protectedField=3;
}
public publicMethod():void{}
private privateMethod():void{}
protected protectedMethod():void{}
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
class AccessTest {
public publicField: number;
private privateField: number;
protected protectedField: number;
public constructor() {
this.publicField = 1;
this.privateField = 2;
this.protectedField = 3;
}
public publicMethod(): void {}
private privateMethod(): void {}
protected protectedMethod(): void {}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_access_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentTypeAssertionTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function assertionTest():number{
let value:object=42;
let num:number=value as number;
return num;
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
function assertionTest(): number {
let value: object = 42;
let num: number = value as number;
return num;
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

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentThrowKeywordTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
class CustomError extends Error{
constructor(message:string){
super(message);
}
}
function throwTest():void{
throw new CustomError("error");
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
class CustomError extends Error {
constructor(message: string) {
super(message);
}
}
function throwTest(): void {
throw new CustomError("error");
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_throw_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentAbstractKeywordTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
abstract class AbstractTest{
abstract abstractMethod():void;
concrete():void{}
}
class ConcreteTest extends AbstractTest{
abstractMethod():void{}
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
abstract class AbstractTest {
abstract abstractMethod(): void;
concrete(): void {}
}
class ConcreteTest extends AbstractTest {
abstractMethod(): void {}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_abstract_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentReadonlyKeywordTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
class ReadonlyTest{
readonly readonlyField:number;
constructor(value:number){
this.readonlyField=value;
}
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
class ReadonlyTest {
readonly readonlyField: number;
constructor(value: number) {
this.readonlyField = value;
}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_readonly_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentNamespaceSpacingTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
namespace TestNamespace{
function namespaceFunction():void{}
// CC-OFFNXT(G.FMT.16-CPP) test logic
class NamespaceClass{}
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
// CC-OFFNXT(G.FMT.16-CPP) test logic
namespace TestNamespace{
function namespaceFunction(): void {}
// CC-OFFNXT(G.FMT.16-CPP) test logic
class NamespaceClass {}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_namespace_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentInterfaceSpacingTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
interface BaseInterface{
baseMethod():void;
}
interface ExtendedInterface extends BaseInterface{
extendedMethod():void;
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
interface BaseInterface {
baseMethod(): void;
}
interface ExtendedInterface extends BaseInterface {
extendedMethod(): void;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_interface_spacing_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentEnumSpacingTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
enum Color{
RED,
GREEN,
BLUE
}
function enumTest():Color{
return Color.RED;
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
enum Color {
RED,
GREEN,
BLUE
}
function enumTest(): Color {
return Color.RED;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_enum_spacing_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentClassExtendsTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
class BaseClass{
baseMethod():void{}
}
class DerivedClass extends BaseClass{
derivedMethod():void{}
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
class BaseClass {
baseMethod(): void {}
}
class DerivedClass extends BaseClass {
derivedMethod(): void {}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_extends_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentImplementsTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
interface TestInterface{
interfaceMethod():void;
}
class ImplementingClass implements TestInterface{
interfaceMethod():void{}
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
interface TestInterface {
interfaceMethod(): void;
}
class ImplementingClass implements TestInterface {
interfaceMethod(): void {}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_implements_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentFinalKeywordTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
class FinalTest{
final finalField:number=42;
final finalMethod():void{}
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
class FinalTest {
final finalField: number = 42;
final finalMethod(): void {}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_final_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentMultilineBlockTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function multilineTest():void{
if(true){
let a:number=1;
let b:number=2;
let c:number=a+b;
}
else{
let x:number=10;
let y:number=20;
let z:number=x*y;
}
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
function multilineTest(): void {
if (true) {
let a: number = 1;
let b: number = 2;
let c: number = a + b;
}
else {
let x: number = 10;
let y: number = 20;
let z: number = x * y;
}
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"format_multiline_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentAlreadyFormattedTest)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function alreadyFormatted(): void {
    let x: number = 10;
    let y: number = 20;
    if (x > y) {
        return;
    }
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"already_formatted.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_TRUE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentAlreadyFormattedTest2)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
class AlreadyFormatted {
    value: number = 10;
    constructor() {
        this.value = 20;
    }
    getValue(): number {
        return this.value;
    }
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"already_formatted2.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    EXPECT_TRUE(changes.empty());
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentInsertSpaceAfterTypeAssertionTest_True)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function assertionTest():number{
let value:object=42;
let num:number=value as number;
let str:string=value as string;
return num;
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
function assertionTest(): number {
let value: object = 42;
let num: number = value as number;
let str: string = value as string;
return num;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"type_assertion_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceAfterTypeAssertion(true);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentInsertSpaceBeforeTypeAnnotationTest_True)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function typeAnnotationTest():void{
let x:number=10;
let y:string="hello";
let z:boolean=true;
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
function typeAnnotationTest() : void {
let x : number = 10;
let y : string = "hello";
let z : boolean = true;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"type_annotation_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceBeforeTypeAnnotation(true);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPFormattingTests2, FormatDocumentBinaryOperators_False)
{
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string testCode = R"(
function binaryTest():number{
let a:number=10+20;
let b:number=a*2;
return a+b;
}
)";
    // CC-OFFNXT(G.FMT.16-CPP) test logic
    std::string expectedCode = R"(
function binaryTest(): number {
let a: number=10+20;
let b: number=a*2;
return a+b;
}
)";
    const int index0 = 0;
    auto tempFiles = CreateTempFile({"binary_operators_test.ets"}, {testCode});
    ASSERT_FALSE(tempFiles.empty());

    ark::es2panda::lsp::Initializer initializer = ark::es2panda::lsp::Initializer();
    es2panda_Context *ctx = initializer.CreateContext(tempFiles.at(index0).c_str(), ES2PANDA_STATE_PARSED);

    ark::es2panda::lsp::FormatCodeSettings settings;
    settings.SetInsertSpaceBeforeAndAfterBinaryOperators(false);
    auto formatContext = ark::es2panda::lsp::GetFormatContext(settings);

    auto changes = ark::es2panda::lsp::FormatDocument(ctx, formatContext);

    ASSERT_FALSE(changes.empty());
    std::string result = ApplyChanges(testCode, changes);
    ASSERT_EQ(result, expectedCode);
    initializer.DestroyContext(ctx);
}

}  // namespace
