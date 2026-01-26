/**
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

#ifndef MERGE_ABC_POROBUFSNAPSHOTGENERATOR_H
#define MERGE_ABC_POROBUFSNAPSHOTGENERATOR_H

#include "assemblyProgramProto.h"
#include "compositeProgram.pb.h"
#include "es2panda.h"

namespace panda::proto {
class ProtobufSnapshotGenerator {
public:
    static void GenerateSnapshot(const panda::pandasm::Program &program, const std::string &outputName);
    static void GenerateProgram(const std::string &inputName, panda::pandasm::Program &prog,
                                panda::ArenaAllocator *allocator);
    static panda::es2panda::util::ProgramCache *GetCacheContext(const std::string &cacheFilePath, uint32_t newFileHash,
        panda::ArenaAllocator *allocator);
    static void UpdateCacheFile(const panda::es2panda::util::ProgramCache *programCache,
        const std::string &cacheFilePath);
    static panda::es2panda::util::AbcProgramsCache *GetAbcInputCacheContext(const std::string &cacheFilePath,
        uint32_t newFileHash, panda::ArenaAllocator *allocator);
    static void UpdateAbcCacheFile(const panda::es2panda::util::AbcProgramsCache *abcProgramsCache,
        const std::string &cacheFilePath);

    template<class T>
    static void SerializeToProtoPath(T &protoCache, std::string cacheFilePath)
    {
        std::fstream output = panda::es2panda::util::Helpers::FileStream<std::fstream>(
            panda::os::file::File::GetExtendedFilePath(cacheFilePath),
            std::ios::out | std::ios::trunc | std::ios::binary);
        if (!output) {
            std::cerr << "Failed to create cache file: " << cacheFilePath << std::endl;
            return;
        }
        protoCache.SerializeToOstream(&output);
        output.close();
    }
};
} // panda::proto
#endif
