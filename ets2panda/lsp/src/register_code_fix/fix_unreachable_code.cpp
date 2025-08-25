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

#include "lsp/include/register_code_fix/fix_unreachable_code.h"
#include <iostream>
#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"

namespace ark::es2panda::lsp {

using codefixes::FIX_UNREACHABLE_CODE;

FixUnreachableCode::FixUnreachableCode()
{
    auto errorCodes = FIX_UNREACHABLE_CODE.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_UNREACHABLE_CODE.GetFixId().data()});
}

static inline bool IsTerminatorStmt(const ir::AstNode *s)
{
    return (s != nullptr) && (s->IsReturnStatement() || s->IsThrowStatement());
}

TextRange FixUnreachableCode::HandleUnreachableAfterTerminator(ir::AstNode *stmt)
{
    ir::AstNode *parent = stmt->Parent();
    if (parent == nullptr) {
        return {0, 0};
    }

    if (!parent->IsBlockStatement()) {
        return {0, 0};
    }

    auto *block = parent->AsBlockStatement();
    const auto &stmts = block->Statements();

    int idx = -1;
    for (int i = 0; i < static_cast<int>(stmts.size()); ++i) {
        if (stmts[i] == stmt) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        return {0, 0};
    }

    for (int j = idx - 1; j >= 0; --j) {
        const ir::AstNode *prev = stmts[j];
        if (prev != nullptr && IsTerminatorStmt(prev)) {
            return {stmt->Start().index, stmt->End().index};
        }
    }

    return {0, 0};
}

TextRange FixUnreachableCode::HandleUnreachableStatement(ir::AstNode *statement)
{
    if (statement == nullptr) {
        return {0, 0};
    }

    while (statement != nullptr) {
        if (statement->IsWhileStatement() || statement->IsIfStatement() || statement->IsForUpdateStatement()) {
            break;
        }
        statement = statement->Parent();
    }

    if (statement == nullptr) {
        return {0, 0};
    }

    ir::Expression *expr = nullptr;
    if (statement->IsWhileStatement()) {
        expr = statement->AsWhileStatement()->Test();
    } else if (statement->IsIfStatement()) {
        expr = statement->AsIfStatement()->Test();
    } else if (statement->IsForUpdateStatement()) {
        expr = statement->AsForUpdateStatement()->Test();
    }

    if (expr == nullptr) {
        return {0, 0};
    }

    if (expr->IsBooleanLiteral()) {
        auto boolLiteral = expr->AsBooleanLiteral();
        if (!boolLiteral->Value()) {
            return {statement->Start().index, statement->End().index};
        }
    } else if (expr->IsNumberLiteral()) {
        if (expr->AsNumberLiteral()->Number().IsZero()) {
            return {statement->Start().index, statement->End().index};
        }
    } else if (expr->IsStringLiteral()) {
        if (expr->AsStringLiteral()->ToString().empty()) {
            return {statement->Start().index, statement->End().index};
        }
    } else if (expr->IsCharLiteral()) {
        if (expr->AsCharLiteral()->ToString().empty()) {
            return {statement->Start().index, statement->End().index};
        }
    } else if (expr->IsNullLiteral()) {
        return {statement->Start().index, statement->End().index};
    }

    return {0, 0};
}

void FixUnreachableCode::MakeChangeForUnreachableCode(ChangeTracker &changeTracker, es2panda_Context *context,
                                                      size_t pos)
{
    TextRange range = {0, 0};
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr) {
        return;
    }

    while (token != nullptr && !token->IsStatement()) {
        token = token->Parent();
    }

    if (token == nullptr) {
        return;
    }

    range = HandleUnreachableStatement(token);
    if (range.pos != range.end) {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        changeTracker.DeleteRange(ctx->sourceFile, {range.pos, range.end});
        return;
    }

    range = HandleUnreachableAfterTerminator(token);
    if (range.pos != range.end) {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        changeTracker.DeleteRange(ctx->sourceFile, {range.pos, range.end});
        return;
    }
}

std::vector<FileTextChanges> FixUnreachableCode::GetCodeActionsToRemoveUnreachableCode(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    return ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForUnreachableCode(tracker, context.context, context.span.start);
    });
}

std::vector<CodeFixAction> FixUnreachableCode::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> actions;
    auto changes = GetCodeActionsToRemoveUnreachableCode(context);
    if (!changes.empty()) {
        CodeFixAction fix;
        fix.fixName = FIX_UNREACHABLE_CODE.GetFixId().data();
        fix.description = "Remove unreachable code";
        fix.fixAllDescription = "Remove all unreachable code";
        fix.changes = changes;
        fix.fixId = FIX_UNREACHABLE_CODE.GetFixId().data();
        actions.push_back(std::move(fix));
    }

    return actions;
}

CombinedCodeActions FixUnreachableCode::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            MakeChangeForUnreachableCode(tracker, codeFixAllCtx.context, diag.GetStart());
        });

    CombinedCodeActions combined;
    combined.changes = std::move(changes.changes);
    combined.commands = std::move(changes.commands);
    return combined;
}
// NOLINTNEXTLINE
AutoCodeFixRegister<FixUnreachableCode> g_fixUnreachableCode(FIX_UNREACHABLE_CODE.GetFixId().data());

}  // namespace ark::es2panda::lsp
