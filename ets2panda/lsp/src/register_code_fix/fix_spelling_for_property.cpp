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

#include "lsp/include/register_code_fix/fix_spelling_for_property.h"

#include <string>
#include <vector>

#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "lsp/include/symbol_reference_index.h"
#include "checker/types/ets/etsObjectType.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/memberExpression.h"
#include "public/public.h"

namespace ark::es2panda::lsp {

using codefixes::FIX_SPELLING_FOR_PROPERTY;

FixSpellingForProperty::FixSpellingForProperty()
{
    auto errorCodes = FIX_SPELLING_FOR_PROPERTY.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_SPELLING_FOR_PROPERTY.GetFixId().data()});
}

std::vector<std::string> FixSpellingForProperty::GetPropertyCandidatesFromType(checker::ETSObjectType *objType,
                                                                               const std::string &misspelledName)
{
    std::vector<std::string> candidates;
    objType->IterateProperties([&](const varbinder::LocalVariable *prop) {
        std::string propName(prop->Name().Utf8());
        candidates.push_back(propName);
    });

    std::string best = GetSpellingSuggestion(misspelledName, candidates);
    std::vector<std::string> result;
    if (!best.empty()) {
        result.push_back(best);
    }
    return result;
}

void FixSpellingForProperty::MakeChangeForFixSpellingForProperty(ChangeTracker &changeTracker,
                                                                 es2panda_Context *context, size_t pos,
                                                                 const std::string &target)
{
    auto *token = GetTouchingTokenRightMatch(context, pos, false);
    if (token == nullptr || !token->IsIdentifier()) {
        return;
    }

    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    TextRange range = {token->Start().index, token->End().index};
    changeTracker.ReplaceRangeWithText(ctx->sourceFile, range, target);
}

std::vector<FileTextChanges> FixSpellingForProperty::GetCodeActionsToFixSpellingForProperty(
    const CodeFixContext &context, const std::string &target)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    return ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForFixSpellingForProperty(tracker, context.context, context.span.start, target);
    });
}

std::vector<CodeFixAction> FixSpellingForProperty::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;

    auto *token = GetTouchingTokenRightMatch(context.context, context.span.start, false);
    if (token == nullptr || !token->IsIdentifier()) {
        return returnedActions;
    }

    // Walk up to find MemberExpression parent where this token is the property
    auto *parent = token->Parent();
    if (parent == nullptr || !parent->IsMemberExpression()) {
        return returnedActions;
    }
    auto *memberExpr = parent->AsMemberExpression();
    if (memberExpr->Property() != token) {
        return returnedActions;
    }

    // Get the object type from the member expression
    auto *objType = memberExpr->ObjType();
    if (objType == nullptr) {
        return returnedActions;
    }

    std::string misspelledName(token->AsIdentifier()->Name().Utf8());
    auto candidates = GetPropertyCandidatesFromType(objType, misspelledName);
    if (candidates.empty()) {
        return returnedActions;
    }

    CodeFixAction action;
    action.fixName = FIX_SPELLING_FOR_PROPERTY.GetFixId().data();
    action.fixId = FIX_SPELLING_FOR_PROPERTY.GetFixId().data();
    action.fixAllDescription = "Fix all property spelling errors";

    for (const auto &candidate : candidates) {
        auto changes = GetCodeActionsToFixSpellingForProperty(context, candidate);
        if (changes.empty()) {
            continue;
        }

        action.description = "Did you mean '" + candidate + "'?";
        action.changes = changes;
        returnedActions.push_back(action);
    }

    return returnedActions;
}

CombinedCodeActions FixSpellingForProperty::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            auto *token = GetTouchingTokenRightMatch(codeFixAllCtx.context, diag.GetStart(), false);
            if (token == nullptr || !token->IsIdentifier()) {
                return;
            }
            auto *parent = token->Parent();
            if (parent == nullptr || !parent->IsMemberExpression()) {
                return;
            }
            auto *memberExpr = parent->AsMemberExpression();
            if (memberExpr->Property() != token) {
                return;
            }
            auto *objType = memberExpr->ObjType();
            if (objType == nullptr) {
                return;
            }
            std::string misspelledName(token->AsIdentifier()->Name().Utf8());
            auto candidates = GetPropertyCandidatesFromType(objType, misspelledName);
            for (const auto &candidate : candidates) {
                MakeChangeForFixSpellingForProperty(tracker, codeFixAllCtx.context, diag.GetStart(), candidate);
            }
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;
    return combinedCodeActions;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixSpellingForProperty> g_fixSpellingForProperty(FIX_SPELLING_FOR_PROPERTY.GetFixId().data());

}  // namespace ark::es2panda::lsp
