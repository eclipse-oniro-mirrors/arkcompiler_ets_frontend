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

#include "lsp/include/register_code_fix/fix_spelling_for_type.h"

#include <string>
#include <vector>

#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/symbol_reference_index.h"
#include "ir/expressions/identifier.h"
#include "public/public.h"

namespace ark::es2panda::lsp {

using codefixes::FIX_SPELLING_FOR_TYPE;

FixSpellingForType::FixSpellingForType()
{
    auto errorCodes = FIX_SPELLING_FOR_TYPE.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_SPELLING_FOR_TYPE.GetFixId().data()});
}

void FixSpellingForType::MakeChangeForFixSpellingForType(ChangeTracker &changeTracker, es2panda_Context *context,
                                                         size_t pos, const std::string &target)
{
    auto *token = GetTouchingTokenRightMatch(context, pos, false);
    if (token == nullptr || !token->IsIdentifier()) {
        return;
    }

    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    TextRange range = {token->Start().index, token->End().index};
    changeTracker.ReplaceRangeWithText(ctx->sourceFile, range, target);
}

std::vector<FileTextChanges> FixSpellingForType::GetCodeActionsToFixSpellingForType(const CodeFixContext &context,
                                                                                    const std::string &target)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    return ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForFixSpellingForType(tracker, context.context, context.span.start, target);
    });
}

std::vector<CodeFixAction> FixSpellingForType::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;

    auto *token = GetTouchingTokenRightMatch(context.context, context.span.start, false);
    if (token == nullptr || !token->IsIdentifier()) {
        return returnedActions;
    }
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context.context);
    std::string fileName = ctx->parserProgram->AbsoluteName().Mutf8();
    std::string typeName(token->AsIdentifier()->Name().Utf8());
    auto candidates = FindSimilarSymbolNames(typeName, fileName);
    if (candidates.empty()) {
        return returnedActions;
    }

    CodeFixAction action;
    action.fixName = FIX_SPELLING_FOR_TYPE.GetFixId().data();
    action.fixId = FIX_SPELLING_FOR_TYPE.GetFixId().data();
    action.fixAllDescription = "Fix all type spelling errors";

    for (const auto &candidate : candidates) {
        auto changes = GetCodeActionsToFixSpellingForType(context, candidate);
        if (changes.empty()) {
            continue;
        }

        action.description = "Did you mean '" + candidate + "'?";
        action.changes = changes;
        returnedActions.push_back(action);
    }

    return returnedActions;
}

CombinedCodeActions FixSpellingForType::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            auto *token = GetTouchingTokenRightMatch(codeFixAllCtx.context, diag.GetStart(), false);
            if (token == nullptr || !token->IsIdentifier()) {
                return;
            }
            auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(codeFixAllCtx.context);
            std::string fileName = ctx->parserProgram->AbsoluteName().Mutf8();
            std::string typeName(token->AsIdentifier()->Name().Utf8());
            auto candidates = FindSimilarSymbolNames(typeName, fileName);
            for (const auto &candidate : candidates) {
                MakeChangeForFixSpellingForType(tracker, codeFixAllCtx.context, diag.GetStart(), candidate);
            }
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;
    return combinedCodeActions;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixSpellingForType> g_fixSpellingForType(FIX_SPELLING_FOR_TYPE.GetFixId().data());

}  // namespace ark::es2panda::lsp
