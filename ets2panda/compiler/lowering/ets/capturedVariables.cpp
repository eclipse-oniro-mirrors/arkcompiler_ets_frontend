/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "capturedVariables.h"
#include "util/options.h"

namespace ark::es2panda::compiler {

static bool OnLeftSideOfAssignment(ir::Identifier const *const ident) noexcept
{
    return ident->Parent()->IsAssignmentExpression() && ident->Parent()->AsAssignmentExpression()->Left() == ident;
}

static void AddScopes(ir::AstNode *node, std::set<varbinder::Scope *> &scopes) noexcept
{
    if (node->Scope()->IsFunctionScope()) {
        scopes.emplace(node->Scope()->AsFunctionScope()->ParamScope());
    }
    if (node->Scope()->IsCatchScope()) {
        scopes.emplace(node->Scope()->AsCatchScope()->ParamScope());
    }
    if (node->Scope()->IsLoopScope()) {
        scopes.emplace(node->Scope()->AsLoopScope()->DeclScope());
    }
    scopes.emplace(node->Scope());
}

static varbinder::Variable *FindVariable(ir::Identifier *ident, std::set<varbinder::Scope *> const &scopes) noexcept
{
    if (ident->IsErrorPlaceHolder()) {
        return nullptr;
    }
    auto *var = ident->Variable();
    //  NOTE! For some unknown reasons :) variables exist in scope collections but are not set to identifiers after
    //        'varbinder->IdentifierAnalysis()' pass. Probably need to be investigated and fixed sometimes...
    if (var == nullptr) {
        //  We start from the innermost scope!
        for (auto it = scopes.crbegin(); it != scopes.crend(); ++it) {
            auto res = (*it)->Find(ident->Name(), varbinder::ResolveBindingOptions::VARIABLES);
            if (res.variable != nullptr) {
                var = res.variable;
                break;
            }
        }
    }

    if (var != nullptr) {
        auto *scope = var->GetScope();
        ES2PANDA_ASSERT(scope != nullptr);
        //  We are not interested in variables defined inside arrow function!
        if (scopes.find(scope) != scopes.cend()) {
            return nullptr;
        }
    }

    return var;
}

static void FindModifiedCaptured(ir::ScriptFunction const *const scriptFunction,
                                 std::set<varbinder::Variable *> &variables) noexcept
{
    auto scopes = std::set<varbinder::Scope *> {};
    bool inLambda = false;

    std::function<void(ir::AstNode *)> walker = [&](ir::AstNode *node) -> void {
        if (node->IsArrowFunctionExpression() || node->IsClassDeclaration()) {
            auto savedWL = inLambda;
            auto savedScopes = std::set<varbinder::Scope *> {};
            std::swap(scopes, savedScopes);

            inLambda = true;
            node->Iterate(walker);

            inLambda = savedWL;
            std::swap(scopes, savedScopes);
            savedScopes.clear();

            return;
        }

        if (inLambda && node->IsScopeBearer()) {
            AddScopes(node, scopes);
        } else if (inLambda && node->IsIdentifier() && OnLeftSideOfAssignment(node->AsIdentifier())) {
            if (auto *var = FindVariable(node->AsIdentifier(), scopes); var != nullptr) {
                variables.insert(var);
            }
        }

        node->Iterate(walker);
    };

    scriptFunction->Iterate(walker);
}

static void HandleScriptFunction(ir::ScriptFunction const *const scriptFunction) noexcept
{
    auto variables = std::set<varbinder::Variable *> {};
    FindModifiedCaptured(scriptFunction, variables);

    for (auto *variable : variables) {
        variable->AddFlag(varbinder::VariableFlags::CAPTURED_MODIFIED);
    }

    variables.clear();
}

bool CapturedVariables::PerformForModule([[maybe_unused]] public_lib::Context *ctx, parser::Program *program)
{
    std::function<void(ir::AstNode *)> searchForFunctions = [&](ir::AstNode *ast) {
        if (ast->IsScriptFunction()) {
            HandleScriptFunction(ast->AsScriptFunction());  // no recursion
        } else {
            ast->Iterate(searchForFunctions);
        }
    };

    program->Ast()->Iterate(searchForFunctions);
    return true;
}
}  // namespace ark::es2panda::compiler
