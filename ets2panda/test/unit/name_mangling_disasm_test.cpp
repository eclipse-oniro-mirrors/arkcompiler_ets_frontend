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

#include <string_view>
#include "gtest/gtest.h"
#include "test/utils/asm_test.h"

class NameManglingAsmTest : public ::test::utils::AsmTest {
public:
    void CheckUsingFunctionInstanceTable(std::string_view input, std::string_view expectedName)
    {
        SetCurrentProgram(input);

        auto result = GetFunction(expectedName, program_->functionInstanceTable);
        ASSERT_NE(result, nullptr);
    }

    void CheckUsingRecordTable(std::string_view input, std::string_view expectedName)
    {
        SetCurrentProgram(input);

        auto result = GetRecord(expectedName, program_);
        ASSERT_NE(result, nullptr);
    }

    void CheckPropertyUsingRecordTable(std::string_view input, std::string_view expectedRecordName,
                                       [[maybe_unused]] std::string_view expectedPropName)
    {
        SetCurrentProgram(input);

        auto result = GetRecord(expectedRecordName, program_);
        ASSERT_NE(result, nullptr);

        bool found = false;
        for (auto it = result->fieldList.begin(); it != result->fieldList.end(); it++) {
            std::string name = expectedRecordName.data() + std::string(".") + it->name;
            if (name == expectedPropName) {
                found = true;
                break;
            }
        }

        ASSERT_NE(found, false);
    }
};

TEST_F(NameManglingAsmTest, asyncFunctionNameGen)
{
    std::string_view input = R"(
      async function TestFunc() {}
    )";

    SetCurrentProgram(input);

    std::string_view expectedGeneratedName = "dummy.ETSGLOBAL.%%async-TestFunc:std.core.Object;";

    auto result = GetFunction(expectedGeneratedName, program_->functionStaticTable);
    ASSERT_NE(result, nullptr);
}

TEST_F(NameManglingAsmTest, asyncMethodNameGen)
{
    std::string_view input = R"(
        class TestClass {
            async testMethod() {}
        }
    )";

    std::string_view expectedGeneratedName = "dummy.TestClass.%%async-testMethod:std.core.Object;";

    CheckUsingFunctionInstanceTable(input, expectedGeneratedName);
}

TEST_F(NameManglingAsmTest, interfaceGetterNameGen)
{
    std::string_view input = R"(
        interface TestInterface {
            get testMember() : string;
        }
    )";

    std::string_view expectedGeneratedName = "dummy.TestInterface.%%get-testMember:std.core.String;";

    CheckUsingFunctionInstanceTable(input, expectedGeneratedName);
}

TEST_F(NameManglingAsmTest, classGetterNameGen)
{
    std::string_view input = R"(
        class TestClass {
            get testMember(): string {
                return "hello world";
            }
        }
    )";

    std::string_view expectedGeneratedName = "dummy.TestClass.%%get-testMember:std.core.String;";

    CheckUsingFunctionInstanceTable(input, expectedGeneratedName);
}

TEST_F(NameManglingAsmTest, partialClassNameGen)
{
    std::string_view input = R"(
        class TestClass {
            maybeNullMember: string | undefined = undefined;
        }

        function testFunc(a0: Partial<TestClass>) {
            if (a0.maybeNullMember != undefined) {
                console.log("the member is: " + a0.maybeNullMember)
            } else {
                console.log("all of the members on the argument are null")
            }
        }

        let a: Partial<TestClass> = {}

        testFunc(a)
    )";

    std::string_view expectedGeneratedName = "dummy.%%partial-TestClass";
    CheckUsingRecordTable(input, expectedGeneratedName);
}

TEST_F(NameManglingAsmTest, partialClassFromNamespaceNameGen)
{
    std::string_view input = R"(
        namespace TestNamespace {
          export class ClassInNamespace {
            className: String | undefined = undefined;
          }
        }

        function exampleFunc(a0: Partial<TestNamespace.ClassInNamespace>) {
          if (a0.className != undefined) {
            console.log("className is: " + a0.className)
          }
        }

        let testVar: Partial<TestNamespace.ClassInNamespace> = {
            className: "ClassInNamespace"
        }

        exampleFunc(testVar);
    )";

    std::string_view expectedGeneratedName = "dummy.TestNamespace.%%partial-ClassInNamespace";
    CheckUsingRecordTable(input, expectedGeneratedName);
}

TEST_F(NameManglingAsmTest, partialInterfaceNameGen)
{
    std::string_view input = R"(
        interface Book {
            title: String;
            description: String;
        }

        function exampleFunc(a0: Partial<Book>) {
            if (a0.title != undefined) {
                console.log("The title is: " + a0.title);
            }

            if (a0.description != undefined) {
                console.log("The description is: " + a0.description);
            }
        }

        let myBook: Partial<Book> = {
            title: "The title",
            description: "Some kind of a description"
        }

        exampleFunc(myBook);
    )";

    std::string_view expectedGeneratedName = "dummy.%%partial-Book";
    CheckUsingRecordTable(input, expectedGeneratedName);
}

TEST_F(NameManglingAsmTest, partialInterfaceFromNamespaceNameGen)
{
    std::string_view input = R"(
        namespace TestNamespace {
          export interface TestInterface {
            title: String
            description: String
          }
        }

        function exampleFunc(a0: Partial<TestNamespace.TestInterface>) {
          if (a0.title != undefined) {
            console.log("title is: " + a0.title)
          }

          if (a0.description != undefined) {
            console.log("description is: " + a0.description)
          }
        }

        let testVar: Partial<TestNamespace.TestInterface> = {
            title: "The title",
            description: "Some kind of a description"
        }

        exampleFunc(testVar);
    )";

    std::string_view expectedGeneratedName = "dummy.TestNamespace.%%partial-TestInterface";
    CheckUsingRecordTable(input, expectedGeneratedName);
}

TEST_F(NameManglingAsmTest, propertyNameGen)
{
    std::string_view input = R"(
        interface TestInterface {
            testMember: String;
        }

        class TestClass implements TestInterface {
            testMember = "Hello world";
        }
    )";

    SetCurrentProgram(input);
    std::string_view expectedRecordName = "dummy.TestClass";
    std::string_view expectedPropName = "dummy.TestClass.%%property-testMember";
    CheckPropertyUsingRecordTable(input, expectedRecordName, expectedPropName);
}

TEST_F(NameManglingAsmTest, interfaceSetterNameGen)
{
    std::string_view input = R"(
        interface TestInterface {
            set testMember(a0: String)
        }
    )";

    std::string_view expectedGeneratedName = "dummy.TestInterface.%%set-testMember:std.core.String;void;";
    CheckUsingFunctionInstanceTable(input, expectedGeneratedName);
}

TEST_F(NameManglingAsmTest, classSetterNameGen)
{
    std::string_view input = R"(
        interface TestClass {
            set testMember(a0: String)
        }
    )";

    std::string_view expectedGeneratedName = "dummy.TestClass.%%set-testMember:std.core.String;void;";
    CheckUsingFunctionInstanceTable(input, expectedGeneratedName);
}

TEST_F(NameManglingAsmTest, lambdaNameGen)
{
    std::string_view input = R"(
        let lambdaFunc = (): void => {}
    )";

    std::string_view expectedGeneratedName = "dummy.%%lambda-lambda_invoke-0";
    CheckUsingRecordTable(input, expectedGeneratedName);
}

TEST_F(NameManglingAsmTest, unionPropMethodNameGen)
{
    std::string_view input = R"(
        class TestClassA {
          propMethod(): void {
            console.log("hello world");
          }
        }

        class TestClassB {
          propMethod(): void {
            console.log("hello world");
          }
        }

        function foo(a0: TestClassA | TestClassB) {
          a0.propMethod();
        }

        let myVar = new TestClassA()

        foo(myVar)
    )";

    std::string_view expectedRecordName = "dummy.%%union_prop-TestClassA|TestClassB";
    CheckUsingRecordTable(input, expectedRecordName);

    std::string_view expectedMethodName = "dummy.%%union_prop-TestClassA|TestClassB.propMethod:void;";
    CheckUsingFunctionInstanceTable(input, expectedMethodName);
}

TEST_F(NameManglingAsmTest, unionPropMemberNameGen)
{
    std::string_view input = R"(
        class TestClass {
          testMethod(): void {
            console.log("hello world")
          }
        }

        class TestClassA {
          prop = new TestClass()
        }

        class TestClassB {
          prop = new TestClass()
        }

        function foo(a0: TestClassA | TestClassB) {
          let tmpVar = a0.prop
          tmpVar.testMethod()
        }

        let myVar = new TestClassA()

        foo(myVar)
    )";

    std::string_view expectedRecordName = "dummy.%%union_prop-TestClassA|TestClassB";
    CheckUsingRecordTable(input, expectedRecordName);

    std::string_view expectedPropName = "dummy.%%union_prop-TestClassA|TestClassB.prop";
    CheckPropertyUsingRecordTable(input, expectedRecordName, expectedPropName);
}
