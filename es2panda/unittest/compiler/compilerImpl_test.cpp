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

#include <compiler/core/compilerImpl.h>
#include <es2panda.h>
#include <gtest/gtest.h>
#include <util/helpers.h>
#include <mem/pool_manager.h>
#include <parser/parserImpl.h>
#include <parser/program/program.h>

namespace panda::es2panda::compiler {

using mem::MemConfig;

class MemManager {
public:
    explicit MemManager()
    {
        constexpr auto COMPILER_SIZE = 8192_MB;

        MemConfig::Initialize(0, 0, COMPILER_SIZE, 0);
        PoolManager::Initialize(PoolType::MMAP);
    }

    NO_COPY_SEMANTIC(MemManager);
    NO_MOVE_SEMANTIC(MemManager);

    ~MemManager()
    {
        PoolManager::Finalize();
        MemConfig::Finalize();
    }
};

class CompilerImplTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mm_ = std::make_unique<MemManager>();
    }

    void TearDown() override
    {
        mm_.reset();
    }

    // Parse source and set up Binder properly, matching the pipeline in Compiler::Compile
    parser::Program ParseSource(const std::string &source, const CompilerOptions &options)
    {
        parser::ParserImpl parser(ScriptExtension::JS);
        SourceFile sourceFile("test.js", source, parser::ScriptKind::SCRIPT, ScriptExtension::JS);
        sourceFile.isSourceMode = true;
        auto program = parser.Parse(sourceFile, options);
        program.Binder()->SetProgram(&program);
        return program;
    }

    std::unique_ptr<MemManager> mm_;
};

// Test: API >= threshold in merge-abc mode should set record source_file
TEST_F(CompilerImplTest, TestSetSourceFileApi26MergeAbc)
{
    CompilerOptions options;
    options.mergeAbc = true;
    options.targetApiVersion = util::Helpers::SET_SOURCE_FILE_MIN_SUPPORTED_API_VERSION;

    auto program = ParseSource("var x = 1;", options);
    std::string debugInfoSourceFile = "test_source_file.ets";

    CompilerImpl compiler(1);
    auto *result = compiler.Compile(&program, options, debugInfoSourceFile, "");

    ASSERT_NE(result, nullptr);
    auto it = result->record_table.find(std::string(program.RecordName()));
    ASSERT_NE(it, result->record_table.end());
    EXPECT_EQ(it->second.source_file, debugInfoSourceFile);
}

// Test: API < threshold in merge-abc mode should NOT set record source_file
TEST_F(CompilerImplTest, TestSetSourceFileApiBelow26MergeAbc)
{
    CompilerOptions options;
    options.mergeAbc = true;
    options.targetApiVersion = util::Helpers::SET_SOURCE_FILE_MIN_SUPPORTED_API_VERSION - 1;

    auto program = ParseSource("var x = 1;", options);
    std::string debugInfoSourceFile = "test_source_file.ets";

    CompilerImpl compiler(1);
    auto *result = compiler.Compile(&program, options, debugInfoSourceFile, "");

    ASSERT_NE(result, nullptr);
    auto it = result->record_table.find(std::string(program.RecordName()));
    ASSERT_NE(it, result->record_table.end());
    EXPECT_TRUE(it->second.source_file.empty());
}

// Test: API 0 (default/unset) should NOT set record source_file
TEST_F(CompilerImplTest, TestSetSourceFileDefaultApiVersion)
{
    CompilerOptions options;
    options.mergeAbc = true;
    // targetApiVersion defaults to 0

    auto program = ParseSource("var x = 1;", options);
    std::string debugInfoSourceFile = "default_api_test.ets";

    CompilerImpl compiler(1);
    auto *result = compiler.Compile(&program, options, debugInfoSourceFile, "");

    ASSERT_NE(result, nullptr);
    auto it = result->record_table.find(std::string(program.RecordName()));
    ASSERT_NE(it, result->record_table.end());
    EXPECT_TRUE(it->second.source_file.empty());
}

// Test: API > threshold should set record source_file
TEST_F(CompilerImplTest, TestSetSourceFileApiAbove26)
{
    CompilerOptions options;
    options.mergeAbc = true;
    options.targetApiVersion = util::Helpers::SET_SOURCE_FILE_MIN_SUPPORTED_API_VERSION + 1;

    auto program = ParseSource("function foo() { return 42; }", options);
    std::string debugInfoSourceFile = "high_api_test.ets";

    CompilerImpl compiler(1);
    auto *result = compiler.Compile(&program, options, debugInfoSourceFile, "");

    ASSERT_NE(result, nullptr);
    auto it = result->record_table.find(std::string(program.RecordName()));
    ASSERT_NE(it, result->record_table.end());
    EXPECT_EQ(it->second.source_file, debugInfoSourceFile);
}

}  // namespace panda::es2panda::compiler
