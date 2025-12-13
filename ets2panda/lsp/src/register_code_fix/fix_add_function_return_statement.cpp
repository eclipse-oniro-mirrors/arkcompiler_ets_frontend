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

#include "lsp/include/register_code_fix/fix_add_function_return_statement.h"
#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "public/es2panda_lib.h"

namespace ark::es2panda::lsp {
using codefixes::FIX_ADD_FUNCTION_RETURN_STATEMENT;

FixAddFunctionReturnStatement::FixAddFunctionReturnStatement()
{
    auto errorCodes = FIX_ADD_FUNCTION_RETURN_STATEMENT.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});  // change this to the error code you want to handle
    SetFixIds({FIX_ADD_FUNCTION_RETURN_STATEMENT.GetFixId().data()});
}

std::vector<CodeFixAction> FixAddFunctionReturnStatement::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto info = GetInfo(context.context, context.span.start);
    if (info.GetReturnTypeNode() == nullptr || info.GetBody() == nullptr) {
        return returnedActions;  // No valid return type or body found
    }

    TextChangesContext textChangesContext {context.host, context.formatContext, context.preferences};
    auto replaceReturnTypeChanges = ChangeTracker::With(
        textChangesContext, [&](ChangeTracker &tracker) { ReplaceReturnType(tracker, context.context, info); });
    auto addReturnStatementChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        AddReturnStatement(tracker, context.context, info.GetStatements(), info.GetBody());
    });
    CodeFixAction action;
    action.fixName = FIX_ADD_FUNCTION_RETURN_STATEMENT.GetFixId().data();
    action.description = "Add missing return statement";
    action.fixId = FIX_ADD_FUNCTION_RETURN_STATEMENT.GetFixId().data();
    action.fixAllDescription = "Add all missing return statement";
    action.changes.insert(action.changes.end(), replaceReturnTypeChanges.begin(), replaceReturnTypeChanges.end());
    action.changes.insert(action.changes.end(), addReturnStatementChanges.begin(), addReturnStatementChanges.end());
    returnedActions.push_back(action);

    return returnedActions;
}

CombinedCodeActions FixAddFunctionReturnStatement::GetAllCodeActions([[maybe_unused]] const CodeFixAllContext &ctx)
{
    CombinedCodeActions combinedActions;
    return combinedActions;
}

Info GetInfo(es2panda_Context *context, size_t position)
{
    const auto token = GetDefinitionAtPositionImpl(context, position);
    const auto node = token.first;
    const auto declaration = FindAncessor(node);
    if (!declaration->IsFunctionExpression()) {
        return Info(nullptr, nullptr, {});
    }
    const auto returnTypeNode = declaration->AsFunctionExpression()->Function()->ReturnTypeAnnotation();
    if (returnTypeNode == nullptr || !returnTypeNode->IsETSTypeReference()) {
        return Info(nullptr, nullptr, {});
    }
    if (!declaration->AsFunctionExpression()->Function()->Body()->IsBlockStatement()) {
        return Info(nullptr, nullptr, {});
    }

    const auto body = declaration->AsFunctionExpression()->Function()->Body();
    const auto statements = body->AsBlockStatement()->Statements();
    return Info(returnTypeNode, body, {statements.begin(), statements.end()});
}

void ReplaceReturnType(ChangeTracker &changes, es2panda_Context *context, Info &info)
{
    auto ctx = reinterpret_cast<public_lib::Context *>(context);
    const auto &statements = info.GetStatements();
    bool statementFlag = false;
    for (const auto &statement : statements) {
        if (statement->IsReturnStatement()) {
            statementFlag = true;
            break;
        }
    }
    if (statementFlag) {
        return;
    }
    auto newNode = info.GetReturnTypeNode()->Clone(ctx->Allocator(), info.GetReturnTypeNode()->Parent());
    if (!newNode->IsETSTypeReference()) {
        return;  // Not a valid type reference node
    }
    auto typeRef = newNode->AsETSTypeReference();
    if (typeRef->Part()->IsETSTypeReferencePart()) {
        auto part = typeRef->Part()->AsETSTypeReferencePart();
        part->GetIdent()->SetName("void");  // Change the type to 'void'
    }

    changes.ReplaceNode(context, info.GetReturnTypeNode(), newNode, {});
}
void AddReturnStatement(ChangeTracker &changes, es2panda_Context *context, std::vector<ir::Statement *> statements,
                        ir::AstNode *body)
{
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    if (!statements.empty()) {
        // If the body is empty, we can add a return statement
        auto *returnStmt = impl->CreateReturnStatement(context);
        changes.InsertNodeAfter(context, statements[statements.size() - 1]->AsStatement(),
                                reinterpret_cast<ir::AstNode *>(returnStmt));
    } else {
        size_t newSize = statements.size() + 1;
        std::vector<es2panda_AstNode *> newStatementsVec(newSize, nullptr);
        for (size_t i = 0; i < statements.size(); ++i) {
            newStatementsVec[i] = reinterpret_cast<es2panda_AstNode *>(statements[i]->AsStatement());
        }
        newStatementsVec[statements.size()] = impl->CreateReturnStatement(context);
        auto newBody = impl->CreateBlockStatement(context, newStatementsVec.data(), statements.size());
        changes.ReplaceNode(context, body->AsBlockStatement(), reinterpret_cast<ir::AstNode *>(newBody), {});
    }
}

ir::AstNode *FindAncessor(ir::AstNode *node)
{
    if (node->IsFunctionDeclaration() || node->IsFunctionExpression()) {
        return node;
    }
    return FindAncessor(node->Parent());
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixAddFunctionReturnStatement> g_fixAddFunctionReturnStatement("FixAddFunctionReturnStatement");
}  // namespace ark::es2panda::lsp
