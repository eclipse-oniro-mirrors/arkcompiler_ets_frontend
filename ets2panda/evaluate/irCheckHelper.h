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

#ifndef ES2PANDA_EVALUATE_NON_RECURSIVE_IR_CHECKER_H
#define ES2PANDA_EVALUATE_NON_RECURSIVE_IR_CHECKER_H

#include "mem/arena_allocator.h"
#include "utils/arena_containers.h"

namespace ark::es2panda::checker {
class ETSChecker;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::ir {
class AstNode;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::parser {
class Program;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::varbinder {
class ETSBinder;
class Scope;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::evaluate {

/**
 * @brief Helper class for running type checks
 * All newly created IR nodes from debug-info must be checked through this class,
 * otherwise recursive class creations can lead to varbinder scopes errors.
 */
class IrCheckHelper final {
public:
    struct ScopedAstNode final {
        ScopedAstNode(parser::Program *p, varbinder::Scope *s, ir::AstNode *parent, ir::AstNode *n)
            : program(p), scope(s), parentClass(parent), node(n)
        {
        }

        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        parser::Program *program {nullptr};
        varbinder::Scope *scope {nullptr};
        ir::AstNode *parentClass {nullptr};
        ir::AstNode *node {nullptr};
        // NOLINTEND(misc-non-private-member-variables-in-classes)
    };

public:
    explicit IrCheckHelper(checker::ETSChecker *checker, varbinder::ETSBinder *varBinder);
    NO_COPY_SEMANTIC(IrCheckHelper);
    NO_MOVE_SEMANTIC(IrCheckHelper);
    ~IrCheckHelper() noexcept = default;

    /**
     * @brief Runs scope and type checks on the given IR node
     * @param node to check, must not be null.
     * @param scope to which node is relative to - if null, current checker scope is taken.
     * @param parentClass class to use record table from, might be null.
     * @param program relative to node - if null, current varbinder program is taken;
     * either it or `scope` must be non-null.
     * @returns false if check was delayed until recursion end, true if check was done immediately.
     */
    bool CheckNewNode(ir::AstNode *node, varbinder::Scope *scope, ir::AstNode *parentClass, parser::Program *program);

    void PreCheck();

    void CheckGlobalEntity(parser::Program *program, ir::AstNode *node, bool mustCheck = true);
    void CheckLocalEntity(ir::AstNode *node);

    checker::ETSChecker *GetChecker() const
    {
        return checker_;
    }

private:
    void HandleCustomNodes();
    void CheckDecls();

private:
    checker::ETSChecker *checker_ {nullptr};
    varbinder::ETSBinder *varBinder_ {nullptr};

    bool isRecursive_ {false};
    // List is required due to possible push-backs during iteration.
    ArenaList<ScopedAstNode> recursiveDecls_;
    // Indicates that PreCheck was called from ETSChecker::StartChecker()
    bool isPrecheckPassed_ {false};
};

}  // namespace ark::es2panda::evaluate

#endif  // ES2PANDA_EVALUATE_NON_RECURSIVE_IR_CHECKER_H
