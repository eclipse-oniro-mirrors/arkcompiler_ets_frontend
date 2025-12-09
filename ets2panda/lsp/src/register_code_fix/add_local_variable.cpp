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

#include "lsp/include/register_code_fix/add_local_variable.h"

#include <string>
#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "ir/astNode.h"
#include "ir/base/classDefinition.h"
#include "ir/expressions/identifier.h"
#include "ir/statements/functionDeclaration.h"
#include "ir/base/methodDefinition.h"

namespace ark::es2panda::lsp {
using codefixes::ADD_LOCAL_VARIABLE;
using codefixes::ADD_LOCAL_VARIABLE_FOR_CLASS;

namespace {
constexpr const char *OBJECT_TYPE = "Object";
constexpr const char *DOUBLE_TYPE = "Double";
constexpr const char *STRING_TYPE = "String";
constexpr const char *BOOLEAN_TYPE = "Boolean";
constexpr const char *BIGINT_TYPE = "BigInt";
constexpr const char *CHAR_TYPE = "Char";
constexpr const char *ARRAY_SUFFIX = "[]";
constexpr const char *LET_KEYWORD = "let ";
constexpr const char *TYPE_SEPARATOR = ": ";
constexpr const char *STATEMENT_TERMINATOR = ";";
constexpr const char *INDENT = "  ";
constexpr const char *CLASS_FIELD_DESCRIPTION = "Add class field declaration";
constexpr const char *CLASS_FIELD_FIX_ALL_DESCRIPTION = "Add all missing class fields";
constexpr const char *LOCAL_VARIABLE_DESCRIPTION = "Add local variable declaration";
constexpr const char *LOCAL_VARIABLE_FIX_ALL_DESCRIPTION = "Add all missing variable declarations";
}  // namespace

std::string AddLocalVariable::GetTypeFromDirectAssignment(ir::AstNode *unresolvedNode, ir::AstNode *parent)
{
    if (!parent->IsAssignmentExpression()) {
        return "";
    }

    auto *assignment = parent->AsAssignmentExpression();
    if (assignment->Left() != unresolvedNode) {
        return "";
    }

    auto *rightSide = assignment->Right();
    if (rightSide == nullptr) {
        return "";
    }

    return InferTypeFromExpression(rightSide);
}

std::string AddLocalVariable::GetTypeFromMemberAssignment(ir::AstNode *unresolvedNode, ir::AstNode *parent)
{
    if (!parent->IsMemberExpression()) {
        return "";
    }

    auto *memberExpr = parent->AsMemberExpression();
    if (memberExpr->Property() != unresolvedNode) {
        return "";
    }

    auto *memberParent = memberExpr->Parent();
    if (memberParent == nullptr || !memberParent->IsAssignmentExpression()) {
        return "";
    }

    auto *assignment = memberParent->AsAssignmentExpression();
    if (assignment->Left() != memberExpr) {
        return "";
    }

    auto *rightSide = assignment->Right();
    if (rightSide == nullptr) {
        return "";
    }

    return InferTypeFromExpression(rightSide);
}

std::string AddLocalVariable::DetermineVariableType(ir::AstNode *unresolvedNode)
{
    if (unresolvedNode == nullptr) {
        return OBJECT_TYPE;
    }

    auto *parent = unresolvedNode->Parent();
    if (parent == nullptr) {
        return OBJECT_TYPE;
    }

    std::string directType = GetTypeFromDirectAssignment(unresolvedNode, parent);
    if (!directType.empty()) {
        return directType;
    }

    std::string memberType = GetTypeFromMemberAssignment(unresolvedNode, parent);
    if (!memberType.empty()) {
        return memberType;
    }

    return OBJECT_TYPE;
}

std::string AddLocalVariable::InferTypeFromLiteral(ir::AstNode *expression)
{
    if (expression->IsNumberLiteral()) {
        return DOUBLE_TYPE;
    }

    if (expression->IsStringLiteral()) {
        return STRING_TYPE;
    }

    if (expression->IsBooleanLiteral()) {
        return BOOLEAN_TYPE;
    }

    if (expression->IsBigIntLiteral()) {
        return BIGINT_TYPE;
    }

    if (expression->IsCharLiteral()) {
        return CHAR_TYPE;
    }

    if (expression->IsNullLiteral()) {
        return OBJECT_TYPE;
    }

    if (expression->IsUndefinedLiteral()) {
        return OBJECT_TYPE;
    }

    return "";
}

std::string AddLocalVariable::InferTypeFromBinaryExpression(ir::AstNode *expression)
{
    auto *binary = expression->AsBinaryExpression();
    auto leftType = InferTypeFromExpression(binary->Left());
    auto rightType = InferTypeFromExpression(binary->Right());
    if (leftType == BIGINT_TYPE || rightType == BIGINT_TYPE) {
        return BIGINT_TYPE;
    }
    if (leftType == DOUBLE_TYPE || rightType == DOUBLE_TYPE) {
        return DOUBLE_TYPE;
    }
    if (leftType == STRING_TYPE || rightType == STRING_TYPE) {
        return STRING_TYPE;
    }
    if (leftType == BOOLEAN_TYPE && rightType == BOOLEAN_TYPE) {
        return BOOLEAN_TYPE;
    }
    if (leftType == CHAR_TYPE || rightType == CHAR_TYPE) {
        return CHAR_TYPE;
    }

    return OBJECT_TYPE;
}

std::string AddLocalVariable::InferTypeFromOtherExpressions(ir::AstNode *expression)
{
    if (expression->IsArrayExpression()) {
        auto *arrayExpr = expression->AsArrayExpression();
        auto elements = arrayExpr->Elements();
        if (!elements.empty()) {
            auto elementType = InferTypeFromExpression(elements[0]);
            return elementType + std::string(ARRAY_SUFFIX);
        }
        return std::string(OBJECT_TYPE) + ARRAY_SUFFIX;
    }

    if (expression->IsCallExpression()) {
        return OBJECT_TYPE;
    }

    if (expression->IsObjectExpression()) {
        return OBJECT_TYPE;
    }

    if (expression->IsThisExpression()) {
        return OBJECT_TYPE;
    }

    if (expression->IsNewExpression()) {
        return OBJECT_TYPE;
    }

    return OBJECT_TYPE;
}

std::string AddLocalVariable::InferTypeFromComplexExpression(ir::AstNode *expression)
{
    if (expression->IsBinaryExpression()) {
        return InferTypeFromBinaryExpression(expression);
    }

    return InferTypeFromOtherExpressions(expression);
}

std::string AddLocalVariable::InferTypeFromExpression(ir::AstNode *expression)
{
    if (expression == nullptr) {
        return OBJECT_TYPE;
    }

    std::string literalType = InferTypeFromLiteral(expression);
    if (!literalType.empty()) {
        return literalType;
    }

    return InferTypeFromComplexExpression(expression);
}

std::string AddLocalVariable::GenerateVariableDeclaration(const std::string &variableName,
                                                          const std::string &variableType)
{
    return std::string(LET_KEYWORD) + variableName + TYPE_SEPARATOR + variableType + STATEMENT_TERMINATOR;
}

bool AddLocalVariable::IsThisPropertyAccess(es2panda_Context *context, size_t pos)
{
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr || !token->IsIdentifier()) {
        return false;
    }

    auto *parent = token->Parent();
    if (parent == nullptr) {
        return false;
    }

    if (parent->IsMemberExpression()) {
        auto *memberExpr = parent->AsMemberExpression();
        if (memberExpr->Object() != nullptr && memberExpr->Object()->IsThisExpression()) {
            return true;
        }
    }

    return false;
}

ir::AstNode *AddLocalVariable::FindClassInsertionPoint(ir::AstNode *current)
{
    while (current != nullptr) {
        if (current->IsClassDefinition()) {
            return current;
        }
        current = current->Parent();
    }
    return nullptr;
}

ir::AstNode *AddLocalVariable::FindFunctionInsertionPoint(ir::AstNode *current)
{
    while (current != nullptr) {
        if (current->IsBlockStatement()) {
            return current;
        }

        ir::AstNode *functionBody = GetFunctionBody(current);
        if (functionBody != nullptr) {
            return functionBody;
        }

        current = current->Parent();
    }
    return nullptr;
}

ir::AstNode *AddLocalVariable::GetFunctionBody(ir::AstNode *node)
{
    if (!node->IsFunctionDeclaration() && !node->IsMethodDefinition()) {
        return nullptr;
    }

    auto *functionNode = node->IsFunctionDeclaration() ? node->AsFunctionDeclaration()->Function()
                                                       : node->AsMethodDefinition()->Function();

    if (functionNode == nullptr || functionNode->Body() == nullptr) {
        return nullptr;
    }

    return functionNode->Body();
}

ir::AstNode *AddLocalVariable::FindInsertionPoint(ir::AstNode *unresolvedNode, bool isThisProperty)
{
    if (unresolvedNode == nullptr) {
        return nullptr;
    }

    if (isThisProperty) {
        return FindClassInsertionPoint(unresolvedNode);
    }

    return FindFunctionInsertionPoint(unresolvedNode);
}

void AddLocalVariable::MakeChangeForAddLocalVariable(ChangeTracker &changeTracker, es2panda_Context *context,
                                                     size_t pos)
{
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr || !token->IsIdentifier()) {
        return;
    }

    auto *identifier = token->AsIdentifier();
    std::string variableName = std::string(identifier->Name());

    bool isThisProperty = IsThisPropertyAccess(context, pos);

    auto *insertionPoint = FindInsertionPoint(token, isThisProperty);
    if (insertionPoint == nullptr) {
        return;
    }

    std::string variableType = DetermineVariableType(token);
    std::string declaration;
    size_t insertPos = insertionPoint->Start().index;

    auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    auto sourceCode = std::string(ctx->parserProgram->SourceCode());

    size_t bracePos = sourceCode.find('{', insertPos);
    if (bracePos != std::string::npos) {
        insertPos = bracePos + 1;
    }

    if (isThisProperty) {
        declaration = std::string(INDENT) + variableName + TYPE_SEPARATOR + variableType + STATEMENT_TERMINATOR;
    } else {
        declaration = std::string(INDENT) + GenerateVariableDeclaration(variableName, variableType);
    }

    TextRange insertRange = {insertPos, insertPos};
    auto astContext = reinterpret_cast<ark::es2panda::public_lib::Context *>(context);
    changeTracker.ReplaceRangeWithText(astContext->sourceFile, insertRange, declaration);
}

std::vector<FileTextChanges> AddLocalVariable::GetCodeActionsToAddLocalVariable(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForAddLocalVariable(tracker, context.context, context.span.start);
    });

    return fileTextChanges;
}

AddLocalVariable::AddLocalVariable()
{
    auto functionErrorCodes = ADD_LOCAL_VARIABLE.GetSupportedCodeNumbers();
    auto classErrorCodes = ADD_LOCAL_VARIABLE_FOR_CLASS.GetSupportedCodeNumbers();

    std::vector<int> allErrorCodes;
    allErrorCodes.insert(allErrorCodes.end(), functionErrorCodes.begin(), functionErrorCodes.end());
    allErrorCodes.insert(allErrorCodes.end(), classErrorCodes.begin(), classErrorCodes.end());

    SetErrorCodes(allErrorCodes);
    SetFixIds({ADD_LOCAL_VARIABLE.GetFixId().data(), ADD_LOCAL_VARIABLE_FOR_CLASS.GetFixId().data()});
}

std::vector<CodeFixAction> AddLocalVariable::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToAddLocalVariable(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;

        bool isThisProperty = IsThisPropertyAccess(context.context, context.span.start);
        if (isThisProperty) {
            codeAction.fixName = ADD_LOCAL_VARIABLE_FOR_CLASS.GetFixId().data();
            codeAction.fixId = ADD_LOCAL_VARIABLE_FOR_CLASS.GetFixId().data();
            codeAction.description = CLASS_FIELD_DESCRIPTION;
            codeAction.fixAllDescription = CLASS_FIELD_FIX_ALL_DESCRIPTION;
        } else {
            codeAction.fixName = ADD_LOCAL_VARIABLE.GetFixId().data();
            codeAction.fixId = ADD_LOCAL_VARIABLE.GetFixId().data();
            codeAction.description = LOCAL_VARIABLE_DESCRIPTION;
            codeAction.fixAllDescription = LOCAL_VARIABLE_FIX_ALL_DESCRIPTION;
        }

        codeAction.changes = changes;
        returnedActions.push_back(codeAction);
    }

    return returnedActions;
}

CombinedCodeActions AddLocalVariable::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            MakeChangeForAddLocalVariable(tracker, codeFixAllCtx.context, diag.GetStart());
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;

    return combinedCodeActions;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<AddLocalVariable> g_addLocalVariable(ADD_LOCAL_VARIABLE.GetFixId().data());

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<AddLocalVariable> g_addLocalVariableForClass(ADD_LOCAL_VARIABLE_FOR_CLASS.GetFixId().data());

}  // namespace ark::es2panda::lsp
