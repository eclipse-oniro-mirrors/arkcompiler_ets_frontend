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

#ifndef ES2PANDA_COMPILER_LOWERING_UNBOX_LOWERING_H
#define ES2PANDA_COMPILER_LOWERING_UNBOX_LOWERING_H

#include "compiler/lowering/phase.h"

namespace ark::es2panda::compiler {

struct UnboxContext {
    void Setup(public_lib::Context *ctx)
    {
        parser = ctx->parser->AsETSParser();
        varbinder = ctx->GetChecker()->VarBinder()->AsETSBinder();
        checker = ctx->GetChecker()->AsETSChecker();
        allocator = ctx->Allocator();
    }

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    parser::ETSParser *parser;
    varbinder::ETSBinder *varbinder;
    checker::ETSChecker *checker;
    ArenaAllocator *allocator;
    std::set<ir::AstNode *> handled;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

class UnboxPhase : public PhaseForProgramsToBeEmitted {
public:
    std::string_view Name() const override
    {
        return "Unbox";
    }

    void Setup() override;

    bool PerformForProgram(parser::Program *program) override;

private:
    UnboxContext uctx_;
};

}  // namespace ark::es2panda::compiler

#endif
