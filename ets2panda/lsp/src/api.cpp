/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "api.h"
#include "util/eheap.h"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "class_hierarchy.h"
#include "get_node.h"
#include "lsp/include/organize_imports.h"
#include "get_safe_delete_info.h"
#include "get_definition_at_position.h"
#include "internal_api.h"
#include "ir/astNode.h"
#include "find_safe_delete_location.h"
#include "references.h"
#include "public/es2panda_lib.h"
#include "cancellation_token.h"
#include "generate_constructor.h"
#include "public/public.h"
#include "util/options.h"
#include "quick_info.h"
#include "suggestion_diagnostics.h"
#include "brace_matching.h"
#include "line_column_offset.h"
#include "script_element_kind.h"
#include "services/services.h"
#include "get_class_property_info.h"
#include "inlay_hints.h"
#include "signature_help.h"
#include "completions_details.h"
#include "get_name_or_dotted_name_span.h"
#include "get_signature.h"
#include "node_matchers.h"
#include "compiler/lowering/util.h"
#include "formatting/formatting.h"
#include "lsp_utils.h"
#include "symbol_reference_index.h"

using ark::es2panda::lsp::details::GetCompletionEntryDetailsImpl;

extern "C" {
namespace ark::es2panda::lsp {

DefinitionInfo GetDefinitionAtPosition(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);

    std::string source = std::string(ctx->parserProgram->SourceCode());

    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);

    auto target = GetDefinitionTargetAtPosition(context, byteOffset);
    if (target.source.empty()) {
        return {target.fileName, target.start, target.length};
    }

    size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(target.source, target.start);
    size_t endCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(target.source, target.start + target.length);
    return {target.fileName, startCharOffset, endCharOffset - startCharOffset};
}

DefinitionInfo GetImplementationAtPosition(es2panda_Context *context, size_t position)
{
    return GetDefinitionAtPosition(context, position);
}

bool IsPackageModule(es2panda_Context *context)
{
    return reinterpret_cast<public_lib::Context *>(context)->parserProgram->Is<util::ModuleKind::PACKAGE>();
}

CompletionEntryKind GetAliasScriptElementKind(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    auto result = GetAliasScriptElementKindImpl(context, byteOffset);
    return result;
}

// Maybe the "isPackageModule" parameter should be removed, because the frontend will remove ModuleKind::MODULE.
References GetFileReferences(char const *fileName, es2panda_Context *context, bool isPackageModule)
{
    References result {};
    if (context == nullptr || fileName == nullptr) {
        return result;
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    result = GetFileReferencesFromIndex(context, std::string(fileName), isPackageModule);

    for (auto &ref : result.referenceInfos) {
        auto fileSource = GetIndexedFileSource(ref.fileName);
        if (fileSource.empty()) {
            continue;
        }
        size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(fileSource, ref.start);
        size_t lengthChar =
            ark::es2panda::lsp::ByteOffsetToCodePointOffset(fileSource, ref.start + ref.length) - startCharOffset;
        ref.start = startCharOffset;
        ref.length = lengthChar;
    }

    auto compare = [](const ReferenceInfo &lhs, const ReferenceInfo &rhs) {
        if (lhs.fileName != rhs.fileName) {
            return lhs.fileName < rhs.fileName;
        }
        if (lhs.start != rhs.start) {
            return lhs.start < rhs.start;
        }
        return lhs.length < rhs.length;
    };
    RemoveDuplicates(result.referenceInfos, compare);
    return result;
}

DeclInfo GetDeclInfo(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    DeclInfo result;
    if (context == nullptr) {
        return result;
    }
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    auto astNode = GetTouchingToken(context, byteOffset, false);
    auto declInfo = GetDeclInfoImpl(astNode);
    result.fileName = std::get<0>(declInfo);
    result.fileText = std::get<1>(declInfo);
    return result;
}

std::vector<ClassHierarchyItemInfo> GetClassHierarchies(std::vector<es2panda_Context *> *contextList,
                                                        const char *fileName, size_t pos)
{
    auto *ctxList = reinterpret_cast<std::vector<es2panda_Context *> *>(contextList);
    std::string source;
    if (!ctxList->empty()) {
        auto ctx = reinterpret_cast<public_lib::Context *>((*ctxList)[0]);
        SetPhaseManager(ctx->phaseManager);
        source = std::string(ctx->parserProgram->SourceCode());
    }
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, pos);
    return GetClassHierarchiesImpl(contextList, std::string(fileName), byteOffset);
}

bool GetSafeDeleteInfo(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    return GetSafeDeleteInfoImpl(context, byteOffset);
}

References GetReferencesAtPosition(es2panda_Context *context, DeclInfo *declInfo)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    auto result = GetReferencesAtPositionImpl(context, {declInfo->fileName, declInfo->fileText});
    for (auto &ref : result.referenceInfos) {
        size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, ref.start);
        size_t lengthChar =
            ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, ref.start + ref.length) - startCharOffset;
        ref.start = startCharOffset;
        ref.length = lengthChar;
    }
    auto compare = [](const ReferenceInfo &lhs, const ReferenceInfo &rhs) {
        if (lhs.fileName != rhs.fileName) {
            return lhs.fileName < rhs.fileName;
        }
        if (lhs.start != rhs.start) {
            return lhs.start < rhs.start;
        }
        return lhs.length < rhs.length;
    };
    RemoveDuplicates(result.referenceInfos, compare);
    return result;
}

void InitSymbolReferenceIndexWrapper()
{
    InitSymbolReferenceIndex();
}

void ClearSymbolReferenceIndexWrapper()
{
    ClearSymbolReferenceIndex();
}

void ResetEHeapWrapper()
{
    ark::es2panda::EHeap::ResetEHeapSpace();
}

bool BuildSymbolReferenceIndexForContextWrapper(es2panda_Context *context)
{
    if (context == nullptr) {
        return false;
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    return BuildSymbolReferenceIndexForContext(context);
}

bool BuildSymbolReferenceIndexForContextWithExternalWrapper(es2panda_Context *context)
{
    if (context == nullptr) {
        return false;
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    return BuildSymbolReferenceIndexForContextWithExternal(context);
}

bool RemoveSymbolReferenceIndexForFileWrapper(const char *fileName)
{
    if (fileName == nullptr) {
        return false;
    }
    return RemoveSymbolReferenceIndexForFile(std::string(fileName));
}

References GetReferencesAtPositionFromIndexWrapper(es2panda_Context *context, size_t position)
{
    References result {};
    if (context == nullptr) {
        return result;
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);

    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    result = GetReferencesAtPositionFromIndex(context, byteOffset);
    if (!result.definitionInfo.fileName.empty()) {
        auto defFileSource = GetIndexedFileSource(result.definitionInfo.fileName);
        if (!defFileSource.empty()) {
            size_t startCharOffset =
                ark::es2panda::lsp::ByteOffsetToCodePointOffset(defFileSource, result.definitionInfo.start);
            size_t lengthChar = ark::es2panda::lsp::ByteOffsetToCodePointOffset(
                                    defFileSource, result.definitionInfo.start + result.definitionInfo.length) -
                                startCharOffset;
            result.definitionInfo.start = startCharOffset;
            result.definitionInfo.length = lengthChar;
        }
    }

    for (auto &ref : result.referenceInfos) {
        auto fileSource = GetIndexedFileSource(ref.fileName);
        if (fileSource.empty()) {
            continue;
        }
        size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(fileSource, ref.start);
        size_t lengthChar =
            ark::es2panda::lsp::ByteOffsetToCodePointOffset(fileSource, ref.start + ref.length) - startCharOffset;
        ref.start = startCharOffset;
        ref.length = lengthChar;
    }

    auto compare = [](const ReferenceInfo &lhs, const ReferenceInfo &rhs) {
        if (lhs.fileName != rhs.fileName) {
            return lhs.fileName < rhs.fileName;
        }
        if (lhs.start != rhs.start) {
            return lhs.start < rhs.start;
        }
        return lhs.length < rhs.length;
    };
    RemoveDuplicates(result.referenceInfos, compare);
    return result;
}

std::string GetIndexedFileSourceWrapper(const std::string &fileName)
{
    return GetIndexedFileSource(fileName);
}

es2panda_AstNode *GetPrecedingToken(es2panda_Context *context, const size_t pos)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    auto ast = ctx->parserProgram->Ast();
    return reinterpret_cast<es2panda_AstNode *>(FindPrecedingToken(pos, ast));
}

std::string GetCurrentTokenValue(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);

    auto result = GetCurrentTokenValueImpl(context, byteOffset, nullptr);
    return result;
}

std::vector<FileTextChanges> OrganizeImportsImpl(es2panda_Context *context, char const *fileName)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    auto result = OrganizeImports::Organize(context, fileName);

    for (auto &change : result) {
        for (auto &textChange : change.textChanges) {
            size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, textChange.span.start);
            size_t endCharOffset =
                ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, textChange.span.start + textChange.span.length);
            textChange.span.start = startCharOffset;
            textChange.span.length = endCharOffset - startCharOffset;
        }
    }
    return result;
}

QuickInfo GetQuickInfoAtPosition(const char *fileName, es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    auto res = GetQuickInfoAtPositionImpl(context, byteOffset, fileName);

    TextSpan span = res.GetTextSpan();
    size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, span.start);
    size_t endCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, span.start + span.length);
    span.start = startCharOffset;
    span.length = endCharOffset - startCharOffset;

    return res;
}

// find the Definition node by using the entryname And return CompletionEntryDetails
CompletionEntryDetails GetCompletionEntryDetails(const char *entryName, const char *fileName, es2panda_Context *context,
                                                 size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    auto result = GetCompletionEntryDetailsImpl(context, byteOffset, fileName, entryName);
    return result;
}

TextSpan GetSpanOfEnclosingComment(es2panda_Context *context, size_t pos, bool onlyMultiLine)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, pos);
    auto *range = ctx->allocator->New<CommentRange>();
    GetRangeOfEnclosingComment(context, byteOffset, range);
    if ((range != nullptr) && (!onlyMultiLine || range->kind_ == CommentKind::MULTI_LINE)) {
        size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, range->pos_);
        size_t endCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, range->end_);
        return TextSpan(startCharOffset, endCharOffset - startCharOffset);
    }
    return TextSpan(0, 0);
}

DiagnosticReferences GetSemanticDiagnostics(es2panda_Context *context)
{
    DiagnosticReferences result {};
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    ctx->diagnosticEngine->CleanDuplicateLog(util::DiagnosticType::SEMANTIC);
    ctx->diagnosticEngine->CleanDuplicateLog(util::DiagnosticType::FATAL);
    ctx->diagnosticEngine->CleanDuplicateLog(util::DiagnosticType::WARNING);
    SetPhaseManager(ctx->phaseManager);
    const auto &diagnostics = ctx->diagnosticEngine->GetDiagnosticStorage(util::DiagnosticType::SEMANTIC);
    const auto &diagnosticsFatal = ctx->diagnosticEngine->GetDiagnosticStorage(util::DiagnosticType::FATAL);
    const auto &diagnosticsWarning = ctx->diagnosticEngine->GetDiagnosticStorage(util::DiagnosticType::WARNING);
    MakeDiagnosticReferences(context, diagnostics, result);
    MakeDiagnosticReferences(context, diagnosticsFatal, result);
    MakeDiagnosticReferences(context, diagnosticsWarning, result);
    return result;
}

DiagnosticReferences GetSyntacticDiagnostics(es2panda_Context *context)
{
    DiagnosticReferences result {};
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    ctx->diagnosticEngine->CleanDuplicateLog(util::DiagnosticType::SYNTAX);
    SetPhaseManager(ctx->phaseManager);
    const auto &diagnostics = ctx->diagnosticEngine->GetDiagnosticStorage(util::DiagnosticType::SYNTAX);
    const auto &diagnosticsPluginError =
        ctx->diagnosticEngine->GetDiagnosticStorage(util::DiagnosticType::PLUGIN_ERROR);
    const auto &diagnosticsPluginWarning =
        ctx->diagnosticEngine->GetDiagnosticStorage(util::DiagnosticType::PLUGIN_WARNING);
    MakeDiagnosticReferences(context, diagnostics, result);
    MakeDiagnosticReferences(context, diagnosticsPluginError, result);
    MakeDiagnosticReferences(context, diagnosticsPluginWarning, result);
    return result;
}

DiagnosticReferences GetCompilerOptionsDiagnostics(char const *fileName, CancellationToken cancellationToken)
{
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED);

    DiagnosticReferences result {};
    if (cancellationToken.IsCancellationRequested()) {
        return result;
    }
    GetOptionDiagnostics(context, result);

    auto options = reinterpret_cast<public_lib::Context *>(context)->config->options;
    auto compilationList = FindProjectSources(options->ArkTSConfig());
    initializer.DestroyContext(context);

    for (auto const &file : compilationList) {
        if (cancellationToken.IsCancellationRequested()) {
            return result;
        }
        auto fileContext = initializer.CreateContext(file.first.c_str(), ES2PANDA_STATE_CHECKED);
        GetGlobalDiagnostics(fileContext, result);
        initializer.DestroyContext(fileContext);
    }

    return result;
}

TypeHierarchiesInfo GetTypeHierarchies(es2panda_Context *searchContext, es2panda_Context *context, const size_t pos)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, pos);
    auto declaration = GetTargetDeclarationNodeByPosition(context, byteOffset);
    return GetTypeHierarchiesImpl(searchContext, byteOffset, declaration);
}

DocumentHighlightsReferences GetDocumentHighlights(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());

    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);

    DocumentHighlightsReferences result = {};
    auto docHighlight = GetDocumentHighlightsImpl(context, byteOffset);

    for (auto &span : docHighlight.highlightSpans_) {
        size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, span.textSpan_.start);
        size_t endCharOffset =
            ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, span.textSpan_.start + span.textSpan_.length);
        span.textSpan_.start = startCharOffset;
        span.textSpan_.length = endCharOffset - startCharOffset;
    }

    result.documentHighlights_.push_back(docHighlight);
    return result;
}

std::vector<SafeDeleteLocation> FindSafeDeleteLocation(es2panda_Context *ctx, size_t position)
{
    return FindSafeDeleteLocationImpl(ctx, position);
}

std::vector<ark::es2panda::lsp::ReferencedNode> FindReferencesWrapper(
    ark::es2panda::lsp::CancellationToken *tkn, const std::vector<ark::es2panda::SourceFile> &srcFiles,
    const ark::es2panda::SourceFile &srcFile, size_t position)
{
    auto tmp = FindReferences(tkn, srcFiles, srcFile, position);
    std::vector<ark::es2panda::lsp::ReferencedNode> res(tmp.size());
    for (const auto &entry : tmp) {
        res.emplace_back(entry);
    }
    return res;
}

RenameInfoType GetRenameInfoWrapper(es2panda_Context *context, size_t pos, const char *pandaLibPath)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, pos);
    RenameInfoType info = GetRenameInfo(context, byteOffset, std::string(pandaLibPath));
    if (std::holds_alternative<ark::es2panda::lsp::RenameInfoSuccess>(info)) {
        const auto &success = std::get<ark::es2panda::lsp::RenameInfoSuccess>(info);
        const TextSpan &oldSpan = success.GetTriggerSpan();
        size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, oldSpan.start);
        size_t endCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, oldSpan.start + oldSpan.length);
        TextSpan newSpan(startCharOffset, endCharOffset - startCharOffset);

        RenameInfoSuccess newSuccess(success.GetCanRenameSuccess(), success.GetFileToRename(), success.GetKind(),
                                     success.GetDisplayName(), success.GetFullDisplayName(), success.GetKindModifiers(),
                                     newSpan);
        return newSuccess;
    }
    return info;
}

std::vector<TextSpan> GetBraceMatchingAtPositionWrapper(char const *fileName, size_t position)
{
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED);
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    auto result = GetBraceMatchingAtPosition(context, byteOffset);
    for (auto &span : result) {
        size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, span.start);
        size_t endCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, span.start + span.length);
        span.start = startCharOffset;
        span.length = endCharOffset - startCharOffset;
    }

    initializer.DestroyContext(context);
    return result;
}

std::vector<ark::es2panda::lsp::RenameLocation> FindRenameLocationsWrapper(
    const std::vector<es2panda_Context *> &fileContexts, es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    auto locations = FindRenameLocations(fileContexts, context, byteOffset);

    std::unordered_map<std::string, std::string> fileSourceMap;
    for (auto *fileCtxRaw : fileContexts) {
        auto *fileCtx = reinterpret_cast<public_lib::Context *>(fileCtxRaw);
        fileSourceMap[std::string(fileCtx->parserProgram->SourceFile().GetAbsolutePath().Utf8())] =
            std::string(fileCtx->parserProgram->SourceCode());
    }

    std::vector<ark::es2panda::lsp::RenameLocation> result;
    result.reserve(locations.size());
    for (auto loc : locations) {
        auto it = fileSourceMap.find(std::string(loc.fileName));
        std::string fileSource = (it != fileSourceMap.end()) ? it->second : "";
        loc.start = ark::es2panda::lsp::ByteOffsetToCodePointOffset(fileSource, loc.start);
        loc.end = ark::es2panda::lsp::ByteOffsetToCodePointOffset(fileSource, loc.end);
        result.push_back(loc);
    }
    return result;
}

std::set<RenameLocation> FindRenameLocationsInCurrentFileWrapper(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    auto locations = FindRenameLocationsInCurrentFile(context, byteOffset);

    std::set<RenameLocation> result;
    for (auto loc : locations) {
        loc.start = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, loc.start);
        loc.end = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, loc.end);
        result.insert(loc);
    }
    return result;
}

bool NeedsCrossFileRenameWrapper(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    return NeedsCrossFileRename(context, position);
}

std::vector<ark::es2panda::lsp::RenameLocation> FindRenameLocationsWithCancellationWrapper(
    ark::es2panda::lsp::CancellationToken *tkn, const std::vector<es2panda_Context *> &fileContexts,
    es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    auto locations = FindRenameLocations(tkn, fileContexts, context, byteOffset);

    std::unordered_map<std::string, std::string> fileSourceMap;
    for (auto *fileCtxRaw : fileContexts) {
        auto *fileCtx = reinterpret_cast<public_lib::Context *>(fileCtxRaw);
        fileSourceMap[std::string(fileCtx->parserProgram->SourceFile().GetAbsolutePath().Utf8())] =
            std::string(fileCtx->parserProgram->SourceCode());
    }

    std::vector<ark::es2panda::lsp::RenameLocation> res;
    res.reserve(locations.size());
    for (auto loc : locations) {
        auto it = fileSourceMap.find(std::string(loc.fileName));
        std::string fileSource = (it != fileSourceMap.end()) ? it->second : "";
        loc.start = ark::es2panda::lsp::ByteOffsetToCodePointOffset(fileSource, loc.start);
        loc.end = ark::es2panda::lsp::ByteOffsetToCodePointOffset(fileSource, loc.end);
        res.push_back(loc);
    }
    return res;
}

std::vector<FieldsInfo> GetClassPropertyInfoWrapper(es2panda_Context *context, size_t position,
                                                    bool shouldCollectInherited)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    auto result = GetClassPropertyInfo(context, byteOffset, shouldCollectInherited);
    for (auto &fieldsInfo : result) {
        for (auto &prop : fieldsInfo.properties) {
            size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, prop.start);
            size_t endCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, prop.end);
            prop.start = startCharOffset;
            prop.end = endCharOffset;
        }
    }
    return result;
}

DiagnosticReferences GetSuggestionDiagnostics(es2panda_Context *context)
{
    DiagnosticReferences res {};
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    auto ast = ctx->parserProgram->Ast();
    auto vec = GetSuggestionDiagnosticsImpl(ast, context);
    res.diagnostic.reserve(vec.size());
    for (const auto &diag : vec) {
        res.diagnostic.push_back(diag.diagnostic);
    }
    return res;
}

ark::es2panda::lsp::CompletionInfo GetCompletionsAtPosition(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    auto result = CompletionInfo(GetCompletionsAtPositionImpl(context, byteOffset));
    return result;
}

ClassHierarchy GetClassHierarchyInfo(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    auto result = GetClassHierarchyInfoImpl(context, byteOffset);
    return result;
}

std::vector<Location> GetImplementationLocationAtPositionWrapper(es2panda_Context *context, int position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    return GetImplementationLocationAtPosition(context, position);
}

RefactorEditInfo GetClassConstructorInfo(es2panda_Context *context, size_t position,
                                         const std::vector<std::string> &properties)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    RefactorEditInfo info = RefactorEditInfo(GetRefactorActionsToGenerateConstructor(context, byteOffset, properties));

    auto fileTextChanges = info.GetFileTextChanges();
    for (auto &fileChange : fileTextChanges) {
        for (auto &textChange : fileChange.textChanges) {
            size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, textChange.span.start);
            size_t endCharOffset =
                ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, textChange.span.start + textChange.span.length);
            textChange.span.start = startCharOffset;
            textChange.span.length = endCharOffset - startCharOffset;
        }
    }
    info.SetFileTextChanges(fileTextChanges);
    return info;
}

LineAndCharacter ToLineColumnOffsetWrapper(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    auto result = ToLineColumnOffset(context, byteOffset);

    size_t charOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, result.GetCharacter());
    return LineAndCharacter(result.GetLine(), charOffset);
}

// Returns type of refactoring and action that can be performed based
// on the input kind information and cursor position
std::vector<ApplicableRefactorInfo> GetApplicableRefactors(es2panda_Context *context, const char *kind, size_t startPos,
                                                           size_t endPos)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t startByteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, startPos);
    size_t endByteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, endPos);

    RefactorContext refactorContext;
    refactorContext.context = context;
    refactorContext.kind = kind;
    refactorContext.span.pos = startByteOffset;
    refactorContext.span.end = endByteOffset;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    return result;
}

std::unique_ptr<ark::es2panda::lsp::RefactorEditInfo> GetEditsForRefactor(
    const ark::es2panda::lsp::RefactorContext &context, const std::string &refactorName, const std::string &actionName)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context.context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());

    RefactorContext newContext = context;
    newContext.span.pos = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, context.span.pos);
    newContext.span.end = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, context.span.end);

    auto edits = ark::es2panda::lsp::GetEditsForRefactorsImpl(newContext, refactorName, actionName);
    if (edits == nullptr) {
        return edits;
    }

    auto fileTextChanges = edits->GetFileTextChanges();
    std::string refactoredSource = source;
    if (edits->GetRenameLocation().has_value()) {
        refactoredSource = GetRefactoredSourceForRenameLocation(source, fileTextChanges, edits->GetRenameFileName());
    }
    for (auto &fileChange : fileTextChanges) {
        for (auto &textChange : fileChange.textChanges) {
            size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, textChange.span.start);
            size_t endCharOffset =
                ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, textChange.span.start + textChange.span.length);
            textChange.span.start = startCharOffset;
            textChange.span.length = endCharOffset - startCharOffset;
        }
    }
    edits->SetFileTextChanges(fileTextChanges);
    if (edits->GetRenameLocation().has_value()) {
        const size_t renameByteOffset = edits->GetRenameLocation().value();
        edits->SetRenameLocation(ark::es2panda::lsp::ByteOffsetToCodePointOffset(refactoredSource, renameByteOffset));
    }

    return edits;
}

std::vector<ark::es2panda::lsp::TodoComment> GetTodoComments(
    char const *fileName, std::vector<ark::es2panda::lsp::TodoCommentDescriptor> &descriptors,
    CancellationToken *cancellationToken)
{
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED);
    auto result = GetTodoCommentsImpl(context, descriptors, cancellationToken);
    initializer.DestroyContext(context);
    return result;
}

InlayHintList ProvideInlayHints(es2panda_Context *context, const TextSpan *span)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    const size_t defaultTime = 20;
    auto cancellationToken = CancellationToken(defaultTime, nullptr);
    UserPreferences preferences = UserPreferences::GetDefaultUserPreferences();
    preferences.SetIncludeInlayParameterNameHints(UserPreferences::IncludeInlayParameterNameHints::ALL);
    return ProvideInlayHintsImpl(context, span, cancellationToken, preferences);
}

SignatureHelpItems GetSignatureHelpItems(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    auto items = ark::es2panda::lsp::GetSignature(context, byteOffset);

    auto span = items.GetApplicableSpan();
    size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, span.start);
    size_t lengthCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, span.length);
    items.SetApplicableSpan(startCharOffset, lengthCharOffset);

    return items;
}
size_t GetOffsetByColAndLine(const std::string &sourceCode, size_t line, size_t column)
{
    auto index = lexer::LineIndex(util::StringView(sourceCode));
    size_t byteOffset = index.GetOffset(lexer::SourceLocation(line, column, nullptr));
    return ark::es2panda::lsp::ByteOffsetToCodePointOffset(sourceCode, byteOffset);
}

std::pair<size_t, size_t> GetColAndLineByOffset(const std::string &sourceCode, size_t offset)
{
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(sourceCode, offset);

    auto index = lexer::LineIndex(util::StringView(sourceCode));
    return index.GetLocation(byteOffset);
}

std::vector<CodeFixActionInfo> GetCodeFixesAtPosition(es2panda_Context *context, size_t startPosition,
                                                      size_t endPosition, std::vector<int> &errorCodes,
                                                      CodeFixOptions &codeFixOptions)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t startByteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, startPosition);
    size_t endByteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, endPosition);

    auto result = ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, startByteOffset, endByteOffset, errorCodes,
                                                                 codeFixOptions);
    for (auto &action : result) {
        for (auto &fileChange : action.changes_) {
            for (auto &textChange : fileChange.textChanges) {
                size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, textChange.span.start);
                size_t endCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(
                    source, textChange.span.start + textChange.span.length);
                textChange.span.start = startCharOffset;
                textChange.span.length = endCharOffset - startCharOffset;
            }
        }
    }
    return result;
}

CombinedCodeActionsInfo GetCombinedCodeFix(const char *fileName, const std::string &fixId,
                                           CodeFixOptions &codeFixOptions)
{
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED);
    auto result = ark::es2panda::lsp::GetCombinedCodeFixImpl(context, fixId, codeFixOptions);
    initializer.DestroyContext(context);
    return result;
}

TextSpan *GetNameOrDottedNameSpan(es2panda_Context *context, int startPos)
{
    auto result = ark::es2panda::lsp::GetNameOrDottedNameSpanImpl(context, startPos);
    return result.release();
}

es2panda_AstNode *GetProgramAst(es2panda_Context *context)
{
    return GetProgramAstImpl(context);
}

static bool IsProgramMatchedByFileName(const parser::Program *program, std::string_view fileName)
{
    if (program == nullptr || fileName.empty()) {
        return false;
    }
    return std::string_view(program->SourceFilePath().Utf8()) == fileName;
}

static parser::Program *GetProgramByFileName(public_lib::Context *ctx, std::string_view fileName)
{
    if (ctx->parserProgram == nullptr) {
        return nullptr;
    }

    if (IsProgramMatchedByFileName(ctx->parserProgram, fileName)) {
        return ctx->parserProgram;
    }

    const auto &externalSources = ctx->parserProgram->GetExternalPrograms()->Get<util::ModuleKind::SOURCE_DECL>();
    for (auto *extProgram : externalSources) {
        if (IsProgramMatchedByFileName(extProgram, fileName)) {
            return extProgram;
        }
    }
    return nullptr;
}

static bool IsNodeInfoSymbolKind(ir::AstNodeType kind)
{
    return kind == ir::AstNodeType::ANNOTATION_DECLARATION || kind == ir::AstNodeType::ANNOTATION_USAGE ||
           kind == ir::AstNodeType::AWAIT_EXPRESSION || kind == ir::AstNodeType::CALL_EXPRESSION ||
           kind == ir::AstNodeType::CLASS_DECLARATION || kind == ir::AstNodeType::CLASS_PROPERTY ||
           kind == ir::AstNodeType::FUNCTION_DECLARATION || kind == ir::AstNodeType::FUNCTION_EXPRESSION ||
           kind == ir::AstNodeType::IMPORT_DEFAULT_SPECIFIER || kind == ir::AstNodeType::IMPORT_NAMESPACE_SPECIFIER ||
           kind == ir::AstNodeType::IMPORT_SPECIFIER || kind == ir::AstNodeType::METHOD_DEFINITION ||
           kind == ir::AstNodeType::MEMBER_EXPRESSION || kind == ir::AstNodeType::PROPERTY ||
           kind == ir::AstNodeType::SPREAD_ELEMENT || kind == ir::AstNodeType::STRUCT_DECLARATION ||
           kind == ir::AstNodeType::TS_CLASS_IMPLEMENTS || kind == ir::AstNodeType::TS_ENUM_DECLARATION ||
           kind == ir::AstNodeType::TS_ENUM_MEMBER || kind == ir::AstNodeType::TS_INTERFACE_DECLARATION ||
           kind == ir::AstNodeType::TS_MODULE_DECLARATION || kind == ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION ||
           kind == ir::AstNodeType::VARIABLE_DECLARATOR;
}

static std::vector<NodeInfo> GetNodeInfosByDefinitionDataImpl(es2panda_Context *context, std::string_view fileName,
                                                              size_t position)
{
    if (context == nullptr) {
        return {};
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    auto *targetProgram = GetProgramByFileName(ctx, fileName);
    auto *node = GetTouchingTokenRightMatch(context, position, targetProgram);
    if (node == nullptr) {
        return {};
    }

    std::vector<NodeInfo> result;
    while (node != nullptr) {
        const auto &nodeInfoHandlers = GetNodeInfoHandlers();
        auto it = nodeInfoHandlers.find(node->Type());
        if (it != nodeInfoHandlers.end() && IsNodeInfoSymbolKind(node->Type())) {
            it->second(node, result);
        }
        node = node->Parent();
    }
    return std::vector<NodeInfo>(result.rbegin(), result.rend());
}

std::vector<NodeInfo> GetNodeInfosByDefinitionData(es2panda_Context *context, const char *fileName, size_t position)
{
    std::string_view fileNameView = fileName == nullptr ? std::string_view {} : std::string_view(fileName);
    return GetNodeInfosByDefinitionDataImpl(context, fileNameView, position);
}

es2panda_AstNode *GetClassDefinition(es2panda_AstNode *astNode, const std::string &nodeName)
{
    return GetClassDefinitionImpl(astNode, nodeName);
}

es2panda_AstNode *GetIdentifier(es2panda_AstNode *astNode, const std::string &nodeName)
{
    return GetIdentifierImpl(astNode, nodeName);
}

void GetNodeCharOffsets(const ir::AstNode *node, const public_lib::Context *ctx, size_t &startCharOffset,
                        size_t &lengthChar)
{
    const parser::Program *program = nullptr;
    size_t start = 0;
    size_t end = 0;
    if (node != nullptr) {
        program = node->Range().start.Program();
        start = node->Start().index;
        end = node->End().index;
    }
    std::string nodeSource =
        program != nullptr ? std::string(program->SourceCode()) : std::string(ctx->parserProgram->SourceCode());
    startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(nodeSource, start);
    lengthChar = ark::es2panda::lsp::ByteOffsetToCodePointOffset(nodeSource, end) - startCharOffset;
}

// Matches a single node info element within the given subtree.
static ir::AstNode *FindNodeMatchingInfo(ir::AstNode *searchRoot, const NodeInfo *infoToMatch)
{
    const auto &nodeMatchers = GetNodeMatchers();
    auto it = nodeMatchers.find(infoToMatch->kind);
    if (it == nodeMatchers.end()) {
        return nullptr;
    }
    return searchRoot->FindChild(
        [it, infoToMatch](ir::AstNode *childNode) { return it->second(childNode, infoToMatch); });
}

// Finds the node matching a single node info chain element within the given subtree.
// The matcher honours the node info's parameter count (overload disambiguation, also
// separating getter/setter pairs); when no overload matches the arity, the search falls
// back to the first same-name candidate, preserving the pre-disambiguation behaviour.
static ir::AstNode *FindNodeByNodeInfo(ir::AstNode *searchRoot, const NodeInfo *info)
{
    auto *foundNode = FindNodeMatchingInfo(searchRoot, info);
    if (foundNode == nullptr && info->paramNumber.has_value()) {
        NodeInfo relaxed {*info};
        relaxed.paramNumber.reset();
        foundNode = FindNodeMatchingInfo(searchRoot, &relaxed);
    }
    return foundNode;
}

// Export wrapper kinds that may appear in a node info chain. The static-language AST has
// no dedicated node for most export statements: a plain `export { x }` without a source
// becomes an empty statement, while a default export is the flagged declaration itself
// and keeps its declaration subtree. These chain elements are therefore advisory rather
// than structural.
static bool IsPlainExportWrapperKind(ir::AstNodeType kind)
{
    switch (kind) {
        case ir::AstNodeType::EXPORT_ALL_DECLARATION:
        case ir::AstNodeType::EXPORT_NAMED_DECLARATION:
        case ir::AstNodeType::EXPORT_SPECIFIER:
        case ir::AstNodeType::REEXPORT_STATEMENT:
            return true;
        default:
            return false;
    }
}

static bool IsExportWrapperNodeInfo(const NodeInfo *info)
{
    return IsPlainExportWrapperKind(info->kind) || info->kind == ir::AstNodeType::EXPORT_DEFAULT_DECLARATION;
}

static bool IsPlainExportWrapperNodeInfo(const NodeInfo *info)
{
    return IsPlainExportWrapperKind(info->kind);
}

static bool IsIdentifierChar(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '$';
}

// Finds the first whole-word occurrence of `name` in `source` within [from, to) that starts
// in live code (see BuildLiveCodeMask): commented-out or textually embedded occurrences are
// skipped, so a commented-out 'export { foo }' is not mistaken for the statement.
static size_t FindNameInRange(const std::string &source, const std::string &name, size_t from, size_t to,
                              const std::vector<bool> &live)
{
    auto pos = source.find(name, from);
    while (pos != std::string::npos && pos + name.size() <= to) {
        const bool leftOk = pos == 0 || !IsIdentifierChar(source[pos - 1]);
        const bool rightOk = pos + name.size() >= source.size() || !IsIdentifierChar(source[pos + name.size()]);
        // The match consists of identifier characters only, so it cannot reach across a
        // comment or string opening (a non-identifier character): checking the start
        // position against the mask covers the whole match.
        if (leftOk && rightOk && pos < live.size() && live[pos]) {
            return pos;
        }
        pos = source.find(name, pos + 1);
    }
    return std::string::npos;
}

static std::string GetNodeProgramSource(const ir::AstNode *node, const public_lib::Context *ctx)
{
    const auto *program = node != nullptr ? node->Range().start.Program() : nullptr;
    return std::string(program != nullptr ? program->SourceCode()
                                          : static_cast<std::string_view>(ctx->parserProgram->SourceCode()));
}

// A plain `export { x }` without a source collapses into an empty statement in the static
// language, but the statement range is preserved. The empty statement keeps no specifier
// children, so the exported name is matched against the statement source text (skipping
// comments and strings, see BuildLiveCodeMask); degraded ranges (for example a
// default-exported function) cannot carry a position and never match.
static bool MatchesPlainExportStatement(const ir::AstNode *node, const std::string &name,
                                        const public_lib::Context *ctx)
{
    if (node == nullptr || !node->IsEmptyStatement()) {
        return false;
    }
    const auto start = node->Start().index;
    const auto end = node->End().index;
    if (end <= start) {
        return false;
    }
    const std::string source = GetNodeProgramSource(node, ctx);
    const auto live = BuildLiveCodeMask(source);
    return FindNameInRange(source, "export", start, end, live) != std::string::npos &&
           FindNameInRange(source, name, start, end, live) != std::string::npos;
}

static ir::AstNode *FindPlainExportStatement(ir::AstNode *searchRoot, const NodeInfo *info,
                                             const public_lib::Context *ctx)
{
    return searchRoot->FindChild(
        [info, ctx](ir::AstNode *childNode) { return MatchesPlainExportStatement(childNode, info->name, ctx); });
}

// Skips whitespace and non-code (comments and string literals) starting at 'pos'.
static size_t SkipTrivia(const std::string &source, size_t pos, const std::vector<bool> &live)
{
    while (pos < source.size()) {
        if (std::isspace(static_cast<unsigned char>(source[pos])) != 0) {
            ++pos;
            continue;
        }
        if (pos < live.size() && !live[pos]) {
            ++pos;
            continue;
        }
        break;
    }
    return pos;
}

// The first brace that closes the specifier list in live code; braces inside comments or
// string literals are not statement structure.
static size_t FindClosingBrace(const std::string &source, size_t openBrace, const std::vector<bool> &live)
{
    for (size_t pos = openBrace + 1U; pos < source.size(); ++pos) {
        if (pos < live.size() && live[pos] && source[pos] == '}') {
            return pos;
        }
    }
    return std::string::npos;
}

// True when the export statement whose braces close at 'closeBrace' is a re-export
// ('export { x } from "..."'), which does not carry a local exported name.
static bool IsReExportStatement(const std::string &source, size_t closeBrace, const std::vector<bool> &live)
{
    constexpr std::string_view FROM_KEYWORD = "from";
    const auto afterBrace = SkipTrivia(source, closeBrace + 1U, live);
    const bool isFrom = source.compare(afterBrace, FROM_KEYWORD.size(), FROM_KEYWORD) == 0;
    const auto fromEnd = afterBrace + FROM_KEYWORD.size();
    return isFrom && (fromEnd >= source.size() || !IsIdentifierChar(source[fromEnd])) && afterBrace < live.size() &&
           live[afterBrace];
}

// Finds the exported name inside the braces of a plain export statement, or nullopt.
static std::optional<size_t> FindNameInExportBraces(const std::string &source, const std::string &name,
                                                    size_t openBrace, const std::vector<bool> &live)
{
    const auto closeBrace = FindClosingBrace(source, openBrace, live);
    if (closeBrace == std::string::npos || IsReExportStatement(source, closeBrace, live)) {
        return std::nullopt;
    }
    const auto namePos = FindNameInRange(source, name, openBrace + 1U, closeBrace, live);
    if (namePos == std::string::npos) {
        return std::nullopt;
    }
    return namePos;
}

// Finds the name inside a plain export statement ('export { name }' without a 'from' clause)
// in the program source. On a clean compile the global-class lowering removes the statement
// from the AST entirely - only a build with errors keeps a range-preserving empty statement
// (see MatchesPlainExportStatement) - so when no node matches, the position is recovered
// from the statement text alone. The scan is lexer-aware through the live-code mask: a
// commented-out or textually embedded 'export { name }' is not mistaken for the statement.
static size_t FindNameInPlainExportStatement(const std::string &source, const std::string &name)
{
    constexpr std::string_view EXPORT_KEYWORD = "export";
    const auto live = BuildLiveCodeMask(source);
    auto exportPos = FindNameInRange(source, std::string(EXPORT_KEYWORD), 0U, source.size(), live);
    while (exportPos != std::string::npos) {
        const auto openBrace = SkipTrivia(source, exportPos + EXPORT_KEYWORD.size(), live);
        if (openBrace < source.size() && source[openBrace] == '{' && live[openBrace]) {
            if (auto namePos = FindNameInExportBraces(source, name, openBrace, live); namePos.has_value()) {
                return *namePos;
            }
        }
        exportPos = FindNameInRange(source, std::string(EXPORT_KEYWORD), exportPos + 1U, source.size(), live);
    }
    return std::string::npos;
}

// Resolves the reported (start, length) for the matched node. A plain export statement
// match (empty statement) has no identifier child, so the position points at the exported
// name inside the statement, consistent with the re-export specifier results. A null node
// is the text anchor of a plain export statement that a clean compile removed from the AST;
// its position is recovered from the source text the same way.
static void GetReportedSymbolOffsets(ir::AstNode *node, const NodeInfo *info, const public_lib::Context *ctx,
                                     size_t &startCharOffset, size_t &lengthChar)
{
    if (node == nullptr) {
        const std::string source = GetNodeProgramSource(nullptr, ctx);
        const auto namePos = FindNameInPlainExportStatement(source, info->name);
        if (namePos != std::string::npos) {
            startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, namePos);
            lengthChar =
                ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, namePos + info->name.size()) - startCharOffset;
        }
        return;
    }
    if (node->IsEmptyStatement()) {
        const std::string source = GetNodeProgramSource(node, ctx);
        const auto namePos =
            FindNameInRange(source, info->name, node->Start().index, node->End().index, BuildLiveCodeMask(source));
        if (namePos != std::string::npos) {
            startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, namePos);
            lengthChar =
                ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, namePos + info->name.size()) - startCharOffset;
            return;
        }
    }
    ir::AstNode *identifierNode = ExtractIdentifierFromNode(node, info);
    ir::AstNode *target = identifierNode != nullptr ? identifierNode : node;
    GetNodeCharOffsets(target, ctx, startCharOffset, lengthChar);
}

// Resolves a node info chain to the matched node and its info: the entry point shared
// by the definition and rename queries. The chain is ordered outermost first (the
// extractor walks up from the definition and reverses); every info is matched within the
// subtree of the previous match so same-name members of sibling scopes cannot shadow each
// other. Returns nullopt when a non-export element of the chain matches nothing. A null node
// with a non-null info is the text anchor of a plain export statement that matched no AST
// node (removed by a clean compile).
struct NodeInfoChainMatch {
    ir::AstNode *node = nullptr;
    NodeInfo *info = nullptr;
};
static std::optional<NodeInfoChainMatch> ResolveNodeInfoChain(es2panda_Context *context,
                                                              const std::vector<NodeInfo *> &nodeInfos)
{
    if (context == nullptr || nodeInfos.empty()) {
        return std::nullopt;
    }
    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    auto *rootNode = reinterpret_cast<ir::AstNode *>(ctx->parserProgram->Ast());
    if (rootNode == nullptr) {
        return std::nullopt;
    }

    ir::AstNode *lastFoundNode = nullptr;
    NodeInfo *lastNodeInfo = nullptr;
    NodeInfo *plainExportAnchor = nullptr;
    ir::AstNode *searchRoot = rootNode;
    for (auto info : nodeInfos) {
        auto foundNode = FindNodeByNodeInfo(searchRoot, info);
        if (foundNode == nullptr && IsPlainExportWrapperNodeInfo(info)) {
            foundNode = FindPlainExportStatement(searchRoot, info, ctx);
        }
        if (foundNode == nullptr) {
            // Workaround for the missing export statement nodes in the static language:
            // an export wrapper element that matches nothing is skipped and the search
            // continues from the current root, so the chain still resolves through its
            // declaration elements.
            if (IsExportWrapperNodeInfo(info)) {
                // A brace-form export statement that matched no node is remembered as the
                // anchor of last resort: a clean compile removes the statement from the
                // AST, and its position is then recovered from the source text.
                if (info->kind == ir::AstNodeType::EXPORT_NAMED_DECLARATION ||
                    info->kind == ir::AstNodeType::EXPORT_SPECIFIER) {
                    plainExportAnchor = info;
                }
                continue;
            }
            return std::nullopt;
        }
        lastFoundNode = foundNode;
        lastNodeInfo = info;
        // Export statements are wrappers without a declaration subtree: keep the outer
        // root so the remaining chain elements resolve their declaration, which then
        // takes precedence over the statement anchor. A default export is the flagged
        // declaration itself and keeps the normal scoped descent.
        if (!IsPlainExportWrapperNodeInfo(info)) {
            searchRoot = foundNode;
        }
    }
    if (lastFoundNode == nullptr || lastNodeInfo == nullptr) {
        if (plainExportAnchor != nullptr) {
            return NodeInfoChainMatch {nullptr, plainExportAnchor};
        }
        return std::nullopt;
    }
    return NodeInfoChainMatch {lastFoundNode, lastNodeInfo};
}

DefinitionInfo GetDefinitionDataFromNode(es2panda_Context *context, const std::vector<NodeInfo *> &nodeInfos)
{
    DefinitionInfo result {"", 0, 0};
    auto match = ResolveNodeInfoChain(context, nodeInfos);
    if (!match.has_value()) {
        return result;
    }
    size_t startCharOffset = 0;
    size_t lengthChar = 0;
    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    GetReportedSymbolOffsets(match->node, match->info, ctx, startCharOffset, lengthChar);
    result = {"", startCharOffset, lengthChar};
    return result;
}

ark::es2panda::lsp::RenameLocation FindRenameLocationsFromNode(es2panda_Context *context,
                                                               const std::vector<NodeInfo *> &nodeInfos)
{
    ark::es2panda::lsp::RenameLocation result {"", 0, 0, 0};
    auto match = ResolveNodeInfoChain(context, nodeInfos);
    if (!match.has_value()) {
        return result;
    }
    size_t startCharOffset = 0;
    size_t lengthChar = 0;
    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    GetReportedSymbolOffsets(match->node, match->info, ctx, startCharOffset, lengthChar);
    result = {"", startCharOffset, startCharOffset + lengthChar, lengthChar};
    return result;
}

TokenTypeInfo GetTokenTypes(es2panda_Context *context, size_t offset)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, offset);

    auto token = GetTouchingToken(context, byteOffset, false);
    std::string result;
    std::string name;
    ir::ModifierFlags flags;
    if (token != nullptr && token->IsIdentifier()) {
        name = std::string(token->AsIdentifier()->Name());
        token = compiler::DeclarationFromIdentifier(token->AsIdentifier());
        if (token != nullptr) {
            flags = token->Modifiers();
            result = GetTokenTypes(flags);
        }
    }
    return {name, result};
}

std::vector<TextChange> GetFormattingEditsForDocument(es2panda_Context *context, FormatCodeSettings &options)
{
    if (context == nullptr) {
        return {};
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);

    FormatContext formatContext = GetFormatContext(options);
    return FormatDocument(context, formatContext);
}

std::vector<TextChange> GetFormattingEditsForRange(es2panda_Context *context, FormatCodeSettings &options,
                                                   const TextSpan &span)
{
    if (context == nullptr) {
        return {};
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);

    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t startByte = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, span.start);
    size_t endByte = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, span.start + span.length);
    TextSpan byteSpan(startByte, endByte - startByte);

    FormatContext formatContext = GetFormatContext(options);
    auto result = FormatRange(context, formatContext, byteSpan);

    for (auto &change : result) {
        size_t startChar = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, change.span.start);
        size_t endChar =
            ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, change.span.start + change.span.length);
        change.span.start = startChar;
        change.span.length = endChar - startChar;
    }
    return result;
}

std::vector<TextChange> GetFormattingEditsAfterKeystroke(es2panda_Context *context, FormatCodeSettings &options,
                                                         char key, const TextSpan &span)
{
    if (context == nullptr) {
        return {};
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);

    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t startByte = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, span.start);
    size_t endByte = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, span.start + span.length);
    TextSpan byteSpan(startByte, endByte - startByte);

    FormatContext formatContext = GetFormatContext(options);
    auto result = FormatAfterKeystroke(context, formatContext, key, byteSpan);

    for (auto &change : result) {
        size_t startChar = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, change.span.start);
        size_t endChar =
            ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, change.span.start + change.span.length);
        change.span.start = startChar;
        change.span.length = endChar - startChar;
    }
    return result;
}

static int BatchDeleteProgramsForFiles(public_lib::Context *ctx,
                                       const std::unordered_set<std::string> &uniqueDeleteNames)
{
    if (uniqueDeleteNames.empty()) {
        return 0;
    }

    auto *importPathManager = ctx->parser->GetImportPathManager();
    for (const auto &file : uniqueDeleteNames) {
        if (file.empty()) {
            continue;
        }
        ArenaString key {file};
        importPathManager->RemoveProgramFromResolvedSources(key);
    }
    importPathManager->RemoveProgramsFromFileDependencies(uniqueDeleteNames);
    importPathManager->ClearResolutionCaches();
    return 0;
}

int DeleteProgramForFile(es2panda_Context *context, const char *fileName)
{
    if (fileName == nullptr) {
        return 0;
    }

    std::unordered_set<std::string> filesToDelete {};
    filesToDelete.emplace(fileName);
    return BatchDeleteProgramsForFiles(reinterpret_cast<public_lib::Context *>(context), filesToDelete);
}

int DeleteDependantProgramsForFiles(es2panda_Context *context, const char *fileName)
{
    if (fileName == nullptr) {
        return 0;
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    return BatchDeleteProgramsForFiles(ctx, ctx->parser->GetImportPathManager()->CollectReverseDependencies(fileName));
}

bool CollectApiInfoWrapper(es2panda_Context *context)
{
    if (context == nullptr) {
        return false;
    }
    return ark::es2panda::lsp::CollectApiCompletionInfo(context);
}

LSPAPI g_lspImpl = {GetDefinitionAtPosition,
                    GetApplicableRefactors,
                    GetEditsForRefactor,
                    GetImplementationAtPosition,
                    IsPackageModule,
                    GetAliasScriptElementKind,
                    GetFileReferences,
                    GetDeclInfo,
                    GetClassHierarchies,
                    GetSafeDeleteInfo,
                    GetReferencesAtPosition,
                    GetPrecedingToken,
                    GetCurrentTokenValue,
                    OrganizeImportsImpl,
                    GetQuickInfoAtPosition,
                    GetCompletionEntryDetails,
                    GetSpanOfEnclosingComment,
                    GetSemanticDiagnostics,
                    GetSyntacticDiagnostics,
                    GetCompilerOptionsDiagnostics,
                    GetTypeHierarchies,
                    GetDocumentHighlights,
                    FindRenameLocationsWrapper,
                    FindRenameLocationsInCurrentFileWrapper,
                    NeedsCrossFileRenameWrapper,
                    FindRenameLocationsWithCancellationWrapper,
                    FindSafeDeleteLocation,
                    FindReferencesWrapper,
                    GetRenameInfoWrapper,
                    GetClassPropertyInfoWrapper,
                    GetSuggestionDiagnostics,
                    GetCompletionsAtPosition,
                    GetClassHierarchyInfo,
                    GetBraceMatchingAtPositionWrapper,
                    GetClassConstructorInfo,
                    GetImplementationLocationAtPositionWrapper,
                    ToLineColumnOffsetWrapper,
                    GetTodoComments,
                    ProvideInlayHints,
                    GetSignatureHelpItems,
                    GetOffsetByColAndLine,
                    GetColAndLineByOffset,
                    GetCodeFixesAtPosition,
                    GetCombinedCodeFix,
                    GetNameOrDottedNameSpan,
                    GetProgramAst,
                    GetNodeInfosByDefinitionData,
                    GetClassDefinition,
                    GetIdentifier,
                    GetDefinitionDataFromNode,
                    FindRenameLocationsFromNode,
                    GetTokenTypes,
                    GetFormattingEditsForDocument,
                    GetFormattingEditsForRange,
                    GetFormattingEditsAfterKeystroke,
                    GetFormatContext,
                    GetDefaultFormatCodeSettings,
                    InitSymbolReferenceIndexWrapper,
                    ClearSymbolReferenceIndexWrapper,
                    BuildSymbolReferenceIndexForContextWrapper,
                    BuildSymbolReferenceIndexForContextWithExternalWrapper,
                    RemoveSymbolReferenceIndexForFileWrapper,
                    GetReferencesAtPositionFromIndexWrapper,
                    GetIndexedFileSourceWrapper,
                    DeleteProgramForFile,
                    DeleteDependantProgramsForFiles,
                    CollectApiInfoWrapper,
                    ResetEHeapWrapper};
}  // namespace ark::es2panda::lsp

CAPI_EXPORT LSPAPI const *GetImpl()
{
    return &ark::es2panda::lsp::g_lspImpl;
}

// NOTE: Capability detection symbol exported for dynamic loader (dlsym).
// When bindings dynamically load this library, they query HasLspCapability("resetEHeap")
// before accessing extended slots in struct LSPAPI.
// Capability names and new LSPAPI struct fields must be released in the same version.
CAPI_EXPORT bool HasLspCapability(const char *capabilityName)
{
    if (capabilityName == nullptr) {
        return false;
    }
    if (std::strcmp(capabilityName, "resetEHeap") == 0) {
        return true;
    }
    return false;
}
}
