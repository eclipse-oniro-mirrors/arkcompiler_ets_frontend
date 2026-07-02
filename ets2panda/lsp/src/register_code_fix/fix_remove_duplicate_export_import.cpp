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
#include "generated/diagnostic.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "public/public.h"

namespace ark::es2panda::lsp {

using codefixes::FIX_REMOVE_DUPLICATE_EXPORT_IMPORT;

namespace {
constexpr char LINE_FEED = '\n';
constexpr std::string_view LINE_BREAK_CHARACTERS = "\r\n";
constexpr size_t START_OF_SOURCE = 0U;
constexpr size_t PREVIOUS_CHARACTER_OFFSET = 1U;

const ir::ETSImportDeclaration *FindImportAtPosition(public_lib::Context *ctx, size_t position)
{
    if (ctx == nullptr || ctx->parserProgram == nullptr) {
        return nullptr;
    }
    auto *ast = ctx->parserProgram->Ast();
    if (ast == nullptr) {
        return nullptr;
    }
    for (auto *statement : ast->Statements()) {
        if (statement != nullptr && statement->IsETSImportDeclaration() && statement->Start().index <= position &&
            position <= statement->End().index) {
            return statement->AsETSImportDeclaration();
        }
    }
    return nullptr;
}

void DeleteImportLine(ChangeTracker &changeTracker, public_lib::Context *ctx, size_t position)
{
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->sourceFile == nullptr) {
        return;
    }
    const std::string_view sourceCode = ctx->parserProgram->SourceCode();
    const size_t previousLineBreak = position == START_OF_SOURCE
                                         ? std::string_view::npos
                                         : sourceCode.rfind(LINE_FEED, position - PREVIOUS_CHARACTER_OFFSET);
    const size_t start = previousLineBreak == std::string_view::npos ? START_OF_SOURCE : previousLineBreak + 1U;
    const size_t lineBreak = sourceCode.find_first_of(LINE_BREAK_CHARACTERS, position);
    size_t end = lineBreak == std::string_view::npos ? sourceCode.size() : lineBreak;
    if (end < sourceCode.size() && sourceCode[end] == '\r') {
        ++end;
    }
    if (end < sourceCode.size() && sourceCode[end] == '\n') {
        ++end;
    }
    changeTracker.DeleteRange(ctx->sourceFile, {start, end});
}

bool IsSameIdentifier(const ir::Identifier *left, const ir::Identifier *right)
{
    return left != nullptr && right != nullptr && left->Name() == right->Name();
}

bool IsSameImportSpecifier(const ir::AstNode *left, const ir::AstNode *right)
{
    if (left == nullptr || right == nullptr || left->Type() != right->Type()) {
        return false;
    }
    if (left->IsImportSpecifier()) {
        auto *leftSpecifier = left->AsImportSpecifier();
        auto *rightSpecifier = right->AsImportSpecifier();
        return IsSameIdentifier(leftSpecifier->Imported(), rightSpecifier->Imported()) &&
               IsSameIdentifier(leftSpecifier->Local(), rightSpecifier->Local());
    }
    if (left->IsImportDefaultSpecifier()) {
        return IsSameIdentifier(left->AsImportDefaultSpecifier()->Local(), right->AsImportDefaultSpecifier()->Local());
    }
    if (left->IsImportNamespaceSpecifier()) {
        return IsSameIdentifier(left->AsImportNamespaceSpecifier()->Local(),
                                right->AsImportNamespaceSpecifier()->Local());
    }
    return false;
}

bool IsSameImportDeclaration(const ir::ETSImportDeclaration *left, const ir::ETSImportDeclaration *right)
{
    if (left->Source() == nullptr || right->Source() == nullptr || left->Source()->Str() != right->Source()->Str() ||
        left->IsTypeKind() != right->IsTypeKind() || left->Specifiers().size() != right->Specifiers().size()) {
        return false;
    }
    for (size_t index = 0; index < left->Specifiers().size(); ++index) {
        if (!IsSameImportSpecifier(left->Specifiers()[index], right->Specifiers()[index])) {
            return false;
        }
    }
    return true;
}

bool HasMatchingPreviousImport(public_lib::Context *ctx, size_t position)
{
    if (ctx == nullptr || ctx->parserProgram == nullptr) {
        return false;
    }
    auto *ast = ctx->parserProgram->Ast();
    if (ast == nullptr) {
        return false;
    }

    const auto *currentImport = FindImportAtPosition(ctx, position);
    if (currentImport == nullptr) {
        return false;
    }

    for (auto *statement : ast->Statements()) {
        if (statement == currentImport) {
            return false;
        }
        if (statement != nullptr && statement->IsETSImportDeclaration() &&
            IsSameImportDeclaration(statement->AsETSImportDeclaration(), currentImport)) {
            return true;
        }
    }
    return false;
}
}  // namespace

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
    if (HasMatchingPreviousImport(ctx, start)) {
        DeleteImportLine(changeTracker, ctx, start);
        return;
    }
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
    auto *ctx = reinterpret_cast<public_lib::Context *>(context.context);
    const auto *importDeclaration = ctx != nullptr ? FindImportAtPosition(ctx, context.span.start) : nullptr;
    if (context.errorCode == GetDiagnosticCode(diagnostic::DUPLICATE_IMPORT) && importDeclaration == nullptr) {
        return {};
    }
    if (context.errorCode == GetDiagnosticCode(diagnostic::DUPLICATE_EXPORT_ALIASES) && importDeclaration != nullptr) {
        return {};
    }

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
