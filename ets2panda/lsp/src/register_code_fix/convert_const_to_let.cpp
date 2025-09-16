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

#include "lsp/include/register_code_fix/convert_const_to_let.h"

#include <iostream>
#include <string>

#include "compiler/lowering/util.h"
#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"

namespace ark::es2panda::lsp {
using codefixes::FIX_CONVERT_CONST_TO_LET;

constexpr std::string_view KEYW_CONST_STR = "const";
constexpr std::string_view KEYW_LET_STR = "let";

void FixConvertConstToLet::MakeChangeForConvertConstToLet(ChangeTracker &changeTracker, es2panda_Context *context,
                                                          size_t pos)
{
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr) {
        return;
    }

    auto *scope = compiler::NearestScope(token);
    auto *resolvedDecl = FindDeclInScopeWithFallback(scope, token->AsIdentifier()->Name());
    if (resolvedDecl == nullptr) {
        return;
    }

    TextRange constTokenRange = {0, 0};
    changeTracker.RfindNearestKeyWordTextRange(context, resolvedDecl->Node()->Start().index, KEYW_CONST_STR,
                                               constTokenRange);
    if (constTokenRange.end == 0) {
        return;
    }
    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const std::string replaceText(KEYW_LET_STR);
    changeTracker.ReplaceRangeWithText(astContext->sourceFile, constTokenRange, replaceText);
}

std::vector<FileTextChanges> FixConvertConstToLet::GetCodeActionsToConvertConstToLet(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForConvertConstToLet(tracker, context.context, context.span.start);
    });

    return fileTextChanges;
}

FixConvertConstToLet::FixConvertConstToLet()
{
    auto errorCodes = FIX_CONVERT_CONST_TO_LET.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_CONVERT_CONST_TO_LET.GetFixId().data()});
}

std::vector<CodeFixAction> FixConvertConstToLet::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToConvertConstToLet(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = FIX_CONVERT_CONST_TO_LET.GetFixId().data();
        codeAction.description = "Convert const to let";
        codeAction.changes = changes;
        codeAction.fixId = FIX_CONVERT_CONST_TO_LET.GetFixId().data();
        codeAction.fixAllDescription = "Convert all const to let";
        returnedActions.push_back(codeAction);
    }

    return returnedActions;
}

CombinedCodeActions FixConvertConstToLet::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            MakeChangeForConvertConstToLet(tracker, codeFixAllCtx.context, diag.GetStart());
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;

    return combinedCodeActions;
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixConvertConstToLet> g_fixConvertConstToLet(FIX_CONVERT_CONST_TO_LET.GetFixId().data());
}  // namespace ark::es2panda::lsp
