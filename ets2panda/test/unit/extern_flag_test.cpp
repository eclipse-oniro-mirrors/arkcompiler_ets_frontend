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

#include <memory>
#include <ostream>
#include <string_view>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "assembler/assembly-program.h"
#include "assembly-function.h"
#include "assembly-record.h"
#include "util/eheap.h"
#include "util/diagnosticEngine.h"
#include "util/options.h"

namespace {

inline bool FunctionExternalFlag(ark::pandasm::Function const &fn)
{
    return fn.metadata->GetAttribute("external");
}

inline bool RecordExternalFlag(ark::pandasm::Record const &record)
{
    return record.metadata->GetAttribute("external");
}

struct SaveFmtFlags final {
    explicit SaveFmtFlags(std::ostream &s) : os_ {s}, oldFlags_ {s.flags()} {}
    ~SaveFmtFlags()
    {
        os_.setf(oldFlags_);
    }
    NO_COPY_SEMANTIC(SaveFmtFlags);
    NO_MOVE_SEMANTIC(SaveFmtFlags);

private:
    // CC-OFFNXT(G.NAM.03-CPP) project code style
    std::ostream &os_;
    // CC-OFFNXT(G.NAM.03-CPP) project code style
    std::ios_base::fmtflags oldFlags_;
};

}  // namespace

namespace ark::pandasm {

// The value printer for expects.
std::ostream &operator<<(std::ostream &s, const Function &arg)
{
    SaveFmtFlags const flags {s};
    return s << std::boolalpha << "Function{" << std::quoted(arg.name) << ", external=" << FunctionExternalFlag(arg)
             << "}";
}

// The value printer for expects.
std::ostream &operator<<(std::ostream &s, const Record &arg)
{
    SaveFmtFlags const flags {s};
    return s << std::boolalpha << "Function{" << std::quoted(arg.name) << ", external=" << RecordExternalFlag(arg)
             << "}";
}

}  // namespace ark::pandasm

namespace ark::es2panda::compiler::test {

MATCHER_P(ExternAttribute, flag, "")  // NOLINT (misc-non-private-member-variables-in-classes)
{
    bool value = arg.metadata->GetAttribute("external");
    *result_listener << "'external' attribute is " << value;
    return value == flag;
}

class DeclareTest : public testing::Test {
public:
    DeclareTest()
    {
        ScopedAllocatorsManager::Initialize();
    }
    ~DeclareTest() override
    {
        ScopedAllocatorsManager::Finalize();
    }

protected:
    pandasm::Program const &Program() const
    {
        return *program_;
    }

    void SetCurrentProgram(std::string_view src)
    {
        static constexpr std::string_view FILE_NAME = "ets_decl_test.ets";
        std::array<char const *, 2> args = {"../../../../bin/es2panda",
                                            "--ets-unnamed"};  // NOLINT(modernize-avoid-c-arrays)

        program_ = GetProgram({args.data(), args.size()}, FILE_NAME, src);
    }

    void CheckRecordExternalFlag(std::string recordName, bool externFlag = true)
    {
        ASSERT_NE(program_, nullptr);
        using ::testing::Contains, ::testing::Pair;
        auto const matcher = Contains(Pair(std::move(recordName), ExternAttribute(externFlag)));
        EXPECT_THAT(program_->recordTable, matcher);
    }

    void CheckFunctionExternalFlag(std::string functionName, bool isStatic = false, bool externFlag = true)
    {
        ASSERT_NE(program_, nullptr);
        using ::testing::Contains, ::testing::Pair;
        auto const matcher = Contains(Pair(std::move(functionName), ExternAttribute(externFlag)));
        if (isStatic) {
            EXPECT_THAT(program_->functionStaticTable, matcher);
        } else {
            EXPECT_THAT(program_->functionInstanceTable, matcher);
        }
    }

    void CheckRecordNotExists(std::string name)
    {
        ASSERT_NE(program_, nullptr);
        using ::testing::Contains, ::testing::Pair, ::testing::Not, ::testing::Key;
        EXPECT_THAT(program_->recordTable, Not(Contains(Key(std::move(name)))));
    }

private:
    NO_COPY_SEMANTIC(DeclareTest);
    NO_MOVE_SEMANTIC(DeclareTest);

    static std::unique_ptr<pandasm::Program> GetProgram(ark::Span<const char *const> args, std::string_view fileName,
                                                        std::string_view src)
    {
        auto de = util::DiagnosticEngine();
        auto options = std::make_unique<es2panda::util::Options>(args[0], de);
        if (!options->Parse(args)) {
            return nullptr;
        }

        Logger::ComponentMask mask {};
        mask.set(Logger::Component::ES2PANDA);
        Logger::InitializeStdLogging(options->LogLevel(), mask);

        es2panda::Compiler compiler(options->GetExtension(), options->GetThread());
        es2panda::SourceFile input(fileName, src, options->IsModule());

        return std::unique_ptr<pandasm::Program>(compiler.Compile(input, *options, de));
    }

    pandasm::Function const *GetFunction(std::string_view functionName,
                                         const std::map<std::string, pandasm::Function> &table)
    {
        auto it = table.find(functionName.data());
        if (it == table.end()) {
            return nullptr;
        }
        return &it->second;
    }

    pandasm::Record const *GetRecord(std::string_view recordName, const std::unique_ptr<ark::pandasm::Program> &program)
    {
        auto it = program->recordTable.find(recordName.data());
        if (it == program->recordTable.end()) {
            return nullptr;
        }
        return &it->second;
    }

private:
    std::unique_ptr<pandasm::Program> program_ {};
};

// === Function ===
TEST_F(DeclareTest, function_without_overloads_0)
{
    SetCurrentProgram(R"(
        declare function foo(tmp: double): string
    )");
    CheckFunctionExternalFlag("ETSGLOBAL.foo:f64;std.core.String;", true);
}

TEST_F(DeclareTest, function_with_overloads_0)
{
    SetCurrentProgram(R"(
        declare function foo(tmp?: double): string
    )");
    CheckFunctionExternalFlag("ETSGLOBAL.foo:std.core.Double;std.core.String;", true);
}

// === Method of class ===
TEST_F(DeclareTest, noImplclass_def_with_overload_0)
{
    SetCurrentProgram(R"(
        declare class my_class {
            public foo(arg?: int): string
        }
        function foo(my: my_class){ return my.foo() }
    )");
    CheckFunctionExternalFlag("my_class.foo:std.core.Int;std.core.String;");
}

// === Constructor of class ===
TEST_F(DeclareTest, class_constructor_without_parameters_0)
{
    SetCurrentProgram(R"(
        declare class A_class {
            static x: double
        }
        let a = new A_class()
    )");
    CheckFunctionExternalFlag("A_class.<ctor>:void;");
}

TEST_F(DeclareTest, class_constructor_without_parameters_1)
{
    SetCurrentProgram(R"(
        declare class A {
            constructor();
        }
        let a = new A();
    )");
    CheckFunctionExternalFlag("A.<ctor>:void;");
}

TEST_F(DeclareTest, class_implicit_constructor_0)
{
    SetCurrentProgram(R"(
        declare class A {
        }
        let a = new A();
    )");
    CheckFunctionExternalFlag("A.<ctor>:void;", false);
}

// === Method of interface ===
TEST_F(DeclareTest, noImplinterface_def_with_overload_0)
{
    SetCurrentProgram(R"(
        declare interface my_inter {
            foo(arg?: int): void
        }
        function foo(my: my_inter){ return my.foo() }
    )");
    CheckFunctionExternalFlag("my_inter.foo:std.core.Int;void;");
}

TEST_F(DeclareTest, namespace_0)
{
    SetCurrentProgram(R"(
        declare namespace A {
        }
    )");
    CheckRecordNotExists("A");
}

}  // namespace ark::es2panda::compiler::test
