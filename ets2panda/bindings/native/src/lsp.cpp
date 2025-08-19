/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "convertors-napi.h"
#include "ir/astNode.h"
#include "lsp/include/api.h"
#include "common.h"
#include "panda_types.h"
#include "public/es2panda_lib.h"
#include "lsp/include/refactors/refactor_types.h"
#include <cstddef>
#include <string>

namespace {
using ark::es2panda::lsp::ClassHierarchy;
using ark::es2panda::lsp::ClassHierarchyInfo;
using ark::es2panda::lsp::ClassHierarchyItem;
using ark::es2panda::lsp::ClassMethodItem;
using ark::es2panda::lsp::ClassPropertyItem;
}  // namespace

char *GetStringCopy(KStringPtr &ptr)
{
    return strdup(ptr.CStr());
}

KNativePointer impl_getCurrentTokenValue(KNativePointer context, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    return new std::string(
        ctx->getCurrentTokenValue(reinterpret_cast<es2panda_Context *>(context), static_cast<std::size_t>(position)));
}
TS_INTEROP_2(getCurrentTokenValue, KNativePointer, KNativePointer, KInt)

// diagnostics related
KNativePointer impl_getSemanticDiagnostics(KNativePointer context)
{
    LSPAPI const *ctx = GetImpl();
    auto *ptrDiag =
        new DiagnosticReferences(ctx->getSemanticDiagnostics(reinterpret_cast<es2panda_Context *>(context)));
    return ptrDiag;
}
TS_INTEROP_1(getSemanticDiagnostics, KNativePointer, KNativePointer)

KNativePointer impl_getClassPropertyInfo(KNativePointer context, KInt position, KBoolean shouldCollectInherited)
{
    LSPAPI const *ctx = GetImpl();
    auto info = ctx->getClassPropertyInfo(reinterpret_cast<es2panda_Context *>(context),
                                          static_cast<std::size_t>(position), shouldCollectInherited != 0);
    return new std::vector<FieldsInfo>(info);
}
TS_INTEROP_3(getClassPropertyInfo, KNativePointer, KNativePointer, KInt, KBoolean)

KNativePointer impl_getRenameLocationFileName(KNativePointer renameLocationPtr)
{
    auto *renameLocationRef = reinterpret_cast<ark::es2panda::lsp::RenameLocation *>(renameLocationPtr);
    return new std::string(renameLocationRef->fileName);
}
TS_INTEROP_1(getRenameLocationFileName, KNativePointer, KNativePointer)

KBoolean impl_renameLocationHasPrefixText(KNativePointer renameLocationPtr)
{
    auto *renameLocationRef = reinterpret_cast<ark::es2panda::lsp::RenameLocation *>(renameLocationPtr);
    return renameLocationRef->prefixText.has_value() ? 1 : 0;
}
TS_INTEROP_1(renameLocationHasPrefixText, KBoolean, KNativePointer)

KNativePointer impl_getRenameLocationPrefixText(KNativePointer renameLocationPtr)
{
    auto *renameLocationRef = reinterpret_cast<ark::es2panda::lsp::RenameLocation *>(renameLocationPtr);
    return new std::string(renameLocationRef->prefixText.has_value() ? renameLocationRef->prefixText.value() : "");
}
TS_INTEROP_1(getRenameLocationPrefixText, KNativePointer, KNativePointer)

KBoolean impl_renameLocationHasSuffixText(KNativePointer renameLocationPtr)
{
    auto *renameLocationRef = reinterpret_cast<ark::es2panda::lsp::RenameLocation *>(renameLocationPtr);
    return renameLocationRef->suffixText.has_value() ? 1 : 0;
}
TS_INTEROP_1(renameLocationHasSuffixText, KBoolean, KNativePointer)

KNativePointer impl_getRenameLocationSuffixText(KNativePointer renameLocationPtr)
{
    auto *renameLocationRef = reinterpret_cast<ark::es2panda::lsp::RenameLocation *>(renameLocationPtr);
    return new std::string(renameLocationRef->suffixText.has_value() ? renameLocationRef->suffixText.value() : "");
}
TS_INTEROP_1(getRenameLocationSuffixText, KNativePointer, KNativePointer)

KInt impl_getRenameLocationStart(KNativePointer renameLocationPtr)
{
    auto *renameLocationRef = reinterpret_cast<ark::es2panda::lsp::RenameLocation *>(renameLocationPtr);
    return renameLocationRef->start;
}
TS_INTEROP_1(getRenameLocationStart, KInt, KNativePointer)

KInt impl_getRenameLocationEnd(KNativePointer renameLocationPtr)
{
    auto *renameLocationRef = reinterpret_cast<ark::es2panda::lsp::RenameLocation *>(renameLocationPtr);
    return renameLocationRef->end;
}
TS_INTEROP_1(getRenameLocationEnd, KInt, KNativePointer)

KInt impl_getRenameLocationLine(KNativePointer renameLocationPtr)
{
    auto *renameLocationRef = reinterpret_cast<ark::es2panda::lsp::RenameLocation *>(renameLocationPtr);
    return renameLocationRef->line;
}
TS_INTEROP_1(getRenameLocationLine, KInt, KNativePointer)

// NOLINTBEGIN
inline KUInt UnpackUInt(const KByte *bytes)
{
    return (bytes[0] | (bytes[1] << 8U) | (bytes[2U] << 16U) | (bytes[3U] << 24U));
}
// NOLINTEND

/*
 * Parses an array of pointers from a KStringArray.
 * format:
 * | header(4 bytes) | strLen(4 bytes) | strData(strLen bytes) | strLen(4 bytes) | strData(strLen bytes) | ...
 */
static std::vector<void *> ParsePointerArray(KInt argc, KStringArray pointerArrayPtr)
{
    const std::size_t headerLen = 4;
    auto bigintPtrs = std::vector<void *>();
    bigintPtrs.reserve(static_cast<std::size_t>(argc));
    std::size_t offset = headerLen;
    std::size_t strLen = 0;

    for (std::size_t i = 0; i < static_cast<std::size_t>(argc); ++i) {
        strLen = UnpackUInt(pointerArrayPtr + offset);
        offset += headerLen;
        std::string bigintStr(reinterpret_cast<const char *>(pointerArrayPtr + offset), strLen);
        offset += strLen;

        uintptr_t ptrValue = 0;
        const size_t prefixLen = 2;
        const size_t hex = 16;
        const size_t decimal = 10;
        if (bigintStr.substr(0, prefixLen) == "0x" || bigintStr.substr(0, prefixLen) == "0X") {
            ptrValue = std::stoull(bigintStr, nullptr, hex);
        } else {
            ptrValue = std::stoull(bigintStr, nullptr, decimal);
        }
        bigintPtrs.push_back(reinterpret_cast<void *>(ptrValue));
    }

    return bigintPtrs;
}

KNativePointer impl_findRenameLocations(KInt argc, KStringArray pointerArrayPtr, KNativePointer context, KInt position)
{
    auto pointerArray = ParsePointerArray(argc, pointerArrayPtr);
    auto fileContexts = std::vector<es2panda_Context *> {};
    fileContexts.reserve(argc);
    for (std::size_t i = 0; i < static_cast<std::size_t>(argc); ++i) {
        auto contextPtr = reinterpret_cast<es2panda_Context *>(pointerArray[i]);
        if (contextPtr != nullptr) {
            fileContexts.push_back(contextPtr);
        }
    }
    LSPAPI const *ctx = GetImpl();
    auto result = ctx->findRenameLocations(fileContexts, reinterpret_cast<es2panda_Context *>(context),
                                           static_cast<std::size_t>(position));
    auto ptrs = std::make_unique<std::vector<void *>>();
    ptrs->reserve(result.size());
    for (auto &el : result) {
        ptrs->push_back(new ark::es2panda::lsp::RenameLocation(std::move(el)));
    }
    return ptrs.release();
}
TS_INTEROP_4(findRenameLocations, KNativePointer, KInt, KStringArray, KNativePointer, KInt)

KNativePointer impl_findRenameLocationsInCurrentFile(KNativePointer context, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    auto result = ctx->findRenameLocationsInCurrentFile(reinterpret_cast<es2panda_Context *>(context),
                                                        static_cast<std::size_t>(position));
    auto ptrs = std::make_unique<std::vector<void *>>();
    ptrs->reserve(result.size());
    for (auto &el : result) {
        ptrs->push_back(new ark::es2panda::lsp::RenameLocation(el));
    }
    return ptrs.release();
}
TS_INTEROP_2(findRenameLocationsInCurrentFile, KNativePointer, KNativePointer, KInt)

KBoolean impl_needsCrossFileRename(KNativePointer context, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    auto res = ctx->needsCrossFileRename(reinterpret_cast<es2panda_Context *>(context), position);
    return res ? 1 : 0;
}
TS_INTEROP_2(needsCrossFileRename, KBoolean, KNativePointer, KInt)

KNativePointer impl_getRenameSuccessFileName(KNativePointer successPtr)
{
    auto successInfo = reinterpret_cast<ark::es2panda::lsp::RenameInfoSuccess *>(successPtr);
    return new std::string(successInfo->GetFileToRename());
}
TS_INTEROP_1(getRenameSuccessFileName, KNativePointer, KNativePointer)

KNativePointer impl_getRenameSuccessKind(KNativePointer successPtr)
{
    auto successInfo = reinterpret_cast<ark::es2panda::lsp::RenameInfoSuccess *>(successPtr);
    return new std::string(successInfo->GetKind());
}
TS_INTEROP_1(getRenameSuccessKind, KNativePointer, KNativePointer)

KNativePointer impl_getRenameSuccessDisplayName(KNativePointer successPtr)
{
    auto successInfo = reinterpret_cast<ark::es2panda::lsp::RenameInfoSuccess *>(successPtr);
    return new std::string(successInfo->GetDisplayName());
}
TS_INTEROP_1(getRenameSuccessDisplayName, KNativePointer, KNativePointer)

KNativePointer impl_getRenameSuccessFullDisplayName(KNativePointer successPtr)
{
    auto successInfo = reinterpret_cast<ark::es2panda::lsp::RenameInfoSuccess *>(successPtr);
    return new std::string(successInfo->GetFullDisplayName());
}
TS_INTEROP_1(getRenameSuccessFullDisplayName, KNativePointer, KNativePointer)

KNativePointer impl_getRenameSuccessKindModifiers(KNativePointer successPtr)
{
    auto successInfo = reinterpret_cast<ark::es2panda::lsp::RenameInfoSuccess *>(successPtr);
    return new std::string(successInfo->GetKindModifiers());
}
TS_INTEROP_1(getRenameSuccessKindModifiers, KNativePointer, KNativePointer)

KNativePointer impl_getRenameSuccessTriggerSpan(KNativePointer successPtr)
{
    auto successInfo = reinterpret_cast<ark::es2panda::lsp::RenameInfoSuccess *>(successPtr);
    return new TextSpan(successInfo->GetTriggerSpan());
}
TS_INTEROP_1(getRenameSuccessTriggerSpan, KNativePointer, KNativePointer)

KNativePointer impl_getRenameFailureLocalizedErrorMessage(KNativePointer failurePtr)
{
    auto failureInfo = reinterpret_cast<ark::es2panda::lsp::RenameInfoFailure *>(failurePtr);
    return new std::string(failureInfo->GetLocalizedErrorMessage());
}
TS_INTEROP_1(getRenameFailureLocalizedErrorMessage, KNativePointer, KNativePointer)

KBoolean impl_getRenameInfoIsSuccess(KNativePointer renameInfoPtr)
{
    auto renameInfo = reinterpret_cast<std::tuple<bool, ark::es2panda::lsp::RenameInfoType *> *>(renameInfoPtr);
    return std::get<0>(*renameInfo) ? 1 : 0;
}
TS_INTEROP_1(getRenameInfoIsSuccess, KBoolean, KNativePointer)

KNativePointer impl_getRenameInfoSuccess(KNativePointer renameInfoPtr)
{
    auto renameInfo = reinterpret_cast<std::tuple<bool, ark::es2panda::lsp::RenameInfoSuccess *> *>(renameInfoPtr);
    auto [flag, successInfo] = *renameInfo;
    return flag ? successInfo : nullptr;
}
TS_INTEROP_1(getRenameInfoSuccess, KNativePointer, KNativePointer)

KNativePointer impl_getRenameInfoFailure(KNativePointer renameInfoPtr)
{
    auto renameInfo = reinterpret_cast<std::tuple<bool, ark::es2panda::lsp::RenameInfoFailure *> *>(renameInfoPtr);
    auto [flag, failureInfo] = *renameInfo;
    return flag ? nullptr : failureInfo;
}
TS_INTEROP_1(getRenameInfoFailure, KNativePointer, KNativePointer)

KNativePointer impl_getRenameInfo(KNativePointer context, KInt position, KStringPtr &pandaLibPath)
{
    LSPAPI const *ctx = GetImpl();
    auto result = ctx->getRenameInfo(reinterpret_cast<es2panda_Context *>(context), static_cast<std::size_t>(position),
                                     GetStringCopy(pandaLibPath));
    if (std::holds_alternative<ark::es2panda::lsp::RenameInfoSuccess>(result)) {
        auto &successInfo = std::get<ark::es2panda::lsp::RenameInfoSuccess>(result);
        return new std::tuple(true, new ark::es2panda::lsp::RenameInfoSuccess(std::move(successInfo)));
    }
    auto &failureInfo = std::get<ark::es2panda::lsp::RenameInfoFailure>(result);
    return new std::tuple(false, new ark::es2panda::lsp::RenameInfoFailure(std::move(failureInfo)));
}
TS_INTEROP_3(getRenameInfo, KNativePointer, KNativePointer, KInt, KStringPtr)

KNativePointer impl_getFieldsInfoFromPropertyInfo(KNativePointer infoPtr)
{
    auto info = reinterpret_cast<std::vector<FieldsInfo> *>(infoPtr);
    std::vector<void *> ptrs;
    for (auto &el : *info) {
        ptrs.push_back(new FieldsInfo(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getFieldsInfoFromPropertyInfo, KNativePointer, KNativePointer)

KNativePointer impl_getNameFromPropertyInfo(KNativePointer infoPtr)
{
    auto info = reinterpret_cast<FieldsInfo *>(infoPtr);
    return new std::string(info->name);
}
TS_INTEROP_1(getNameFromPropertyInfo, KNativePointer, KNativePointer)

KNativePointer impl_getFieldListPropertyFromPropertyInfo(KNativePointer infoPtr)
{
    auto info = reinterpret_cast<FieldsInfo *>(infoPtr);
    std::vector<void *> ptrs;
    for (auto &el : info->properties) {
        ptrs.push_back(new FieldListProperty(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getFieldListPropertyFromPropertyInfo, KNativePointer, KNativePointer)

KNativePointer impl_getKindFromPropertyInfo(KNativePointer infoPtr)
{
    auto info = reinterpret_cast<FieldListProperty *>(infoPtr);
    return new std::string(info->kind);
}
TS_INTEROP_1(getKindFromPropertyInfo, KNativePointer, KNativePointer)

KNativePointer impl_getModifierKindsFromPropertyInfo(KNativePointer infoPtr)
{
    auto info = reinterpret_cast<FieldListProperty *>(infoPtr);
    std::vector<void *> ptrs;
    for (auto &el : info->modifierKinds.value()) {
        ptrs.push_back(new std::string(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getModifierKindsFromPropertyInfo, KNativePointer, KNativePointer)

KNativePointer impl_getDisplayNameFromPropertyInfo(KNativePointer infoPtr)
{
    auto info = reinterpret_cast<FieldListProperty *>(infoPtr);
    return new std::string(info->displayName);
}
TS_INTEROP_1(getDisplayNameFromPropertyInfo, KNativePointer, KNativePointer)

KInt impl_getStartFromPropertyInfo(KNativePointer infoPtr)
{
    auto info = reinterpret_cast<FieldListProperty *>(infoPtr);
    return info->start;
}
TS_INTEROP_1(getStartFromPropertyInfo, KInt, KNativePointer)

KInt impl_getEndFromPropertyInfo(KNativePointer infoPtr)
{
    auto info = reinterpret_cast<FieldListProperty *>(infoPtr);
    return info->end;
}
TS_INTEROP_1(getEndFromPropertyInfo, KInt, KNativePointer)

KNativePointer impl_getSyntacticDiagnostics(KNativePointer context)
{
    LSPAPI const *ctx = GetImpl();
    auto *ptrDiag =
        new DiagnosticReferences(ctx->getSyntacticDiagnostics(reinterpret_cast<es2panda_Context *>(context)));
    return ptrDiag;
}
TS_INTEROP_1(getSyntacticDiagnostics, KNativePointer, KNativePointer)

KNativePointer impl_getDiags(KNativePointer diagRefsPtr)
{
    auto *diagRefs = reinterpret_cast<DiagnosticReferences *>(diagRefsPtr);
    std::vector<void *> ptrs;
    for (auto &el : diagRefs->diagnostic) {
        ptrs.push_back(new Diagnostic(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getDiags, KNativePointer, KNativePointer)

KNativePointer impl_getDiagMsg(KNativePointer diagRefPtr)
{
    auto *diagRef = reinterpret_cast<Diagnostic *>(diagRefPtr);
    return new std::string(diagRef->message_);
}
TS_INTEROP_1(getDiagMsg, KNativePointer, KNativePointer)

KNativePointer impl_getDiagRange(KNativePointer diagRefPtr)
{
    auto *diagRef = reinterpret_cast<Diagnostic *>(diagRefPtr);
    return &diagRef->range_;
}
TS_INTEROP_1(getDiagRange, KNativePointer, KNativePointer)

KNativePointer impl_getRangeEnd(KNativePointer rangePtr)
{
    auto *range = reinterpret_cast<Range *>(rangePtr);
    return &range->end;
}
TS_INTEROP_1(getRangeEnd, KNativePointer, KNativePointer)

KNativePointer impl_getRangeStart(KNativePointer rangePtr)
{
    auto *range = reinterpret_cast<Range *>(rangePtr);
    return &range->start;
}
TS_INTEROP_1(getRangeStart, KNativePointer, KNativePointer)

KInt impl_getPosLine(KNativePointer posPtr)
{
    auto *pos = reinterpret_cast<Position *>(posPtr);
    return pos->line_;
}
TS_INTEROP_1(getPosLine, KInt, KNativePointer)

KInt impl_getPosChar(KNativePointer posPtr)
{
    auto *pos = reinterpret_cast<Position *>(posPtr);
    return pos->character_;
}
TS_INTEROP_1(getPosChar, KInt, KNativePointer)

KInt impl_getDiagSeverity(KNativePointer diagRefPtr)
{
    auto *diagRef = reinterpret_cast<Diagnostic *>(diagRefPtr);
    return static_cast<size_t>(diagRef->severity_);
}
TS_INTEROP_1(getDiagSeverity, KInt, KNativePointer)

KNativePointer impl_getDiagCode(KNativePointer diagRefPtr)
{
    auto *diagRef = reinterpret_cast<Diagnostic *>(diagRefPtr);
    return &diagRef->code_;
}
TS_INTEROP_1(getDiagCode, KNativePointer, KNativePointer)

KNativePointer impl_getDiagCodeDescription(KNativePointer diagRefPtr)
{
    auto *diagRef = reinterpret_cast<Diagnostic *>(diagRefPtr);
    return &diagRef->codeDescription_;
}
TS_INTEROP_1(getDiagCodeDescription, KNativePointer, KNativePointer)

KNativePointer impl_getCodeDescriptionHref(KNativePointer codeDescrRefPtr)
{
    auto *codeDescrRef = reinterpret_cast<CodeDescription *>(codeDescrRefPtr);
    return new std::string(codeDescrRef->href_);
}
TS_INTEROP_1(getCodeDescriptionHref, KNativePointer, KNativePointer)

KNativePointer impl_getDiagTags(KNativePointer diagRefPtr)
{
    auto *diagRef = reinterpret_cast<Diagnostic *>(diagRefPtr);
    std::vector<void *> ptrs;
    for (auto el : diagRef->tags_) {
        auto castedEl = static_cast<size_t>(el);
        ptrs.push_back(&castedEl);
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getDiagTags, KNativePointer, KNativePointer)

KNativePointer impl_getDiagData(KNativePointer diagRefPtr)
{
    auto *diagRef = reinterpret_cast<Diagnostic *>(diagRefPtr);
    return &diagRef->data_;
}
TS_INTEROP_1(getDiagData, KNativePointer, KNativePointer)

KNativePointer impl_getDiagRelatedInfo(KNativePointer diagRefPtr)
{
    auto *diagRef = reinterpret_cast<Diagnostic *>(diagRefPtr);
    std::vector<void *> ptrs;
    for (auto el : diagRef->relatedInformation_) {
        ptrs.push_back(&el);
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getDiagRelatedInfo, KNativePointer, KNativePointer)

KNativePointer impl_getRelatedInfoMsg(KNativePointer relatedInfoPtr)
{
    auto *relatedInfoRef = reinterpret_cast<DiagnosticRelatedInformation *>(relatedInfoPtr);
    return new std::string(relatedInfoRef->message_);
}
TS_INTEROP_1(getRelatedInfoMsg, KNativePointer, KNativePointer)

KNativePointer impl_getRelatedInfoLoc(KNativePointer relatedInfoPtr)
{
    auto *relatedInfoRef = reinterpret_cast<DiagnosticRelatedInformation *>(relatedInfoPtr);
    return &relatedInfoRef->location_;
}
TS_INTEROP_1(getRelatedInfoLoc, KNativePointer, KNativePointer)

KNativePointer impl_getLocUri(KNativePointer locPtr)
{
    auto *locRef = reinterpret_cast<Location *>(locPtr);
    return new std::string(locRef->uri_);
}
TS_INTEROP_1(getLocUri, KNativePointer, KNativePointer)

KNativePointer impl_getLocRange(KNativePointer locPtr)
{
    auto *locRef = reinterpret_cast<Location *>(locPtr);
    return &locRef->range_;
}
TS_INTEROP_1(getLocRange, KNativePointer, KNativePointer)

KNativePointer impl_getDiagSource(KNativePointer diagRefPtr)
{
    auto *diagRef = reinterpret_cast<Diagnostic *>(diagRefPtr);
    return new std::string(diagRef->source_);
}
TS_INTEROP_1(getDiagSource, KNativePointer, KNativePointer)

KBoolean impl_isPackageModule(KNativePointer context)
{
    LSPAPI const *ctx = GetImpl();
    return static_cast<KBoolean>(ctx->isPackageModule(reinterpret_cast<es2panda_Context *>(context)));
}
TS_INTEROP_1(isPackageModule, KBoolean, KNativePointer)

KNativePointer impl_getFileReferences(KStringPtr &filenamePtr, KNativePointer context, KBoolean isPackageModule)
{
    LSPAPI const *ctx = GetImpl();
    auto *ref = new References(ctx->getFileReferences(
        GetStringCopy(filenamePtr), reinterpret_cast<es2panda_Context *>(context), isPackageModule != 0));
    return ref;
}
TS_INTEROP_3(getFileReferences, KNativePointer, KStringPtr, KNativePointer, KBoolean)

KNativePointer impl_getDeclInfo(KNativePointer context, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    auto *declInfo = new DeclInfo(
        ctx->getDeclInfo(reinterpret_cast<es2panda_Context *>(context), static_cast<std::size_t>(position)));
    return declInfo;
}
TS_INTEROP_2(getDeclInfo, KNativePointer, KNativePointer, KInt)

KNativePointer impl_getClassConstructorInfo(KNativePointer context, KInt position, KStringArray strArrayPtr)
{
    std::vector<std::string> properties;
    for (const auto &el : MakeStringVector(strArrayPtr)) {
        properties.emplace_back(GetStringCopy(const_cast<KStringPtr &>(el)));
    }
    LSPAPI const *ctx = GetImpl();
    auto *info = new ark::es2panda::lsp::RefactorEditInfo(ctx->getClassConstructorInfo(
        reinterpret_cast<es2panda_Context *>(context), static_cast<std::size_t>(position), properties));
    return info;
}
TS_INTEROP_3(getClassConstructorInfo, KNativePointer, KNativePointer, KInt, KStringArray)

KNativePointer impl_getFileTextChangesFromConstructorInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<ark::es2panda::lsp::RefactorEditInfo *>(infoPtr);
    std::vector<void *> ptrs;
    for (auto &el : info->GetFileTextChanges()) {
        ptrs.push_back(new FileTextChanges(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getFileTextChangesFromConstructorInfo, KNativePointer, KNativePointer)

KNativePointer impl_getFileNameFromConstructorInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<FileTextChanges *>(infoPtr);
    return new std::string(info->fileName);
}
TS_INTEROP_1(getFileNameFromConstructorInfo, KNativePointer, KNativePointer)

KNativePointer impl_getTextChangeFromConstructorInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<FileTextChanges *>(infoPtr);
    std::vector<void *> ptrs;
    for (auto &el : info->textChanges) {
        ptrs.push_back(new TextChange(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getTextChangeFromConstructorInfo, KNativePointer, KNativePointer)

KNativePointer impl_getNewTextFromConstructorInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<TextChange *>(infoPtr);
    return new std::string(info->newText);
}
TS_INTEROP_1(getNewTextFromConstructorInfo, KNativePointer, KNativePointer)

KNativePointer impl_getTextSpanFromConstructorInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<TextChange *>(infoPtr);
    return new TextSpan(info->span);
}
TS_INTEROP_1(getTextSpanFromConstructorInfo, KNativePointer, KNativePointer)

KNativePointer impl_getCompletionEntryDetailsSymbolDisplayPart(KNativePointer completionEntryDetailsPtr)
{
    auto *completionEntryDetails = reinterpret_cast<CompletionEntryDetails *>(completionEntryDetailsPtr);
    std::vector<void *> ptrs;
    for (auto &el : completionEntryDetails->GetDisplayParts()) {
        ptrs.push_back(new SymbolDisplayPart(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getCompletionEntryDetailsSymbolDisplayPart, KNativePointer, KNativePointer)

KNativePointer impl_getCompletionEntryDetailsKind(KNativePointer completionEntryDetailsPtr)
{
    auto *completionEntryDetails = reinterpret_cast<CompletionEntryDetails *>(completionEntryDetailsPtr);
    return new std::string(completionEntryDetails->GetKind());
}
TS_INTEROP_1(getCompletionEntryDetailsKind, KNativePointer, KNativePointer)

KNativePointer impl_getCompletionEntryDetailsKindModifier(KNativePointer ref)
{
    auto *refPtr = reinterpret_cast<CompletionEntryDetails *>(ref);
    return new std::string(refPtr->GetKindModifiers());
}
TS_INTEROP_1(getCompletionEntryDetailsKindModifier, KNativePointer, KNativePointer)

KNativePointer impl_getCompletionEntryDetailsFileName(KNativePointer ref)
{
    auto *refPtr = reinterpret_cast<CompletionEntryDetails *>(ref);
    return new std::string(refPtr->GetFileName());
}
TS_INTEROP_1(getCompletionEntryDetailsFileName, KNativePointer, KNativePointer)

KNativePointer impl_getCompletionEntryDetailsEntryName(KNativePointer ref)
{
    auto *refPtr = reinterpret_cast<CompletionEntryDetails *>(ref);
    return new std::string(refPtr->GetName());
}
TS_INTEROP_1(getCompletionEntryDetailsEntryName, KNativePointer, KNativePointer)

KNativePointer impl_findSafeDeleteLocation(KNativePointer context, KNativePointer declInfo)
{
    LSPAPI const *ctx = GetImpl();
    auto *result = new std::vector<SafeDeleteLocation>(
        ctx->FindSafeDeleteLocation(reinterpret_cast<es2panda_Context *>(context),
                                    reinterpret_cast<std::tuple<std::string, std::string> *>(declInfo)));
    return result;
}
TS_INTEROP_2(findSafeDeleteLocation, KNativePointer, KNativePointer, KNativePointer)

KNativePointer impl_getSafeDeleteLocations(KNativePointer safeDeleteLocationsPtr)
{
    auto *locations = reinterpret_cast<std::vector<SafeDeleteLocation> *>(safeDeleteLocationsPtr);
    std::vector<void *> ptrs;
    for (auto &loc : *locations) {
        ptrs.push_back(new SafeDeleteLocation(loc));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getSafeDeleteLocations, KNativePointer, KNativePointer)

KNativePointer impl_getSafeDeleteLocationUri(KNativePointer locationPtr)
{
    auto *location = reinterpret_cast<SafeDeleteLocation *>(locationPtr);
    return new std::string(location->uri);
}
TS_INTEROP_1(getSafeDeleteLocationUri, KNativePointer, KNativePointer)

KInt impl_getSafeDeleteLocationStart(KNativePointer locationPtr)
{
    auto *location = reinterpret_cast<SafeDeleteLocation *>(locationPtr);
    return static_cast<KInt>(location->start);
}
TS_INTEROP_1(getSafeDeleteLocationStart, KInt, KNativePointer)

KInt impl_getSafeDeleteLocationLength(KNativePointer locationPtr)
{
    auto *location = reinterpret_cast<SafeDeleteLocation *>(locationPtr);
    return static_cast<KInt>(location->length);
}
TS_INTEROP_1(getSafeDeleteLocationLength, KInt, KNativePointer)

KNativePointer impl_getReferencesAtPosition(KNativePointer context, KNativePointer declInfo)
{
    LSPAPI const *ctx = GetImpl();
    auto *ref = new References(ctx->getReferencesAtPosition(reinterpret_cast<es2panda_Context *>(context),
                                                            reinterpret_cast<DeclInfo *>(declInfo)));
    return ref;
}
TS_INTEROP_2(getReferencesAtPosition, KNativePointer, KNativePointer, KNativePointer)

KNativePointer impl_getReferenceInfos(KNativePointer refs)
{
    auto *refsPtr = reinterpret_cast<References *>(refs);
    std::vector<void *> ptrs;
    for (auto &el : refsPtr->referenceInfos) {
        ptrs.push_back(new ReferenceInfo(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getReferenceInfos, KNativePointer, KNativePointer)

KInt impl_getReferenceStart(KNativePointer ref)
{
    auto *refPtr = reinterpret_cast<ReferenceInfo *>(ref);
    return refPtr->start;
}
TS_INTEROP_1(getReferenceStart, KInt, KNativePointer)

KInt impl_getReferenceLength(KNativePointer ref)
{
    auto *refPtr = reinterpret_cast<ReferenceInfo *>(ref);
    return refPtr->length;
}
TS_INTEROP_1(getReferenceLength, KInt, KNativePointer)

KNativePointer impl_getReferenceFileName(KNativePointer ref)
{
    auto *refPtr = reinterpret_cast<ReferenceInfo *>(ref);
    return new std::string(refPtr->fileName);
}
TS_INTEROP_1(getReferenceFileName, KNativePointer, KNativePointer)

KNativePointer impl_getDeclInfoFileName(KNativePointer declInfo)
{
    auto *declInfoPtr = reinterpret_cast<DeclInfo *>(declInfo);
    return new std::string(declInfoPtr->fileName);
}
TS_INTEROP_1(getDeclInfoFileName, KNativePointer, KNativePointer)

KNativePointer impl_getDeclInfoFileText(KNativePointer declInfo)
{
    auto *declInfoPtr = reinterpret_cast<DeclInfo *>(declInfo);
    return new std::string(declInfoPtr->fileText);
}
TS_INTEROP_1(getDeclInfoFileText, KNativePointer, KNativePointer)

KNativePointer impl_getQuickInfoAtPosition(KStringPtr &filenamePtr, KNativePointer context, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    auto *qi = new QuickInfo(ctx->getQuickInfoAtPosition(GetStringCopy(filenamePtr),
                                                         reinterpret_cast<es2panda_Context *>(context), position));
    return qi;
}
TS_INTEROP_3(getQuickInfoAtPosition, KNativePointer, KStringPtr, KNativePointer, KInt)

KNativePointer impl_getCompletionAtPosition(KNativePointer context, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    auto *ci = new ark::es2panda::lsp::CompletionInfo(
        ctx->getCompletionsAtPosition(reinterpret_cast<es2panda_Context *>(context), position));
    return ci;
}
TS_INTEROP_2(getCompletionAtPosition, KNativePointer, KNativePointer, KInt)

KNativePointer impl_organizeImports(KNativePointer context, KStringPtr &filenamePtr)
{
    LSPAPI const *ctx = GetImpl();
    auto result = ctx->OrganizeImportsImpl(reinterpret_cast<es2panda_Context *>(context), GetStringCopy(filenamePtr));
    return new std::vector<FileTextChanges>(result);
}
TS_INTEROP_2(organizeImports, KNativePointer, KNativePointer, KStringPtr)

KNativePointer impl_getFileTextChanges(KNativePointer fileTextChangesVecPtr)
{
    auto *vec = reinterpret_cast<std::vector<FileTextChanges> *>(fileTextChangesVecPtr);
    std::vector<void *> ptrs;
    for (auto &el : *vec) {
        ptrs.push_back(new FileTextChanges(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getFileTextChanges, KNativePointer, KNativePointer)

KNativePointer impl_getFileNameFromFileTextChanges(KNativePointer fileTextChangesPtr)
{
    auto *ftc = reinterpret_cast<FileTextChanges *>(fileTextChangesPtr);
    return new std::string(ftc->fileName);
}
TS_INTEROP_1(getFileNameFromFileTextChanges, KNativePointer, KNativePointer)

KNativePointer impl_getTextChangesFromFileTextChanges(KNativePointer fileTextChangesPtr)
{
    auto *ftc = reinterpret_cast<FileTextChanges *>(fileTextChangesPtr);
    std::vector<void *> ptrs;
    for (auto &el : ftc->textChanges) {
        ptrs.push_back(new TextChange(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getTextChangesFromFileTextChanges, KNativePointer, KNativePointer)

KNativePointer impl_getTextSpanFromTextChange(KNativePointer textChangePtr)
{
    auto *tc = reinterpret_cast<TextChange *>(textChangePtr);
    return new TextSpan(tc->span);
}
TS_INTEROP_1(getTextSpanFromTextChange, KNativePointer, KNativePointer)

KNativePointer impl_getNewTextFromTextChange(KNativePointer textChangePtr)
{
    auto *tc = reinterpret_cast<TextChange *>(textChangePtr);
    return new std::string(tc->newText);
}
TS_INTEROP_1(getNewTextFromTextChange, KNativePointer, KNativePointer)

KNativePointer impl_getCompletionEntryDetails(KStringPtr &entrynamePtr, KStringPtr &filenamePtr, KNativePointer context,
                                              KInt position)
{
    LSPAPI const *ctx = GetImpl();
    auto *ci = new CompletionEntryDetails(
        ctx->getCompletionEntryDetails(GetStringCopy(entrynamePtr), GetStringCopy(filenamePtr),
                                       reinterpret_cast<es2panda_Context *>(context), position));
    return ci;
}
TS_INTEROP_4(getCompletionEntryDetails, KNativePointer, KStringPtr, KStringPtr, KNativePointer, KInt)

KNativePointer impl_getImplementationAtPosition(KNativePointer context, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    auto *defInfo = new DefinitionInfo(ctx->getImplementationAtPosition(reinterpret_cast<es2panda_Context *>(context),
                                                                        static_cast<std::size_t>(position)));
    return defInfo;
}
TS_INTEROP_2(getImplementationAtPosition, KNativePointer, KNativePointer, KInt)

KNativePointer impl_getDefinitionAtPosition(KNativePointer context, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    auto *defInfo = new DefinitionInfo(ctx->getDefinitionAtPosition(reinterpret_cast<es2panda_Context *>(context),
                                                                    static_cast<std::size_t>(position)));
    return defInfo;
}
TS_INTEROP_2(getDefinitionAtPosition, KNativePointer, KNativePointer, KInt)

KNativePointer impl_GetFileNameFromDef(KNativePointer defPtr)
{
    auto *defInfo = reinterpret_cast<DefinitionInfo *>(defPtr);
    return new std::string(defInfo->fileName);
}
TS_INTEROP_1(GetFileNameFromDef, KNativePointer, KNativePointer)

KInt impl_GetStartFromDef(KNativePointer defPtr)
{
    auto *defInfo = reinterpret_cast<DefinitionInfo *>(defPtr);
    return defInfo->start;
}
TS_INTEROP_1(GetStartFromDef, KInt, KNativePointer)

KInt impl_getLengthFromDef(KNativePointer defPtr)
{
    auto *defInfo = reinterpret_cast<DefinitionInfo *>(defPtr);
    return defInfo->length;
}
TS_INTEROP_1(getLengthFromDef, KInt, KNativePointer)

KNativePointer impl_getDocumentHighlights(KNativePointer context, KInt pos)
{
    LSPAPI const *ctx = GetImpl();
    auto *docs = new DocumentHighlightsReferences(
        ctx->getDocumentHighlights(reinterpret_cast<es2panda_Context *>(context), pos));
    return docs;
}
TS_INTEROP_2(getDocumentHighlights, KNativePointer, KNativePointer, KInt)

KNativePointer impl_getDocumentHighs(KNativePointer doc)
{
    auto *dhr = reinterpret_cast<DocumentHighlightsReferences *>(doc);
    std::vector<void *> ptrs;
    for (auto &el : dhr->documentHighlights_) {
        ptrs.push_back(new DocumentHighlights(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getDocumentHighs, KNativePointer, KNativePointer)

KNativePointer impl_getSuggestionDiagnostics(KNativePointer context)
{
    LSPAPI const *ctx = GetImpl();
    auto *ptrDiag =
        new DiagnosticReferences(ctx->getSuggestionDiagnostics(reinterpret_cast<es2panda_Context *>(context)));
    return ptrDiag;
}
TS_INTEROP_1(getSuggestionDiagnostics, KNativePointer, KNativePointer)

KNativePointer impl_getDisplayPartsText(KNativePointer ref)
{
    auto *refPtr = reinterpret_cast<SymbolDisplayPart *>(ref);
    return new std::string(refPtr->GetText());
}
TS_INTEROP_1(getDisplayPartsText, KNativePointer, KNativePointer)

KNativePointer impl_getDisplayPartsKind(KNativePointer ref)
{
    auto *refPtr = reinterpret_cast<SymbolDisplayPart *>(ref);
    return new std::string(refPtr->GetKind());
}
TS_INTEROP_1(getDisplayPartsKind, KNativePointer, KNativePointer)

KNativePointer impl_getQuickInfoKind(KNativePointer quickInfoPtr)
{
    auto *quickInfo = reinterpret_cast<QuickInfo *>(quickInfoPtr);
    return new std::string(quickInfo->GetKind());
}
TS_INTEROP_1(getQuickInfoKind, KNativePointer, KNativePointer)

KNativePointer impl_getQuickInfoKindModifier(KNativePointer ref)
{
    auto *refPtr = reinterpret_cast<QuickInfo *>(ref);
    return new std::string(refPtr->GetKindModifiers());
}
TS_INTEROP_1(getQuickInfoKindModifier, KNativePointer, KNativePointer)

KNativePointer impl_getQuickInfoFileName(KNativePointer ref)
{
    auto *refPtr = reinterpret_cast<QuickInfo *>(ref);
    return new std::string(refPtr->GetFileName());
}
TS_INTEROP_1(getQuickInfoFileName, KNativePointer, KNativePointer)

KNativePointer impl_getClassHierarchyInfo(KNativePointer context, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    if (ctx == nullptr) {
        return nullptr;
    }
    auto *classHierarchyPtr =
        new ClassHierarchy(ctx->getClassHierarchyInfo(reinterpret_cast<es2panda_Context *>(context), position));
    return classHierarchyPtr;
}
TS_INTEROP_2(getClassHierarchyInfo, KNativePointer, KNativePointer, KInt)

KNativePointer impl_castToClassHierarchyInfos(KNativePointer infos)
{
    auto *infosPtr = reinterpret_cast<ClassHierarchy *>(infos);
    if (infosPtr == nullptr) {
        return nullptr;
    }
    std::vector<void *> ptrs;
    for (const auto &element : *infosPtr) {
        ptrs.push_back(new ClassHierarchyInfo(element));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(castToClassHierarchyInfos, KNativePointer, KNativePointer)

KNativePointer impl_getClassNameFromClassHierarchyInfo(KNativePointer info)
{
    auto *infoPtr = reinterpret_cast<ClassHierarchyInfo *>(info);
    if (infoPtr == nullptr) {
        return nullptr;
    }
    return new std::string(infoPtr->GetClassName());
}
TS_INTEROP_1(getClassNameFromClassHierarchyInfo, KNativePointer, KNativePointer)

KNativePointer impl_getMethodItemsFromClassHierarchyInfo(KNativePointer info)
{
    auto *infoPtr = reinterpret_cast<ClassHierarchyInfo *>(info);
    if (infoPtr == nullptr) {
        return nullptr;
    }
    std::vector<void *> ptrs;
    for (const auto &element : infoPtr->GetMethodItemList()) {
        if (element.second == nullptr) {
            continue;
        }
        ptrs.push_back(new ClassMethodItem(*(element.second)));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getMethodItemsFromClassHierarchyInfo, KNativePointer, KNativePointer)

KNativePointer impl_getPropertyItemsFromClassHierarchyInfo(KNativePointer info)
{
    auto *infoPtr = reinterpret_cast<ClassHierarchyInfo *>(info);
    if (infoPtr == nullptr) {
        return nullptr;
    }
    std::vector<void *> ptrs;
    for (const auto &element : infoPtr->GetPropertyItemList()) {
        if (element.second == nullptr) {
            continue;
        }
        ptrs.push_back(new ClassPropertyItem(*(element.second)));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getPropertyItemsFromClassHierarchyInfo, KNativePointer, KNativePointer)

KNativePointer impl_getDetailFromClassHierarchyItem(KNativePointer item)
{
    auto *itemPtr = reinterpret_cast<ClassHierarchyItem *>(item);
    if (itemPtr == nullptr) {
        return nullptr;
    }
    return new std::string(itemPtr->GetDetail());
}
TS_INTEROP_1(getDetailFromClassHierarchyItem, KNativePointer, KNativePointer)

KInt impl_getSetterStyleFromClassMethodItem(KNativePointer item)
{
    auto *itemPtr = reinterpret_cast<ClassMethodItem *>(item);
    if (itemPtr == nullptr) {
        return 0;
    }
    return static_cast<size_t>(itemPtr->GetSetterStyle());
}
TS_INTEROP_1(getSetterStyleFromClassMethodItem, KInt, KNativePointer)

KInt impl_getAccessModifierStyleFromClassHierarchyItem(KNativePointer item)
{
    auto *itemPtr = reinterpret_cast<ClassHierarchyItem *>(item);
    if (itemPtr == nullptr) {
        return 0;
    }
    return static_cast<size_t>(itemPtr->GetAccessModifierStyle());
}
TS_INTEROP_1(getAccessModifierStyleFromClassHierarchyItem, KInt, KNativePointer)

KInt impl_getAliasScriptElementKind(KNativePointer context, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    if (ctx == nullptr) {
        return 1;
    }
    auto kind =
        static_cast<KInt>(ctx->getAliasScriptElementKind(reinterpret_cast<es2panda_Context *>(context), position));
    return kind;
}
TS_INTEROP_2(getAliasScriptElementKind, KInt, KNativePointer, KInt)

KNativePointer impl_pushBackToNativeContextVector(KNativePointer context, KNativePointer contextList, KBoolean isNew)
{
    auto contextPtr = reinterpret_cast<es2panda_Context *>(context);
    if (isNew != 0) {
        auto *newVector = new std::vector<es2panda_Context *>();
        newVector->push_back(contextPtr);
        return newVector;
    }
    auto contextVector = reinterpret_cast<std::vector<es2panda_Context *> *>(contextList);
    contextVector->push_back(contextPtr);
    return contextVector;
}
TS_INTEROP_3(pushBackToNativeContextVector, KNativePointer, KNativePointer, KNativePointer, KBoolean)

KNativePointer impl_getClassHierarchies(KNativePointer context, KStringPtr &fileNamePtr, KInt pos)
{
    LSPAPI const *ctx = GetImpl();
    if (ctx == nullptr) {
        return nullptr;
    }
    auto *contextlist = reinterpret_cast<std::vector<es2panda_Context *> *>(context);
    auto infos = ctx->getClassHierarchiesImpl(contextlist, GetStringCopy(fileNamePtr), pos);
    std::vector<void *> ptrs;
    ptrs.reserve(infos.size());
    for (auto &info : infos) {
        ptrs.push_back(new ark::es2panda::lsp::ClassHierarchyItemInfo(info));
    }
    return new std::vector<void *>(std::move(ptrs));
}
TS_INTEROP_3(getClassHierarchies, KNativePointer, KNativePointer, KStringPtr, KInt)

KNativePointer impl_getApplicableRefactors(KNativePointer context, KStringPtr &kindPtr, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    auto *result = new std::vector<ark::es2panda::lsp::ApplicableRefactorInfo>(ctx->getApplicableRefactors(
        reinterpret_cast<es2panda_Context *>(context), GetStringCopy(kindPtr), static_cast<std::size_t>(position)));
    return result;
}
TS_INTEROP_3(getApplicableRefactors, KNativePointer, KNativePointer, KStringPtr, KInt)

KNativePointer impl_getApplicableRefactorInfoList(KNativePointer infosPtr)
{
    auto *infos = reinterpret_cast<std::vector<ark::es2panda::lsp::ApplicableRefactorInfo> *>(infosPtr);
    std::vector<void *> ptrs;
    for (auto &info : *infos) {
        ptrs.push_back(new ark::es2panda::lsp::ApplicableRefactorInfo(info));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getApplicableRefactorInfoList, KNativePointer, KNativePointer)

KNativePointer impl_getRefactorActionName(KNativePointer refactorActionPtr)
{
    auto *refactorAction = reinterpret_cast<ark::es2panda::lsp::RefactorAction *>(refactorActionPtr);
    return new std::string(refactorAction->name);
}
TS_INTEROP_1(getRefactorActionName, KNativePointer, KNativePointer)

KNativePointer impl_getRefactorActionDescription(KNativePointer refactorActionPtr)
{
    auto *refactorAction = reinterpret_cast<ark::es2panda::lsp::RefactorAction *>(refactorActionPtr);
    return new std::string(refactorAction->description);
}
TS_INTEROP_1(getRefactorActionDescription, KNativePointer, KNativePointer)

KNativePointer impl_getRefactorActionKind(KNativePointer refactorActionPtr)
{
    auto *refactorAction = reinterpret_cast<ark::es2panda::lsp::RefactorAction *>(refactorActionPtr);
    return new std::string(refactorAction->kind);
}
TS_INTEROP_1(getRefactorActionKind, KNativePointer, KNativePointer)

KNativePointer impl_getApplicableRefactorName(KNativePointer applRefsPtr)
{
    auto *applRefsInfo = reinterpret_cast<ark::es2panda::lsp::ApplicableRefactorInfo *>(applRefsPtr);
    return new std::string(applRefsInfo->name);
}
TS_INTEROP_1(getApplicableRefactorName, KNativePointer, KNativePointer)

KNativePointer impl_getApplicableRefactorDescription(KNativePointer applRefsPtr)
{
    auto *applRefsInfo = reinterpret_cast<ark::es2panda::lsp::ApplicableRefactorInfo *>(applRefsPtr);
    return new std::string(applRefsInfo->description);
}
TS_INTEROP_1(getApplicableRefactorDescription, KNativePointer, KNativePointer)

KNativePointer impl_getApplicableRefactorAction(KNativePointer applRefsPtr)
{
    auto *applRefsInfo = reinterpret_cast<ark::es2panda::lsp::ApplicableRefactorInfo *>(applRefsPtr);
    return new ark::es2panda::lsp::RefactorAction(applRefsInfo->action);
}
TS_INTEROP_1(getApplicableRefactorAction, KNativePointer, KNativePointer)

KNativePointer impl_getClassHierarchyList(KNativePointer infosPtr)
{
    auto *infos = reinterpret_cast<std::vector<ark::es2panda::lsp::ClassHierarchyItemInfo *> *>(infosPtr);
    std::vector<void *> infoPtrList;
    for (auto &info : *infos) {
        infoPtrList.push_back(info);
    }
    return new std::vector<void *>(infoPtrList);
}
TS_INTEROP_1(getClassHierarchyList, KNativePointer, KNativePointer)

KInt impl_getPosFromClassHierarchyItemInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<ark::es2panda::lsp::ClassHierarchyItemInfo *>(infoPtr);
    return static_cast<KInt>(info->pos);
}
TS_INTEROP_1(getPosFromClassHierarchyItemInfo, KInt, KNativePointer)

KInt impl_getKindFromClassHierarchyItemInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<ark::es2panda::lsp::ClassHierarchyItemInfo *>(infoPtr);
    return static_cast<KInt>(info->kind);
}
TS_INTEROP_1(getKindFromClassHierarchyItemInfo, KInt, KNativePointer)

KNativePointer impl_getDescriptionFromClassHierarchyItemInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<ark::es2panda::lsp::ClassHierarchyItemInfo *>(infoPtr);
    auto description = info->description;
    return new std::string(description);
}
TS_INTEROP_1(getDescriptionFromClassHierarchyItemInfo, KNativePointer, KNativePointer)

KNativePointer impl_getOverriddenFromClassHierarchyItemInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<ark::es2panda::lsp::ClassHierarchyItemInfo *>(infoPtr);
    auto &overridden = info->overridden;
    std::vector<void *> overriddenPtrList;
    overriddenPtrList.reserve(overridden.size());
    for (auto &details : overridden) {
        overriddenPtrList.push_back(new ark::es2panda::lsp::ClassRelationDetails(details));
    }
    return new std::vector<void *>(std::move(overriddenPtrList));
}
TS_INTEROP_1(getOverriddenFromClassHierarchyItemInfo, KNativePointer, KNativePointer)

KNativePointer impl_getOverridingFromClassHierarchyItemInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<ark::es2panda::lsp::ClassHierarchyItemInfo *>(infoPtr);
    auto &overriding = info->overriding;
    std::vector<void *> overridingPtrList;
    overridingPtrList.reserve(overriding.size());
    for (auto &details : overriding) {
        overridingPtrList.push_back(new ark::es2panda::lsp::ClassRelationDetails(details));
    }
    return new std::vector<void *>(std::move(overridingPtrList));
}
TS_INTEROP_1(getOverridingFromClassHierarchyItemInfo, KNativePointer, KNativePointer)

KNativePointer impl_getImplementedFromClassHierarchyItemInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<ark::es2panda::lsp::ClassHierarchyItemInfo *>(infoPtr);
    auto implemented = info->implemented;
    std::vector<void *> implementedPtrList;
    implementedPtrList.reserve(implemented.size());
    for (auto &details : implemented) {
        implementedPtrList.push_back(new ark::es2panda::lsp::ClassRelationDetails(details));
    }
    return new std::vector<void *>(std::move(implementedPtrList));
}
TS_INTEROP_1(getImplementedFromClassHierarchyItemInfo, KNativePointer, KNativePointer)

KNativePointer impl_getImplementingFromClassHierarchyItemInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<ark::es2panda::lsp::ClassHierarchyItemInfo *>(infoPtr);
    auto implementing = info->implementing;
    std::vector<void *> implementingPtrList;
    implementingPtrList.reserve(implementing.size());
    for (auto &details : implementing) {
        implementingPtrList.push_back(new ark::es2panda::lsp::ClassRelationDetails(details));
    }
    return new std::vector<void *>(std::move(implementingPtrList));
}
TS_INTEROP_1(getImplementingFromClassHierarchyItemInfo, KNativePointer, KNativePointer)

KNativePointer impl_getFileNameFromClassRelationDetails(KNativePointer detailsPtr)
{
    auto *details = reinterpret_cast<ark::es2panda::lsp::ClassRelationDetails *>(detailsPtr);
    return new std::string(details->fileName);
}
TS_INTEROP_1(getFileNameFromClassRelationDetails, KNativePointer, KNativePointer)

KInt impl_getPosFromClassRelationDetails(KNativePointer detailsPtr)
{
    auto *details = reinterpret_cast<ark::es2panda::lsp::ClassRelationDetails *>(detailsPtr);
    return static_cast<KInt>(details->pos);
}
TS_INTEROP_1(getPosFromClassRelationDetails, KInt, KNativePointer)

KInt impl_getKindFromClassRelationDetails(KNativePointer detailsPtr)
{
    auto *details = reinterpret_cast<ark::es2panda::lsp::ClassRelationDetails *>(detailsPtr);
    return static_cast<KInt>(details->kind);
}
TS_INTEROP_1(getKindFromClassRelationDetails, KInt, KNativePointer)

KNativePointer impl_getSymbolDisplayPart(KNativePointer quickInfoPtr)
{
    auto *quickInfo = reinterpret_cast<QuickInfo *>(quickInfoPtr);
    std::vector<void *> ptrs;
    for (auto &el : quickInfo->GetDisplayParts()) {
        ptrs.push_back(new SymbolDisplayPart(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getSymbolDisplayPart, KNativePointer, KNativePointer)

KInt impl_getTextSpanStart(KNativePointer textSpanPtr)
{
    auto *textSpan = reinterpret_cast<TextSpan *>(textSpanPtr);
    return textSpan->start;
}
TS_INTEROP_1(getTextSpanStart, KInt, KNativePointer)

KInt impl_getTextSpanLength(KNativePointer textSpanPtr)
{
    auto *textSpan = reinterpret_cast<TextSpan *>(textSpanPtr);
    return textSpan->length;
}
TS_INTEROP_1(getTextSpanLength, KInt, KNativePointer)

KNativePointer impl_getTextSpan(KNativePointer quickInfoPtr)
{
    auto *quickInfo = reinterpret_cast<QuickInfo *>(quickInfoPtr);
    return new TextSpan(quickInfo->GetTextSpan());
}
TS_INTEROP_1(getTextSpan, KNativePointer, KNativePointer)

KNativePointer impl_createTextSpan(KInt start, KInt length)
{
    return new TextSpan(start, length);
}
TS_INTEROP_2(createTextSpan, KNativePointer, KInt, KInt)

KNativePointer impl_getHighlightTextSpan(KNativePointer highlightPtr)
{
    auto *highlight = reinterpret_cast<HighlightSpan *>(highlightPtr);
    return new TextSpan(highlight->textSpan_);
}
TS_INTEROP_1(getHighlightTextSpan, KNativePointer, KNativePointer)

KNativePointer impl_getHighlightContextSpan(KNativePointer highlightPtr)
{
    auto *highlight = reinterpret_cast<HighlightSpan *>(highlightPtr);
    return new TextSpan(highlight->contextSpan_);
}
TS_INTEROP_1(getHighlightContextSpan, KNativePointer, KNativePointer)

KNativePointer impl_getHighlightFileName(KNativePointer highlightPtr)
{
    auto *highlight = reinterpret_cast<HighlightSpan *>(highlightPtr);
    return new std::string(highlight->fileName_);
}
TS_INTEROP_1(getHighlightFileName, KNativePointer, KNativePointer)

KInt impl_getHighlightIsInString(KNativePointer highlightPtr)
{
    auto *highlight = reinterpret_cast<HighlightSpan *>(highlightPtr);
    return static_cast<int>(highlight->isInString_);
}
TS_INTEROP_1(getHighlightIsInString, KInt, KNativePointer)

KInt impl_getHighlightKind(KNativePointer highlightPtr)
{
    auto *highlight = reinterpret_cast<HighlightSpan *>(highlightPtr);
    return static_cast<size_t>(highlight->kind_);
}
TS_INTEROP_1(getHighlightKind, KInt, KNativePointer)

KNativePointer impl_getHighlightSpanFromHighlights(KNativePointer documentHighlightsPtr)
{
    auto *documentHighlights = reinterpret_cast<DocumentHighlights *>(documentHighlightsPtr);
    std::vector<void *> ptrs;
    for (auto &el : documentHighlights->highlightSpans_) {
        ptrs.push_back(new HighlightSpan(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getHighlightSpanFromHighlights, KNativePointer, KNativePointer)

KNativePointer impl_getDocumentHighlightsFromRef(KNativePointer documentHighlightsReferencesPtr)
{
    auto *documentHighlightsReferences =
        reinterpret_cast<DocumentHighlightsReferences *>(documentHighlightsReferencesPtr);
    std::vector<void *> ptrs;
    for (auto &el : documentHighlightsReferences->documentHighlights_) {
        ptrs.push_back(new DocumentHighlights(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getDocumentHighlightsFromRef, KNativePointer, KNativePointer)

KNativePointer impl_getFileNameFromEntryData(KNativePointer entryDataPtr)
{
    auto *entryData = reinterpret_cast<ark::es2panda::lsp::CompletionEntryData *>(entryDataPtr);
    return new std::string(entryData->GetFileName());
}
TS_INTEROP_1(getFileNameFromEntryData, KNativePointer, KNativePointer)

KNativePointer impl_getNamedExportFromEntryData(KNativePointer entryDataPtr)
{
    auto *entryData = reinterpret_cast<ark::es2panda::lsp::CompletionEntryData *>(entryDataPtr);
    return new std::string(entryData->GetNamedExport());
}
TS_INTEROP_1(getNamedExportFromEntryData, KNativePointer, KNativePointer)

KNativePointer impl_getImportDeclarationFromEntryData(KNativePointer entryDataPtr)
{
    auto *entryData = reinterpret_cast<ark::es2panda::lsp::CompletionEntryData *>(entryDataPtr);
    return new std::string(entryData->GetImportDeclaration());
}
TS_INTEROP_1(getImportDeclarationFromEntryData, KNativePointer, KNativePointer)

KInt impl_getStatusFromEntryData(KNativePointer entryDataPtr)
{
    auto *entryData = reinterpret_cast<ark::es2panda::lsp::CompletionEntryData *>(entryDataPtr);
    return static_cast<size_t>(entryData->GetStatus());
}
TS_INTEROP_1(getStatusFromEntryData, KInt, KNativePointer)

KNativePointer impl_getNameFromEntry(KNativePointer entryPtr)
{
    auto *entry = reinterpret_cast<ark::es2panda::lsp::CompletionEntry *>(entryPtr);
    return new std::string(entry->GetName());
}
TS_INTEROP_1(getNameFromEntry, KNativePointer, KNativePointer)

KNativePointer impl_getSortTextFromEntry(KNativePointer entryPtr)
{
    auto *entry = reinterpret_cast<ark::es2panda::lsp::CompletionEntry *>(entryPtr);
    return new std::string(entry->GetSortText());
}
TS_INTEROP_1(getSortTextFromEntry, KNativePointer, KNativePointer)

KNativePointer impl_getInsertTextFromEntry(KNativePointer entryPtr)
{
    auto *entry = reinterpret_cast<ark::es2panda::lsp::CompletionEntry *>(entryPtr);
    return new std::string(entry->GetInsertText());
}
TS_INTEROP_1(getInsertTextFromEntry, KNativePointer, KNativePointer)

KInt impl_getKindFromEntry(KNativePointer entryPtr)
{
    auto *entry = reinterpret_cast<ark::es2panda::lsp::CompletionEntry *>(entryPtr);
    return static_cast<size_t>(entry->GetCompletionKind());
}
TS_INTEROP_1(getKindFromEntry, KInt, KNativePointer)

KNativePointer impl_getDataFromEntry(KNativePointer entryPtr)
{
    auto *entry = reinterpret_cast<ark::es2panda::lsp::CompletionEntry *>(entryPtr);
    auto data = entry->GetCompletionEntryData();

    if (data.has_value()) {
        return new ark::es2panda::lsp::CompletionEntryData(data.value());
    }
    return nullptr;
}
TS_INTEROP_1(getDataFromEntry, KNativePointer, KNativePointer)

KNativePointer impl_getEntriesFromCompletionInfo(KNativePointer completionInfoPtr)
{
    auto *completionInfo = reinterpret_cast<ark::es2panda::lsp::CompletionInfo *>(completionInfoPtr);
    std::vector<void *> ptrs;
    for (auto &el : completionInfo->GetEntries()) {
        ptrs.push_back(new ark::es2panda::lsp::CompletionEntry(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getEntriesFromCompletionInfo, KNativePointer, KNativePointer)

KNativePointer impl_getUriFromLocation(KNativePointer locPtr)
{
    auto *loc = reinterpret_cast<ReferenceLocation *>(locPtr);
    return new std::string(loc->uri);
}
TS_INTEROP_1(getUriFromLocation, KNativePointer, KNativePointer)

KInt impl_getStartFromLocation(KNativePointer locPtr)
{
    auto *loc = reinterpret_cast<ReferenceLocation *>(locPtr);
    return loc->start;
}
TS_INTEROP_1(getStartFromLocation, KInt, KNativePointer)

KInt impl_getEndFromLocation(KNativePointer locPtr)
{
    auto *loc = reinterpret_cast<ReferenceLocation *>(locPtr);
    return loc->end;
}
TS_INTEROP_1(getEndFromLocation, KInt, KNativePointer)

KBoolean impl_getIsDefinitionFromLocation(KNativePointer locPtr)
{
    auto *loc = reinterpret_cast<ReferenceLocation *>(locPtr);
    return static_cast<KBoolean>(loc->isDefinition);
}
TS_INTEROP_1(getIsDefinitionFromLocation, KBoolean, KNativePointer)

KBoolean impl_getIsImportFromLocation(KNativePointer locPtr)
{
    auto *loc = reinterpret_cast<ReferenceLocation *>(locPtr);
    return static_cast<KBoolean>(loc->isImport);
}
TS_INTEROP_1(getIsImportFromLocation, KBoolean, KNativePointer)

KInt impl_getAccessKindFromLocation(KNativePointer locPtr)
{
    auto *loc = reinterpret_cast<ReferenceLocation *>(locPtr);
    return static_cast<size_t>(loc->accessKind);
}
TS_INTEROP_1(getAccessKindFromLocation, KInt, KNativePointer)

KNativePointer impl_getLocationFromList(KNativePointer listPtr)
{
    auto *list = reinterpret_cast<ReferenceLocationList *>(listPtr);
    std::vector<void *> ptrs;
    for (auto &el : list->referenceLocation) {
        ptrs.push_back(new ReferenceLocation(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getLocationFromList, KNativePointer, KNativePointer)

KBoolean impl_getSafeDeleteInfo(KNativePointer context, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    return static_cast<KBoolean>(ctx->getSafeDeleteInfo(reinterpret_cast<es2panda_Context *>(context), position));
}
TS_INTEROP_2(getSafeDeleteInfo, KBoolean, KNativePointer, KInt)

KNativePointer impl_toLineColumnOffset(KNativePointer context, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    auto *ptrDiag = new ark::es2panda::lsp::LineAndCharacter(
        ctx->toLineColumnOffset(reinterpret_cast<es2panda_Context *>(context), position));
    return ptrDiag;
}
TS_INTEROP_2(toLineColumnOffset, KNativePointer, KNativePointer, KInt)

KInt impl_getLine(KNativePointer locPtr)
{
    auto *loc = reinterpret_cast<ark::es2panda::lsp::LineAndCharacter *>(locPtr);
    return loc->GetLine();
}
TS_INTEROP_1(getLine, KInt, KNativePointer)

KInt impl_getChar(KNativePointer locPtr)
{
    auto *loc = reinterpret_cast<ark::es2panda::lsp::LineAndCharacter *>(locPtr);
    return loc->GetCharacter();
}
TS_INTEROP_1(getChar, KInt, KNativePointer)

KNativePointer impl_getTypeHierarchies(KNativePointer searchContext, KNativePointer context, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    auto *info = new TypeHierarchiesInfo(ctx->getTypeHierarchies(reinterpret_cast<es2panda_Context *>(searchContext),
                                                                 reinterpret_cast<es2panda_Context *>(context),
                                                                 static_cast<std::size_t>(position)));
    return info;
}
TS_INTEROP_3(getTypeHierarchies, KNativePointer, KNativePointer, KNativePointer, KInt)

KNativePointer impl_getFileNameFromTypeHierarchiesInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<TypeHierarchiesInfo *>(infoPtr);
    return new std::string(info->fileName);
}
TS_INTEROP_1(getFileNameFromTypeHierarchiesInfo, KNativePointer, KNativePointer)

KNativePointer impl_getNameFromTypeHierarchiesInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<TypeHierarchiesInfo *>(infoPtr);
    return new std::string(info->name);
}
TS_INTEROP_1(getNameFromTypeHierarchiesInfo, KNativePointer, KNativePointer)

KInt impl_getTypeFromTypeHierarchiesInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<TypeHierarchiesInfo *>(infoPtr);
    return static_cast<size_t>(info->type);
}
TS_INTEROP_1(getTypeFromTypeHierarchiesInfo, KInt, KNativePointer)

KInt impl_getPositionFromTypeHierarchiesInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<TypeHierarchiesInfo *>(infoPtr);
    return static_cast<size_t>(info->pos);
}
TS_INTEROP_1(getPositionFromTypeHierarchiesInfo, KInt, KNativePointer)

KNativePointer impl_getSuperFromTypeHierarchiesInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<TypeHierarchiesInfo *>(infoPtr);
    return new TypeHierarchies(info->superHierarchies);
}
TS_INTEROP_1(getSuperFromTypeHierarchiesInfo, KNativePointer, KNativePointer)

KNativePointer impl_getSubFromTypeHierarchiesInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<TypeHierarchiesInfo *>(infoPtr);
    return new TypeHierarchies(info->subHierarchies);
}
TS_INTEROP_1(getSubFromTypeHierarchiesInfo, KNativePointer, KNativePointer)

KNativePointer impl_getFileNameFromTypeHierarchies(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<TypeHierarchies *>(infoPtr);
    return new std::string(info->fileName);
}
TS_INTEROP_1(getFileNameFromTypeHierarchies, KNativePointer, KNativePointer)

KNativePointer impl_getNameFromTypeHierarchies(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<TypeHierarchies *>(infoPtr);
    return new std::string(info->name);
}
TS_INTEROP_1(getNameFromTypeHierarchies, KNativePointer, KNativePointer)

KInt impl_getPosFromTypeHierarchies(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<TypeHierarchies *>(infoPtr);
    return static_cast<size_t>(info->pos);
}
TS_INTEROP_1(getPosFromTypeHierarchies, KInt, KNativePointer)

KNativePointer impl_getSubOrSuper(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<TypeHierarchies *>(infoPtr);
    std::vector<void *> ptrs;
    for (auto &el : info->subOrSuper) {
        ptrs.push_back(new TypeHierarchies(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getSubOrSuper, KNativePointer, KNativePointer)

KInt impl_getTypeFromTypeHierarchies(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<TypeHierarchies *>(infoPtr);
    return static_cast<size_t>(info->type);
}
TS_INTEROP_1(getTypeFromTypeHierarchies, KInt, KNativePointer)

KNativePointer impl_getCodeFixesAtPosition(KNativePointer context, KInt startPosition, KInt endPosition,
                                           KInt *errorCodesPtr, KInt codeLength)
{
    CodeFixOptions emptyOptions;
    std::vector<int> errorCodesInt;
    if (errorCodesPtr != nullptr && codeLength > 0) {
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic,-warnings-as-errors)
        errorCodesInt = std::vector<int>(reinterpret_cast<int *>(errorCodesPtr),
                                         reinterpret_cast<int *>(errorCodesPtr) + codeLength);
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic,-warnings-as-errors)
    }
    LSPAPI const *ctx = GetImpl();
    auto autofix = ctx->getCodeFixesAtPosition(reinterpret_cast<es2panda_Context *>(context), startPosition,
                                               endPosition, errorCodesInt, emptyOptions);
    return new std::vector<CodeFixActionInfo>(autofix);
}
TS_INTEROP_5(getCodeFixesAtPosition, KNativePointer, KNativePointer, KInt, KInt, KInt *, KInt)

KNativePointer impl_getCodeFixActionInfos(KNativePointer codeFixActionInfoListPtr)
{
    auto *getCodeFixActionInfoList = reinterpret_cast<CodeFixActionInfoList *>(codeFixActionInfoListPtr);
    std::vector<void *> ptrs;
    for (auto &el : getCodeFixActionInfoList->infos_) {
        ptrs.push_back(new CodeFixActionInfo(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getCodeFixActionInfos, KNativePointer, KNativePointer)

KNativePointer impl_getFileTextChangesFromCodeActionInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<CodeActionInfo *>(infoPtr);
    std::vector<void *> ptrs;
    for (auto &el : info->changes_) {
        ptrs.push_back(new FileTextChanges(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getFileTextChangesFromCodeActionInfo, KNativePointer, KNativePointer)

KNativePointer impl_getDescriptionFromCodeActionInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<CodeActionInfo *>(infoPtr);
    return new std::string(info->description_);
}
TS_INTEROP_1(getDescriptionFromCodeActionInfo, KNativePointer, KNativePointer)

KNativePointer impl_getFixNameFromCodeFixActionInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<CodeFixActionInfo *>(infoPtr);
    return new std::string(info->fixName_);
}
TS_INTEROP_1(getFixNameFromCodeFixActionInfo, KNativePointer, KNativePointer)

KNativePointer impl_getFixIdFromCodeFixActionInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<CodeFixActionInfo *>(infoPtr);
    return new std::string(info->fixId_);
}
TS_INTEROP_1(getFixIdFromCodeFixActionInfo, KNativePointer, KNativePointer)

KNativePointer impl_getFixAllDescriptionFromCodeFixActionInfo(KNativePointer infoPtr)
{
    auto *info = reinterpret_cast<CodeFixActionInfo *>(infoPtr);
    return new std::string(info->fixAllDescription_);
}
TS_INTEROP_1(getFixAllDescriptionFromCodeFixActionInfo, KNativePointer, KNativePointer)

KNativePointer impl_getSpanOfEnclosingComment(KNativePointer context, KInt position, KBoolean onlyMultiLine)
{
    LSPAPI const *ctx = GetImpl();
    auto *textSpan = new TextSpan(
        ctx->getSpanOfEnclosingComment(reinterpret_cast<es2panda_Context *>(context), position, onlyMultiLine != 0));
    return textSpan;
}
TS_INTEROP_3(getSpanOfEnclosingComment, KNativePointer, KNativePointer, KInt, KBoolean)

KNativePointer impl_getInlayHintText(KNativePointer hintPtr)
{
    auto *hint = reinterpret_cast<InlayHint *>(hintPtr);
    return new std::string(hint->text);
}
TS_INTEROP_1(getInlayHintText, KNativePointer, KNativePointer)

KInt impl_getInlayHintNumber(KNativePointer hintPtr)
{
    auto *hint = reinterpret_cast<InlayHint *>(hintPtr);
    return hint->number;
}
TS_INTEROP_1(getInlayHintNumber, KInt, KNativePointer)

KInt impl_getInlayHintKind(KNativePointer hintPtr)
{
    auto *hint = reinterpret_cast<InlayHint *>(hintPtr);
    return static_cast<size_t>(hint->kind);
}
TS_INTEROP_1(getInlayHintKind, KInt, KNativePointer)

KBoolean impl_getInlayHintWhitespaceBefore(KNativePointer hintPtr)
{
    auto *hint = reinterpret_cast<InlayHint *>(hintPtr);
    return hint->whitespaceBefore ? 1 : 0;
}
TS_INTEROP_1(getInlayHintWhitespaceBefore, KBoolean, KNativePointer)

KBoolean impl_getInlayHintWhitespaceAfter(KNativePointer hintPtr)
{
    auto *hint = reinterpret_cast<InlayHint *>(hintPtr);
    return hint->whitespaceAfter ? 1 : 0;
}
TS_INTEROP_1(getInlayHintWhitespaceAfter, KBoolean, KNativePointer)

KNativePointer impl_getInlayHints(KNativePointer inlayHintListPtr)
{
    auto *inlayHintList = reinterpret_cast<InlayHintList *>(inlayHintListPtr);
    std::vector<void *> ptrs;
    for (auto &el : inlayHintList->hints) {
        ptrs.push_back(new InlayHint(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getInlayHints, KNativePointer, KNativePointer)

KNativePointer impl_getInlayHintList(KNativePointer context, KNativePointer span)
{
    LSPAPI const *ctx = GetImpl();
    auto *inlayHints = new InlayHintList(
        ctx->provideInlayHints(reinterpret_cast<es2panda_Context *>(context), reinterpret_cast<TextSpan *>(span)));
    return inlayHints;
}
TS_INTEROP_2(getInlayHintList, KNativePointer, KNativePointer, KNativePointer)

KNativePointer impl_getSignatureHelpParameterName(KNativePointer parameterPtr)
{
    auto *parameterRef = reinterpret_cast<SignatureHelpParameter *>(parameterPtr);
    return new std::string(parameterRef->GetName());
}
TS_INTEROP_1(getSignatureHelpParameterName, KNativePointer, KNativePointer)

KNativePointer impl_getSignatureHelpParameterDocumentation(KNativePointer parameterPtr)
{
    auto *parameterRef = reinterpret_cast<SignatureHelpParameter *>(parameterPtr);
    std::vector<void *> ptrs;
    for (auto &el : parameterRef->GetDocumentation()) {
        ptrs.push_back(new SymbolDisplayPart(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getSignatureHelpParameterDocumentation, KNativePointer, KNativePointer)

KNativePointer impl_getSignatureHelpParameterDisplayParts(KNativePointer parameterPtr)
{
    auto *parameterRef = reinterpret_cast<SignatureHelpParameter *>(parameterPtr);
    std::vector<void *> ptrs;
    for (auto &el : parameterRef->GetDisplayParts()) {
        ptrs.push_back(new SymbolDisplayPart(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getSignatureHelpParameterDisplayParts, KNativePointer, KNativePointer)

KNativePointer impl_getSignatureHelpItemPrefix(KNativePointer itemPtr)
{
    auto *itemRef = reinterpret_cast<SignatureHelpItem *>(itemPtr);
    std::vector<void *> ptrs;
    for (auto &el : itemRef->GetPrefixDisplayParts()) {
        ptrs.push_back(new SymbolDisplayPart(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getSignatureHelpItemPrefix, KNativePointer, KNativePointer)

KNativePointer impl_getSignatureHelpItemSuffix(KNativePointer itemPtr)
{
    auto *itemRef = reinterpret_cast<SignatureHelpItem *>(itemPtr);
    std::vector<void *> ptrs;
    for (auto &el : itemRef->GetSuffixDisplayParts()) {
        ptrs.push_back(new SymbolDisplayPart(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getSignatureHelpItemSuffix, KNativePointer, KNativePointer)

KNativePointer impl_getSignatureHelpItemSeparator(KNativePointer itemPtr)
{
    auto *itemRef = reinterpret_cast<SignatureHelpItem *>(itemPtr);
    std::vector<void *> ptrs;
    for (auto &el : itemRef->GetSeparatorDisplayParts()) {
        ptrs.push_back(new SymbolDisplayPart(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getSignatureHelpItemSeparator, KNativePointer, KNativePointer)

KNativePointer impl_getSignatureHelpItemParameter(KNativePointer itemPtr)
{
    auto *itemRef = reinterpret_cast<SignatureHelpItem *>(itemPtr);
    std::vector<void *> ptrs;
    for (auto &el : itemRef->GetParameters()) {
        ptrs.push_back(new SignatureHelpParameter(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getSignatureHelpItemParameter, KNativePointer, KNativePointer)

KNativePointer impl_getSignatureHelpItemDocumentation(KNativePointer itemPtr)
{
    auto *itemRef = reinterpret_cast<SignatureHelpItem *>(itemPtr);
    std::vector<void *> ptrs;
    for (auto &el : itemRef->GetDocumentation()) {
        ptrs.push_back(new SymbolDisplayPart(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getSignatureHelpItemDocumentation, KNativePointer, KNativePointer)

KNativePointer impl_getSignatureHelpItem(KNativePointer itemsPtr)
{
    auto *itemsRef = reinterpret_cast<SignatureHelpItems *>(itemsPtr);
    std::vector<void *> ptrs;
    for (auto &el : itemsRef->GetItems()) {
        ptrs.push_back(new SignatureHelpItem(el));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_1(getSignatureHelpItem, KNativePointer, KNativePointer)

KNativePointer impl_getApplicableSpan(KNativePointer itemsPtr)
{
    auto *itemsRef = reinterpret_cast<SignatureHelpItems *>(itemsPtr);
    return new TextSpan(itemsRef->GetApplicableSpan());
}
TS_INTEROP_1(getApplicableSpan, KNativePointer, KNativePointer)

KInt impl_getSelectedItemIndex(KNativePointer itemsPtr)
{
    auto *itemsRef = reinterpret_cast<SignatureHelpItems *>(itemsPtr);
    return itemsRef->GetSelectedItemIndex();
}
TS_INTEROP_1(getSelectedItemIndex, KInt, KNativePointer)

KInt impl_getArgumentIndex(KNativePointer itemsPtr)
{
    auto *itemsRef = reinterpret_cast<SignatureHelpItems *>(itemsPtr);
    return itemsRef->GetArgumentIndex();
}
TS_INTEROP_1(getArgumentIndex, KInt, KNativePointer)

KInt impl_getArgumentCount(KNativePointer itemsPtr)
{
    auto *itemsRef = reinterpret_cast<SignatureHelpItems *>(itemsPtr);
    return itemsRef->GetArgumentCount();
}
TS_INTEROP_1(getArgumentCount, KInt, KNativePointer)

KNativePointer impl_getSignatureHelpItems(KNativePointer context, KInt position)
{
    LSPAPI const *ctx = GetImpl();
    auto *textSpan =
        new SignatureHelpItems(ctx->getSignatureHelpItems(reinterpret_cast<es2panda_Context *>(context), position));
    return textSpan;
}
TS_INTEROP_2(getSignatureHelpItems, KNativePointer, KNativePointer, KInt)

KInt impl_getOffsetByColAndLine(KStringPtr &sourceCodePtr, KInt line, KInt column)
{
    LSPAPI const *impl = GetImpl();
    return impl->getOffsetByColAndLine(sourceCodePtr.Data(), line, column);
}
TS_INTEROP_3(getOffsetByColAndLine, KInt, KStringPtr, KInt, KInt)

KNativePointer impl_getClassDefinition(KNativePointer astNodePtr, KStringPtr &nodeNamePtr)
{
    auto ast = reinterpret_cast<es2panda_AstNode *>(astNodePtr);
    LSPAPI const *impl = GetImpl();
    return impl->getClassDefinition(ast, nodeNamePtr.Data());
}
TS_INTEROP_2(getClassDefinition, KNativePointer, KNativePointer, KStringPtr)

KNativePointer impl_getIdentifier(KNativePointer astNodePtr, KStringPtr &nodeNamePtr)
{
    auto ast = reinterpret_cast<es2panda_AstNode *>(astNodePtr);
    LSPAPI const *impl = GetImpl();
    return impl->getIdentifier(ast, nodeNamePtr.Data());
}
TS_INTEROP_2(getIdentifier, KNativePointer, KNativePointer, KStringPtr)

KNativePointer impl_getProgramAst(KNativePointer contextPtr)
{
    auto context = reinterpret_cast<es2panda_Context *>(contextPtr);
    LSPAPI const *impl = GetImpl();
    return impl->getProgramAst(context);
}
TS_INTEROP_1(getProgramAst, KNativePointer, KNativePointer)

KNativePointer impl_getNodeInfosByDefinitionData(KNativePointer context, KInt position)
{
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    LSPAPI const *impl = GetImpl();
    std::vector<void *> ptrs;
    for (auto &item : impl->getNodeInfosByDefinitionData(ctx, position)) {
        ptrs.push_back(new NodeInfo(item));
    }
    return new std::vector<void *>(ptrs);
}
TS_INTEROP_2(getNodeInfosByDefinitionData, KNativePointer, KNativePointer, KInt)

KNativePointer impl_getNameByNodeInfo(KNativePointer nodeInfo)
{
    auto *info = reinterpret_cast<NodeInfo *>(nodeInfo);
    return new std::string(info->name);
}
TS_INTEROP_1(getNameByNodeInfo, KNativePointer, KNativePointer)

KNativePointer impl_CreateNodeInfoPtr(KStringPtr &nodeName, KInt nodeKind)
{
    return new NodeInfo(nodeName.Data(), ark::es2panda::ir::AstNodeType(nodeKind));
}
TS_INTEROP_2(CreateNodeInfoPtr, KNativePointer, KStringPtr, KInt)

KNativePointer impl_getKindByNodeInfo(KNativePointer nodeInfo)
{
    auto *info = reinterpret_cast<NodeInfo *>(nodeInfo);
    return new std::string(ark::es2panda::ir::ToString(info->kind));
}
TS_INTEROP_1(getKindByNodeInfo, KNativePointer, KNativePointer)

KNativePointer impl_getDefinitionDataFromNode(KNativePointer context, KStringArray pointerArrayPtr, KInt arraySize)
{
    auto pointerArray = ParsePointerArray(arraySize, pointerArrayPtr);
    auto nodeInfos = std::vector<NodeInfo *> {};
    nodeInfos.reserve(arraySize);
    for (std::size_t i = 0; i < static_cast<std::size_t>(arraySize); ++i) {
        auto contextPtr = reinterpret_cast<NodeInfo *>(pointerArray[i]);
        if (contextPtr != nullptr) {
            nodeInfos.push_back(contextPtr);
        }
    }
    auto ctx = reinterpret_cast<es2panda_Context *>(context);
    LSPAPI const *impl = GetImpl();
    return new DefinitionInfo(impl->getDefinitionDataFromNode(ctx, nodeInfos));
}
TS_INTEROP_3(getDefinitionDataFromNode, KNativePointer, KNativePointer, KStringArray, KInt)

KInt impl_getSourceLocationLine(KNativePointer locationPtr)
{
    auto *location = reinterpret_cast<std::pair<size_t, size_t> *>(locationPtr);
    return location->first;
}
TS_INTEROP_1(getSourceLocationLine, KInt, KNativePointer)

KInt impl_getSourceLocationColumn(KNativePointer locationPtr)
{
    auto *location = reinterpret_cast<std::pair<size_t, size_t> *>(locationPtr);
    return location->second;
}
TS_INTEROP_1(getSourceLocationColumn, KInt, KNativePointer)

KNativePointer impl_getColAndLineByOffset(KStringPtr &sourceCodePtr, KInt offset)
{
    LSPAPI const *impl = GetImpl();
    return new std::pair<size_t, size_t>(impl->getColAndLineByOffset(sourceCodePtr.Data(), offset));
}
TS_INTEROP_2(getColAndLineByOffset, KNativePointer, KStringPtr, KInt)
