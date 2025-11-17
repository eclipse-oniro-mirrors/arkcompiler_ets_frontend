/*
 * Copyright (c) 2023 - 2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_LOWERING_UTIL_INL_H
#define ES2PANDA_COMPILER_LOWERING_UTIL_INL_H

#include "ir/statements/blockStatement.h"
#include "ir/statements/classDeclaration.h"
#include "ir/base/classDefinition.h"
#include "ir/base/methodDefinition.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/ts/tsInterfaceBody.h"

#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

// Optimized version of ASTNode::TransformChildren for module-level declarations (classes, interfaces etc)
template <typename F>
// CC-OFFNXT(G.FUD.06) perf critical, ODR
inline void TransformRecords(ir::BlockStatement *ast, const F &cb, std::string_view transformationName = "")
{
    ES2PANDA_ASSERT(ast->IsProgram());

    std::function<void(ir::ClassDeclaration *)> traverse = [&cb, &transformationName,
                                                            &traverse](ir::ClassDeclaration *node) {
        auto def = node->AsClassDeclaration()->Definition();
        auto const &body = def->Body();
        for (size_t ix = 0; ix < body.size(); ix++) {
            auto member = body[ix];
            if (UNLIKELY(member->IsClassDeclaration() &&
                         member->AsClassDeclaration()->Definition()->IsNamespaceTransformed())) {
                traverse(member->AsClassDeclaration());
            }
            if (auto *transformedNode = cb(body[ix]); UNLIKELY(body[ix] != transformedNode)) {
                body[ix]->SetTransformedNode(transformationName, transformedNode);
                def->SetValueBody(transformedNode, ix);
            }
        }
    };

    auto const &constStatements = ast->Statements();
    for (size_t idx = 0; idx < constStatements.size(); ++idx) {
        auto stmt = constStatements[idx];
        if (UNLIKELY(stmt->IsClassDeclaration() &&
                     stmt->AsClassDeclaration()->Definition()->IsNamespaceTransformed())) {
            traverse(stmt->AsClassDeclaration());
        }
        if (auto *transformed = cb(stmt); UNLIKELY(stmt != transformed)) {
            stmt->SetTransformedNode(transformationName, transformed);
            ast->StatementsForUpdates()[idx] = transformed->AsStatement();
        }
    }
}

// Optimized version of ASTNode::TransformChildren for interface and class bodies
template <typename F>
// CC-OFFNXT(G.FUD.06) perf critical, ODR
inline void TransformRecordBodies(ir::BlockStatement *ast, const F &cb, std::string_view transformationName = "")
{
    auto const transformClassBody = [&cb, transformationName](ir::ClassDeclaration *node) {
        auto def = node->AsClassDeclaration()->Definition();
        auto ctor = def->Ctor();
        if (ctor != nullptr) {
            if (auto *transformedNode = cb(ctor); UNLIKELY(ctor != transformedNode)) {
                ctor->SetTransformedNode(transformationName, transformedNode);
                def->SetCtor(transformedNode->AsMethodDefinition());
            }
        }
        auto const &body = def->Body();
        for (size_t ix = 0; ix < body.size(); ix++) {
            if (auto *transformedNode = cb(body[ix]); UNLIKELY(body[ix] != transformedNode)) {
                body[ix]->SetTransformedNode(transformationName, transformedNode);
                def->SetValueBody(transformedNode, ix);
            }
        }
    };

    auto const transformInterfaceBody = [&cb, transformationName](ir::TSInterfaceDeclaration *node) {
        auto def = node->AsTSInterfaceDeclaration()->Body();
        for (auto &it : def->Body()) {
            if (auto *transformedNode = cb(it); UNLIKELY(it != transformedNode)) {
                it->SetTransformedNode(transformationName, transformedNode);
                it = transformedNode;
            }
        }
    };

    TransformRecords(
        ast,
        [&transformClassBody, &transformInterfaceBody](ir::AstNode *node) {
            if (node->IsClassDeclaration()) {
                transformClassBody(node->AsClassDeclaration());
            } else if (node->IsTSInterfaceDeclaration()) {
                transformInterfaceBody(node->AsTSInterfaceDeclaration());
            }
            return node;
        },
        transformationName);
}

}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_LOWERING_UTIL_INL_H
