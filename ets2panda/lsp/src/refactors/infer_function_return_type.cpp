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

#include "refactors/infer_function_return_type.h"
#include <string>
#include <vector>
#include "es2panda.h"
#include "internal_api.h"
#include "ir/astNode.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "refactor_provider.h"
#include "refactors/refactor_types.h"
#include "services/text_change/change_tracker.h"
#include "services/text_change/text_change_context.h"

namespace ark::es2panda::lsp {

/**
 * @file infer_function_return_type.cpp
 * @brief Implements the "Infer Function Return Type" refactor for the Ark/ETS LSP service.
 *
 * This module provides the logic required to analyze a function declaration or expression
 * in an ETS/TypeScript source file and automatically insert an explicit return type
 * annotation based on the function body.
 *
 * <b>Key Responsibilities</b>
 * - Locate the target function AST node at the current refactor context (cursor position).
 * - Determine whether the function already has an explicit return type.
 * - If no return type is present, traverse the function body to collect the returned
 *   expression(s) and infer a suitable ETS/TypeScript type.
 * - Generate the appropriate text edit (e.g. inserting ": number", ": string", ": boolean",
 *   or a custom array/object type) and apply it to the source through the
 *   ChangeTracker service.
 *
 * <b>Main Components</b>
 * - @c GetInfoInferRet() : Scans the AST to find a convertible function declaration and
 *   returns both the declaration node and a representative return-expression node
 *   from which the type can be inferred.
 * - @c DoChanges() : Uses the ChangeTracker to insert the inferred return type string
 *   into the function signature at the correct source location.
 * - @c GetRefactorEditsToInferReturnType() : Entry point that wraps the change operation
 *   into a RefactorEditInfo structure to be consumed by the LSP refactor engine.
 * - @c InferFunctionRefactor() : Concrete refactor provider class that registers this
 *   feature with the global refactor framework (AutoRefactorRegister).
 *
 * <b>Usage</b>
 * When a user invokes the "Infer function return type" action from an IDE (for example,
 * via a context menu or quick-fix), the LSP server calls
 * GetRefactorActionsToInferReturnType() to check applicability and
 *
 * @note
 * - Supports normal functions, arrow functions, function expressions, and method
 *   definitions.
 * - Currently recognizes primitive literals (boolean, number, string), binary expressions,
 *   and some simple identifier references (including array type inference).
 *
 * @see ChangeTracker
 * @see RefactorContext
 */

InferFunctionRefactor::InferFunctionRefactor()
{
    AddKind(std::string(TO_INFER_FUNCTION_RETURN_TYPE.kind));
}

ir::AstNode *GetTypeNode(ir::AstNode *declaration)
{
    ir::AstNode *type = nullptr;
    if (declaration->Parent()->IsFunctionDeclaration()) {
        declaration->Parent()->AsFunctionDeclaration()->Function()->Body()->Iterate([&type](ir::AstNode *child) {
            if (child->IsReturnStatement()) {
                type = child->AsReturnStatement()->Argument();
            }
        });
    } else if (declaration->Parent()->IsFunctionExpression()) {
        declaration->Parent()->AsFunctionExpression()->Function()->Body()->Iterate([&type](ir::AstNode *child) {
            if (child->IsReturnStatement()) {
                type = child->AsReturnStatement()->Argument();
            }
        });
    } else if (declaration->Parent()->IsArrowFunctionExpression()) {
        type = declaration->FindChild([](ir::AstNode *child) { return child->IsReturnStatement(); });
        type = type == nullptr
                   ? declaration->Parent()->AsArrowFunctionExpression()->Function()->ReturnStatements().at(0)
                   : type;
        if (type != nullptr) {
            type = type->AsReturnStatement()->Argument();
        }
    } else if (declaration->Parent()->IsMethodDefinition()) {
        declaration->Parent()->AsMethodDefinition()->Function()->Body()->Iterate([&type](ir::AstNode *child) {
            if (child->IsReturnStatement()) {
                type = child->AsReturnStatement()->Argument();
            }
        });
    }
    return type;
}

std::string GetDeclaratorName(ir::AstNode *namedNode)
{
    std::string name;
    namedNode->FindChild([&name](ir::AstNode *typeChild) {
        if (typeChild->IsETSTypeReference()) {
            name = " : " + std::string(typeChild->AsETSTypeReference()->Part()->Name()->AsIdentifier()->Name()) + "[]";
            return true;
        }
        return false;
    });
    return name;
}

FunctionInfo GetInfoInferRet(const RefactorContext &context)
{
    auto token = GetTouchingToken(context.context, context.span.pos, false);
    auto cb = [](ir::AstNode *node) {
        return node->Parent()->IsArrowFunctionExpression() || node->Parent()->IsFunctionDeclaration() ||
               node->Parent()->IsFunctionExpression() || node->Parent()->IsFunctionDeclaration() ||
               node->Parent()->IsMethodDefinition();
    };
    const auto declaration = FindAncestor(token, cb);
    if (declaration == nullptr) {
        return {};
    }
    bool hasReturnType = false;
    if (declaration->IsScriptFunction() && declaration->AsScriptFunction()->ReturnTypeAnnotation() != nullptr) {
        hasReturnType = true;
    }
    if (declaration->IsETSFunctionType() && declaration->AsETSFunctionType()->ReturnType() != nullptr) {
        hasReturnType = true;
    }
    if (hasReturnType) {
        return {declaration, declaration};
    }
    return {declaration, GetTypeNode(declaration)};
}
void DoChanges(ChangeTracker &changes, es2panda_Context *context, ir::AstNode *declaration, ir::AstNode *typeNode)
{
    const auto startPos = declaration->AsScriptFunction()->Body()->Start().index - 1;
    std::string name;
    if (typeNode->Type() == ir::AstNodeType::BINARY_EXPRESSION) {
        name = " : number";
    } else if (typeNode->Type() == ir::AstNodeType::BOOLEAN_LITERAL) {
        name = " : boolean";
    } else if (typeNode->Type() == ir::AstNodeType::NUMBER_LITERAL) {
        name = " : number";
    } else if (typeNode->Type() == ir::AstNodeType::STRING_LITERAL) {
        name = " : string";
    } else if (typeNode->IsIdentifier()) {
        auto namedNode = declaration->FindChild([&typeNode](ir::AstNode *child) {
            return (child->IsVariableDeclarator() && child->AsVariableDeclarator()->Id()->IsIdentifier() &&
                    std::string(child->AsVariableDeclarator()->Id()->AsIdentifier()->Name()) ==
                        std::string(typeNode->AsIdentifier()->Name()));
        });
        if (namedNode == nullptr) {
            name = "";
            return;
        }
        if (namedNode->IsVariableDeclarator()) {
            name = GetDeclaratorName(namedNode);
        }
    }
    auto source = reinterpret_cast<public_lib::Context *>(context);
    changes.InsertText(source->sourceFile, startPos, name);
}

RefactorEditInfo GetRefactorEditsToInferReturnType(const RefactorContext &context)
{
    const auto info = GetInfoInferRet(context);
    std::vector<FileTextChanges> edits;
    if (info.declaration != nullptr) {
        TextChangesContext textChangesContext = *context.textChangesContext;
        edits = ChangeTracker::With(textChangesContext, [&](ChangeTracker &tracker) {
            DoChanges(tracker, context.context, info.declaration, info.returnTypeNode);
        });
    }
    RefactorEditInfo refactorEdits(std::move(edits));
    return refactorEdits;
}

ApplicableRefactorInfo InferFunctionRefactor::GetAvailableActions(const RefactorContext &refContext) const
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

    if (node->Parent() != nullptr && (node->Parent()->IsExpression() && node->Parent()->IsBinaryExpression())) {
        res.name = refactor_name::INFER_FUNCTION_RETURN_TYPE;
        res.description = refactor_description::INFER_FUNCTION_RETURN_TYPE_DESC;
        res.action.kind = std::string(TO_INFER_FUNCTION_RETURN_TYPE.kind);
        res.action.name = std::string(TO_INFER_FUNCTION_RETURN_TYPE.name);
        res.action.description = std::string(TO_INFER_FUNCTION_RETURN_TYPE.description);
    }
    return res;
}

std::unique_ptr<RefactorEditInfo> InferFunctionRefactor::GetEditsForAction(
    const RefactorContext &context, [[maybe_unused]] const std::string &actionName) const
{
    RefactorEditInfo refactor = GetRefactorEditsToInferReturnType(context);
    return refactor.GetFileTextChanges().empty() ? nullptr : std::make_unique<RefactorEditInfo>(std::move(refactor));
}
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects, cert-err58-cpp)
AutoRefactorRegister<InferFunctionRefactor> g_inferFunctionReturnType("InferFunctionRefactor");

}  // namespace ark::es2panda::lsp
