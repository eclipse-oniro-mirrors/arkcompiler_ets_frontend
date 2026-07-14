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

#ifndef ES2PANDA_UTIL_FS_QUERY_CACHE_H
#define ES2PANDA_UTIL_FS_QUERY_CACHE_H

#include "util/es2pandaMacros.h"

#include <string>
#include <unordered_map>

namespace ark::es2panda::util {

class FsQueryCache {
public:
    FsQueryCache() = default;
    ~FsQueryCache() = default;

    NO_COPY_SEMANTIC(FsQueryCache);
    NO_MOVE_SEMANTIC(FsQueryCache);

    bool IsRegularFile(const std::string &path, bool caseSensitive = false);
    bool IsDirectory(const std::string &path);
    void Clear();

private:
    template <typename Value, typename Callback>
    static Value GetOrInsert(std::unordered_map<std::string, Value> &cache, const std::string &path, Callback callback);

    std::unordered_map<std::string, bool> isRegularFile_;
#if defined(PANDA_TARGET_WINDOWS)
    std::unordered_map<std::string, bool> isRegularFileCaseSensitive_;
#endif
    std::unordered_map<std::string, bool> isDirectory_;
};

}  // namespace ark::es2panda::util

#endif  // ES2PANDA_UTIL_FS_QUERY_CACHE_H
