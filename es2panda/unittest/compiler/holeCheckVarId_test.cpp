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
#include <hole_check_target.h>
#include <mem/pool_manager.h>
#include <parser/parserImpl.h>
#include <parser/program/program.h>
#include <util/helpers.h>

#include <set>

namespace panda::es2panda::compiler {

using mem::MemConfig;
using testing::ext::TestSize;

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

class HoleCheckVarIdTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mm_ = std::make_unique<MemManager>();
    }

    void TearDown() override
    {
        mm_.reset();
    }

    // Parse source as an ES module so that top-level `let`/`const` become
    // module variables with TDZ checks, matching the real module pipeline.
    // NOTE: SourceFile's constructor sets recordName (2nd arg), NOT source.
    // The source field (string_view) must be set explicitly so the parser
    // actually reads the JavaScript code instead of an empty string.
    parser::Program ParseModule(const std::string &source, const CompilerOptions &options)
    {
        parser::ParserImpl parser(ScriptExtension::JS);
        SourceFile sourceFile("test.js", "test", parser::ScriptKind::MODULE, ScriptExtension::JS);
        sourceFile.source = source;
        sourceFile.isSourceMode = true;
        auto program = parser.Parse(sourceFile, options);
        program.Binder()->SetProgram(&program);
        return program;
    }

    // Collect every valid HoleCheckTarget varId from a function table side.
    static std::set<uint32_t> CollectVarIds(const panda::pandasm::Function &func, bool fromStore)
    {
        std::set<uint32_t> ids;
        const auto &table = fromStore ? func.storeVarTable : func.holeCheckVarTable;
        for (const auto &[ins, target] : table) {
            if (target.IsValid()) {
                ids.insert(target.GetVarId());
            }
        }
        return ids;
    }

    std::unique_ptr<MemManager> mm_;
};

// stmodulevar for a module-level `let` store must be recorded in storeVarTable
// with a non-zero varId (MODULE_LOCAL kind).
// NOTE: plain `let` (without export) goes through StLetOrClassToGlobalRecord
// and does NOT emit stmodulevar. Use `export let` so the variable becomes a
// module variable whose store goes through StoreModuleVariable.
HWTEST_F(HoleCheckVarIdTest, StoreVarTablePopulatedForModuleLet, TestSize.Level1)
{
    CompilerOptions options;
    auto program = ParseModule("export let x = 1;", options);

    CompilerImpl compiler(1);
    auto *result = compiler.Compile(&program, options, "", "");
    ASSERT_NE(result, nullptr);

    bool foundStore = false;
    for (auto &[name, func] : result->function_table) {
        for (const auto &[ins, target] : func.storeVarTable) {
            EXPECT_TRUE(target.IsValid());
            EXPECT_NE(target.GetVarId(), 0U);
            foundStore = true;
        }
    }
    EXPECT_TRUE(foundStore);
}

// throw.undefinedifholewithname for a local-export `let` read must be recorded
// in holeCheckVarTable with a non-zero varId.
HWTEST_F(HoleCheckVarIdTest, HoleCheckVarTablePopulatedForLocalExportRead, TestSize.Level1)
{
    CompilerOptions options;
    auto program = ParseModule("export let x = 1; function f() { return x; }", options);

    CompilerImpl compiler(1);
    auto *result = compiler.Compile(&program, options, "", "");
    ASSERT_NE(result, nullptr);

    bool foundThrow = false;
    for (auto &[name, func] : result->function_table) {
        for (const auto &[ins, target] : func.holeCheckVarTable) {
            EXPECT_TRUE(target.IsValid());
            EXPECT_NE(target.GetVarId(), 0U);
            foundThrow = true;
        }
    }
    EXPECT_TRUE(foundThrow);
}

// The store (in func_main_0) and the throw (in function f) for the same
// variable must share the same varId — this is the invariant the optimizer
// relies on to match and eliminate redundant throws.
HWTEST_F(HoleCheckVarIdTest, StoreAndThrowShareVarId, TestSize.Level1)
{
    CompilerOptions options;
    auto program = ParseModule("export let x = 1; function f() { return x; }", options);

    CompilerImpl compiler(1);
    auto *result = compiler.Compile(&program, options, "", "");
    ASSERT_NE(result, nullptr);

    std::set<uint32_t> storeIds;
    std::set<uint32_t> throwIds;
    for (auto &[name, func] : result->function_table) {
        auto s = CollectVarIds(func, true);
        auto t = CollectVarIds(func, false);
        storeIds.insert(s.begin(), s.end());
        throwIds.insert(t.begin(), t.end());
    }
    EXPECT_FALSE(storeIds.empty());
    EXPECT_FALSE(throwIds.empty());
    // The throw's varId must appear among the store varIds (same variable x).
    bool shared = false;
    for (auto id : throwIds) {
        if (storeIds.count(id) > 0) {
            shared = true;
            break;
        }
    }
    EXPECT_TRUE(shared);
}

// Different variables (x, y) must get distinct varIds — no aliasing.
HWTEST_F(HoleCheckVarIdTest, DifferentVariablesGetDifferentVarIds, TestSize.Level1)
{
    CompilerOptions options;
    auto program = ParseModule("export let x = 1; export let y = 2; function f() { return x + y; }", options);

    CompilerImpl compiler(1);
    auto *result = compiler.Compile(&program, options, "", "");
    ASSERT_NE(result, nullptr);

    std::set<uint32_t> throwIds;
    for (auto &[name, func] : result->function_table) {
        auto t = CollectVarIds(func, false);
        throwIds.insert(t.begin(), t.end());
    }
    // x and y are two distinct local-export let reads → at least 2 distinct varIds.
    EXPECT_GE(throwIds.size(), 2U);
}

// AssignVarId must be idempotent: the same Variable* always returns the same id.
HWTEST_F(HoleCheckVarIdTest, AssignVarIdIsIdempotent, TestSize.Level1)
{
    CompilerOptions options;
    auto program = ParseModule("export let x = 1; function f() { return x; }", options);

    CompilerImpl compiler(1);
    auto *result = compiler.Compile(&program, options, "", "");
    ASSERT_NE(result, nullptr);

    // After compilation, every throw entry for x (across functions, if any)
    // must carry the same varId — the same Variable* maps to one id.
    std::set<uint32_t> throwIds;
    for (auto &[name, func] : result->function_table) {
        auto t = CollectVarIds(func, false);
        throwIds.insert(t.begin(), t.end());
    }
    // All throws in this module read the same variable x, so at most one
    // distinct varId should appear among the throws (the single read of x
    // in function f). If there are multiple throws for x, they must share
    // the id.
    ASSERT_LE(throwIds.size(), 1U);
    if (!throwIds.empty()) {
        EXPECT_NE(*throwIds.begin(), 0U);
    }
}

}  // namespace panda::es2panda::compiler
