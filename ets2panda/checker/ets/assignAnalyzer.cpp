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

#include "assignAnalyzer.h"

#include "ir/base/classDefinition.h"
#include "ir/base/classProperty.h"
#include "ir/base/classStaticBlock.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/ets/etsDestructuring.h"
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
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/binaryExpression.h"
#include "ir/expressions/conditionalExpression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/memberExpression.h"
#include "ir/expressions/objectExpression.h"
#include "ir/expressions/unaryExpression.h"
#include "ir/expressions/updateExpression.h"
#include "ir/expressions/typeofExpression.h"
#include "ir/ets/etsNewClassInstanceExpression.h"
#include "ir/ets/etsStructDeclaration.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "varbinder/ETSBinder.h"
#include "varbinder/variable.h"
#include "varbinder/scope.h"
#include "varbinder/declaration.h"
#include "checker/ETSchecker.h"
#include "checker/ETSAnalyzerHelpers.h"
#include "ir/base/catchClause.h"
#include "parser/program/program.h"
#include "checker/types/ts/objectType.h"

namespace ark::es2panda::checker {

static constexpr NodeId INVALID_ID = -1;
static constexpr bool CHECK_ALL_PROPERTIES = true;
// NOTE(pantos) generic field initialization issue, skip them for now
static constexpr bool CHECK_GENERIC_NON_READONLY_PROPERTIES = false;
static constexpr bool WARN_NO_INIT_ONCE_PER_VARIABLE = false;
static constexpr int LOOP_PHASES = 2;

static bool IsStaticClassProperty(const ir::AstNode *node)
{
    return node->IsClassProperty() && node->IsStatic();
}

static bool ShouldAnalyzeStaticFieldInitializer(const ir::ClassProperty *prop)
{
    return prop->Value() != nullptr && !prop->IsTopLevelLexicalDecl();
}

static bool IsStaticInitNode(const ir::AstNode *node)
{
    return node->IsClassStaticBlock() ||
           (node->IsStatic() && node->IsMethodDefinition() &&
            node->AsMethodDefinition()->Key()->AsIdentifier()->Name().Is(compiler::Signatures::INIT_METHOD));
}

static bool IsNamespaceInitializerBlockMethod(const ir::AstNode *node)
{
    return node->IsStatic() && node->IsMethodDefinition() &&
           node->AsMethodDefinition()->Key()->AsIdentifier()->Name().StartsWith(
               compiler::Signatures::INITIALIZER_BLOCK_INIT);
}

template <typename... Ts>
struct ScopeGuard {
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    std::tuple<Ts...> values;
    std::tuple<Ts &...> refs;
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    explicit ScopeGuard(Ts &...ts) : values(ts...), refs(ts...) {}
    ~ScopeGuard()
    {
        refs = values;
    }

    DEFAULT_COPY_SEMANTIC(ScopeGuard);
    DEFAULT_MOVE_SEMANTIC(ScopeGuard);
};

static std::string Capitalize(const util::StringView &str)
{
    if (str.Empty()) {
        return "";
    }
    std::string ret(str.Utf8());
    ret[0] = std::toupper(ret[0]);
    return ret;
}

AssignAnalyzer::AssignAnalyzer(ETSChecker *checker) : checker_(checker) {}

void AssignAnalyzer::Analyze(const ir::AstNode *node)
{
    const auto program = checker_->VarBinder()->Program();
    globalClass_ = program->GlobalClass();

    AnalyzeClassDef(globalClass_);

    firstNonGlobalAdr_ = nextAdr_;
    AnalyzeNodes(node);
}

void AssignAnalyzer::Warning(const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &list,
                             const lexer::SourcePosition &pos)
{
    ++numErrors_;
    checker_->LogDiagnostic(kind, list, pos);
}

void AssignAnalyzer::AnalyzeNodes(const ir::AstNode *node, const ir::AstNode *currentTopLevelDecl)
{
    node->Iterate([this, currentTopLevelDecl](auto *childNode) { AnalyzeNode(childNode, currentTopLevelDecl); });
}

void AssignAnalyzer::AnalyzeNode(const ir::AstNode *node, const ir::AstNode *currentTopLevelDecl)
{
    ES2PANDA_ASSERT(node != nullptr);

    // NOTE(pantos) these are dummy methods to conform the CI's method size and complexity requirements
    if (AnalyzeStmtNode1(node, currentTopLevelDecl) || AnalyzeStmtNode2(node, currentTopLevelDecl) ||
        AnalyzeExprNode1(node, currentTopLevelDecl) || AnalyzeExprNode2(node, currentTopLevelDecl)) {
        return;
    }

    switch (node->Type()) {
        case ir::AstNodeType::STRUCT_DECLARATION: {
            AnalyzeStructDecl(node->AsETSStructDeclaration());
            break;
        }
        case ir::AstNodeType::CLASS_DECLARATION: {
            AnalyzeClassDecl(node->AsClassDeclaration());
            break;
        }
        case ir::AstNodeType::CLASS_DEFINITION: {
            if (node->AsClassDefinition() != globalClass_) {
                AnalyzeClassDef(node->AsClassDefinition());
            }
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
        default: {
            AnalyzeNodes(node, currentTopLevelDecl);
            if (node->IsExpression()) {
                if (inits_.IsReset()) {
                    Merge();
                }
            }
            break;
        }
    }
}

bool AssignAnalyzer::AnalyzeStmtNode1(const ir::AstNode *node, const ir::AstNode *currentTopLevelDecl)
{
    switch (node->Type()) {
        case ir::AstNodeType::EXPRESSION_STATEMENT: {
            AnalyzeNode(node->AsExpressionStatement()->GetExpression(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::BLOCK_STATEMENT: {
            AnalyzeBlock(node->AsBlockStatement(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::DO_WHILE_STATEMENT: {
            AnalyzeDoLoop(node->AsDoWhileStatement(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::WHILE_STATEMENT: {
            AnalyzeWhileLoop(node->AsWhileStatement(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::FOR_UPDATE_STATEMENT: {
            AnalyzeForLoop(node->AsForUpdateStatement(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::FOR_OF_STATEMENT: {
            AnalyzeForOfLoop(node->AsForOfStatement(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::IF_STATEMENT: {
            AnalyzeIf(node->AsIfStatement(), currentTopLevelDecl);
            break;
        }
        default:
            return false;
    }

    return true;
}

bool AssignAnalyzer::AnalyzeStmtNode2(const ir::AstNode *node, const ir::AstNode *currentTopLevelDecl)
{
    switch (node->Type()) {
        case ir::AstNodeType::LABELLED_STATEMENT: {
            AnalyzeLabelled(node->AsLabelledStatement(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::SWITCH_STATEMENT: {
            AnalyzeSwitch(node->AsSwitchStatement(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::TRY_STATEMENT: {
            AnalyzeTry(node->AsTryStatement(), currentTopLevelDecl);
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
            AnalyzeReturn(node->AsReturnStatement(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::THROW_STATEMENT: {
            AnalyzeThrow(node->AsThrowStatement(), currentTopLevelDecl);
            break;
        }
        default:
            return false;
    }

    return true;
}

bool AssignAnalyzer::AnalyzeExprNode1(const ir::AstNode *node, const ir::AstNode *currentTopLevelDecl)
{
    switch (node->Type()) {
        case ir::AstNodeType::ETS_NEW_CLASS_INSTANCE_EXPRESSION: {
            AnalyzeNewClass(node->AsETSNewClassInstanceExpression(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::CALL_EXPRESSION: {
            AnalyzeCallExpr(node->AsCallExpression(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::IDENTIFIER: {
            AnalyzeId(node->AsIdentifier(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::ASSIGNMENT_EXPRESSION: {
            AnalyzeAssignExpr(node->AsAssignmentExpression(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::CONDITIONAL_EXPRESSION: {
            AnalyzeCondExpr(node->AsConditionalExpression(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::MEMBER_EXPRESSION: {
            AnalyzeMemberExpr(node->AsMemberExpression(), currentTopLevelDecl);
            break;
        }
        default:
            return false;
    }

    return true;
}

bool AssignAnalyzer::AnalyzeExprNode2(const ir::AstNode *node, const ir::AstNode *currentTopLevelDecl)
{
    switch (node->Type()) {
        case ir::AstNodeType::BINARY_EXPRESSION: {
            AnalyzeBinaryExpr(node->AsBinaryExpression(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::UNARY_EXPRESSION: {
            AnalyzeUnaryExpr(node->AsUnaryExpression(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::UPDATE_EXPRESSION: {
            AnalyzeUpdateExpr(node->AsUpdateExpression(), currentTopLevelDecl);
            break;
        }
        case ir::AstNodeType::ARROW_FUNCTION_EXPRESSION: {
            AnalyzeArrowFunctionExpr(node->AsArrowFunctionExpression(), currentTopLevelDecl);
            break;
        }
        default:
            return false;
    }

    return true;
}

void AssignAnalyzer::AnalyzeStat(const ir::AstNode *node, const ir::AstNode *currentTopLevelDecl)
{
    ES2PANDA_ASSERT(node != nullptr);
    AnalyzeNode(node, currentTopLevelDecl);
}

void AssignAnalyzer::AnalyzeStats(const ArenaVector<ir::Statement *> &stats, const ir::AstNode *currentTopLevelDecl)
{
    for (const auto it : stats) {
        AnalyzeStat(it, currentTopLevelDecl);
    }
}

void AssignAnalyzer::AnalyzeBlock(const ir::BlockStatement *blockStmt, const ir::AstNode *currentTopLevelDecl)
{
    ScopeGuard save(nextAdr_);

    AnalyzeStats(blockStmt->Statements(), currentTopLevelDecl);
}

void AssignAnalyzer::AnalyzeStructDecl(const ir::ETSStructDeclaration *structDecl)
{
    AnalyzeNode(structDecl->Definition());
}

void AssignAnalyzer::AnalyzeClassDecl(const ir::ClassDeclaration *classDecl)
{
    AnalyzeNode(classDecl->Definition());
}

void AssignAnalyzer::AnalyzeClassDef(const ir::ClassDefinition *classDef)
{
    SetOldPendingExits(PendingExits());

    ScopeGuard save(firstAdr_, nextAdr_, classDef_, classFirstAdr_);

    classDef_ = classDef;
    firstAdr_ = nextAdr_;
    classFirstAdr_ = nextAdr_;

    ProcessClassDefStaticFields(classDef_);

    // define all the instance fields
    for (const auto it : classDef->Body()) {
        if (it->IsClassProperty() && !it->IsStatic()) {
            const auto prop = it->AsClassProperty();
            NewVar(prop);
            if (prop->Value() != nullptr) {
                LetInit(prop);
            }
        }
    }

    CheckAnonymousClassCtor(classDef_);

    // process all the methods
    std::vector<const ir::AstNode *> methods;
    for (const auto it : classDef->Body()) {
        if (it->IsMethodDefinition()) {
            const auto methodDef = it->AsMethodDefinition();
            if (methodDef->Key()->AsIdentifier()->Name().Is(compiler::Signatures::INIT_METHOD)) {
                // skip the special init method as we have already checked it
                continue;
            }

            methods.push_back(methodDef);

            for (const auto it2 : methodDef->Overloads()) {
                methods.push_back(it2);
            }
        }
    }

    for (const auto it : methods) {
        AnalyzeNode(it);
    }

    SetPendingExits(OldPendingExits());
}

// NOTE (pantos) awkward methods to conform method length/complexity requirements of CI...
void AssignAnalyzer::ProcessClassDefStaticFields(const ir::ClassDefinition *classDef)
{
    for (const auto it : classDef->Body()) {
        if (IsStaticClassProperty(it)) {
            NewVar(it->AsClassProperty());
        }
    }

    for (const auto it : classDef->Body()) {
        inStaticFieldInit_ = false;
        if (!IsStaticClassProperty(it)) {
            continue;
        }

        const auto prop = it->AsClassProperty();
        if (!ShouldAnalyzeStaticFieldInitializer(prop)) {
            continue;
        }

        inStaticFieldInit_ = true;
        AnalyzeNode(prop->Value());
        LetInit(prop);
    }
    for (const auto it : classDef->Body()) {
        if (IsStaticInitNode(it)) {
            AnalyzeNodes(it);
            ClearPendingExits();
        }
    }

    for (const auto it : classDef->Body()) {
        if (IsNamespaceInitializerBlockMethod(it)) {
            AnalyzeNodes(it);
            ClearPendingExits();
        }
    }
    if (!classDef->IsModule()) {
        for (int i = firstAdr_; i < nextAdr_; i++) {
            const ir::AstNode *var = varDecls_[i];
            if (var->IsStatic() && (var->IsConst() || CHECK_ALL_PROPERTIES)) {
                CheckInit(var);
            }
        }
    }
}

void AssignAnalyzer::CheckAnonymousClassCtor(const ir::ClassDefinition *classDef)
{
    if (classDef == globalClass_) {
        return;
    }

    // NOTE(pantos) anonymous classes of new expressions has no default ctor right now
    // but this feature might be completely removed from the spec...
    bool hasCtor = false;
    for (const auto it : classDef->Body()) {
        if (it->IsMethodDefinition() && it->AsMethodDefinition()->IsConstructor()) {
            hasCtor = true;
            break;
        }
    }
    if (!hasCtor) {
        for (int i = firstAdr_; i < nextAdr_; i++) {
            const ir::AstNode *var = varDecls_[i];
            if (!var->IsStatic() && (var->IsConst() || CHECK_ALL_PROPERTIES)) {
                CheckInit(var);
            }
        }
    }
}

// NOTE(pantos) modified version of ETSChecker::CheckCyclicConstructorCall
static bool IsInitialConstructor(const ir::AstNode *node)
{
    if (!node->IsMethodDefinition() || !node->AsMethodDefinition()->IsConstructor()) {
        return false;
    }

    const auto methodDef = node->AsMethodDefinition();
    ES2PANDA_ASSERT(methodDef != nullptr);
    if (methodDef->Function()->Body() == nullptr || methodDef->Function()->IsExternal()) {
        return false;
    }

    auto stmts = methodDef->Function()->Body()->AsBlockStatement()->Statements();
    if (stmts.empty()) {
        return true;
    }
    auto firstStmt = *std::find_if(stmts.begin(), stmts.end(), [](const ir::Statement *stmt) {
        return !stmt->HasAstNodeFlags(ir::AstNodeFlags::DEFAULT_PARAM);
    });
    if (firstStmt == nullptr) {
        return true;
    }
    return !(firstStmt->IsExpressionStatement() &&
             firstStmt->AsExpressionStatement()->GetExpression()->IsCallExpression() &&
             firstStmt->AsExpressionStatement()->GetExpression()->AsCallExpression()->Callee()->IsThisExpression());
}

void AssignAnalyzer::AnalyzeMethodDef(const ir::MethodDefinition *methodDef)
{
    auto *func = methodDef->Function();
    ES2PANDA_ASSERT(func != nullptr);
    if (func->Body() == nullptr || func->IsProxy()) {
        return;
    }

    Set initsPrev = inits_;
    Set uninitsPrev = uninits_;

    ScopeGuard save(firstAdr_, nextAdr_, returnAdr_, isInitialConstructor_);

    hasTryFinallyBlock_ = func->IsAnyChild([](ir::AstNode *ast) {
        return (ast->Type() == ir::AstNodeType::TRY_STATEMENT && ast->AsTryStatement()->FinallyBlock() != nullptr);
    });
    isInitialConstructor_ = IsInitialConstructor(methodDef);
    if (!isInitialConstructor_) {
        firstAdr_ = nextAdr_;
    }

    AnalyzeStat(func->Body());

    if (isInitialConstructor_) {
        for (int i = firstAdr_; i < nextAdr_; i++) {
            const ir::AstNode *var = varDecls_[i];
            if (!var->IsStatic() && (var->IsConst() || CHECK_ALL_PROPERTIES)) {
                CheckInit(var);
            }
        }
    }

    CheckPendingExits();

    inits_ = std::move(initsPrev);
    uninits_ = std::move(uninitsPrev);
}

void AssignAnalyzer::AnalyzeVarDef(const ir::VariableDeclaration *varDef)
{
    for (auto *var : varDef->Declarators()) {
        NewVar(var);

        if (var->Init() != nullptr) {
            AnalyzeExpr(var->Init());
            LetInit(var);
        }
    }
}

void AssignAnalyzer::AnalyzeDoLoop(const ir::DoWhileStatement *doWhileStmt, const ir::AstNode *currentTopLevelDecl)
{
    SetOldPendingExits(PendingExits());

    Set initsSkip {};
    Set uninitsSkip {};
    int prevErrors = numErrors_;

    for (int phase = 1; phase <= LOOP_PHASES; phase++) {
        Set uninitsEntry = uninits_;
        uninitsEntry.ExcludeFrom(nextAdr_);

        AnalyzeStat(doWhileStmt->Body(), currentTopLevelDecl);

        ResolveContinues(doWhileStmt);

        AnalyzeCond(doWhileStmt->Test(), currentTopLevelDecl);

        if (phase == 1) {
            initsSkip = initsWhenFalse_;
            uninitsSkip = uninitsWhenFalse_;
        }

        if (prevErrors != numErrors_ || phase == LOOP_PHASES ||
            uninitsEntry.DiffSet(uninitsWhenTrue_).Next(firstAdr_) == -1) {
            break;
        }

        inits_ = initsWhenTrue_;
        uninits_ = uninitsEntry.AndSet(uninitsWhenTrue_);
    }

    inits_ = std::move(initsSkip);
    uninits_ = std::move(uninitsSkip);

    ResolveBreaks(doWhileStmt);
}

void AssignAnalyzer::AnalyzeWhileLoop(const ir::WhileStatement *whileStmt, const ir::AstNode *currentTopLevelDecl)
{
    SetOldPendingExits(PendingExits());

    Set initsSkip {};
    Set uninitsSkip {};
    int prevErrors = numErrors_;

    Set uninitsEntry = uninits_;
    uninitsEntry.ExcludeFrom(nextAdr_);

    for (int phase = 1; phase <= LOOP_PHASES; phase++) {
        AnalyzeCond(whileStmt->Test(), currentTopLevelDecl);

        if (phase == 1) {
            initsSkip = initsWhenFalse_;
            uninitsSkip = uninitsWhenFalse_;
        }

        inits_ = initsWhenTrue_;
        uninits_ = uninitsWhenTrue_;

        AnalyzeStat(whileStmt->Body(), currentTopLevelDecl);

        ResolveContinues(whileStmt);

        if (prevErrors != numErrors_ || phase == LOOP_PHASES || uninitsEntry.DiffSet(uninits_).Next(firstAdr_) == -1) {
            break;
        }

        uninits_ = uninitsEntry.AndSet(uninits_);
    }

    inits_ = std::move(initsSkip);
    uninits_ = std::move(uninitsSkip);

    ResolveBreaks(whileStmt);
}

void AssignAnalyzer::AnalyzeForLoop(const ir::ForUpdateStatement *forStmt, const ir::AstNode *currentTopLevelDecl)
{
    ScopeGuard save(nextAdr_);

    if (forStmt->Init() != nullptr) {
        AnalyzeNode(forStmt->Init(), currentTopLevelDecl);
    }

    Set initsSkip {};
    Set uninitsSkip {};
    int prevErrors = numErrors_;

    SetOldPendingExits(PendingExits());

    for (int phase = 1; phase <= LOOP_PHASES; phase++) {
        Set uninitsEntry = uninits_;
        uninitsEntry.ExcludeFrom(nextAdr_);

        if (forStmt->Test() != nullptr) {
            AnalyzeCond(forStmt->Test(), currentTopLevelDecl);

            if (phase == 1) {
                initsSkip = initsWhenFalse_;
                uninitsSkip = uninitsWhenFalse_;
            }

            inits_ = initsWhenTrue_;
            uninits_ = uninitsWhenTrue_;
        } else if (phase == 1) {
            initsSkip = inits_;
            initsSkip.InclRange(firstAdr_, nextAdr_);
            uninitsSkip = uninits_;
            uninitsSkip.InclRange(firstAdr_, nextAdr_);
        }

        AnalyzeStat(forStmt->Body(), currentTopLevelDecl);

        ResolveContinues(forStmt);

        if (forStmt->Update() != nullptr) {
            AnalyzeNode(forStmt->Update(), currentTopLevelDecl);
        }

        if (prevErrors != numErrors_ || phase == LOOP_PHASES || uninitsEntry.DiffSet(uninits_).Next(firstAdr_) == -1) {
            break;
        }

        uninits_ = uninitsEntry.AndSet(uninits_);
    }

    inits_ = std::move(initsSkip);
    uninits_ = std::move(uninitsSkip);

    ResolveBreaks(forStmt);
}

void AssignAnalyzer::AnalyzeForOfLoop(const ir::ForOfStatement *forOfStmt, const ir::AstNode *currentTopLevelDecl)
{
    ScopeGuard save(nextAdr_);

    if (forOfStmt->Left()->IsVariableDeclaration()) {
        AnalyzeVarDef(forOfStmt->Left()->AsVariableDeclaration());
        for (auto *var : forOfStmt->Left()->AsVariableDeclaration()->Declarators()) {
            LetInit(var);
        }
    } else {
        LetInit(forOfStmt->Left());
    }

    AnalyzeNode(forOfStmt->Right(), currentTopLevelDecl);

    Set initsStart = inits_;
    Set uninitsStart = uninits_;
    int prevErrors = numErrors_;

    SetOldPendingExits(PendingExits());

    for (int phase = 1; phase <= LOOP_PHASES; phase++) {
        Set uninitsEntry = uninits_;
        uninitsEntry.ExcludeFrom(nextAdr_);

        AnalyzeStat(forOfStmt->Body(), currentTopLevelDecl);

        ResolveContinues(forOfStmt);

        if (prevErrors != numErrors_ || phase == LOOP_PHASES || uninitsEntry.DiffSet(uninits_).Next(firstAdr_) == -1) {
            break;
        }

        uninits_ = uninitsEntry.AndSet(uninits_);
    }

    inits_ = initsStart;
    uninits_ = std::move(uninitsStart.AndSet(uninits_));

    ResolveBreaks(forOfStmt);
}

void AssignAnalyzer::AnalyzeIf(const ir::IfStatement *ifStmt, const ir::AstNode *currentTopLevelDecl)
{
    AnalyzeCond(ifStmt->Test(), currentTopLevelDecl);

    Set initsBeforeElse = initsWhenFalse_;
    Set uninitsBeforeElse = uninitsWhenFalse_;
    inits_ = initsWhenTrue_;
    uninits_ = uninitsWhenTrue_;

    AnalyzeStat(ifStmt->Consequent(), currentTopLevelDecl);

    if (ifStmt->Alternate() != nullptr) {
        Set initsAfterThen = std::move(inits_);
        Set uninitsAfterThen = std::move(uninits_);
        inits_ = std::move(initsBeforeElse);
        uninits_ = std::move(uninitsBeforeElse);

        AnalyzeStat(ifStmt->Alternate(), currentTopLevelDecl);

        inits_.AndSet(initsAfterThen);
        uninits_.AndSet(uninitsAfterThen);
    } else {
        inits_.AndSet(initsBeforeElse);
        uninits_.AndSet(uninitsBeforeElse);
    }
}

void AssignAnalyzer::AnalyzeLabelled(const ir::LabelledStatement *labelledStmt, const ir::AstNode *currentTopLevelDecl)
{
    SetOldPendingExits(PendingExits());

    AnalyzeStat(labelledStmt->Body(), currentTopLevelDecl);

    ResolveBreaks(labelledStmt);
}

void AssignAnalyzer::AnalyzeSwitch(const ir::SwitchStatement *switchStmt, const ir::AstNode *currentTopLevelDecl)
{
    SetOldPendingExits(PendingExits());

    ScopeGuard save(nextAdr_);

    AnalyzeNode(switchStmt->Discriminant(), currentTopLevelDecl);

    Set initsSwitch = inits_;
    Set uninitsSwitch = uninits_;

    bool hasDefault = false;

    for (const auto caseClause : switchStmt->Cases()) {
        inits_ = initsSwitch;
        uninits_ = uninits_.AndSet(uninitsSwitch);

        if (caseClause->Test() == nullptr) {
            hasDefault = true;
        } else {
            AnalyzeNode(caseClause->Test(), currentTopLevelDecl);
        }

        if (hasDefault) {
            inits_ = initsSwitch;
            uninits_ = uninits_.AndSet(uninitsSwitch);
        }

        AnalyzeStats(caseClause->Consequent(), currentTopLevelDecl);

        for (const auto stmt : caseClause->Consequent()) {
            if (!stmt->IsVariableDeclaration()) {
                continue;
            }
            for (auto *var : stmt->AsVariableDeclaration()->Declarators()) {
                NodeId adr = GetNodeId(var);
                ES2PANDA_ASSERT(adr >= 0);
                initsSwitch.Excl(adr);
                uninitsSwitch.Incl(adr);
            }
        }

        if (!hasDefault) {
            inits_ = initsSwitch;
            uninits_ = uninits_.AndSet(uninitsSwitch);
        }
    }

    if (!hasDefault) {
        inits_.AndSet(initsSwitch);
    }

    ResolveBreaks(switchStmt);
}

void AssignAnalyzer::AnalyzeTry(const ir::TryStatement *tryStmt, const ir::AstNode *currentTopLevelDecl)
{
    Set uninitsTryPrev = uninitsTry_;

    PendingExitsSet prevPendingExits = PendingExits();
    SetOldPendingExits(prevPendingExits);

    Set initsTry = inits_;
    uninitsTry_ = uninits_;

    AnalyzeNode(tryStmt->Block(), currentTopLevelDecl);

    uninitsTry_.AndSet(uninits_);

    Set initsEnd = inits_;
    Set uninitsEnd = uninits_;
    int nextAdrCatch = nextAdr_;

    Set initsCatchPrev = initsTry;  // NOLINT(performance-unnecessary-copy-initialization)
    Set uninitsCatchPrev = uninitsTry_;

    for (const auto catchClause : tryStmt->CatchClauses()) {
        inits_ = initsCatchPrev;
        uninits_ = uninitsCatchPrev;

        AnalyzeNode(catchClause->Body(), currentTopLevelDecl);

        initsEnd.AndSet(inits_);
        uninitsEnd.AndSet(uninits_);
        nextAdr_ = nextAdrCatch;
    }

    if (tryStmt->FinallyBlock() != nullptr) {
        inits_ = std::move(initsTry);
        uninits_ = uninitsTry_;

        PendingExitsSet exits = PendingExits();
        SetPendingExits(prevPendingExits);

        AnalyzeNode(tryStmt->FinallyBlock(), currentTopLevelDecl);

        if (tryStmt->FinallyCanCompleteNormally()) {
            uninits_.AndSet(uninitsEnd);
            for (auto exit : exits) {
                exit.exitInits_.OrSet(inits_);
                exit.exitUninits_.AndSet(uninits_);
                PendingExits().insert(exit);
            }
            inits_.OrSet(initsEnd);
        }
    } else {
        inits_ = std::move(initsEnd);
        uninits_ = std::move(uninitsEnd);

        PendingExitsSet exits = PendingExits();
        SetPendingExits(prevPendingExits);

        for (const auto &exit : exits) {
            PendingExits().insert(exit);
        }
    }

    uninitsTry_.AndSet(uninitsTryPrev).AndSet(uninits_);
}

void AssignAnalyzer::AnalyzeBreak(const ir::BreakStatement *breakStmt)
{
    RecordExit(AssignPendingExit(breakStmt, inits_, uninits_, isInitialConstructor_, hasTryFinallyBlock_));
}

void AssignAnalyzer::AnalyzeContinue(const ir::ContinueStatement *contStmt)
{
    RecordExit(AssignPendingExit(contStmt, inits_, uninits_, isInitialConstructor_, hasTryFinallyBlock_));
}

void AssignAnalyzer::AnalyzeReturn(const ir::ReturnStatement *retStmt, const ir::AstNode *currentTopLevelDecl)
{
    if (retStmt->Argument() != nullptr) {
        AnalyzeNode(retStmt->Argument(), currentTopLevelDecl);
    }
    RecordExit(AssignPendingExit(retStmt, inits_, uninits_, isInitialConstructor_, hasTryFinallyBlock_));
}

void AssignAnalyzer::AnalyzeThrow(const ir::ThrowStatement *throwStmt, const ir::AstNode *currentTopLevelDecl)
{
    AnalyzeNode(throwStmt->Argument(), currentTopLevelDecl);
    MarkDead();
}

void AssignAnalyzer::AnalyzeExpr(const ir::AstNode *node, const ir::AstNode *currentTopLevelDecl)
{
    ES2PANDA_ASSERT(node != nullptr);
    AnalyzeNode(node, currentTopLevelDecl);
    if (inits_.IsReset()) {
        Merge();
    }
}

void AssignAnalyzer::AnalyzeExprs(const ArenaVector<ir::Expression *> &exprs, const ir::AstNode *currentTopLevelDecl)
{
    for (const auto it : exprs) {
        AnalyzeExpr(it, currentTopLevelDecl);
    }
}

void AssignAnalyzer::AnalyzeCond(const ir::AstNode *node, const ir::AstNode *currentTopLevelDecl)
{
    ES2PANDA_ASSERT(node->IsExpression());
    const ir::Expression *expr = node->AsExpression();

    if (auto etype = expr->TsType();
        etype != nullptr && etype->IsETSBooleanType() && etype->HasTypeFlag(TypeFlag::CONSTANT)) {
        const ETSBooleanType *condType = etype->AsETSBooleanType();
        if (inits_.IsReset()) {
            Merge();
        }
        if (condType->GetValue()) {
            initsWhenFalse_ = inits_;
            initsWhenFalse_.InclRange(firstAdr_, nextAdr_);
            uninitsWhenFalse_ = uninits_;
            uninitsWhenFalse_.InclRange(firstAdr_, nextAdr_);
            initsWhenTrue_ = inits_;
            uninitsWhenTrue_ = uninits_;
        } else {
            initsWhenTrue_ = inits_;
            initsWhenTrue_.InclRange(firstAdr_, nextAdr_);
            uninitsWhenTrue_ = uninits_;
            uninitsWhenTrue_.InclRange(firstAdr_, nextAdr_);
            initsWhenFalse_ = inits_;
            uninitsWhenFalse_ = uninits_;
        }
    } else {
        AnalyzeNode(node, currentTopLevelDecl);
        if (!inits_.IsReset()) {
            Split(true);
        }
    }

    inits_.Reset();
    uninits_.Reset();
}

void AssignAnalyzer::AnalyzeId(const ir::Identifier *id, const ir::AstNode *currentTopLevelDecl)
{
    if (id->Parent()->IsProperty() && id->Parent()->AsProperty()->Key() == id &&
        id->Parent()->Parent()->IsObjectExpression()) {
        return;  // inside ObjectExpression
    }

    if (id->Parent()->IsBinaryExpression()) {
        const ir::BinaryExpression *binExpr = id->Parent()->AsBinaryExpression();
        if ((binExpr->OperatorType() == lexer::TokenType::PUNCTUATOR_EQUAL ||
             binExpr->OperatorType() == lexer::TokenType::PUNCTUATOR_NOT_EQUAL) &&
            (binExpr->Left()->IsNullLiteral() || binExpr->Right()->IsNullLiteral() ||
             binExpr->Left()->IsUndefinedLiteral() || binExpr->Right()->IsUndefinedLiteral())) {
            return;  // null/undefined comparison with == or != operators (e.g. in assert statement)
        }
    }

    if (id->Parent()->IsMemberExpression()) {
        const ir::MemberExpression *membExpr = id->Parent()->AsMemberExpression();
        if (id == membExpr->Property() && !membExpr->Object()->IsThisExpression() &&
            membExpr->HasMemberKind(ir::MemberExpressionKind::PROPERTY_ACCESS)) {
            return;  // something.property
        }
    }

    CheckInit(id, currentTopLevelDecl);
}

static bool IsIdentOrThisDotIdent(const ir::AstNode *node)
{
    return node->IsIdentifier() ||
           (node->IsMemberExpression() && node->AsMemberExpression()->Object()->IsThisExpression());
}

static bool IsDeclaredBefore(const ir::AstNode *declNode, const ir::AstNode *node)
{
    return declNode->Start().Program() == node->Start().Program() && declNode->Start().index < node->Start().index;
}

static bool CanUseDefaultValueAtCurrentPoint(const ir::AstNode *declNode, const ir::AstNode *node,
                                             const ir::AstNode *currentTopLevelDecl)
{
    if (declNode == currentTopLevelDecl) {
        return false;
    }

    if (currentTopLevelDecl == nullptr || !declNode->IsClassProperty() || !currentTopLevelDecl->IsClassProperty()) {
        return true;
    }

    const auto *declProp = declNode->AsClassProperty();
    const auto *currentProp = currentTopLevelDecl->AsClassProperty();
    if (!declProp->IsTopLevelLexicalDecl() || !currentProp->IsTopLevelLexicalDecl()) {
        return true;
    }

    return IsDeclaredBefore(declNode, node);
}

static bool ShouldReportTopLevelAssignmentTargetUseBeforeInit(const ir::AstNode *declNode,
                                                              const ir::AstNode *currentTopLevelDecl,
                                                              const ir::AstNode *target)
{
    if (declNode == nullptr || currentTopLevelDecl == nullptr || !declNode->IsClassProperty() ||
        !currentTopLevelDecl->IsClassProperty()) {
        return false;
    }

    const auto *declProp = declNode->AsClassProperty();
    const auto *currentProp = currentTopLevelDecl->AsClassProperty();
    return declProp->IsTopLevelLexicalDecl() && currentProp->IsTopLevelLexicalDecl() &&
           !IsDeclaredBefore(declNode, target);
}

void AssignAnalyzer::AnalyzeAssignExpr(const ir::AssignmentExpression *assignExpr,
                                       const ir::AstNode *currentTopLevelDecl)
{
    const ir::AstNode *declNode = GetDeclaringNode(assignExpr->Left());

    if (assignExpr->Left()->IsETSDestructuring()) {
        auto *dstrNode = assignExpr->Left()->AsETSDestructuring();
        for (auto *elem : dstrNode->Elements()) {
            LetInit(elem);
        }
    } else if (!IsIdentOrThisDotIdent(assignExpr->Left())) {
        AnalyzeExpr(assignExpr->Left(), currentTopLevelDecl);
    }

    // handle arrow function assignment, where the arrow function is recursive, e.g. f = () => { ...;f();... };
    NodeId provisionalInitAdr = INVALID_ID;
    if (assignExpr->OperatorType() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION &&
        assignExpr->Right()->IsArrowFunctionExpression()) {
        if (declNode != nullptr && !declNode->IsDeclare()) {
            provisionalInitAdr = GetNodeId(declNode);
            if (provisionalInitAdr != INVALID_ID && !inits_.IsMember(provisionalInitAdr)) {
                inits_.Incl(provisionalInitAdr);
            } else {
                provisionalInitAdr = INVALID_ID;
            }
        }
    }

    const ir::AstNode *rhsCurrentTopLevelDecl = currentTopLevelDecl;
    if (IsTopLevelDeclInitAssignment(assignExpr, declNode)) {
        rhsCurrentTopLevelDecl = declNode;
    }
    AnalyzeExpr(assignExpr->Right(), rhsCurrentTopLevelDecl);

    if (provisionalInitAdr != INVALID_ID) {
        inits_.Excl(provisionalInitAdr);
    }

    if (assignExpr->OperatorType() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION) {
        if (ShouldReportTopLevelAssignmentTargetUseBeforeInit(declNode, currentTopLevelDecl, assignExpr->Left())) {
            ReportUseBeforeInit(assignExpr->Left(), declNode);
        }
        LetInit(assignExpr->Left());
    } else {
        CheckInit(assignExpr->Left(), currentTopLevelDecl);
    }
}

void AssignAnalyzer::AnalyzeCondExpr(const ir::ConditionalExpression *condExpr, const ir::AstNode *currentTopLevelDecl)
{
    AnalyzeCond(condExpr->Test(), currentTopLevelDecl);

    if (auto const testValue = TryResolveConditionalTestValue(condExpr->Test()); testValue.has_value()) {
        auto *const takenBranch = testValue.value() ? condExpr->Consequent() : condExpr->Alternate();
        inits_ = testValue.value() ? initsWhenTrue_ : initsWhenFalse_;
        uninits_ = testValue.value() ? uninitsWhenTrue_ : uninitsWhenFalse_;

        if (auto *const branchType = takenBranch->TsType(); branchType != nullptr && branchType->IsETSBooleanType()) {
            AnalyzeCond(takenBranch, currentTopLevelDecl);
        } else {
            AnalyzeExpr(takenBranch, currentTopLevelDecl);
        }
        return;
    }

    Set initsBeforeElse = initsWhenFalse_;
    Set uninitsBeforeElse = uninitsWhenFalse_;
    inits_ = initsWhenTrue_;
    uninits_ = uninitsWhenTrue_;

    ES2PANDA_ASSERT(condExpr->Consequent()->TsType() && condExpr->Alternate()->TsType());

    if (condExpr->Consequent()->TsType()->IsETSBooleanType() && condExpr->Alternate()->TsType()->IsETSBooleanType()) {
        AnalyzeCond(condExpr->Consequent(), currentTopLevelDecl);

        Set initsAfterThenWhenTrue = initsWhenTrue_;
        Set initsAfterThenWhenFalse = initsWhenFalse_;
        Set uninitsAfterThenWhenTrue = uninitsWhenTrue_;
        Set uninitsAfterThenWhenFalse = uninitsWhenFalse_;
        inits_ = std::move(initsBeforeElse);
        uninits_ = std::move(uninitsBeforeElse);

        AnalyzeCond(condExpr->Alternate(), currentTopLevelDecl);

        initsWhenTrue_.AndSet(initsAfterThenWhenTrue);
        initsWhenFalse_.AndSet(initsAfterThenWhenFalse);
        uninitsWhenTrue_.AndSet(uninitsAfterThenWhenTrue);
        uninitsWhenFalse_.AndSet(uninitsAfterThenWhenFalse);
    } else {
        AnalyzeExpr(condExpr->Consequent(), currentTopLevelDecl);

        Set initsAfterThen = inits_;
        Set uninitsAfterThen = uninits_;
        inits_ = std::move(initsBeforeElse);
        uninits_ = std::move(uninitsBeforeElse);

        AnalyzeExpr(condExpr->Alternate(), currentTopLevelDecl);

        inits_.AndSet(initsAfterThen);
        uninits_.AndSet(uninitsAfterThen);
    }
}

void AssignAnalyzer::AnalyzeCallExpr(const ir::CallExpression *callExpr, const ir::AstNode *currentTopLevelDecl)
{
    AnalyzeExpr(callExpr->Callee(), currentTopLevelDecl);
    AnalyzeExprs(callExpr->Arguments(), currentTopLevelDecl);
}

void AssignAnalyzer::AnalyzeMemberExpr(const ir::MemberExpression *membExpr, const ir::AstNode *currentTopLevelDecl)
{
    if (inStaticFieldInit_) {
        CheckInit(membExpr, currentTopLevelDecl);
    }
    if (membExpr->Object()->IsThisExpression() && membExpr->HasMemberKind(ir::MemberExpressionKind::PROPERTY_ACCESS)) {
        CheckInit(membExpr, currentTopLevelDecl);
    } else {
        AnalyzeNode(membExpr->Object(), currentTopLevelDecl);
        AnalyzeNode(membExpr->Property(), currentTopLevelDecl);
    }
}

void AssignAnalyzer::AnalyzeNewClass(const ir::ETSNewClassInstanceExpression *newClass,
                                     const ir::AstNode *currentTopLevelDecl)
{
    AnalyzeExpr(newClass->GetTypeRef(), currentTopLevelDecl);
    AnalyzeExprs(newClass->GetArguments(), currentTopLevelDecl);
}

void AssignAnalyzer::AnalyzeUnaryExpr(const ir::UnaryExpression *unaryExpr, const ir::AstNode *currentTopLevelDecl)
{
    AnalyzeCond(unaryExpr->Argument(), currentTopLevelDecl);

    switch (unaryExpr->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK: {
            Set ti = std::move(initsWhenFalse_);
            initsWhenFalse_ = std::move(initsWhenTrue_);
            initsWhenTrue_ = std::move(ti);
            Set tu = std::move(uninitsWhenFalse_);
            uninitsWhenFalse_ = std::move(uninitsWhenTrue_);
            uninitsWhenTrue_ = std::move(tu);
            break;
        }
        default: {
            AnalyzeExpr(unaryExpr->Argument(), currentTopLevelDecl);
            break;
        }
    }
}

void AssignAnalyzer::AnalyzeBinaryExpr(const ir::BinaryExpression *binExpr, const ir::AstNode *currentTopLevelDecl)
{
    switch (binExpr->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_LOGICAL_AND: {
            AnalyzeCond(binExpr->Left(), currentTopLevelDecl);
            Set initsWhenFalseLeft = initsWhenFalse_;
            Set uninitsWhenFalseLeft = uninitsWhenFalse_;
            inits_ = initsWhenTrue_;
            uninits_ = uninitsWhenTrue_;
            AnalyzeCond(binExpr->Right(), currentTopLevelDecl);
            initsWhenFalse_.AndSet(initsWhenFalseLeft);
            uninitsWhenFalse_.AndSet(uninitsWhenFalseLeft);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_LOGICAL_OR: {
            AnalyzeCond(binExpr->Left(), currentTopLevelDecl);
            Set initsWhenTrueLeft = initsWhenTrue_;
            Set uninitsWhenTrueLeft = uninitsWhenTrue_;
            inits_ = initsWhenFalse_;
            uninits_ = uninitsWhenFalse_;
            AnalyzeCond(binExpr->Right(), currentTopLevelDecl);
            initsWhenTrue_.AndSet(initsWhenTrueLeft);
            uninitsWhenTrue_.AndSet(uninitsWhenTrueLeft);
            break;
        }
        default: {
            AnalyzeExpr(binExpr->Left(), currentTopLevelDecl);
            AnalyzeExpr(binExpr->Right(), currentTopLevelDecl);
            break;
        }
    }
}

void AssignAnalyzer::AnalyzeUpdateExpr(const ir::UpdateExpression *updateExpr, const ir::AstNode *currentTopLevelDecl)
{
    AnalyzeExpr(updateExpr->Argument(), currentTopLevelDecl);
    LetInit(updateExpr->Argument());
}

void AssignAnalyzer::AnalyzeArrowFunctionExpr(const ir::ArrowFunctionExpression *arrowFuncExpr,
                                              const ir::AstNode *currentTopLevelDecl)
{
    auto *func = arrowFuncExpr->Function();
    ES2PANDA_ASSERT(func != nullptr);
    if (func->Body() == nullptr || func->IsProxy()) {
        return;
    }

    Set initsPrev = inits_;
    Set uninitsPrev = uninits_;
    ScopeGuard save(firstAdr_, nextAdr_, returnAdr_, isInitialConstructor_);
    hasTryFinallyBlock_ = func->IsAnyChild([](ir::AstNode *ast) {
        return (ast->Type() == ir::AstNodeType::TRY_STATEMENT && ast->AsTryStatement()->FinallyBlock() != nullptr);
    });
    isInitialConstructor_ = false;
    firstAdr_ = nextAdr_;

    AnalyzeStat(func->Body(), currentTopLevelDecl);
    CheckPendingExits();

    inits_ = std::move(initsPrev);
    uninits_ = std::move(uninitsPrev);
}

util::StringView AssignAnalyzer::GetVariableType(const ir::AstNode *node) const
{
    switch (node->Type()) {
        case ir::AstNodeType::CLASS_PROPERTY:
            if (node->AsClassProperty()->Parent() == globalClass_) {
                return "variable";
            } else {
                return "property";
            }
        case ir::AstNodeType::VARIABLE_DECLARATOR:
            return "variable";
        default:
            ES2PANDA_UNREACHABLE();
    }
}

util::StringView AssignAnalyzer::GetVariableName(const ir::AstNode *node) const
{
    switch (node->Type()) {
        case ir::AstNodeType::CLASS_PROPERTY: {
            const ir::Identifier *identifier = node->AsClassProperty()->Id();
            ES2PANDA_ASSERT(identifier != nullptr);
            return identifier->Name();
        }
        case ir::AstNodeType::VARIABLE_DECLARATOR:
            return node->AsVariableDeclarator()->Id()->AsIdentifier()->Name();
        default:
            ES2PANDA_UNREACHABLE();
    }
}

lexer::SourcePosition AssignAnalyzer::GetVariablePosition(const ir::AstNode *node) const
{
    switch (node->Type()) {
        case ir::AstNodeType::CLASS_PROPERTY:
            return node->AsClassProperty()->Key()->Start();
        case ir::AstNodeType::VARIABLE_DECLARATOR:
        default:
            return node->Start();
    }
}

NodeId AssignAnalyzer::GetNodeId(const ir::AstNode *node) const
{
    auto res = nodeIdMap_.find(node);
    if (res != nodeIdMap_.end()) {
        return res->second;
    }
    return INVALID_ID;
}

bool AssignAnalyzer::Trackable(const ir::AstNode *node) const
{
    switch (node->Type()) {
        case ir::AstNodeType::CLASS_PROPERTY:
        case ir::AstNodeType::VARIABLE_DECLARATOR:
            return true;
        default:
            return false;
    }
}

bool AssignAnalyzer::IsConstUninitializedField(const ir::AstNode *node) const
{
    return node->IsClassProperty() && node->IsConst();
}

bool AssignAnalyzer::IsConstUninitializedStaticField(const ir::AstNode *node) const
{
    return IsConstUninitializedField(node) && node->IsStatic();
}

static ir::AstNode const *OwnerDef(const ir::AstNode *node)
{
    return util::Helpers::GetContainingClassDefinition(node);
}

void AssignAnalyzer::NewVar(const ir::AstNode *node)
{
    if (!Trackable(node)) {
        return;
    }

    if (GetNodeId(node) != INVALID_ID) {
        return;
    }

    auto ownerDef = OwnerDef(node);
    if (ownerDef != classDef_ && ownerDef != globalClass_) {
        return;
    }

    nodeIdMap_[node] = nextAdr_;
    varDecls_.reserve(nextAdr_ + 1);
    varDecls_.insert(varDecls_.begin() + nextAdr_, node);
    inits_.Excl(nextAdr_);
    uninits_.Incl(nextAdr_);
    ++nextAdr_;
}

varbinder::Variable *AssignAnalyzer::GetBoundVariable(const ir::AstNode *node)
{
    varbinder::Variable *ret = nullptr;

    if (node->IsClassProperty()) {
        const ir::Identifier *identifier = node->AsClassProperty()->Id();
        ES2PANDA_ASSERT(identifier != nullptr);
        ret = identifier->Variable();
    } else if (node->IsVariableDeclarator()) {
        ret = node->AsVariableDeclarator()->Id()->AsIdentifier()->Variable();
    } else {
        ES2PANDA_UNREACHABLE();
    }

    return ret;
}

static const ir::AstNode *CheckInterfaceProp(const ark::es2panda::ir::AstNode *const node,
                                             const ir::ClassDefinition *classDef)
{
    const util::StringView targetName = node->IsMethodDefinition()
                                            ? node->AsMethodDefinition()->Key()->AsIdentifier()->Name()
                                            : node->AsClassProperty()->Key()->AsIdentifier()->Name();
    for (const auto it : classDef->Body()) {
        // Check if there is corresponding class property in the same class.
        if (it->IsClassProperty() && !it->IsStatic()) {
            const auto *prop = it->AsClassProperty();
            auto *propIdentifier = prop->Key()->AsIdentifier();
            if (propIdentifier->Name().Is(std::string(targetName.Utf8()))) {
                // Use property node as declNode to ensure obtaining NodeId and add it to inits.
                return prop;
            }
        }
    }
    return nullptr;
}

const ir::AstNode *AssignAnalyzer::GetDeclaringNode(const ir::AstNode *node)
{
    if (node->IsClassProperty() || node->IsVariableDeclarator()) {
        return node;
    }

    const ir::AstNode *ret = nullptr;

    if (node->IsMemberExpression()) {
        const ir::MemberExpression *membExpr = node->AsMemberExpression();
        if (membExpr->PropVar() != nullptr) {
            if (membExpr->PropVar()->Declaration() != nullptr) {
                ret = membExpr->PropVar()->Declaration()->Node();
            }
        }
    } else if (node->IsIdentifier()) {
        if (auto *const variable = node->Variable(); variable != nullptr) {
            ret = variable->Declaration()->Node();
        }
    }

    if (ret != nullptr) {
        if (ret->IsIdentifier() && ret->Parent()->IsVariableDeclarator() &&
            ret == ret->Parent()->AsVariableDeclarator()->Id()) {
            ret = ret->Parent();
        }
    }

    if (ret != nullptr) {
        // if declNode is a getter/setter method, actual node initialized should be a class proterty node.
        if ((ret->Modifiers() & ir::ModifierFlags::GETTER_SETTER) != 0U) {
            if (const auto *interfaceProp = CheckInterfaceProp(ret, classDef_); interfaceProp != nullptr) {
                ret = interfaceProp;
            }
        }
    }

    return ret;
}

static bool IsDefaultValueType(const Type *type, bool isNonReadonlyField)
{
    if (type == nullptr) {
        return false;
    }

    ES2PANDA_ASSERT(!type->IsETSPrimitiveType());

    bool boxedPrimitive = (type->IsETSObjectType() && type->AsETSObjectType()->IsBoxedPrimitive());
    bool nullOrUndefined = type->IsETSUndefinedType() || type->IsETSNullType();
    if (boxedPrimitive || nullOrUndefined) {
        return true;
    }

    if (type->PossiblyETSUndefined()) {
        if (!type->HasTypeFlag(checker::TypeFlag::GENERIC)) {
            return true;
        }
        if (!CHECK_GENERIC_NON_READONLY_PROPERTIES && isNonReadonlyField) {
            return true;
        }
    }

    return false;
}

bool AssignAnalyzer::VariableHasDefaultValue(const ir::AstNode *node)
{
    ES2PANDA_ASSERT(node != nullptr);

    const checker::Type *type = nullptr;
    bool isNonReadonlyField = false;

    if (node->IsClassProperty()) {
        type = node->AsClassProperty()->TsType();
        isNonReadonlyField = !node->IsReadonly();  // NOTE(pantos) readonly is true, const is not set?
    } else if (node->IsVariableDeclarator()) {
        varbinder::Variable *variable = GetBoundVariable(node);
        ES2PANDA_ASSERT(variable != nullptr);
        type = variable->TsType();
    } else {
        ES2PANDA_UNREACHABLE();
    }
    return IsDefaultValueType(type, isNonReadonlyField);
}

void AssignAnalyzer::LetInit(const ir::AstNode *node)
{
    const ir::AstNode *declNode = GetDeclaringNode(node);

    if (declNode == nullptr || declNode->IsDeclare()) {
        return;
    }

    NodeId adr = GetNodeId(declNode);
    if (adr == INVALID_ID) {
        return;
    }

    auto ownerDef = OwnerDef(declNode);
    if (ownerDef != classDef_) {
        CheckInheritedReadonlyAssignment(node, declNode);
        return;
    }

    if (node != declNode && declNode->IsConst()) {
        // check reassignment of readonly properties
        util::StringView type = GetVariableType(declNode);
        util::StringView name = GetVariableName(declNode);
        const lexer::SourcePosition pos = GetVariablePosition(node);

        auto uninit = [this](NodeId a) {
            uninits_.Excl(a);
            if (!inits_.IsMember(a)) {
                uninitsTry_.Excl(a);
            }
        };

        if (classDef_ == globalClass_ || (adr < classFirstAdr_ || adr >= firstAdr_)) {
            if (declNode->IsClassProperty() && classDef_ != declNode->Parent()) {
                Warning(diagnostic::ASSIGN_TO_READONLY, {name}, pos);
            } else if (!uninits_.IsMember(adr)) {
                Warning(diagnostic::MAYBE_REASSIGNED, {Capitalize(type).c_str(), name}, pos);
            } else {
                uninit(adr);
            }
        }
    }

    inits_.Incl(adr);
}

bool AssignAnalyzer::CheckStaticFieldInit(const ir::AstNode *node, const ir::AstNode *declNode, NodeId adr)
{
    if (node == nullptr || declNode == nullptr) {
        return true;
    }

    if (!inStaticFieldInit_) {
        return true;
    }

    if (!node->IsMemberExpression()) {
        return true;
    }

    if (declNode->Parent() != classDef_) {
        return true;
    }

    if (inits_.IsMember(adr)) {
        return true;
    }

    util::StringView type = GetVariableType(declNode);
    util::StringView name = GetVariableName(declNode);
    const lexer::SourcePosition pos = GetVariablePosition(node);

    checker_->LogError(diagnostic::USE_BEFORE_INIT, {Capitalize(type), name}, pos);

    return false;
}

bool AssignAnalyzer::CheckClassProperty(const ir::AstNode *node, const ir::AstNode *declNode)
{
    if (declNode->IsClassProperty()) {
        if (!CHECK_ALL_PROPERTIES && !declNode->IsConst()) {
            // non readonly property
            return false;
        }

        if (node->IsDefinite() || node->IsOverride()) {
            return false;
        }

        if (declNode->AsClassProperty()->IsImmediateInit()) {
            if (!declNode->AsClassProperty()->IsTopLevelLexicalDecl()) {
                return false;
            }
        }
    }
    return true;
}

void AssignAnalyzer::CheckInheritedReadonlyAssignment(const ir::AstNode *node, const ir::AstNode *declNode)
{
    if (node == declNode) {
        return;
    }

    if (!declNode->IsReadonly()) {
        return;
    }

    if (!declNode->IsClassProperty()) {
        return;
    }

    if (classDef_ == declNode->Parent()) {
        return;
    }

    NodeId adr = GetNodeId(declNode);
    util::StringView name = GetVariableName(declNode);
    const lexer::SourcePosition pos = GetVariablePosition(node);

    ++numErrors_;
    if (inits_.IsMember(adr)) {
        checker_->LogError(diagnostic::FIELD_REASSIGNMENT, {"readonly", name}, pos);
    } else {
        checker_->LogError(diagnostic::FIELD_ASSIGN_TO_READONLY, {name}, pos);
    }
}

bool AssignAnalyzer::IsTopLevelDeclInitAssignment(const ir::AssignmentExpression *assignExpr,
                                                  const ir::AstNode *declNode) const
{
    if (assignExpr->OperatorType() != lexer::TokenType::PUNCTUATOR_SUBSTITUTION || declNode == nullptr ||
        !declNode->IsClassProperty()) {
        return false;
    }

    const auto *declProp = declNode->AsClassProperty();
    if (!declProp->IsTopLevelLexicalDecl()) {
        return false;
    }

    const auto *exprStmt = assignExpr->Parent();
    if (exprStmt == nullptr || !exprStmt->IsExpressionStatement()) {
        return false;
    }

    return exprStmt->Start().Program() == declNode->Start().Program() &&
           exprStmt->Start().index == declNode->Start().index &&
           exprStmt->End().Program() == declNode->End().Program() && exprStmt->End().index == declNode->End().index;
}

bool AssignAnalyzer::ReportTopLevelDeclInitViolationIfNeeded(const ir::AstNode *node, const ir::AstNode *declNode,
                                                             const ir::AstNode *currentTopLevelDecl, NodeId adr)
{
    if (currentTopLevelDecl == nullptr || !declNode->IsClassProperty() || !currentTopLevelDecl->IsClassProperty()) {
        return false;
    }

    const auto *declProp = declNode->AsClassProperty();
    const auto *currentProp = currentTopLevelDecl->AsClassProperty();
    if (!declProp->IsTopLevelLexicalDecl() || !currentProp->IsTopLevelLexicalDecl()) {
        return false;
    }

    if (declNode->Start().Program() != currentTopLevelDecl->Start().Program()) {
        return false;
    }

    if (!inits_.IsMember(adr)) {
        ReportUseBeforeInit(node, declNode);
        return true;
    }

    return false;
}

bool AssignAnalyzer::ShouldSkipRegularInitCheck(const ir::AstNode *node, const ir::AstNode *declNode, NodeId adr,
                                                const ir::AstNode *currentTopLevelDecl)
{
    if (!CheckStaticFieldInit(node, declNode, adr)) {
        return true;
    }

    if (VariableHasDefaultValue(declNode) && CanUseDefaultValueAtCurrentPoint(declNode, node, currentTopLevelDecl)) {
        return true;
    }

    if (ReportTopLevelDeclInitViolationIfNeeded(node, declNode, currentTopLevelDecl, adr)) {
        return true;
    }

    if (OwnerDef(declNode) != classDef_) {
        return true;
    }

    return !CheckClassProperty(node, declNode);
}

bool AssignAnalyzer::ShouldReportRegularUseBeforeInit(NodeId adr) const
{
    const bool inCurrentInitRange = adr < classFirstAdr_ || adr >= firstAdr_;
    const bool inGlobalClass = classDef_ == globalClass_;
    return (inGlobalClass || inCurrentInitRange) && !inits_.IsMember(adr);
}

void AssignAnalyzer::ReportUseBeforeInit(const ir::AstNode *node, const ir::AstNode *declNode)
{
    util::StringView type = GetVariableType(declNode);
    util::StringView name = GetVariableName(declNode);
    const lexer::SourcePosition pos = GetVariablePosition(node);

    if (node->IsClassProperty()) {
        checker_->LogError(diagnostic::PROPERTY_MAYBE_MISSING_INIT, {name}, pos);
    } else {
        checker_->LogError(diagnostic::USE_BEFORE_INIT, {Capitalize(type), name}, pos);
    }
}

void AssignAnalyzer::CheckInit(const ir::AstNode *node, const ir::AstNode *currentTopLevelDecl)
{
    const ir::AstNode *declNode = GetDeclaringNode(node);
    if (declNode == nullptr || declNode->IsDeclare()) {
        return;
    }

    NodeId adr = GetNodeId(declNode);
    if (adr == INVALID_ID) {
        return;
    }

    if (ShouldSkipRegularInitCheck(node, declNode, adr, currentTopLevelDecl)) {
        return;
    }

    if (!ShouldReportRegularUseBeforeInit(adr)) {
        return;
    }

    if (WARN_NO_INIT_ONCE_PER_VARIABLE && !foundErrors_.insert(declNode).second) {
        return;
    }

    ReportUseBeforeInit(node, declNode);
}

void AssignAnalyzer::Split(const bool setToNull)
{
    initsWhenFalse_ = inits_;
    uninitsWhenFalse_ = uninits_;
    initsWhenTrue_ = inits_;
    uninitsWhenTrue_ = uninits_;
    if (setToNull) {
        inits_.Reset();
        uninits_.Reset();
    }
}

void AssignAnalyzer::Merge()
{
    inits_ = initsWhenFalse_.AndSet(initsWhenTrue_);
    uninits_ = uninitsWhenFalse_.AndSet(uninitsWhenTrue_);
}

void AssignAnalyzer::CheckPendingExits()
{
    for (const auto &it : PendingExits()) {
        if (!it.Node()->IsReturnStatement()) {
            continue;
        }
        if (isInitialConstructor_) {
            inits_ = it.exitInits_;

            for (int i = firstAdr_; i < nextAdr_; i++) {
                CheckInit(varDecls_[i]);
            }
        }
    }
    ClearPendingExits();
}

void AssignAnalyzer::MarkDead()
{
    if (!isInitialConstructor_) {
        inits_.InclRange(returnAdr_, nextAdr_);
    } else {
        for (int address = returnAdr_; address < nextAdr_; address++) {
            if (!IsConstUninitializedStaticField(varDecls_[address])) {
                inits_.Incl(address);
            }
        }
    }
    uninits_.InclRange(returnAdr_, nextAdr_);
}

}  // namespace ark::es2panda::checker
