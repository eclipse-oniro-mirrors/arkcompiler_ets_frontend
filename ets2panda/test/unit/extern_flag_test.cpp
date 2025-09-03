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

#include "assembler/assembly-program.h"
#include "generated/signatures.h"
#include "mem/mem.h"
#include "macros.h"
#include "mem/pool_manager.h"
#include "util/options.h"
#include "test/utils/asm_test.h"

namespace ark::es2panda::compiler::test {

class DeclareTest : public testing::Test {
public:
    DeclareTest()
    {
        mem::MemConfig::Initialize(0, 0, ark::es2panda::COMPILER_SIZE, 0, 0, 0);
        PoolManager::Initialize(PoolType::MMAP);
    }
    ~DeclareTest() override
    {
        PoolManager::Finalize();
        mem::MemConfig::Finalize();
    }

    void SetCurrentProgram(std::string_view src)
    {
        static constexpr std::string_view FILE_NAME = "ets_decl_test.ets";
        std::array<char const *, 2> args = {"../../../../bin/es2panda",
                                            "--ets-unnamed"};  // NOLINT(modernize-avoid-c-arrays)

        program_ = GetProgram({args.data(), args.size()}, FILE_NAME, src);
        ASSERT_NE(program_.get(), nullptr);
    }

    void CheckRecordExternalFlag(std::string_view recordName)
    {
        pandasm::Record *record = GetRecord(recordName, program_);
        ASSERT_TRUE(record != nullptr) << "Record '" << recordName << "' not found";
        ASSERT_TRUE(HasExternalFlag(record)) << "Record '" << record->name << "' doesn't have External flag";
    }

    void CheckFunctionExternalFlag(std::string_view functionName, bool isStatic = false)
    {
        pandasm::Function *fn =
            GetFunction(functionName, isStatic ? program_->functionStaticTable : program_->functionInstanceTable);
        ASSERT_TRUE(fn != nullptr) << "Function '" << functionName << "' not found";
        ASSERT_TRUE(HasExternalFlag(fn)) << "Function '" << fn->name << "' doesn't have External flag";
    }

    void CheckFunctionNoExternalFlag(std::string_view functionName, bool isStatic = false)
    {
        pandasm::Function *fn =
            GetFunction(functionName, isStatic ? program_->functionStaticTable : program_->functionInstanceTable);
        ASSERT_TRUE(fn != nullptr) << "Function '" << functionName << "' not found";
        ASSERT_FALSE(HasExternalFlag(fn)) << "Function '" << fn->name << "' has External flag";
    }

private:
    bool HasExternalFlag(pandasm::Function *fn)
    {
        return (fn->metadata->GetAttribute("external"));
    }

    bool HasExternalFlag(pandasm::Record *record)
    {
        return (record->metadata->GetAttribute("external"));
    }

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

    pandasm::Function *GetFunction(std::string_view functionName, const std::map<std::string, pandasm::Function> &table)
    {
        auto it = table.find(functionName.data());
        if (it == table.end()) {
            return nullptr;
        }
        return const_cast<pandasm::Function *>(&it->second);
    }

    pandasm::Record *GetRecord(std::string_view recordName, const std::unique_ptr<ark::pandasm::Program> &program)
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
    )");
    CheckFunctionExternalFlag("A_class.<ctor>:void;");
}

TEST_F(DeclareTest, class_constructor_without_parameters_1)
{
    SetCurrentProgram(R"(
        declare class A {
            constructor();
        }
    )");
    CheckFunctionExternalFlag("A.<ctor>:void;");
}

TEST_F(DeclareTest, class_implicit_constructor_0)
{
    SetCurrentProgram(R"(
        declare class A {
        }
    )");
    CheckFunctionExternalFlag("A.<ctor>:void;");
}

// === Method of interface ===
TEST_F(DeclareTest, noImplinterface_def_with_overload_0)
{
    SetCurrentProgram(R"(
        declare interface my_inter {
            foo(arg?: int): void
        }
    )");
    CheckFunctionExternalFlag("my_inter.foo:std.core.Int;void;");
}

TEST_F(DeclareTest, namespace_0)
{
    SetCurrentProgram(R"(
        declare namespace A {
        }
    )");
    CheckRecordExternalFlag("A");
}

}  // namespace ark::es2panda::compiler::test
