/*
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

#ifndef ES2PANDA_GRADUAL_TYPE_NARROWING_H
#define ES2PANDA_GRADUAL_TYPE_NARROWING_H

#include "compiler/lowering/phase.h"
#include "ir/typeNode.h"

namespace ark::es2panda::compiler {

class GradualTypeNarrowing : public PhaseForBodies {
public:
    std::string_view Name() const override
    {
        return "GradualTypeNarrowing";
    }

    bool PerformForModule(public_lib::Context *ctx, parser::Program *program) override;

private:
    checker::Type *TransformType(checker::Type *type, const std::function<checker::Type *(checker::Type *)> &func);
    void NarrowGradualType(ir::AstNode *node);
    ir::AstNode *ProcessGradualTypeNode(ir::ETSTypeReference *node);

    public_lib::Context *context_ {nullptr};
    checker::ETSChecker *checker_ {nullptr};
};
}  // namespace ark::es2panda::compiler

#endif