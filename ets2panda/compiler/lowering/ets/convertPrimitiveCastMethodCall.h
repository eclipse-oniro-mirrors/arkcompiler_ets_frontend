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

#ifndef ES2PANDA_COMPILER_LOWERING_CONVERT_MEMBEREXPRESSION_H
#define ES2PANDA_COMPILER_LOWERING_CONVERT_MEMBEREXPRESSION_H

#include "compiler/lowering/phase.h"

namespace ark::es2panda::compiler {

// NOTE(mshimenkov): To be removed after delivering 'primitive types refactoring' patch
//
// The purpose of this phase is to convert all primitive type cast method calls into 'as' expressions
// Example:
// let d: double = 3.14
// let i: int = d.toInt()
//
// After phase:
// let d: double = 3.14
// let i: int = d as int
//
class ConvertPrimitiveCastMethodCall : public PhaseForBodies {
public:
    std::string_view Name() const override;
    bool PerformForModule(public_lib::Context *ctx, parser::Program *program) override;
};

}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_LOWERING_CONVERT_MEMBEREXPRESSION_H
