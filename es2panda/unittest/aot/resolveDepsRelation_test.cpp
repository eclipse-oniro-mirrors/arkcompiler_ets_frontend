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

#include <aot/resolveDepsRelation.h>
#include <aot/options.h>
#include <es2panda.h>
#include <gtest/gtest.h>
#include <mem/pool_manager.h>
#include <util/commonUtil.h>
#include <util/programCache.h>

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace panda::es2panda::aot {

using mem::MemConfig;
using util::JSON_FilE_CONTENT;
using util::IS_COMMONJS;

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

class ResolveDepsRelationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mm_ = std::make_unique<MemManager>();
        options_ = std::make_unique<Options>();
    }

    void TearDown() override
    {
        for (auto &pair : progsInfoOwned_) {
            delete pair.second;
        }
        progsInfoOwned_.clear();
        progsInfo_.clear();
        options_.reset();
        mm_.reset();
    }

    // Helper: create a Field that looks like a JSON content field
    static panda::pandasm::Field MakeJsonField()
    {
        panda::pandasm::Field field(panda::panda_file::SourceLang::ECMASCRIPT);
        field.name = JSON_FilE_CONTENT;
        return field;
    }

    // Helper: create a Field that looks like a commonjs marker field (isCommonjs = 1)
    static panda::pandasm::Field MakeCommonjsField(uint8_t value = 1)
    {
        panda::pandasm::Field field(panda::panda_file::SourceLang::ECMASCRIPT);
        field.name = IS_COMMONJS;
        field.type = panda::pandasm::Type("u8", 0);
        field.metadata->SetValue(
            panda::pandasm::ScalarValue::Create<panda::pandasm::ScalarValue::Type::U8>(value));
        return field;
    }

    // Helper: build a Record with given name, adding fields via move
    static panda::pandasm::Record MakeRecord(const std::string &name)
    {
        return panda::pandasm::Record(name, panda::panda_file::SourceLang::ECMASCRIPT);
    }

    // Helper: build a Program, adding records via move
    static panda::pandasm::Program MakeProgram()
    {
        return panda::pandasm::Program();
    }

    // Helper: add a program with given progKey to progsInfo
    void AddProgram(const std::string &progKey, panda::pandasm::Program program)
    {
        auto *cache = new util::ProgramCache(std::move(program));
        progsInfoOwned_[progKey] = cache;
        progsInfo_[progKey] = cache;
    }

    void SetCompileEntries(std::vector<std::string> entries)
    {
        options_->CompilerOptions().compileContextInfo.compileEntries = std::move(entries);
    }

    std::unique_ptr<MemManager> mm_;
    std::unique_ptr<Options> options_;
    std::map<std::string, util::ProgramCache *> progsInfo_;
    // Owns the ProgramCache objects for cleanup
    std::map<std::string, util::ProgramCache *> progsInfoOwned_;
    std::map<std::string, std::unordered_set<std::string>> resolvedDepsRelation_;
};

// Test: Resolve with empty progsInfo returns true with empty result
TEST_F(ResolveDepsRelationTest, TestResolveEmptyProgsInfo)
{
    SetCompileEntries({"entry"});
    DepsRelationResolver resolver(progsInfo_, options_, resolvedDepsRelation_);
    bool result = resolver.Resolve();
    EXPECT_TRUE(result);
    EXPECT_TRUE(resolvedDepsRelation_.empty());
}

// Test: JSON file record is correctly collected
TEST_F(ResolveDepsRelationTest, TestCollectJsonRecord)
{
    auto program = MakeProgram();
    auto record = MakeRecord("sample/json/data");
    record.field_list.push_back(MakeJsonField());
    program.record_table.emplace("sample/json/data", std::move(record));
    AddProgram("abc1|sample/json.abc", std::move(program));

    SetCompileEntries({"sample/json/data"});
    DepsRelationResolver resolver(progsInfo_, options_, resolvedDepsRelation_);
    bool result = resolver.Resolve();
    EXPECT_TRUE(result);

    auto it = resolvedDepsRelation_.find("abc1|sample/json.abc");
    ASSERT_NE(it, resolvedDepsRelation_.end());
    EXPECT_NE(it->second.find("sample/json/data"), it->second.end());
}

// Test: Commonjs file record is correctly collected
TEST_F(ResolveDepsRelationTest, TestCollectCommonjsRecord)
{
    auto program = MakeProgram();
    auto record = MakeRecord("sample/commonjs/module");
    record.field_list.push_back(MakeCommonjsField(1));
    program.record_table.emplace("sample/commonjs/module", std::move(record));
    AddProgram("abc1|sample/commonjs.abc", std::move(program));

    SetCompileEntries({"sample/commonjs/module"});
    DepsRelationResolver resolver(progsInfo_, options_, resolvedDepsRelation_);
    bool result = resolver.Resolve();
    EXPECT_TRUE(result);

    auto it = resolvedDepsRelation_.find("abc1|sample/commonjs.abc");
    ASSERT_NE(it, resolvedDepsRelation_.end());
    EXPECT_NE(it->second.find("sample/commonjs/module"), it->second.end());
}

// Test: Commonjs with value 0 is not collected as commonjs
TEST_F(ResolveDepsRelationTest, TestCommonjsFieldValueZeroNotCollected)
{
    auto program = MakeProgram();
    auto record = MakeRecord("sample/esmodule/data");
    record.field_list.push_back(MakeCommonjsField(0));
    program.record_table.emplace("sample/esmodule/data", std::move(record));
    AddProgram("abc1|sample/esmodule.abc", std::move(program));

    SetCompileEntries({"sample/esmodule/data"});
    DepsRelationResolver resolver(progsInfo_, options_, resolvedDepsRelation_);
    bool result = resolver.Resolve();
    EXPECT_TRUE(result);

    // Should not be in resolvedDepsRelation_ as commonjs, but should be in record2ProgramMap
    // and then collected via normal resolve path
    auto it = resolvedDepsRelation_.find("abc1|sample/esmodule.abc");
    ASSERT_NE(it, resolvedDepsRelation_.end());
    EXPECT_NE(it->second.find("sample/esmodule/data"), it->second.end());
}

// Test: Dedup of JSON files with same recordName across different programs
TEST_F(ResolveDepsRelationTest, TestDedupJsonRecordsSameName)
{
    // Program 1 with JSON record
    auto prog1 = MakeProgram();
    auto rec1 = MakeRecord("sample/shared/data");
    rec1.field_list.push_back(MakeJsonField());
    prog1.record_table.emplace("sample/shared/data", std::move(rec1));
    AddProgram("abc1|prog1.abc", std::move(prog1));

    // Program 2 with same JSON recordName
    auto prog2 = MakeProgram();
    auto rec2 = MakeRecord("sample/shared/data");
    rec2.field_list.push_back(MakeJsonField());
    prog2.record_table.emplace("sample/shared/data", std::move(rec2));
    AddProgram("abc2|prog2.abc", std::move(prog2));

    SetCompileEntries({"sample/shared/data"});
    DepsRelationResolver resolver(progsInfo_, options_, resolvedDepsRelation_);
    bool result = resolver.Resolve();
    EXPECT_TRUE(result);

    // recordName should appear in only one progKey
    int totalOccurrences = 0;
    for (const auto &[key, records] : resolvedDepsRelation_) {
        if (records.count("sample/shared/data") > 0) {
            totalOccurrences++;
        }
    }
    EXPECT_EQ(totalOccurrences, 1);
}

// Test: Dedup of commonjs files with same recordName across different programs
TEST_F(ResolveDepsRelationTest, TestDedupCommonjsRecordsSameName)
{
    auto prog1 = MakeProgram();
    auto rec1 = MakeRecord("sample/shared/cjsmod");
    rec1.field_list.push_back(MakeCommonjsField(1));
    prog1.record_table.emplace("sample/shared/cjsmod", std::move(rec1));
    AddProgram("abc1|prog1.abc", std::move(prog1));

    auto prog2 = MakeProgram();
    auto rec2 = MakeRecord("sample/shared/cjsmod");
    rec2.field_list.push_back(MakeCommonjsField(1));
    prog2.record_table.emplace("sample/shared/cjsmod", std::move(rec2));
    AddProgram("abc2|prog2.abc", std::move(prog2));

    SetCompileEntries({"sample/shared/cjsmod"});
    DepsRelationResolver resolver(progsInfo_, options_, resolvedDepsRelation_);
    bool result = resolver.Resolve();
    EXPECT_TRUE(result);

    int totalOccurrences = 0;
    for (const auto &[key, records] : resolvedDepsRelation_) {
        if (records.count("sample/shared/cjsmod") > 0) {
            totalOccurrences++;
        }
    }
    EXPECT_EQ(totalOccurrences, 1);
}

// Test: Different recordNames are not deduped
TEST_F(ResolveDepsRelationTest, TestNoDedupDifferentRecordNames)
{
    auto prog1 = MakeProgram();
    auto rec1 = MakeRecord("sample/json/data1");
    rec1.field_list.push_back(MakeJsonField());
    prog1.record_table.emplace("sample/json/data1", std::move(rec1));
    AddProgram("abc1|prog1.abc", std::move(prog1));

    auto prog2 = MakeProgram();
    auto rec2 = MakeRecord("sample/json/data2");
    rec2.field_list.push_back(MakeJsonField());
    prog2.record_table.emplace("sample/json/data2", std::move(rec2));
    AddProgram("abc2|prog2.abc", std::move(prog2));

    SetCompileEntries({"sample/json/data1", "sample/json/data2"});
    DepsRelationResolver resolver(progsInfo_, options_, resolvedDepsRelation_);
    bool result = resolver.Resolve();
    EXPECT_TRUE(result);

    EXPECT_NE(resolvedDepsRelation_.find("abc1|prog1.abc")->second.find("sample/json/data1"),
              resolvedDepsRelation_.find("abc1|prog1.abc")->second.end());
    EXPECT_NE(resolvedDepsRelation_.find("abc2|prog2.abc")->second.find("sample/json/data2"),
              resolvedDepsRelation_.find("abc2|prog2.abc")->second.end());
}

// Test: Mixed scenario - JSON and commonjs in separate programs both collected
TEST_F(ResolveDepsRelationTest, TestMixedJsonAndCommonjsNoCrossDedup)
{
    // JSON and commonjs records must be in separate programs because
    // FillRecord2ProgramMap breaks after the first commonjs/json record per program.
    auto prog1 = MakeProgram();
    auto jsonRec = MakeRecord("sample/json/data");
    jsonRec.field_list.push_back(MakeJsonField());
    prog1.record_table.emplace("sample/json/data", std::move(jsonRec));
    AddProgram("abc1|json.abc", std::move(prog1));

    auto prog2 = MakeProgram();
    auto cjsRec = MakeRecord("sample/cjs/mod");
    cjsRec.field_list.push_back(MakeCommonjsField(1));
    prog2.record_table.emplace("sample/cjs/mod", std::move(cjsRec));
    AddProgram("abc2|cjs.abc", std::move(prog2));

    SetCompileEntries({"sample/json/data", "sample/cjs/mod"});
    DepsRelationResolver resolver(progsInfo_, options_, resolvedDepsRelation_);
    bool result = resolver.Resolve();
    EXPECT_TRUE(result);

    EXPECT_NE(resolvedDepsRelation_.find("abc1|json.abc")->second.find("sample/json/data"),
              resolvedDepsRelation_.find("abc1|json.abc")->second.end());
    EXPECT_NE(resolvedDepsRelation_.find("abc2|cjs.abc")->second.find("sample/cjs/mod"),
              resolvedDepsRelation_.find("abc2|cjs.abc")->second.end());
}

// Test: Record with empty field_list is skipped entirely and not collected
TEST_F(ResolveDepsRelationTest, TestEmptyFieldListSkipped)
{
    auto program = MakeProgram();
    auto record = MakeRecord("sample/empty/record");
    // field_list is empty by default
    program.record_table.emplace("sample/empty/record", std::move(record));
    AddProgram("abc1|empty.abc", std::move(program));

    SetCompileEntries({"sample/empty/record"});
    DepsRelationResolver resolver(progsInfo_, options_, resolvedDepsRelation_);
    bool result = resolver.Resolve();
    EXPECT_TRUE(result);

    // Record with empty field_list is skipped in FillRecord2ProgramMap,
    // not added to record2ProgramMap, and thus not resolvable.
    EXPECT_TRUE(resolvedDepsRelation_.empty());
}

// Test: NPM entries are collected directly without dedup logic
TEST_F(ResolveDepsRelationTest, TestNpmEntriesCollected)
{
    auto program = MakeProgram();
    auto record = MakeRecord("npm/module/entry");
    record.field_list.push_back(MakeCommonjsField(1));
    program.record_table.emplace("npm/module/entry", std::move(record));
    AddProgram("abc1|npmEntries.txt", std::move(program));

    SetCompileEntries({"npm/module/entry"});
    DepsRelationResolver resolver(progsInfo_, options_, resolvedDepsRelation_);
    bool result = resolver.Resolve();
    EXPECT_TRUE(result);

    auto it = resolvedDepsRelation_.find("abc1|npmEntries.txt");
    ASSERT_NE(it, resolvedDepsRelation_.end());
    EXPECT_NE(it->second.find("npm/module/entry"), it->second.end());
}

}  // namespace panda::es2panda::aot
