/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "spreadLowering.h"
#include <algorithm>
#include <sstream>

#include "checker/ETSchecker.h"
#include "checker/types/ets/etsTupleType.h"
#include "compiler/lowering/ets/iterableSpreadLowering.h"
#include "compiler/lowering/util.h"
#include "ir/base/spreadElement.h"
#include "ir/opaqueTypeNode.h"

namespace ark::es2panda::compiler {

using AstNodePtr = ir::AstNode *;

static ir::OpaqueTypeNode *CreateOpaqueTypeNode(public_lib::Context *ctx, checker::Type *type)
{
    return ctx->AllocNode<ir::OpaqueTypeNode>(type, ctx->allocator);
}

static void RemoveTrailingSeparator(std::string &text)
{
    if (text.empty()) {
        return;
    }

    text.pop_back();
    text.pop_back();
}

static bool HasMaterializedSpreadElement(ir::ArrayExpression *array)
{
    return std::any_of(array->Elements().begin(), array->Elements().end(), [](ir::Expression *element) {
        return element->IsSpreadElement() &&
               element->AsSpreadElement()->GetResolvedSpreadKind() == ir::SpreadElement::ResolvedSpreadKind::ITERABLE;
    });
}

static void SetPossibleTupleType(public_lib::Context *ctx, ir::Identifier *arrIdent, ir::SpreadElement *spreadElement)
{
    if (spreadElement->GetResolvedSpreadKind() == ir::SpreadElement::ResolvedSpreadKind::TUPLE) {
        auto *const sourceType = spreadElement->GetResolvedSpreadSourceType() != nullptr
                                     ? spreadElement->GetResolvedSpreadSourceType()
                                     : spreadElement->TsType();
        auto *const spreadType = ctx->GetChecker()->AsETSChecker()->NormalizeSpreadType(sourceType);
        arrIdent->SetTsType(spreadType);
    }
}

static void CreateSpreadArrayDeclareStatements(public_lib::Context *ctx, ir::ArrayExpression *array,
                                               std::vector<ir::Identifier *> &spreadArrayIds,
                                               ArenaVector<ir::Statement *> &statements)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    for (auto *element : array->Elements()) {
        if (!element->IsSpreadElement()) {
            continue;
        }
        ir::Identifier *const arrIdent = Gensym(allocator);
        auto *const spreadElement = element->AsSpreadElement();
        auto *const initExpr = CloneSpreadArgumentWithSmartType(ctx, spreadElement);
        SetPossibleTupleType(ctx, arrIdent, spreadElement);
        spreadArrayIds.emplace_back(arrIdent);
        statements.emplace_back(parser->CreateFormattedStatement("let @@I1 = (@@E2);", arrIdent, initExpr));
    }
}

static ir::Identifier *CreateNewArrayLengthStatement(public_lib::Context *ctx, ir::ArrayExpression *array,
                                                     std::vector<ir::Identifier *> &spreadArrayIds,
                                                     ArenaVector<ir::Statement *> &statements)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    ir::Identifier *newArrayLengthId = Gensym(allocator);
    ES2PANDA_ASSERT(newArrayLengthId != nullptr);
    std::vector<ir::AstNode *> nodesWaitingInsert {newArrayLengthId->CloneReference(allocator, nullptr)};
    size_t argumentCount = 1;
    std::stringstream lengthString;
    const size_t normalElementCount = array->Elements().size() - spreadArrayIds.size();
    lengthString << "let @@I" << (argumentCount++) << " : int = " << normalElementCount << " + ";
    for (auto *const spaId : spreadArrayIds) {
        if (spaId->TsType() != nullptr && spaId->TsType()->IsETSTupleType()) {
            lengthString << "(" << spaId->TsType()->AsETSTupleType()->GetTupleSize() << ") + ";
        } else {
            lengthString << "(@@I" << (argumentCount++) << ".length.toInt()) + ";
            nodesWaitingInsert.emplace_back(spaId->CloneReference(allocator, nullptr));
        }
    }
    lengthString << "0;";

    ir::Statement *newArrayLengthStatement = parser->CreateFormattedStatement(lengthString.str(), nodesWaitingInsert);
    statements.emplace_back(newArrayLengthStatement);
    return newArrayLengthId;
}

static ir::Identifier *CreatePreSizedNewArrayDeclareStatement(public_lib::Context *ctx, ir::ArrayExpression *array,
                                                              ArenaVector<ir::Statement *> &statements,
                                                              ir::Identifier *newArrayLengthId)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    ir::Identifier *newArrayId = Gensym(allocator);
    ES2PANDA_ASSERT(newArrayId != nullptr);

    std::stringstream newArrayDeclareStr;
    std::vector<ir::AstNode *> newStmts;
    if (array->TsType()->IsETSResizableArrayType()) {
        newArrayDeclareStr << "let @@I1: @@T2 = @@E3;" << std::endl;
        newStmts.emplace_back(newArrayId->CloneReference(allocator, nullptr));
        newStmts.emplace_back(CreateOpaqueTypeNode(ctx, array->TsType()));
        newStmts.emplace_back(CreateUninitializedResizableArray(ctx, newArrayLengthId, array->TsType()));
    } else {
        newArrayDeclareStr << "let @@I1 = @@E2;" << std::endl;
        newStmts.emplace_back(newArrayId->CloneReference(allocator, nullptr));
        newStmts.emplace_back(CreateUninitializedFixedArray(ctx, newArrayLengthId, array->TsType()));
    }

    ES2PANDA_ASSERT(newArrayLengthId != nullptr);
    ir::Statement *newArrayDeclareSt = parser->CreateFormattedStatement(newArrayDeclareStr.str(), newStmts);
    statements.emplace_back(newArrayDeclareSt);
    return newArrayId;
}

static void AppendTupleResultElement(public_lib::Context *ctx, ir::Expression *element,
                                     ArenaVector<ir::Statement *> &statements,
                                     std::vector<ir::AstNode *> &tupleElementNodes, std::string &tupleInitList)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    auto *const elementIdent = Gensym(allocator);
    elementIdent->SetTsType(element->TsType());
    statements.emplace_back(
        parser->CreateFormattedStatement("let @@I1 = @@E2;", elementIdent, element->Clone(allocator, nullptr)));
    tupleInitList += "@@I" + std::to_string(tupleElementNodes.size() + 1) + ", ";
    tupleElementNodes.emplace_back(elementIdent->CloneReference(allocator, nullptr));
}

static void AppendTupleResultSpread(public_lib::Context *ctx, ir::Expression *spreadArgument,
                                    ArenaVector<ir::Statement *> &statements,
                                    std::vector<ir::AstNode *> &tupleElementNodes, std::string &tupleInitList)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    auto *const tupleType =
        ctx->GetChecker()->AsETSChecker()->NormalizeSpreadType(spreadArgument->TsType())->AsETSTupleType();
    auto *const tupleIdent = Gensym(allocator);
    tupleIdent->SetTsType(tupleType);
    statements.emplace_back(
        parser->CreateFormattedStatement("let @@I1 = @@E2;", tupleIdent, spreadArgument->Clone(allocator, nullptr)));

    for (std::size_t idx = 0; idx < tupleType->GetTupleSize(); idx++) {
        tupleInitList += "@@I" + std::to_string(tupleElementNodes.size() + 1) + "[" + std::to_string(idx) + "], ";
        tupleElementNodes.emplace_back(tupleIdent->CloneReference(allocator, nullptr));
    }
}

static ir::Expression *GenerateTupleInitExpr(public_lib::Context *ctx, ir::ArrayExpression *array,
                                             ArenaVector<ir::Statement *> &statements)
{
    auto *const parser = ctx->parser->AsETSParser();

    std::vector<ir::AstNode *> tupleElementNodes {};
    std::string tupleInitList {};

    for (auto *element : array->Elements()) {
        if (!element->IsSpreadElement()) {
            AppendTupleResultElement(ctx, element, statements, tupleElementNodes, tupleInitList);
            continue;
        }

        auto *const spreadArgument = element->AsSpreadElement()->Argument();
        ES2PANDA_ASSERT(
            ctx->GetChecker()->AsETSChecker()->NormalizeSpreadType(spreadArgument->TsType())->IsETSTupleType());
        AppendTupleResultSpread(ctx, spreadArgument, statements, tupleElementNodes, tupleInitList);
    }

    RemoveTrailingSeparator(tupleInitList);
    std::stringstream newTupleExpr;
    newTupleExpr << "[";
    newTupleExpr << tupleInitList;
    newTupleExpr << "];";

    return parser->CreateFormattedExpression(newTupleExpr.str(), tupleElementNodes);
}

static ir::Identifier *CreateNewTupleDeclareStatement(public_lib::Context *ctx, ir::ArrayExpression *array,
                                                      ArenaVector<ir::Statement *> &statements)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    ir::Identifier *newTupleId = Gensym(allocator);
    ES2PANDA_ASSERT(newTupleId != nullptr);
    checker::ETSTupleType *tupleType = array->TsType()->AsETSTupleType();

    std::stringstream newArrayDeclareStr;
    newArrayDeclareStr << "let @@I1: (@@T2) = (@@E3);" << std::endl;

    ir::Expression *tupleCreationExpr = GenerateTupleInitExpr(ctx, array, statements);

    ir::Statement *newTupleInitStmt =
        parser->CreateFormattedStatement(newArrayDeclareStr.str(), newTupleId->Clone(allocator, nullptr),
                                         CreateOpaqueTypeNode(ctx, tupleType), tupleCreationExpr);
    statements.emplace_back(newTupleInitStmt);

    return newTupleId;
}

static void AppendSingleElementPush(public_lib::Context *ctx, ir::Expression *element, ir::Identifier *targetArrayIdent,
                                    checker::Type *elementType, ArenaVector<ir::Statement *> &statements)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    statements.emplace_back(parser->CreateFormattedStatement(
        "@@I1.push(@@E2 as @@T3);", targetArrayIdent->CloneReference(allocator, nullptr),
        element->Clone(allocator, nullptr), CreateOpaqueTypeNode(ctx, elementType)));
}

static ir::Statement *CreateReturnStatement(public_lib::Context *ctx, ir::AstNode *newArrayId,
                                            ir::ArrayExpression *array)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();

    std::vector<ir::AstNode *> nodes {newArrayId};
    std::stringstream ss;
    ss << "@@I1 ";

    if (array->TsType()->IsETSResizableArrayType()) {
        ss << "as Object as Array<@@T2>;" << std::endl;
        nodes.emplace_back(
            allocator->New<ir::OpaqueTypeNode>(array->TsType()->AsETSResizableArrayType()->ElementType(), allocator));
    }

    ir::Statement *returnStatement = parser->CreateFormattedStatement(ss.str(), nodes);
    return returnStatement;
}

static ir::Statement *CreateElementsAssignStatementBySpreadArr(public_lib::Context *ctx, ir::Identifier *spId,
                                                               std::vector<ir::AstNode *> &newArrayAndIndex,
                                                               ir::Identifier *spreadArrIterator)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    auto *const newArrayId = newArrayAndIndex[0];
    auto *const newArrayIndexId = newArrayAndIndex[1];

    std::stringstream elementsAssignStr;
    elementsAssignStr << "for (let @@I1 = 0; @@I2 < @@I3.length; @@I4 = @@I5 + 1) {";
    elementsAssignStr << "@@I6[@@I7] = @@I8[@@I9];";
    elementsAssignStr << "@@I10 = @@I11 + 1;";
    elementsAssignStr << "}";

    ES2PANDA_ASSERT(spreadArrIterator != nullptr);
    return parser->CreateFormattedStatement(
        elementsAssignStr.str(), spreadArrIterator->CloneReference(allocator, nullptr),
        spreadArrIterator->CloneReference(allocator, nullptr), spId->CloneReference(allocator, nullptr),
        spreadArrIterator->CloneReference(allocator, nullptr), spreadArrIterator->CloneReference(allocator, nullptr),
        newArrayId->Clone(allocator, nullptr), newArrayIndexId->Clone(allocator, nullptr),
        spId->CloneReference(allocator, nullptr), spreadArrIterator->CloneReference(allocator, nullptr),
        newArrayIndexId->Clone(allocator, nullptr), newArrayIndexId->Clone(allocator, nullptr));
}

static ir::Statement *CreateElementsAssignStatementBySingle(public_lib::Context *ctx, ir::AstNode *element,
                                                            std::vector<ir::AstNode *> &newArrayAndIndex)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    auto *const newArrayId = newArrayAndIndex[0];
    auto *const newArrayIndexId = newArrayAndIndex[1];
    std::stringstream elementsAssignStr;
    elementsAssignStr << "@@I1[@@I2] = (@@E3);";
    elementsAssignStr << "@@I4 = @@I5 + 1;";

    return parser->CreateFormattedStatement(
        elementsAssignStr.str(), newArrayId->Clone(allocator, nullptr), newArrayIndexId->Clone(allocator, nullptr),
        element->Clone(allocator, nullptr), newArrayIndexId->Clone(allocator, nullptr),
        newArrayIndexId->Clone(allocator, nullptr));
}

static std::vector<ir::Statement *> CreateElementsAssignForTupleElements(public_lib::Context *ctx, ir::Identifier *spId,
                                                                         std::vector<ir::AstNode *> &newArrayAndIndex)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    auto *const newArrayId = newArrayAndIndex[0];
    auto *const newArrayIndexId = newArrayAndIndex[1];

    ES2PANDA_ASSERT(spId->TsType()->IsETSTupleType());
    const auto *const spreadType = spId->TsType()->AsETSTupleType();
    std::vector<ir::Statement *> tupleAssignmentStatements {};

    for (size_t idx = 0; idx < spreadType->GetTupleTypesList().size(); ++idx) {
        std::stringstream tupleAssignmentsStr {};
        tupleAssignmentsStr << "@@I1[@@I2] = (@@I3[" << idx << "]);";
        tupleAssignmentsStr << "@@I4 = @@I5 + 1;";
        tupleAssignmentStatements.emplace_back(parser->CreateFormattedStatement(
            tupleAssignmentsStr.str(), newArrayId->Clone(allocator, nullptr),
            newArrayIndexId->Clone(allocator, nullptr), spId->CloneReference(allocator, nullptr),
            newArrayIndexId->Clone(allocator, nullptr), newArrayIndexId->Clone(allocator, nullptr)));
    }

    return tupleAssignmentStatements;
}

static void CreateIndexedNewArrayElementsAssignStatement(public_lib::Context *ctx, ir::ArrayExpression *array,
                                                         std::vector<ir::Identifier *> &spArrIds,
                                                         ArenaVector<ir::Statement *> &statements,
                                                         std::vector<ir::AstNode *> &newArrayAndIndex)
{
    auto *const allocator = ctx->allocator;
    size_t spArrIdx = 0;

    for (auto *element : array->Elements()) {
        if (element->IsSpreadElement()) {
            if (element->AsSpreadElement()->GetResolvedSpreadKind() == ir::SpreadElement::ResolvedSpreadKind::TUPLE) {
                const auto newTupleAssignmentStatements =
                    CreateElementsAssignForTupleElements(ctx, spArrIds[spArrIdx++], newArrayAndIndex);
                statements.insert(statements.cend(), newTupleAssignmentStatements.cbegin(),
                                  newTupleAssignmentStatements.cend());
            } else {
                ir::Identifier *spreadArrIterator = Gensym(allocator);
                statements.emplace_back(CreateElementsAssignStatementBySpreadArr(ctx, spArrIds[spArrIdx++],
                                                                                 newArrayAndIndex, spreadArrIterator));
            }
        } else {
            statements.emplace_back(CreateElementsAssignStatementBySingle(ctx, element, newArrayAndIndex));
        }
    }

    statements.emplace_back(CreateReturnStatement(ctx, newArrayAndIndex[0]->Clone(allocator, nullptr), array));
}

static ir::BlockExpression *CreateIndexedLoweredExpressionForArray(public_lib::Context *ctx, ir::ArrayExpression *array)
{
    auto *const parser = ctx->parser->AsETSParser();
    auto *const allocator = ctx->allocator;
    ArenaVector<ir::Statement *> statements(allocator->Adapter());
    std::vector<ir::Identifier *> spreadArrayIds = {};

    CreateSpreadArrayDeclareStatements(ctx, array, spreadArrayIds, statements);
    ir::Identifier *newArrayLengthId = CreateNewArrayLengthStatement(ctx, array, spreadArrayIds, statements);
    ir::Identifier *newArrayId = CreatePreSizedNewArrayDeclareStatement(ctx, array, statements, newArrayLengthId);
    ES2PANDA_ASSERT(newArrayId != nullptr);
    ir::Identifier *newArrayIndexId = Gensym(allocator);
    ES2PANDA_ASSERT(newArrayIndexId != nullptr);
    statements.emplace_back(
        parser->CreateFormattedStatement("let @@I1 = 0", newArrayIndexId->Clone(allocator, nullptr)));
    std::vector<ir::AstNode *> newArrayAndIndex {newArrayId->Clone(allocator, nullptr),
                                                 newArrayIndexId->Clone(allocator, nullptr)};

    CreateIndexedNewArrayElementsAssignStatement(ctx, array, spreadArrayIds, statements, newArrayAndIndex);
    return ctx->AllocNode<ir::BlockExpression>(std::move(statements));
}

static ir::BlockExpression *CreateMaterializedLoweredExpressionForArray(public_lib::Context *ctx,
                                                                        ir::ArrayExpression *array)
{
    auto *const checker = ctx->GetChecker()->AsETSChecker();
    auto *const allocator = ctx->allocator;
    ArenaVector<ir::Statement *> statements(allocator->Adapter());
    auto *const elementType = checker->GetElementTypeOfArray(array->TsType());
    auto *const tempArrayIdent = CreateSpreadTempResizableArray(ctx, elementType, statements);

    for (auto *element : array->Elements()) {
        if (element->IsSpreadElement()) {
            AppendSpreadToArray(ctx, element->AsSpreadElement(), tempArrayIdent, elementType, statements);
            continue;
        }

        AppendSingleElementPush(ctx, element, tempArrayIdent, elementType, statements);
    }

    auto *const resultArrayIdent = FinalizeSpreadTempArray(ctx, array->TsType(), tempArrayIdent, statements);
    statements.emplace_back(CreateReturnStatement(ctx, resultArrayIdent->CloneReference(allocator, nullptr), array));
    return ctx->AllocNode<ir::BlockExpression>(std::move(statements));
}

/*
 * NOTE: Expand the SpreadExpr to BlockExpr, the rules as follows :
 * let newTuple: typeOfNewTuple = new std.core.TupleN(normalExpr1, ..., normalExprN);
 */
static ir::BlockExpression *CreateLoweredExpressionForTuple(public_lib::Context *ctx, ir::ArrayExpression *array)
{
    auto *const checker = ctx->GetChecker()->AsETSChecker();
    auto *const parser = ctx->parser->AsETSParser();
    auto *const allocator = ctx->allocator;

    ArenaVector<ir::Statement *> statements(allocator->Adapter());
    ir::Identifier *newTupleId = CreateNewTupleDeclareStatement(ctx, array, statements);
    ES2PANDA_ASSERT(newTupleId != nullptr);
    statements.emplace_back(parser->CreateFormattedStatement("@@I1;", newTupleId->CloneReference(allocator, nullptr)));
    return checker->AllocNode<ir::BlockExpression>(std::move(statements));
}

bool SpreadConstructionPhase::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        [ctx = Context()](ir::AstNode *const node) -> AstNodePtr {
            checker::ETSChecker *const checker = ctx->GetChecker()->AsETSChecker();

            if (node->IsArrayExpression() &&
                std::any_of(node->AsArrayExpression()->Elements().begin(), node->AsArrayExpression()->Elements().end(),
                            [](const auto *param) { return param->Type() == ir::AstNodeType::SPREAD_ELEMENT; })) {
                auto scopeCtx =
                    varbinder::LexicalScope<varbinder::Scope>::Enter(checker->VarBinder(), NearestScope(node));

                const auto *const arrayExprType = node->AsArrayExpression()->TsType();
                ir::BlockExpression *blockExpression = nullptr;
                if (arrayExprType->IsETSArrayType() || arrayExprType->IsETSResizableArrayType()) {
                    blockExpression = HasMaterializedSpreadElement(node->AsArrayExpression())
                                          ? CreateMaterializedLoweredExpressionForArray(ctx, node->AsArrayExpression())
                                          : CreateIndexedLoweredExpressionForArray(ctx, node->AsArrayExpression());
                } else {
                    blockExpression = CreateLoweredExpressionForTuple(ctx, node->AsArrayExpression());
                }
                blockExpression->SetParent(node->Parent());

                // NOTE: this blockExpression is a kind of formatted-dummy code, which is invisible to users,
                //       so, its source range should be same as the original code([element1, element2, ...spreadExpr])
                blockExpression->SetRange(node->Range());

                Recheck(ctx->phaseManager, checker->VarBinder()->AsETSBinder(), checker, blockExpression);
                for (auto st : blockExpression->Statements()) {
                    SetSourceRangesRecursively(st, node->Range());
                }

                return blockExpression;
            }

            return node;
        },
        Name());
    return true;
}

}  // namespace ark::es2panda::compiler
