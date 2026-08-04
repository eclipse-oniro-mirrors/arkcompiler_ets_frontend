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

#ifndef ES2PANDA_COMPILER_CHECKER_ETS_ALIVE_ANALYZER_H
#define ES2PANDA_COMPILER_CHECKER_ETS_ALIVE_ANALYZER_H

#include "checker/ETSchecker.h"
#include "checker/ets/baseAnalyzer.h"

#include <optional>

namespace ark::es2panda::ir {
class AstNode;
class BinaryExpression;
class Expression;
class IfStatement;
class MemberExpression;
class Statement;
class ClassDefinition;
class MethodDefinition;
class TSInterfaceDeclaration;
class DoWhileStatement;
class UpdateExpression;
class VariableDeclaration;
class VariableDeclarator;
class ScriptFunction;
class TSNonNullExpression;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::varbinder {
class Variable;
}

namespace ark::es2panda::checker {
class AliveAnalyzer : public BaseAnalyzer<PendingExit> {
public:
    // NOLINTNEXTLINE(readability-redundant-member-init)
    AliveAnalyzer(const ir::AstNode *node, ETSChecker *checker) : BaseAnalyzer<PendingExit>(), checker_(checker)
    {
        AnalyzeNodes(node);
    }

    void MarkDead() override
    {
        status_ = LivenessStatus::DEAD;
    }

    LivenessStatus Or(LivenessStatus left, LivenessStatus right)
    {
        return static_cast<LivenessStatus>(left | right);
    }

    LivenessStatus And(LivenessStatus left, LivenessStatus right)
    {
        return static_cast<LivenessStatus>(left & right);
    }

private:
    void AnalyzeNodes(const ir::AstNode *node);
    void AnalyzeNode(const ir::AstNode *node);
    void AnalyzeNodeHelper1(const ir::AstNode *node);
    void AnalyzeNodeHelper2(const ir::AstNode *node);
    void AnalyzeDef(const ir::AstNode *node);
    void AnalyzeStat(const ir::AstNode *node);
    void AnalyzeStats(const ArenaVector<ir::Statement *> &stats);
    void AnalyzeStructDecl(const ir::ETSStructDeclaration *structDecl);
    void AnalyzeClassDecl(const ir::ClassDeclaration *classDecl);
    void AnalyzeInterfaceDecl(const ir::TSInterfaceDeclaration *ifaceDecl);
    void AnalyzeMethodDef(const ir::MethodDefinition *methodDef);
    void AnalyzeArrFuncExp(const ir::ArrowFunctionExpression *arrFuncExp);
    void AnalyzeFuncDef(const ir::ScriptFunction *func, Type *returnType, const lexer::SourcePosition &errorPos,
                        bool isArrow = false);
    void AnalyzeVarDef(const ir::VariableDeclaration *varDef);
    void AnalyzeAssignExp(const ir::AssignmentExpression *assignExp);
    void AnalyzeUpdateExp(const ir::UpdateExpression *updateExp);
    void AnalyzeMemberExp(const ir::MemberExpression *memberExpr);
    void AnalyzeTSNonNullExp(const ir::TSNonNullExpression *nonNullExpr);
    void AnalyzeClassProp(const ir::ClassProperty *prop);
    void AnalyzeDoLoop(const ir::DoWhileStatement *doWhile);
    void AnalyzeWhileLoop(const ir::WhileStatement *whileStmt);
    void AnalyzeForLoop(const ir::ForUpdateStatement *forStmt);
    void AnalyzeForOfLoop(const ir::ForOfStatement *forOfStmt);
    void AnalyzeIf(const ir::IfStatement *ifStmt);
    void AnalyzeLabelled(const ir::LabelledStatement *labelledStmt);
    void AnalyzeNewClass(const ir::ETSNewClassInstanceExpression *newClass);
    void AnalyzeCall(const ir::CallExpression *callExpr);
    void AnalyzeThrow(const ir::ThrowStatement *throwStmt);
    void AnalyzeSwitch(const ir::SwitchStatement *switchStmt);
    void AnalyzeTry(const ir::TryStatement *tryStmt);
    void AnalyzeBreak(const ir::BreakStatement *breakStmt);
    void AnalyzeContinue(const ir::ContinueStatement *contStmt);
    void AnalyzeReturn(const ir::ReturnStatement *retStmt);

    using NumericConstants = ArenaUnorderedMap<const varbinder::Variable *, double>;
    using BooleanConstants = ArenaUnorderedMap<const varbinder::Variable *, bool>;
    using TrueConditions = ArenaVector<const varbinder::Variable *>;

    class FunctionAnalysisScope {
    public:
        explicit FunctionAnalysisScope(AliveAnalyzer *analyzer);
        ~FunctionAnalysisScope();

        NO_COPY_SEMANTIC(FunctionAnalysisScope);
        NO_MOVE_SEMANTIC(FunctionAnalysisScope);

    private:
        AliveAnalyzer *analyzer_;
        bool hasOuterNumericConstants_ {false};
        bool hasOuterBooleanConstants_ {false};
        bool hasOuterTrueConditions_ {false};
        std::optional<NumericConstants> numericConstants_;
        std::optional<BooleanConstants> booleanConstants_;
        std::optional<TrueConditions> trueConditions_;
    };

    std::optional<bool> TryResolveTestValue(const ir::Expression *test) const;
    std::optional<double> TryResolveNumberValue(const ir::Expression *expr) const;
    const varbinder::Variable *GetBoundVariable(const ir::Expression *expr) const;
    std::optional<bool> TryResolveNumberComparison(const ir::BinaryExpression *binary) const;
    std::optional<bool> TryResolveInstanceOfComparison(const ir::BinaryExpression *binary) const;
    bool ContainsDefinitelyNullishNonNullExpression(const ir::AstNode *node) const;
    void AnalyzeIfConsequent(const ir::IfStatement *ifStmt);
    void AnalyzeDeadStatement(const ir::Statement *stmt);
    void TrackVariableDeclaration(const ir::VariableDeclarator *declarator);
    void ForgetAssignmentTarget(const ir::Expression *expr);
    void ForgetBoundVariable(const varbinder::Variable *boundVar);
    void ForgetAllConstants();

    ETSChecker *checker_;
    LivenessStatus status_ {LivenessStatus::ALIVE};
    NumericConstants numericConstants_;
    BooleanConstants booleanConstants_;
    TrueConditions trueConditions_;
};
}  // namespace ark::es2panda::checker

#endif
