/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ES2PANDA_LSP_CREATE_TYPE_HELP_ITEMS_H
#define ES2PANDA_LSP_CREATE_TYPE_HELP_ITEMS_H

#include "types.h"
#include <string>
#include "ir/astNode.h"
#include "lexer/token/sourceLocation.h"
#include "libarkbase/utils/arena_containers.h"
#include <checker/checker.h>
#include <checker/typeChecker/TypeChecker.h>
#include <utility>
#include <variant>
#include <vector>

namespace ark::es2panda::lsp {

enum class InvocationKind { CALL, TYPE_ARGS, CONTEXTUAL };

struct CallInvocation {
    InvocationKind kind = InvocationKind::CALL;
    ir::AstNode *callExpressionNode = nullptr;
};

struct TypeArgsInvocation {
    InvocationKind kind = InvocationKind::TYPE_ARGS;
    ir::AstNode *identifierNode = nullptr;
};

struct ContextualInvocation {
    InvocationKind kind = InvocationKind::CONTEXTUAL;
    checker::Signature *signature = nullptr;
    ir::AstNode *node = nullptr;
};

using Invocation = std::variant<CallInvocation, TypeArgsInvocation, ContextualInvocation>;

void GetLocalTypeParametersOfClassOrInterfaceOrTypeAlias(const ir::AstNode *node, std::vector<checker::Type *> &result);
std::vector<checker::Type *> GetEffectiveTypeParameterDeclarations(const ir::AstNode *node,
                                                                   std::vector<checker::Type *> &result);

void GetTypeHelpItem(std::vector<checker::Type *> *typeParameters, const ir::AstNode *node, SignatureHelpItem &result);

SignatureHelpItems CreateTypeHelpItems(const ir::AstNode *node, lexer::SourceRange location, TextSpan applicableSpan);

}  // namespace ark::es2panda::lsp

#endif  // ES2PANDA_LSP_CREATE_TYPE_HELP_ITEMS_H