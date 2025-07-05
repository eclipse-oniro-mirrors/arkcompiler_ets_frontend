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

#ifndef ES2PANDA_COMPILER_LOWERING_DECL_GEN_PHASE_H
#define ES2PANDA_COMPILER_LOWERING_DECL_GEN_PHASE_H

#include "compiler/lowering/phase.h"

namespace ark::es2panda::compiler {

// 1. Generate declaration for checked AST
// 2. Store declaration to annotation to ETSGLOBAL class
class DeclGenPhase : public PhaseForBodies {
public:
    std::string_view Name() const override
    {
        return "DeclGenPhase";
    }

    bool PerformForModule(public_lib::Context *ctx, parser::Program *program) override;

    static constexpr std::string_view const MODULE_DECLARATION_NAME {"ModuleDeclaration"};
    static constexpr std::string_view const MODULE_DECLARATION_ANNOTATION {"std.annotations.ModuleDeclaration"};

private:
    void DumpDeclaration(parser::Program *program);
    void CreateModuleDeclarationAnnotation(parser::Program *program);

    checker::ETSChecker *checker_ {nullptr};
    compiler::PhaseManager *phaseManager_ {nullptr};
    std::string declaration_;
};

}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_LOWERING_DECL_GEN_PHASE_H
