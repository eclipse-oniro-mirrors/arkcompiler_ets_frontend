/**
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

#include "lsp/include/register_code_fix/ui_plugin_suggest.h"
#include <iostream>
#include <string>
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"

namespace ark::es2panda::lsp {
const int G_UI_PLUGIN_SUGGEST_CODE = 4000;  // change this to the error code you want to handle
constexpr const char *G_UI_PLUGIN_SUGGEST_ID = "UIPluginSuggest";
std::vector<FileTextChanges> fixAll;

UIPluginSuggest::UIPluginSuggest()
{
    SetErrorCodes({G_UI_PLUGIN_SUGGEST_CODE});
    SetFixIds({G_UI_PLUGIN_SUGGEST_ID});
}

CodeFixAction CreateCodeFixAction(const ark::es2panda::util::Diagnostic *diag, std::vector<FileTextChanges> &changes)
{
    CodeFixAction codeAction;
    codeAction.fixName = "Fix";
    codeAction.description =
        diag->HasSuggestions() && !diag->Suggestion().empty()
            ? !diag->Suggestion().at(0)->Title().empty() ? diag->Suggestion().at(0)->Title() : "Fix Description"
            : "Fix Description";
    codeAction.changes = std::move(changes);
    codeAction.fixId = "UI_PLUGIN_SUGGEST";
    codeAction.fixAllDescription = "Fix All Description";
    InstallPackageAction codeActionCommand;
    codeActionCommand.file = diag->File();
    codeActionCommand.packageName = "";
    codeAction.commands.push_back(codeActionCommand);
    return codeAction;
}

std::vector<TextChange> GetTextChangesFromSuggestions(const ark::es2panda::util::Diagnostic *diag, size_t pos,
                                                      bool isAll, es2panda_Context *context)
{
    std::vector<TextChange> textChanges;
    if (!diag->HasSuggestions()) {
        return textChanges;
    }
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    auto index = lexer::LineIndex(ctx->parserProgram->SourceCode());
    auto offset = index.GetOffset(lexer::SourceLocation(diag->Line(), diag->Offset(), ctx->parserProgram));
    auto touchingToken = GetTouchingToken(context, offset, false);
    if (touchingToken == nullptr) {
        return textChanges;
    }
    auto start = touchingToken->Start().index;
    auto end = touchingToken->End().index;
    for (auto suggestion : diag->Suggestion()) {
        auto sourceStart = suggestion->SourceRange()->start.index;
        auto sourceEnd = suggestion->SourceRange()->end.index;
        auto span = TextSpan(sourceStart, sourceEnd - sourceStart);
        if (isAll || (pos >= start && pos <= end)) {
            // compare diag range instead of suggestion range
            // to support rules of different ranges of diag and suggestion
            textChanges.emplace_back(TextChange(span, suggestion->SubstitutionCode()));
        }
    }
    return textChanges;
}

std::vector<FileTextChanges> GetUIPluginCodeFixesByDiagType(es2panda_Context *context, size_t pos, bool isAll,
                                                            std::vector<CodeFixAction> &actions,
                                                            util::DiagnosticType type)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    const auto &diagnostics = ctx->diagnosticEngine->GetDiagnosticStorage(type);
    std::vector<FileTextChanges> changes;
    for (const auto &diagnostic : diagnostics) {
        std::vector<FileTextChanges> fileChanges;
        auto diag = reinterpret_cast<const ark::es2panda::util::Diagnostic *>(&(*diagnostic));
        auto textChanges = GetTextChangesFromSuggestions(diag, pos, isAll, context);
        FileTextChanges fileTextChanges(ctx->sourceFileName, textChanges);
        fileChanges.emplace_back(fileTextChanges);
        changes.emplace_back(fileTextChanges);
        actions.push_back(CreateCodeFixAction(diag, fileChanges));
    }
    return changes;
}

std::vector<CodeFixAction> UIPluginSuggest::GetUIPluginCodeFixes(es2panda_Context *context, size_t pos, bool isAll)
{
    std::vector<util::DiagnosticType> types = {util::DiagnosticType::PLUGIN_ERROR,
                                               util::DiagnosticType::PLUGIN_WARNING};
    std::vector<FileTextChanges> changes;
    std::vector<CodeFixAction> returnedActions;
    for (const auto &type : types) {
        auto typeChanges = GetUIPluginCodeFixesByDiagType(context, pos, isAll, returnedActions, type);
        changes.insert(changes.end(), typeChanges.begin(), typeChanges.end());
    }
    fixAll = changes;
    return returnedActions;
}

std::vector<CodeFixAction> UIPluginSuggest::GetCodeActions(const CodeFixContext &context)
{
    auto returnedActions = GetUIPluginCodeFixes(context.context, context.span.start, false);
    return returnedActions;
}

CombinedCodeActions UIPluginSuggest::GetAllCodeActions(const CodeFixAllContext &codeFixAll)
{
    CombinedCodeActions combinedCodeActions;
    if (fixAll.empty()) {
        GetUIPluginCodeFixes(codeFixAll.context, 0, true);
    }
    combinedCodeActions.changes = fixAll;
    InstallPackageAction codeActionCommand;
    codeActionCommand.file = reinterpret_cast<public_lib::Context *>(codeFixAll.context)->sourceFileName;
    codeActionCommand.packageName = "";
    combinedCodeActions.commands.push_back(codeActionCommand);

    return combinedCodeActions;
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<UIPluginSuggest> g_uiPluginSuggest(G_UI_PLUGIN_SUGGEST_ID);
}  // namespace ark::es2panda::lsp
