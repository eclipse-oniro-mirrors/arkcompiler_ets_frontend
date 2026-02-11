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

#include <mem/pool_manager.h>
#include <protobufSnapshotGenerator.h>

#include <cstdio>

#include <gtest/gtest.h>

namespace panda::test {

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

class ProgramCacheTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mm_ = std::make_unique<MemManager>();
        pandasm::Program prog;
        programCache_ = std::make_unique<es2panda::util::ProgramCache>(GenerateHash(), std::move(prog));
    }

    void TearDown() override
    {
        programCache_.reset();
        mm_.reset();
        std::ifstream file(cacheFilePath_);
        if (file.good()) {
            file.close();
            remove(cacheFilePath_.c_str());
        }
    }

    uint32_t GenerateHash()
    {
        return GetHash32String(reinterpret_cast<const uint8_t *>(hashKey_.c_str()));
    }

    std::unique_ptr<MemManager> mm_;
    std::unique_ptr<es2panda::util::ProgramCache> programCache_;
    std::string hashKey_ { "Test" };
    std::string cacheFilePath_;
};

TEST_F(ProgramCacheTest, ValidProgramCache)
{
    cacheFilePath_ = "valid_program_cache.protoBin";
    proto::ProtobufSnapshotGenerator::UpdateCacheFile(programCache_.get(), cacheFilePath_);

    ArenaAllocator allocator(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);
    auto programCache = proto::ProtobufSnapshotGenerator::GetCacheContext(cacheFilePath_, GenerateHash(), &allocator);
    ASSERT_NE(programCache, nullptr);
    ASSERT_EQ(programCache->hashCode, GenerateHash());
}

TEST_F(ProgramCacheTest, InvalidProgramCache)
{
    cacheFilePath_ = "invalid_program_cache.protoBin";
    proto::ProtobufSnapshotGenerator::UpdateCacheFile(programCache_.get(), cacheFilePath_);

    // modify hash key to invalidate cache
    hashKey_ += "#invalid";

    ArenaAllocator allocator(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);
    auto programCache = proto::ProtobufSnapshotGenerator::GetCacheContext(cacheFilePath_, GenerateHash(), &allocator);
    ASSERT_EQ(programCache, nullptr);
}

TEST_F(ProgramCacheTest, ValidAbcProgramsCache)
{
    cacheFilePath_ = "valid_abc_programs_cache.protoBin";

    std::map<std::string, es2panda::util::ProgramCache *> programsCache;
    programsCache.emplace("module1", programCache_.get());

    es2panda::util::AbcProgramsCache abcProgramsCache(GenerateHash(), programsCache);
    proto::ProtobufSnapshotGenerator::UpdateAbcCacheFile(&abcProgramsCache, cacheFilePath_);

    ArenaAllocator allocator(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);
    auto abcCache = proto::ProtobufSnapshotGenerator::GetAbcInputCacheContext(cacheFilePath_, GenerateHash(),
                                                                              &allocator);
    ASSERT_NE(abcCache, nullptr);
    ASSERT_EQ(abcCache->hashCode, GenerateHash());
    ASSERT_EQ(abcCache->programsCache.size(), 1);
    ASSERT_NE(abcCache->programsCache.find("module1"), abcCache->programsCache.end());
}

TEST_F(ProgramCacheTest, InvalidAbcProgramsCache)
{
    cacheFilePath_ = "invalid_abc_programs_cache.protoBin";

    std::map<std::string, es2panda::util::ProgramCache *> programsCache;
    programsCache.emplace("module1", programCache_.get());

    es2panda::util::AbcProgramsCache abcProgramsCache(GenerateHash(), programsCache);
    proto::ProtobufSnapshotGenerator::UpdateAbcCacheFile(&abcProgramsCache, cacheFilePath_);

    // modify hash key to invalidate cache
    hashKey_ += "#invalid";

    ArenaAllocator allocator(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);
    auto abcCache = proto::ProtobufSnapshotGenerator::GetAbcInputCacheContext(cacheFilePath_, GenerateHash(),
                                                                              &allocator);
    ASSERT_EQ(abcCache, nullptr);
}

} // namespace panda::test