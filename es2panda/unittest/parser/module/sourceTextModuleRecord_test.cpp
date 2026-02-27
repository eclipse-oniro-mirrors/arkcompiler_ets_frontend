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

#include <parser/module/sourceTextModuleRecord.h>
#include <binder/scope.h>
#include <ir/expressions/identifier.h>
#include <gtest/gtest.h>
#include <mem/pool_manager.h>

namespace panda::es2panda::parser {

constexpr int TWO_MODULE_REQUESTS = 2;
constexpr int TWO_IMPORTS = 2;
constexpr int TWO_EXPORTS = 2;
constexpr int FIRST_MODULE_REQUEST_IDX = 1;
constexpr int SECOND_MODULE_REQUEST_IDX = 2;

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

class SourceTextModuleRecordTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mm_ = std::make_unique<MemManager>();
        allocator_ = std::make_unique<ArenaAllocator>(SpaceType::SPACE_TYPE_COMPILER, nullptr, "");
        moduleRecord_ = std::make_unique<SourceTextModuleRecord>(allocator_.get());
    }

    void TearDown() override
    {
        moduleRecord_.reset();
        allocator_.reset();
        mm_.reset();
    }

    std::unique_ptr<MemManager> mm_;
    std::unique_ptr<ArenaAllocator> allocator_;
    std::unique_ptr<SourceTextModuleRecord> moduleRecord_;
};

TEST_F(SourceTextModuleRecordTest, TestAddModuleRequest)
{
    SourceTextModuleRecord::ModuleRequestRecord record1("module1.js");
    int idx1 = moduleRecord_->AddModuleRequest(record1);
    EXPECT_EQ(idx1, 0);
    EXPECT_EQ(moduleRecord_->GetModuleRequests().size(), 1);
    EXPECT_FALSE(moduleRecord_->HasLazyImport());

    SourceTextModuleRecord::ModuleRequestRecord record2("module2.js");
    int idx2 = moduleRecord_->AddModuleRequest(record2);
    EXPECT_EQ(idx2, 1);
    EXPECT_EQ(moduleRecord_->GetModuleRequests().size(), TWO_MODULE_REQUESTS);
}

TEST_F(SourceTextModuleRecordTest, TestAddModuleRequestDuplicate)
{
    SourceTextModuleRecord::ModuleRequestRecord record1("module1.js");
    int idx1 = moduleRecord_->AddModuleRequest(record1);
    EXPECT_EQ(idx1, 0);

    SourceTextModuleRecord::ModuleRequestRecord record2("module1.js");
    int idx2 = moduleRecord_->AddModuleRequest(record2);
    EXPECT_EQ(idx2, 0);
    EXPECT_EQ(moduleRecord_->GetModuleRequests().size(), 1);
}

TEST_F(SourceTextModuleRecordTest, TestAddModuleRequestLazy)
{
    SourceTextModuleRecord::ModuleRequestRecord record1("module1.js", true);
    int idx1 = moduleRecord_->AddModuleRequest(record1);
    EXPECT_EQ(idx1, 0);
    EXPECT_TRUE(moduleRecord_->HasLazyImport());

    SourceTextModuleRecord::ModuleRequestRecord record2("module2.js", false);
    int idx2 = moduleRecord_->AddModuleRequest(record2);
    EXPECT_EQ(idx2, 1);
    EXPECT_TRUE(moduleRecord_->HasLazyImport());
}

TEST_F(SourceTextModuleRecordTest, TestAddModuleRequestTemplate)
{
    int idx1 = moduleRecord_->AddModuleRequest("module1.js");
    EXPECT_EQ(idx1, 0);

    int idx2 = moduleRecord_->AddModuleRequest("module2.js", true);
    EXPECT_EQ(idx2, 1);
    EXPECT_TRUE(moduleRecord_->HasLazyImport());
}

TEST_F(SourceTextModuleRecordTest, TestAddImportEntry)
{
    int modIdx = moduleRecord_->AddModuleRequest("module.js");

    auto *entry = allocator_->New<SourceTextModuleRecord::ImportEntry>("local", "import", modIdx, nullptr, nullptr);
    moduleRecord_->AddImportEntry(entry);

    const auto &imports = moduleRecord_->GetRegularImportEntries();
    EXPECT_EQ(imports.size(), 1);
    EXPECT_NE(imports.find("local"), imports.end());
}

TEST_F(SourceTextModuleRecordTest, TestAddImportEntryMultiple)
{
    int modIdx = moduleRecord_->AddModuleRequest("module.js");

    auto *entry1 = allocator_->New<SourceTextModuleRecord::ImportEntry>("local1", "import1", modIdx, nullptr, nullptr);
    moduleRecord_->AddImportEntry(entry1);

    auto *entry2 = allocator_->New<SourceTextModuleRecord::ImportEntry>("local2", "import2", modIdx, nullptr, nullptr);
    moduleRecord_->AddImportEntry(entry2);

    const auto &imports = moduleRecord_->GetRegularImportEntries();
    EXPECT_EQ(imports.size(), TWO_IMPORTS);
}

TEST_F(SourceTextModuleRecordTest, TestAddStarImportEntry)
{
    int modIdx = moduleRecord_->AddModuleRequest("module.js");

    auto *entry = allocator_->New<SourceTextModuleRecord::ImportEntry>("local", modIdx, nullptr);
    moduleRecord_->AddStarImportEntry(entry);

    const auto &imports = moduleRecord_->GetNamespaceImportEntries();
    EXPECT_EQ(imports.size(), 1);
    EXPECT_EQ(imports[0]->localName_, "local");
}

TEST_F(SourceTextModuleRecordTest, TestAddStarImportEntryMultiple)
{
    int modIdx = moduleRecord_->AddModuleRequest("module.js");

    auto *entry1 = allocator_->New<SourceTextModuleRecord::ImportEntry>("local1", modIdx, nullptr);
    moduleRecord_->AddStarImportEntry(entry1);

    auto *entry2 = allocator_->New<SourceTextModuleRecord::ImportEntry>("local2", modIdx, nullptr);
    moduleRecord_->AddStarImportEntry(entry2);

    const auto &imports = moduleRecord_->GetNamespaceImportEntries();
    EXPECT_EQ(imports.size(), TWO_IMPORTS);
}

TEST_F(SourceTextModuleRecordTest, TestAddLocalExportEntry)
{
    auto *entry = allocator_->New<SourceTextModuleRecord::ExportEntry>("export", "local", nullptr, nullptr);
    bool result = moduleRecord_->AddLocalExportEntry(entry);

    EXPECT_TRUE(result);
    const auto &exports = moduleRecord_->GetLocalExportEntries();
    EXPECT_EQ(exports.size(), 1);
}

TEST_F(SourceTextModuleRecordTest, TestAddLocalExportEntryDuplicate)
{
    auto *entry1 = allocator_->New<SourceTextModuleRecord::ExportEntry>("export", "local1", nullptr, nullptr);
    bool result1 = moduleRecord_->AddLocalExportEntry(entry1);
    EXPECT_TRUE(result1);

    auto *entry2 = allocator_->New<SourceTextModuleRecord::ExportEntry>("export", "local2", nullptr, nullptr);
    bool result2 = moduleRecord_->AddLocalExportEntry(entry2);
    EXPECT_FALSE(result2);
}

TEST_F(SourceTextModuleRecordTest, TestAddLocalExportEntryMultiple)
{
    auto *entry1 = allocator_->New<SourceTextModuleRecord::ExportEntry>("export1", "local1", nullptr, nullptr);
    moduleRecord_->AddLocalExportEntry(entry1);

    auto *entry2 = allocator_->New<SourceTextModuleRecord::ExportEntry>("export2", "local2", nullptr, nullptr);
    moduleRecord_->AddLocalExportEntry(entry2);

    const auto &exports = moduleRecord_->GetLocalExportEntries();
    EXPECT_EQ(exports.size(), TWO_EXPORTS);
}

TEST_F(SourceTextModuleRecordTest, TestAddLocalExportEntrySameLocalDifferentExport)
{
    auto *entry1 = allocator_->New<SourceTextModuleRecord::ExportEntry>("export1", "local", nullptr, nullptr);
    moduleRecord_->AddLocalExportEntry(entry1);

    auto *entry2 = allocator_->New<SourceTextModuleRecord::ExportEntry>("export2", "local", nullptr, nullptr);
    bool result = moduleRecord_->AddLocalExportEntry(entry2);

    EXPECT_TRUE(result);
    const auto &exports = moduleRecord_->GetLocalExportEntries();
    EXPECT_EQ(exports.size(), TWO_EXPORTS);
}

TEST_F(SourceTextModuleRecordTest, TestAddIndirectExportEntry)
{
    int modIdx = moduleRecord_->AddModuleRequest("module.js");

    auto *entry = allocator_->New<SourceTextModuleRecord::ExportEntry>("export", "import", modIdx, nullptr, nullptr);
    bool result = moduleRecord_->AddIndirectExportEntry(entry);

    EXPECT_TRUE(result);
    const auto &exports = moduleRecord_->GetIndirectExportEntries();
    EXPECT_EQ(exports.size(), 1);
}

TEST_F(SourceTextModuleRecordTest, TestAddIndirectExportEntryDuplicate)
{
    int modIdx = moduleRecord_->AddModuleRequest("module.js");

    auto *entry1 = allocator_->New<SourceTextModuleRecord::ExportEntry>("export", "import1", modIdx, nullptr, nullptr);
    bool result1 = moduleRecord_->AddIndirectExportEntry(entry1);
    EXPECT_TRUE(result1);

    auto *entry2 = allocator_->New<SourceTextModuleRecord::ExportEntry>("export", "import2", modIdx, nullptr, nullptr);
    bool result2 = moduleRecord_->AddIndirectExportEntry(entry2);
    EXPECT_FALSE(result2);
}

TEST_F(SourceTextModuleRecordTest, TestAddStarExportEntry)
{
    int modIdx = moduleRecord_->AddModuleRequest("module.js");

    auto *entry = allocator_->New<SourceTextModuleRecord::ExportEntry>(modIdx);
    moduleRecord_->AddStarExportEntry(entry);

    const auto &exports = moduleRecord_->GetStarExportEntries();
    EXPECT_EQ(exports.size(), 1);
}

TEST_F(SourceTextModuleRecordTest, TestAddStarExportEntryMultiple)
{
    int modIdx1 = moduleRecord_->AddModuleRequest("module1.js");
    int modIdx2 = moduleRecord_->AddModuleRequest("module2.js");

    auto *entry1 = allocator_->New<SourceTextModuleRecord::ExportEntry>(modIdx1);
    moduleRecord_->AddStarExportEntry(entry1);

    auto *entry2 = allocator_->New<SourceTextModuleRecord::ExportEntry>(modIdx2);
    moduleRecord_->AddStarExportEntry(entry2);

    const auto &exports = moduleRecord_->GetStarExportEntries();
    EXPECT_EQ(exports.size(), TWO_EXPORTS);
}

TEST_F(SourceTextModuleRecordTest, TestRemoveDefaultLocalExportEntry)
{
    auto *entry = allocator_->New<SourceTextModuleRecord::ExportEntry>(
        SourceTextModuleRecord::DEFAULT_EXTERNAL_NAME,
        SourceTextModuleRecord::DEFAULT_LOCAL_NAME, nullptr, nullptr);
    moduleRecord_->AddLocalExportEntry(entry);

    const auto &exportsBefore = moduleRecord_->GetLocalExportEntries();
    EXPECT_EQ(exportsBefore.size(), 1);

    moduleRecord_->RemoveDefaultLocalExportEntry();

    const auto &exportsAfter = moduleRecord_->GetLocalExportEntries();
    EXPECT_EQ(exportsAfter.size(), 0);
}

TEST_F(SourceTextModuleRecordTest, TestRemoveDefaultLocalExportEntryNoMatch)
{
    auto *entry = allocator_->New<SourceTextModuleRecord::ExportEntry>("export", "local", nullptr, nullptr);
    moduleRecord_->AddLocalExportEntry(entry);

    const auto &exportsBefore = moduleRecord_->GetLocalExportEntries();
    EXPECT_EQ(exportsBefore.size(), 1);

    moduleRecord_->RemoveDefaultLocalExportEntry();

    const auto &exportsAfter = moduleRecord_->GetLocalExportEntries();
    EXPECT_EQ(exportsAfter.size(), 1);
}

TEST_F(SourceTextModuleRecordTest, TestGetModuleRequestIdxFromRegularImport)
{
    int modIdx = moduleRecord_->AddModuleRequest("module.js");

    auto *entry = allocator_->New<SourceTextModuleRecord::ImportEntry>("local", "import", modIdx, nullptr, nullptr);
    moduleRecord_->AddImportEntry(entry);

    int result = moduleRecord_->GetModuleRequestIdx("local");
    EXPECT_EQ(result, modIdx);
}

TEST_F(SourceTextModuleRecordTest, TestGetModuleRequestIdxFromNamespaceImport)
{
    int modIdx = moduleRecord_->AddModuleRequest("module.js");

    auto *entry = allocator_->New<SourceTextModuleRecord::ImportEntry>("local", modIdx, nullptr);
    moduleRecord_->AddStarImportEntry(entry);

    int result = moduleRecord_->GetModuleRequestIdx("local");
    EXPECT_EQ(result, modIdx);
}

TEST_F(SourceTextModuleRecordTest, TestGetModuleRequestIdxNotFound)
{
    int result = moduleRecord_->GetModuleRequestIdx("nonexistent");
    EXPECT_EQ(result, SourceTextModuleRecord::INVALID_MODULEREQUEST_ID);
}

TEST_F(SourceTextModuleRecordTest, TestGetModuleRequestIdxMultipleImports)
{
    int modIdx1 = moduleRecord_->AddModuleRequest("module1.js");
    int modIdx2 = moduleRecord_->AddModuleRequest("module2.js");

    auto *entry1 = allocator_->New<SourceTextModuleRecord::ImportEntry>("local1", "import1", modIdx1, nullptr, nullptr);
    moduleRecord_->AddImportEntry(entry1);

    auto *entry2 = allocator_->New<SourceTextModuleRecord::ImportEntry>("local2", "import2", modIdx2, nullptr, nullptr);
    moduleRecord_->AddImportEntry(entry2);

    int result1 = moduleRecord_->GetModuleRequestIdx("local1");
    int result2 = moduleRecord_->GetModuleRequestIdx("local2");

    EXPECT_EQ(result1, modIdx1);
    EXPECT_EQ(result2, modIdx2);
}

TEST_F(SourceTextModuleRecordTest, TestCheckImplicitIndirectExportExportEntry)
{
    int modIdx = moduleRecord_->AddModuleRequest("module.js");

    auto *importEntry = allocator_->New<SourceTextModuleRecord::ImportEntry>("x", "x", modIdx, nullptr, nullptr);
    moduleRecord_->AddImportEntry(importEntry);

    auto *exportEntry = allocator_->New<SourceTextModuleRecord::ExportEntry>("x", "x", nullptr, nullptr);
    bool result = moduleRecord_->CheckImplicitIndirectExport(exportEntry);

    EXPECT_TRUE(result);
    const auto &indirectExports = moduleRecord_->GetIndirectExportEntries();
    EXPECT_EQ(indirectExports.size(), 1);
}

TEST_F(SourceTextModuleRecordTest, TestCheckImplicitIndirectExportExportEntryNoMatch)
{
    auto *exportEntry = allocator_->New<SourceTextModuleRecord::ExportEntry>("export", "local", nullptr, nullptr);
    bool result = moduleRecord_->CheckImplicitIndirectExport(exportEntry);

    EXPECT_FALSE(result);
    const auto &indirectExports = moduleRecord_->GetIndirectExportEntries();
    EXPECT_EQ(indirectExports.size(), 0);
}

TEST_F(SourceTextModuleRecordTest, TestCheckImplicitIndirectExportImportEntry)
{
    auto *exportEntry = allocator_->New<SourceTextModuleRecord::ExportEntry>("x", "x", nullptr, nullptr);
    moduleRecord_->AddLocalExportEntry(exportEntry);

    int modIdx = moduleRecord_->AddModuleRequest("module.js");
    auto *importEntry = allocator_->New<SourceTextModuleRecord::ImportEntry>("x", "x", modIdx, nullptr, nullptr);
    moduleRecord_->AddImportEntry(importEntry);

    const auto &localExports = moduleRecord_->GetLocalExportEntries();
    EXPECT_EQ(localExports.size(), 0);

    const auto &indirectExports = moduleRecord_->GetIndirectExportEntries();
    EXPECT_EQ(indirectExports.size(), 1);
}

TEST_F(SourceTextModuleRecordTest, TestCheckImplicitIndirectExportImportEntryNoMatch)
{
    int modIdx = moduleRecord_->AddModuleRequest("module.js");

    auto *importEntry = allocator_->New<SourceTextModuleRecord::ImportEntry>(
        "local", "import", modIdx, nullptr, nullptr);
    moduleRecord_->AddImportEntry(importEntry);

    const auto &localExports = moduleRecord_->GetLocalExportEntries();
    EXPECT_EQ(localExports.size(), 0);

    const auto &indirectExports = moduleRecord_->GetIndirectExportEntries();
    EXPECT_EQ(indirectExports.size(), 0);
}

TEST_F(SourceTextModuleRecordTest, TestCheckImplicitIndirectExportImportEntryMultipleExports)
{
    auto *exportEntry1 = allocator_->New<SourceTextModuleRecord::ExportEntry>("export1", "x", nullptr, nullptr);
    moduleRecord_->AddLocalExportEntry(exportEntry1);

    auto *exportEntry2 = allocator_->New<SourceTextModuleRecord::ExportEntry>("export2", "x", nullptr, nullptr);
    moduleRecord_->AddLocalExportEntry(exportEntry2);

    int modIdx = moduleRecord_->AddModuleRequest("module.js");
    auto *importEntry = allocator_->New<SourceTextModuleRecord::ImportEntry>("x", "x", modIdx, nullptr, nullptr);
    moduleRecord_->AddImportEntry(importEntry);

    const auto &localExports = moduleRecord_->GetLocalExportEntries();
    EXPECT_EQ(localExports.size(), 0);

    const auto &indirectExports = moduleRecord_->GetIndirectExportEntries();
    EXPECT_EQ(indirectExports.size(), TWO_EXPORTS);
}

TEST_F(SourceTextModuleRecordTest, TestModuleRequestRecordOperatorLess)
{
    SourceTextModuleRecord::ModuleRequestRecord record1("module.js", false);
    SourceTextModuleRecord::ModuleRequestRecord record2("module.js", true);
    SourceTextModuleRecord::ModuleRequestRecord record3("zmodule.js", false);

    EXPECT_FALSE(record1 < record2);
    EXPECT_TRUE(record1 < record3);
    EXPECT_TRUE(record2 < record1);
    EXPECT_FALSE(record3 < record1);
}

TEST_F(SourceTextModuleRecordTest, TestModuleRequestRecordOperatorLessSame)
{
    SourceTextModuleRecord::ModuleRequestRecord record1("module.js", false);
    SourceTextModuleRecord::ModuleRequestRecord record2("module.js", false);

    EXPECT_FALSE(record1 < record2);
    EXPECT_FALSE(record2 < record1);
}

TEST_F(SourceTextModuleRecordTest, TestExportEntrySetAsConstant)
{
    auto *entry = allocator_->New<SourceTextModuleRecord::ExportEntry>("export", "local", nullptr, nullptr);
    EXPECT_FALSE(entry->isConstant_);

    entry->SetAsConstant();
    EXPECT_TRUE(entry->isConstant_);
}

TEST_F(SourceTextModuleRecordTest, TestNewEntry)
{
    auto *importEntry = moduleRecord_->NewEntry<SourceTextModuleRecord::ImportEntry>(
        "local", "import", 0, nullptr, nullptr);
    EXPECT_NE(importEntry, nullptr);
    EXPECT_EQ(importEntry->localName_, "local");
    EXPECT_EQ(importEntry->importName_, "import");

    auto *exportEntry = moduleRecord_->NewEntry<SourceTextModuleRecord::ExportEntry>(
        "export", "local", nullptr, nullptr);
    EXPECT_NE(exportEntry, nullptr);
    EXPECT_EQ(exportEntry->exportName_, "export");
    EXPECT_EQ(exportEntry->localName_, "local");
}

TEST_F(SourceTextModuleRecordTest, TestImportEntryConstructors)
{
    auto *entry1 = allocator_->New<SourceTextModuleRecord::ImportEntry>("local", "import", FIRST_MODULE_REQUEST_IDX,
        nullptr, nullptr);
    EXPECT_EQ(entry1->localName_, "local");
    EXPECT_EQ(entry1->importName_, "import");
    EXPECT_EQ(entry1->moduleRequestIdx_, FIRST_MODULE_REQUEST_IDX);

    auto *entry2 = allocator_->New<SourceTextModuleRecord::ImportEntry>("local", SECOND_MODULE_REQUEST_IDX, nullptr);
    EXPECT_EQ(entry2->localName_, "local");
    EXPECT_TRUE(entry2->importName_.Empty());
    EXPECT_EQ(entry2->moduleRequestIdx_, SECOND_MODULE_REQUEST_IDX);
}

TEST_F(SourceTextModuleRecordTest, TestExportEntryConstructors)
{
    auto *entry1 = allocator_->New<SourceTextModuleRecord::ExportEntry>(FIRST_MODULE_REQUEST_IDX);
    EXPECT_EQ(entry1->moduleRequestIdx_, FIRST_MODULE_REQUEST_IDX);

    auto *entry2 = allocator_->New<SourceTextModuleRecord::ExportEntry>("export", "local", nullptr, nullptr);
    EXPECT_EQ(entry2->exportName_, "export");
    EXPECT_EQ(entry2->localName_, "local");
    EXPECT_EQ(entry2->moduleRequestIdx_, SourceTextModuleRecord::INVALID_MODULEREQUEST_ID);

    auto *entry3 = allocator_->New<SourceTextModuleRecord::ExportEntry>("export", "import",
        SECOND_MODULE_REQUEST_IDX, nullptr, nullptr);
    EXPECT_EQ(entry3->exportName_, "export");
    EXPECT_EQ(entry3->importName_, "import");
    EXPECT_EQ(entry3->moduleRequestIdx_, SECOND_MODULE_REQUEST_IDX);
}

TEST_F(SourceTextModuleRecordTest, TestMixedImportExportScenario)
{
    int modIdx1 = moduleRecord_->AddModuleRequest("module1.js");
    int modIdx2 = moduleRecord_->AddModuleRequest("module2.js");

    auto *importEntry = allocator_->New<SourceTextModuleRecord::ImportEntry>("x", "x", modIdx1, nullptr, nullptr);
    moduleRecord_->AddImportEntry(importEntry);

    auto *exportEntry = allocator_->New<SourceTextModuleRecord::ExportEntry>("y", "y", nullptr, nullptr);
    moduleRecord_->AddLocalExportEntry(exportEntry);

    auto *starImportEntry = allocator_->New<SourceTextModuleRecord::ImportEntry>("ns", modIdx2, nullptr);
    moduleRecord_->AddStarImportEntry(starImportEntry);

    auto *indirectExportEntry = allocator_->New<SourceTextModuleRecord::ExportEntry>(
        "z", "z", modIdx1, nullptr, nullptr);
    moduleRecord_->AddIndirectExportEntry(indirectExportEntry);

    auto *starExportEntry = allocator_->New<SourceTextModuleRecord::ExportEntry>(modIdx2);
    moduleRecord_->AddStarExportEntry(starExportEntry);

    EXPECT_EQ(moduleRecord_->GetRegularImportEntries().size(), 1);
    EXPECT_EQ(moduleRecord_->GetLocalExportEntries().size(), 1);
    EXPECT_EQ(moduleRecord_->GetNamespaceImportEntries().size(), 1);
    EXPECT_EQ(moduleRecord_->GetIndirectExportEntries().size(), 1);
    EXPECT_EQ(moduleRecord_->GetStarExportEntries().size(), 1);
}

TEST_F(SourceTextModuleRecordTest, TestReExportScenario)
{
    int modIdx = moduleRecord_->AddModuleRequest("module.js");

    auto *importEntry = allocator_->New<SourceTextModuleRecord::ImportEntry>("x", "x", modIdx, nullptr, nullptr);
    moduleRecord_->AddImportEntry(importEntry);

    auto *exportEntry = allocator_->New<SourceTextModuleRecord::ExportEntry>("exportX", "x", nullptr, nullptr);
    moduleRecord_->AddLocalExportEntry(exportEntry);

    const auto &localExports = moduleRecord_->GetLocalExportEntries();
    EXPECT_EQ(localExports.size(), 0);

    const auto &indirectExports = moduleRecord_->GetIndirectExportEntries();
    EXPECT_EQ(indirectExports.size(), 1);
    EXPECT_EQ(indirectExports[0]->exportName_, "exportX");
    EXPECT_EQ(indirectExports[0]->importName_, "x");
    EXPECT_EQ(indirectExports[0]->moduleRequestIdx_, modIdx);
}

TEST_F(SourceTextModuleRecordTest, TestDuplicateExportInLocalAndIndirect)
{
    int modIdx = moduleRecord_->AddModuleRequest("module.js");

    auto *localExportEntry = allocator_->New<SourceTextModuleRecord::ExportEntry>("x", "local", nullptr, nullptr);
    moduleRecord_->AddLocalExportEntry(localExportEntry);

    auto *indirectExportEntry = allocator_->New<SourceTextModuleRecord::ExportEntry>(
        "x", "x", modIdx, nullptr, nullptr);
    bool result = moduleRecord_->AddIndirectExportEntry(indirectExportEntry);

    EXPECT_FALSE(result);
}

TEST_F(SourceTextModuleRecordTest, TestDuplicateExportInIndirect)
{
    int modIdx = moduleRecord_->AddModuleRequest("module.js");

    auto *indirectExportEntry1 = allocator_->New<SourceTextModuleRecord::ExportEntry>(
        "x", "x1", modIdx, nullptr, nullptr);
    moduleRecord_->AddIndirectExportEntry(indirectExportEntry1);

    auto *indirectExportEntry2 = allocator_->New<SourceTextModuleRecord::ExportEntry>(
        "x", "x2", modIdx, nullptr, nullptr);
    bool result = moduleRecord_->AddIndirectExportEntry(indirectExportEntry2);

    EXPECT_FALSE(result);
}

TEST_F(SourceTextModuleRecordTest, TestLazyImportTracking)
{
    EXPECT_FALSE(moduleRecord_->HasLazyImport());

    moduleRecord_->AddModuleRequest("module.js", false);
    EXPECT_FALSE(moduleRecord_->HasLazyImport());

    moduleRecord_->AddModuleRequest("lazy.js", true);
    EXPECT_TRUE(moduleRecord_->HasLazyImport());
}

TEST_F(SourceTextModuleRecordTest, TestMultipleModuleRequestsSameSourceDifferentLazy)
{
    int idx1 = moduleRecord_->AddModuleRequest("module.js", false);
    EXPECT_EQ(idx1, 0);
    EXPECT_EQ(moduleRecord_->GetModuleRequests().size(), 1);

    int idx2 = moduleRecord_->AddModuleRequest("module.js", true);
    EXPECT_EQ(idx2, 1);
    EXPECT_EQ(moduleRecord_->GetModuleRequests().size(), TWO_MODULE_REQUESTS);

    int idx3 = moduleRecord_->AddModuleRequest("module.js", false);
    EXPECT_EQ(idx3, 0);
    EXPECT_EQ(moduleRecord_->GetModuleRequests().size(), TWO_MODULE_REQUESTS);
}

TEST_F(SourceTextModuleRecordTest, TestMultipleModuleRequestsSameSourceSameLazy)
{
    int idx1 = moduleRecord_->AddModuleRequest("module.js", true);
    EXPECT_EQ(idx1, 0);
    EXPECT_EQ(moduleRecord_->GetModuleRequests().size(), 1);

    int idx2 = moduleRecord_->AddModuleRequest("module.js", true);
    EXPECT_EQ(idx2, 0);
    EXPECT_EQ(moduleRecord_->GetModuleRequests().size(), 1);
}

}  // namespace panda::es2panda::parser
