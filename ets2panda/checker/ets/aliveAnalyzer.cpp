/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "aliveAnalyzer.h"
#include <cstddef>

#include "checker/types/ets/etsAsyncFuncReturnType.h"
#include "ir/base/classDefinition.h"
#include "ir/base/classProperty.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/statements/classDeclaration.h"
#include "ir/statements/variableDeclaration.h"
#include "ir/statements/doWhileStatement.h"
#include "ir/statements/expressionStatement.h"
#include "ir/statements/whileStatement.h"
#include "ir/statements/forUpdateStatement.h"
#include "ir/statements/labelledStatement.h"
#include "ir/statements/forOfStatement.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/ifStatement.h"
#include "ir/statements/switchStatement.h"
#include "ir/statements/variableDeclarator.h"
#include "ir/statements/throwStatement.h"
#include "ir/statements/switchCaseStatement.h"
#include "ir/statements/breakStatement.h"
#include "ir/statements/continueStatement.h"
#include "ir/statements/returnStatement.h"
#include "ir/statements/tryStatement.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/ets/etsNewClassInstanceExpression.h"
#include "ir/ets/etsStructDeclaration.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "checker/ETSAnalyzerHelpers.h"
#include "checker/types/globalTypesHolder.h"
#include "varbinder/variable.h"
#include "varbinder/declaration.h"
#include "checker/ETSchecker.h"
#include "ir/base/catchClause.h"

namespace ark::es2panda::checker {

void AliveAnalyzer::AnalyzeNodes(const ir::AstNode *node)
{
    node->Iterate([this](auto *childNode) { AnalyzeNode(childNode); });
}

void AliveAnalyzer::AnalyzeNode(const ir::AstNode *node)
{
    if (node == nullptr) {
        return;
    }

    switch (node->Type()) {
        case ir::AstNodeType::EXPRESSION_STATEMENT: {
            AnalyzeNode(node->AsExpressionStatement()->GetExpression());
            break;
        }
        case ir::AstNodeType::STRUCT_DECLARATION: {
            AnalyzeStructDecl(node->AsETSStructDeclaration());
            break;
        }
        case ir::AstNodeType::CLASS_DECLARATION: {
            AnalyzeClassDecl(node->AsClassDeclaration());
            break;
        }
        case ir::AstNodeType::METHOD_DEFINITION: {
            AnalyzeMethodDef(node->AsMethodDefinition());
            break;
        }
        case ir::AstNodeType::VARIABLE_DECLARATION: {
            AnalyzeVarDef(node->AsVariableDeclaration());
            break;
        }
        case ir::AstNodeType::BLOCK_STATEMENT: {
            AnalyzeStats(node->AsBlockStatement()->Statements());
            break;
        }
        case ir::AstNodeType::DO_WHILE_STATEMENT: {
            AnalyzeDoLoop(node->AsDoWhileStatement());
            break;
        }
        default: {
            break;
        }
    }

    // Helpers to reduce function size and pass code checker
    AnalyzeNodeHelper1(node);
    AnalyzeNodeHelper2(node);
}

// Helper function to reduce AnalyzeNode size and pass code checker
void AliveAnalyzer::AnalyzeNodeHelper1(const ir::AstNode *node)
{
    switch (node->Type()) {
        case ir::AstNodeType::WHILE_STATEMENT: {
            AnalyzeWhileLoop(node->AsWhileStatement());
            break;
        }
        case ir::AstNodeType::FOR_UPDATE_STATEMENT: {
            AnalyzeForLoop(node->AsForUpdateStatement());
            break;
        }
        case ir::AstNodeType::FOR_OF_STATEMENT: {
            AnalyzeForOfLoop(node->AsForOfStatement());
            break;
        }
        case ir::AstNodeType::IF_STATEMENT: {
            AnalyzeIf(node->AsIfStatement());
            break;
        }
        case ir::AstNodeType::LABELLED_STATEMENT: {
            AnalyzeLabelled(node->AsLabelledStatement());
            break;
        }
        case ir::AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION: {
            AnalyzeNewClass(node->AsETSNewClassInstanceExpression());
            break;
        }
        case ir::AstNodeType::CALL_EXPRESSION: {
            AnalyzeCall(node->AsCallExpression());
            break;
        }
        case ir::AstNodeType::THROW_STATEMENT: {
            AnalyzeThrow(node->AsThrowStatement());
            break;
        }
        case ir::AstNodeType::SWITCH_STATEMENT: {
            AnalyzeSwitch(node->AsSwitchStatement());
            break;
        }
        default: {
            break;
        }
    }
}

// Helper function to reduce AnalyzeNode size and pass code checker
void AliveAnalyzer::AnalyzeNodeHelper2(const ir::AstNode *node)
{
    switch (node->Type()) {
        case ir::AstNodeType::TRY_STATEMENT: {
            AnalyzeTry(node->AsTryStatement());
            break;
        }
        case ir::AstNodeType::BREAK_STATEMENT: {
            AnalyzeBreak(node->AsBreakStatement());
            break;
        }
        case ir::AstNodeType::CONTINUE_STATEMENT: {
            AnalyzeContinue(node->AsContinueStatement());
            break;
        }
        case ir::AstNodeType::RETURN_STATEMENT: {
            AnalyzeReturn(node->AsReturnStatement());
            break;
        }
        default: {
            break;
        }
    }
}

void AliveAnalyzer::AnalyzeDef(const ir::AstNode *node)
{
    AnalyzeStat(node);
    if (node != nullptr && node->IsClassStaticBlock() && status_ == LivenessStatus::DEAD) {
        checker_->LogError(diagnostic::INIT_DOESNT_COMPLETE, {}, node->Start());
    }
}

void AliveAnalyzer::AnalyzeStat(const ir::AstNode *node)
{
    if (node == nullptr) {
        return;
    }

    if (status_ == LivenessStatus::DEAD) {
        checker_->LogDiagnostic(diagnostic::UNREACHABLE_STMT, node->Start());
        return;
    }

    if (node->IsClassStaticBlock()) {
        AnalyzeNodes(node);
        return;
    }

    AnalyzeNode(node);
}

void AliveAnalyzer::AnalyzeStats(const ArenaVector<ir::Statement *> &stats)
{
    for (const auto *it : stats) {
        AnalyzeStat(it);
    }
}

void AliveAnalyzer::AnalyzeStructDecl(const ir::ETSStructDeclaration *structDecl)
{
    for (const auto *it : structDecl->Definition()->Body()) {
        AnalyzeNode(it);
    }
}

void AliveAnalyzer::AnalyzeClassDecl(const ir::ClassDeclaration *classDecl)
{
    LivenessStatus prevStatus = status_;

    for (const auto *it : classDecl->Definition()->Body()) {
        AnalyzeNode(it);
    }

    status_ = prevStatus;
}

void AliveAnalyzer::AnalyzeMethodDef(const ir::MethodDefinition *methodDef)
{
    for (ir::MethodDefinition *overload : methodDef->Overloads()) {
        AnalyzeNode(overload);
    }

    auto *func = methodDef->Function();

    ES2PANDA_ASSERT(func != nullptr);
    if (func->Body() == nullptr || func->IsProxy()) {
        return;
    }

    status_ = LivenessStatus::ALIVE;
    AnalyzeStat(func->Body());
    ES2PANDA_ASSERT(methodDef->TsType() && methodDef->TsType()->IsETSFunctionType());
    const auto *signature = methodDef->TsType()->AsETSFunctionType()->FindSignature(func);
    ES2PANDA_ASSERT(signature != nullptr);
    const auto *returnType = signature->ReturnType();
    const auto isVoid = returnType->IsETSVoidType() || returnType == checker_->GlobalVoidType();

    auto isPromiseVoid = false;

    if (returnType->IsETSObjectType() &&
        returnType->AsETSObjectType()->AssemblerName() == compiler::Signatures::BUILTIN_PROMISE) {
        const auto *asAsync = returnType->AsETSObjectType();
        isPromiseVoid = asAsync->TypeArguments().front() == checker_->GlobalETSUndefinedType() ||
                        asAsync->TypeArguments().front() == checker_->GlobalVoidType();
    }

    if (status_ == LivenessStatus::ALIVE && !isVoid && !isPromiseVoid) {
        ES2PANDA_ASSERT(methodDef->Function() != nullptr);
        if (!methodDef->Function()->HasReturnStatement()) {
            if (!util::Helpers::IsAsyncMethod(methodDef)) {
                checker_->LogError(diagnostic::MISSING_RETURN_STMT, {}, func->Start());
                ClearPendingExits();
            }
            return;
        }

        checker_->LogError(diagnostic::NONRETURNING_PATHS, {}, func->Start());
    }

    ClearPendingExits();
}

void AliveAnalyzer::AnalyzeVarDef(const ir::VariableDeclaration *varDef)
{
    for (auto *it : varDef->Declarators()) {
        if (it->Init() == nullptr) {
            continue;
        }

        AnalyzeNode(it->Init());
    }
}

void AliveAnalyzer::AnalyzeDoLoop(const ir::DoWhileStatement *doWhile)
{
    SetOldPendingExits(PendingExits());
    AnalyzeStat(doWhile->Body());
    status_ = Or(status_, ResolveContinues(doWhile));
    AnalyzeNode(doWhile->Test());
    ES2PANDA_ASSERT(doWhile->Test()->TsType());
    const auto exprRes = IsConstantTestValue(doWhile->Test());
    status_ = And(status_, static_cast<LivenessStatus>(!std::get<0>(exprRes) || !std::get<1>(exprRes)));
    status_ = Or(status_, ResolveBreaks(doWhile));
}

void AliveAnalyzer::AnalyzeWhileLoop(const ir::WhileStatement *whileStmt)
{
    SetOldPendingExits(PendingExits());
    AnalyzeNode(whileStmt->Test());
    ES2PANDA_ASSERT(whileStmt->Test()->TsType());
    const auto exprRes = IsConstantTestValue(whileStmt->Test());
    status_ = And(status_, static_cast<LivenessStatus>(!std::get<0>(exprRes) || std::get<1>(exprRes)));
    AnalyzeStat(whileStmt->Body());
    status_ = Or(status_, ResolveContinues(whileStmt));
    status_ = Or(ResolveBreaks(whileStmt), From(!std::get<0>(exprRes) || !std::get<1>(exprRes)));
}

void AliveAnalyzer::AnalyzeForLoop(const ir::ForUpdateStatement *forStmt)
{
    AnalyzeNode(forStmt->Init());
    SetOldPendingExits(PendingExits());
    const Type *condType {};
    bool resolveType = false;
    bool res = false;

    if (forStmt->Test() != nullptr) {
        AnalyzeNode(forStmt->Test());
        ES2PANDA_ASSERT(forStmt->Test()->TsType());
        condType = forStmt->Test()->TsType();
        std::tie(resolveType, res) = IsConstantTestValue(forStmt->Test());
        status_ = From(!resolveType || res);
    } else {
        status_ = LivenessStatus::ALIVE;
    }

    AnalyzeStat(forStmt->Body());
    status_ = Or(status_, ResolveContinues(forStmt));
    AnalyzeNode(forStmt->Update());
    status_ = Or(ResolveBreaks(forStmt), From(condType != nullptr && (!resolveType || !res)));
}

void AliveAnalyzer::AnalyzeForOfLoop(const ir::ForOfStatement *forOfStmt)
{
    //  Note: iterator definition can be a reference to variable defined in outer scope!
    if (forOfStmt->Left()->IsVariableDeclaration()) {
        AnalyzeVarDef(forOfStmt->Left()->AsVariableDeclaration());
    } else {
        AnalyzeNode(forOfStmt->Left());
    }
    AnalyzeNode(forOfStmt->Right());
    SetOldPendingExits(PendingExits());

    AnalyzeStat(forOfStmt->Body());
    status_ = Or(status_, ResolveContinues(forOfStmt));
    ResolveBreaks(forOfStmt);
    status_ = LivenessStatus::ALIVE;
}

void AliveAnalyzer::AnalyzeIf(const ir::IfStatement *ifStmt)
{
    AnalyzeNode(ifStmt->Test());
    AnalyzeStat(ifStmt->Consequent());
    if (ifStmt->Alternate() != nullptr) {
        LivenessStatus prevStatus = status_;
        status_ = LivenessStatus::ALIVE;
        AnalyzeStat(ifStmt->Alternate());
        status_ = Or(status_, prevStatus);
    } else {
        status_ = LivenessStatus::ALIVE;
    }
}

void AliveAnalyzer::AnalyzeLabelled(const ir::LabelledStatement *labelledStmt)
{
    SetOldPendingExits(PendingExits());
    AnalyzeStat(labelledStmt->Body());
    status_ = Or(status_, ResolveBreaks(labelledStmt));
}

void AliveAnalyzer::AnalyzeNewClass(const ir::ETSNewClassInstanceExpression *newClass)
{
    for (const auto *it : newClass->GetArguments()) {
        AnalyzeNode(it);
    }
}

void AliveAnalyzer::AnalyzeCall(const ir::CallExpression *callExpr)
{
    AnalyzeNode(callExpr->Callee());
    for (const auto *it : callExpr->Arguments()) {
        AnalyzeNode(it);
    }
    if (callExpr->Signature()->ReturnType() == checker_->GetGlobalTypesHolder()->GlobalETSNeverType()) {
        MarkDead();
    }
}

void AliveAnalyzer::AnalyzeThrow(const ir::ThrowStatement *throwStmt)
{
    AnalyzeNode(throwStmt->Argument());
    MarkDead();
}

void AliveAnalyzer::AnalyzeSwitch(const ir::SwitchStatement *switchStmt)
{
    SetOldPendingExits(PendingExits());

    AnalyzeNode(switchStmt->Discriminant());

    bool hasDefault = false;
    for (std::size_t i = 0, size = switchStmt->Cases().size(); i < size; i++) {
        const auto *caseClause = switchStmt->Cases()[i];
        status_ = LivenessStatus::ALIVE;

        if (caseClause->Test() == nullptr) {
            hasDefault = true;
        } else {
            AnalyzeNode(caseClause->Test());
        }

        AnalyzeStats(caseClause->Consequent());

        if (status_ == LivenessStatus::ALIVE && !caseClause->Consequent().empty() && i < size - 1) {
            // NOTE(user) Add lint categories and option to enable/disable compiler warnings
            checker_->LogDiagnostic(diagnostic::MAYBE_FALLTHROUGH, caseClause->Start());
        }
    }

    if (!hasDefault) {
        status_ = LivenessStatus::ALIVE;
    }

    status_ = Or(status_, ResolveBreaks(switchStmt));
}

void AliveAnalyzer::AnalyzeBreak(const ir::BreakStatement *breakStmt)
{
    RecordExit(PendingExit(breakStmt));
}

void AliveAnalyzer::AnalyzeContinue(const ir::ContinueStatement *contStmt)
{
    RecordExit(PendingExit(contStmt));
}

void AliveAnalyzer::AnalyzeReturn(const ir::ReturnStatement *retStmt)
{
    AnalyzeNode(retStmt->Argument());
    RecordExit(PendingExit(retStmt));
}

void AliveAnalyzer::AnalyzeTry(const ir::TryStatement *tryStmt)
{
    status_ = LivenessStatus::ALIVE;
    bool isAlive = false;
    AnalyzeStats(tryStmt->Block()->Statements());

    if (status_ != LivenessStatus::DEAD) {
        isAlive = true;
    }

    for (const auto &it : tryStmt->CatchClauses()) {
        status_ = LivenessStatus::ALIVE;
        AnalyzeStats(it->Body()->Statements());
        if (status_ == LivenessStatus::ALIVE) {
            isAlive = true;
        }
    }

    if (tryStmt->FinallyBlock() != nullptr) {
        status_ = LivenessStatus::ALIVE;
        AnalyzeStats(tryStmt->FinallyBlock()->Statements());
        const_cast<ir::TryStatement *>(tryStmt)->SetFinallyCanCompleteNormally(status_ == LivenessStatus::ALIVE);
        if (status_ == LivenessStatus::DEAD) {
            isAlive = false;
            // NOTE(user) Add lint categories and option to enable/disable compiler warnings
            checker_->LogDiagnostic(diagnostic::FINALLY_CANT_COMPLETE, tryStmt->FinallyBlock()->Start());
        }
    }

    status_ = isAlive ? LivenessStatus::ALIVE : LivenessStatus::DEAD;
}
}  // namespace ark::es2panda::checker
