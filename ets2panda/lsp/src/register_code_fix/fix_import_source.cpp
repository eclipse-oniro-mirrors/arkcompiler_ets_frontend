/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "lsp/include/register_code_fix/fix_import_source.h"

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "generated/code_fix_register.h"
#include "generated/diagnostic.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/services/import_utils.h"
#include "lsp/include/symbol_reference_index.h"
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {
using codefixes::FIX_IMPORT_SOURCE;

namespace {
constexpr char LINE_FEED = '\n';
constexpr char CARRIAGE_RETURN = '\r';
constexpr std::string_view LINE_BREAK_CHARACTERS = "\r\n";
constexpr std::string_view EXPORT_KEYWORD = "export";
constexpr size_t START_OF_SOURCE = 0U;
constexpr size_t PREVIOUS_CHARACTER_OFFSET = 1U;

bool IsRedefinitionError(int errorCode)
{
    return errorCode == GetDiagnosticCode(diagnostic::REDEFINITION);
}

bool IsDifferentTypeRedefinitionError(int errorCode)
{
    return errorCode == GetDiagnosticCode(diagnostic::REDEFINITION_DIFF_TYPE);
}

bool IsOverloadedFunctionsScopeError(int errorCode)
{
    return errorCode == GetDiagnosticCode(diagnostic::OVERLOADED_FUNCS_MUST_BE_IN_SAME_SCOPE);
}

bool IsImportConflictError(int errorCode)
{
    return IsRedefinitionError(errorCode) || IsDifferentTypeRedefinitionError(errorCode) ||
           IsOverloadedFunctionsScopeError(errorCode);
}

bool IsAmbiguousExportError(int errorCode)
{
    return errorCode == GetDiagnosticCode(diagnostic::AMBIGUOUS_EXPORT);
}

bool IsDuplicateExportAliasesError(int errorCode)
{
    return errorCode == GetDiagnosticCode(diagnostic::DUPLICATE_EXPORT_ALIASES);
}

bool IsConflictingExportAliasError(int errorCode)
{
    return errorCode == GetDiagnosticCode(diagnostic::CANNOT_EXPORT_DIFFERENT_OBJECTS_WITH_SAME_NAME);
}

bool IsExportConflictError(int errorCode)
{
    return IsAmbiguousExportError(errorCode) || IsDuplicateExportAliasesError(errorCode) ||
           IsConflictingExportAliasError(errorCode);
}

bool IsConflictingDeclarationError(int errorCode)
{
    return IsImportConflictError(errorCode) || IsExportConflictError(errorCode);
}

std::string BuildDescription(const std::string &unresolvedName, const std::string &modulePath, bool isDefaultExport)
{
    std::string description;
    description.append("Add import ");
    if (isDefaultExport) {
        description.append(unresolvedName);
    } else {
        description.append("{");
        description.append(unresolvedName);
        description.append("}");
    }
    description.append(" from '");
    description.append(modulePath);
    description.append("'");
    return description;
}

struct ActionBuildContext {
    const CodeFixContext &context;
    public_lib::Context *ctx;
    const std::string &unresolvedName;
    std::string_view sourceCode;
    size_t insertPos;
};

std::optional<CodeFixAction> BuildActionForDefinition(const ActionBuildContext &buildCtx,
                                                      const SymbolDefSearchResult &def)
{
    auto modulePath = ComputeRelativeImportPath(buildCtx.ctx->sourceFileName, def.fileName);
    if (modulePath.empty()) {
        return std::nullopt;
    }
    TextChangesContext textChangesContext = {buildCtx.context.host, buildCtx.context.formatContext,
                                             buildCtx.context.preferences};
    auto *mergeImportDecl =
        FindMergeableImportDeclarationForModule(buildCtx.ctx->parserProgram, modulePath, def.isDefaultExport);
    auto changes = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        if (mergeImportDecl != nullptr) {
            auto mergedImportText = BuildImportDeclarationTextWithAddedSymbol(mergeImportDecl, buildCtx.unresolvedName,
                                                                              def.isDefaultExport);
            if (mergedImportText.empty()) {
                return;
            }
            tracker.ReplaceRangeWithText(buildCtx.ctx->sourceFile,
                                         {mergeImportDecl->Start().index, mergeImportDecl->End().index},
                                         mergedImportText);
            return;
        }
        auto insertText = BuildImportInsertText(buildCtx.sourceCode, buildCtx.insertPos, buildCtx.unresolvedName,
                                                modulePath, def.isDefaultExport);
        tracker.InsertText(buildCtx.ctx->sourceFile, buildCtx.insertPos, insertText);
    });
    if (changes.empty()) {
        return std::nullopt;
    }

    CodeFixAction codeAction;
    codeAction.fixName = FIX_IMPORT_SOURCE.GetFixId().data();
    codeAction.description = BuildDescription(buildCtx.unresolvedName, modulePath, def.isDefaultExport);
    codeAction.changes = std::move(changes);
    return codeAction;
}

struct ConflictTarget {
    ir::AstNode *declaration;
    bool isLoweredExport;
};

std::optional<ConflictTarget> FindConflictTarget(int errorCode, ir::AstNode *token)
{
    const bool isImportConflict = IsImportConflictError(errorCode);
    const bool isExportConflict = IsExportConflictError(errorCode);
    auto *declaration = token != nullptr && token->OriginalNode() != nullptr ? token->OriginalNode() : token;
    if (isExportConflict && declaration == nullptr) {
        return ConflictTarget {nullptr, true};
    }

    while (declaration != nullptr) {
        if (isImportConflict && declaration->IsETSImportDeclaration()) {
            return ConflictTarget {declaration, false};
        }
        if (isExportConflict && declaration->IsExportNamedDeclaration()) {
            return ConflictTarget {declaration, false};
        }
        if (isExportConflict && declaration->Parent() != nullptr && declaration->Parent()->IsETSModule()) {
            return ConflictTarget {declaration, true};
        }
        declaration = declaration->Parent();
    }

    return std::nullopt;
}

std::optional<size_t> FindLoweredExportStart(const CodeFixContext &context, const ConflictTarget &target,
                                             std::string_view sourceCode)
{
    const size_t start = target.declaration != nullptr ? target.declaration->Start().index
                                                       : sourceCode.rfind(EXPORT_KEYWORD, context.span.start);
    if (start == std::string_view::npos || start + EXPORT_KEYWORD.size() > sourceCode.size() ||
        sourceCode.substr(start, EXPORT_KEYWORD.size()) != EXPORT_KEYWORD) {
        return std::nullopt;
    }

    const size_t previousLineBreak = start == START_OF_SOURCE
                                         ? std::string_view::npos
                                         : sourceCode.rfind(LINE_FEED, start - PREVIOUS_CHARACTER_OFFSET);
    const size_t lineStart = previousLineBreak == std::string_view::npos ? START_OF_SOURCE : previousLineBreak + 1U;
    if (sourceCode.substr(lineStart, start - lineStart).find_first_not_of(" \t") == std::string_view::npos) {
        return lineStart;
    }
    return start;
}

size_t FindEndOfExportClause(std::string_view sourceCode, size_t exportStart, size_t fallback)
{
    const size_t leftBrace = sourceCode.find('{', exportStart);
    if (leftBrace == std::string_view::npos) {
        return fallback;
    }

    size_t braceDepth = 0;
    for (size_t pos = leftBrace; pos < sourceCode.size(); ++pos) {
        if (sourceCode[pos] == '{') {
            ++braceDepth;
        } else if (sourceCode[pos] == '}' && --braceDepth == 0) {
            return pos + 1U;
        }
    }
    return fallback;
}

void IncludeTrailingLineBreak(std::string_view sourceCode, size_t &end)
{
    while (end < sourceCode.size() && (sourceCode[end] == ' ' || sourceCode[end] == '\t')) {
        ++end;
    }
    if (end < sourceCode.size() && sourceCode[end] == CARRIAGE_RETURN) {
        ++end;
        if (end < sourceCode.size() && sourceCode[end] == LINE_FEED) {
            ++end;
        }
    } else if (end < sourceCode.size() && sourceCode[end] == LINE_FEED) {
        ++end;
    }
}

std::optional<TextRange> BuildLoweredExportDeleteRange(const CodeFixContext &context, const ConflictTarget &target,
                                                       std::string_view sourceCode)
{
    auto start = FindLoweredExportStart(context, target, sourceCode);
    if (!start.has_value()) {
        return std::nullopt;
    }

    size_t searchStart =
        target.declaration != nullptr ? target.declaration->End().index : context.span.start + context.span.length;
    searchStart = FindEndOfExportClause(sourceCode, start.value(), searchStart);

    const size_t lineEnd = sourceCode.find_first_of(LINE_BREAK_CHARACTERS, searchStart);
    const size_t semicolon = sourceCode.find(';', searchStart);
    size_t end = semicolon != std::string_view::npos && (lineEnd == std::string_view::npos || semicolon < lineEnd)
                     ? semicolon + 1U
                     : (lineEnd == std::string_view::npos ? sourceCode.size() : lineEnd);
    IncludeTrailingLineBreak(sourceCode, end);
    return TextRange {start.value(), end};
}

std::vector<CodeFixAction> BuildRemoveConflictingDeclarationAction(const CodeFixContext &context, ir::AstNode *token)
{
    if (!IsConflictingDeclarationError(context.errorCode)) {
        return {};
    }

    auto target = FindConflictTarget(context.errorCode, token);
    if (!target.has_value()) {
        return {};
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    std::optional<TextRange> loweredExportRange;
    if (target->isLoweredExport) {
        loweredExportRange = BuildLoweredExportDeleteRange(context, target.value(), ctx->parserProgram->SourceCode());
        if (!loweredExportRange.has_value()) {
            return {};
        }
    }

    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    auto changes = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        if (loweredExportRange.has_value()) {
            tracker.DeleteRange(ctx->sourceFile, loweredExportRange.value());
            return;
        }
        tracker.DeleteNode(context.context, ctx->sourceFile, target->declaration);
    });
    if (changes.empty()) {
        return {};
    }

    CodeFixAction codeAction;
    codeAction.fixName = FIX_IMPORT_SOURCE.GetFixId().data();
    codeAction.description =
        IsImportConflictError(context.errorCode) ? "Remove conflicting import" : "Remove ambiguous export";
    codeAction.changes = std::move(changes);
    return {std::move(codeAction)};
}

}  // namespace

FixImportSource::FixImportSource()
{
    auto errorCodes = FIX_IMPORT_SOURCE.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_IMPORT_SOURCE.GetFixId().data()});
}

std::vector<CodeFixAction> FixImportSource::GetCodeActions(const CodeFixContext &context)
{
    if (context.context == nullptr) {
        return {};
    }

    auto *token = GetTouchingTokenRightMatch(context.context, context.span.start);
    if (IsConflictingDeclarationError(context.errorCode)) {
        return BuildRemoveConflictingDeclarationAction(context, token);
    }

    if (token == nullptr || !token->IsIdentifier()) {
        return {};
    }

    std::string unresolvedName(token->AsIdentifier()->Name());
    if (unresolvedName.empty()) {
        return {};
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx->parserProgram == nullptr || ctx->sourceFile == nullptr) {
        return {};
    }

    if (IsSymbolAlreadyImported(ctx->parserProgram, unresolvedName)) {
        return {};
    }

    BuildSymbolReferenceIndexForContextWithExternal(context.context);

    auto definitions = FindSymbolDefinitionsByName(unresolvedName, ctx->sourceFileName);
    if (definitions.empty()) {
        return {};
    }

    const std::string_view sourceCode = ctx->parserProgram->SourceCode();
    size_t insertPos = GetImportInsertPosition(ctx->parserProgram);
    insertPos = AdjustInsertPositionForUseStaticDirective(insertPos, sourceCode);

    ActionBuildContext buildCtx {context, ctx, unresolvedName, sourceCode, insertPos};

    std::vector<CodeFixAction> actions;
    std::unordered_set<std::string> seenModules;
    for (const auto &def : definitions) {
        auto modulePath = ComputeRelativeImportPath(ctx->sourceFileName, def.fileName);
        if (modulePath.empty() || !seenModules.insert(modulePath).second) {
            continue;
        }
        auto action = BuildActionForDefinition(buildCtx, def);
        if (action.has_value()) {
            actions.push_back(std::move(action.value()));
        }
    }

    return actions;
}

CombinedCodeActions FixImportSource::GetAllCodeActions(const CodeFixAllContext &codeFixAll)
{
    CombinedCodeActions combinedCodeActions;
    if (codeFixAll.fixId == FIX_IMPORT_SOURCE.GetFixId().data()) {
    }

    return combinedCodeActions;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixImportSource> g_fixImportSource(FIX_IMPORT_SOURCE.GetFixId().data());

}  // namespace ark::es2panda::lsp
