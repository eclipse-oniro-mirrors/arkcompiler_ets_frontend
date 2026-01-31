/**
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_UTIL_PATH_H
#define ES2PANDA_UTIL_PATH_H

#include "util/ustring.h"

namespace ark::es2panda::util {
const char PATH_DELIMITER =
#ifdef _WIN32
    '\\';
#else
    '/';
#endif

class Path {
public:
    Path() = delete;
    ~Path() = default;

    explicit Path(ArenaAllocator *allocator);
    explicit Path(const std::string &absolutePath, ArenaAllocator *allocator);
    explicit Path(const std::string &relativePath, const std::string &basePath, ArenaAllocator *allocator);
    explicit Path(const util::StringView &absolutePath, ArenaAllocator *allocator);
    explicit Path(const util::StringView &relativePath, const util::StringView &basePath, ArenaAllocator *allocator);

    DEFAULT_COPY_SEMANTIC(Path);
    DEFAULT_MOVE_SEMANTIC(Path);

    bool IsRelative();
    bool IsAbsolute();

    const util::StringView &GetPath() const;
    const util::StringView &GetAbsolutePath() const;
    const util::StringView &GetExtension() const;
    const util::StringView &GetFileName() const;
    const util::StringView &GetFileNameWithExtension() const;
    const util::StringView &GetParentFolder() const;
    const util::StringView &GetAbsoluteParentFolder() const;
    constexpr static char GetPathDelimiter()
    {
        return PATH_DELIMITER;
    }

private:
    ArenaAllocator *allocator_ {};
    bool isRelative_ {};
    util::StringView path_ {};
    util::StringView basePath_ {};
    util::StringView absolutePath_ {};
    util::StringView fileName_ {};
    util::StringView fileNameWithExtension_ {};
    util::StringView fileExtension_ {};
    util::StringView parentFolder_ {};
    util::StringView absoluteParentFolder_ {};

    void Initializer(const std::string &absolutePath);
    void InitializeBasePath(std::string basePath);
    void InitializeAbsolutePath();
    void InitializeParentFolder();
    void InitializeAbsoluteParentFolder();
    void InitializeFileName();
    void InitializeFileNameWithExtension();
    void InitializeFileExtension();
};
}  // namespace ark::es2panda::util

#endif
