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

#include "lsp_api_test.h"
#include "lsp/include/internal_api.h"

namespace {

using ark::es2panda::lsp::Initializer;

class LSPClassInfoTests : public LSPAPITests {};

void AssertClassConstructorInfo(const std::vector<FileTextChanges> &fileTextChanges,
                                const std::vector<FileTextChanges> &expectedFileTextChanges)
{
    auto emptyCheck = fileTextChanges.empty();
    ASSERT_FALSE(emptyCheck) << "The result is empty.";

    auto curFileChanges = fileTextChanges.at(0);
    auto expectedFileChanges = expectedFileTextChanges.at(0);
    bool check = false;
    if (curFileChanges.fileName != expectedFileChanges.fileName) {
        check = true;
    }
    ASSERT_FALSE(check) << "The fileName is not expected.";

    auto textChangeEmptyCheck = curFileChanges.textChanges.empty();
    ASSERT_FALSE(textChangeEmptyCheck) << "The modified file content is empty.";

    auto curTextChange = curFileChanges.textChanges.at(0);
    auto expectedTextChange = expectedFileChanges.textChanges.at(0);
    if (curTextChange.span.start != expectedTextChange.span.start) {
        check = true;
    }
    ASSERT_FALSE(check) << "The insertPosition is not expected.";
    if (curTextChange.newText != expectedTextChange.newText) {
        check = true;
    }
    ASSERT_FALSE(check) << "The newText is not expected.";
}

std::vector<FileTextChanges> CreateExpectedFileTextChanges(const std::string &expectedFileName, size_t expectedPosition,
                                                           const std::string &expectedText)
{
    std::vector<FileTextChanges> expectedFileTextChanges;

    TextSpan span(expectedPosition, expectedText.size());
    std::vector<TextChange> textChanges;
    textChanges.emplace_back(TextChange(span, expectedText));
    FileTextChanges textChange(expectedFileName, textChanges);
    expectedFileTextChanges.push_back(textChange);

    return expectedFileTextChanges;
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo1)
{
    std::vector<std::string> fileNames = {"getClassConstructorInfo1.ets"};
    std::vector<std::string> fileContents = {
        R"(
class FooParent {
    f: Number = 0;
    str: String = "aaa";
    constructor (f: Number, str: String) {
        this.f = f;
        this.str = str;
    }
};

enum Colors {Red = "#FF0000", Green = "#00FF00", Blue = "#0000FF"};
export class Foo extends FooParent {
    name: String = "unassigned";
    isActive: Boolean = true;
    items: String[] = ["aaa", "bbb"];
    point: Number[] = [0, 0];
    primaryColor: Colors = Colors.Blue;
    optionalValue?:String|null|undefined;
    x: Number = 1;
    static y: Number = 2;
    z: Number = 3;
};)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 400;
    std::vector<std::string> properties = {"name", "x", "primaryColor", "isActive", "items", "point", "optionalValue"};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText =
        "constructor(f: Number, str: String, name: String, x: Number, primaryColor: Colors, isActive: Boolean, "
        "items: Array<String>, point: Array<Number>, optionalValue: String | null | undefined) {\n  super(f, str);\n"
        "  this.name = name;\n  this.x = x;\n  this.primaryColor = primaryColor;\n  this.isActive = isActive;\n"
        "  this.items = items;\n  this.point = point;\n  this.optionalValue = optionalValue;\n}";
    size_t const expectedPosition = 269;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo2)
{
    std::vector<std::string> files = {"getClassConstructorInfo2.ets"};
    std::vector<std::string> texts = {
        R"(
class Foo {
    f: Number = 0;
    str: String = "aaa";
};)"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 30;
    std::vector<std::string> properties = {"f", "str"};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText = "constructor(f: Number, str: String) {\n  this.f = f;\n  this.str = str;\n}";
    size_t const expectedPosition = 17;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo3)
{
    std::vector<std::string> files = {"getClassConstructorInfo3.ets"};
    std::vector<std::string> texts = {
        R"(
class Foo {
};)"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 10;
    std::vector<std::string> properties = {};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText = "constructor() {\n}";
    size_t const expectedPosition = 13;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo4)
{
    std::vector<std::string> files = {"getClassConstructorInfo4.ets"};
    std::vector<std::string> texts = {
        R"(
namespace space {
    export class classInSpace {
        c: Number = 2;

        print(): Number {
            return 2;
        }
    }
})"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 35;
    std::vector<std::string> properties = {"c"};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText = "constructor(c: Number) {\n  this.c = c;\n}";
    size_t const expectedPosition = 59;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo5)
{
    std::vector<std::string> files = {"getClassConstructorInfo5.ets"};
    std::vector<std::string> texts = {
        R"(
interface Address {
    street: String;
    city: String;
}

class Engine {
    horsepower: Number;

    constructor(horsepower: Number) {
        this.horsepower = horsepower;
    }
}

enum Color {Red, Green, Blue}
class Car {
    name: String = "Alice";
    address: Address = { street: "111", city: "222" };
    tupleProperty: [String, Number] = ["TypeScript", 4.5];
    enumProperty: Color = Color.Red;
    onClick: () => void = () => console.log("Function called");
    engine: Engine = new Engine(1);
})"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 230;
    std::vector<std::string> properties = {"name", "address", "tupleProperty", "enumProperty", "onClick", "engine"};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText =
        "constructor(name: String, address: Address, tupleProperty: [String, Number], enumProperty: Color, "
        "onClick: (() => void), engine: Engine) {\n  this.name = name;\n  this.address = address;\n  "
        "this.tupleProperty = tupleProperty;\n  this.enumProperty = enumProperty;\n  this.onClick = onClick;\n  "
        "this.engine = engine;\n}";
    size_t const expectedPosition = 233;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo6)
{
    std::vector<std::string> files = {"getClassConstructorInfo6.ets"};
    std::vector<std::string> texts = {
        R"(
class Animal {
    name: String;
    num: Number
    constructor(name: String, num: Number) {
        this.name = name;
        this.num = num;
    }
};

class Mammal extends Animal {
    age: Number;
    flag: Boolean;
    constructor(name: String, num: Number, age: Number, flag: Boolean) {
        super(name, num);
        this.age = age;
        this.flag = flag;
    }
};

class Dog extends Mammal {
    breed: String;
})"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 406;
    std::vector<std::string> properties = {"breed"};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText =
        "constructor(name: String, num: Number, age: Number, flag: Boolean, breed: String) {\n  super(name, num, age, "
        "flag);\n  this.breed = breed;\n}";
    size_t const expectedPosition = 411;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo7)
{
    std::vector<std::string> files = {"getClassConstructorInfo7.ets"};
    std::vector<std::string> texts = {
        R"(
interface AA {
    aa: Number;

    getAA(): Number;
}

class BaseNN {
    aa: Number;

    constructor(aa: Number) {
        this.aa = aa;
    }

    getAA(): Number {
        return 1;
    }
}

class MM extends BaseNN implements AA {
    aa: Number;

    getAA(): Number {
        return super.getAA();
    }
})"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 230;
    std::vector<std::string> properties = {"aa"};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText = "constructor(aa: Number) {\n  super(aa);\n  this.aa = aa;\n}";
    size_t const expectedPosition = 241;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo8)
{
    std::vector<std::string> files = {"getClassConstructorInfo8.ets"};
    std::vector<std::string> texts = {
        R"(
abstract class AA {
    test: Number;
}

class NN extends AA {
    jkk: String = "";
})"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 70;
    std::vector<std::string> properties = {"jkk"};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText = "constructor(jkk: String) {\n  super();\n  this.jkk = jkk;\n}";
    size_t const expectedPosition = 68;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo9)
{
    std::vector<std::string> files = {"getClassConstructorInfo9.ets"};
    std::vector<std::string> texts = {
        R"(
abstract class AA2 {
    test: Number;
    private kn: String;
    das: String;

    constructor(test: Number, kn: String) {
        this.test = test;
        this.kn = kn;
    }
}

class NN2 extends AA2 {
    jkk: String;
    wwa: Number;
})"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 192;
    std::vector<std::string> properties = {"jkk", "wwa"};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText =
        "constructor(test: Number, kn: String, jkk: String, wwa: Number) {\n  super(test, kn);\n  this.jkk = jkk;\n  "
        "this.wwa = wwa;\n}";
    size_t const expectedPosition = 211;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo10)
{
    std::vector<std::string> files = {"getClassConstructorInfo10.ets"};
    std::vector<std::string> texts = {
        R"(
abstract class TY {
    abstract a: String|Number;
})"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 34;
    std::vector<std::string> properties = {"a"};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText = "constructor() {\n}";
    size_t const expectedPosition = 34;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo11)
{
    std::vector<std::string> files = {"getClassConstructorInfo11.ets"};
    std::vector<std::string> texts = {
        R"(
import CommonEventManager from '@ohos.commonEventManager';

interface SubscribeInfoType {
    events: String[];
}

class CommonEventRegister {
    subscriber: CommonEventManager.CommonEventSubscriber | null = null;
    public subscribeInfo: SubscribeInfoType;
})"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 141;
    std::vector<std::string> properties = {"subscriber", "subscribeInfo"};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText =
        "constructor(subscriber: CommonEventManager.CommonEventSubscriber | null, subscribeInfo: SubscribeInfoType) "
        "{\n  this.subscriber = subscriber;\n  this.subscribeInfo = subscribeInfo;\n}";
    size_t const expectedPosition = 148;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo12)
{
    std::vector<std::string> files = {"getClassConstructorInfo12.ets", "getClassConstructorInfo13.ets"};
    std::vector<std::string> texts = {
        R"(
import {AAA} from "./getClassConstructorInfo13.ets";

class BBB extends AAA {
    score: Number;
})",
        R"(
export class AAA {
    name: String;
    age: Number;
    constructor(name: String, age: Number) {
        this.name = name;
        this.age = age;
    }
})"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 2;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 77;
    std::vector<std::string> properties = {"score"};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText =
        "constructor(name: String, age: Number, score: Number) "
        "{\n  super(name, age);\n  this.score = score;\n}";
    size_t const expectedPosition = 83;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo13)
{
    std::vector<std::string> files = {"getClassConstructorInfo14.ets"};
    std::vector<std::string> texts = {
        R"(
class KKK {
    tr: String;

    constructor(tr: String) {
        this.tr = tr;
    }
}

class NNN extends KKK {
})"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 111;
    std::vector<std::string> properties = {};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText = "constructor(tr: String) {\n  super(tr);\n}";
    size_t const expectedPosition = 114;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo14)
{
    std::vector<std::string> files = {"getClassConstructorInfo15.ets"};
    std::vector<std::string> texts = {
        R"(
class ok {
    p1: Number = 0;
    p2: String = "";
    static p3: String = "";
    readonly p4: String = "";
    private p5: String = "";
    protected p6: String = "";
    p7: undefined;
    p8: null = null;
})"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 11;
    std::vector<std::string> properties = {"p1", "p2", "p3", "p4", "p5", "p6", "p7", "p8"};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText =
        "constructor(p1: Number, p2: String, p4: String, p5: String, p6: String, p7: undefined, p8: null) {\n  this.p1 "
        "= p1;\n  this.p2 = p2;\n  this.p4 = p4;\n  this.p5 = p5;\n  this.p6 = p6;\n  this.p7 = p7;\n  this.p8 = "
        "p8;\n}";
    size_t const expectedPosition = 16;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo15)
{
    std::vector<std::string> files = {"getClassConstructorInfo16.ets"};
    std::vector<std::string> texts = {
        R"(
class Demo {
    name = 'jack';
    age = 25;
    flag = false;
    doTask1 = () => 34;
    doTask2 = () => "hello";
    doTask3 = () => false;
    doTask4 = () => console.log("Function called");
})"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 11;
    std::vector<std::string> properties = {"name", "age", "flag", "doTask1", "doTask2", "doTask3", "doTask4"};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText =
        "constructor(name: String, age: Number, flag: Boolean, doTask1: (() => Number), doTask2: (() => String), "
        "doTask3: (() => Boolean), doTask4: (() => void)) {\n  this.name = name;\n  this.age = age;\n  this.flag = "
        "flag;\n  this.doTask1 = doTask1;\n  this.doTask2 = doTask2;\n  this.doTask3 = doTask3;\n  this.doTask4 = "
        "doTask4;\n}";
    size_t const expectedPosition = 18;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo16)
{
    std::vector<std::string> fileNames = {"getClassConstructorInfo17.ets"};
    std::vector<std::string> fileContents = {
        R"(
class FooParent {
    f: Number = 0;
    str: String = "aaa";
    constructor (f: Number, str: String) {
        this.f = f;
        this.str = str;
    }
};

enum Colors {Red = "#FF0000", Green = "#00FF00", Blue = "#0000FF"};
export class Foo extends FooParent {
    name: String = "unassigned";
    isActive: Boolean = true;
    items: String[] = ["aaa", "bbb"];
    point: Number[] = [0, 0];
    //中文测试
    primaryColor: Colors = Colors.Blue;
    optionalValue?:String|null|undefined;
    x: Number = 1;
    static y: Number = 2;
    z: Number = 3;
};)"};
    auto filePaths = CreateTempFile(fileNames, fileContents);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 411;
    std::vector<std::string> properties = {"name", "x", "primaryColor", "isActive", "items", "point", "optionalValue"};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText =
        "constructor(f: Number, str: String, name: String, x: Number, primaryColor: Colors, isActive: Boolean, "
        "items: Array<String>, point: Array<Number>, optionalValue: String | null | undefined) {\n  super(f, str);\n"
        "  this.name = name;\n  this.x = x;\n  this.primaryColor = primaryColor;\n  this.isActive = isActive;\n"
        "  this.items = items;\n  this.point = point;\n  this.optionalValue = optionalValue;\n}";
    size_t const expectedPosition = 269;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

TEST_F(LSPClassInfoTests, getClassConstructorInfo17)
{
    std::vector<std::string> files = {"getClassConstructorInfo18.ets"};
    std::vector<std::string> texts = {
        R"(
//中文测试
class KKK {
    tr: String;

    //中文测试
    constructor(tr: String) {
        //中文测试
        this.tr = "中文测试";
    }
}

//中文测试
class NNN extends KKK {
})"};
    auto filePaths = CreateTempFile(files, texts);

    int const expectedFileCount = 1;
    ASSERT_EQ(filePaths.size(), expectedFileCount);

    LSPAPI const *lspApi = GetImpl();
    size_t const offset = 155;
    std::vector<std::string> properties = {};
    Initializer initializer = Initializer();
    auto ctx = initializer.CreateContext(filePaths[0].c_str(), ES2PANDA_STATE_CHECKED);
    auto res = lspApi->getClassConstructorInfo(ctx, offset, properties);

    std::string expectedText = "constructor(tr: String) {\n  super(tr);\n}";
    size_t const expectedPosition = 158;
    std::vector<FileTextChanges> expectedFileTextChanges =
        CreateExpectedFileTextChanges(filePaths.at(0), expectedPosition, expectedText);
    AssertClassConstructorInfo(res.GetFileTextChanges(), expectedFileTextChanges);
    initializer.DestroyContext(ctx);
}

}  // namespace