/**
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

#ifndef ES2PANDA_TEST_UTILS_SCOPE_INIT_TEST_H
#define ES2PANDA_TEST_UTILS_SCOPE_INIT_TEST_H

#include "ir/expression.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/variableDeclarator.h"
#include "ir/statements/variableDeclaration.h"
#include "util/eheap.h"
#include "compiler/lowering/phase.h"

#include <gtest/gtest.h>

namespace ir_alias = ark::es2panda::ir;

namespace test::utils {

class ScopeInitTest : public testing::Test {
public:
    ScopeInitTest() : allocator_(ark::es2panda::EHeap::NewAllocator())
    {
        phaseManager_ = std::make_unique<ark::es2panda::compiler::PhaseManager>(ark::es2panda::ScriptExtension::ETS,
                                                                                allocator_.get());
        ark::es2panda::compiler::SetPhaseManager(phaseManager_.get());
    }

    /*
     * Shortcut to convert single elemnt block expression body to it's name
     * Example: { let x; } => x
     */
    ir_alias::Identifier *BodyToFirstName(ir_alias::Statement *body);

    static void SetUpTestCase()
    {
        ark::es2panda::EHeap::Initialize();
    }

    ark::es2panda::ArenaAllocator *Allocator()
    {
        return allocator_.get();
    }

private:
    std::unique_ptr<ark::es2panda::ArenaAllocator> allocator_;
    std::unique_ptr<ark::es2panda::compiler::PhaseManager> phaseManager_;
};

}  // namespace test::utils

#endif  // ES2PANDA_TEST_UTILS_SCOPE_INIT_TEST_H
