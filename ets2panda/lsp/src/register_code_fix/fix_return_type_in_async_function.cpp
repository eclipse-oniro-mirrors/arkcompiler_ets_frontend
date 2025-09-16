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

#include "lsp/include/register_code_fix/fix_return_type_in_async_function.h"
#include "generated/code_fix_register.h"
#include "lsp/include/code_fix_provider.h"
#include "lsp/include/internal_api.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::lsp {
using codefixes::FIX_RETURN_TYPE_IN_ASYNC_FUNCTION;
constexpr size_t K_GENERIC_BRACKETS_LENGTH = 2;  // '<' and '>'
FixReturnTypeInAsyncFunction::FixReturnTypeInAsyncFunction()
{
    auto errorCodes = FIX_RETURN_TYPE_IN_ASYNC_FUNCTION.GetSupportedCodeNumbers();
    SetErrorCodes({errorCodes.begin(), errorCodes.end()});
    SetFixIds({FIX_RETURN_TYPE_IN_ASYNC_FUNCTION.GetFixId().data()});
}

ir::AstNode *FixReturnTypeInAsyncFunction::GetFunctionReturnType(es2panda_Context *context, size_t position)
{
    const auto token = lsp::GetDefinitionAtPositionImpl(context, position);
    const auto declaration = token.first;

    if (declaration == nullptr) {
        return nullptr;
    }

    auto *returnTypeNode =
        declaration->FindChild([](ir::AstNode *childNode) { return childNode->IsETSTypeReference(); });

    if (returnTypeNode == nullptr || !returnTypeNode->IsETSTypeReference()) {
        return nullptr;
    }

    return returnTypeNode;
}

void FixReturnTypeInAsyncFunction::MakeChangeReturnTypeInAsyncFunction(ChangeTracker &changeTracker,
                                                                       es2panda_Context *context, size_t pos)
{
    auto *returnTypeNode = GetFunctionReturnType(context, pos);

    if (returnTypeNode == nullptr || !returnTypeNode->IsETSTypeReference()) {
        return;
    }

    auto *ctx = reinterpret_cast<public_lib::Context *>(context);
    auto *allocator = ctx->Allocator();

    // Clone the original return type node
    auto *originalInnerType1 = returnTypeNode->Clone(allocator, returnTypeNode->Parent());
    // Create a new Identifier for "Promise"
    auto *promiseName = allocator->New<ir::Identifier>(util::StringView("Promise"), allocator);

    // Wrap the original type inside Promise<T>
    ArenaVector<ir::TypeNode *> typeParams(allocator->Adapter());
    auto *typedInner = static_cast<ir::TypeNode *>(originalInnerType1);
    typeParams.push_back(typedInner);

    auto *typeParamInstantiation = allocator->New<ir::TSTypeParameterInstantiation>(std::move(typeParams));

    // Create the type reference part: Promise<...>
    auto *promisePart = allocator->New<ir::ETSTypeReferencePart>(promiseName, typeParamInstantiation,
                                                                 nullptr,  // no previous part
                                                                 allocator);

    // Create the final ETSTypeReference node
    auto *newReturnType = allocator->New<ir::ETSTypeReference>(promisePart, allocator);

    newReturnType->SetParent(returnTypeNode->Parent());
    originalInnerType1->SetParent(newReturnType);
    newReturnType->SetStart(returnTypeNode->Start());
    auto returnTypeNodeIdent = returnTypeNode->AsETSTypeReference()->Part()->Name()->AsIdentifier();

    lexer::SourcePosition endPos(newReturnType->Start().index + promiseName->Name().Length() +
                                     returnTypeNodeIdent->Name().Length() + K_GENERIC_BRACKETS_LENGTH,
                                 returnTypeNode->Start().line, ctx->parserProgram);
    newReturnType->SetEnd(endPos);
    // Replace the old return type with the new Promise<T> type
    changeTracker.ReplaceNode(context, returnTypeNode, newReturnType, {});
}

std::vector<FileTextChanges> FixReturnTypeInAsyncFunction::GetChanges(const CodeFixContext &context)
{
    TextChangesContext textChangesContext = {context.host, context.formatContext, context.preferences};

    auto fileTextChanges = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
        MakeChangeReturnTypeInAsyncFunction(tracker, context.context, context.span.start);
    });

    return fileTextChanges;
}

std::vector<CodeFixAction> FixReturnTypeInAsyncFunction::GetCodeActions(const CodeFixContext &context)
{
    std::vector<CodeFixAction> actions;
    auto changes = GetChanges(context);
    if (!changes.empty()) {
        CodeFixAction action;
        action.fixName = FIX_RETURN_TYPE_IN_ASYNC_FUNCTION.GetFixId().data();
        action.description = "Wrap return type in Promise<T>";
        action.changes = changes;
        action.fixId = FIX_RETURN_TYPE_IN_ASYNC_FUNCTION.GetFixId().data();
        action.fixAllDescription = "Wrap all incorrect async return types with Promise";
        actions.push_back(action);
    }

    return actions;
}

CombinedCodeActions FixReturnTypeInAsyncFunction::GetAllCodeActions(const CodeFixAllContext &codeFixAllCtx)
{
    CodeFixProvider provider;
    const auto changes = provider.CodeFixAll(
        codeFixAllCtx, GetErrorCodes(), [&](ChangeTracker &tracker, const DiagnosticWithLocation &diag) {
            MakeChangeReturnTypeInAsyncFunction(tracker, codeFixAllCtx.context, diag.GetStart());
        });

    CombinedCodeActions combined;
    combined.changes = changes.changes;
    combined.commands = changes.commands;
    return combined;
}

// NOLINTNEXTLINE
AutoCodeFixRegister<FixReturnTypeInAsyncFunction> g_fixReturnTypeInAsyncFunction(
    FIX_RETURN_TYPE_IN_ASYNC_FUNCTION.GetFixId().data());

}  // namespace ark::es2panda::lsp