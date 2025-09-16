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

#include "evaluate/helpers.h"
#include "evaluate/irCheckHelper.h"
#include "compiler/lowering/util.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "checker/ETSchecker.h"

namespace ark::es2panda::evaluate {

IrCheckHelper::IrCheckHelper(checker::ETSChecker *checker, varbinder::ETSBinder *varBinder)
    : checker_(checker), varBinder_(varBinder), recursiveDecls_(checker->Allocator()->Adapter())
{
    ES2PANDA_ASSERT(checker_);
    ES2PANDA_ASSERT(varBinder_);
}

bool IrCheckHelper::CheckNewNode(ir::AstNode *node, varbinder::Scope *scope, ir::AstNode *parentClass,
                                 parser::Program *program)
{
    ES2PANDA_ASSERT(node);

    if (program == nullptr) {
        program = varBinder_->Program();
    }
    if (scope == nullptr) {
        scope = checker_->Scope();
    }

    recursiveDecls_.emplace_back(program, scope, parentClass, node);

    if (isRecursive_) {
        return false;
    }
    isRecursive_ = true;

    if (isPrecheckPassed_) {
        HandleCustomNodes();
        CheckDecls();
    }

    isRecursive_ = false;

    return true;
}

void IrCheckHelper::PreCheck()
{
    HandleCustomNodes();
    CheckDecls();

    isPrecheckPassed_ = true;
}

// NOLINTBEGIN(readability-identifier-naming)
void IrCheckHelper::CheckDecls()
{
    // All dependent user-classes must be created at this point, so we can run checker.
    while (!recursiveDecls_.empty()) {
        auto [program, scope, parent, node] = recursiveDecls_.front();
        recursiveDecls_.pop_front();
        helpers::DoScopedAction(checker_, varBinder_, program, scope, parent, [this, node_ = node, scope_ = scope]() {
            varBinder_->ResolveReferencesForScope(node_, scope_);
            node_->Check(checker_);
        });
    }
}

void IrCheckHelper::HandleCustomNodes()
{
    auto iter = recursiveDecls_.begin();
    while (iter != recursiveDecls_.end()) {
        // Can trigger `ETSBinder::BuildClassDefinition`,
        // which can eventually call debug-info plugin to create another class.
        // Hence we delay `ETSChecker::Check` until all required classes are built and initialized in varbinder.
        auto [program, scope, parent, node] = *iter;
        helpers::DoScopedAction(checker_, varBinder_, program, scope, parent,
                                [varBinder = varBinder_, node_ = node]() { varBinder->HandleCustomNodes(node_); });
        ++iter;
    }
}
// NOLINTEND(readability-identifier-naming)

void IrCheckHelper::CheckGlobalEntity(parser::Program *program, ir::AstNode *node, bool mustCheck)
{
    ES2PANDA_ASSERT(program);

    auto *globalClass = program->GlobalClass();
    auto *globalClassScope = program->GlobalClassScope();

    helpers::DoScopedAction(checker_, varBinder_, program, globalClassScope, globalClass,
                            [this, globalClassScope, node]() {
                                compiler::InitScopesPhaseETS::RunExternalNode(node, varBinder_);
                                varBinder_->ResolveReferencesForScope(node, globalClassScope);
                            });
    if (mustCheck) {
        CheckNewNode(node, globalClassScope, globalClass, program);
    }
}

void IrCheckHelper::CheckLocalEntity(ir::AstNode *node)
{
    compiler::InitScopesPhaseETS::RunExternalNode(node, varBinder_);
    CheckNewNode(node, nullptr, nullptr, nullptr);
}

}  // namespace ark::es2panda::evaluate
