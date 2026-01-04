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
#include "ir/expressions/arrayExpression.h"
#include "ir/expressions/literals/undefinedLiteral.h"
#include "ir/ts/tsAsExpression.h"
#include <checker/ETSchecker.h>
#include <cstddef>
#include <iterator>
#include <sstream>

namespace ark::es2panda::compiler {

using AstNodePtr = ir::AstNode *;

static ir::BlockExpression *CreateRestArgsBlockExpression(public_lib::Context *context,
                                                          ir::SpreadElement *spreadElement, bool isArrowType)
{
    auto *allocator = context->allocator;
    auto *parser = context->parser->AsETSParser();
    auto *checker = context->GetChecker()->AsETSChecker();

    ArenaVector<ir::Statement *> blockStatements(allocator->Adapter());
    const auto arraySymbol = Gensym(allocator);
    ES2PANDA_ASSERT(arraySymbol != nullptr);
    const auto argumentSymbol = Gensym(allocator);
    ES2PANDA_ASSERT(argumentSymbol != nullptr);
    const auto iteratorIndex = Gensym(allocator);
    ES2PANDA_ASSERT(iteratorIndex != nullptr);
    const auto elementType = checker->GetElementTypeOfArray(spreadElement->Argument()->TsType());
    auto *typeNode = allocator->New<ir::OpaqueTypeNode>(elementType, allocator);
    blockStatements.push_back(
        parser->CreateFormattedStatement("let @@I1 = @@E2;", argumentSymbol, spreadElement->Argument()));
    blockStatements.push_back(parser->CreateFormattedStatement("let @@I1 = 0;", iteratorIndex));
    if (isArrowType) {
        blockStatements.push_back(parser->CreateFormattedStatement("let @@I1: FixedArray<Any> = new Any[@@I2.length];",
                                                                   arraySymbol,
                                                                   argumentSymbol->Clone(allocator, nullptr)));
    } else {
        blockStatements.push_back(parser->CreateFormattedStatement("let @@I1 = new Array<@@T2>(@@I3.length);",
                                                                   arraySymbol, typeNode,
                                                                   argumentSymbol->Clone(allocator, nullptr)));
    }
    std::vector<ir::AstNode *> args;
    std::stringstream ss;
    ss << "for (let @@I1 = 0; @@I2 < @@I3.length; @@I4++){";
    args.emplace_back(iteratorIndex->Clone(allocator, nullptr));
    args.emplace_back(iteratorIndex->Clone(allocator, nullptr));
    args.emplace_back(argumentSymbol->Clone(allocator, nullptr));
    args.emplace_back(iteratorIndex->Clone(allocator, nullptr));
    auto *arraySymbolWithoutTypeAnnotation = arraySymbol->Clone(allocator, nullptr)->AsIdentifier();
    arraySymbolWithoutTypeAnnotation->SetTypeAnnotation(nullptr);
    ss << "@@I5[@@I6] = @@I7[@@I8];";
    args.emplace_back(arraySymbolWithoutTypeAnnotation);
    args.emplace_back(iteratorIndex->Clone(allocator, nullptr));
    args.emplace_back(argumentSymbol->Clone(allocator, nullptr));
    args.emplace_back(iteratorIndex->Clone(allocator, nullptr));
    ss << "}";
    ir::Statement *loopStatement = parser->CreateFormattedStatement(ss.str(), args);

    blockStatements.push_back(loopStatement);
    blockStatements.push_back(
        parser->CreateFormattedStatement("@@I1", arraySymbolWithoutTypeAnnotation->Clone(allocator, nullptr)));
    auto *blockExpr = util::NodeAllocator::ForceSetParent<ir::BlockExpression>(allocator, std::move(blockStatements));
    return blockExpr;
}

static ir::BlockExpression *ConvertSpreadToBlockExpression(public_lib::Context *context,
                                                           ir::SpreadElement *spreadElement, bool isArrowType)
{
    auto *blockExpression = CreateRestArgsBlockExpression(context, spreadElement, isArrowType);
    ES2PANDA_ASSERT(blockExpression != nullptr);
    blockExpression->SetParent(spreadElement->Parent());
    blockExpression->SetRange(spreadElement->Range());

    for (auto *statement : blockExpression->Statements()) {
        SetSourceRangesRecursively(statement, spreadElement->Range());
    }
    return blockExpression;
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

    bool isRestVarArrayType = restVarType->IsETSArrayType();
    bool isRestVarTupleType = restVarType->IsETSTupleType();
    if (isRestVarArrayType || isRestVarTupleType) {
        return false;
    }

    bool hasEnoughArguments = arguments.size() >= signature->Params().size();
    bool shouldSkipParamCheck = ShouldSkipParamCheck(signature, arguments);
    if (!hasEnoughArguments && !shouldSkipParamCheck) {
        return false;
    }

    return calleeType->IsETSArrowType() || !signature->Function()->IsDynamic();
}

static ir::Expression *CreateRestArgsArray(public_lib::Context *context, ArenaVector<ir::Expression *> &arguments,
                                           checker::Signature *signature, bool isArrowType = false)
{
    auto *allocator = context->allocator;
    auto *parser = context->parser->AsETSParser();
    auto *checker = context->GetChecker()->AsETSChecker();

    // Handle single spread element case
    const int diffArgs = arguments.size() - signature->Params().size();
    const size_t extraArgs = diffArgs < 0 ? 0 : diffArgs;
    if (extraArgs == 1 && arguments.back()->IsSpreadElement()) {
        return ConvertSpreadToBlockExpression(context, arguments.back()->AsSpreadElement(), isArrowType);
    }
    // Determine array type
    checker::Type *arrayType = signature->RestVar()->TsType();
    auto *type = checker->AllocNode<ir::OpaqueTypeNode>(checker->GetElementTypeOfArray(arrayType), allocator);
    if (extraArgs == 0) {
        return parser->CreateFormattedExpression("new Array<@@T1>(0)", type);
    }

    ArenaVector<ir::Expression *> copiedArguments(arguments.begin() + signature->Params().size(), arguments.end(),
                                                  allocator->Adapter());

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
    ES2PANDA_ASSERT(type != nullptr);
    auto *loweringResult = parser->CreateFormattedExpression(
        ss.str(), genSymIdent, type->Clone(allocator, nullptr), arrayExpr, genSymIdent2, type,
        type->Clone(allocator, nullptr), genSymIdent->Clone(allocator, nullptr), genSymIdent->Clone(allocator, nullptr),
        genSymIdent2->Clone(allocator, nullptr), genSymIdent->Clone(allocator, nullptr),
        genSymIdent2->Clone(allocator, nullptr));
    loweringResult->SetRange({arguments[signature->Params().size()]->Range().start, arguments.back()->Range().end});
    return loweringResult;
}

static ir::CallExpression *RebuildCallExpression(public_lib::Context *context, ir::CallExpression *originalCall,
                                                 checker::Signature *signature, ir::Expression *restArgsArray)

{
    if (originalCall->Callee()->TsType()->IsETSArrowType()) {
        restArgsArray->AddAstNodeFlags(ir::AstNodeFlags::REST_ARGUMENT);
    }

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

    restArgsArray->SetParent(newCall);
    newCall->SetParent(originalCall->Parent());
    newCall->AddModifier(originalCall->Modifiers());
    auto *scope = NearestScope(newCall->Parent());
    auto bscope =
        varbinder::LexicalScope<varbinder::Scope>::Enter(context->GetChecker()->VarBinder()->AsETSBinder(), scope);
    CheckLoweredNode(context->GetChecker()->VarBinder()->AsETSBinder(), context->GetChecker()->AsETSChecker(), newCall);
    return newCall;
}

ir::ETSNewClassInstanceExpression *RestArgsLowering::TransformCallConstructWithRestArgs(
    ir::ETSNewClassInstanceExpression *expr, public_lib::Context *context)
{
    checker::Signature *signature = expr->GetSignature();
    if (!ShouldProcessRestParameters(signature, expr->GetTypeRef()->TsType(), expr->GetArguments())) {
        return expr;
    }

    auto *restArgsArray = CreateRestArgsArray(context, expr->GetArguments(), signature);
    auto arg =
        context->AllocNode<ir::SpreadElement>(ir::AstNodeType::SPREAD_ELEMENT, context->allocator, restArgsArray);
    return RebuildNewClassInstanceExpression(context, expr, signature, arg);
}

ir::CallExpression *RestArgsLowering::TransformCallExpressionWithRestArgs(ir::CallExpression *callExpr,
                                                                          public_lib::Context *context)
{
    checker::Type *calleeType = callExpr->Callee()->TsType();
    if (calleeType == nullptr) {
        return callExpr;
    }

    bool isArrowType = calleeType->IsETSArrowType();
    checker::Signature *signature = callExpr->Signature();
    if (signature == nullptr || (!isArrowType && signature->Function()->IsDynamic())) {
        return callExpr;
    }

    bool hasSpreadArg = !callExpr->Arguments().empty() && callExpr->Arguments().back()->IsSpreadElement();
    if (isArrowType && !hasSpreadArg) {
        return callExpr;
    }

    if (!ShouldProcessRestParameters(signature, calleeType, callExpr->Arguments())) {
        return callExpr;
    }

    auto *restArgsArray = CreateRestArgsArray(context, callExpr->Arguments(), signature, calleeType->IsETSArrowType());
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
    if (args.size() != callExpr->Signature()->Params().size() + 1) {
        return false;
    }

    return true;
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

    auto *newCall = ctx->AllocNode<ir::CallExpression>(callExpr->Callee(), std::move(newCallArgs),
                                                       std::move(callExpr->TypeParams()), false);
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

bool RestArgsLowering::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        [this, ctx](ir::AstNode *node) -> AstNodePtr {
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
