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

#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/completions.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/register_code_fix/import_fixes.h"
#include "lsp/include/services/import_utils.h"
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {
using codefixes::ADD_LOCAL_VARIABLE;
using codefixes::ADD_LOCAL_VARIABLE_FOR_CLASS;

namespace {
constexpr int G_IMPORT_FIXES_CODE = 1005;
constexpr const char *G_IMPORT_FIXES_ID = "ImportFixes";

std::string GetUnresolvedIdentifierName(es2panda_Context *context, size_t position)
{
    auto *token = GetTouchingTokenRightMatch(context, position);
    if (token == nullptr || !token->IsIdentifier()) {
        return "";
    }

    return std::string(token->AsIdentifier()->Name());
}

bool IsUseStaticDirectiveAtFileTop(std::string_view source, size_t firstNonWhitespace)
{
    constexpr std::string_view SINGLE_QUOTE_USE_STATIC = "'use static'";
    constexpr std::string_view DOUBLE_QUOTE_USE_STATIC = "\"use static\"";

    if (firstNonWhitespace >= source.size()) {
        return false;
    }

    auto remains = source.substr(firstNonWhitespace);
    if (remains.rfind(SINGLE_QUOTE_USE_STATIC, 0) != 0 && remains.rfind(DOUBLE_QUOTE_USE_STATIC, 0) != 0) {
        return false;
    }

    size_t directiveLen = remains.rfind(SINGLE_QUOTE_USE_STATIC, 0) == 0 ? SINGLE_QUOTE_USE_STATIC.size()
                                                                         : DOUBLE_QUOTE_USE_STATIC.size();
    size_t i = firstNonWhitespace + directiveLen;
    while (i < source.size() && (source[i] == ' ' || source[i] == '\t')) {
        ++i;
    }
    return i == source.size() || source[i] == ';' || source[i] == '\r' || source[i] == '\n';
}

size_t GetInsertPositionAfterFirstLine(std::string_view source, size_t lineStart)
{
    size_t lineBreakPos = source.find_first_of("\r\n", lineStart);
    if (lineBreakPos == std::string_view::npos) {
        return source.size();
    }

    if (source[lineBreakPos] == '\r' && (lineBreakPos + 1U) < source.size() && source[lineBreakPos + 1U] == '\n') {
        return lineBreakPos + 2U;
    }
    return lineBreakPos + 1U;
}

size_t AdjustInsertPositionForUseStaticDirective(size_t originalPos, std::string_view source)
{
    size_t firstNonWhitespace = source.find_first_not_of(" \t\r\n");
    if (firstNonWhitespace == std::string_view::npos) {
        return originalPos;
    }
    if (originalPos != firstNonWhitespace) {
        return originalPos;
    }

    if (!IsUseStaticDirectiveAtFileTop(source, firstNonWhitespace)) {
        return originalPos;
    }

    return GetInsertPositionAfterFirstLine(source, firstNonWhitespace);
}

struct ImportCodeActionBuildContext {
    const std::string &unresolvedName;
    public_lib::Context *ctx;
    TextChangesContext textChangesContext;
    size_t insertPos;
    std::string_view sourceCode;
};

bool TryBuildImportCodeActionForCollectInfo(const ExternalApiCollectInfo &info, const std::string &moduleName,
                                            ImportCodeActionBuildContext &buildContext, CodeFixAction *codeAction)
{
    auto *mergeImportDecl =
        FindMergeableImportDeclarationForModule(buildContext.ctx->parserProgram, moduleName, info.isDefault);
    auto changes = ChangeTracker::With(buildContext.textChangesContext, [&](ChangeTracker &tracker) {
        if (mergeImportDecl != nullptr) {
            auto mergedImportText =
                BuildImportDeclarationTextWithAddedSymbol(mergeImportDecl, buildContext.unresolvedName, info.isDefault);
            if (mergedImportText.empty()) {
                return;
            }
            tracker.ReplaceRangeWithText(buildContext.ctx->sourceFile,
                                         {mergeImportDecl->Start().index, mergeImportDecl->End().index},
                                         mergedImportText);
            return;
        }

        auto insertText = BuildImportInsertText(buildContext.sourceCode, buildContext.insertPos,
                                                buildContext.unresolvedName, moduleName, info.isDefault);
        tracker.InsertText(buildContext.ctx->sourceFile, buildContext.insertPos, insertText);
    });
    if (changes.empty()) {
        return false;
    }

    codeAction->fixName = G_IMPORT_FIXES_ID;
    codeAction->description = info.isDefault
                                  ? ("Add import " + buildContext.unresolvedName + " from '" + moduleName + "'")
                                  : ("Add import {" + buildContext.unresolvedName + "} from '" + moduleName + "'");
    codeAction->additionalMessage = info.importDeclaration;
    codeAction->changes = std::move(changes);
    return true;
}

std::vector<CodeFixAction> BuildImportCodeActionsForCollectedInfos(
    const std::vector<ExternalApiCollectInfo> &collectedInfos, ImportCodeActionBuildContext &buildContext)
{
    std::vector<CodeFixAction> actions;
    std::unordered_set<std::string> importKeys;
    importKeys.reserve(collectedInfos.size());

    for (const auto &info : collectedInfos) {
        auto moduleName = NormalizeImportModulePath(info.importDeclaration);
        if (moduleName.empty()) {
            continue;
        }

        std::string importKey = moduleName + (info.isDefault ? "#default" : "#named");
        if (!importKeys.insert(importKey).second) {
            continue;
        }

        CodeFixAction codeAction;
        if (!TryBuildImportCodeActionForCollectInfo(info, moduleName, buildContext, &codeAction)) {
            continue;
        }

        actions.push_back(std::move(codeAction));
    }

    return actions;
}
}  // namespace

ImportFixes::ImportFixes()
{
    auto unresolvedCodes = ADD_LOCAL_VARIABLE.GetSupportedCodeNumbers();
    auto unresolvedClassCodes = ADD_LOCAL_VARIABLE_FOR_CLASS.GetSupportedCodeNumbers();

    std::vector<int> errorCodes {G_IMPORT_FIXES_CODE};
    errorCodes.insert(errorCodes.end(), unresolvedCodes.begin(), unresolvedCodes.end());
    errorCodes.insert(errorCodes.end(), unresolvedClassCodes.begin(), unresolvedClassCodes.end());
    std::sort(errorCodes.begin(), errorCodes.end());
    errorCodes.erase(std::unique(errorCodes.begin(), errorCodes.end()), errorCodes.end());

    SetErrorCodes(errorCodes);
    SetFixIds({G_IMPORT_FIXES_ID});
}

std::vector<CodeFixAction> ImportFixes::GetCodeActions(const CodeFixContext &context)
{
    if (context.context == nullptr) {
        return {};
    }

    auto unresolvedName = GetUnresolvedIdentifierName(context.context, context.span.start);
    if (unresolvedName.empty()) {
        return {};
    }

    auto collectedInfos = GetExternalApiCollectInfos(unresolvedName);
    if (collectedInfos.empty()) {
        return {};
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    if (ctx->parserProgram == nullptr || ctx->sourceFile == nullptr) {
        return {};
    }
    if (IsSymbolAlreadyImported(ctx->parserProgram, unresolvedName)) {
        return {};
    }

    const std::string_view sourceCode = ctx->parserProgram->SourceCode();
    size_t insertPos = GetImportInsertPosition(ctx->parserProgram);
    insertPos = AdjustInsertPositionForUseStaticDirective(insertPos, sourceCode);

    ImportCodeActionBuildContext buildContext {
        unresolvedName, ctx, {context.host, context.formatContext, context.preferences}, insertPos, sourceCode};
    return BuildImportCodeActionsForCollectedInfos(collectedInfos, buildContext);
}

CombinedCodeActions ImportFixes::GetAllCodeActions(const CodeFixAllContext &codeFixAll)
{
    CombinedCodeActions combinedCodeActions;
    if (codeFixAll.fixId == G_IMPORT_FIXES_ID) {
    }

    return combinedCodeActions;
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<ImportFixes> g_importFixes(G_IMPORT_FIXES_ID);
}  // namespace ark::es2panda::lsp
