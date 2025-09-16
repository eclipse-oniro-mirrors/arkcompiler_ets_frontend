
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

#include "lsp/include/register_code_fix/fix_nan_equality.h"
#include <iostream>
#include <string>
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"

namespace ark::es2panda::lsp {

const int G_FIX_NAN_EQUALITY_CODE = 1003;  // change this to the error code you want to handle

void FixNaNEquality::MakeChangeForNaNEquality(ChangeTracker &changeTracker, es2panda_Context *context, size_t pos,
                                              std::vector<ark::es2panda::ir::AstNode *> &fixedNodes)
{
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr || !token->IsBinaryExpression()) {
        return;
    }

    const auto *binaryExpr = token->AsBinaryExpression();
    if (binaryExpr->Left() == nullptr || binaryExpr->Right() == nullptr) {
        return;
    }

    auto isLeftNaN = binaryExpr->Left()->IsIdentifier() && binaryExpr->Left()->AsIdentifier()->Name() == "NaN";
    auto isRightNaN = binaryExpr->Right()->IsIdentifier() && binaryExpr->Right()->AsIdentifier()->Name() == "NaN";
    if (!isLeftNaN && !isRightNaN) {
        return;
    }

    auto *expr = isLeftNaN ? binaryExpr->Right() : binaryExpr->Left();
    std::string exprText = expr->ToString();
    std::string newText;

    if (binaryExpr->Type() == ir::AstNodeType::TS_IMPORT_EQUALS_DECLARATION) {
        newText = "Number.isNaN(" + exprText + ")";
    } else {
        return;
    }

    // auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);

    fixedNodes.push_back(const_cast<ir::AstNode *>(token));
    // ark::es2panda::ir::AstNode *bClone = token->Clone(ctx->allocator, nullptr);

    ChangeNodeOptions changeNodeOptions;
    changeNodeOptions.insertNodeOptions->prefix = "";
    changeNodeOptions.insertNodeOptions->suffix = "";
    changeNodeOptions.insertNodeOptions->delta = 0;

    ark::es2panda::lsp::ChangeNodeOptions options = {};
    changeTracker.ReplaceNode(context, token, token, changeNodeOptions);
}

std::vector<FileTextChanges> FixNaNEquality::GetCodeActionsToFixNaNEquality(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    std::vector<ir::AstNode *> fixedNodes;
    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForNaNEquality(tracker, context.context, context.span.start, fixedNodes);
    });

    return fileTextChanges;
}

FixNaNEquality::FixNaNEquality()
{
    const char *fixNanEqualityId = "FixNaNEquality";
    SetErrorCodes({G_FIX_NAN_EQUALITY_CODE});  // "NaN comparison" error code
    SetFixIds({fixNanEqualityId});             // "fixNaNEquality" fix ID
}

std::vector<CodeFixAction> FixNaNEquality::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToFixNaNEquality(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = "fixNaNEquality";
        codeAction.description = "Use Number.isNaN instead of comparing with NaN";
        codeAction.changes = changes;
        codeAction.fixId = "FixNaNEquality";
        codeAction.fixAllDescription = "Replace all NaN equality comparisons";
        returnedActions.push_back(codeAction);
    }
    return returnedActions;
}

CombinedCodeActions FixNaNEquality::GetAllCodeActions(const CodeFixAllContext &codeFixAll)
{
    const std::vector<ir::AstNode *> fixedNodes;
    CodeFixProvider provider;

    const auto changes = provider.CodeFixAll(
        codeFixAll, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            Initializer initializer = Initializer();
            auto ctx = initializer.CreateContext(diag.GetFile().source.data(), ES2PANDA_STATE_CHECKED,
                                                 diag.GetFile().source.data());
            MakeChangeForNaNEquality(tracker, ctx, diag.GetStart(),
                                     const_cast<std::vector<ir::AstNode *> &>(fixedNodes));
            initializer.DestroyContext(ctx);
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;
    return combinedCodeActions;
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixNaNEquality> g_fixNaNEquality("FixNaNEquality");
}  // namespace ark::es2panda::lsp
