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
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/register_code_fix/fix_expected_comma.h"

namespace ark::es2panda::lsp {
const int G_FIX_EXPECTED_COMMA = 1016;

void FixExpectedComma::MakeChange(ChangeTracker &changeTracker, es2panda_Context *context, Range range,
                                  const std::string &possibleFix)
{
    if (possibleFix.empty()) {
        return;
    }

    auto node = GetNodeAtLocation(context, range);
    if (node == nullptr) {
        return;
    }

    if (node->Parent()->IsObjectExpression() && node->IsProperty()) {
        changeTracker.ReplaceNodeWithText(context, node->Parent(), possibleFix);
    }
}

ir::AstNode *FixExpectedComma::GetNodeAtLocation(es2panda_Context *context, Range range)
{
    auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto ast = ctx->parserProgram->Ast();
    auto nodeAtLocation =
        ast->FindChild([&range](ir::AstNode *node) { return node->Range().start.line == range.start.line_; });

    return nodeAtLocation;
}

std::vector<FileTextChanges> FixExpectedComma::GetCodeActionsToFix(const CodeFixContext &context)
{
    CodeFixProvider provider;
    auto diagnostics = provider.GetDiagnostics(context);
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    std::vector<FileTextChanges> fileTextChanges;

    for (auto &diag : diagnostics->diagnostic) {
        auto code = std::get<int>(diag.code_);
        if (code != G_FIX_EXPECTED_COMMA) {
            continue;
        }

        auto changes = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
            MakeChange(tracker, context.context, diag.range_, diag.source_);
        });

        fileTextChanges.insert(fileTextChanges.end(), changes.begin(), changes.end());
    }

    return fileTextChanges;
}

FixExpectedComma::FixExpectedComma()
{
    const char *fixId = "FixExpectedComma";
    SetErrorCodes({G_FIX_EXPECTED_COMMA});
    SetFixIds({fixId});
}

std::vector<CodeFixAction> FixExpectedComma::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;

    auto changes = GetCodeActionsToFix(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = "fixExpectedComma";
        codeAction.description = "Use comma instead of semicolon at possition";
        codeAction.changes = changes;
        codeAction.fixId = "FixExpectedComma";
        returnedActions.push_back(codeAction);
    }

    return returnedActions;
}

CombinedCodeActions FixExpectedComma::GetAllCodeActions(const CodeFixAllContext &codeFixAll)
{
    std::vector<ark::es2panda::ir::AstNode *> fixedNodes;
    CodeFixProvider provider;

    return provider.GetAllFixes(codeFixAll);
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
}  // namespace ark::es2panda::lsp
