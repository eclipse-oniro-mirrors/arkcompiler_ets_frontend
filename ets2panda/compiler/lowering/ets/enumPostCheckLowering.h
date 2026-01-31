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

#ifndef ES2PANDA_COMPILER_ENUM_POST_CHECK_LOWERING_H
#define ES2PANDA_COMPILER_ENUM_POST_CHECK_LOWERING_H

#include "compiler/lowering/phase.h"
#include "ir/ts/tsAsExpression.h"

namespace ark::es2panda::compiler {

enum class EnumCastType {
    NONE,
    CAST_TO_STRING,
    CAST_TO_INT,
    CAST_TO_LONG,
    CAST_TO_DOUBLE,
    CAST_TO_FLOAT,
    CAST_TO_BYTE,
    CAST_TO_SHORT,
    CAST_TO_NUMERIC_ENUM,
    CAST_TO_STRING_ENUM,
};

class EnumPostCheckLoweringPhase : public PhaseForProgramsWithBodies_LEGACY {
public:
    EnumPostCheckLoweringPhase() noexcept = default;
    std::string_view Name() const override
    {
        return "EnumPostCheckLoweringPhase";
    }
    bool PerformForProgram(parser::Program *program) override;

    void Setup() override
    {
        parser_ = Context()->parser->AsETSParser();
        varbinder_ = Context()->parserProgram->VarBinder()->AsETSBinder();
        checker_ = Context()->GetChecker()->AsETSChecker();
    }

private:
    ir::Statement *CreateStatement(const std::string &src, ir::Expression *ident, ir::Expression *init);
    ir::AstNode *BuildEnumCasting(ir::AstNode *const node);
    void CreateStatementForUnionConstituentType(EnumCastType castType, ir::Identifier *ident, checker::Type *type,
                                                ir::TSAsExpression *tsAsExpr, ArenaVector<ir::Statement *> &statements);
    ir::SwitchStatement *GenerateGetOrdinalCallForSwitch(ir::SwitchStatement *const node);
    ir::AstNode *GenerateEnumCasting(ir::TSAsExpression *node, EnumCastType castType);
    ir::AstNode *GenerateValueOfCall(ir::AstNode *const node);
    ir::Expression *GenerateFromValueCall(ir::Expression *const node, util::StringView name);
    ir::Expression *HandleEnumTypeCasting(checker::Type *type, ir::Expression *expr, ir::TSAsExpression *tsAsExpr);
    ir::Expression *HandleUnionTypeForCalls(checker::ETSUnionType *unionType, ir::Expression *expr,
                                            ir::TSAsExpression *tsAsExpr, EnumCastType castType);

    parser::ETSParser *parser_ {nullptr};
    checker::ETSChecker *checker_ {nullptr};
    varbinder::ETSBinder *varbinder_ {nullptr};
};

}  // namespace ark::es2panda::compiler

#endif  // ES2PANDA_COMPILER_ENUM_POST_CHECK_LOWERING_H
