/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "arrayForOfLowering.h"

#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"
#include "parser/ETSparser.h"

namespace ark::es2panda::compiler {

static constexpr std::size_t const LOOP_STATEMENT_POSITION = 2U;

std::string_view ArrayForOfLowering::Name() const
{
    static std::string const NAME = "ArrayForOfLowering";
    return NAME;
}

static void TransferForOfLoopBody(ir::Statement *const forBody, ir::BlockStatement *const loopBody) noexcept
{
    ES2PANDA_ASSERT(forBody != nullptr && loopBody != nullptr);
    auto &loopStatements = loopBody->StatementsForUpdates();
    loopStatements.push_back(forBody);
    forBody->SetParent(loopBody);
}

static void UpdateTransferredLoopBodyScope(ir::Statement *const forBody, ir::BlockStatement *const loopBody) noexcept
{
    ES2PANDA_ASSERT(forBody != nullptr && loopBody != nullptr);
    if (forBody->IsBlockStatement() && forBody->AsBlockStatement()->Scope() != nullptr) {
        forBody->AsBlockStatement()->Scope()->SetParent(loopBody->Scope());
    }
}

static void RebindTransferredLoopVariableReferences(ir::Statement *const forBody,
                                                    varbinder::Variable *const oldVariable,
                                                    varbinder::Variable *const newVariable) noexcept
{
    ES2PANDA_ASSERT(forBody != nullptr);
    ES2PANDA_ASSERT(oldVariable != nullptr);
    ES2PANDA_ASSERT(newVariable != nullptr);

    forBody->IterateRecursively([oldVariable, newVariable](ir::AstNode *node) {
        if (!node->IsIdentifier()) {
            return;
        }

        auto *ident = node->AsIdentifier();
        if (ident->Variable() == oldVariable) {
            ident->SetVariable(newVariable);
        }
    });
}

static ir::Identifier *GetLoopVariable(ArenaAllocator *allocator, ir::ForOfStatement *forOfStatement)
{
    if (auto *const left = forOfStatement->Left(); left->IsVariableDeclaration()) {
        auto *const declaration = left->AsVariableDeclaration();
        return declaration->Declarators().at(0U)->Id()->AsIdentifier()->Clone(allocator, nullptr);
    }

    if (auto *const left = forOfStatement->Left(); left->IsIdentifier()) {
        auto *loopVariableIdent = Gensym(allocator);
        ES2PANDA_ASSERT(loopVariableIdent != nullptr);
        loopVariableIdent->SetName(left->AsIdentifier()->Name());
        return loopVariableIdent;
    }

    ES2PANDA_UNREACHABLE();
}

static std::string GetDeclarationPrefix(ir::ForOfStatement *forOfStatement)
{
    if (auto *const left = forOfStatement->Left(); left->IsVariableDeclaration()) {
        return left->AsVariableDeclaration()->Kind() != ir::VariableDeclaration::VariableDeclarationKind::CONST
                   ? "let "
                   : "const ";
    }
    return "";
}

static bool IsArrayOrStringType(checker::Type *type)
{
    return type != nullptr && (type->IsETSArrayType() || type->IsETSStringType());
}

static bool IsArrayOrStringUnion(checker::Type *type)
{
    return type != nullptr && type->IsETSUnionType() &&
           type->AsETSUnionType()->AllOfConstituentTypes(
               [](checker::Type *const ct) { return IsArrayOrStringType(ct); });
}

static ir::Statement *CreateLoopVariableAssignment(
    ArenaAllocator *allocator, parser::ETSParser *parser,
    std::tuple<ir::Identifier *, ir::Identifier *, ir::Identifier *> const &idents)
{
    auto [iterableIdent, indexIdent, loopVariableIdent] = idents;
    return parser->CreateFormattedStatement("@@I1 = @@I2[@@I3];", loopVariableIdent->Clone(allocator, nullptr),
                                            iterableIdent->Clone(allocator, nullptr),
                                            indexIdent->Clone(allocator, nullptr));
}

static ir::Statement *CreateSimpleLoopVariableLoad(
    ArenaAllocator *allocator, parser::ETSParser *parser, ir::ForOfStatement *forOfStatement,
    std::tuple<ir::Identifier *, ir::Identifier *, ir::Identifier *> const &idents, checker::Type *loopVariableType)
{
    auto [iterableIdent, indexIdent, loopVariableIdent] = idents;
    if (!forOfStatement->Left()->IsVariableDeclaration()) {
        return CreateLoopVariableAssignment(allocator, parser, idents);
    }

    return parser->CreateFormattedStatement(
        GetDeclarationPrefix(forOfStatement) + "@@I1: @@T2 = @@I3[@@I4];", loopVariableIdent, loopVariableType,
        iterableIdent->Clone(allocator, nullptr), indexIdent->Clone(allocator, nullptr));
}

static ir::IfStatement *CreateLoopVariableTypeGuard(
    ArenaAllocator *allocator, parser::ETSParser *parser,
    std::tuple<ir::Identifier *, ir::Identifier *, ir::Identifier *> const &idents, checker::Type *constituentType)
{
    auto [iterableIdent, indexIdent, loopVariableIdent] = idents;
    auto *test = parser->CreateFormattedExpression("@@I1 instanceof @@T2", iterableIdent->Clone(allocator, nullptr),
                                                   constituentType);
    auto *consequent = CreateLoopVariableAssignment(allocator, parser, idents);
    auto *ifStatement = allocator->New<ir::IfStatement>(test, consequent, nullptr);
    test->SetParent(ifStatement);
    consequent->SetParent(ifStatement);
    return ifStatement;
}

static ir::Statement *CreateUnionLoopVariableLoad(
    ArenaAllocator *allocator, parser::ETSParser *parser, ir::ForOfStatement *forOfStatement,
    std::tuple<ir::Identifier *, ir::Identifier *, ir::Identifier *> const &idents, checker::Type *loopVariableType)
{
    auto [iterableIdent, indexIdent, loopVariableIdent] = idents;
    ArenaVector<ir::Statement *> statements(allocator->Adapter());
    if (forOfStatement->Left()->IsVariableDeclaration()) {
        statements.push_back(parser->CreateFormattedStatement(GetDeclarationPrefix(forOfStatement) + "@@I1: @@T2;",
                                                              loopVariableIdent->Clone(allocator, nullptr),
                                                              loopVariableType));
    }

    auto const &constituentTypes = forOfStatement->Right()->TsType()->AsETSUnionType()->ConstituentTypes();
    ES2PANDA_ASSERT(!constituentTypes.empty());

    ir::IfStatement *ifRoot = nullptr;
    ir::IfStatement *ifCurrent = nullptr;
    for (std::size_t i = 0; i + 1U < constituentTypes.size(); ++i) {
        auto *ifStatement = CreateLoopVariableTypeGuard(allocator, parser, idents, constituentTypes[i]);
        if (ifRoot == nullptr) {
            ifRoot = ifStatement;
        } else {
            ifCurrent->SetAlternate(ifStatement);
        }
        ifCurrent = ifStatement;
    }

    auto *finalBranch = CreateLoopVariableAssignment(allocator, parser, idents);
    if (ifRoot == nullptr) {
        statements.push_back(finalBranch);
    } else {
        ifCurrent->SetAlternate(finalBranch);
        statements.push_back(ifRoot);
    }

    auto *block = allocator->New<ir::BlockStatement>(allocator, std::move(statements));
    for (auto *st : block->Statements()) {
        st->SetParent(block);
    }
    return block;
}

static ir::Statement *GenerateLoopVariableLoad(
    ArenaAllocator *allocator, parser::ETSParser *parser, ir::ForOfStatement *forOfStatement,
    std::tuple<ir::Identifier *, ir::Identifier *, ir::Identifier *> const &idents, checker::Type *loopVariableType)
{
    auto *exprType = forOfStatement->Right()->TsType();
    ES2PANDA_ASSERT(exprType != nullptr);
    if (!exprType->IsETSUnionType()) {
        return CreateSimpleLoopVariableLoad(allocator, parser, forOfStatement, idents, loopVariableType);
    }

    return CreateUnionLoopVariableLoad(allocator, parser, forOfStatement, idents, loopVariableType);
}

static ir::Statement *GenerateLoweredStatement(public_lib::Context *context, ir::ForOfStatement *forOfStatement)
{
    auto *const parser = context->parser->AsETSParser();
    ES2PANDA_ASSERT(parser != nullptr);
    auto *const allocator = context->Allocator();
    auto *indexIdent = Gensym(allocator);
    auto *iterableIdent = Gensym(allocator);
    auto *lengthIdent = Gensym(allocator);
    auto *loopVariableIdent = GetLoopVariable(allocator, forOfStatement)->Clone(allocator, nullptr)->AsIdentifier();
    loopVariableIdent->SetVariable(nullptr);
    loopVariableIdent->SetTsType(nullptr);
    loopVariableIdent->SetTypeAnnotation(nullptr);
    checker::Type *loopVariableType = nullptr;
    auto *const loopLeft = forOfStatement->Left();
    if (loopLeft->IsVariableDeclaration()) {
        loopVariableType = loopLeft->AsVariableDeclaration()->Declarators()[0]->Id()->Variable()->TsType();
    } else if (loopLeft->IsIdentifier()) {
        loopVariableType = loopLeft->AsIdentifier()->Variable()->TsType();
    }
    ES2PANDA_ASSERT(loopVariableType != nullptr);
    auto *load = GenerateLoopVariableLoad(allocator, parser, forOfStatement,
                                          {iterableIdent, indexIdent, loopVariableIdent}, loopVariableType);

    auto *lowered = parser->CreateFormattedStatement(
        "let @@I1 = @@E2; let @@I3: int = @@I4.length; for (let @@I5: int = 0; @@I6 < @@I7; @@I8 = @@I9 + 1) { }",
        iterableIdent->Clone(allocator, nullptr), forOfStatement->Right(), lengthIdent->Clone(allocator, nullptr),
        iterableIdent->Clone(allocator, nullptr), indexIdent->Clone(allocator, nullptr),
        indexIdent->Clone(allocator, nullptr), lengthIdent->Clone(allocator, nullptr),
        indexIdent->Clone(allocator, nullptr), indexIdent->Clone(allocator, nullptr));
    auto *loopBody = lowered->AsBlockStatement()
                         ->Statements()[LOOP_STATEMENT_POSITION]
                         ->AsForUpdateStatement()
                         ->Body()
                         ->AsBlockStatement();
    if (load->IsBlockStatement()) {
        for (auto *statement : load->AsBlockStatement()->Statements()) {
            loopBody->AddStatement(statement);
        }
    } else {
        loopBody->AddStatement(load);
    }

    return lowered;
}

static ir::Statement *ProcessArrayForOf(public_lib::Context *context, ir::ForOfStatement *forOfStatement)
{
    auto *const checker = context->GetChecker()->AsETSChecker();

    auto *const varbinder = context->GetChecker()->VarBinder()->AsETSBinder();
    ES2PANDA_ASSERT(varbinder != nullptr);
    auto statementScope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, NearestScope(forOfStatement));

    varbinder::Variable *oldLoopVariable = nullptr;
    auto *const loopLeft = forOfStatement->Left();
    if (loopLeft->IsVariableDeclaration()) {
        oldLoopVariable = loopLeft->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier()->Variable();
    } else if (loopLeft->IsIdentifier()) {
        oldLoopVariable = loopLeft->AsIdentifier()->Variable();
    }

    ir::Statement *loweringResult = GenerateLoweredStatement(context, forOfStatement);

    ES2PANDA_ASSERT(loweringResult != nullptr);
    loweringResult->SetParent(forOfStatement->Parent());
    loweringResult->SetRange(forOfStatement->Range());
    RefineSourceRanges(loweringResult);

    auto loweredLoop =
        loweringResult->AsBlockStatement()->Statements()[LOOP_STATEMENT_POSITION]->AsForUpdateStatement();
    auto loopBody = loweredLoop->Body()->AsBlockStatement();
    TransferForOfLoopBody(forOfStatement->Body(), loopBody);

    BindLoweredNode(varbinder, loweringResult);

    ES2PANDA_ASSERT(oldLoopVariable != nullptr);
    UpdateTransferredLoopBodyScope(forOfStatement->Body(), loopBody);
    if (loopLeft->IsVariableDeclaration()) {
        auto *newLoopVariable = loopBody->Statements()[0]->AsVariableDeclaration()->Declarators()[0]->Id()->Variable();
        ES2PANDA_ASSERT(newLoopVariable != nullptr);
        RebindTransferredLoopVariableReferences(forOfStatement->Body(), oldLoopVariable, newLoopVariable);
    }

    checker::SavedCheckerContext savedCheckerContext(checker, checker::CheckerStatus::NO_OPTS);
    checker::ScopeContext scopeContext(checker, NearestScope(loweringResult));
    loweringResult->Check(checker);

    if (loweringResult->Parent()->IsLabelledStatement()) {
        loweringResult->Parent()->AsLabelledStatement()->Ident()->Variable()->GetScope()->BindNode(loweringResult);
    }
    return loweringResult;
}

bool ArrayForOfLowering::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        // clang-format off
        [ctx = Context()](ir::AstNode *ast) -> ir::AstNode* {
            // clang-format on
            if (ast->IsForOfStatement()) {
                if (auto *const exprType = ast->AsForOfStatement()->Right()->TsType();
                    exprType != nullptr && (IsArrayOrStringType(exprType) || IsArrayOrStringUnion(exprType))) {
                    return ProcessArrayForOf(ctx, ast->AsForOfStatement());
                }
            }
            return ast;
        },
        Name());

    return true;
}

bool ArrayForOfLowering::PostconditionForProgram(const parser::Program *program)
{
    return !program->Ast()->IsAnyChild([](const ir::AstNode *ast) -> bool { return ast->IsForOfStatement(); });
}

}  // namespace ark::es2panda::compiler
