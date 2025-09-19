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

#include "refactors/convert_function.h"
#include <cstddef>
#include <string>
#include <vector>
#include "ir/astNode.h"
#include "ir/expression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/statements/functionDeclaration.h"
#include "public/es2panda_lib.h"
#include "refactor_provider.h"
#include "internal_api.h"
#include "refactors/refactor_types.h"
#include "services/text_change/change_tracker.h"
#include "services/text_change/text_change_context.h"
#include "types.h"

namespace ark::es2panda::lsp {
ConvertFunctionRefactor::ConvertFunctionRefactor()
{
    AddKind(std::string(TO_ANONYMOUS_FUNCTION_ACTION.kind));
    AddKind(std::string(TO_NAMED_FUNCTION_ACTION.kind));
    AddKind(std::string(TO_ARROW_FUNCTION_ACTION.kind));
}

bool HasArrowFunction(ark::es2panda::ir::AstNode *node)
{
    if (!node->IsCallExpression() && !node->IsClassProperty() && !node->IsVariableDeclarator()) {
        return false;
    }
    if ((node->IsClassProperty() && node->AsClassProperty()->Value() != nullptr &&
         node->AsClassProperty()->Value()->IsArrowFunctionExpression()) ||
        (node->IsVariableDeclarator() && node->AsVariableDeclarator()->Init() != nullptr &&
         node->AsVariableDeclarator()->Init()->IsArrowFunctionExpression())) {
        return true;
    }
    if (node->IsCallExpression()) {
        auto arguments = node->AsCallExpression()->Arguments();
        for (auto argument : arguments) {
            if (argument->IsArrowFunctionExpression()) {
                return true;
            }
        }
    }
    return false;
}

ir::Expression *TryGetFunctionFromVariableDeclaration(ir::AstNode *parent)
{
    if (!parent->IsVariableDeclaration()) {
        return nullptr;
    }
    const ir::VariableDeclaration *variableDeclaration = parent->AsVariableDeclaration();
    const auto initializer = variableDeclaration->Declarators().front()->Init();
    if (initializer != nullptr && (initializer->IsArrowFunctionExpression() || initializer->IsFunctionExpression())) {
        return initializer;
    }
    return nullptr;
}
bool ContainingThis(ir::AstNode *node)
{
    auto isThis = [](ir::AstNode *n) { return n->IsThisExpression(); };
    return isThis(node);
}
bool StartEndContainsRange(size_t start, size_t end, TextRange range)
{
    return start <= range.pos && end >= range.end;
}

bool RangeContainsRange(TextRange r1, TextRange r2)
{
    return StartEndContainsRange(r1.pos, r1.end, r2);
}

FunctionInfo GetFunctionInfo(es2panda_Context *context, const size_t startPosition)
{
    const auto token = GetTouchingToken(context, startPosition, false);
    if (token == nullptr) {
        return {};
    }
    const auto func = TryGetFunctionFromVariableDeclaration(token);
    if (func != nullptr && !ContainingThis(func)) {
        return {true, func};
    }
    auto funcDecl = token;
    while (funcDecl != nullptr && !funcDecl->IsFunctionDeclaration() && !funcDecl->IsArrowFunctionExpression()) {
        funcDecl = funcDecl->Parent();
    }
    if (funcDecl == nullptr) {
        return {};
    }
    const auto maybeFunc =
        funcDecl->IsFunctionDeclaration() || funcDecl->IsArrowFunctionExpression() ? funcDecl : nullptr;
    if (maybeFunc != nullptr && !ContainingThis(maybeFunc)) {
        return {false, maybeFunc};
    }

    return {};
}

std::optional<VariableInfo> GetVariableInfo(ir::AstNode *func)
{
    if (func == nullptr) {
        return std::nullopt;
    }

    ir::AstNode *variableDeclaration = func->Parent();
    while (variableDeclaration != nullptr && !variableDeclaration->IsVariableDeclaration()) {
        variableDeclaration = variableDeclaration->Parent();
    }

    if (variableDeclaration == nullptr || !variableDeclaration->IsVariableDeclaration()) {
        return std::nullopt;
    }

    ir::AstNode *statement = variableDeclaration->Parent();
    if (statement == nullptr || !statement->IsStatement()) {
        return std::nullopt;
    }

    ir::AstNode *name = variableDeclaration->AsVariableDeclaration()->Declarators().front()->Id();
    if (name == nullptr || !name->IsIdentifier()) {
        return std::nullopt;
    }
    VariableInfo variableInfo = {variableDeclaration->AsVariableDeclaration(), statement->AsStatement(),
                                 name->AsIdentifier()};

    return variableInfo;
}

ApplicableRefactorInfo ConvertFunctionRefactor::GetAvailableActions(const RefactorContext &refContext) const
{
    es2panda_Context *context = refContext.context;
    size_t position = refContext.span.pos;

    ApplicableRefactorInfo res;

    if (!IsKind(refContext.kind)) {
        return res;
    }

    auto node = GetTouchingToken(context, position, false);
    if (node == nullptr) {
        return res;
    }
    auto cb = [](ir::AstNode *ancestorNode) { return HasArrowFunction(ancestorNode); };
    auto ancestor = FindAncestor(node, cb);
    if (ancestor != nullptr && ancestor->IsClassProperty()) {
        res.name = refactor_name::CONVERT_FUNCTION_REFACTOR_NAME;
        res.description = refactor_description::CONVERT_FUNCTION_REFACTOR_DESC;
        res.action.kind = std::string(TO_NAMED_FUNCTION_ACTION.kind);
        res.action.name = std::string(TO_NAMED_FUNCTION_ACTION.name);
        res.action.description = std::string(TO_NAMED_FUNCTION_ACTION.description);
    }

    return res;
}

static es2panda_FunctionSignature *CreateFunctionSignature(es2panda_Context *ctx, es2panda_AstNode *scriptFunc)
{
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    size_t paramCount = 0;
    auto **params = impl->ScriptFunctionParams(ctx, scriptFunc, &paramCount);
    return impl->CreateFunctionSignature(ctx, nullptr, params, paramCount, nullptr, false);
}

static es2panda_AstNode *CreateFunctionBody(es2panda_Context *ctx, es2panda_AstNode *scriptFunc)
{
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    es2panda_AstNode *body = impl->ScriptFunctionBody(ctx, scriptFunc);
    if (body == nullptr) {
        return nullptr;
    }

    es2panda_AstNode *blockStmt = impl->CreateBlockStatement(ctx, nullptr, 0);
    es2panda_AstNode *returnStmt = impl->CreateReturnStatement(ctx);
    if (blockStmt == nullptr || returnStmt == nullptr) {
        return nullptr;
    }

    if (impl->IsBlockStatement(body)) {
        size_t stmtCount = 0;
        es2panda_AstNode **stmtsRaw = impl->BlockStatementStatements(ctx, body, &stmtCount);
        if (stmtsRaw != nullptr && stmtCount > 0) {
            Span<es2panda_AstNode *> stmtsView(stmtsRaw, stmtCount);
            std::vector<es2panda_AstNode *> stmts;
            stmts.reserve(stmtsView.size());
            for (auto *node : stmtsView) {
                stmts.push_back(node);
            }
            if (!stmts.empty() && impl->IsReturnStatement(stmts.front())) {
                es2panda_AstNode *returnArg = impl->ReturnStatementArgument(ctx, stmts.front());
                impl->ReturnStatementSetArgument(ctx, returnStmt, returnArg);
            }
        }
    } else if (impl->IsExpression(body)) {
        impl->ReturnStatementSetArgument(ctx, returnStmt, body);
    }

    std::vector<es2panda_AstNode *> newStmts = {returnStmt};
    auto *stmtsMem =
        static_cast<es2panda_AstNode **>(impl->AllocMemory(ctx, newStmts.size(), sizeof(es2panda_AstNode *)));

    Span<es2panda_AstNode *> stmtsMemView(stmtsMem, newStmts.size());
    std::copy(newStmts.begin(), newStmts.end(), stmtsMemView.begin());
    impl->BlockStatementSetStatements(ctx, blockStmt, stmtsMem, newStmts.size());
    return blockStmt;
}

static std::vector<es2panda_AstNode *> MakeVector(es2panda_AstNode **arr, size_t count)
{
    std::vector<es2panda_AstNode *> vec;
    vec.reserve(count);

    Span<es2panda_AstNode *> view(arr, count);
    for (auto *node : view) {
        if (node != nullptr) {
            vec.push_back(node);
        }
    }

    return vec;
}

std::vector<FileTextChanges> GetEditInfoForConvertToNamedFunction(RefactorContext context,
                                                                  ir::ArrowFunctionExpression *const arrow,
                                                                  VariableInfo info)
{
    const auto ctx = context.context;
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    if (arrow == nullptr || info.name == nullptr) {
        return {};
    }

    const auto nameStr = info.name->Name().Mutf8();
    auto *funcId = impl->CreateIdentifier(ctx);
    if (funcId == nullptr) {
        return {};
    }
    impl->IdentifierSetName(ctx, funcId, const_cast<char *>(nameStr.c_str()));
    auto *scriptFunc = impl->ArrowFunctionExpressionFunction(ctx, reinterpret_cast<es2panda_AstNode *>(arrow));
    auto *blockStmt = CreateFunctionBody(ctx, scriptFunc);
    auto *signature = CreateFunctionSignature(ctx, scriptFunc);
    if (scriptFunc == nullptr || blockStmt == nullptr || signature == nullptr) {
        return {};
    }

    auto *newScriptFunc =
        impl->CreateScriptFunction(ctx, blockStmt, reinterpret_cast<es2panda_FunctionSignature *>(signature), 0, 0);
    if (newScriptFunc == nullptr) {
        return {};
    }

    impl->ScriptFunctionSetIdent(ctx, newScriptFunc, funcId);
    auto *funcDecl = impl->CreateFunctionDeclaration(ctx, newScriptFunc, nullptr, 0, false);
    if (funcDecl == nullptr || !impl->IsFunctionDeclaration(funcDecl)) {
        return {};
    }

    impl->AstNodeSetParent(ctx, funcId, funcDecl);
    impl->AstNodeSetParent(ctx, blockStmt, funcDecl);
    size_t paramCount = 0;
    es2panda_AstNode **paramsRaw = impl->ScriptFunctionParams(ctx, scriptFunc, &paramCount);
    if (paramsRaw != nullptr && paramCount > 0) {
        for (auto *param : MakeVector(paramsRaw, paramCount)) {
            impl->AstNodeSetParent(ctx, param, funcDecl);
        }
    }

    TextChangesContext textChangesContext = {context.textChangesContext->host,
                                             context.textChangesContext->formatContext,
                                             context.textChangesContext->preferences};
    return ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        tracker.ReplaceNode(context.context, arrow, reinterpret_cast<ir::AstNode *>(funcDecl), {});
    });
}

std::vector<FileTextChanges> GetEditInfoForConvertToArrowFunction(RefactorContext context,
                                                                  ir::FunctionDeclaration *func)
{
    const auto impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    auto *funcId = impl->CreateIdentifier(context.context);
    if (funcId == nullptr) {
        return {};
    }
    impl->IdentifierSetName(context.context, funcId,
                            const_cast<char *>(func->Function()->Id()->Name().Mutf8().c_str()));

    auto *scriptFunc = reinterpret_cast<es2panda_AstNode *>(func->Function());
    auto *blockStmt = CreateFunctionBody(context.context, scriptFunc);
    auto *signature = CreateFunctionSignature(context.context, scriptFunc);
    if (signature == nullptr) {
        return {};
    }
    auto *newScriptFunc = impl->CreateScriptFunction(context.context, blockStmt,
                                                     reinterpret_cast<es2panda_FunctionSignature *>(signature),
                                                     Es2pandaScriptFunctionFlags::SCRIPT_FUNCTION_FLAGS_ARROW, 0);
    if (newScriptFunc == nullptr) {
        return {};
    }
    impl->ScriptFunctionSetIdent(context.context, newScriptFunc, funcId);
    auto *funcDecl = impl->CreateArrowFunctionExpression(context.context, newScriptFunc);
    if (funcDecl == nullptr || !impl->IsArrowFunctionExpression(funcDecl)) {
        return {};
    }
    TextChangesContext textChangesContext = {context.textChangesContext->host,
                                             context.textChangesContext->formatContext,
                                             context.textChangesContext->preferences};

    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        tracker.ReplaceNode(context.context, func, reinterpret_cast<ir::AstNode *>(funcDecl), {});
    });

    return fileTextChanges;
}

RefactorEditInfo GetRefactorEditsToConvertFunctionExpressions(RefactorContext &context, std::string_view actionName)
{
    const auto info = GetFunctionInfo(context.context, context.span.pos);
    if (info.func == nullptr) {
        return RefactorEditInfo {};
    }
    std::vector<FileTextChanges> edits;
    if (actionName == TO_NAMED_FUNCTION_ACTION.name) {
        const auto variableInfo = GetVariableInfo(info.func);
        if (!variableInfo.has_value()) {
            return RefactorEditInfo {};
        }
        if (!info.func->IsArrowFunctionExpression()) {
            return RefactorEditInfo {};
        }
        const auto edit =
            GetEditInfoForConvertToNamedFunction(context, info.func->AsArrowFunctionExpression(), variableInfo.value());
        if (edit.empty()) {
            return RefactorEditInfo {};
        }
        edits.insert(edits.end(), edit.begin(), edit.end());
    } else if (actionName == TO_ARROW_FUNCTION_ACTION.name) {
        const auto funcDecl = info.func;
        if (funcDecl == nullptr || !funcDecl->IsFunctionDeclaration()) {
            return RefactorEditInfo {};
        }
        const auto edit = GetEditInfoForConvertToArrowFunction(context, funcDecl->AsFunctionDeclaration());
        if (edit.empty()) {
            return RefactorEditInfo {};
        }
        edits.insert(edits.end(), edit.begin(), edit.end());
    }

    return RefactorEditInfo(edits);
}

std::unique_ptr<RefactorEditInfo> ConvertFunctionRefactor::GetEditsForAction(const RefactorContext &context,
                                                                             const std::string &actionName) const
{
    RefactorContext mutableCopyCtx = context;
    RefactorEditInfo refactor = GetRefactorEditsToConvertFunctionExpressions(mutableCopyCtx, actionName);
    return refactor.GetFileTextChanges().empty() ? nullptr : std::make_unique<RefactorEditInfo>(std::move(refactor));
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<ConvertFunctionRefactor> g_convertFunctionRefactorRegister("ConvertFunctionRefactor");

}  // namespace ark::es2panda::lsp
