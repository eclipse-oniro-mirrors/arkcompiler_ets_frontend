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

#ifndef ES2PANDA_CHECKER_ETSANALYZER_H
#define ES2PANDA_CHECKER_ETSANALYZER_H

#include "checker/ETSchecker.h"
#include "ETSAnalyzerHelpers.h"

namespace ark::es2panda::checker {

class ETSAnalyzer final : public SemanticAnalyzer {
public:
    explicit ETSAnalyzer(Checker *checker) : SemanticAnalyzer(checker) {};
// CC-OFFNXT(G.PRE.02,G.PRE.09) name part
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_ETSANALYZER_CHECK_METHOD(_, nodeType) checker::Type *Check(ir::nodeType *node) const override;
    AST_NODE_MAPPING(DECLARE_ETSANALYZER_CHECK_METHOD)
#undef DECLARE_ETSANALYZER_CHECK_METHOD

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECLARE_ETSANALYZER_CHECK_METHOD(_, __, nodeType, ___) \
    virtual checker::Type *Check(ir::nodeType *node) const override;  // CC-OFF(G.PRE.02,G.PRE.09) name part
    AST_NODE_REINTERPRET_MAPPING(DECLARE_ETSANALYZER_CHECK_METHOD)
#undef DECLARE_ETSANALYZER_CHECK_METHOD
    checker::Type *CheckDynamic(ir::ObjectExpression *expr) const;
    checker::Type *GetPreferredType(ir::ArrayExpression *expr) const;
    void GetUnionPreferredType(ir::Expression *expr, Type *originalType) const;
    void CollectNonOptionalProperty(const ETSObjectType *objType,
                                    std::unordered_map<util::StringView, ETSObjectType *> &props) const;
    void CheckObjectExprProps(const ir::ObjectExpression *expr, checker::ETSObjectType *objectTypeForProperties,
                              checker::PropertySearchFlags searchFlags) const;
    std::tuple<Type *, ir::Expression *> CheckAssignmentExprOperatorType(ir::AssignmentExpression *expr,
                                                                         Type *leftType) const;
    [[nodiscard]] checker::Type *ReturnTypeForStatement([[maybe_unused]] const ir::Statement *const st) const;
    checker::Type *CheckObjectExprBaseOnObjectType(ir::ObjectExpression *expr, checker::ETSObjectType *objType) const;
    checker::Type *CheckObjectExprBaseOnUnionType(ir::ObjectExpression *expr,
                                                  checker::ETSUnionType *preferredType) const;

private:
    ETSChecker *GetETSChecker() const;
    void CheckInstantatedClass(ir::ETSNewClassInstanceExpression *expr, ETSObjectType *&calleeObj) const;
    void CheckMethodModifiers(ir::MethodDefinition *node) const;
    checker::Type *GetFunctionReturnType(ir::ReturnStatement *st, ir::ScriptFunction *containingFunc) const;
    checker::Type *UnwrapPromiseType(checker::Type *type) const;
    checker::Type *GetSmartTypeForAssignment(ir::AssignmentExpression *const expr, checker::Type *const leftType,
                                             checker::Type *const rightType, ir::Expression *const relationNode) const;
    checker::Type *GetSmartType(ir::AssignmentExpression *expr, checker::Type *leftType,
                                checker::Type *rightType) const;
    bool SetAssignmentExpressionTarget(ir::AssignmentExpression *const expr, ETSChecker *checker) const;
    bool CheckInferredFunctionReturnType(ir::ReturnStatement *st, ir::ScriptFunction *containingFunc,
                                         checker::Type *&funcReturnType, ir::TypeNode *returnTypeAnnotation,
                                         ETSChecker *checker) const;
    void CheckClassProperty(ETSChecker *checker, ir::ScriptFunction *scriptFunc) const;

    checker::Type *ResolveMemberExpressionByBaseType(ETSChecker *checker, checker::Type *baseType,
                                                     ir::MemberExpression *expr) const;

    void CheckVoidTypeExpression(ETSChecker *checker, ir::Expression *expr) const
    {
        // Existing void expression inconsistency,refer to #17762
        if (expr->TsType() == nullptr || !expr->TsType()->IsETSVoidType() || expr->Parent() == nullptr) {
            return;
        }
        auto parent = expr->Parent();
        while (parent->IsConditionalExpression()) {
            parent = parent->Parent();
            if (parent == nullptr) {
                return;
            }
        }
        bool acceptVoid =
            parent->IsExpressionStatement() || parent->IsReturnStatement() ||
            (parent->IsSequenceExpression() && parent->Parent() != nullptr && parent->Parent()->IsForUpdateStatement());
        if (!acceptVoid) {
            expr->SetTsType(checker->GlobalTypeError());
            checker->LogError(diagnostic::VOID_VALUE, {}, expr->Start());
        }
    }
    mutable std::vector<const varbinder::Variable *> catchParamStack_ {};
};

}  // namespace ark::es2panda::checker

#endif  // ES2PANDA_CHECKER_ETSANALYZER_H
