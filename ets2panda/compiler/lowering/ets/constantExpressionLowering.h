/*
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

#ifndef ES2PANDA_COMPILER_LOWERING_CONSTANT_EXPRESSION_LOWERING_H
#define ES2PANDA_COMPILER_LOWERING_CONSTANT_EXPRESSION_LOWERING_H

#include "compiler/lowering/phase.h"

namespace ark::es2panda::compiler {

enum class TypeRank {
    // Keep this order
    INT8,
    CHAR,
    INT16,
    INT32,
    INT64,
    FLOAT,
    DOUBLE
};

class ConstantExpressionLowering : public PhaseForDeclarations {
public:
    std::string_view Name() const override
    {
        return "ConstantExpressionLowering";
    }

    bool PerformForModule(public_lib::Context *ctx, parser::Program *program) override;

private:
    ir::AstNode *MaybeUnfold(ir::AstNode *node);
    ir::AstNode *MaybeUnfoldIdentifier(ir::Identifier *node);
    ir::AstNode *MaybeUnfoldMemberExpression(ir::MemberExpression *node);
    ir::AstNode *UnfoldResolvedReference(ir::AstNode *resolved, ir::AstNode *node);

    ir::AstNode *Fold(ir::AstNode *constantNode);
    ir::AstNode *FoldTernaryConstant(ir::ConditionalExpression *cond);

    void IsInitByConstant(ir::AstNode *node);
    void TryFoldInitializerOfPackage(ir::ClassDefinition *globalClass);

    public_lib::Context *context_ {nullptr};
    parser::Program *program_ {nullptr};
    varbinder::ETSBinder *varbinder_ {nullptr};
    bool isSelfDependence_ = {false};
    std::unordered_set<const ir::AstNode *> unfoldingSet_;
};

}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_LOWERING_CONSTANT_EXPRESSION_LOWERING_H
