/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fsQueryCache.h"

#include <libarkbase/os/file.h>

namespace ark::es2panda::util {

bool FsQueryCache::IsRegularFile(const std::string &path, [[maybe_unused]] bool caseSensitive)
{
#if defined(PANDA_TARGET_WINDOWS)
    if (caseSensitive) {
        return GetOrInsert(isRegularFileCaseSensitive_, path, [](const std::string &filePath) {
            return ark::os::file::File::IsRegularFileCaseSensitive(filePath);
        });
    }
#endif
    return GetOrInsert(isRegularFile_, path,
                       [](const std::string &filePath) { return ark::os::file::File::IsRegularFile(filePath); });
}

bool FsQueryCache::IsDirectory(const std::string &path)
{
    return GetOrInsert(isDirectory_, path,
                       [](const std::string &filePath) { return ark::os::file::File::IsDirectory(filePath); });
}

void FsQueryCache::Clear()
{
    isRegularFile_.clear();
#if defined(PANDA_TARGET_WINDOWS)
    isRegularFileCaseSensitive_.clear();
#endif
    isDirectory_.clear();
}

template <typename Value, typename Callback>
Value FsQueryCache::GetOrInsert(std::unordered_map<std::string, Value> &cache, const std::string &path,
                                Callback callback)
{
    auto [it, inserted] = cache.try_emplace(path);
    if (inserted) {
        it->second = callback(path);
    }
    return it->second;
}

}  // namespace ark::es2panda::util
