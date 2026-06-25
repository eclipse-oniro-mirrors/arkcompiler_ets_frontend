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

#include "lsp/include/register_code_fix/fix_remove_illegal_await.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_set>

#include "generated/code_fix_register.h"
#include "ir/base/methodDefinition.h"
#include "ir/ets/etsTypeReference.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/statements/functionDeclaration.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "public/public.h"
#include "util/helpers.h"

namespace ark::es2panda::lsp {

using codefixes::FIX_REMOVE_ILLEGAL_AWAIT;

namespace {
constexpr std::string_view ASYNC_KEYWORD = "async ";
constexpr std::string_view FUNCTION_KEYWORD = "function";
constexpr std::string_view FUNCTION_KEYWORD_WITH_SPACE = "function ";
constexpr std::string_view PROMISE_NAME = "Promise";
constexpr size_t INVALID_POS = static_cast<size_t>(-1);

struct IllegalAwaitFixInfo {
    size_t asyncInsertPos {INVALID_POS};
    ir::TypeNode *returnType {};
};

public_lib::Context *GetPublicContext(es2panda_Context *context)
{
    return reinterpret_cast<public_lib::Context *>(context);
}

ir::AwaitExpression *FindAwaitExpressionAt(es2panda_Context *context, size_t pos)
{
    auto *ctx = GetPublicContext(context);
    if (ctx == nullptr || ctx->parserProgram == nullptr || ctx->parserProgram->Ast() == nullptr) {
        return nullptr;
    }

    ir::AwaitExpression *awaitExpression = nullptr;
    ctx->parserProgram->Ast()->FindChild([pos, &awaitExpression](ir::AstNode *node) {
        if (node == nullptr || !node->IsAwaitExpression()) {
            return false;
        }

        if (node->Start().index <= pos && pos <= node->End().index) {
            awaitExpression = node->AsAwaitExpression();
            return true;
        }

        return false;
    });

    return awaitExpression;
}

ir::AstNode *FindFunctionEditNode(ir::ScriptFunction *function)
{
    ir::AstNode *candidate = nullptr;
    for (auto *node = function != nullptr ? function->Parent() : nullptr; node != nullptr; node = node->Parent()) {
        if (node->IsMethodDefinition() && node->AsMethodDefinition()->Function() == function) {
            return node;
        }

        if (candidate == nullptr &&
            (node->IsFunctionDeclaration() || node->IsFunctionExpression() || node->IsArrowFunctionExpression())) {
            candidate = node;
        }
    }

    return candidate;
}

bool StartsWith(std::string_view source, size_t pos, std::string_view text)
{
    return pos <= source.size() && text.size() <= source.size() - pos && source.substr(pos, text.size()) == text;
}

size_t FindFunctionKeywordPos(std::string_view source, size_t end)
{
    if (end > source.size()) {
        end = source.size();
    }

    const auto pos = source.substr(0, end).rfind(FUNCTION_KEYWORD_WITH_SPACE);
    return pos == std::string_view::npos ? INVALID_POS : pos;
}

bool HasOnlyTrivia(std::string_view source, size_t start, size_t end)
{
    if (start > end || end > source.size()) {
        return false;
    }

    while (start < end) {
        if (std::isspace(static_cast<unsigned char>(source[start])) != 0) {
            start++;
            continue;
        }

        if (start + 1 < end && source[start] == '/' && source[start + 1] == '/') {
            start += 2U;
            while (start < end && source[start] != '\n' && source[start] != '\r') {
                start++;
            }
            continue;
        }

        if (start + 1 < end && source[start] == '/' && source[start + 1] == '*') {
            start += 2U;
            while (start + 1 < end && !(source[start] == '*' && source[start + 1] == '/')) {
                start++;
            }
            if (start + 1 >= end) {
                return false;
            }
            start += 2U;
            continue;
        }

        return false;
    }

    return true;
}

size_t FindFunctionKeywordBeforeName(std::string_view source, size_t namePos)
{
    const auto functionKeywordPos = FindFunctionKeywordPos(source, namePos + 1);
    if (functionKeywordPos == INVALID_POS) {
        return INVALID_POS;
    }

    const auto functionKeywordEnd = functionKeywordPos + FUNCTION_KEYWORD_WITH_SPACE.size();
    return HasOnlyTrivia(source, functionKeywordEnd, namePos) ? functionKeywordPos : INVALID_POS;
}

size_t GetAsyncInsertPosition(std::string_view source, ir::ScriptFunction *function)
{
    auto *editNode = FindFunctionEditNode(function);
    if (editNode == nullptr) {
        return INVALID_POS;
    }

    if (editNode->IsArrowFunctionExpression()) {
        return editNode->Start().index;
    }

    if (editNode->IsMethodDefinition()) {
        auto *method = editNode->AsMethodDefinition();
        if (method->IsConstructor() || method->IsGetter() || method->IsSetter()) {
            return INVALID_POS;
        }
        auto *key = method->Key();
        if (key == nullptr) {
            return INVALID_POS;
        }

        const auto functionKeywordPos = FindFunctionKeywordBeforeName(source, key->Start().index);
        return functionKeywordPos == INVALID_POS ? key->Start().index : functionKeywordPos;
    }

    const auto searchEnd = std::max(function->Start().index, editNode->Start().index) + 1;
    auto functionKeywordPos = FindFunctionKeywordPos(source, searchEnd);
    if (functionKeywordPos != INVALID_POS) {
        return functionKeywordPos;
    }

    if (StartsWith(source, function->Start().index, FUNCTION_KEYWORD)) {
        return function->Start().index;
    }
    if (StartsWith(source, editNode->Start().index, FUNCTION_KEYWORD)) {
        return editNode->Start().index;
    }

    return INVALID_POS;
}

bool IsPromiseReturnTypeByName(ir::TypeNode *returnType)
{
    if (returnType == nullptr || !returnType->IsETSTypeReference()) {
        return false;
    }

    auto *part = returnType->AsETSTypeReference()->Part();
    if (part == nullptr || part->Name() == nullptr || !part->Name()->IsIdentifier()) {
        return false;
    }

    return std::string(part->Name()->AsIdentifier()->Name().Utf8()) == PROMISE_NAME;
}

bool IsPromiseType(checker::Type *type)
{
    if (type == nullptr || !type->IsETSObjectType()) {
        return false;
    }

    auto *baseType = type->AsETSObjectType()->GetOriginalBaseType();
    return baseType != nullptr && std::string(baseType->Name().Utf8()) == PROMISE_NAME;
}

bool IsPromiseReturnType(ir::TypeNode *returnType)
{
    if (returnType == nullptr) {
        return false;
    }

    return IsPromiseType(returnType->TsType()) || IsPromiseReturnTypeByName(returnType);
}

bool GetIllegalAwaitFixInfo(es2panda_Context *context, size_t pos, IllegalAwaitFixInfo &info)
{
    auto *awaitExpression = FindAwaitExpressionAt(context, pos);
    if (awaitExpression == nullptr) {
        return false;
    }

    auto *ancestor = util::Helpers::FindAncestorGivenByType(awaitExpression, ir::AstNodeType::SCRIPT_FUNCTION);
    if (ancestor == nullptr || !ancestor->IsScriptFunction()) {
        return false;
    }

    auto *function = ancestor->AsScriptFunction();
    if (function->IsDeclaredAsync() || function->Body() == nullptr) {
        return false;
    }

    if (awaitExpression->Start().index < function->Body()->Start().index ||
        awaitExpression->End().index > function->Body()->End().index) {
        return false;
    }

    auto *ctx = GetPublicContext(context);
    if (ctx == nullptr || ctx->parserProgram == nullptr) {
        return false;
    }

    const auto source = ctx->parserProgram->SourceCode();
    auto asyncInsertPos = GetAsyncInsertPosition(source, function);
    if (asyncInsertPos == INVALID_POS) {
        return false;
    }

    info.asyncInsertPos = asyncInsertPos;
    info.returnType = function->ReturnTypeAnnotation();
    return true;
}

void WrapReturnTypeInPromise(ChangeTracker &changeTracker, const SourceFile *sourceFile, std::string_view source,
                             ir::TypeNode *returnType)
{
    if (returnType == nullptr || IsPromiseReturnType(returnType)) {
        return;
    }

    const auto start = returnType->Start().index;
    const auto end = returnType->End().index;
    if (start >= end || end > source.size()) {
        return;
    }

    std::string newReturnType = std::string(PROMISE_NAME) + "<" + std::string(source.substr(start, end - start)) + ">";
    changeTracker.ReplaceRangeWithText(sourceFile, {start, end}, newReturnType);
}
}  // namespace

FixRemoveIllegalAwait::FixRemoveIllegalAwait()
{
    auto errorCodes = FIX_REMOVE_ILLEGAL_AWAIT.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_REMOVE_ILLEGAL_AWAIT.GetFixId().data()});
}

void FixRemoveIllegalAwait::MakeChangeForRemoveIllegalAwait(ChangeTracker &changeTracker, es2panda_Context *context,
                                                            size_t pos)
{
    IllegalAwaitFixInfo info;
    if (!GetIllegalAwaitFixInfo(context, pos, info)) {
        return;
    }

    auto *ctx = GetPublicContext(context);
    const auto source = ctx->parserProgram->SourceCode();
    changeTracker.InsertText(ctx->sourceFile, info.asyncInsertPos, std::string(ASYNC_KEYWORD));
    WrapReturnTypeInPromise(changeTracker, ctx->sourceFile, source, info.returnType);
}

std::vector<FileTextChanges> FixRemoveIllegalAwait::GetCodeActionsToRemoveIllegalAwait(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};
    return ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeForRemoveIllegalAwait(tracker, context.context, context.span.start);
    });
}

std::vector<CodeFixAction> FixRemoveIllegalAwait::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> returnedActions;
    auto changes = GetCodeActionsToRemoveIllegalAwait(context);
    if (!changes.empty()) {
        CodeFixAction codeAction;
        codeAction.fixName = FIX_REMOVE_ILLEGAL_AWAIT.GetFixId().data();
        codeAction.fixId = FIX_REMOVE_ILLEGAL_AWAIT.GetFixId().data();
        codeAction.fixAllDescription = "Add async modifier to all containing functions";
        codeAction.description = "Add async modifier to containing function";
        codeAction.changes = changes;
        returnedActions.push_back(std::move(codeAction));
    }

    return returnedActions;
}

CombinedCodeActions FixRemoveIllegalAwait::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    std::unordered_set<size_t> fixedFunctions;
    const auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            IllegalAwaitFixInfo info;
            if (!GetIllegalAwaitFixInfo(codeFixAllCtx.context, diag.GetStart(), info) ||
                !fixedFunctions.insert(info.asyncInsertPos).second) {
                return;
            }

            MakeChangeForRemoveIllegalAwait(tracker, codeFixAllCtx.context, diag.GetStart());
        });

    CombinedCodeActions combinedCodeActions;
    combinedCodeActions.changes = changes.changes;
    combinedCodeActions.commands = changes.commands;
    return combinedCodeActions;
}

// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoCodeFixRegister<FixRemoveIllegalAwait> g_fixRemoveIllegalAwait(FIX_REMOVE_ILLEGAL_AWAIT.GetFixId().data());

}  // namespace ark::es2panda::lsp
