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

using ark::es2panda::lsp::details::GetCompletionEntryDetailsImpl;

extern "C" {
namespace ark::es2panda::lsp {

DefinitionInfo GetDefinitionAtPosition(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    auto importFilePath = GetImportFilePath(context, position);
    if (!importFilePath.empty()) {
        return {importFilePath, 0, 0};
    }
    auto declInfo = GetDefinitionAtPositionImpl(context, position);
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
        result = {name, targetNode->Start().index, targetNode->End().index - targetNode->Start().index};
    }
    return result;
}

DefinitionInfo GetImplementationAtPosition(es2panda_Context *context, size_t position)
{
    return GetDefinitionAtPosition(context, position);
}

bool IsPackageModule(es2panda_Context *context)
{
    return reinterpret_cast<public_lib::Context *>(context)->parserProgram->IsPackage();
}

CompletionEntryKind GetAliasScriptElementKind(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    auto result = GetAliasScriptElementKindImpl(context, position);
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
    auto astNode = GetTouchingToken(context, position, false);
    auto declInfo = GetDeclInfoImpl(astNode);
    result.fileName = std::get<0>(declInfo);
    result.fileText = std::get<1>(declInfo);
    return result;
}

std::vector<ClassHierarchyItemInfo> GetClassHierarchies(std::vector<es2panda_Context *> *contextList,
                                                        const char *fileName, size_t pos)
{
    auto *ctxList = reinterpret_cast<std::vector<es2panda_Context *> *>(contextList);
    for (auto *context : *ctxList) {
        auto ctx = reinterpret_cast<public_lib::Context *>(context);
        SetPhaseManager(ctx->phaseManager);
    }
    return GetClassHierarchiesImpl(contextList, std::string(fileName), pos);
}

bool GetSafeDeleteInfo(es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    return GetSafeDeleteInfoImpl(context, position);
}

References GetReferencesAtPosition(es2panda_Context *context, DeclInfo *declInfo)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    auto result = GetReferencesAtPositionImpl(context, {declInfo->fileName, declInfo->fileText});
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
    auto result = GetCurrentTokenValueImpl(context, position);
    return result;
}

std::vector<FileTextChanges> OrganizeImportsImpl(es2panda_Context *context, char const *fileName)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    auto result = OrganizeImports::Organize(context, fileName);
    return result;
}

QuickInfo GetQuickInfoAtPosition(const char *fileName, es2panda_Context *context, size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    auto res = GetQuickInfoAtPositionImpl(context, position, fileName);
    return res;
}

// find the Definition node by using the entryname And return CompletionEntryDetails
CompletionEntryDetails GetCompletionEntryDetails(const char *entryName, const char *fileName, es2panda_Context *context,
                                                 size_t position)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    auto result = GetCompletionEntryDetailsImpl(context, position, fileName, entryName);
    return result;
}

TextSpan GetSpanOfEnclosingComment(es2panda_Context *context, size_t pos, bool onlyMultiLine)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    SetPhaseManager(ctx->phaseManager);
    auto *range = ctx->allocator->New<CommentRange>();
    GetRangeOfEnclosingComment(context, pos, range);
    return (range != nullptr) && (!onlyMultiLine || range->kind_ == CommentKind::MULTI_LINE)
               ? TextSpan(range->pos_, range->end_ - range->pos_)
               : TextSpan(0, 0);
}

DiagnosticReferences GetSemanticDiagnostics(es2panda_Context *context)
{
    DiagnosticReferences result {};
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    ctx->diagnosticEngine->CleanDuplicateLog(util::DiagnosticType::SEMANTIC);
    SetPhaseManager(ctx->phaseManager);
    const auto &diagnostics = ctx->diagnosticEngine->GetDiagnosticStorage(util::DiagnosticType::SEMANTIC);
    for (const auto &diagnostic : diagnostics) {
        result.diagnostic.push_back(CreateDiagnosticForError(context, *diagnostic));
    }
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
    for (const auto &diagnostic : diagnostics) {
        result.diagnostic.push_back(CreateDiagnosticForError(context, *diagnostic));
    }
    for (const auto &diagnostic : diagnosticsPluginError) {
        result.diagnostic.push_back(CreateDiagnosticForError(context, *diagnostic));
    }
    for (const auto &diagnostic : diagnosticsPluginWarning) {
        result.diagnostic.push_back(CreateDiagnosticForError(context, *diagnostic));
    }
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
    auto declaration = GetTargetDeclarationNodeByPosition(context, pos);
    return GetTypeHierarchiesImpl(searchContext, pos, declaration);
}

DocumentHighlightsReferences GetDocumentHighlights(es2panda_Context *context, size_t position)
{
    DocumentHighlightsReferences result = {};
    result.documentHighlights_.push_back(GetDocumentHighlightsImpl(context, position));
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
    return GetRenameInfo(context, pos, std::string(pandaLibPath));
}

std::vector<TextSpan> GetBraceMatchingAtPositionWrapper(char const *fileName, size_t position)
{
    Initializer initializer = Initializer();
    auto context = initializer.CreateContext(fileName, ES2PANDA_STATE_CHECKED);
    auto result = GetBraceMatchingAtPosition(context, position);
    initializer.DestroyContext(context);
    return result;
}

std::vector<ark::es2panda::lsp::RenameLocation> FindRenameLocationsWrapper(
    const std::vector<es2panda_Context *> &fileContexts, es2panda_Context *context, size_t position)
{
    auto locations = FindRenameLocations(fileContexts, context, position);
    return std::vector<ark::es2panda::lsp::RenameLocation> {locations.begin(), locations.end()};
}

std::set<RenameLocation> FindRenameLocationsInCurrentFileWrapper(es2panda_Context *context, size_t position)
{
    return FindRenameLocationsInCurrentFile(context, position);
}

bool NeedsCrossFileRenameWrapper(es2panda_Context *context, size_t position)
{
    return NeedsCrossFileRename(context, position);
}

std::vector<ark::es2panda::lsp::RenameLocation> FindRenameLocationsWithCancellationWrapper(
    ark::es2panda::lsp::CancellationToken *tkn, const std::vector<es2panda_Context *> &fileContexts,
    es2panda_Context *context, size_t position)
{
    auto locations = FindRenameLocations(tkn, fileContexts, context, position);
    std::vector<ark::es2panda::lsp::RenameLocation> res(locations.size());
    for (const auto &entry : locations) {
        res.emplace_back(entry);
    }
    return res;
}

std::vector<FieldsInfo> GetClassPropertyInfoWrapper(es2panda_Context *context, size_t position,
                                                    bool shouldCollectInherited)
{
    return GetClassPropertyInfo(context, position, shouldCollectInherited);
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
    auto result = CompletionInfo(GetCompletionsAtPositionImpl(context, position));
    return result;
}

ClassHierarchy GetClassHierarchyInfo(es2panda_Context *context, size_t position)
{
    auto result = GetClassHierarchyInfoImpl(context, position);
    return result;
}

std::vector<Location> GetImplementationLocationAtPositionWrapper(es2panda_Context *context, int position)
{
    return GetImplementationLocationAtPosition(context, position);
}

RefactorEditInfo GetClassConstructorInfo(es2panda_Context *context, size_t position,
                                         const std::vector<std::string> &properties)
{
    auto result = RefactorEditInfo(GetRefactorActionsToGenerateConstructor(context, position, properties));
    return result;
}

LineAndCharacter ToLineColumnOffsetWrapper(es2panda_Context *context, size_t position)
{
    auto result = ToLineColumnOffset(context, position);
    return result;
}

// Returns type of refactoring and action that can be performed based
// on the input kind information and cursor position
std::vector<ApplicableRefactorInfo> GetApplicableRefactors(es2panda_Context *context, const char *kind, size_t position)
{
    RefactorContext refactorContext;
    refactorContext.context = context;
    refactorContext.kind = kind;
    refactorContext.span.pos = position;
    auto result = GetApplicableRefactorsImpl(&refactorContext);
    return result;
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
    return ark::es2panda::lsp::GetSignature(context, position);
}

size_t GetOffsetByColAndLine(const std::string &sourceCode, size_t line, size_t column)
{
    auto index = lexer::LineIndex(util::StringView(sourceCode));
    return index.GetOffset(lexer::SourceLocation(line, column, nullptr));
}

std::pair<size_t, size_t> GetColAndLineByOffset(const std::string &sourceCode, size_t offset)
{
    auto index = lexer::LineIndex(util::StringView(sourceCode));
    return index.GetLocation(offset);
}

std::vector<CodeFixActionInfo> GetCodeFixesAtPosition(es2panda_Context *context, size_t startPosition,
                                                      size_t endPosition, std::vector<int> &errorCodes,
                                                      CodeFixOptions &codeFixOptions)
{
    auto result =
        ark::es2panda::lsp::GetCodeFixesAtPositionImpl(context, startPosition, endPosition, errorCodes, codeFixOptions);
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
        switch (node->Type()) {
            case ir::AstNodeType::IDENTIFIER:
                result.emplace_back(std::string(node->AsIdentifier()->Name()), ir::AstNodeType::IDENTIFIER);
                break;
            case ir::AstNodeType::CLASS_DEFINITION:
                if (auto ident = node->AsClassDefinition()->Ident()) {
                    result.emplace_back(std::string(ident->Name()), ir::AstNodeType::CLASS_DEFINITION);
                }
                break;
            default:
                break;
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

DefinitionInfo GetDefinitionDataFromNode(es2panda_Context *context, const std::vector<NodeInfo *> nodeInfos)
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
        if (identifierNode != nullptr) {
            result = {"", identifierNode->Start().index, identifierNode->End().index - identifierNode->Start().index};
        } else {
            result = {"", lastFoundNode->Start().index, lastFoundNode->End().index - lastFoundNode->Start().index};
        }
    }

    return result;
}

LSPAPI g_lspImpl = {GetDefinitionAtPosition,
                    GetApplicableRefactors,
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
                    GetDefinitionDataFromNode};
}  // namespace ark::es2panda::lsp

CAPI_EXPORT LSPAPI const *GetImpl()
{
    return &ark::es2panda::lsp::g_lspImpl;
}
}
