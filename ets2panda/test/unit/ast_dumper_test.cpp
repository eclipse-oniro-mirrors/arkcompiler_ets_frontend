/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include <algorithm>
#include "macros.h"

#include "assembler/assembly-program.h"
#include "ir/astDump.h"
#include "ir/expressions/literals/stringLiteral.h"
#include "bytecode_optimizer/bytecodeopt_options.h"
#include "compiler/compiler_logger.h"
#include "mem/arena_allocator.h"
#include "mem/pool_manager.h"
#include "es2panda.h"
#include "util/arktsconfig.h"
#include "util/generateBin.h"
#include "mem/mem.h"
#include "test/utils/panda_executable_path_getter.h"
#include "test/utils/asm_test.h"

namespace {

class TestParams {
public:
    TestParams(std::string_view src, std::initializer_list<const char *> argsList) : src_ {src}
    {
        argsList_.push_back(test::utils::PandaExecutablePathGetter::Get()[0]);
        argsList_.insert(argsList_.end(), argsList.begin(), argsList.end());
    }
    auto GetSrc()
    {
        return src_;
    };
    auto GetExec()
    {
        return argsList_[0];
    };
    auto GetFilename()
    {
        return "dummy.ets";
    };
    auto GetArgs() const
    {
        return argsList_;
    };

private:
    std::string_view src_;
    std::vector<const char *> argsList_;
};

TestParams DumpJsonSimple()
{
    static constexpr std::string_view SRC =
        "\
        function main(args: FixedArray<String>): int {\
            let a: int = 2;\
            let b: int = 3;\
            return a + b;\
        }";

    return TestParams {SRC, {}};
}

TestParams DumpJsonUTF16Char()
{
    static constexpr std::string_view SRC =
        "\
        function main(args: FixedArray<String>): int {\
            let a: char = c'\\uDBFF';\
            let b: char = c'\\uDC00';\
            console.log(a);\
            console.log(b);\
            return 0;\
        }";

    return TestParams {SRC, {}};
}

TestParams DumpEtsSrcSimple()
{
    static constexpr std::string_view SRC =
        "\
        function main(args: FixedArray<String>): int {\
            let a: int = 2;\
            let b: int = 3;\
            return a + b;\
        }";

    return TestParams {SRC,
                       {"--extension=ets",
                        "--dump-ets-src-before-phases=plugins-after-parse,plugins-after-check,plugins-after-lowering"}};
}

class ASTDumperTest : public testing::TestWithParam<TestParams> {
public:
    ASTDumperTest()
    {
        ark::mem::MemConfig::Initialize(0, 0, ark::es2panda::COMPILER_SIZE, 0, 0, 0);
        ark::PoolManager::Initialize(ark::PoolType::MMAP);
    }

    ~ASTDumperTest() override
    {
        ark::PoolManager::Finalize();
        ark::mem::MemConfig::Finalize();
    };

    NO_COPY_SEMANTIC(ASTDumperTest);
    NO_MOVE_SEMANTIC(ASTDumperTest);
};

TEST_P(ASTDumperTest, CheckJsonDump)
{
    auto param = GetParam();
    auto argsList = param.GetArgs();
    auto program = std::unique_ptr<ark::pandasm::Program> {
        test::utils::AsmTest::GetProgram(argsList.size(), argsList.data(), param.GetFilename(), param.GetSrc())};
    ASSERT(program);

    auto dumpStr = program->JsonDump();
    ASSERT(!dumpStr.empty());
}

INSTANTIATE_TEST_SUITE_P(ASTDumperTestParamList, ASTDumperTest,
                         ::testing::Values(DumpJsonSimple(), DumpJsonUTF16Char()));

TEST_F(ASTDumperTest, CheckSrcDump)
{
    std::stringstream dumpStr;
    std::streambuf *prevcoutbuf = std::cout.rdbuf(dumpStr.rdbuf());

    auto param = DumpEtsSrcSimple();
    auto argsList = param.GetArgs();
    auto program = std::unique_ptr<ark::pandasm::Program> {
        test::utils::AsmTest::GetProgram(argsList.size(), argsList.data(), param.GetFilename(), param.GetSrc())};
    ASSERT(program);

    std::cout.rdbuf(prevcoutbuf);

    ASSERT(program);
    ASSERT(!dumpStr.str().empty());
}

}  // namespace
