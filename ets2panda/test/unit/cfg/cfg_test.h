/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_TEST_CFG_CFG_TEST_H
#define ES2PANDA_TEST_CFG_CFG_TEST_H

#include <gtest/gtest.h>
#include "compiler/core/CFG.h"
#include "test/utils/ast_verifier_test.h"

namespace ark::es2panda::compiler {

class CFGTest : public test::utils::AstVerifierTest {
public:
    [[nodiscard]] static const CFG::BasicBlock *GetBBByID(const ArenaSet<CFG::BasicBlock *> &basicBlocks, size_t id)
    {
        for (auto bb : basicBlocks) {
            if (bb->GetIndex() == id) {
                return bb;
            }
        }
        return nullptr;
    }
};

constexpr size_t ID_01 = 1;
constexpr size_t ID_02 = 2;
constexpr size_t ID_03 = 3;
constexpr size_t ID_04 = 4;
constexpr size_t ID_05 = 5;
constexpr size_t ID_06 = 6;
constexpr size_t ID_07 = 7;
constexpr size_t ID_13 = 13;

constexpr size_t EXPECTED_ZERO = 0;
constexpr size_t EXPECTED_ONE = 1;
constexpr size_t EXPECTED_TWO = 2;
constexpr size_t EXPECTED_THREE = 3;
constexpr size_t EXPECTED_FOUR = 4;
constexpr size_t EXPECTED_FIVE = 5;
constexpr size_t EXPECTED_FIFTEEN = 15;

}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_TEST_CFG_CFG_TEST_H
