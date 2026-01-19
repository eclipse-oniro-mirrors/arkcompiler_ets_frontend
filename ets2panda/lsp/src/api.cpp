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
#include <cstddef>
#include <string>
#include <vector>
#include "class_hierarchy.h"
#include "get_node.h"
#include "lsp/include/organize_imports.h"
#include "get_safe_delete_info.h"
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

using ark::es2panda::lsp::details::GetCompletionEntryDetailsImpl;

extern "C" {
namespace ark::es2panda::lsp {

DefinitionInfo GetDefinitionAtPosition(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);

    std::string source = std::string(ctx->parserProgram->SourceCode());

    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);

    auto importFilePath = GetImportFilePath(context, byteOffset);
    if (!importFilePath.empty()) {
        return {importFilePath, 0, 0};
    }
    auto declInfo = GetDefinitionAtPositionImpl(context, byteOffset);
    DefinitionInfo result {};
    if (declInfo.first == nullptr) {
        return result;
    }
    auto node = declInfo.first;
    auto targetNode = declInfo.first->FindChild([&declInfo](ir::AstNode *childNode) {
        return childNode->IsIdentifier() && childNode->AsIdentifier()->Name() == declInfo.second;
    });
    std::string name;
    while (node != nullptr) {
        if (node->Range().start.Program() != nullptr) {
            name = std::string(node->Range().start.Program()->SourceFile().GetAbsolutePath().Utf8());
            break;
        }
        if (node->IsETSModule()) {
            name = std::string(node->AsETSModule()->Program()->SourceFilePath());
            break;
        }
        node = node->Parent();
    }
    if (targetNode != nullptr) {
        std::string targetSource;
        if (targetNode->Range().start.Program() != nullptr) {
            targetSource = std::string(targetNode->Range().start.Program()->SourceCode());
        } else {
            targetSource = source;
        }
        size_t startCharOffset =
            ark::es2panda::lsp::ByteOffsetToCodePointOffset(targetSource, targetNode->Start().index);
        size_t lengthChar =
            ark::es2panda::lsp::ByteOffsetToCodePointOffset(targetSource, targetNode->End().index) - startCharOffset;
        result = {name, startCharOffset, lengthChar};
    }
    return result;
}

DefinitionInfo GetImplementationAtPosition(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    std::string source = std::string(ctx->parserProgram->SourceCode());

    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);

    DefinitionInfo result = GetDefinitionAtPosition(context, byteOffset);

    size_t startCharOffset = ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, result.start);
    size_t lengthChar =
        ark::es2panda::lsp::ByteOffsetToCodePointOffset(source, result.start + result.length) - startCharOffset;

    return {result.fileName, startCharOffset, lengthChar};
}

bool IsPackageModule(es2panda_Context *context)
{
    return reinterpret_cast<public_lib::Context *>(context)->parserProgram->IsPackage();
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

References GetFileReferences(char const *fileName, es2panda_Context *context, bool isPackageModule)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    return GetFileReferencesImpl(context, fileName, isPackageModule);
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

es2panda_AstNode *GetPrecedingToken(es2panda_Context *context, const size_t pos)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    auto ast = ctx->parserProgram->Ast();
    return reinterpret_cast<es2panda_AstNode *>(FindPrecedingToken(pos, ast, ctx->allocator));
}

std::string GetCurrentTokenValue(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);

    auto result = GetCurrentTokenValueImpl(context, byteOffset);
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
    SetPhaseManager(ctx->phaseManager);
    const auto &diagnostics = ctx->diagnosticEngine->GetDiagnosticStorage(util::DiagnosticType::SEMANTIC);
    MakeDiagnosticReferences(context, diagnostics, result);
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
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, pos);
    auto declaration = GetTargetDeclarationNodeByPosition(context, byteOffset);
    return GetTypeHierarchiesImpl(searchContext, byteOffset, declaration);
}

DocumentHighlightsReferences GetDocumentHighlights(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
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

std::vector<SafeDeleteLocation> FindSafeDeleteLocation(es2panda_Context *ctx,
                                                       const std::tuple<std::string, std::string> *declInfo)
{
    std::vector<SafeDeleteLocation> result;
    if (declInfo == nullptr) {
        return result;
    }
    result = FindSafeDeleteLocationImpl(ctx, *declInfo);
    return result;
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
    return NeedsCrossFileRename(context, position);
}

std::vector<ark::es2panda::lsp::RenameLocation> FindRenameLocationsWithCancellationWrapper(
    ark::es2panda::lsp::CancellationToken *tkn, const std::vector<es2panda_Context *> &fileContexts,
    es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
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
    std::string source = std::string(ctx->parserProgram->SourceCode());
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, position);
    auto result = GetClassHierarchyInfoImpl(context, byteOffset);
    return result;
}

std::vector<Location> GetImplementationLocationAtPositionWrapper(es2panda_Context *context, int position)
{
    return GetImplementationLocationAtPosition(context, position);
}

RefactorEditInfo GetClassConstructorInfo(es2panda_Context *context, size_t position,
                                         const std::vector<std::string> &properties)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
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
    std::string source = std::string(ctx->parserProgram->SourceCode());

    RefactorContext newContext = context;
    newContext.span.pos = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, context.span.pos);
    newContext.span.end = ark::es2panda::lsp::CodePointOffsetToByteOffset(source, context.span.end);

    return ark::es2panda::lsp::GetEditsForRefactorsImpl(newContext, refactorName, actionName);
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
    const size_t defaultTime = 20;
    auto cancellationToken = CancellationToken(defaultTime, nullptr);
    UserPreferences preferences = UserPreferences::GetDefaultUserPreferences();
    preferences.SetIncludeInlayParameterNameHints(UserPreferences::IncludeInlayParameterNameHints::ALL);
    return ProvideInlayHintsImpl(context, span, cancellationToken, preferences);
}

SignatureHelpItems GetSignatureHelpItems(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
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
    size_t byteColumn = ark::es2panda::lsp::CodePointOffsetToByteOffset(sourceCode, column);
    auto index = lexer::LineIndex(util::StringView(sourceCode));
    size_t byteOffset = index.GetOffset(lexer::SourceLocation(line, byteColumn, nullptr));
    return ark::es2panda::lsp::ByteOffsetToCodePointOffset(sourceCode, byteOffset);
}

std::pair<size_t, size_t> GetColAndLineByOffset(const std::string &sourceCode, size_t offset)
{
    size_t byteOffset = ark::es2panda::lsp::CodePointOffsetToByteOffset(sourceCode, offset);

    auto index = lexer::LineIndex(util::StringView(sourceCode));
    auto [line, byteColumn] = index.GetLocation(byteOffset);

    size_t charColumn = ark::es2panda::lsp::ByteOffsetToCodePointOffset(sourceCode, byteColumn);

    return {line, charColumn};
}

std::vector<CodeFixActionInfo> GetCodeFixesAtPosition(es2panda_Context *context, size_t startPosition,
                                                      size_t endPosition, std::vector<int> &errorCodes,
                                                      CodeFixOptions &codeFixOptions)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
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
    return result;
}

es2panda_AstNode *GetProgramAst(es2panda_Context *context)
{
    return GetProgramAstImpl(context);
}

std::vector<NodeInfo> GetNodeInfosByDefinitionData(es2panda_Context *context, size_t position)
{
    if (context == nullptr) {
        return {};
    }

    auto node = GetTouchingToken(context, position, false);
    if (node == nullptr) {
        return {};
    }

    std::vector<NodeInfo> result;
    while (node != nullptr) {
        const auto &nodeInfoHandlers = GetNodeInfoHandlers();
        auto it = nodeInfoHandlers.find(node->Type());
        if (it != nodeInfoHandlers.end()) {
            it->second(node, result);
        }
        node = node->Parent();
    }
    return std::vector<NodeInfo>(result.rbegin(), result.rend());
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

DefinitionInfo GetDefinitionDataFromNode(es2panda_Context *context, const std::vector<NodeInfo *> &nodeInfos)
{
    DefinitionInfo result {"", 0, 0};
    if (context == nullptr || nodeInfos.empty()) {
        return result;
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    auto rootNode = reinterpret_cast<ir::AstNode *>(ctx->parserProgram->Ast());
    if (rootNode == nullptr) {
        return result;
    }

    ir::AstNode *lastFoundNode = nullptr;
    NodeInfo *lastNodeInfo = nullptr;
    for (auto info : nodeInfos) {
        auto foundNode = rootNode->FindChild([info](ir::AstNode *childNode) -> bool {
            const auto &nodeMatchers = GetNodeMatchers();
            auto it = nodeMatchers.find(info->kind);
            if (it != nodeMatchers.end()) {
                return it->second(childNode, info);
            }
            return false;
        });
        if (foundNode == nullptr) {
            return {"", 0, 0};
        }
        lastFoundNode = foundNode;
        lastNodeInfo = info;
    }

    if (lastFoundNode != nullptr && lastNodeInfo != nullptr) {
        ir::AstNode *identifierNode = ExtractIdentifierFromNode(lastFoundNode, lastNodeInfo);
        size_t startCharOffset = 0;
        size_t lengthChar = 0;
        ir::AstNode *target = identifierNode ? identifierNode : lastFoundNode;
        GetNodeCharOffsets(target, ctx, startCharOffset, lengthChar);
        result = {"", startCharOffset, lengthChar};
    }

    return result;
}

ark::es2panda::lsp::RenameLocation FindRenameLocationsFromNode(es2panda_Context *context,
                                                               const std::vector<NodeInfo *> &nodeInfos)
{
    ark::es2panda::lsp::RenameLocation result {"", 0, 0, 0};
    if (context == nullptr || nodeInfos.empty()) {
        return result;
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    auto rootNode = reinterpret_cast<ir::AstNode *>(ctx->parserProgram->Ast());
    if (rootNode == nullptr) {
        return result;
    }

    ir::AstNode *lastFoundNode = nullptr;
    NodeInfo *lastNodeInfo = nullptr;
    for (auto info : nodeInfos) {
        auto foundNode = rootNode->FindChild([info](ir::AstNode *childNode) -> bool {
            const auto &nodeMatchers = GetNodeMatchers();
            auto it = nodeMatchers.find(info->kind);
            if (it != nodeMatchers.end()) {
                return it->second(childNode, info);
            }
            return false;
        });
        if (foundNode == nullptr) {
            return {"", 0, 0, 0};
        }
        lastFoundNode = foundNode;
        lastNodeInfo = info;
    }

    if (lastFoundNode != nullptr && lastNodeInfo != nullptr) {
        ir::AstNode *identifierNode = ExtractIdentifierFromNode(lastFoundNode, lastNodeInfo);
        if (identifierNode != nullptr) {
            result = {"", identifierNode->Start().index, identifierNode->End().index,
                      identifierNode->End().index - identifierNode->Start().index};
        } else {
            result = {"", lastFoundNode->Start().index, lastFoundNode->End().index,
                      lastFoundNode->End().index - lastFoundNode->Start().index};
        }
    }

    return result;
}

TokenTypeInfo GetTokenTypes(es2panda_Context *context, size_t offset)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
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
                    GetDefaultFormatCodeSettings};
}  // namespace ark::es2panda::lsp

CAPI_EXPORT LSPAPI const *GetImpl()
{
    return &ark::es2panda::lsp::g_lspImpl;
}
}
