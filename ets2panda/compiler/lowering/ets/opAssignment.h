/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_LOWERING_OP_ASSIGNMENT_H
#define ES2PANDA_COMPILER_LOWERING_OP_ASSIGNMENT_H

#include "compiler/lowering/phase.h"

namespace ark::es2panda::compiler {

class OpAssignmentLowering : public PhaseForProgramsToBeEmitted {
public:
    static constexpr std::string_view const NAME = "OpAssignmentLowering";
    std::string_view Name() const override
    {
        return NAME;
    }

    bool PerformForProgram(parser::Program *program) override;
    bool PostconditionForProgram(const parser::Program *program) override;
};

}  // namespace ark::es2panda::compiler

#endif
