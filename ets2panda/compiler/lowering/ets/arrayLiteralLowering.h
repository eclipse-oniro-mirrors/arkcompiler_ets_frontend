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

#ifndef ES2PANDA_COMPILER_LOWERING_ARRAY_LITERAL_LOWERING_H
#define ES2PANDA_COMPILER_LOWERING_ARRAY_LITERAL_LOWERING_H

#include "checker/types/ets/etsResizableArrayType.h"
#include "compiler/lowering/phase.h"
#include "ir/astNode.h"
#include "ir/ets/etsNewArrayInstanceExpression.h"
#include "ir/ts/tsArrayType.h"
#include "parser/ETSparser.h"
#include "libarkbase/utils/arena_containers.h"
#include "varbinder/ETSBinder.h"

namespace ark::es2panda::compiler {

class ArrayLiteralLowering : public PhaseForBodies {
public:
    std::string_view Name() const override;
    bool PerformForModule(public_lib::Context *ctx, parser::Program *program) override;

private:
    ArenaVector<ir::Statement *> GenerateDefaultCallToConstructor(ir::Identifier *arraySymbol, checker::Type *eleType);
    ir::Statement *CreateNestedArrayCreationStatement(ArenaVector<ir::Identifier *> &identDims, size_t currentDim,
                                                      checker::Type *type, ir::Expression *expr);
    ir::AstNode *TryTransformLiteralArrayToRefArray(ir::ArrayExpression *literalArray);
    ir::AstNode *TryTransformNewArrayExprToRefArray(ir::ETSNewArrayInstanceExpression *newExpr);
    ArenaVector<ir::Identifier *> TransformDimVectorToIdentVector(ArenaVector<ir::Expression *> &dimVector,
                                                                  ArenaVector<ir::Statement *> &stmts);
    ir::AstNode *TryTransformNewMultiDimArrayToRefArray(ir::ETSNewMultiDimArrayInstanceExpression *newExpr);

    ArenaAllocator *Allocator();

    parser::ETSParser *parser_ {nullptr};
    varbinder::ETSBinder *varbinder_ {nullptr};
    checker::ETSChecker *checker_ {nullptr};
};

}  // namespace ark::es2panda::compiler

#endif