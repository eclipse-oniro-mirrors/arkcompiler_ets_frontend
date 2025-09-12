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

#include <iomanip>
#include <iterator>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "assembly-function.h"
#include "assembly-program.h"

#include "test/utils/asm_test.h"

namespace ark::pandasm {

// The value printer for expects.
std::ostream &operator<<(std::ostream &s, const Function &arg)
{
    return s << std::quoted(arg.name);
}

}  // namespace ark::pandasm

namespace ark::es2panda::compiler::test {

class UnionAsmTest : public ::test::utils::AsmTest {
public:
    UnionAsmTest() = default;

    ~UnionAsmTest() override = default;

    void CheckFunction(std::string_view funcSig, bool found = true)
    {
        ASSERT_NE(program_.get(), nullptr);
        pandasm::Function *func = GetFunction(funcSig, program_->functionStaticTable);
        if (found) {
            EXPECT_NE(func, nullptr) << "Function '" << funcSig << "' not found";
        } else {
            EXPECT_EQ(func, nullptr) << "Function '" << funcSig << "' found";
        }
    }

    void CheckInsInFunction(std::string_view functionName, std::string_view insString, bool result = true)
    {
        ASSERT_NE(program_.get(), nullptr);
        pandasm::Function *fn = GetFunction(functionName, program_->functionStaticTable);
        ASSERT_NE(fn, nullptr) << "Function '" << functionName << "' not found";
        bool found = false;
        for (const auto &i : fn->ins) {
            std::string iStr = i.ToString("", true);
            if (iStr.find(insString) != std::string::npos) {
                found = true;
            }
        }
        EXPECT_EQ(found, result) << "Instruction '" << insString << "' in function '" << functionName
                                 << "' not met expectations";
    }

    template <typename Iterator>
    void GetAllParameterTypeNames(Iterator result)
    {
        ASSERT_NE(program_.get(), nullptr);
        for (auto const &pair : program_->functionStaticTable) {
            auto const &params = pair.second.params;
            std::transform(params.cbegin(), params.cend(), result,
                           [](auto const &param) { return param.type.GetName(); });
        }
    }

    void CheckParameterTypes(const std::set<std::string> &types)
    {
        std::set<std::string> paramTypes;
        GetAllParameterTypeNames(std::inserter(paramTypes, paramTypes.end()));
        EXPECT_THAT(paramTypes, ::testing::ContainerEq(types));
    }

private:
    NO_COPY_SEMANTIC(UnionAsmTest);
    NO_MOVE_SEMANTIC(UnionAsmTest);
};

TEST_F(UnionAsmTest, union_test)
{
    SetCurrentProgram(R"(
        class A {}
        class B {}
        class C {}
        class D {}
        function test1(v:A|B) {}
        function test2(v:A|B|C) {}
        function test3(v:A|B|FixedArray<C>) {}
        function test4(v:B|C|D|int) {}
    )");

    CheckParameterTypes({
        "{Udummy.A,dummy.B}",
        "{Udummy.A,dummy.B,dummy.C[]}",
        "{Udummy.A,dummy.B,dummy.C}",
        "{Udummy.B,dummy.C,dummy.D,std.core.Int}",
    });
}

TEST_F(UnionAsmTest, union_test_extends)
{
    SetCurrentProgram(R"(
        class A {}
        class B {}
        class C extends B {}
        class D extends B {}
        function test1(v:A|B) {}
        function test2(v:A|B|C) {}
        function test3(v:A|B|FixedArray<C>) {}
        function test4(v:B|C|D|int) {}
    )");

    CheckParameterTypes({
        "{Udummy.A,dummy.B}",
        "{Udummy.A,dummy.B,dummy.C[]}",
        "{Udummy.B,std.core.Int}",
    });
}

TEST_F(UnionAsmTest, union_test_2)
{
    SetCurrentProgram(R"(
        class A {}
        class B {}
        class C {}
        class D {}
        function test1(v:C|C|B|C|B) {}
        function test2(v:D|C|B|A) {}
        function test3(v:A|A|A|B) {}
        function test4(v:A|double) {}
        function test5(v:double|double|long) {}
        function test6(v:double|int|long) {}
        function test7(v:B|C|B|A) {}
        function test8(v:A|D|D|D) {}
    )");

    CheckParameterTypes({
        "{Udummy.A,dummy.B,dummy.C,dummy.D}",
        "{Udummy.A,dummy.B,dummy.C}",
        "{Udummy.A,dummy.B}",
        "{Udummy.A,dummy.D}",
        "{Udummy.A,std.core.Double}",
        "{Udummy.B,dummy.C}",
        "{Ustd.core.Double,std.core.Int,std.core.Long}",
        "{Ustd.core.Double,std.core.Long}",
    });
}

TEST_F(UnionAsmTest, union_test_arrays)
{
    SetCurrentProgram(R"(
        class A {}
        class B {}
        class C {}
        class D {}
        function test1(v:B|C|FixedArray<A|D|double>) {}
        function test1(v:FixedArray<A|D|double>|FixedArray<A|D|int>|FixedArray<A|D|long>) {}
        function test1(v:B|C|FixedArray<A|B>) {}
        function test1(v:B|FixedArray<A|D>|FixedArray<D|B>|FixedArray<C|B>|FixedArray<C|D>) {}
        function test1(v:FixedArray<A|D>|FixedArray<D|B>|FixedArray<C|B>|FixedArray<C|D>) {}
        function test1(v:FixedArray<A|D>|FixedArray<D|B>) {}
        function test1(v:FixedArray<D|C>|FixedArray<C|B>) {}
        function test1(v:FixedArray<D|C>|C|B) {}
        function test1(v:FixedArray<int|double>|long|double) {}
        function test1(v:FixedArray<FixedArray<int>|FixedArray<double>>|FixedArray<long>|FixedArray<double>) {}
    )");

    CheckParameterTypes({
        "{Udummy.B,dummy.C,{Udummy.A,dummy.B}[]}",
        "{Udummy.B,dummy.C,{Udummy.A,dummy.D,std.core.Double}[]}",
        "{Udummy.B,dummy.C,{Udummy.C,dummy.D}[]}",
        "{Udummy.B,{Udummy.A,dummy.D}[],{Udummy.B,dummy.C}[],{Udummy.B,dummy.D}[],{Udummy.C,dummy.D}[]}",
        "{U{Udummy.A,dummy.D,std.core.Double}[],{Udummy.A,dummy.D,std.core.Int}[],{Udummy.A,dummy.D,std.core.Long}[]}",
        "{U{Udummy.A,dummy.D}[],{Udummy.B,dummy.C}[],{Udummy.B,dummy.D}[],{Udummy.C,dummy.D}[]}",
        "{U{Udummy.A,dummy.D}[],{Udummy.B,dummy.D}[]}",
        "{U{Udummy.B,dummy.C}[],{Udummy.C,dummy.D}[]}",
        "{Uf64[],i64[],{Uf64[],i32[]}[]}",
        "{Ustd.core.Double,std.core.Long,{Ustd.core.Double,std.core.Int}[]}",
    });
}

TEST_F(UnionAsmTest, union_test_null)
{
    SetCurrentProgram(R"(
        class A {}
        class B {}
        class C {}
        class D {}
        function test1(v:D|C|B|null|A) {}
    )");

    CheckParameterTypes({
        "{Udummy.A,dummy.B,dummy.C,dummy.D,std.core.Null}",
    });
}

TEST_F(UnionAsmTest, union_test_undefined)
{
    SetCurrentProgram(R"(
        class A {}
        class B {}
        class C {}
        class D {}
        function test1(v:D|C|B|undefined|A) {}
        function test2(v:undefined|A) {}
        function test3(v:undefined|A|null) {}
    )");

    CheckParameterTypes({
        "{Udummy.A,dummy.B,dummy.C,dummy.D}",
        "dummy.A",
        "{Udummy.A,std.core.Null}",
    });
}

TEST_F(UnionAsmTest, union_test_isinstanceof)
{
    SetCurrentProgram(R"(
        function test1(v: string): void { v instanceof string | number }
    )");

    CheckInsInFunction("dummy.ETSGLOBAL.test1:std.core.String;void;", "isinstance {Ustd.core.Double,std.core.String}",
                       true);
}

TEST_F(UnionAsmTest, union_test_generic_checkcast)
{
    SetCurrentProgram(R"(
        class X<T> {
            constructor(v: T) { this.p = v}
            p: T
        }
        function test1(x: X<string|number>) { return x.p }
        function test2(x: X<string|number|undefined>) { return x.p }
    )");

    CheckInsInFunction("dummy.ETSGLOBAL.test1:dummy.X;{Ustd.core.Double,std.core.String};",
                       "checkcast {Ustd.core.Double,std.core.String}", true);
    CheckInsInFunction("dummy.ETSGLOBAL.test1:dummy.X;{Ustd.core.Double,std.core.String};",
                       "std.core.Runtime.failedTypeCastException", true);
    CheckInsInFunction("dummy.ETSGLOBAL.test2:dummy.X;{Ustd.core.Double,std.core.String};",
                       "checkcast {Ustd.core.Double,std.core.String}", true);
    CheckInsInFunction("dummy.ETSGLOBAL.test2:dummy.X;{Ustd.core.Double,std.core.String};",
                       "std.core.Runtime.failedTypeCastException", true);
}

TEST_F(UnionAsmTest, union_test_as)
{
    SetCurrentProgram(R"(
        function test1(v: string): void { v as string | number }
    )");

    CheckInsInFunction("dummy.ETSGLOBAL.test1:std.core.String;void;", "checkcast {Ustd.core.Double,std.core.String}",
                       true);
    CheckInsInFunction("dummy.ETSGLOBAL.test1:std.core.String;void;", "std.core.Runtime.failedTypeCastException", true);
}

TEST_F(UnionAsmTest, union_null_object)
{
    SetCurrentProgram(R"(
        type T1 = string | null | undefined | object
        type T2 = string | object | null | undefined
        function foo1(a: T1) {}
        function foo2(a: T2) {}
    )");

    CheckFunction("dummy.ETSGLOBAL.foo1:{Ustd.core.Null,std.core.Object};void;");
    CheckFunction("dummy.ETSGLOBAL.foo2:{Ustd.core.Null,std.core.Object};void;");
    CheckFunction("dummy.ETSGLOBAL.foo1:std.core.Object;void;", false);
    CheckFunction("dummy.ETSGLOBAL.foo2:std.core.Object;void;", false);
}

}  // namespace ark::es2panda::compiler::test
