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

#include "arrayLiteralLowering.h"
#include <sstream>
#include <utility>
#include <vector>

#include "compiler/lowering/util.h"
#include "ir/astNode.h"
#include "ir/expressions/literals/numberLiteral.h"
#include "lexer/token/number.h"
#include "util/es2pandaMacros.h"
#include "checker/ETSchecker.h"
#include "libarkbase/utils/arena_containers.h"

namespace ark::es2panda::compiler {

using AstNodePtr = ir::AstNode *;

std::string_view ArrayLiteralLowering::Name() const
{
    return "ArrayLiteralLowering";
}

ArenaVector<ir::Statement *> ArrayLiteralLowering::GenerateDefaultCallToConstructor(ir::Identifier *arraySymbol,
                                                                                    checker::Type *eleType)
{
    std::stringstream ss;
    std::vector<ir::AstNode *> newStmts;
    if (!eleType->IsETSUnionType() && !eleType->IsETSAnyType()) {
        auto *indexSymbol = Gensym(Allocator());
        auto *lengthSymbol = Gensym(Allocator());
        auto *typeNode = checker_->AllocNode<ir::OpaqueTypeNode>(eleType, Allocator());
        ES2PANDA_ASSERT(typeNode != nullptr);

        ss << "let @@I1 : int = @@I2.length.toInt();";
        newStmts.emplace_back(lengthSymbol);
        newStmts.emplace_back(arraySymbol->Clone(Allocator(), nullptr));
        ss << "for (let @@I3 = 0; @@I4 < @@E5;  @@I6 = @@I7 + 1) {";
        newStmts.emplace_back(indexSymbol);
        newStmts.emplace_back(indexSymbol->Clone(Allocator(), nullptr));
        newStmts.emplace_back(lengthSymbol->Clone(Allocator(), nullptr));
        newStmts.emplace_back(indexSymbol->Clone(Allocator(), nullptr));
        newStmts.emplace_back(indexSymbol->Clone(Allocator(), nullptr));
        ss << "@@I8[@@I9] = new @@T10() }";
        newStmts.emplace_back(arraySymbol->Clone(Allocator(), nullptr));
        newStmts.emplace_back(indexSymbol->Clone(Allocator(), nullptr));
        newStmts.emplace_back(typeNode->Clone(Allocator(), nullptr));
    } else {
        ArenaVector<ir::Statement *> emptyStatement(Allocator()->Adapter());
        return emptyStatement;
    }

    return parser_->CreateFormattedStatements(ss.str(), newStmts);
}

static bool IsInAnnotationContext(ir::AstNode *node)
{
    while (node != nullptr && !(node->IsClassDefinition() && node->AsClassDefinition()->IsGlobal())) {
        if (node->IsAnnotationDeclaration() || node->IsAnnotationUsage()) {
            return true;
        }
        node = node->Parent();
    }
    return false;
}

ir::AstNode *ArrayLiteralLowering::TryTransformLiteralArrayToRefArray(ir::ArrayExpression *literalArray)
{
    auto literalArrayType = literalArray->TsType() != nullptr ? literalArray->TsType() : literalArray->PreferredType();
    if (literalArrayType->IsETSArrayType() || literalArrayType->IsETSTupleType() ||
        !literalArrayType->IsETSResizableArrayType() || IsInAnnotationContext(literalArray)) {
        return literalArray;
    }
    auto *arrayType = literalArrayType->AsETSResizableArrayType()->ElementType();
    std::vector<ir::AstNode *> newStmts;
    std::stringstream ss;
    auto *genSymIdent = Gensym(Allocator());
    auto *genSymIdent2 = Gensym(Allocator());
    auto *type = checker_->AllocNode<ir::OpaqueTypeNode>(arrayType, Allocator());
    // ss << "let @@I1 : FixedArray<@@T2> = @@E3;";
    // ss << "Array.from<@@T4>(@@I5);";
    ss << "let @@I1 : FixedArray<@@T2> = @@E3;";
    ss << "let @@I4 : Array<@@T5> = new Array<@@T6>(@@I7.length);";
    ss << "for (let i = 0; i < @@I8.length; ++i) { @@I9[i] = @@I10[i]}";
    ss << "@@I11;";
    newStmts.emplace_back(genSymIdent);
    newStmts.emplace_back(type);
    newStmts.emplace_back(literalArray);
    literalArray->SetTsType(nullptr);
    newStmts.emplace_back(genSymIdent2);
    newStmts.emplace_back(type->Clone(Allocator(), nullptr));
    newStmts.emplace_back(type->Clone(Allocator(), nullptr));
    newStmts.emplace_back(genSymIdent->Clone(Allocator(), nullptr));
    newStmts.emplace_back(genSymIdent->Clone(Allocator(), nullptr));
    newStmts.emplace_back(genSymIdent2->Clone(Allocator(), nullptr));
    newStmts.emplace_back(genSymIdent->Clone(Allocator(), nullptr));
    newStmts.emplace_back(genSymIdent2->Clone(Allocator(), nullptr));

    auto *parent = literalArray->Parent();
    auto *loweringResult = parser_->CreateFormattedExpression(ss.str(), newStmts);
    ES2PANDA_ASSERT(loweringResult != nullptr);
    loweringResult->SetRange(literalArray->Range());
    loweringResult->SetParent(parent);

    auto *scope = NearestScope(parent);
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder_, scope);
    CheckLoweredNode(varbinder_, checker_, loweringResult);
    return loweringResult;
}

ir::AstNode *ArrayLiteralLowering::TryTransformNewArrayExprToRefArray(ir::ETSNewArrayInstanceExpression *newExpr)
{
    if (newExpr->TsType()->IsETSArrayType()) {
        return newExpr;
    }
    ES2PANDA_ASSERT(newExpr->TsType()->IsETSResizableArrayType());

    auto *arrayType = newExpr->TsType()->AsETSResizableArrayType()->ElementType();
    std::vector<ir::AstNode *> newStmts;
    auto *genSymIdent = Gensym(Allocator());

    std::stringstream ss;
    ss << "let @@I1 = new Array<@@T2>(@@E3.toDouble());";
    auto *type = checker_->AllocNode<ir::OpaqueTypeNode>(arrayType, Allocator());
    auto *dimension = newExpr->Dimension()->Clone(Allocator(), nullptr);
    newStmts.emplace_back(genSymIdent);
    newStmts.emplace_back(type);
    newStmts.emplace_back(dimension);

    ArenaVector<ir::Statement *> statements(Allocator()->Adapter());
    auto *newArrStatement = parser_->CreateFormattedStatement(ss.str(), newStmts);
    statements.emplace_back(newArrStatement);
    auto newArrElementStatements = GenerateDefaultCallToConstructor(genSymIdent, arrayType);
    statements.insert(statements.end(), newArrElementStatements.begin(), newArrElementStatements.end());
    auto returnStmt = parser_->CreateFormattedStatement("@@I1", genSymIdent->Clone(Allocator(), nullptr));
    statements.emplace_back(returnStmt);
    auto *loweringResult = checker_->AllocNode<ir::BlockExpression>(std::move(statements));
    loweringResult->SetRange(newExpr->Range());
    loweringResult->SetParent(newExpr->Parent());
    auto *scope = NearestScope(loweringResult->Parent());
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder_, scope);
    CheckLoweredNode(varbinder_, checker_, loweringResult);
    return loweringResult;
}

ir::Statement *ArrayLiteralLowering::CreateNestedArrayCreationStatement(ArenaVector<ir::Identifier *> &identDims,
                                                                        size_t currentDim, checker::Type *type,
                                                                        ir::Expression *expr)
{
    auto *genSymIdent = Gensym(Allocator());
    auto *arraySymbol = Gensym(Allocator());
    auto *lastDimIdent = identDims[currentDim - 1];
    auto *currentDimIdent = identDims[currentDim];
    auto *arrayType = type->AsETSResizableArrayType()->ElementType();
    auto typeNode = checker_->AllocNode<ir::OpaqueTypeNode>(arrayType, Allocator());
    auto arrayAccessExpr = checker_->AllocNode<ir::MemberExpression>(
        expr->Clone(Allocator(), nullptr)->AsExpression(), genSymIdent->Clone(Allocator(), nullptr),
        ir::MemberExpressionKind::ELEMENT_ACCESS, true, false);

    std::string creationTemplate =
        "for (let @@I1 = 0; @@I2 < @@I3; @@I4 = @@I5 + 1) { let @@I6 = new Array<@@T7>(@@E8.toDouble()); @@E9 = @@I10}";
    ir::Statement *forUpdateStmt = parser_->CreateFormattedStatement(
        creationTemplate, genSymIdent, genSymIdent->Clone(Allocator(), nullptr),
        lastDimIdent->Clone(Allocator(), nullptr), genSymIdent->Clone(Allocator(), nullptr),
        genSymIdent->Clone(Allocator(), nullptr), arraySymbol, typeNode, currentDimIdent->Clone(Allocator(), nullptr),
        arrayAccessExpr, arraySymbol->Clone(Allocator(), nullptr));
    if (identDims.size() > currentDim + 1) {
        auto consequentStmt = CreateNestedArrayCreationStatement(identDims, currentDim + 1, arrayType, arrayAccessExpr);
        forUpdateStmt->AsForUpdateStatement()->Body()->AsBlockStatement()->AddStatement(consequentStmt);
    } else if (identDims.size() == currentDim + 1) {
        // For last dim, initialize the array elements.
        auto newArrElementStatements = GenerateDefaultCallToConstructor(arraySymbol, arrayType);
        forUpdateStmt->AsForUpdateStatement()->Body()->AsBlockStatement()->AddStatements(newArrElementStatements);
    }

    return forUpdateStmt;
}

ArenaVector<ir::Identifier *> ArrayLiteralLowering::TransformDimVectorToIdentVector(
    ArenaVector<ir::Expression *> &dimVector, ArenaVector<ir::Statement *> &stmts)
{
    std::vector<ir::AstNode *> statements;
    ArenaVector<ir::Identifier *> idents(Allocator()->Adapter());
    auto addNode = [&statements](ir::AstNode *node) -> size_t {
        statements.emplace_back(node);
        return statements.size();
    };

    std::stringstream ss;
    for (size_t i = 0; i < dimVector.size(); ++i) {
        idents.emplace_back(Gensym(Allocator()));
        ss << "let @@I" << addNode(idents[i]) << " = @@E" << addNode(dimVector[i]->Clone(Allocator(), nullptr)) << ";";
    }
    auto parsedStatement = parser_->CreateFormattedStatements(ss.str(), statements);
    stmts.insert(stmts.end(), parsedStatement.begin(), parsedStatement.end());
    return idents;
}

ir::AstNode *ArrayLiteralLowering::TryTransformNewMultiDimArrayToRefArray(
    ir::ETSNewMultiDimArrayInstanceExpression *newExpr)
{
    if (newExpr->TsType()->IsETSArrayType()) {
        return newExpr;
    }
    ES2PANDA_ASSERT(newExpr->TsType()->IsETSResizableArrayType());
    ArenaVector<ir::Statement *> statements(Allocator()->Adapter());
    // Create outer forloop
    auto arrayType = newExpr->TsType()->AsETSResizableArrayType()->ElementType();
    auto *type = checker_->AllocNode<ir::OpaqueTypeNode>(arrayType, Allocator());
    auto *genSymIdent = Gensym(Allocator());
    std::string newArray = "let @@I1 = new Array<@@T2>(@@I3.toDouble())";
    auto idents = TransformDimVectorToIdentVector(newExpr->Dimensions(), statements);
    auto newArraystatement =
        parser_->CreateFormattedStatements(newArray, genSymIdent, type, idents[0]->Clone(Allocator(), nullptr));
    auto nestedArrayCreationStmt = CreateNestedArrayCreationStatement(idents, 1, arrayType, genSymIdent);
    auto returnStmt = parser_->CreateFormattedStatement("@@I1", genSymIdent->Clone(Allocator(), nullptr));
    statements.insert(statements.end(), newArraystatement.begin(), newArraystatement.end());
    statements.push_back(nestedArrayCreationStmt);
    statements.push_back(returnStmt);
    auto loweringResult = checker_->AllocNode<ir::BlockExpression>(std::move(statements));
    loweringResult->SetRange(newExpr->Range());
    loweringResult->SetParent(newExpr->Parent());
    auto *scope = NearestScope(loweringResult->Parent());
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder_, scope);
    CheckLoweredNode(varbinder_, checker_, loweringResult);
    return loweringResult;
}

bool ArrayLiteralLowering::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    parser_ = ctx->parser->AsETSParser();
    varbinder_ = ctx->parserProgram->VarBinder()->AsETSBinder();
    checker_ = ctx->GetChecker()->AsETSChecker();
    program->Ast()->TransformChildrenRecursively(
        [this](ir::AstNode *ast) -> AstNodePtr {
            if (ast->IsArrayExpression()) {
                return TryTransformLiteralArrayToRefArray(ast->AsArrayExpression());
            }
            if (ast->IsETSNewArrayInstanceExpression()) {
                return TryTransformNewArrayExprToRefArray(ast->AsETSNewArrayInstanceExpression());
            }
            if (ast->IsETSNewMultiDimArrayInstanceExpression()) {
                return TryTransformNewMultiDimArrayToRefArray(ast->AsETSNewMultiDimArrayInstanceExpression());
            }
            return ast;
        },
        Name());

    return true;
}

ArenaAllocator *ArrayLiteralLowering::Allocator()
{
    return checker_->Allocator();
}

}  // namespace ark::es2panda::compiler
