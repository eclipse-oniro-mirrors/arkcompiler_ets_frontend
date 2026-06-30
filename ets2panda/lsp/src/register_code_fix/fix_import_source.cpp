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
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/services/import_utils.h"
#include "lsp/include/symbol_reference_index.h"
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {
using codefixes::FIX_IMPORT_SOURCE;

namespace {

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
