/**
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include <cstdio>
#include <string>
#include "os/filesystem.h"
#include "path.h"

namespace ark::es2panda::util {

constexpr size_t ALLOWED_EXTENSIONS_SIZE = 8;
inline constexpr std::array<std::string_view, ALLOWED_EXTENSIONS_SIZE> SUPPORTED_EXTENSIONS = {
    ".d.ets", ".ets", ".d.sts", ".sts", ".d.ts", ".ts", ".js", ".abc"};

Path::Path() = default;

Path::Path(const util::StringView &absolutePath, ArenaAllocator *allocator)
{
    Initializer(absolutePath.Mutf8(), allocator);
}

static bool EndsWith(std::string_view str, std::string_view suffix)
{
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

void Path::Initializer(const std::string &path, ArenaAllocator *allocator)
{
    isRelative_ = false;
    allocator_ = allocator;
    path_ = util::UString(path, allocator).View();
    if (*(path_.Bytes()) == '.') {
        isRelative_ = true;
    }

    absolutePath_ = util::UString(os::GetAbsolutePath(path_.Utf8()), allocator_).View();

    InitializeFileNameWithExtension();
    InitializeFileExtension();
    InitializeFileName();
    InitializeParentFolder();
    InitializeAbsoluteParentFolder();
}

void Path::InitializeFileName()
{
    if (path_.Empty()) {
        return;
    }

    if (GetExtension().Empty()) {
        fileName_ = fileNameWithExtension_;
        return;
    }

    for (auto extension : SUPPORTED_EXTENSIONS) {
        if (EndsWith(fileNameWithExtension_.Utf8(), extension)) {
            fileName_ = fileNameWithExtension_.Substr(0, fileNameWithExtension_.Length() - extension.length());
            return;
        }
    }

    size_t extensionPosition = fileNameWithExtension_.Mutf8().find_last_of('.');
    fileName_ = fileNameWithExtension_.Substr(0, extensionPosition);
}

void Path::InitializeFileNameWithExtension()
{
    if (path_.Empty()) {
        return;
    }

    size_t position = path_.Mutf8().find_last_of(PATH_DELIMITER);
    fileNameWithExtension_ = path_.Substr(position + 1, path_.Length());
}

void Path::InitializeFileExtension()
{
    if (path_.Empty()) {
        return;
    }

    for (auto extension : SUPPORTED_EXTENSIONS) {
        if (EndsWith(path_.Utf8(), extension)) {
            fileExtension_ = util::UString(extension, allocator_).View();
            return;
        }
    }

    size_t position = path_.Mutf8().find_last_of('.');
    if (position != std::string::npos && position + 1 <= path_.Length()) {
        fileExtension_ = path_.Substr(position + 1, path_.Length());
    }
}

void Path::InitializeAbsoluteParentFolder()
{
    if (path_.Empty()) {
        return;
    }

    size_t position = absolutePath_.Mutf8().find_last_of(PATH_DELIMITER);

    absoluteParentFolder_ = absolutePath_.Substr(0, position);
}

void Path::InitializeParentFolder()
{
    if (path_.Empty()) {
        return;
    }

    size_t position = path_.Mutf8().find_last_of(PATH_DELIMITER);

    parentFolder_ = path_.Substr(0, position);
}

void Path::InitializeBasePath(std::string basePath)
{
    if (!basePath.empty() && basePath.back() == PATH_DELIMITER) {
        basePath_ = util::UString(basePath.substr(0, basePath.length() - 1), allocator_).View();
    } else {
        basePath_ = util::UString(basePath, allocator_).View();
    }

    isRelative_ = true;
}

Path::Path(const util::StringView &relativePath, const util::StringView &basePath, ArenaAllocator *allocator)
{
    Initializer(relativePath.Mutf8(), allocator);
    InitializeBasePath(basePath.Mutf8());
}

Path::Path(const std::string &absolutePath, ArenaAllocator *allocator)
{
    Initializer(absolutePath, allocator);
}

Path::Path(const std::string &relativePath, const std::string &basePath, ArenaAllocator *allocator)
{
    Initializer(relativePath, allocator);
    InitializeBasePath(basePath);
}

bool Path::IsRelative()
{
    return isRelative_;
}

bool Path::IsAbsolute()
{
    return !isRelative_;
}

const util::StringView &Path::GetPath() const
{
    return path_;
}

const util::StringView &Path::GetAbsolutePath() const
{
    return absolutePath_;
}

const util::StringView &Path::GetExtension() const
{
    return fileExtension_;
}

const util::StringView &Path::GetFileName() const
{
    return fileName_;
}

const util::StringView &Path::GetFileNameWithExtension() const
{
    return fileNameWithExtension_;
}

const util::StringView &Path::GetParentFolder() const
{
    return parentFolder_;
}

const util::StringView &Path::GetAbsoluteParentFolder() const
{
    return absoluteParentFolder_;
}

}  // namespace ark::es2panda::util
