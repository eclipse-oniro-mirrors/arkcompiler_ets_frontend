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

#include "evaluate/debugInfoStorage.h"
#include "assembler/assembly-type.h"
#include "generated/signatures.h"
#include "evaluate/helpers.h"

#include "libarkfile/class_data_accessor-inl.h"
#include "libarkfile/file-inl.h"

namespace ark::es2panda::evaluate {

namespace {

std::string GetFullRecordName(const panda_file::File &pf, const panda_file::File::EntityId &classId)
{
    std::string name = utf::Mutf8AsCString(pf.GetStringData(classId).data);

    auto type = pandasm::Type::FromDescriptor(name);
    type = pandasm::Type(type.GetNameWithoutRank(), type.GetRank());

    return type.GetPandasmName();
}

}  // namespace

ImportExportTable::ImportExportTable(ArenaAllocator *allocator)
    : imports_(allocator->Adapter()), exports_(allocator->Adapter())
{
}

DebugInfoStorage::DebugInfoStorage(const util::Options &options, ArenaAllocator *allocator)
    : allocator_(allocator), sourceFileToDebugInfo_(allocator->Adapter()), moduleNameToDebugInfo_(allocator->Adapter())
{
    for (const auto &pfPath : options.GetDebuggerEvalPandaFiles()) {
        LoadFileDebugInfo(pfPath);
    }
}

void DebugInfoStorage::LoadFileDebugInfo(std::string_view pfPath)
{
    auto pf = panda_file::OpenPandaFile(pfPath);
    if (!pf) {
        LOG(FATAL, ES2PANDA) << "Failed to load a provided abc file: " << pfPath;
    }

    for (auto id : pf->GetClasses()) {
        panda_file::File::EntityId classId(id);
        if (pf->IsExternal(classId)) {
            continue;
        }

        panda_file::ClassDataAccessor cda(*pf, classId);
        bool isModule = false;
        cda.EnumerateAnnotation(ANNOTATION_MODULE.data(), [&isModule](auto &) { return (isModule = true); });
        if (!isModule) {
            continue;
        }
        auto recordName = GetFullRecordName(*pf, classId);

        std::string_view moduleName = helpers::SplitRecordName(recordName).first;
        auto *debugInfo = allocator_->New<FileDebugInfo>(std::move(pf), classId, moduleName);
        auto sourceFileId = debugInfo->globalClassAcc.GetSourceFileId();
        ES2PANDA_ASSERT(sourceFileId.has_value());
        std::string_view sourceFileName = utf::Mutf8AsCString(debugInfo->pf->GetStringData(*sourceFileId).data);
        debugInfo->sourceFilePath = sourceFileName;

        sourceFileToDebugInfo_.emplace(sourceFileName, debugInfo);
        moduleNameToDebugInfo_.emplace(moduleName, debugInfo);
        return;
    }

    LOG(FATAL, ES2PANDA) << "ETSGLOBAL not found in provided file: " << pfPath;
}

const panda_file::File *DebugInfoStorage::GetPandaFile(std::string_view filePath)
{
    auto iter = sourceFileToDebugInfo_.find(filePath);
    if (iter == sourceFileToDebugInfo_.end()) {
        return nullptr;
    }
    return iter->second->pf.get();
}

const ImportExportTable *DebugInfoStorage::GetImportExportTable(std::string_view filePath)
{
    auto iter = sourceFileToDebugInfo_.find(filePath);
    if (iter == sourceFileToDebugInfo_.end()) {
        return nullptr;
    }
    return &LazyLoadImportExportTable(iter->second);
}

panda_file::ClassDataAccessor *DebugInfoStorage::GetGlobalClassAccessor(std::string_view filePath)
{
    auto iter = sourceFileToDebugInfo_.find(filePath);
    if (iter == sourceFileToDebugInfo_.end()) {
        return nullptr;
    }
    return &iter->second->globalClassAcc;
}

std::string_view DebugInfoStorage::GetModuleName(std::string_view filePath)
{
    auto iter = sourceFileToDebugInfo_.find(filePath);
    if (iter == sourceFileToDebugInfo_.end()) {
        return {};
    }
    return iter->second->moduleName;
}

panda_file::File::EntityId DebugInfoStorage::FindClass(std::string_view filePath, std::string_view className)
{
    auto iter = sourceFileToDebugInfo_.find(filePath);
    if (iter == sourceFileToDebugInfo_.end()) {
        return panda_file::File::EntityId();
    }

    const auto &records = LazyLoadRecords(iter->second);

    auto classIter = records.find(className);
    return classIter == records.end() ? panda_file::File::EntityId() : classIter->second;
}

bool DebugInfoStorage::FillEvaluateContext(EvaluateContext &context)
{
    const auto *contextPandaFile = GetPandaFile(context.sourceFilePath.Utf8());
    if (contextPandaFile == nullptr) {
        LOG(WARNING, ES2PANDA) << "Could not find context file: " << context.sourceFilePath << std::endl;
        return false;
    }

    context.file = contextPandaFile;
    context.extractor = std::make_unique<panda_file::DebugInfoExtractor>(contextPandaFile);

    for (auto methodId : context.extractor->GetMethodIdList()) {
        for (const auto &entry : context.extractor->GetLineNumberTable(methodId)) {
            if (context.lineNumber == entry.line) {
                context.methodId = methodId;
                context.bytecodeOffset = entry.offset;
                util::UString sourceFilePath(std::string_view(context.extractor->GetSourceFile(methodId)), allocator_);
                context.sourceFilePath = sourceFilePath.View();
                return true;
            }
        }
    }
    LOG(WARNING, ES2PANDA) << "Could not find code at line: " << context.lineNumber << std::endl;
    return false;
}

const ImportExportTable &DebugInfoStorage::LazyLoadImportExportTable(FileDebugInfo *info)
{
    ES2PANDA_ASSERT(info);

    if (info->importExportTable.has_value()) {
        return *info->importExportTable;
    }

    // NOTE: load table after it is implemented in compiler.
    info->importExportTable.emplace(allocator_);
    return info->importExportTable.value();
}

const FileDebugInfo::RecordsMap &DebugInfoStorage::LazyLoadRecords(FileDebugInfo *info)
{
    ES2PANDA_ASSERT(info);

    if (info->records.has_value()) {
        return *info->records;
    }

    info->records.emplace(allocator_->Adapter());
    auto &records = *info->records;

    const auto *pf = info->pf.get();
    for (auto id : pf->GetClasses()) {
        panda_file::File::EntityId classId(id);
        if (pf->IsExternal(classId)) {
            // Сlass that marked in currect .abc file as <external> should be define in some other .abc file.
            // Thus we will not lose information about this class.
            continue;
        }

        std::string fullRecordName = GetFullRecordName(*pf, classId);
        // Be aware of lifecycle of string and string_view
        auto splitedPair = helpers::SplitRecordName(fullRecordName);
        auto recordName = splitedPair.second;
        auto recordNameView = util::UString(recordName, allocator_).View();
        if (!records.emplace(recordNameView, classId).second) {
            LOG(FATAL, ES2PANDA) << "Failed to emplace class '" << recordNameView << "' in records."
                                 << "There should be only one declaration of the same class.";
        }
    }

    return records;
}

FileDebugInfo *DebugInfoStorage::GetDebugInfoByModuleName(std::string_view moduleName) const
{
    auto find = moduleNameToDebugInfo_.find(moduleName);
    if (find != moduleNameToDebugInfo_.end()) {
        return find->second;
    }
    return nullptr;
}

}  // namespace ark::es2panda::evaluate
