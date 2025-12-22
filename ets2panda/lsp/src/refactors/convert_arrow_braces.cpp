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

#include "refactors/convert_arrow_braces.h"
#include "ir/astNode.h"
#include "ir/expression.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/returnStatement.h"
#include "ir/base/scriptFunction.h"
#include "internal_api.h"
#include "refactor_provider.h"
#include "services/text_change/change_tracker.h"
#include "util/helpers.h"

namespace ark::es2panda::lsp {

ConvertArrowBracesRefactor::ConvertArrowBracesRefactor()
{
    AddKind(std::string(ADD_BRACES_ACTION.kind));
    AddKind(std::string(REMOVE_BRACES_ACTION.kind));
}

static bool HasExpressionBody(ir::ScriptFunction *func)
{
    return (func->Flags() & ir::ScriptFunctionFlags::EXPRESSION) != 0;
}

static ir::ArrowFunctionExpression *FindArrowFunctionAtCursor(const RefactorContext &context)
{
    ir::AstNode *node = GetTouchingToken(context.context, context.span.pos, false);
    if (node == nullptr) {
        return nullptr;
    }

    ir::AstNode *current = node;
    while (current != nullptr && !current->IsArrowFunctionExpression()) {
        current = current->Parent();
    }

    if (current == nullptr || !current->IsArrowFunctionExpression()) {
        return nullptr;
    }

    return current->AsArrowFunctionExpression();
}

static bool ProcessExpressionBody(ArrowBracesInfo &result, ir::AstNode *body)
{
    result.addBraces = true;

    if (body->IsBlockStatement()) {
        auto *blockStmt = body->AsBlockStatement();
        const auto &statements = blockStmt->Statements();
        if (statements.size() == 1 && statements[0]->IsReturnStatement()) {
            result.expression = statements[0]->AsReturnStatement()->Argument();
        }
    }

    return true;
}

static bool ProcessBlockBody(ArrowBracesInfo &result, ir::AstNode *body)
{
    if (!body->IsBlockStatement()) {
        return false;
    }

    auto *blockStmt = body->AsBlockStatement();
    const auto &statements = blockStmt->Statements();

    if (statements.size() != 1) {
        return false;
    }

    ir::Statement *firstStmt = statements[0];
    if (!firstStmt->IsReturnStatement()) {
        return false;
    }

    auto *returnStmt = firstStmt->AsReturnStatement();
    result.addBraces = false;
    result.returnStatement = returnStmt;
    result.expression = returnStmt->Argument();

    return true;
}

static ArrowBracesInfo GetConvertibleArrowFunctionAtPosition(const RefactorContext &context)
{
    ArrowBracesInfo result;

    auto *arrowFunc = FindArrowFunctionAtCursor(context);
    if (arrowFunc == nullptr) {
        return result;
    }

    result.arrowFunc = arrowFunc;

    ir::ScriptFunction *func = arrowFunc->Function();
    if (func == nullptr) {
        return ArrowBracesInfo {};
    }

    ir::AstNode *body = func->Body();
    if (body == nullptr) {
        return ArrowBracesInfo {};
    }

    bool isExpressionBody = HasExpressionBody(func);
    if (isExpressionBody) {
        ProcessExpressionBody(result, body);
        return result;
    }

    if (!ProcessBlockBody(result, body)) {
        return ArrowBracesInfo {};
    }

    return result;
}

static bool NeedsParentheses(ir::Expression *expr)
{
    if (expr == nullptr) {
        return false;
    }

    if (expr->IsObjectExpression()) {
        return true;
    }

    if (expr->IsSequenceExpression()) {
        return true;
    }

    if (expr->IsAssignmentExpression()) {
        return true;
    }

    return false;
}

static std::string GenerateBlockBody(ir::AstNode *blockBody)
{
    std::string bodyContent = blockBody->DumpEtsSrc();
    return "{ " + bodyContent + " }";
}

static std::string GenerateExpressionBody(ir::ReturnStatement *returnStmt)
{
    ir::Expression *expr = returnStmt->Argument();

    if (expr == nullptr) {
        return "undefined";
    }

    std::string exprText = expr->DumpEtsSrc();

    if (NeedsParentheses(expr)) {
        return "(" + exprText + ")";
    }

    return exprText;
}

static RefactorAction CreateRefactorAction(bool addBraces)
{
    RefactorAction action;
    if (addBraces) {
        action.name = std::string(ADD_BRACES_ACTION.name);
        action.description = std::string(ADD_BRACES_ACTION.description);
        action.kind = std::string(ADD_BRACES_ACTION.kind);
    } else {
        action.name = std::string(REMOVE_BRACES_ACTION.name);
        action.description = std::string(REMOVE_BRACES_ACTION.description);
        action.kind = std::string(REMOVE_BRACES_ACTION.kind);
    }
    return action;
}

static bool IsKindMatching(const std::string &contextKind, const std::string &actionKind)
{
    if (contextKind.empty()) {
        return true;
    }
    return contextKind == actionKind;
}

std::vector<ApplicableRefactorInfo> ConvertArrowBracesRefactor::GetAvailableActions(
    const RefactorContext &context) const
{
    ArrowBracesInfo info = GetConvertibleArrowFunctionAtPosition(context);
    if (info.arrowFunc == nullptr) {
        return {};
    }

    RefactorAction availableAction = CreateRefactorAction(info.addBraces);
    if (!IsKindMatching(context.kind, availableAction.kind)) {
        return {};
    }

    ApplicableRefactorInfo infoResult;
    infoResult.name = std::string(refactor_name::CONVERT_ARROW_BRACES_REFACTOR_NAME);
    infoResult.description = std::string(refactor_description::CONVERT_ARROW_BRACES_REFACTOR_DESC);
    infoResult.action = availableAction;

    return {infoResult};
}

static bool ValidateRefactorAction(const std::string &actionName, const ArrowBracesInfo &info)
{
    bool isAddBraces = (actionName == std::string(ADD_BRACES_ACTION.name));
    bool isRemoveBraces = (actionName == std::string(REMOVE_BRACES_ACTION.name));
    if (!isAddBraces && !isRemoveBraces) {
        return false;
    }
    if (isAddBraces && !info.addBraces) {
        return false;
    }
    if (isRemoveBraces && info.addBraces) {
        return false;
    }

    return true;
}

static std::string GenerateNewBodyText(const std::string &actionName, const ArrowBracesInfo &info, ir::AstNode *oldBody)
{
    bool isAddBraces = (actionName == std::string(ADD_BRACES_ACTION.name));
    if (isAddBraces) {
        return GenerateBlockBody(oldBody);
    }

    return GenerateExpressionBody(info.returnStatement);
}

std::unique_ptr<RefactorEditInfo> ConvertArrowBracesRefactor::GetEditsForAction(const RefactorContext &context,
                                                                                const std::string &actionName) const
{
    ArrowBracesInfo info = GetConvertibleArrowFunctionAtPosition(context);
    if (info.arrowFunc == nullptr) {
        return nullptr;
    }

    if (!ValidateRefactorAction(actionName, info)) {
        return nullptr;
    }

    ir::AstNode *oldBody = info.arrowFunc->Function()->Body();
    std::string newBodyText = GenerateNewBodyText(actionName, info, oldBody);

    ChangeTracker tracker = ChangeTracker::FromContext(*context.textChangesContext);
    tracker.ReplaceNodeWithText(context.context, oldBody, newBodyText);

    auto changes = tracker.GetChanges();

    auto editInfo = std::make_unique<RefactorEditInfo>();
    for (const auto &change : changes) {
        editInfo->AddFileTextChange(change);
    }

    return editInfo;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ConvertArrowBracesRefactor> g_convertArrowBracesRefactorRegister("ConvertArrowBracesRefactor");

}  // namespace ark::es2panda::lsp
