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

#include "lsp/include/register_code_fix/fix_unreachable_code.h"
#include <iostream>
#include "generated/code_fix_register.h"
#include "lsp/include/api.h"
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

static TextRange GetStatementRange(ir::Statement *statement)
{
    // For block bodies, delete only the unreachable statements inside braces.
    if (statement == nullptr) {
        return {0, 0};
    }

    if (statement->IsBlockStatement()) {
        const auto &statements = statement->AsBlockStatement()->Statements();
        if (statements.empty()) {
            return {0, 0};
        }
        return {statements.front()->Start().index, statements.back()->End().index};
    }

    return {statement->Start().index, statement->End().index};
}

static ir::AstNode *FindControlStatement(ir::AstNode *statement)
{
    // Diagnostics may point at a nested token, so climb to the owning control statement.
    while (statement != nullptr) {
        if (statement->IsWhileStatement() || statement->IsIfStatement() || statement->IsForUpdateStatement()) {
            return statement;
        }
        statement = statement->Parent();
    }
    return nullptr;
}

static ir::Expression *GetControlStatementTest(ir::AstNode *statement)
{
    if (statement->IsWhileStatement()) {
        return statement->AsWhileStatement()->Test();
    }
    if (statement->IsIfStatement()) {
        return statement->AsIfStatement()->Test();
    }
    if (statement->IsForUpdateStatement()) {
        return statement->AsForUpdateStatement()->Test();
    }
    return nullptr;
}

static bool IsFalseLikeExpression(ir::Expression *expr)
{
    // Match the literal false-like tests that make a control body unreachable.
    if (expr == nullptr) {
        return false;
    }
    if (expr->IsBooleanLiteral()) {
        return !expr->AsBooleanLiteral()->Value();
    }
    if (expr->IsNumberLiteral()) {
        return expr->AsNumberLiteral()->Number().IsZero();
    }
    if (expr->IsStringLiteral()) {
        return expr->AsStringLiteral()->ToString().empty();
    }
    if (expr->IsCharLiteral()) {
        return expr->AsCharLiteral()->ToString().empty();
    }
    return expr->IsNullLiteral() || expr->IsUndefinedLiteral();
}

static TextRange GetUnreachableBodyRange(ir::AstNode *statement)
{
    // Prefer removing the unreachable body while preserving the surrounding control form.
    if (statement->IsIfStatement()) {
        auto *ifStmt = statement->AsIfStatement();
        if (ifStmt->Consequent()->IsBlockStatement()) {
            return GetStatementRange(ifStmt->Consequent());
        }
        if (ifStmt->Alternate() == nullptr) {
            return {statement->Start().index, statement->End().index};
        }
        return {0, 0};
    }
    if (statement->IsWhileStatement()) {
        auto *body = statement->AsWhileStatement()->Body();
        return body->IsBlockStatement() ? GetStatementRange(body)
                                        : TextRange {statement->Start().index, statement->End().index};
    }
    if (statement->IsForUpdateStatement()) {
        auto *body = statement->AsForUpdateStatement()->Body();
        return body->IsBlockStatement() ? GetStatementRange(body)
                                        : TextRange {statement->Start().index, statement->End().index};
    }
    return {0, 0};
}

static bool HasUnreachableDiagnosticAtPosition(es2panda_Context *context, size_t pos)
{
    // Confirm token-level fixes against diagnostics to avoid deleting unrelated statements.
    auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto *parserProgram = ctx->parserProgram;
    auto index = lexer::LineIndex(parserProgram->SourceCode());
    auto isTargetDiagnostic = [pos, &index, parserProgram](const Diagnostic &diagnostic) {
        if (!std::holds_alternative<int>(diagnostic.code_)) {
            return false;
        }

        auto errorCodes = FIX_UNREACHABLE_CODE.GetSupportedCodeNumbers();
        if (std::find(errorCodes.begin(), errorCodes.end(), std::get<int>(diagnostic.code_)) == errorCodes.end()) {
            return false;
        }
        const auto start = index.GetOffset(
            lexer::SourceLocation(diagnostic.range_.start.line_, diagnostic.range_.start.character_, parserProgram));
        return start == pos;
    };

    LSPAPI const *lspApi = GetImpl();
    auto semanticDiagnostics = lspApi->getSemanticDiagnostics(context);
    auto syntacticDiagnostics = lspApi->getSyntacticDiagnostics(context);
    auto suggestionDiagnostics = lspApi->getSuggestionDiagnostics(context);
    return std::any_of(semanticDiagnostics.diagnostic.begin(), semanticDiagnostics.diagnostic.end(),
                       isTargetDiagnostic) ||
           std::any_of(syntacticDiagnostics.diagnostic.begin(), syntacticDiagnostics.diagnostic.end(),
                       isTargetDiagnostic) ||
           std::any_of(suggestionDiagnostics.diagnostic.begin(), suggestionDiagnostics.diagnostic.end(),
                       isTargetDiagnostic);
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

    // Find the terminator statement before this unreachable statement
    int terminatorIdx = -1;
    for (int j = idx - 1; j >= 0; --j) {
        const ir::AstNode *prev = stmts[j];
        if (prev != nullptr && IsTerminatorStmt(prev)) {
            terminatorIdx = j;
            break;
        }
    }
    if (terminatorIdx < 0) {
        return {0, 0};
    }

    // Collect all unreachable statements after the terminator (batch delete)
    size_t rangeStart = stmt->Start().index;
    size_t rangeEnd = stmt->End().index;
    for (size_t k = static_cast<size_t>(terminatorIdx) + 1; k < stmts.size(); ++k) {
        if (stmts[k] != nullptr && stmts[k]->End().index > rangeEnd) {
            rangeEnd = stmts[k]->End().index;
        }
    }

    return {rangeStart, rangeEnd};
}

TextRange FixUnreachableCode::HandleUnreachableStatement(ir::AstNode *statement)
{
    statement = FindControlStatement(statement);
    if (statement == nullptr) {
        return {0, 0};
    }

    auto *expr = GetControlStatementTest(statement);
    if (IsFalseLikeExpression(expr)) {
        return GetUnreachableBodyRange(statement);
    }
    return {0, 0};
}

void FixUnreachableCode::MakeChangeForUnreachableCode(ChangeTracker &changeTracker, es2panda_Context *context,
                                                      size_t pos)
{
    TextRange range = {0, 0};
    auto *token = GetTouchingTokenRightMatch(context, pos);
    if (token == nullptr) {
        return;
    }

    while (token != nullptr && !token->IsStatement()) {
        token = token->Parent();
    }

    if (token == nullptr) {
        return;
    }

    auto *controlStatement = FindControlStatement(token);
    if (controlStatement != nullptr && controlStatement->IsIfStatement()) {
        auto *ifStmt = controlStatement->AsIfStatement();
        if (IsFalseLikeExpression(ifStmt->Test()) && !ifStmt->Consequent()->IsBlockStatement() &&
            ifStmt->Alternate() != nullptr) {
            auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
            const auto consequentRange =
                TextRange {ifStmt->Consequent()->Start().index, ifStmt->Consequent()->End().index};
            changeTracker.ReplaceRangeWithText(ctx->sourceFile, consequentRange, "{}");
            return;
        }
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

    if (pos == token->Start().index && HasUnreachableDiagnosticAtPosition(context, pos)) {
        auto ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
        changeTracker.DeleteRange(ctx->sourceFile, {token->Start().index, token->End().index});
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
