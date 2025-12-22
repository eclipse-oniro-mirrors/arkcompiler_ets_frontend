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

#include "lsp/include/internal_api.h"
#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/register_code_fix/remove_accidental_call_parentheses.h"

namespace ark::es2panda::lsp {
using codefixes::REMOVE_ACCIDENTAL_CALL_PARENTHESES;

bool FixRemoveAccidentalCallParentheses::IsValidCallExpression(const ir::AstNode *node)
{
    if (node == nullptr || !node->IsCallExpression()) {
        return false;
    }

    const auto *callExpr = node->AsCallExpression();
    const auto *callee = callExpr->Callee();
    if (callee == nullptr) {
        return false;
    }

    if (!callee->IsMemberExpression()) {
        return false;
    }

    if (!callExpr->Arguments().empty()) {
        return false;
    }

    return true;
}

TextRange FixRemoveAccidentalCallParentheses::CalculateDeleteRange(const ir::AstNode *callExpr)
{
    const auto *callExpression = callExpr->AsCallExpression();
    if (callExpression == nullptr || callExpression->Callee() == nullptr) {
        return TextRange {0, 0};
    }

    size_t openParenPos = static_cast<size_t>(callExpression->Callee()->End().index);
    size_t closeParenPos = static_cast<size_t>(callExpression->End().index);
    if (closeParenPos <= openParenPos) {
        return TextRange {0, 0};
    }

    return TextRange {openParenPos, closeParenPos};
}

void FixRemoveAccidentalCallParentheses::MakeChange(ChangeTracker &changeTracker, es2panda_Context *context, size_t pos,
                                                    std::vector<ir::AstNode *> &fixedNodes)
{
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr) {
        return;
    }

    const ir::AstNode *callExpr = token;
    while (callExpr != nullptr && !callExpr->IsCallExpression()) {
        callExpr = callExpr->Parent();
    }

    if (!IsValidCallExpression(callExpr)) {
        return;
    }

    TextRange deleteRange = CalculateDeleteRange(callExpr);
    if (deleteRange.pos == 0 && deleteRange.end == 0) {
        return;
    }

    fixedNodes.push_back(const_cast<ir::AstNode *>(callExpr));
    auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    const auto sourceFile = ctx->sourceFile;
    changeTracker.DeleteRange(sourceFile, deleteRange);
}

std::vector<FileTextChanges> FixRemoveAccidentalCallParentheses::GetCodeActionsToFix(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    std::vector<ir::AstNode *> fixedNodes;
    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChange(tracker, context.context, context.span.start, fixedNodes);
    });

    return fileTextChanges;
}

FixRemoveAccidentalCallParentheses::FixRemoveAccidentalCallParentheses()
{
    const auto &codes = REMOVE_ACCIDENTAL_CALL_PARENTHESES.GetSupportedCodeNumbers();
    SetErrorCodes({codes.begin(), codes.end()});
    SetFixIds({REMOVE_ACCIDENTAL_CALL_PARENTHESES.GetFixId().data()});
}

std::vector<CodeFixAction> FixRemoveAccidentalCallParentheses::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToFix(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = REMOVE_ACCIDENTAL_CALL_PARENTHESES.GetFixId().data();
        codeAction.description = "Remove parentheses from accessor call";
        codeAction.changes = changes;
        codeAction.fixId = REMOVE_ACCIDENTAL_CALL_PARENTHESES.GetFixId().data();
        returnedActions.push_back(codeAction);
    }
    return returnedActions;
}

CombinedCodeActions FixRemoveAccidentalCallParentheses::GetAllCodeActions([[maybe_unused]] const CodeFixAllContext &ctx)
{
    return {};
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixRemoveAccidentalCallParentheses> g_removeCallParens(
    REMOVE_ACCIDENTAL_CALL_PARENTHESES.GetFixId().data());
}  // namespace ark::es2panda::lsp