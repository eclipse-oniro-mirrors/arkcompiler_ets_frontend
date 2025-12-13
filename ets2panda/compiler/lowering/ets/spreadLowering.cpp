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

#include "spreadLowering.h"
#include "checker/ETSchecker.h"
#include "checker/types/ets/etsTupleType.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

using AstNodePtr = ir::AstNode *;

void SetPossibleTupleType(ir::Identifier *arrIdent, ir::Expression *spreadArgument)
{
    // Tuple types are used when referenceing the generated identifier of the spread argument node, as tuples don't have
    // 'length' property, and cannot be iterated by for-of statements
    auto *const spreadType = spreadArgument->TsType();
    if (spreadType->IsETSTupleType()) {
        arrIdent->SetTsType(spreadType);
    }
}

void CreateSpreadArrayDeclareStatements(public_lib::Context *ctx, ir::ArrayExpression *array,
                                        std::vector<ir::Identifier *> &spreadArrayIds,
                                        ArenaVector<ir::Statement *> &statements)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    for (auto element : array->Elements()) {
        if (element->Type() != ir::AstNodeType::SPREAD_ELEMENT) {
            continue;
        }
        ir::Identifier *const arrIdent = Gensym(allocator);
        auto *const spreadArgument = element->AsSpreadElement()->Argument();
        auto *const initExpr = spreadArgument->Clone(allocator, nullptr);
        SetPossibleTupleType(arrIdent, spreadArgument);
        spreadArrayIds.emplace_back(arrIdent);
        statements.emplace_back(parser->CreateFormattedStatement("let @@I1 = (@@E2);", arrIdent, initExpr));
    }
}

ir::Identifier *CreateNewArrayLengthStatement(public_lib::Context *ctx, ir::ArrayExpression *array,
                                              std::vector<ir::Identifier *> &spreadArrayIds,
                                              ArenaVector<ir::Statement *> &statements)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    ir::Identifier *newArrayLengthId = Gensym(allocator);
    ES2PANDA_ASSERT(newArrayLengthId != nullptr);
    std::vector<ir::AstNode *> nodesWaitingInsert {newArrayLengthId->Clone(allocator, nullptr)};
    size_t argumentCount = 1;
    std::stringstream lengthString;
    const size_t normalElementCount = array->Elements().size() - spreadArrayIds.size();
    lengthString << "let @@I" << (argumentCount++) << " : int = " << normalElementCount << " + ";
    for (auto *const spaId : spreadArrayIds) {
        if (spaId->TsType() != nullptr && spaId->TsType()->IsETSTupleType()) {
            lengthString << "(" << spaId->TsType()->AsETSTupleType()->GetTupleSize() << ") + ";
        } else {
            lengthString << "(@@I" << (argumentCount++) << ".length.toInt()) + ";
            nodesWaitingInsert.emplace_back(spaId->Clone(allocator, nullptr));
        }
    }
    lengthString << "0;";

    ir::Statement *newArrayLengthStatement = parser->CreateFormattedStatement(lengthString.str(), nodesWaitingInsert);
    statements.emplace_back(newArrayLengthStatement);
    return newArrayLengthId;
}

static ir::Identifier *CreateNewArrayDeclareStatement(public_lib::Context *ctx, ir::ArrayExpression *array,
                                                      ArenaVector<ir::Statement *> &statements,
                                                      ir::Identifier *newArrayLengthId)
{
    auto *const checker = ctx->GetChecker()->AsETSChecker();
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    ir::Identifier *newArrayId = Gensym(allocator);
    ES2PANDA_ASSERT(newArrayId != nullptr);
    checker::Type *arrayElementType = checker->GetElementTypeOfArray(array->TsType());

    // NOTE: If arrayElementType is ETSUnionType(String|Int) or ETSObjectType(private constructor) or ..., we cannot
    //       use "new Type[]" to declare an array, so we generate a new UnionType "arrayElementType|null" to solve
    //       array initialization problems temporarily.
    //       We probably need to use cast Expression in the end of the generated source code to remove "|null", such as
    //       "newArrayName as arrayType[]".
    //       But now cast Expression doesn't support built-in array (cast fatherType[] to sonType[]), so "newArrayName
    //       as arrayType" should be added after cast Expression is implemented completely.
    //       Related issue: #issue20162
    if (checker->IsReferenceType(arrayElementType) &&
        !(arrayElementType->IsETSObjectType() && arrayElementType->AsETSObjectType()->IsBoxedPrimitive())) {
        arrayElementType = checker->CreateETSUnionType({arrayElementType, checker->GlobalETSUndefinedType()});
    }

    std::stringstream newArrayDeclareStr;
    if (array->TsType()->IsETSResizableArrayType()) {
        newArrayDeclareStr << "let @@I1: Array<@@T2> = new Array<@@T3>(@@I4);" << std::endl;
    } else {
        newArrayDeclareStr << "let @@I1: FixedArray<@@T2> = new (@@T3)[@@I4];" << std::endl;
    }

    ES2PANDA_ASSERT(newArrayLengthId != nullptr);
    ir::Statement *newArrayDeclareSt = parser->CreateFormattedStatement(
        newArrayDeclareStr.str(), newArrayId->Clone(allocator, nullptr), arrayElementType, arrayElementType,
        newArrayLengthId->Clone(allocator, nullptr));
    statements.emplace_back(newArrayDeclareSt);

    return newArrayId;
}

static std::string GenerateNewTupleInitList(ArenaAllocator *allocator, ir::ArrayExpression *array,
                                            std::vector<ir::AstNode *> &elementNodes)
{
    std::string tupleInitList {};

    for (auto *element : array->Elements()) {
        if (element->IsSpreadElement()) {
            // Only a tuple type can be spread into a new tuple type
            ES2PANDA_ASSERT(element->AsSpreadElement()->Argument()->TsType()->IsETSTupleType());
            auto *const argumentTupleType = element->AsSpreadElement()->Argument()->TsType()->AsETSTupleType();

            // NOTE (smartin): make a distinct variable for the spread argument. Now if the argument is a function call
            // (that returns a tuple), it'll run every time when inserted into the new initializer. It should however
            // run once, and index the element from that result.

            for (std::size_t idx = 0; idx < argumentTupleType->GetTupleSize(); idx++) {
                tupleInitList +=
                    "@@E" + std::to_string(elementNodes.size() + 1) + "[" + std::to_string(idx) + "]" + ", ";
                elementNodes.emplace_back(element->Clone(allocator, nullptr)->AsSpreadElement()->Argument());
            }
        } else {
            tupleInitList += "@@E" + std::to_string(elementNodes.size() + 1) + ", ";
            elementNodes.emplace_back(element->Clone(allocator, nullptr));
        }
    }

    tupleInitList.pop_back();
    return tupleInitList;
}

static ir::Expression *GenerateTupleInitExpr(public_lib::Context *ctx, ir::ArrayExpression *array)
{
    auto *const parser = ctx->parser->AsETSParser();
    auto *const allocator = ctx->allocator;

    std::vector<ir::AstNode *> arrayExprElementNodes {};
    std::stringstream newTupleExpr;

    newTupleExpr << "[";
    newTupleExpr << GenerateNewTupleInitList(allocator, array, arrayExprElementNodes);
    newTupleExpr << "];";

    return parser->CreateFormattedExpression(newTupleExpr.str(), arrayExprElementNodes);
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

    ir::Expression *tupleCreationExpr = GenerateTupleInitExpr(ctx, array);

    ir::Statement *newTupleInitStmt = parser->CreateFormattedStatement(
        newArrayDeclareStr.str(), newTupleId->Clone(allocator, nullptr), tupleType, tupleCreationExpr);
    statements.emplace_back(newTupleInitStmt);

    return newTupleId;
}

static ir::Statement *CreateElementsAssignStatementBySpreadArr(public_lib::Context *ctx, ir::Identifier *spId,
                                                               std::vector<ir::AstNode *> &newArrayAndIndex,
                                                               ir::Identifier *spreadArrIterator,
                                                               checker::Type *arrayElementType)
{
    auto *const allocator = ctx->allocator;
    auto *const parser = ctx->parser->AsETSParser();
    auto *const newArrayId = newArrayAndIndex[0];
    auto *const newArrayIndexId = newArrayAndIndex[1];

    std::stringstream elementsAssignStr;
    elementsAssignStr << "for (let @@I1 of @@I2) {";
    elementsAssignStr << "@@I3[@@I4] = @@I5 as @@T6;";
    elementsAssignStr << "@@I7++;";
    elementsAssignStr << "}";

    ES2PANDA_ASSERT(spreadArrIterator != nullptr);
    ir::Statement *elementsAssignStatement = parser->CreateFormattedStatement(
        elementsAssignStr.str(), spreadArrIterator->Clone(allocator, nullptr), spId->Clone(allocator, nullptr),
        newArrayId->Clone(allocator, nullptr), newArrayIndexId->Clone(allocator, nullptr),
        spreadArrIterator->Clone(allocator, nullptr), arrayElementType, newArrayIndexId->Clone(allocator, nullptr));

    return elementsAssignStatement;
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
    elementsAssignStr << "@@I4++;";

    ir::Statement *elementsAssignStatement = parser->CreateFormattedStatement(
        elementsAssignStr.str(), newArrayId->Clone(allocator, nullptr), newArrayIndexId->Clone(allocator, nullptr),
        element->Clone(allocator, nullptr), newArrayIndexId->Clone(allocator, nullptr));

    return elementsAssignStatement;
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
        auto *elementType = spreadType->GetTupleTypesList()[idx];
        tupleAssignmentsStr << "@@I1[@@I2] = (@@I3[" << idx << "] as @@T4);";
        tupleAssignmentsStr << "@@I5++;";
        tupleAssignmentStatements.emplace_back(parser->CreateFormattedStatement(
            tupleAssignmentsStr.str(), newArrayId->Clone(allocator, nullptr),
            newArrayIndexId->Clone(allocator, nullptr), spId->Clone(allocator, nullptr), elementType,
            newArrayIndexId->Clone(allocator, nullptr)));
    }

    return tupleAssignmentStatements;
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

static void CreateNewArrayElementsAssignStatement(public_lib::Context *ctx, ir::ArrayExpression *array,
                                                  std::vector<ir::Identifier *> &spArrIds,
                                                  ArenaVector<ir::Statement *> &statements,
                                                  std::vector<ir::AstNode *> &newArrayAndIndex)
{
    auto *const allocator = ctx->allocator;
    auto *const newArrayId = newArrayAndIndex[0];
    size_t spArrIdx = 0;

    for (auto *element : array->Elements()) {
        if (element->IsSpreadElement()) {
            if (element->AsSpreadElement()->Argument()->TsType()->IsETSTupleType()) {
                const auto newTupleAssignmentStatements =
                    CreateElementsAssignForTupleElements(ctx, spArrIds[spArrIdx++], newArrayAndIndex);
                statements.insert(statements.cend(), newTupleAssignmentStatements.cbegin(),
                                  newTupleAssignmentStatements.cend());
            } else {
                ir::Identifier *spreadArrIterator = Gensym(allocator);
                checker::Type *arrayElementType =
                    ctx->GetChecker()->AsETSChecker()->GetElementTypeOfArray(array->TsType());
                statements.emplace_back(CreateElementsAssignStatementBySpreadArr(
                    ctx, spArrIds[spArrIdx++], newArrayAndIndex, spreadArrIterator, arrayElementType));
            }
        } else {
            statements.emplace_back(CreateElementsAssignStatementBySingle(ctx, element, newArrayAndIndex));
        }
    }

    statements.emplace_back(CreateReturnStatement(ctx, newArrayId->Clone(allocator, nullptr), array));
}

/*
 * NOTE: Expand the SpreadExpr to BlockExpr, the rules as follows :
 * let spreadArrayId1 = spreadExpr1
 * let length : int = normalExprCount + spreadArrayId1.length + 0
 * type typeOfNewArray = arrayType
 * let newArray: typeOfNewArray[] = new typeOfNewArray[length]
 * let newArrayIndex = 0
 * let elementOfSpreadArray1: arrayType
 * for (elementOfSpreadArray1 of spreadArray1) {
 *     newArray[newArrayIndex] = elementOfSpreadArray1
 *     newArrayIndex++
 * }
 * newArray[newArrayIndex] = normalExpr1
 * newArrayIndex++
 * ...
 * newArray;
 */
static ir::BlockExpression *CreateLoweredExpressionForArray(public_lib::Context *ctx, ir::ArrayExpression *array)
{
    auto *const parser = ctx->parser->AsETSParser();
    auto *const allocator = ctx->allocator;
    ArenaVector<ir::Statement *> statements(allocator->Adapter());
    std::vector<ir::Identifier *> spreadArrayIds = {};

    CreateSpreadArrayDeclareStatements(ctx, array, spreadArrayIds, statements);

    ir::Identifier *newArrayLengthId = CreateNewArrayLengthStatement(ctx, array, spreadArrayIds, statements);

    ir::Identifier *newArrayId = CreateNewArrayDeclareStatement(ctx, array, statements, newArrayLengthId);
    ES2PANDA_ASSERT(newArrayId != nullptr);
    ir::Identifier *newArrayIndexId = Gensym(allocator);
    ES2PANDA_ASSERT(newArrayIndexId != nullptr);
    statements.emplace_back(
        parser->CreateFormattedStatement("let @@I1 = 0", newArrayIndexId->Clone(allocator, nullptr)));
    std::vector<ir::AstNode *> newArrayAndIndex {newArrayId->Clone(allocator, nullptr),
                                                 newArrayIndexId->Clone(allocator, nullptr)};

    CreateNewArrayElementsAssignStatement(ctx, array, spreadArrayIds, statements, newArrayAndIndex);
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
    statements.emplace_back(parser->CreateFormattedStatement("@@I1;", newTupleId->Clone(allocator, nullptr)));
    return checker->AllocNode<ir::BlockExpression>(std::move(statements));
}

bool SpreadConstructionPhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    checker::ETSChecker *const checker = ctx->GetChecker()->AsETSChecker();
    varbinder::ETSBinder *const varbinder = checker->VarBinder()->AsETSBinder();

    program->Ast()->TransformChildrenRecursively(
        [&checker, &varbinder, &ctx](ir::AstNode *const node) -> AstNodePtr {
            if (node->IsArrayExpression() &&
                std::any_of(node->AsArrayExpression()->Elements().begin(), node->AsArrayExpression()->Elements().end(),
                            [](const auto *param) { return param->Type() == ir::AstNodeType::SPREAD_ELEMENT; })) {
                auto scopeCtx =
                    varbinder::LexicalScope<varbinder::Scope>::Enter(checker->VarBinder(), NearestScope(node));

                const auto *const arrayExprType = node->AsArrayExpression()->TsType();
                ir::BlockExpression *blockExpression =
                    arrayExprType->IsETSArrayType() || arrayExprType->IsETSResizableArrayType()
                        ? CreateLoweredExpressionForArray(ctx, node->AsArrayExpression())
                        : CreateLoweredExpressionForTuple(ctx, node->AsArrayExpression());
                blockExpression->SetParent(node->Parent());

                // NOTE: this blockExpression is a kind of formatted-dummy code, which is invisible to users,
                //       so, its source range should be same as the original code([element1, element2, ...spreadExpr])
                blockExpression->SetRange(node->Range());
                Recheck(ctx->phaseManager, varbinder, checker, blockExpression);
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
