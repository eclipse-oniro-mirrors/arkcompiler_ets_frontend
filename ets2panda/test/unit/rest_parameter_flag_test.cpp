/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "assembler/assembly-program.h"
#include "es2panda.h"
#include "generated/signatures.h"
#include "mem/mem.h"
#include "macros.h"
#include "mem/pool_manager.h"
#include "test/utils/asm_test.h"

namespace ark::es2panda::compiler::test {

class RestParameterTest : public ::test::utils::AsmTest {
public:
    RestParameterTest() = default;

    ~RestParameterTest() override = default;

    void CheckRestParameterFlag(std::string_view functionName, bool isStatic)
    {
        pandasm::Function *fn =
            GetFunction(functionName, isStatic ? program_->functionStaticTable : program_->functionInstanceTable);
        ASSERT_TRUE(fn != nullptr) << "Function '" << functionName << "' not found";
        ASSERT_TRUE(HasRestParameterFlag(fn)) << "Function '" << fn->name << "' doesn't have ACC_VARARGS flag";
    }

    void CheckNoRestParameterFlag(std::string_view functionName, bool isStatic)
    {
        pandasm::Function *fn =
            GetFunction(functionName, isStatic ? program_->functionStaticTable : program_->functionInstanceTable);
        ASSERT_TRUE(fn != nullptr) << "Function '" << functionName << "' not found";
        ASSERT_FALSE(HasRestParameterFlag(fn)) << "Function '" << fn->name << "' has ACC_VARARGS flag";
    }

private:
    bool HasRestParameterFlag(pandasm::Function *fn)
    {
        return (fn->metadata->GetAccessFlags() & ACC_VARARGS) != 0;
    }

    NO_COPY_SEMANTIC(RestParameterTest);
    NO_MOVE_SEMANTIC(RestParameterTest);
};

// === Function ===
TEST_F(RestParameterTest, function_without_rest_parameters_0)
{
    SetCurrentProgram(R"(
        function fn(): void {
        }
    )");
    CheckNoRestParameterFlag("dummy.ETSGLOBAL.fn:void;", true);
}

TEST_F(RestParameterTest, function_without_rest_parameters_1)
{
    SetCurrentProgram(R"(
        function fn(args: FixedArray<int>): void {
        }
    )");
    CheckNoRestParameterFlag("dummy.ETSGLOBAL.fn:i32[];void;", true);
}

TEST_F(RestParameterTest, function_without_rest_parameters_2)
{
    SetCurrentProgram(R"(
        function fn(arg0: int, args: FixedArray<String>): void {
        }
    )");
    CheckNoRestParameterFlag("dummy.ETSGLOBAL.fn:i32;std.core.String[];void;", true);
}

TEST_F(RestParameterTest, function_with_rest_parameter_0)
{
    SetCurrentProgram(R"(
        function fn(...args: FixedArray<String>): void {
        }
    )");
    CheckRestParameterFlag("dummy.ETSGLOBAL.fn:std.core.String[];void;", true);
}

TEST_F(RestParameterTest, function_with_rest_parameter_1)
{
    SetCurrentProgram(R"(
        function fn(o: Object, ...args: FixedArray<int>): void {
        }
    )");
    CheckRestParameterFlag("dummy.ETSGLOBAL.fn:std.core.Object;i32[];void;", true);
}

// === Method of class ===
TEST_F(RestParameterTest, class_method_without_rest_parameters_0)
{
    SetCurrentProgram(R"(
        class A {
            fn() {};
        }
    )");
    CheckNoRestParameterFlag("dummy.A.fn:void;", false);
}

TEST_F(RestParameterTest, class_method_without_rest_parameters_1)
{
    SetCurrentProgram(R"(
        class A {
            fn(arg0: int) {};
        }
    )");
    CheckNoRestParameterFlag("dummy.A.fn:i32;void;", false);
}

TEST_F(RestParameterTest, class_method_with_rest_parameters_0)
{
    SetCurrentProgram(R"(
        class A {
            fn(...args: FixedArray<int>) {};
        }
    )");
    CheckRestParameterFlag("dummy.A.fn:i32[];void;", false);
}

// === Static method of class ===
TEST_F(RestParameterTest, static_class_method_without_rest_parameters_0)
{
    SetCurrentProgram(R"(
        class A {
            static fn() {};
        }
    )");
    CheckNoRestParameterFlag("dummy.A.fn:void;", true);
}

TEST_F(RestParameterTest, static_class_method_without_rest_parameters_1)
{
    SetCurrentProgram(R"(
        class A {
            static fn(arg0: int) {};
        }
    )");
    CheckNoRestParameterFlag("dummy.A.fn:i32;void;", true);
}

TEST_F(RestParameterTest, static_class_method_with_rest_parameters_0)
{
    SetCurrentProgram(R"(
        class A {
            static fn(...args: FixedArray<int>) {};
        }
    )");
    CheckRestParameterFlag("dummy.A.fn:i32[];void;", true);
}

TEST_F(RestParameterTest, static_class_method_with_rest_parameters_1)
{
    SetCurrentProgram(R"(
        class A {
            static fn(a: FixedArray<String>, ...args: FixedArray<int>) {};
        }
    )");
    CheckRestParameterFlag("dummy.A.fn:std.core.String[];i32[];void;", true);
}

// === Constructor of class ===
TEST_F(RestParameterTest, class_constructor_without_rest_parameters_0)
{
    SetCurrentProgram(R"(
        class A {
            constructor() {};
        }
    )");
    CheckNoRestParameterFlag("dummy.A.<ctor>:void;", false);
}

TEST_F(RestParameterTest, class_constructor_without_rest_parameters_1)
{
    SetCurrentProgram(R"(
        class A {
            constructor(args: FixedArray<String>) {};
        }
    )");
    CheckNoRestParameterFlag("dummy.A.<ctor>:std.core.String[];void;", false);
}

TEST_F(RestParameterTest, class_constructor_with_rest_parameters_0)
{
    SetCurrentProgram(R"(
        class A {
            constructor(...args: FixedArray<int>) {};
        }
    )");
    CheckRestParameterFlag("dummy.A.<ctor>:i32[];void;", false);
}

TEST_F(RestParameterTest, class_constructor_with_rest_parameters_1)
{
    SetCurrentProgram(R"(
        class A {
            constructor(v0: long, ...args: FixedArray<String>) {};
        }
    )");
    CheckRestParameterFlag("dummy.A.<ctor>:i64;std.core.String[];void;", false);
}

// === Method of interface ===
TEST_F(RestParameterTest, interface_without_rest_parameters_0)
{
    SetCurrentProgram(R"(
        interface A {
            fn() {};
        }
    )");
    CheckNoRestParameterFlag("dummy.A.fn:void;", false);
}

TEST_F(RestParameterTest, interface_without_rest_parameters_1)
{
    SetCurrentProgram(R"(
        interface A {
            fn(args: FixedArray<String>) {};
        }
    )");
    CheckNoRestParameterFlag("dummy.A.fn:std.core.String[];void;", false);
}

TEST_F(RestParameterTest, interface_with_rest_parameters_0)
{
    SetCurrentProgram(R"(
        interface A {
            fn(...args: FixedArray<Object>) {};
        }
    )");
    CheckRestParameterFlag("dummy.A.fn:std.core.Object[];void;", false);
}

TEST_F(RestParameterTest, interface_with_rest_parameters_1)
{
    SetCurrentProgram(R"(
        interface A {
            fn(o: Object, ...args: FixedArray<String>) {};
        }
    )");
    CheckRestParameterFlag("dummy.A.fn:std.core.Object;std.core.String[];void;", false);
}

// === Lambda method ===
TEST_F(RestParameterTest, lambda_without_rest_parameters_0)
{
    SetCurrentProgram(R"(
        let fn: ()=>int = (): int => {
            return 1;
        }
    )");
    CheckNoRestParameterFlag("dummy.ETSGLOBAL.lambda$invoke$0:i32;", true);
}

TEST_F(RestParameterTest, lambda_without_rest_parameters_1)
{
    SetCurrentProgram(R"(
        let fn: (args: FixedArray<long>)=>int = (args: FixedArray<long>): int => {
            return 1;
        }
    )");
    CheckNoRestParameterFlag("dummy.ETSGLOBAL.lambda$invoke$0:i64[];i32;", true);
}

// === Abstract method of abstract class ===
TEST_F(RestParameterTest, abstract_function_without_rest_parameter_0)
{
    SetCurrentProgram(R"(
        abstract class A {
            abstract fn(): void
        }
    )");
    CheckNoRestParameterFlag("dummy.A.fn:void;", false);
}

TEST_F(RestParameterTest, abstract_function_without_rest_parameter_1)
{
    SetCurrentProgram(R"(
        abstract class A {
            abstract fn(args: FixedArray<String>): void
        }
    )");
    CheckNoRestParameterFlag("dummy.A.fn:std.core.String[];void;", false);
}

TEST_F(RestParameterTest, abstract_function_with_rest_parameter_0)
{
    SetCurrentProgram(R"(
        abstract class A {
            abstract fn(...args: FixedArray<String>): void
        }
    )");
    CheckRestParameterFlag("dummy.A.fn:std.core.String[];void;", false);
}

TEST_F(RestParameterTest, abstract_function_with_rest_parameter_1)
{
    SetCurrentProgram(R"(
        abstract class A {
            abstract fn(v: int, ...args: FixedArray<String>): void
        }
    )");
    CheckRestParameterFlag("dummy.A.fn:i32;std.core.String[];void;", false);
}

// === External methods ===
TEST_F(RestParameterTest, external_function_with_rest_parameter_0)
{
    SetCurrentProgram("");
    CheckRestParameterFlag("std.core.LambdaValue.invoke:escompat.Array;std.core.Object;", false);
}

TEST_F(RestParameterTest, external_function_with_rest_parameter_1)
{
    SetCurrentProgram("");
    CheckRestParameterFlag("escompat.Math.max:escompat.Array;f64;", true);
}

}  // namespace ark::es2panda::compiler::test
