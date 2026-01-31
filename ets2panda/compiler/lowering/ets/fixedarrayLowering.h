/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_LOWERING_FIXEDARRAY_LOWERING_H
#define ES2PANDA_COMPILER_LOWERING_FIXEDARRAY_LOWERING_H

#include "compiler/lowering/phase.h"
#include "parser/ETSparser.h"
#include "varbinder/ETSBinder.h"
#include "ir/astNode.h"
#include "ir/ets/etsNewClassInstanceExpression.h"

namespace ark::es2panda::compiler {

class FixedArrayLowering : public PhaseForProgramsWithBodies_LEGACY {  // #22952: could be a PhaseForDeclarations
public:
    std::string_view Name() const override
    {
        return "FixedArrayLowering";
    }

    bool PerformForProgram(parser::Program *program) override;
};

}  // namespace ark::es2panda::compiler

#endif