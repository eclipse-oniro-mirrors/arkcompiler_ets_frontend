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
#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/register_code_fix/constructor_for_derived_need_super_call.h"

namespace ark::es2panda::lsp {
using codefixes::CONSTRUCTOR_DERIVED_NEED_SUPER;
ConstructorDerivedNeedSuper::ConstructorDerivedNeedSuper()
{
    auto errorCodes = CONSTRUCTOR_DERIVED_NEED_SUPER.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({CONSTRUCTOR_DERIVED_NEED_SUPER.GetFixId().data()});
}

bool ConstructorDerivedNeedSuper::IsValidTarget(const ir::AstNode *node)
{
    return node != nullptr && (node->IsETSStructDeclaration() || node->IsClassDeclaration());
}

const ir::AstNode *ConstructorDerivedNeedSuper::FindEnclosingClassNode(const ir::AstNode *start)
{
    const ir::AstNode *node = start;
    while (node != nullptr && !node->IsETSStructDeclaration() && !node->IsClassDeclaration()) {
        node = node->Parent();
    }
    return node;
}

ir::ClassDefinition *ConstructorDerivedNeedSuper::ExtractClassDefinition(const ir::AstNode *classNode)
{
    if (classNode->IsETSStructDeclaration()) {
        auto *structDecl = const_cast<ir::ETSStructDeclaration *>(classNode->AsETSStructDeclaration());
        return (structDecl != nullptr && structDecl->Definition() != nullptr) ? structDecl->Definition() : nullptr;
    }

    if (classNode->IsClassDeclaration()) {
        auto *classDecl = const_cast<ir::ClassDeclaration *>(classNode->AsClassDeclaration());
        if (classDecl == nullptr || classDecl->Definition() == nullptr || classDecl->Definition()->Super() == nullptr) {
            return nullptr;
        }
        return classDecl->Definition();
    }

    return nullptr;
}

ir::MethodDefinition *ConstructorDerivedNeedSuper::GetConstructorMethodFromDefinition(ir::ClassDefinition *definition)
{
    for (auto *member : definition->Body()) {
        if (!member->IsMethodDefinition()) {
            continue;
        }
        auto *method = member->AsMethodDefinition();
        if (method->Kind() == ir::MethodDefinitionKind::CONSTRUCTOR) {
            return method;
        }
    }
    return nullptr;
}

bool ConstructorDerivedNeedSuper::NeedsSuperCall(ir::ScriptFunction *scriptFunc)
{
    if (scriptFunc == nullptr || scriptFunc->Body() == nullptr || !scriptFunc->Body()->IsBlockStatement()) {
        return false;
    }

    auto *block = scriptFunc->Body()->AsBlockStatement();
    for (const auto *stmt : block->Statements()) {
        if (!stmt->IsExpressionStatement()) {
            continue;
        }
        const auto *exprStmt = stmt->AsExpressionStatement();
        const auto *expr = exprStmt->GetExpression();
        if (!expr->IsCallExpression()) {
            continue;
        }
        const auto *callExpr = expr->AsCallExpression();
        const auto *callee = callExpr->Callee();
        if (callee != nullptr && callee->IsSuperExpression()) {
            return false;
        }
    }
    return true;
}

ir::Statement *ConstructorDerivedNeedSuper::CreateSuperStatement(es2panda_Context *context)
{
    const auto *impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto *superExpr = impl->CreateSuperExpression(context);
    auto *superCall = impl->CreateCallExpression(context, superExpr, nullptr, 0, nullptr, false, false);
    auto *superStmt = impl->CreateExpressionStatement(context, superCall);

    impl->AstNodeSetParent(context, superExpr, superCall);
    impl->AstNodeSetParent(context, superCall, superStmt);

    auto *stmt = reinterpret_cast<ir::AstNode *>(superStmt);
    return stmt != nullptr && stmt->IsStatement() ? stmt->AsStatement() : nullptr;
}

void ConstructorDerivedNeedSuper::MakeChange(ChangeTracker &changeTracker, es2panda_Context *context, size_t pos,
                                             std::vector<ir::AstNode *> &fixedNodes)
{
    auto *token = GetTouchingToken(context, pos, false);
    if (token == nullptr) {
        return;
    }

    const ir::AstNode *classNode = FindEnclosingClassNode(token);
    if (!IsValidTarget(classNode)) {
        return;
    }

    ir::ClassDefinition *definition = ExtractClassDefinition(classNode);
    if (definition == nullptr) {
        return;
    }

    auto *ctorMethod = GetConstructorMethodFromDefinition(definition);
    if (ctorMethod == nullptr || ctorMethod->Value() == nullptr || !ctorMethod->Value()->IsFunctionExpression()) {
        return;
    }

    auto *funcExpr = ctorMethod->Value()->AsFunctionExpression();
    auto *scriptFunc = funcExpr->Function();
    if (!NeedsSuperCall(scriptFunc)) {
        return;
    }

    auto *superStatement = CreateSuperStatement(context);
    if (superStatement == nullptr) {
        return;
    }

    auto *block = scriptFunc->Body()->AsBlockStatement();
    changeTracker.InsertNodeAtConstructorStart(context, block, superStatement);
    fixedNodes.push_back(ctorMethod);
}

std::vector<FileTextChanges> ConstructorDerivedNeedSuper::GetCodeActionsToFix(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    std::vector<ir::AstNode *> fixedNodes;

    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChange(tracker, context.context, context.span.start, fixedNodes);
    });

    return fileTextChanges;
}

std::vector<CodeFixAction> ConstructorDerivedNeedSuper::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToFix(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = CONSTRUCTOR_DERIVED_NEED_SUPER.GetFixId().data();
        codeAction.description = "Add missing 'super()' call to derived constructor";
        codeAction.changes = changes;
        codeAction.fixId = CONSTRUCTOR_DERIVED_NEED_SUPER.GetFixId().data();
        returnedActions.push_back(codeAction);
    }
    return returnedActions;
}

CombinedCodeActions ConstructorDerivedNeedSuper::GetAllCodeActions([[maybe_unused]] const CodeFixAllContext &codeFixAll)
{
    return {};
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<ConstructorDerivedNeedSuper> g_constructorForDerivedNeedSuperCall(
    CONSTRUCTOR_DERIVED_NEED_SUPER.GetFixId().data());
}  // namespace ark::es2panda::lsp