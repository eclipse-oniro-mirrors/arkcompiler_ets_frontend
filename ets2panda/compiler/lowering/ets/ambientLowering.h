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

#ifndef ES2PANDA_COMPILER_LOWERING_AMBIENT_LOWING_H
#define ES2PANDA_COMPILER_LOWERING_AMBIENT_LOWING_H
#include "compiler/lowering/phase.h"

namespace ark::es2panda::compiler {
class AmbientLowering : public PhaseForSourcePrograms {
public:
    std::string_view Name() const override;
    bool PerformForProgram(parser::Program *program) override;
    bool PostconditionForProgram(const parser::Program *program) override;

private:
    ir::AstNode *CreateIndexerMethodIfNeeded(ir::AstNode *ast);
    void RemoveRedundantIndexerDeclarations(ir::AstNode *ast);
};
}  // namespace ark::es2panda::compiler

#endif
