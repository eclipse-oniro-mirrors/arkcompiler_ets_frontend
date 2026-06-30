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

#include "lsp/include/register_code_fix/fix_forbidden_any_type.h"

#include <string>

#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"

namespace ark::es2panda::lsp {
using codefixes::FIX_FORBIDDEN_ANY_TYPE;

constexpr std::string_view ANY_LOWER = "any";
constexpr std::string_view ANY_UPPER = "Any";

void FixForbiddenAnyType::MakeChange(ChangeTracker &changeTracker, es2panda_Context *context, size_t pos)
{
    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    if (astContext == nullptr || astContext->sourceFile == nullptr) {
        return;
    }

    auto source = std::string_view(astContext->sourceFile->source);
    if (pos + ANY_LOWER.size() > source.size() || source.substr(pos, ANY_LOWER.size()) != ANY_LOWER) {
        return;
    }

    TextRange range = {pos, pos + ANY_LOWER.size()};
    changeTracker.ReplaceRangeWithText(astContext->sourceFile, range, std::string(ANY_UPPER));
}

std::vector<FileTextChanges> FixForbiddenAnyType::GetCodeActionsToFixForbiddenAnyType(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    return ChangeTracker::With(
        textChangesContext, [&](ChangeTracker &tracker) { MakeChange(tracker, context.context, context.span.start); });
}

FixForbiddenAnyType::FixForbiddenAnyType()
{
    auto errorCodes = FIX_FORBIDDEN_ANY_TYPE.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_FORBIDDEN_ANY_TYPE.GetFixId().data()});
}

std::vector<CodeFixAction> FixForbiddenAnyType::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToFixForbiddenAnyType(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = FIX_FORBIDDEN_ANY_TYPE.GetFixId().data();
        codeAction.description = "Fix 'any' to 'Any'";
        codeAction.changes = changes;
        codeAction.fixId = FIX_FORBIDDEN_ANY_TYPE.GetFixId().data();
        codeAction.fixAllDescription = "Fix all 'any' to 'Any'";
        returnedActions.push_back(codeAction);
    }

    return returnedActions;
}

CombinedCodeActions FixForbiddenAnyType::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(codeFixAllCtx, GetErrorCodes(),
                                             [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
                                                 MakeChange(tracker, codeFixAllCtx.context, diag.GetStart());
                                             });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;

    return combinedCodeActions;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixForbiddenAnyType> g_fixForbiddenAnyType(FIX_FORBIDDEN_ANY_TYPE.GetFixId().data());
}  // namespace ark::es2panda::lsp
