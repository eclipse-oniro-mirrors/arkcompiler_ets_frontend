/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_EVALUATE_DEBUG_INFO_STORAGE_H
#define ES2PANDA_EVALUATE_DEBUG_INFO_STORAGE_H

#include "evaluate/evaluateContext.h"
#include "evaluate/importExportTable.h"
#include "util/ustring.h"

#include "libarkfile/debug_info_extractor.h"
#include "libarkfile/file.h"
#include "libarkfile/class_data_accessor.h"

#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace ark::es2panda::util {
class Options;
}  // namespace ark::es2panda::util

namespace ark::es2panda::evaluate {

struct FileDebugInfo final {
    using RecordsMap = ArenaUnorderedMap<util::StringView, panda_file::File::EntityId>;

    FileDebugInfo() = delete;
    explicit FileDebugInfo(std::unique_ptr<const panda_file::File> &&pandaFile, panda_file::File::EntityId classId,
                           std::string_view module)
        : pf(std::move(pandaFile)), globalClassAcc(*pf, classId), moduleName(module)
    {
        ES2PANDA_ASSERT(pf);
    }

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    std::unique_ptr<const panda_file::File> pf {nullptr};
    panda_file::ClassDataAccessor globalClassAcc;
    std::string_view moduleName;
    std::string_view sourceFilePath;
    std::optional<ImportExportTable> importExportTable;
    std::optional<RecordsMap> records;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

// Context-independent debug info lazy-loading storage.
// All "find" methods must accept paths to source files.
class DebugInfoStorage final {
public:
    explicit DebugInfoStorage(const util::Options &options, ArenaAllocator *allocator);

    NO_COPY_SEMANTIC(DebugInfoStorage);
    NO_MOVE_SEMANTIC(DebugInfoStorage);

    ~DebugInfoStorage() = default;

    ArenaAllocator *Allocator()
    {
        return allocator_;
    }

    [[nodiscard]] bool FillEvaluateContext(EvaluateContext &context);

    const panda_file::File *GetPandaFile(std::string_view filePath);
    const ImportExportTable *GetImportExportTable(std::string_view filePath);
    panda_file::ClassDataAccessor *GetGlobalClassAccessor(std::string_view filePath);
    std::string_view GetModuleName(std::string_view filePath);

    FileDebugInfo *GetDebugInfoByModuleName(std::string_view moduleName) const;

    /**
     * @brief Returns class'es panda file Id on success, invalid EntityId otherwise
     */
    panda_file::File::EntityId FindClass(std::string_view filePath, std::string_view className);

    template <typename Callback>
    void EnumerateContextFiles(const Callback &cb)
    {
        for (const auto &[sourceFilePath, debugInfo] : sourceFileToDebugInfo_) {
            if (!cb(sourceFilePath, debugInfo->pf.get(), debugInfo->globalClassAcc, debugInfo->moduleName)) {
                return;
            }
        }
    }

private:
    using DebugInfoMap = ArenaUnorderedMap<std::string_view, FileDebugInfo *>;
    static constexpr std::string_view ANNOTATION_MODULE = "Lstd/annotations/Module;";

private:
    void LoadFileDebugInfo(std::string_view pfPath);

    const ImportExportTable &LazyLoadImportExportTable(FileDebugInfo *info);
    const FileDebugInfo::RecordsMap &LazyLoadRecords(FileDebugInfo *info);

private:
    ArenaAllocator *allocator_ {nullptr};

    // Mapping from sources' files names into the corresponding debug information struct.
    // Used for fast lookups basing on imports/exports tables.
    DebugInfoMap sourceFileToDebugInfo_;
    // Mapping from module names into the corresponding debug information struct.
    // Used for fast lookups during inheritance chain resolution.
    DebugInfoMap moduleNameToDebugInfo_;
};

}  // namespace ark::es2panda::evaluate

#endif  // ES2PANDA_EVALUATE_DEBUG_INFO_STORAGE_H
