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

#include "restArgsLowering.h"
#include "checker/types/ets/etsTupleType.h"
#include "compiler/lowering/util.h"
#include "ir/astNode.h"
#include "ir/expression.h"
#include "ir/expressions/arrayExpression.h"
#include "ir/expressions/literals/undefinedLiteral.h"
#include "ir/ts/tsAsExpression.h"
#include "ir/typeNode.h"
#include <checker/ETSchecker.h>
#include <cstddef>
#include <iterator>
#include <sstream>

namespace ark::es2panda::compiler {

using AstNodePtr = ir::AstNode *;
constexpr size_t ARRAY_INDEX_FOR_ACCESS_OFFSET = 1;
constexpr size_t ARGUMENT_SYMBOL_OFFSET = 2;
constexpr size_t ARRAY_INDEX_FOR_ASSIGNMENT_OFFSET = 3;
constexpr size_t ARRAY_INDEX_FOR_INCREMENT_OFFSET = 4;
constexpr size_t TUPLE_SPREAD_ARG_COUNT = 5;

struct RestArgsBlockExpressionData {
    std::string spreadArgsArray;
    std::string arrayLength;
    ir::Identifier *arraySymbol;
    ir::Identifier *arrayIndex;
    ir::Identifier *argumentIndex;
};

RestArgsBlockExpressionData CreateRestArgsBlockExpressionData(public_lib::Context *context)
{
    auto *allocator = context->allocator;

    const auto spreadArgsArray = GenName();
    const auto arrayLength = GenName();
    const auto arraySymbol = Gensym(allocator);
    ES2PANDA_ASSERT(arraySymbol != nullptr);
    const auto arrayIndex = Gensym(allocator);
    ES2PANDA_ASSERT(arrayIndex != nullptr);
    const auto argumentIndex = Gensym(allocator);
    ES2PANDA_ASSERT(argumentIndex != nullptr);
    return {spreadArgsArray, arrayLength, arraySymbol, arrayIndex, argumentIndex};
}

static ir::Statement *FillArrayWithSpreadElement(public_lib::Context *context, ir::Expression *argument,
                                                 ir::Identifier *arraySymbolWithoutTypeAnnotation,
                                                 ir::Identifier *argumentSymbol,
                                                 const RestArgsBlockExpressionData &data)
{
    auto *allocator = context->allocator;
    auto *parser = context->parser->AsETSParser();

    std::vector<ir::AstNode *> args;
    std::stringstream ss;
    auto spreadType = argument->AsSpreadElement()->Argument()->TsType();
    if (spreadType->IsETSTupleType()) {
        size_t index = 1;
        for (size_t j = 0; j < spreadType->AsETSTupleType()->GetTupleSize(); j++) {
            ss << "@@I" << index << "[@@I" << (index + ARRAY_INDEX_FOR_ACCESS_OFFSET) << "] = @@I"
               << (index + ARGUMENT_SYMBOL_OFFSET) << "[" << j << "];";
            args.emplace_back(arraySymbolWithoutTypeAnnotation->Clone(allocator, nullptr));
            args.emplace_back(data.arrayIndex->Clone(allocator, nullptr));
            args.emplace_back(argumentSymbol->Clone(allocator, nullptr));
            ss << "@@I" << (index + ARRAY_INDEX_FOR_ASSIGNMENT_OFFSET) << " = @@I"
               << (index + ARRAY_INDEX_FOR_INCREMENT_OFFSET) << " + 1;";
            args.emplace_back(data.arrayIndex->Clone(allocator, nullptr));
            args.emplace_back(data.arrayIndex->Clone(allocator, nullptr));
            index += TUPLE_SPREAD_ARG_COUNT;
        }
    } else {
        ss << "for (let @@I1: int = 0; @@I2 < @@I3.length; @@I4++){";
        args.emplace_back(data.argumentIndex->Clone(allocator, nullptr));
        args.emplace_back(data.argumentIndex->Clone(allocator, nullptr));
        args.emplace_back(argumentSymbol->Clone(allocator, nullptr));
        args.emplace_back(data.argumentIndex->Clone(allocator, nullptr));
        ss << "@@I5[@@I6] = @@I7[@@I8];";
        args.emplace_back(arraySymbolWithoutTypeAnnotation->Clone(allocator, nullptr));
        args.emplace_back(data.arrayIndex->Clone(allocator, nullptr));
        args.emplace_back(argumentSymbol->Clone(allocator, nullptr));
        args.emplace_back(data.argumentIndex->Clone(allocator, nullptr));
        ss << "@@I9 = @@I10 + 1;";
        args.emplace_back(data.arrayIndex->Clone(allocator, nullptr));
        args.emplace_back(data.arrayIndex->Clone(allocator, nullptr));
        ss << "}";
    }
    return parser->CreateFormattedStatement(ss.str(), args);
}

static ir::Expression *FillArrayWithArguments(public_lib::Context *context,
                                              ArenaVector<ir::Statement *> &blockStatements,
                                              const ArenaVector<ir::Expression *> &arguments,
                                              const RestArgsBlockExpressionData &data)
{
    auto *allocator = context->allocator;
    auto *parser = context->parser->AsETSParser();

    auto *arraySymbolWithoutTypeAnnotation = data.arraySymbol->Clone(allocator, nullptr)->AsIdentifier();
    arraySymbolWithoutTypeAnnotation->SetTypeAnnotation(nullptr);

    blockStatements.push_back(parser->CreateFormattedStatement("let @@I1 = 0;", data.arrayIndex));
    size_t spreadArgIndex = 0;
    for (auto argument : arguments) {
        const auto argumentSymbol = Gensym(allocator);
        ES2PANDA_ASSERT(argumentSymbol != nullptr);
        if (argument->IsSpreadElement()) {
            auto spreadType = argument->AsSpreadElement()->Argument()->TsType();
            auto spreadTypeNode = allocator->New<ir::OpaqueTypeNode>(spreadType, allocator);
            blockStatements.push_back(
                parser->CreateFormattedStatement("let @@I1 = @@I2[" + std::to_string(spreadArgIndex) + "] as @@T3;",
                                                 argumentSymbol, data.spreadArgsArray, spreadTypeNode));
            spreadArgIndex++;
            blockStatements.push_back(
                FillArrayWithSpreadElement(context, argument, arraySymbolWithoutTypeAnnotation, argumentSymbol, data));
        } else {
            auto *argTypeNode = allocator->New<ir::OpaqueTypeNode>(argument->TsType(), allocator);
            blockStatements.push_back(parser->CreateFormattedStatement(
                "@@I1[@@I2] = @@E3 as @@T4;", arraySymbolWithoutTypeAnnotation->Clone(allocator, nullptr),
                data.arrayIndex->Clone(allocator, nullptr), argument->Clone(allocator, nullptr), argTypeNode));
            blockStatements.push_back(parser->CreateFormattedStatement("@@I1 = @@I2 + 1;",
                                                                       data.arrayIndex->Clone(allocator, nullptr),
                                                                       data.arrayIndex->Clone(allocator, nullptr)));
        }
    }

    return arraySymbolWithoutTypeAnnotation;
}

static size_t GetNonSpreadArgCount(const ArenaVector<ir::Expression *> &arguments,
                                   ArenaVector<ir::Expression *> &arrayElements)
{
    size_t nonSpreadArgCount = 0;
    for (auto *argExpr : arguments) {
        if (argExpr->IsSpreadElement()) {
            arrayElements.push_back(argExpr->AsSpreadElement()->Argument());
        } else {
            nonSpreadArgCount++;
        }
    }
    return nonSpreadArgCount;
}

static ir::TypeNode *GetConstraintTypeNode(checker::ETSChecker *checker, checker::Type *constraintType,
                                           bool useConstraintType, checker::Type *arrayType)
{
    auto *allocator = checker->Allocator();
    const auto elementType = checker->GetElementTypeOfArray(arrayType);
    ir::TypeNode *constraintTypeNode = nullptr;
    if (useConstraintType) {
        // For rest parameter in arrow function and generic FixedArray rest parameter in normal function, we need to use
        // the constraint type.
        constraintTypeNode = allocator->New<ir::OpaqueTypeNode>(constraintType, allocator);
    } else if (arrayType->IsETSArrayType()) {
        constraintTypeNode = allocator->New<ir::OpaqueTypeNode>(elementType, allocator);
    } else {
        constraintTypeNode = allocator->New<ir::OpaqueTypeNode>(elementType, allocator);
    }

    return constraintTypeNode;
}

static ir::BlockExpression *CreateRestArgsBlockExpression(public_lib::Context *context,
                                                          ArenaVector<ir::Expression *> &arguments,
                                                          checker::Type *constraintType, checker::Type *arrayType,
                                                          bool useConstraintType)
{
    auto *allocator = context->allocator;
    auto *parser = context->parser->AsETSParser();
    auto *checker = context->GetChecker()->AsETSChecker();

    ArenaVector<ir::Statement *> blockStatements(allocator->Adapter());
    auto data = CreateRestArgsBlockExpressionData(context);

    ArenaVector<ir::Expression *> spreadElements(allocator->Adapter());

    auto nonSpreadArgCount = GetNonSpreadArgCount(arguments, spreadElements);

    // tmp array to store spread arguments for avoiding repeated evaluation
    blockStatements.push_back(parser->CreateFormattedStatement("let @@I1: FixedArray<Any> = new FixedArray<Any>(" +
                                                                   std::to_string(spreadElements.size()) + ");",
                                                               data.spreadArgsArray));

    // calculate the length of array to be created
    blockStatements.push_back(parser->CreateFormattedStatement("let @@I1: int = 0;", data.arrayLength));
    for (size_t i = 0; i < spreadElements.size(); ++i) {
        auto arg = spreadElements[i];
        auto argType = arg->TsType();
        auto argTypeNode = allocator->New<ir::OpaqueTypeNode>(argType, allocator);
        blockStatements.push_back(parser->CreateFormattedStatement("@@I1[" + std::to_string(i) + "] = @@E2 as @@T3;",
                                                                   data.spreadArgsArray, arg, argTypeNode));
        if (argType->IsETSTupleType() && argType->AsETSTupleType()->GetTupleSize() > 0) {
            blockStatements.push_back(parser->CreateFormattedStatement(
                "@@I1 = @@I2 + " + std::to_string(argType->AsETSTupleType()->GetTupleSize()) + " ;", data.arrayLength,
                data.arrayLength));
        } else {
            blockStatements.push_back(parser->CreateFormattedStatement(
                "@@I1 = @@I2 + (@@I3[" + std::to_string(i) + "] as @@T4).length;", data.arrayLength, data.arrayLength,
                data.spreadArgsArray, argTypeNode->Clone(allocator, nullptr)));
        }
    }
    if (nonSpreadArgCount > 0) {
        blockStatements.push_back(parser->CreateFormattedStatement(
            "@@I1 = @@I2 + " + std::to_string(nonSpreadArgCount) + ";", data.arrayLength, data.arrayLength));
    }

    auto constraintTypeNode = GetConstraintTypeNode(checker, constraintType, useConstraintType, arrayType);
    // For rest paramter in arrow function(whether it is FixedArray or Array), we need to create a FixedArray, for
    // normal function it depends on the type of the rest parameter.
    if (useConstraintType || arrayType->IsETSArrayType()) {
        blockStatements.push_back(parser->CreateFormattedStatement(
            "let @@I1: FixedArray<@@T2> = new FixedArray<@@T3>(@@I4);", data.arraySymbol, constraintTypeNode,
            constraintTypeNode->Clone(allocator, nullptr), data.arrayLength));
    } else {
        blockStatements.push_back(parser->CreateFormattedStatement(
            "let @@I1 = new Array<@@T2>(@@I3);", data.arraySymbol, constraintTypeNode, data.arrayLength));
    }

    // fill the array with the arguments in original callExpression.
    auto *arraySymbolWithoutTypeAnnotation = FillArrayWithArguments(context, blockStatements, arguments, data);

    blockStatements.push_back(
        parser->CreateFormattedStatement("@@I1", arraySymbolWithoutTypeAnnotation->Clone(allocator, nullptr)));
    auto *blockExpr = util::NodeAllocator::ForceSetParent<ir::BlockExpression>(allocator, std::move(blockStatements));
    return blockExpr;
}

bool ShouldSkipParamCheck(checker::Signature *signature, const ArenaVector<ir::Expression *> &args)
{
    if (signature == nullptr) {
        return false;
    }

    if (signature->Params().empty()) {
        return false;
    }

    const auto &params = signature->Params();

    if (params.empty()) {
        return false;
    }

    if (!signature->HasRestParameter()) {
        return false;
    }

    auto *restVar = signature->RestVar();
    if (restVar == nullptr) {
        return false;
    }

    size_t nMandatory = 0;
    size_t nOptional = 0;

    for (auto *param : params) {
        if (param == nullptr) {
            return false;
        }

        auto *decl = param->Declaration();
        if (decl == nullptr) {
            return false;
        }

        auto *node = decl->Node();
        if (node == nullptr || !node->IsETSParameterExpression()) {
            return false;
        }

        auto *etsParam = node->AsETSParameterExpression();
        if (etsParam == nullptr) {
            return false;
        }

        if (etsParam->IsOptional()) {
            ++nOptional;
        } else {
            ++nMandatory;
        }
    }

    return args.size() < (nOptional + nMandatory) && nOptional > 0;
}

static bool ShouldProcessRestParameters(checker::Signature *signature, checker::Type *calleeType,
                                        const ArenaVector<ir::Expression *> &arguments)
{
    if (signature == nullptr || !signature->HasRestParameter()) {
        return false;
    }

    auto *restVar = signature->RestVar();
    if (restVar == nullptr) {
        return false;
    }

    auto *restVarType = restVar->TsType();
    if (restVarType == nullptr) {
        return false;
    }

    bool isRestVarTupleType = restVarType->IsETSTupleType();
    if (isRestVarTupleType) {
        return false;
    }

    bool hasEnoughArguments = arguments.size() >= signature->Params().size();
    bool shouldSkipParamCheck = ShouldSkipParamCheck(signature, arguments);
    if (!hasEnoughArguments && !shouldSkipParamCheck) {
        return false;
    }

    return calleeType->IsETSArrowType() || !signature->Function()->IsDynamic();
}

static ir::Expression *CreateRestArgsArrayWithoutSpread(public_lib::Context *context,
                                                        ArenaVector<ir::Expression *> &copiedArguments,
                                                        ir::TypeNode *arrayTypeNode)
{
    auto *allocator = context->allocator;
    auto *parser = context->parser->AsETSParser();
    auto *checker = context->GetChecker()->AsETSChecker();
    const lexer::SourceRange range = {copiedArguments.front()->Start(), copiedArguments.back()->End()};

    std::stringstream ss;
    auto *genSymIdent = Gensym(allocator);
    auto *genSymIdent2 = Gensym(allocator);
    // Was:
    // ss << "let @@I1 : FixedArray<@@T2> = @@E3;";
    // ss << "Array.from<@@T4>(@@I5);";
    // Now:
    // NOTE: refactor me!
    ES2PANDA_ASSERT(genSymIdent != nullptr && genSymIdent2 != nullptr);
    ss << "let @@I1 : FixedArray<@@T2> = @@E3;";
    ss << "let @@I4 : Array<@@T5> = new Array<@@T6>(@@I7.length);";
    ss << "for (let i = 0; i < @@I8.length; ++i) { @@I9[i] = @@I10[i]}";
    ss << "@@I11;";
    auto *arrayExpr = checker->AllocNode<ir::ArrayExpression>(std::move(copiedArguments), allocator);
    ES2PANDA_ASSERT(arrayTypeNode != nullptr);
    auto *loweringResult = parser->CreateFormattedExpression(
        ss.str(), genSymIdent, arrayTypeNode->Clone(allocator, nullptr), arrayExpr, genSymIdent2, arrayTypeNode,
        arrayTypeNode->Clone(allocator, nullptr), genSymIdent->Clone(allocator, nullptr),
        genSymIdent->Clone(allocator, nullptr), genSymIdent2->Clone(allocator, nullptr),
        genSymIdent->Clone(allocator, nullptr), genSymIdent2->Clone(allocator, nullptr));

    loweringResult->SetRange(range);
    return loweringResult;
}

static std::pair<checker::Type *, bool> CalculateRestArgsConstraintInfo(checker::ETSChecker *checker,
                                                                        const checker::Signature *signature,
                                                                        bool isArrowType)
{
    // Due to the defects left after the Primitive Type refactoring, the type of FixedArray here must be constraintType
    // to prevent unboxing.
    checker::Type *constraintType = nullptr;
    bool needsConstraintType = false;
    if (isArrowType) {
        // the arrow function will be lowered in LambdaLowering, so the corresponding rest parameter type is
        // FixedArray<Any>.
        constraintType = checker->GlobalETSAnyType();
        needsConstraintType = true;
    } else {
        auto originalRestParamType = signature->Function()->Signature()->RestVar()->TsType();
        auto paramRestType = checker->GetElementTypeOfArray(originalRestParamType);
        needsConstraintType = originalRestParamType->IsETSArrayType() && paramRestType->IsETSTypeParameter();
        constraintType = originalRestParamType->IsETSTypeParameter()
                             ? originalRestParamType->AsETSTypeParameter()->GetConstraintType()
                             : checker->GlobalETSAnyType();
    }

    return {constraintType, needsConstraintType};
}

static bool IsSameArrayType(checker::Type *arrayType1, checker::Type *arrayType2)
{
    return (arrayType1->IsETSArrayType() && arrayType2->IsETSArrayType()) ||
           (arrayType1->IsETSResizableArrayType() && arrayType2->IsETSResizableArrayType()) ||
           (arrayType1->IsETSReadonlyArrayType() && arrayType2->IsETSReadonlyArrayType());
}

// desc: The following cases we can skip lowering:
// Definition side: - Case 1: "function foo(...arr: FixedArray<T>) { ... }"
//   - Case 2: "let foo = (...arr: FixedArray<T>) => { ... }"
//   - Case 3: "let foo = (...arr: T[]) => { ... }"
// Call side: - Direct arguments: "foo(arg1, arg2, arg3, ...)"
//   - Only one spread argument: "let arr: FixedArray<T> = ...; foo(...arr);"
static bool CanSkipRestArgsLowering(checker::ETSChecker *checker, const checker::Signature *signature,
                                    checker::Type *restParamType, const ArenaVector<ir::Expression *> &copiedArguments,
                                    bool isArrowType)
{
    auto isElementTypePrimitive =
        checker->MaybeUnboxType(checker->GetElementTypeOfArray(restParamType))->IsETSPrimitiveType();
    bool hasSpreadArg = std::any_of(copiedArguments.begin(), copiedArguments.end(),
                                    [](ir::Expression *arg) { return arg->IsSpreadElement(); });
    bool hasOnlyOneSpreadArg = hasSpreadArg && copiedArguments.size() == 1;
    bool hasIncompatibleArrayType =
        std::any_of(copiedArguments.begin(), copiedArguments.end(), [&](ir::Expression *arg) {
            if (arg->IsSpreadElement()) {
                return !IsSameArrayType(arg->AsSpreadElement()->Argument()->TsType(),
                                        isArrowType ? checker->CreateETSArrayType(checker->GlobalETSAnyType())
                                                    : restParamType);
            }
            return false;
        });

    // Due to the defects left after the Primitive Type refactoring, we can't skip lowering for generic rest
    // parameter. Should be removed after the bug is fixed.
    if ((isElementTypePrimitive && restParamType->IsETSArrayType()) || isArrowType) {
        if (!hasSpreadArg) {
            return true;
        }
        bool isRestParamGeneric =
            isArrowType || checker->GetElementTypeOfArray(signature->Function()->Signature()->RestVar()->TsType())
                               ->IsETSTypeParameter();
        if (isRestParamGeneric) {
            return false;
        }
        return hasOnlyOneSpreadArg && !hasIncompatibleArrayType;
    }

    if (!isArrowType && (restParamType->IsETSResizableArrayType() || restParamType->IsETSReadonlyArrayType())) {
        return false;
    }
    if (!hasSpreadArg) {
        return true;
    }
    if (hasOnlyOneSpreadArg && !hasIncompatibleArrayType) {
        return true;
    }
    return false;
}

static ir::Expression *CreateRestArgsArray(public_lib::Context *context, ir::Expression *expr, bool isArrowType = false)
{
    auto *allocator = context->allocator;
    auto *parser = context->parser->AsETSParser();
    auto *checker = context->GetChecker()->AsETSChecker();

    const auto &arguments = expr->IsCallExpression() ? expr->AsCallExpression()->Arguments()
                                                     : expr->AsETSNewClassInstanceExpression()->GetArguments();
    const auto *signature = expr->IsCallExpression() ? expr->AsCallExpression()->Signature()
                                                     : expr->AsETSNewClassInstanceExpression()->Signature();

    checker::Type *restParamType = signature->RestVar()->TsType();
    auto arrayTypeNode =
        checker->AllocNode<ir::OpaqueTypeNode>(checker->GetElementTypeOfArray(restParamType), allocator);

    const size_t numRegularParams = signature->Params().size();
    const size_t numExtraArgs = arguments.size() > numRegularParams ? arguments.size() - numRegularParams : 0;
    // If the rest parameter is resizable array and there are no extra arguments, return an empty array.For fixed array,
    // we don't need to handle it here.
    if (!restParamType->IsETSArrayType() && numExtraArgs == 0) {
        return parser->CreateFormattedExpression("new Array<@@T1>(0)", arrayTypeNode);
    }

    ArenaVector<ir::Expression *> copiedArguments(arguments.begin() + numRegularParams, arguments.end(),
                                                  allocator->Adapter());

    bool hasSpreadArg = std::any_of(copiedArguments.begin(), copiedArguments.end(),
                                    [](ir::Expression *arg) { return arg->IsSpreadElement(); });

    if (CanSkipRestArgsLowering(checker, signature, restParamType, copiedArguments, isArrowType)) {
        return nullptr;
    }

    if (hasSpreadArg) {
        auto [constraintType, needsConstraintType] = CalculateRestArgsConstraintInfo(checker, signature, isArrowType);
        const lexer::SourceRange range = {copiedArguments.front()->Start(), copiedArguments.back()->End()};
        auto *blockExpression =
            CreateRestArgsBlockExpression(context, copiedArguments, constraintType, restParamType, needsConstraintType);
        ES2PANDA_ASSERT(blockExpression != nullptr);
        blockExpression->SetParent(arguments.back()->Parent());
        blockExpression->SetRange(range);
        return blockExpression;
    }

    return CreateRestArgsArrayWithoutSpread(context, copiedArguments, arrayTypeNode);
}

static ir::CallExpression *RebuildCallExpression(public_lib::Context *context, ir::CallExpression *originalCall,
                                                 checker::Signature *signature, ir::Expression *restArgsArray)

{
    auto *allocator = context->allocator;
    auto *varbinder = context->GetChecker()->VarBinder()->AsETSBinder();
    ArenaVector<ir::Expression *> newArgs(allocator->Adapter());

    if (!originalCall->Arguments().empty()) {
        for (size_t i = 0; i < signature->Params().size() && i < originalCall->Arguments().size(); ++i) {
            newArgs.push_back(originalCall->Arguments()[i]);
        }
    }
    if (ShouldSkipParamCheck(signature, originalCall->Arguments())) {
        auto *checker = context->GetChecker()->AsETSChecker();
        auto *undefinedExpr = checker->AllocNode<ir::UndefinedLiteral>();
        size_t nMissing = signature->Params().size() - originalCall->Arguments().size();
        auto it = signature->Params().begin();
        std::advance(it, originalCall->Arguments().size());
        for (size_t i = 0; i < nMissing; ++i) {
            undefinedExpr->SetTsType((*it)->TsType());
            newArgs.push_back(undefinedExpr);
            ++it;
        }
    }
    newArgs.push_back(restArgsArray);

    auto *newCall = util::NodeAllocator::ForceSetParent<ir::CallExpression>(allocator, originalCall->Callee(),
                                                                            std::move(newArgs), nullptr, false);
    ES2PANDA_ASSERT(newCall != nullptr);
    restArgsArray->SetParent(newCall);
    newCall->SetParent(originalCall->Parent());
    newCall->AddModifier(originalCall->Modifiers());
    newCall->SetTypeParams(originalCall->TypeParams());
    restArgsArray->AddAstNodeFlags(ir::AstNodeFlags::REST_ARGUMENT);

    SetSourceRangesRecursively(newCall, originalCall->Range());

    auto *scope = NearestScope(newCall->Parent());
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(varbinder, scope);
    CheckLoweredNode(context->GetChecker()->VarBinder()->AsETSBinder(), context->GetChecker()->AsETSChecker(), newCall);
    return newCall;
}

static ir::ETSNewClassInstanceExpression *RebuildNewClassInstanceExpression(
    public_lib::Context *context, ir::ETSNewClassInstanceExpression *originalCall, checker::Signature *signature,
    ir::Expression *restArgsArray)
{
    auto *allocator = context->allocator;
    ArenaVector<ir::Expression *> newArgs(allocator->Adapter());

    for (size_t i = 0; i < signature->Params().size(); ++i) {
        newArgs.push_back(originalCall->GetArguments()[i]);
    }

    newArgs.push_back(restArgsArray);

    auto *newCall = util::NodeAllocator::ForceSetParent<ir::ETSNewClassInstanceExpression>(
        allocator, originalCall->GetTypeRef()->Clone(allocator, nullptr)->AsETSTypeReference(), std::move(newArgs));
    restArgsArray->AddAstNodeFlags(ir::AstNodeFlags::REST_ARGUMENT);
    restArgsArray->SetParent(newCall);
    newCall->SetParent(originalCall->Parent());
    newCall->AddModifier(originalCall->Modifiers());
    SetSourceRangesRecursively(newCall, originalCall->Range());

    auto *scope = NearestScope(newCall->Parent());
    auto bscope =
        varbinder::LexicalScope<varbinder::Scope>::Enter(context->GetChecker()->VarBinder()->AsETSBinder(), scope);
    CheckLoweredNode(context->GetChecker()->VarBinder()->AsETSBinder(), context->GetChecker()->AsETSChecker(), newCall);
    return newCall;
}

static ir::ETSNewClassInstanceExpression *TransformCallConstructWithRestArgs(ir::ETSNewClassInstanceExpression *expr,
                                                                             public_lib::Context *context)
{
    checker::Signature *signature = expr->Signature();
    if (!ShouldProcessRestParameters(signature, expr->GetTypeRef()->TsType(), expr->GetArguments())) {
        return expr;
    }

    auto *restArgsArray = CreateRestArgsArray(context, expr);
    if (restArgsArray == nullptr) {
        return expr;
    }

    auto arg =
        context->AllocNode<ir::SpreadElement>(ir::AstNodeType::SPREAD_ELEMENT, context->allocator, restArgsArray);
    return RebuildNewClassInstanceExpression(context, expr, signature, arg);
}

static ir::CallExpression *TransformCallExpressionWithRestArgs(ir::CallExpression *callExpr,
                                                               public_lib::Context *context)
{
    checker::Type *calleeType = context->GetChecker()->AsETSChecker()->GetNormalizedType(callExpr->Callee()->TsType());
    if (calleeType == nullptr) {
        return callExpr;
    }

    bool isArrowType = calleeType->IsETSArrowType();
    checker::Signature *signature = callExpr->Signature();
    if (signature == nullptr || (!isArrowType && signature->Function()->IsDynamic())) {
        return callExpr;
    }

    bool hasSpreadArg = std::any_of(callExpr->Arguments().begin(), callExpr->Arguments().end(),
                                    [](ir::Expression *arg) { return arg->IsSpreadElement(); });
    if (isArrowType && !hasSpreadArg) {
        return callExpr;
    }

    if (!ShouldProcessRestParameters(signature, calleeType, callExpr->Arguments())) {
        return callExpr;
    }

    auto *restArgsArray = CreateRestArgsArray(context, callExpr, calleeType->IsETSArrowType());
    if (restArgsArray == nullptr) {
        return callExpr;
    }
    auto arg =
        context->AllocNode<ir::SpreadElement>(ir::AstNodeType::SPREAD_ELEMENT, context->allocator, restArgsArray);
    arg->SetRange(restArgsArray->Range());
    return RebuildCallExpression(context, callExpr, signature, arg);
}

static bool IsInsideSyntheticFunction(ir::CallExpression *callExpr)
{
    // Check if the call is inside a synthetic function (proxy method)
    for (ir::AstNode *curr = callExpr->Parent(); curr != nullptr; curr = curr->Parent()) {
        if (curr->IsScriptFunction()) {
            auto *scriptFunc = curr->AsScriptFunction();
            if ((scriptFunc->Flags() & ir::ScriptFunctionFlags::SYNTHETIC) != 0) {
                return true;
            }
        }
    }
    return false;
}

static bool ShouldTransformCallWithSpreadTuple(ir::CallExpression *callExpr)
{
    if (IsInsideSyntheticFunction(callExpr)) {
        return false;
    }

    auto calleeType = callExpr->Callee()->TsType();
    if (calleeType->IsETSArrowType()) {
        return false;
    }

    if (callExpr->Signature() == nullptr || !callExpr->Signature()->HasRestParameter()) {
        return false;
    }

    auto *restVar = callExpr->Signature()->RestVar();
    if (restVar == nullptr || !restVar->TsType()->IsETSTupleType()) {
        return false;
    }

    const auto &args = callExpr->Arguments();
    return args.size() == callExpr->Signature()->Params().size() + 1;
}

static ir::BlockExpression *CreateTupleRestArgsBlockExpression(public_lib::Context *context,
                                                               ir::SpreadElement *spreadElement,
                                                               checker::Signature *signature)
{
    auto *allocator = context->allocator;
    auto *parser = context->parser->AsETSParser();
    auto *checker = context->GetChecker()->AsETSChecker();

    auto *restVar = signature->RestVar();
    auto *tupleType = restVar->TsType()->AsETSTupleType();
    auto *spreadArg = spreadElement->Argument();

    // Save original tuple to variable to avoid repeated evaluation
    auto *tupleVar = Gensym(allocator);

    ArenaVector<ir::Statement *> blockStatements(allocator->Adapter());
    blockStatements.push_back(parser->CreateFormattedStatement("let @@I1= @@E2;", tupleVar, spreadArg));

    // Create new tuple with elements referencing original tuple
    ArenaVector<ir::Expression *> tupleElements(allocator->Adapter());
    for (size_t i = 0; i < tupleType->GetTupleSize(); ++i) {
        auto *elemAccess =
            parser->CreateFormattedExpression("@@I1[" + std::to_string(i) + "]", tupleVar->Clone(allocator, nullptr));
        tupleElements.push_back(elemAccess);
    }
    auto *newTupleExpr = checker->AllocNode<ir::ArrayExpression>(std::move(tupleElements), allocator);

    // Create type annotation for the new tuple from the type
    auto *newTypeAnnotation = checker->AllocNode<ir::OpaqueTypeNode>(tupleType, allocator);
    auto *asExpression = context->AllocNode<ir::TSAsExpression>(newTupleExpr, newTypeAnnotation, false);
    blockStatements.push_back(parser->CreateFormattedStatement("@@E1", asExpression));

    auto *blockExpr = util::NodeAllocator::ForceSetParent<ir::BlockExpression>(allocator, std::move(blockStatements));
    return blockExpr;
}

static ir::CallExpression *TransformCallWithSpreadTuple(public_lib::Context *ctx, ir::CallExpression *callExpr)
{
    auto *allocator = ctx->allocator;
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *signature = callExpr->Signature();
    auto *spreadElement = callExpr->Arguments().back()->AsSpreadElement();

    auto *restArgsBlock = CreateTupleRestArgsBlockExpression(ctx, spreadElement, signature);

    ArenaVector<ir::Expression *> newCallArgs(allocator->Adapter());
    for (size_t i = 0; i < signature->Params().size(); ++i) {
        newCallArgs.push_back(callExpr->Arguments()[i]);
    }

    auto *spreadArg = ctx->AllocNode<ir::SpreadElement>(ir::AstNodeType::SPREAD_ELEMENT, allocator, restArgsBlock);
    newCallArgs.push_back(spreadArg);

    auto *newCall =
        ctx->AllocNode<ir::CallExpression>(callExpr->Callee(), std::move(newCallArgs), callExpr->TypeParams(), false);
    newCall->SetParent(callExpr->Parent());
    newCall->SetRange(callExpr->Range());

    for (auto *arg : newCall->Arguments()) {
        arg->SetParent(newCall);
    }

    auto *scope = NearestScope(newCall->Parent());
    auto bscope =
        varbinder::LexicalScope<varbinder::Scope>::Enter(ctx->GetChecker()->VarBinder()->AsETSBinder(), scope);
    CheckLoweredNode(ctx->GetChecker()->VarBinder()->AsETSBinder(), checker, newCall);

    return newCall;
}

bool RestArgsLowering::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        [ctx = Context()](ir::AstNode *node) -> AstNodePtr {
            if (node->IsCallExpression()) {
                auto *callExpr = node->AsCallExpression();
                if (ShouldTransformCallWithSpreadTuple(callExpr)) {
                    return TransformCallWithSpreadTuple(ctx, callExpr);
                }
                return TransformCallExpressionWithRestArgs(callExpr, ctx);
            }
            if (node->IsETSNewClassInstanceExpression()) {
                return TransformCallConstructWithRestArgs(node->AsETSNewClassInstanceExpression(), ctx);
            }
            return node;
        },
        Name());
    return true;
}
}  // namespace ark::es2panda::compiler
