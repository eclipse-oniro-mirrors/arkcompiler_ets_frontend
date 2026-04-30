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

#include "lsp/include/register_code_fix/fix_remove_duplicate_export_import.h"

#include <string>

#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "public/public.h"

namespace ark::es2panda::lsp {

using codefixes::FIX_REMOVE_DUPLICATE_EXPORT_IMPORT;

FixRemoveDuplicateExportImport::FixRemoveDuplicateExportImport()
{
    auto errorCodes = FIX_REMOVE_DUPLICATE_EXPORT_IMPORT.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_REMOVE_DUPLICATE_EXPORT_IMPORT.GetFixId().data()});
}

void FixRemoveDuplicateExportImport::MakeChangeForRemoveDuplicate(ChangeTracker &changeTracker,
                                                                  es2panda_Context *context, size_t start,
                                                                  size_t length)
{
    auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    changeTracker.DeleteRange(ctx->sourceFile, {start, start + length});
}

std::vector<FileTextChanges> FixRemoveDuplicateExportImport::GetCodeActionsToRemoveDuplicate(
    const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    return ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForRemoveDuplicate(tracker, context.context, context.span.start, context.span.length);
    });
}

std::vector<CodeFixAction> FixRemoveDuplicateExportImport::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToRemoveDuplicate(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = FIX_REMOVE_DUPLICATE_EXPORT_IMPORT.GetFixId().data();
        codeAction.fixId = FIX_REMOVE_DUPLICATE_EXPORT_IMPORT.GetFixId().data();
        codeAction.fixAllDescription = "Remove all duplicate exports/imports";
        codeAction.description = "Remove duplicate export/import";
        codeAction.changes = changes;
        returnedActions.push_back(std::move(codeAction));
    }

    return returnedActions;
}

CombinedCodeActions FixRemoveDuplicateExportImport::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            MakeChangeForRemoveDuplicate(tracker, codeFixAllCtx.context, diag.GetStart(), diag.Getlength());
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;
    return combinedCodeActions;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixRemoveDuplicateExportImport> g_fixRemoveDuplicateExportImport(
    FIX_REMOVE_DUPLICATE_EXPORT_IMPORT.GetFixId().data());

}  // namespace ark::es2panda::lsp
