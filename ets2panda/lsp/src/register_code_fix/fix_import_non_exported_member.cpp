/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
#include "lsp/include/register_code_fix/fix_import_non_exported_member.h"
#include <string>
#include <unordered_set>

#include "compiler/lowering/util.h"
#include "generated/code_fix_register.h"
#include "ir/module/importSpecifier.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "parser/program/program.h"

namespace ark::es2panda::lsp {
using codefixes::FIX_IMPORT_NON_EXPORTED_MEMBER;
namespace {
std::vector<std::string> CollectMissingImports(const ir::ETSImportDeclaration *importDecl)
{
    if (importDecl == nullptr) {
        return {};
    }
    std::unordered_set<std::string> names;
    names.reserve(importDecl->Specifiers().size());
    for (auto *specifier : importDecl->Specifiers()) {
        if (specifier == nullptr || !specifier->IsImportSpecifier()) {
            continue;
        }
        auto *importSpecifier = specifier->AsImportSpecifier();
        auto *imported = importSpecifier->Imported();
        if (imported == nullptr) {
            continue;
        }
        auto *var = imported->Variable();
        if (var == nullptr || var->Declaration() == nullptr || var->Declaration()->Node() == nullptr) {
            continue;
        }

        auto *declNode = var->Declaration()->Node();
        if (declNode->IsExported() || declNode->IsDefaultExported() || declNode->HasExportAlias() ||
            declNode->IsImportSpecifier() || declNode->IsETSImportDeclaration()) {
            continue;
        }

        names.emplace(imported->Name().Utf8());
    }

    std::vector<std::string> result;
    result.reserve(names.size());
    for (auto &name : names) {
        result.emplace_back(std::move(name));
    }
    return result;
}
}  // namespace

void FixImportNonExportedMember::MakeChangeForImportNonExportedMember(ChangeTracker &changeTracker,
                                                                      es2panda_Context *context, size_t pos)
{
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr) {
        return;
    }

    auto *importDeclNode = token;
    util::StringView functionName;

    if (!FindImportDeclaration(importDeclNode)) {
        return;
    }

    if (!FindFunctionName(token, importDeclNode, functionName)) {
        if (!importDeclNode->IsETSImportDeclaration()) {
            return;
        }
        auto *importDecl = importDeclNode->AsETSImportDeclaration();
        auto missingImports = CollectMissingImports(importDecl);
        if (missingImports.empty()) {
            return;
        }
        for (const auto &missingName : missingImports) {
            ProcessExportPosition(context, importDecl, util::StringView(missingName), changeTracker);
        }
        return;
    }

    if (importDeclNode->IsETSImportDeclaration()) {
        auto funcDecl = importDeclNode->AsETSImportDeclaration();
        if (!funcDecl->Specifiers().empty()) {
            ProcessExportPosition(context, funcDecl, functionName, changeTracker);
        }
    }
}

bool FixImportNonExportedMember::FindImportDeclaration(ir::AstNode *&importDeclNode)
{
    while (importDeclNode != nullptr && !importDeclNode->IsETSImportDeclaration()) {
        importDeclNode = importDeclNode->Parent();
    }
    return (importDeclNode != nullptr);
}

bool FixImportNonExportedMember::FindFunctionName(ir::AstNode *tokenNode, ir::AstNode *importDeclNode,
                                                  util::StringView &functionName)
{
    if (tokenNode == nullptr || importDeclNode == nullptr) {
        return false;
    }

    auto *specifierNode = tokenNode;
    while (specifierNode != nullptr && !specifierNode->IsImportSpecifier()) {
        specifierNode = specifierNode->Parent();
    }

    if (specifierNode != nullptr && specifierNode->IsImportSpecifier()) {
        auto *imported = specifierNode->AsImportSpecifier()->Imported();
        if (imported != nullptr) {
            functionName = imported->Name();
        }
        return !functionName.Empty();
    }

    if (!importDeclNode->IsETSImportDeclaration()) {
        return false;
    }

    auto *importDecl = importDeclNode->AsETSImportDeclaration();
    ir::ImportSpecifier *singleSpecifier = nullptr;
    for (auto *specifier : importDecl->Specifiers()) {
        if (specifier == nullptr) {
            continue;
        }
        if (!specifier->IsImportSpecifier()) {
            return false;
        }
        if (singleSpecifier != nullptr) {
            return false;
        }
        singleSpecifier = specifier->AsImportSpecifier();
    }

    if (singleSpecifier == nullptr) {
        return false;
    }

    auto *imported = singleSpecifier->Imported();
    if (imported != nullptr) {
        functionName = imported->Name();
    }
    return !functionName.Empty();
}

void FixImportNonExportedMember::ProcessExportPosition(es2panda_Context *context, ir::AstNode *funcDecl,
                                                       const util::StringView &functionName,
                                                       ChangeTracker &changeTracker)
{
    if (context == nullptr || funcDecl == nullptr || !funcDecl->IsETSImportDeclaration()) {
        return;
    }

    auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    if (ctx == nullptr || ctx->parser == nullptr || ctx->parserProgram == nullptr) {
        return;
    }

    auto *targetProgram =
        ctx->parser->GetImportPathManager()->SearchResolved(funcDecl->AsETSImportDeclaration()->ImportMetadata());
    if (targetProgram == nullptr || targetProgram->Ast() == nullptr) {
        return;
    }

    ir::Statement *targetStatement = nullptr;
    targetProgram->Ast()->FindChild([&](ir::AstNode *n) {
        if (!n->IsIdentifier() || n->AsIdentifier()->Name() != functionName) {
            return false;
        }

        for (auto *current = n; current != nullptr; current = current->Parent()) {
            if (current->IsStatement()) {
                targetStatement = current->AsStatement();
                return true;
            }
        }
        return false;
    });

    if (targetStatement == nullptr) {
        return;
    }

    auto *targetSourceFile =
        ctx->Allocator()->New<SourceFile>(targetProgram->SourceFilePath().Utf8(), targetProgram->SourceCode());
    changeTracker.InsertExportModifier(targetSourceFile, targetStatement);
}

std::vector<FileTextChanges> FixImportNonExportedMember::GetCodeActionsToImportNonExportedMember(
    const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForImportNonExportedMember(tracker, context.context, context.span.start);
    });

    return fileTextChanges;
}

FixImportNonExportedMember::FixImportNonExportedMember()
{
    auto errorCodes = FIX_IMPORT_NON_EXPORTED_MEMBER.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_IMPORT_NON_EXPORTED_MEMBER.GetFixId().data()});
}

std::vector<CodeFixAction> FixImportNonExportedMember::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToImportNonExportedMember(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = FIX_IMPORT_NON_EXPORTED_MEMBER.GetFixId().data();
        codeAction.description = "Fix Import Non Exported Member";
        codeAction.changes = changes;
        codeAction.fixId = FIX_IMPORT_NON_EXPORTED_MEMBER.GetFixId().data();
        returnedActions.push_back(codeAction);
    }

    return returnedActions;
}

CombinedCodeActions FixImportNonExportedMember::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            MakeChangeForImportNonExportedMember(tracker, codeFixAllCtx.context, diag.GetStart());
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;

    return combinedCodeActions;
}

// NOLINTNEXTLINE
AutoCodeFixRegister<FixImportNonExportedMember> g_fixImportNonExportedMember(
    FIX_IMPORT_NON_EXPORTED_MEMBER.GetFixId().data());
}  // namespace ark::es2panda::lsp
