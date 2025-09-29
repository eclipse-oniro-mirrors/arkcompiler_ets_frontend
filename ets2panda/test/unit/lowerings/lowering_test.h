/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_TEST_LOWERINGS_LOWERING_TEST_H
#define ES2PANDA_TEST_LOWERINGS_LOWERING_TEST_H

#include <gtest/gtest.h>
#include "test/utils/panda_executable_path_getter.h"
#include "parser/program/program.h"
#include "test/utils/ast_verifier_test.h"

namespace ark::es2panda {

using LoweringTest = test::utils::AstVerifierTest;

}  // namespace ark::es2panda

#endif  // ES2PANDA_TEST_LOWERINGS_LOWERING_TEST_H
