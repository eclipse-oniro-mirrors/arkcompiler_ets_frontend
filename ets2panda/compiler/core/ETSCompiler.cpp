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

#include "ETSCompiler.h"

#include "compiler/base/catchTable.h"
#include "compiler/base/condition.h"
#include "compiler/base/lreference.h"
#include "compiler/core/switchBuilder.h"
#include "compiler/core/targetTypeContext.h"
#include "compiler/core/vReg.h"
#include "compiler/function/functionBuilder.h"
#include "checker/ETSchecker.h"
#include "checker/types/ets/etsTupleType.h"
#include "ETSGen-inl.h"
#include "generated/signatures.h"
#include "util/es2pandaMacros.h"
#include "varbinder/ETSBinder.h"

namespace ark::es2panda::compiler {

ETSGen *ETSCompiler::GetETSGen() const
{
    return static_cast<ETSGen *>(GetCodeGen());
}

void ETSCompiler::Compile(const ir::CatchClause *st) const
{
    ETSGen *etsg = GetETSGen();
    compiler::LocalRegScope lrs(etsg, st->Scope()->ParamScope());
    etsg->SetAccumulatorType(st->TsType());
    auto lref = compiler::ETSLReference::Create(etsg, st->Param(), true);
    lref.SetValue();
    st->Body()->Compile(etsg);
}

void ETSCompiler::Compile(const ir::ClassProperty *st) const
{
    ETSGen *etsg = GetETSGen();
    if (st->Value() == nullptr && st->TsType()->IsETSPrimitiveType()) {
        return;
    }

    auto ttctx = compiler::TargetTypeContext(etsg, st->TsType());
    compiler::RegScope rs(etsg);

    if (st->Value() == nullptr) {
        etsg->LoadDefaultValue(st, st->TsType());
    } else {
        st->Value()->Compile(etsg);
        if (st->Value()->TsType()->IsETSNeverType()) {
            return;
        }
        etsg->ApplyConversion(st->Value(), st->TsType());
    }

    auto fullName = etsg->FormClassOwnPropReference(etsg->ContainingObjectType(), st->Key()->AsIdentifier()->Name());
    if (st->IsStatic()) {
        etsg->StoreStaticProperty(st, st->TsType(), fullName);
    } else {
        etsg->StoreProperty(st, st->TsType(), etsg->GetThisReg(), fullName);
    }
}

void ETSCompiler::Compile(const ir::TemplateElement *expr) const
{
    ETSGen *etsg = GetETSGen();
    etsg->LoadAccumulatorString(expr, expr->Cooked());
    etsg->SetAccumulatorType(expr->TsType());
    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

void ETSCompiler::Compile(const ir::ETSClassLiteral *expr) const
{
    ETSGen *etsg = GetETSGen();

    auto *literal = expr->Expr();
    auto *literalType = literal->TsType();

    bool const isPrimitive = !literalType->IsETSReferenceType();
    if (!isPrimitive) {
        literal->Compile(etsg);
    } else {
        ES2PANDA_ASSERT(literalType->IsETSPrimitiveType());
        etsg->SetAccumulatorType(literalType);
    }

    etsg->EmitLdaType(expr, etsg->GetAccumulatorType()->AsETSObjectType()->AssemblerName());
    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSIntrinsicNode *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile(const ir::ETSFunctionType *node) const
{
    ETSGen *etsg = GetETSGen();
    etsg->LoadAccumulatorPoison(node, node->TsType());
}

void ETSCompiler::Compile(const ir::ETSNewArrayInstanceExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    auto const checker = const_cast<checker::ETSChecker *>(etsg->Checker());
    compiler::RegScope rs(etsg);
    compiler::TargetTypeContext ttctx(etsg, checker->GlobalIntType());

    expr->Dimension()->Compile(etsg);

    compiler::VReg arr = etsg->AllocReg();
    compiler::VReg dim = etsg->AllocReg();
    etsg->ApplyConversionAndStoreAccumulator(expr, dim, expr->Dimension()->TsType());
    etsg->NewArray(expr, arr, dim, expr->TsType());

    const auto *elementType = expr->TypeReference()->TsType();
    const bool undefAssignable = checker->Relation()->IsSupertypeOf(elementType, checker->GlobalETSUndefinedType());
    if (elementType->IsETSPrimitiveType() || undefAssignable) {
        // no-op
    } else {
        compiler::VReg countReg = etsg->AllocReg();
        auto *startLabel = etsg->AllocLabel();
        auto *endLabel = etsg->AllocLabel();
        etsg->MoveImmediateToRegister(expr, countReg, checker::TypeFlag::INT, static_cast<std::int32_t>(0));
        const auto indexReg = etsg->AllocReg();

        etsg->SetLabel(expr, startLabel);
        etsg->LoadAccumulator(expr, dim);
        etsg->JumpCompareRegister<compiler::Jle>(expr, countReg, endLabel);

        etsg->LoadAccumulator(expr, countReg);
        etsg->StoreAccumulator(expr, indexReg);

        if (expr->Signature() != nullptr) {
            const compiler::TargetTypeContext ttctx2(etsg, elementType);
            static const ArenaVector<ir::Expression *> ARGUMENTS(GetCodeGen()->Allocator()->Adapter());
            etsg->InitObject(expr, expr->Signature(), ARGUMENTS);
        } else {
            etsg->LoadAccumulatorPoison(expr, elementType);
        }
        etsg->StoreArrayElement(expr, arr, indexReg, elementType);

        etsg->IncrementImmediateRegister(expr, countReg, checker::TypeFlag::INT, static_cast<std::int32_t>(1));
        etsg->JumpTo(expr, startLabel);
        etsg->SetLabel(expr, endLabel);
    }

    etsg->SetVRegType(arr, expr->TsType());
    etsg->LoadAccumulator(expr, arr);

    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

static void ConvertRestArguments(checker::ETSChecker *const checker, const ir::ETSNewClassInstanceExpression *expr)
{
    if (expr->GetSignature()->RestVar() != nullptr && (expr->GetSignature()->RestVar()->TsType()->IsETSArrayType() ||
                                                       expr->GetSignature()->RestVar()->TsType()->IsETSTupleType())) {
        std::size_t const argumentCount = expr->GetArguments().size();
        std::size_t const parameterCount = expr->GetSignature()->Params().size();
        ES2PANDA_ASSERT(argumentCount >= parameterCount);

        auto &arguments = const_cast<ArenaVector<ir::Expression *> &>(expr->GetArguments());
        std::size_t i = parameterCount;

        if (i < argumentCount && expr->GetArguments()[i]->IsSpreadElement()) {
            arguments[i] = expr->GetArguments()[i]->AsSpreadElement()->Argument();
        } else if (!expr->GetSignature()->RestVar()->TsType()->IsETSTupleType()) {
            ArenaVector<ir::Expression *> elements(checker->Allocator()->Adapter());
            for (; i < argumentCount; ++i) {
                elements.emplace_back(expr->GetArguments()[i]);
            }
            auto *arrayExpression = checker->AllocNode<ir::ArrayExpression>(std::move(elements), checker->Allocator());
            ES2PANDA_ASSERT(arrayExpression != nullptr);
            arrayExpression->SetParent(const_cast<ir::ETSNewClassInstanceExpression *>(expr));
            auto restType = expr->GetSignature()->RestVar()->TsType()->AsETSArrayType();
            arrayExpression->SetTsType(restType);
            arrayExpression->SetPreferredType(restType->ElementType());
            arguments.erase(expr->GetArguments().begin() + parameterCount, expr->GetArguments().end());
            arguments.emplace_back(arrayExpression);
        }
    }
}

static void HandleUnionTypeInForOf(compiler::ETSGen *etsg, checker::Type const *const exprType,
                                   const ir::ForOfStatement *st, VReg objReg, VReg *countReg)
{
    std::vector<Label *> labels {};

    for (auto it : exprType->AsETSUnionType()->ConstituentTypes()) {
        labels.push_back(etsg->AllocLabel());
        etsg->LoadAccumulator(st->Right(), objReg);
        etsg->IsInstance(st->Right(), objReg, it);
        etsg->BranchIfTrue(st, labels.back());
    }

    labels.push_back(etsg->AllocLabel());

    for (size_t i = 0; i < exprType->AsETSUnionType()->ConstituentTypes().size(); i++) {
        compiler::VReg unionReg = etsg->AllocReg();
        auto currentType = exprType->AsETSUnionType()->ConstituentTypes()[i];
        etsg->SetLabel(st->Right(), labels[i]);
        etsg->LoadAccumulator(st, objReg);
        etsg->CastToReftype(st->Right(), currentType, false);
        etsg->StoreAccumulator(st, unionReg);
        etsg->LoadAccumulator(st, unionReg);
        if (countReg == nullptr) {
            if (currentType->IsETSArrayType()) {
                etsg->LoadArrayLength(st, unionReg);
            } else if (currentType->IsETSResizableArrayType()) {
                etsg->LoadResizableArrayLength(st);
            } else {
                etsg->LoadStringLength(st);
            }
        } else {
            if (currentType->IsETSArrayType()) {
                etsg->LoadAccumulator(st, *countReg);
                etsg->LoadArrayElement(st, unionReg);
            } else if (currentType->IsETSResizableArrayType()) {
                etsg->LoadResizableArrayElement(st, unionReg, *countReg);
            } else {
                etsg->LoadStringChar(st, unionReg, *countReg, true);
            }
        }

        if (i + 1 != exprType->AsETSUnionType()->ConstituentTypes().size()) {
            etsg->Branch(st, labels.back());
        }
    }

    etsg->SetLabel(st->Right(), labels.back());
}

static void GetSizeInForOf(compiler::ETSGen *etsg, checker::Type const *const exprType, const ir::ForOfStatement *st,
                           VReg objReg)
{
    if (exprType->IsETSArrayType()) {
        etsg->LoadArrayLength(st, objReg);
    } else if (exprType->IsETSUnionType()) {
        HandleUnionTypeInForOf(etsg, exprType, st, objReg, nullptr);
    } else {
        etsg->LoadStringLength(st);
    }
}

void ETSCompiler::Compile(const ir::ETSNewClassInstanceExpression *expr) const
{
    ETSGen *etsg = GetETSGen();

    if (expr->TsType()->IsETSAnyType()) {
        compiler::RegScope rs(etsg);
        auto objReg = etsg->AllocReg();
        expr->GetTypeRef()->Compile(etsg);
        etsg->StoreAccumulator(expr->GetTypeRef(), objReg);
        etsg->CallAnyNew(expr, Span<ir::Expression const *const>(expr->GetArguments()), objReg);
    } else {
        ConvertRestArguments(const_cast<checker::ETSChecker *>(etsg->Checker()->AsETSChecker()), expr);
        etsg->InitObject(expr, expr->signature_, expr->GetArguments());
    }
    etsg->SetAccumulatorType(expr->TsType());
}

void ETSCompiler::Compile(const ir::ETSNewMultiDimArrayInstanceExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    etsg->InitObject(expr, expr->Signature(), expr->Dimensions());
    etsg->SetAccumulatorType(expr->TsType());
}

void ETSCompiler::Compile(const ir::ETSParameterExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    expr->Ident()->Compile(etsg);

    if (auto *const paramType = expr->TsType();
        !etsg->Checker()->AsETSChecker()->Relation()->IsIdenticalTo(paramType, etsg->GetAccumulatorType())) {
        etsg->SetAccumulatorType(paramType);
    }

    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

void ETSCompiler::Compile(const ir::ETSTypeReference *node) const
{
    ETSGen *etsg = GetETSGen();
    node->Part()->Compile(etsg);
    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), node->TsType()));
}

void ETSCompiler::Compile(const ir::ETSTypeReferencePart *node) const
{
    ETSGen *etsg = GetETSGen();
    node->Name()->Compile(etsg);
    if (node->TypeParams() != nullptr) {
        // Setting accumulator to exact type
        etsg->SetAccumulatorType(node->TsType());
    }
    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), node->TsType()));
}

void ETSCompiler::Compile(const ir::OpaqueTypeNode *node) const
{
    GetETSGen()->SetAccumulatorType(node->TsType());
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ETSWildcardType *node) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::CompileTupleCreation(const ir::ArrayExpression *tupleInitializer) const
{
    ETSGen *etsg = GetETSGen();

    etsg->InitObject(tupleInitializer,
                     tupleInitializer->TsType()->AsETSTupleType()->GetWrapperType()->ConstructSignatures().front(),
                     tupleInitializer->Elements());
    etsg->SetAccumulatorType(tupleInitializer->TsType());
}

void ETSCompiler::CompileArrayCreation(const ir::ArrayExpression *expr) const
{
    ETSGen *etsg = GetETSGen();

    const auto arr = etsg->AllocReg();
    const auto dim = etsg->AllocReg();

    const auto *const arrayExprType = expr->TsType();

    const compiler::TargetTypeContext ttctx(etsg, etsg->Checker()->GlobalIntType());
    etsg->LoadAccumulatorInt(expr, static_cast<std::int32_t>(expr->Elements().size()));
    etsg->StoreAccumulator(expr, dim);
    etsg->NewArray(expr, arr, dim, expr->TsType());

    const auto indexReg = etsg->AllocReg();
    auto const *const elementType = arrayExprType->AsETSArrayType()->ElementType();

    for (std::uint32_t i = 0; i < expr->Elements().size(); ++i) {
        const auto *const expression = expr->Elements()[i];
        etsg->LoadAccumulatorInt(expr, i);
        etsg->StoreAccumulator(expr, indexReg);

        const compiler::TargetTypeContext ttctx2(etsg, elementType);
        expression->Compile(etsg);
        etsg->ApplyConversion(expression, elementType);

        if (expression->TsType()->IsETSArrayType()) {
            etsg->StoreArrayElement(expr, arr, indexReg, expression->TsType());
        } else {
            etsg->StoreArrayElement(expr, arr, indexReg, arrayExprType->AsETSArrayType()->ElementType());
        }
    }

    etsg->LoadAccumulator(expr, arr);
    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), arrayExprType));
}

void ETSCompiler::Compile(const ir::ArrayExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    const compiler::RegScope rs(etsg);

    if (expr->TsType()->IsETSTupleType()) {
        CompileTupleCreation(expr);
    } else {
        CompileArrayCreation(expr);
    }
}

void ETSCompiler::Compile(const ir::AssignmentExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    // All other operations are handled in OpAssignmentLowering
    ES2PANDA_ASSERT(expr->OperatorType() == lexer::TokenType::PUNCTUATOR_SUBSTITUTION);
    const auto *const exprType = expr->TsType();

    compiler::RegScope rs(etsg);
    auto lref = compiler::ETSLReference::Create(etsg, expr->Left(), false);
    auto ttctx = compiler::TargetTypeContext(etsg, exprType);

    expr->Right()->Compile(etsg);
    etsg->ApplyConversion(expr->Right(), exprType);
    etsg->SetAccumulatorType(exprType);

    if (expr->Right()->TsType()->IsETSNeverType()) {
        return;
    }

    if (expr->Right()->TsType()->IsETSBigIntType()) {
        // For bigints we have to copy the bigint object when performing an assignment operation
        const VReg value = etsg->AllocReg();
        etsg->StoreAccumulator(expr, value);
        etsg->CreateBigIntObject(expr, value, Signatures::BUILTIN_BIGINT_CTOR_BIGINT);
    }

    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), exprType));
    lref.SetValue();
}

void ETSCompiler::Compile(const ir::AwaitExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    static constexpr bool IS_UNCHECKED_CAST = false;
    compiler::RegScope rs(etsg);
    compiler::VReg argumentReg = etsg->AllocReg();
    expr->Argument()->Compile(etsg);
    etsg->StoreAccumulator(expr, argumentReg);
    etsg->CallVirtual(expr->Argument(), compiler::Signatures::BUILTIN_PROMISE_AWAIT_RESOLUTION, argumentReg);
    etsg->CastToReftype(expr->Argument(), expr->TsType(), IS_UNCHECKED_CAST);
    etsg->SetAccumulatorType(expr->TsType());
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ImportExpression *expr) const
{
    ES2PANDA_UNREACHABLE();
}

static void CompileNullishCoalescing(compiler::ETSGen *etsg, ir::BinaryExpression const *const node)
{
    auto const compileOperand = [etsg, optype = node->OperationType()](ir::Expression const *expr) {
        etsg->CompileAndCheck(expr);
        etsg->ApplyConversion(expr, nullptr);
    };

    compileOperand(node->Left());

    if (node->Left()->TsType()->DefinitelyNotETSNullish()) {
        // fallthrough
    } else if (node->Left()->TsType()->DefinitelyETSNullish()) {
        compileOperand(node->Right());
    } else {
        auto *ifLeftNullish = etsg->AllocLabel();
        auto *endLabel = etsg->AllocLabel();

        etsg->BranchIfNullish(node, ifLeftNullish);

        etsg->AssumeNonNullish(node, node->OperationType());
        etsg->ApplyConversion(node->Left(), node->OperationType());
        etsg->JumpTo(node, endLabel);

        etsg->SetLabel(node, ifLeftNullish);
        compileOperand(node->Right());

        etsg->SetLabel(node, endLabel);
    }
    etsg->SetAccumulatorType(node->TsType());
}

static void CompileLogical(compiler::ETSGen *etsg, const ir::BinaryExpression *expr)
{
    if (expr->OperatorType() == lexer::TokenType::PUNCTUATOR_NULLISH_COALESCING) {
        CompileNullishCoalescing(etsg, expr);
        return;
    }
    ES2PANDA_ASSERT(expr->IsLogicalExtended());
    // Always compile the left hand side
    etsg->CompileAndCheck(expr->Left());

    // If the Result is given, we can optimize the process.
    if (expr->Result() != nullptr) {
        if (expr->Result() != expr->Left()) {
            ES2PANDA_ASSERT(expr->Result() == expr->Right());
            expr->Result()->Compile(etsg);
        }
        etsg->ApplyConversion(expr->Result(), expr->OperationType());
        etsg->ApplyConversion(expr, expr->TsType());
        etsg->SetAccumulatorType(expr->TsType());
        ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
        return;
    }
    auto ttctx = compiler::TargetTypeContext(etsg, expr->OperationType());
    compiler::RegScope rs(etsg);
    auto endValue = etsg->AllocReg();
    auto orgValue = etsg->AllocReg();

    etsg->StoreAccumulator(expr->Left(), orgValue);
    etsg->ApplyConversionAndStoreAccumulator(expr->Left(), endValue, expr->OperationType());
    auto *endLabel = etsg->AllocLabel();

    etsg->LoadAccumulator(expr, orgValue);
    if (expr->OperatorType() == lexer::TokenType::PUNCTUATOR_LOGICAL_AND) {
        etsg->BranchConditionalIfFalse(expr->Left(), endLabel);
    } else {
        ES2PANDA_ASSERT(expr->OperatorType() == lexer::TokenType::PUNCTUATOR_LOGICAL_OR);
        etsg->BranchConditionalIfTrue(expr->Left(), endLabel);
    }

    etsg->CompileAndCheck(expr->Right());
    etsg->ApplyConversionAndStoreAccumulator(expr->Right(), endValue, expr->OperationType());
    etsg->SetLabel(expr, endLabel);
    etsg->LoadAccumulator(expr, endValue);
    etsg->ApplyConversion(expr, expr->TsType());

    etsg->SetAccumulatorType(expr->TsType());
}

static void CompileInstanceof(compiler::ETSGen *etsg, const ir::BinaryExpression *expr)
{
    ES2PANDA_ASSERT(expr->OperatorType() == lexer::TokenType::KEYW_INSTANCEOF);
    auto ttctx = compiler::TargetTypeContext(etsg, expr->OperationType());
    compiler::RegScope rs(etsg);
    auto lhs = etsg->AllocReg();

    expr->Left()->Compile(etsg);
    etsg->ApplyConversionAndStoreAccumulator(expr->Left(), lhs, expr->OperationType());

    etsg->IsInstance(expr, lhs, expr->Right()->TsType());
    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

std::map<lexer::TokenType, std::string_view> &GetBigintSignatures()
{
    static std::map<lexer::TokenType, std::string_view> bigintSignatures = {
        {lexer::TokenType::PUNCTUATOR_PLUS, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_ADD},
        {lexer::TokenType::PUNCTUATOR_MINUS, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_SUBTRACT},
        {lexer::TokenType::PUNCTUATOR_MULTIPLY, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_MULTIPLY},
        {lexer::TokenType::PUNCTUATOR_DIVIDE, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_DIVIDE},
        {lexer::TokenType::PUNCTUATOR_MOD, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_MODULE},
        {lexer::TokenType::PUNCTUATOR_BITWISE_OR, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_BITWISE_OR},
        {lexer::TokenType::PUNCTUATOR_BITWISE_AND, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_BITWISE_AND},
        {lexer::TokenType::PUNCTUATOR_BITWISE_XOR, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_BITWISE_XOR},
        {lexer::TokenType::PUNCTUATOR_LEFT_SHIFT, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_LEFT_SHIFT},
        {lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_RIGHT_SHIFT},
        {lexer::TokenType::PUNCTUATOR_GREATER_THAN, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_GREATER_THAN},
        {lexer::TokenType::PUNCTUATOR_LESS_THAN, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_LESS_THAN},
        {lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL,
         compiler::Signatures::BUILTIN_BIGINT_OPERATOR_GREATER_THAN_EQUAL},
        {lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_LESS_THAN_EQUAL},
        {lexer::TokenType::PUNCTUATOR_EXPONENTIATION, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_EXPONENTIATION},
        {lexer::TokenType::PUNCTUATOR_EXPONENTIATION_EQUAL,
         compiler::Signatures::BUILTIN_BIGINT_OPERATOR_EXPONENTIATION_EQUAL},
    };

    return bigintSignatures;
}

static bool CompileBigInt(compiler::ETSGen *etsg, const ir::BinaryExpression *expr)
{
    if ((expr->Left()->TsType() == nullptr) || (expr->Right()->TsType() == nullptr)) {
        return false;
    }

    if (!expr->Left()->TsType()->IsETSBigIntType()) {
        return false;
    }

    if (!expr->Right()->TsType()->IsETSBigIntType()) {
        return false;
    }

    auto map = GetBigintSignatures();
    if (map.find(expr->OperatorType()) == map.end()) {
        return false;
    }

    const checker::Type *operationType = expr->OperationType();
    auto ttctx = compiler::TargetTypeContext(etsg, operationType);
    compiler::RegScope rs(etsg);
    compiler::VReg lhs = etsg->AllocReg();
    expr->Left()->Compile(etsg);
    etsg->ApplyConversionAndStoreAccumulator(expr->Left(), lhs, operationType);
    expr->Right()->Compile(etsg);
    etsg->ApplyConversion(expr->Right(), operationType);
    compiler::VReg rhs = etsg->AllocReg();
    etsg->StoreAccumulator(expr, rhs);

    std::string_view signature = map.at(expr->OperatorType());
    switch (expr->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN:
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL:
        case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL:
            etsg->CallBigIntBinaryComparison(expr, lhs, rhs, signature);
            break;
        default:
            etsg->CallBigIntBinaryOperator(expr, lhs, rhs, signature);
            break;
    }

    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
    return true;
}

void ETSCompiler::Compile(const ir::BinaryExpression *expr) const
{
    ETSGen *etsg = GetETSGen();

    if (CompileBigInt(etsg, expr)) {
        return;
    }

    if (expr->IsLogical()) {
        CompileLogical(etsg, expr);
        return;
    }
    if (expr->OperatorType() == lexer::TokenType::KEYW_INSTANCEOF) {
        CompileInstanceof(etsg, expr);
        return;
    }

    auto ttctx = compiler::TargetTypeContext(etsg, expr->OperationType());
    compiler::RegScope rs(etsg);
    compiler::VReg lhs = etsg->AllocReg();

    if (expr->OperatorType() == lexer::TokenType::PUNCTUATOR_PLUS && expr->OperationType()->IsETSStringType()) {
        etsg->BuildString(expr, lhs);
        return;
    }

    expr->CompileOperands(etsg, lhs);
    if (expr->OperationType()->IsIntType()) {
        etsg->ApplyCast(expr->Right(), expr->OperationType());
    }

    etsg->Binary(expr, expr->OperatorType(), lhs);
    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

static void ConvertRestArguments(checker::ETSChecker *const checker, const ir::CallExpression *expr,
                                 checker::Signature *signature)
{
    if (signature->RestVar() != nullptr &&
        (signature->RestVar()->TsType()->IsETSArrayType() || signature->RestVar()->TsType()->IsETSTupleType())) {
        std::size_t const argumentCount = expr->Arguments().size();
        std::size_t const parameterCount = signature->Params().size();
        ES2PANDA_ASSERT(argumentCount >= parameterCount);

        auto &arguments = const_cast<ArenaVector<ir::Expression *> &>(expr->Arguments());
        std::size_t i = parameterCount;

        if (i < argumentCount && expr->Arguments()[i]->IsSpreadElement()) {
            arguments[i] = expr->Arguments()[i]->AsSpreadElement()->Argument();
        } else if (i < argumentCount && expr->Arguments()[i]->IsTSAsExpression() &&
                   expr->Arguments()[i]->AsTSAsExpression()->Expr()->Type() == ir::AstNodeType::SPREAD_ELEMENT) {
            arguments[i] = expr->Arguments()[i]->AsTSAsExpression()->Expr()->AsSpreadElement()->Argument();
        } else if (!signature->RestVar()->TsType()->IsETSTupleType()) {
            ArenaVector<ir::Expression *> elements(checker->Allocator()->Adapter());
            for (; i < argumentCount; ++i) {
                elements.emplace_back(expr->Arguments()[i]);
            }
            auto *arrayExpression = checker->AllocNode<ir::ArrayExpression>(std::move(elements), checker->Allocator());
            ES2PANDA_ASSERT(arrayExpression != nullptr);
            arrayExpression->SetParent(const_cast<ir::CallExpression *>(expr));
            auto restType = signature->RestVar()->TsType()->AsETSArrayType();
            arrayExpression->SetTsType(restType);
            arrayExpression->SetPreferredType(restType->ElementType());
            arguments.erase(expr->Arguments().begin() + parameterCount, expr->Arguments().end());
            arguments.emplace_back(arrayExpression);
        }
    }
}

void ETSCompiler::Compile(const ir::BlockExpression *expr) const
{
    ETSGen *etsg = GetETSGen();

    // Nasty hack: current sccope may not be expr's parent scope.
    // For example. when expr is a field initializer, the current scope will
    // be a constructor's scope, not the class scope where the field definition resides.
    auto *oldParent = expr->Scope()->Parent();
    expr->Scope()->SetParent(const_cast<varbinder::Scope *>(etsg->Scope()));

    compiler::LocalRegScope lrs(etsg, expr->Scope());

    etsg->CompileStatements(expr->Statements());

    expr->Scope()->SetParent(oldParent);
}

bool IsCastCallName(util::StringView name)
{
    return name == Signatures::BYTE_CAST || name == Signatures::SHORT_CAST || name == Signatures::INT_CAST ||
           name == Signatures::LONG_CAST || name == Signatures::FLOAT_CAST || name == Signatures::DOUBLE_CAST ||
           name == Signatures::CHAR_CAST;
}

bool IsCastCall(checker::Signature *signature)
{
    ES2PANDA_ASSERT(signature->HasSignatureFlag(checker::SignatureFlags::STATIC));
    auto *func = signature->Function();
    return (func->Parent()->Parent()->IsMethodDefinition() && IsCastCallName(func->Id()->Name()) &&
            util::Helpers::ContainingClass(func)->AsETSObjectType()->IsBoxedPrimitive() &&
            (signature->Params().size() == 1) && signature->Params()[0]->TsType()->IsETSPrimitiveType());
}

void ETSCompiler::EmitCall(const ir::CallExpression *expr, compiler::VReg &calleeReg,
                           checker::Signature *signature) const
{
    ETSGen *etsg = GetETSGen();
    if (signature->HasSignatureFlag(checker::SignatureFlags::STATIC)) {
        if (IsCastCall(signature)) {
            ES2PANDA_ASSERT(expr->Arguments().size() == 1);
            auto *param = expr->Arguments()[0];
            param->Compile(etsg);

            const auto *const targetType = etsg->Checker()->GetApparentType(expr->TsType());
            auto ttctx = compiler::TargetTypeContext(etsg, nullptr);
            CompileCastPrimitives(param, targetType);
            return;
        }
        etsg->CallExact(expr, expr->Signature(), expr->Arguments());
    } else if (expr->Callee()->IsMemberExpression()) {
        auto me = expr->Callee()->AsMemberExpression();
        auto obj = me->Object();
        if (obj->IsSuperExpression()) {
            etsg->CallExact(expr, signature, calleeReg, expr->Arguments());
            // NOTE: need to refactor: type of member expression object can be obtained via
            // me->ObjType() or me->Object()->TsType() and they may differ!!!!
        } else if (me->ObjType() == etsg->Checker()->GlobalETSObjectType() &&
                   (etsg->Checker()->GetApparentType(me->Object()->TsType()) != nullptr) &&
                   (etsg->Checker()->GetApparentType(me->Object()->TsType())->IsETSUnionType())) {
            etsg->CallByName(expr, signature, calleeReg, expr->Arguments());
        } else {
            etsg->CallVirtual(expr, signature, calleeReg, expr->Arguments());
        }
    } else {
        etsg->CallVirtual(expr, signature, calleeReg, expr->Arguments());
    }

    etsg->GuardUncheckedType(expr, expr->UncheckedType(), expr->TsType());
}

void ETSCompiler::Compile(const ir::CallExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    compiler::RegScope rs(etsg);
    compiler::VReg calleeReg = etsg->AllocReg();

    auto const callee = expr->Callee();
    checker::Signature *const signature = expr->Signature();

    ES2PANDA_ASSERT(signature != nullptr);

    ES2PANDA_ASSERT(!callee->TsType()->IsETSArrowType());  // should have been lowered

    bool const isStatic = signature->HasSignatureFlag(checker::SignatureFlags::STATIC);

    ConvertRestArguments(const_cast<checker::ETSChecker *>(etsg->Checker()->AsETSChecker()), expr, signature);

    if (callee->IsIdentifier()) {
        if (!isStatic) {
            etsg->LoadThis(expr);
            etsg->StoreAccumulator(expr, calleeReg);
        }
        EmitCall(expr, calleeReg, signature);
    } else if (callee->IsMemberExpression()) {
        if (!isStatic) {
            callee->AsMemberExpression()->Object()->Compile(etsg);
            etsg->StoreAccumulator(expr, calleeReg);
        }
        EmitCall(expr, calleeReg, signature);
    } else if (callee->IsSuperExpression() || callee->IsThisExpression()) {
        ES2PANDA_ASSERT(expr->IsETSConstructorCall());
        callee->Compile(etsg);  // ctor is not a value!
        etsg->StoreAccumulator(expr, calleeReg);
        EmitCall(expr, calleeReg, signature);
    } else {
        ES2PANDA_UNREACHABLE();
    }
}

void ETSCompiler::Compile(const ir::ConditionalExpression *expr) const
{
    ETSGen *etsg = GetETSGen();

    auto *falseLabel = etsg->AllocLabel();
    auto *endLabel = etsg->AllocLabel();

    compiler::RegScope rs(etsg);
    compiler::Condition::Compile(etsg, expr->Test(), falseLabel);

    auto ttctx = compiler::TargetTypeContext(etsg, expr->TsType());

    expr->Consequent()->Compile(etsg);
    etsg->ApplyConversion(expr->Consequent());
    etsg->Branch(expr, endLabel);

    etsg->SetLabel(expr, falseLabel);
    expr->Alternate()->Compile(etsg);
    etsg->ApplyConversion(expr->Alternate());

    etsg->SetLabel(expr, endLabel);
    etsg->ApplyConversion(expr, expr->TsType());
    etsg->SetAccumulatorType(expr->TsType());
}

static bool TypeNeedInternalCheckCast(const checker::Type *type)
{
    return type->IsETSObjectType() || type->IsETSArrayType() || type->IsETSTupleType() || type->IsETSFunctionType();
}

bool ETSCompiler::HandleTopLevelGetter(const ir::Identifier *expr, ETSGen *etsg) const
{
    if (auto const *const variable = expr->Variable(); checker::ETSChecker::IsVariableStatic(variable)) {
        if (auto const *const varType = variable->TsType(); varType->HasTypeFlag(checker::TypeFlag::GETTER_SETTER)) {
            checker::Signature *sig = varType->AsETSFunctionType()->FindGetter();
            ES2PANDA_ASSERT(sig != nullptr);
            etsg->CallExact(expr, sig->InternalName());
            etsg->SetAccumulatorType(expr->TsType());

            ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
            return true;
        }
    }
    return false;
}

void ETSCompiler::Compile(const ir::Identifier *expr) const
{
    ETSGen *etsg = GetETSGen();

    auto const *smartType = expr->TsType();
    ES2PANDA_ASSERT(smartType != nullptr);
    if (smartType->IsETSTypeParameter() || smartType->IsETSPartialTypeParameter() || smartType->IsETSNonNullishType() ||
        smartType->IsETSTypeAliasType()) {
        smartType = etsg->Checker()->GetApparentType(smartType);
    }
    auto ttctx = compiler::TargetTypeContext(etsg, smartType);

    ES2PANDA_ASSERT(expr->Variable() != nullptr);

    // Top-level accessor
    if (HandleTopLevelGetter(expr, etsg)) {
        return;
    }
    if (!expr->Variable()->HasFlag(varbinder::VariableFlags::TYPE_ALIAS)) {
        etsg->LoadVar(expr, expr->Variable());
    }

    if (smartType->IsETSReferenceType()) {
        auto *localVar = expr->Variable()->AsLocalVariable();
        //  In case when smart cast type of identifier differs from initial variable type perform cast if required
        if (!etsg->Checker()->AsETSChecker()->Relation()->IsSupertypeOf(smartType, etsg->GetAccumulatorType())) {
            etsg->CastToReftype(expr, smartType, false);
        } else if (!etsg->Checker()->AsETSChecker()->Relation()->IsIdenticalTo(smartType, localVar->TsType()) &&
                   TypeNeedInternalCheckCast(smartType)) {
            //  All the non-union type that can get their properties by memberExpression should add a check cast while
            //  the `smartType` is not same with the type of its variable.
            etsg->InternalCheckCast(expr, smartType);
        }
    } else if (smartType->IsETSPrimitiveType()) {
        etsg->ApplyConversionCast(expr, smartType);
    }
    etsg->SetAccumulatorType(smartType);
}

bool ETSCompiler::CompileComputed(compiler::ETSGen *etsg, const ir::MemberExpression *expr)
{
    if (!expr->IsComputed()) {
        return false;
    }
    auto *const objectType = etsg->Checker()->GetApparentType(expr->Object()->TsType());

    auto ottctx = compiler::TargetTypeContext(etsg, objectType);
    etsg->CompileAndCheck(expr->Object());

    compiler::VReg objReg = etsg->AllocReg();
    etsg->StoreAccumulator(expr, objReg);

    auto pttctx = compiler::TargetTypeContext(etsg, expr->Property()->TsType());

    etsg->CompileAndCheck(expr->Property());
    etsg->ApplyConversion(expr->Property(), expr->Property()->TsType());

    auto ttctx = compiler::TargetTypeContext(etsg, expr->TsType());

    if (objectType->IsETSTupleType()) {
        ES2PANDA_ASSERT(expr->GetTupleIndexValue(etsg->Checker()).has_value());
        auto indexValue = *expr->GetTupleIndexValue(etsg->Checker());
        auto *tupleElementType = objectType->AsETSTupleType()->GetTypeAtIndex(indexValue);
        etsg->LoadTupleElement(expr, objReg, tupleElementType, indexValue);
    } else {
        ES2PANDA_ASSERT(objectType->IsETSArrayType());
        etsg->LoadArrayElement(expr, objReg);
    }

    etsg->GuardUncheckedType(expr, expr->UncheckedType(), expr->TsType());
    etsg->ApplyConversion(expr);

    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
    return true;
}

void ETSCompiler::Compile(const ir::MemberExpression *expr) const
{
    ETSGen *etsg = GetETSGen();

    compiler::RegScope rs(etsg);

    if (CompileComputed(etsg, expr)) {
        return;
    }

    if (HandleArrayTypeLengthProperty(expr, etsg)) {
        return;
    }

    if (HandleStaticProperties(expr, etsg)) {
        return;
    }

    auto *const objectType = etsg->Checker()->GetApparentType(expr->Object()->TsType());

    auto ottctx = compiler::TargetTypeContext(etsg, expr->Object()->TsType());
    etsg->CompileAndCheck(expr->Object());

    etsg->ApplyConversion(expr->Object());
    compiler::VReg objReg = etsg->AllocReg();
    etsg->StoreAccumulator(expr, objReg);

    auto ttctx = compiler::TargetTypeContext(etsg, expr->TsType());
    ES2PANDA_ASSERT(expr->PropVar()->TsType() != nullptr);
    const checker::Type *const variableType = expr->PropVar()->TsType();
    ES2PANDA_ASSERT(variableType != nullptr);
    if (variableType->HasTypeFlag(checker::TypeFlag::GETTER_SETTER)) {
        if (expr->Object()->IsSuperExpression()) {
            etsg->CallExact(expr, variableType->AsETSFunctionType()->FindGetter()->InternalName(), objReg);
        } else {
            etsg->CallVirtual(expr, variableType->AsETSFunctionType()->FindGetter(), objReg);
        }
    } else if (objectType->IsETSUnionType()) {
        etsg->LoadPropertyByName(expr, objReg, checker::ETSChecker::FormNamedAccessMetadata(expr->PropVar()));
    } else {
        etsg->LoadProperty(expr, variableType, objReg, etsg->FormClassPropReference(expr->PropVar()));
    }
    etsg->GuardUncheckedType(expr, expr->UncheckedType(), expr->TsType());

    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

bool ETSCompiler::HandleArrayTypeLengthProperty(const ir::MemberExpression *expr, ETSGen *etsg) const
{
    auto *const objectType = etsg->Checker()->GetApparentType(expr->Object()->TsType());
    ES2PANDA_ASSERT(objectType != nullptr);
    auto &propName = expr->Property()->AsIdentifier()->Name();
    if (objectType->IsETSArrayType() && propName.Is("length")) {
        auto ottctx = compiler::TargetTypeContext(etsg, objectType);
        etsg->CompileAndCheck(expr->Object());

        compiler::VReg objReg = etsg->AllocReg();
        etsg->StoreAccumulator(expr, objReg);

        auto ttctx = compiler::TargetTypeContext(etsg, expr->TsType());
        etsg->LoadArrayLength(expr, objReg);
        etsg->ApplyConversion(expr, expr->TsType());
        return true;
    }
    return false;
}

bool ETSCompiler::HandleStaticProperties(const ir::MemberExpression *expr, ETSGen *etsg) const
{
    if (auto const *const variable = expr->PropVar(); checker::ETSChecker::IsVariableStatic(variable)) {
        auto ttctx = compiler::TargetTypeContext(etsg, expr->TsType());

        if (auto const *const varType = variable->TsType(); varType->HasTypeFlag(checker::TypeFlag::GETTER_SETTER)) {
            checker::Signature *sig = varType->AsETSFunctionType()->FindGetter();
            ES2PANDA_ASSERT(sig != nullptr);
            etsg->CallExact(expr, sig->InternalName());
            etsg->SetAccumulatorType(expr->TsType());
        } else {
            util::StringView const fullName = etsg->FormClassPropReference(variable);
            etsg->LoadStaticProperty(expr, varType, fullName);
            etsg->ApplyConversion(expr, expr->TsType());
        }

        ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));

        return true;
    }

    return false;
}

void ETSCompiler::Compile(const ir::ObjectExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    compiler::RegScope rs {etsg};
    compiler::VReg objReg = etsg->AllocReg();

    auto *signatureInfo = etsg->Allocator()->New<checker::SignatureInfo>(etsg->Allocator());
    auto *createObjSig = etsg->Allocator()->New<checker::Signature>(signatureInfo, nullptr, nullptr);
    createObjSig->SetInternalName(compiler::Signatures::BUILTIN_JSRUNTIME_CREATE_OBJECT);
    compiler::VReg dummyReg = compiler::VReg::RegStart();
    etsg->CallDynamic(ETSGen::CallDynamicData {expr, dummyReg, dummyReg}, createObjSig,
                      ArenaVector<ir::Expression *>(etsg->Allocator()->Adapter()));

    etsg->SetAccumulatorType(expr->TsType());
    etsg->StoreAccumulator(expr, objReg);

    for (ir::Expression *propExpr : expr->Properties()) {
        ES2PANDA_ASSERT(propExpr->IsProperty());
        ir::Property *prop = propExpr->AsProperty();
        ir::Expression *key = prop->Key();
        ir::Expression *value = prop->Value();

        util::StringView pname;
        if (key->IsStringLiteral()) {
            pname = key->AsStringLiteral()->Str();
        } else if (key->IsIdentifier()) {
            pname = key->AsIdentifier()->Name();
        } else {
            ES2PANDA_UNREACHABLE();
        }

        value->Compile(etsg);
        etsg->ApplyConversion(value, key->TsType());
        etsg->StoreProperty(expr, key->TsType(), objReg, etsg->FormClassPropReference(prop->Variable()));
    }

    etsg->LoadAccumulator(expr, objReg);
    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

void ETSCompiler::Compile(const ir::SequenceExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    for (const auto *it : expr->Sequence()) {
        it->Compile(etsg);
    }
}

void ETSCompiler::Compile(const ir::SuperExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    etsg->LoadThis(expr);
    etsg->SetAccumulatorType(etsg->GetAccumulatorType()->AsETSObjectType()->SuperType());
    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

void ETSCompiler::Compile(const ir::TemplateLiteral *expr) const
{
    ETSGen *etsg = GetETSGen();
    etsg->BuildTemplateString(expr);
    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

void ETSCompiler::Compile(const ir::ThisExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    etsg->LoadThis(expr);
    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TypeofExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    ir::Expression *arg = expr->Argument();
    arg->Compile(etsg);
    if (expr->TsType()->IsETSStringType() && expr->TsType()->HasTypeFlag(checker::TypeFlag::CONSTANT)) {
        etsg->LoadAccumulatorString(expr, expr->TsType()->AsETSStringType()->GetValue());
        return;
    }
    auto argReg = etsg->AllocReg();
    etsg->StoreAccumulator(expr, argReg);
    etsg->EmitEtsTypeof(expr, argReg);
    etsg->SetAccumulatorType(expr->TsType());
}

void ETSCompiler::Compile(const ir::UnaryExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    auto ttctx = compiler::TargetTypeContext(etsg, expr->Argument()->TsType());

    expr->Argument()->Compile(etsg);
    etsg->ApplyConversion(expr->Argument(), expr->Argument()->TsType());

    etsg->Unary(expr, expr->OperatorType());
    etsg->ApplyConversion(expr, expr->TsType());

    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

void ETSCompiler::Compile([[maybe_unused]] const ir::BigIntLiteral *expr) const
{
    ETSGen *etsg = GetETSGen();
    compiler::TargetTypeContext ttctx = compiler::TargetTypeContext(etsg, expr->TsType());
    compiler::RegScope rs {etsg};
    etsg->LoadAccumulatorBigInt(expr, expr->Str());
    const compiler::VReg value = etsg->AllocReg();
    etsg->StoreAccumulator(expr, value);
    etsg->CreateBigIntObject(expr, value);
    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

void ETSCompiler::Compile(const ir::BooleanLiteral *expr) const
{
    ETSGen *etsg = GetETSGen();
    etsg->LoadAccumulatorBoolean(expr, expr->Value());
    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

void ETSCompiler::Compile(const ir::CharLiteral *expr) const
{
    ETSGen *etsg = GetETSGen();
    etsg->LoadAccumulatorChar(expr, expr->Char());
    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

void ETSCompiler::Compile(const ir::NullLiteral *expr) const
{
    ETSGen *etsg = GetETSGen();
    etsg->LoadAccumulatorNull(expr);
}

void ETSCompiler::Compile(const ir::NumberLiteral *expr) const
{
    ETSGen *etsg = GetETSGen();
    auto ttctx = compiler::TargetTypeContext(etsg, expr->TsType());

    if (expr->Number().IsInt()) {
        if (util::Helpers::IsTargetFitInSourceRange<checker::ByteType::UType, checker::IntType::UType>(
                expr->Number().GetInt())) {  // CC-OFF(G.FMT.06-CPP) project code style
            etsg->LoadAccumulatorByte(expr, static_cast<int8_t>(expr->Number().GetInt()));
        } else if (util::Helpers::IsTargetFitInSourceRange<checker::ShortType::UType, checker::IntType::UType>(
                       expr->Number().GetInt())) {  // CC-OFF(G.FMT.06-CPP) project code style
            etsg->LoadAccumulatorShort(expr, static_cast<int16_t>(expr->Number().GetInt()));
        } else {
            etsg->LoadAccumulatorInt(expr, static_cast<int32_t>(expr->Number().GetInt()));
        }
    } else if (expr->Number().IsLong()) {
        etsg->LoadAccumulatorWideInt(expr, expr->Number().GetLong());
    } else if (expr->Number().IsFloat()) {
        etsg->LoadAccumulatorFloat(expr, expr->Number().GetFloat());
    } else {
        etsg->LoadAccumulatorDouble(expr, expr->Number().GetDouble());
    }

    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->TsType()));
}

void ETSCompiler::Compile(const ir::StringLiteral *expr) const
{
    ETSGen *etsg = GetETSGen();
    etsg->LoadAccumulatorString(expr, expr->Str());
    etsg->SetAccumulatorType(expr->TsType());
}

void ETSCompiler::Compile([[maybe_unused]] const ir::AssertStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

void ETSCompiler::Compile(const ir::BlockStatement *st) const
{
    ETSGen *etsg = GetETSGen();
    compiler::LocalRegScope lrs(etsg, st->Scope());

    etsg->CompileStatements(st->Statements());
}

template <typename CodeGen>
static void CompileImpl(const ir::BreakStatement *self, [[maybe_unused]] CodeGen *cg)
{
    compiler::Label *target = cg->ControlFlowChangeBreak(self->Ident());
    cg->Branch(self, target);
}

void ETSCompiler::Compile(const ir::BreakStatement *st) const
{
    ETSGen *etsg = GetETSGen();
    if (etsg->ExtendWithFinalizer(st->Parent(), st)) {
        return;
    }
    CompileImpl(st, etsg);
}

void ETSCompiler::Compile([[maybe_unused]] const ir::ClassDeclaration *st) const {}
void ETSCompiler::Compile([[maybe_unused]] const ir::AnnotationDeclaration *st) const {}
void ETSCompiler::Compile([[maybe_unused]] const ir::AnnotationUsage *st) const {}

static void CompileImpl(const ir::ContinueStatement *self, ETSGen *etsg)
{
    compiler::Label *target = etsg->ControlFlowChangeContinue(self->Ident());
    etsg->Branch(self, target);
}

void ETSCompiler::Compile(const ir::ContinueStatement *st) const
{
    ETSGen *etsg = GetETSGen();
    if (etsg->ExtendWithFinalizer(st->Parent(), st)) {
        return;
    }
    CompileImpl(st, etsg);
}

void CompileImpl(const ir::DoWhileStatement *self, ETSGen *etsg)
{
    auto *startLabel = etsg->AllocLabel();
    compiler::LabelTarget labelTarget(etsg);

    etsg->SetLabel(self, startLabel);

    {
        compiler::LocalRegScope regScope(etsg, self->Scope());
        compiler::LabelContext labelCtx(etsg, labelTarget);
        self->Body()->Compile(etsg);
    }

    etsg->SetLabel(self, labelTarget.ContinueTarget());
    compiler::Condition::Compile(etsg, self->Test(), labelTarget.BreakTarget());

    etsg->Branch(self, startLabel);
    etsg->SetLabel(self, labelTarget.BreakTarget());
}

void ETSCompiler::Compile(const ir::DoWhileStatement *st) const
{
    ETSGen *etsg = GetETSGen();
    CompileImpl(st, etsg);
}

void ETSCompiler::Compile([[maybe_unused]] const ir::EmptyStatement *st) const {}

void ETSCompiler::Compile(const ir::ExpressionStatement *st) const
{
    ETSGen *etsg = GetETSGen();
    st->GetExpression()->Compile(etsg);
}

void ETSCompiler::Compile(const ir::ForOfStatement *st) const
{
    ETSGen *etsg = GetETSGen();
    compiler::LocalRegScope declRegScope(etsg, st->Scope()->DeclScope()->InitScope());

    checker::Type const *const exprType = st->Right()->TsType();
    ES2PANDA_ASSERT(exprType->IsETSResizableArrayType() || exprType->IsETSArrayType() || exprType->IsETSStringType() ||
                    exprType->IsETSUnionType());

    st->Right()->Compile(etsg);
    compiler::VReg objReg = etsg->AllocReg();
    etsg->StoreAccumulator(st, objReg);

    GetSizeInForOf(etsg, exprType, st, objReg);

    compiler::VReg sizeReg = etsg->AllocReg();
    etsg->StoreAccumulator(st, sizeReg);

    compiler::LabelTarget labelTarget(etsg);
    auto labelCtx = compiler::LabelContext(etsg, labelTarget);

    etsg->BranchIfFalse(st, labelTarget.BreakTarget());

    compiler::VReg countReg = etsg->AllocReg();
    etsg->MoveImmediateToRegister(st, countReg, checker::TypeFlag::INT, static_cast<std::int32_t>(0));
    etsg->LoadAccumulatorInt(st, static_cast<std::int32_t>(0));

    auto *const startLabel = etsg->AllocLabel();
    etsg->SetLabel(st, startLabel);

    auto lref = compiler::ETSLReference::Create(etsg, st->Left(), false);

    if (exprType->IsETSArrayType()) {
        etsg->LoadArrayElement(st, objReg);
    } else if (exprType->IsETSUnionType()) {
        HandleUnionTypeInForOf(etsg, exprType, st, objReg, &countReg);
    } else {
        etsg->LoadStringChar(st, objReg, countReg);
    }

    lref.SetValue();
    st->Body()->Compile(etsg);

    etsg->SetLabel(st, labelTarget.ContinueTarget());

    etsg->IncrementImmediateRegister(st, countReg, checker::TypeFlag::INT, static_cast<std::int32_t>(1));
    etsg->LoadAccumulator(st, countReg);

    etsg->JumpCompareRegister<compiler::Jlt>(st, sizeReg, startLabel);
    etsg->SetLabel(st, labelTarget.BreakTarget());
}

void ETSCompiler::Compile(const ir::ForUpdateStatement *st) const
{
    ETSGen *etsg = GetETSGen();
    compiler::LocalRegScope declRegScope(etsg, st->Scope()->DeclScope()->InitScope());

    if (st->Init() != nullptr) {
        ES2PANDA_ASSERT(st->Init()->IsVariableDeclaration() || st->Init()->IsExpression());
        st->Init()->Compile(etsg);
    }

    auto *startLabel = etsg->AllocLabel();
    compiler::LabelTarget labelTarget(etsg);
    auto labelCtx = compiler::LabelContext(etsg, labelTarget);
    etsg->SetLabel(st, startLabel);

    {
        compiler::LocalRegScope regScope(etsg, st->Scope());

        if (st->Test() != nullptr) {
            compiler::Condition::Compile(etsg, st->Test(), labelTarget.BreakTarget());
        }

        st->Body()->Compile(etsg);
        etsg->SetLabel(st, labelTarget.ContinueTarget());
    }

    if (st->Update() != nullptr) {
        st->Update()->Compile(etsg);
    }

    etsg->Branch(st, startLabel);
    etsg->SetLabel(st, labelTarget.BreakTarget());
}

void ETSCompiler::Compile(const ir::IfStatement *st) const
{
    ETSGen *etsg = GetETSGen();

    auto *consequentEnd = etsg->AllocLabel();
    compiler::Label *statementEnd = consequentEnd;

    compiler::Condition::Compile(etsg, st->Test(), consequentEnd);

    st->Consequent()->Compile(etsg);

    if (st->Alternate() != nullptr) {
        statementEnd = etsg->AllocLabel();
        etsg->Branch(etsg->Insns().back()->Node(), statementEnd);

        etsg->SetLabel(st, consequentEnd);
        st->Alternate()->Compile(etsg);
    }

    etsg->SetLabel(st, statementEnd);
}

void CompileImpl(const ir::LabelledStatement *self, ETSGen *cg)
{
    compiler::LabelContext labelCtx(cg, self);
    self->Body()->Compile(cg);
}

void ETSCompiler::Compile(const ir::LabelledStatement *st) const
{
    ETSGen *etsg = GetETSGen();
    CompileImpl(st, etsg);
}

static void CheckVoidReturnType(const ir::ReturnStatement *st, const ir::Expression *const argument, ETSGen *etsg)
{
    if (st->ReturnType()->IsETSVoidType() &&
        etsg->Checker()->Relation()->IsSupertypeOf(etsg->Checker()->GlobalVoidType(), argument->TsType())) {
        etsg->EmitReturnVoid(st);
    }
}

void ETSCompiler::Compile(const ir::ReturnStatement *st) const
{
    ETSGen *etsg = GetETSGen();
    bool isAsyncImpl = st->IsAsyncImplReturn();
    auto *const argument = st->Argument();
    if (argument == nullptr) {
        if (etsg->ExtendWithFinalizer(st->Parent(), st)) {
            return;
        }
        if (etsg->CheckControlFlowChange()) {
            etsg->ControlFlowChangeBreak();
        }
        if (isAsyncImpl) {
            etsg->LoadAccumulatorUndefined(st);
            etsg->ReturnAcc(st);
            return;
        }
        etsg->EmitReturnVoid(st);
        return;
    }

    if (argument->IsCallExpression() && argument->AsCallExpression()->Signature()->ReturnType()->IsETSVoidType()) {
        argument->Compile(etsg);
        if (etsg->ReturnType()->IsETSVoidType()) {
            if (isAsyncImpl) {
                etsg->LoadAccumulatorUndefined(st);
                etsg->ReturnAcc(st);
            } else {
                etsg->EmitReturnVoid(st);
            }
        } else {
            etsg->LoadDefaultValue(st, etsg->ReturnType());
            etsg->ReturnAcc(st);
        }
        return;
    }
    auto ttctx = compiler::TargetTypeContext(etsg, st->ReturnType());
    argument->Compile(etsg);
    etsg->ApplyConversion(argument, st->ReturnType());
    if (etsg->ExtendWithFinalizer(st->Parent(), st)) {
        return;
    }
    if (etsg->CheckControlFlowChange()) {
        compiler::RegScope rs(etsg);
        compiler::VReg res = etsg->AllocReg();

        etsg->StoreAccumulator(st, res);
        etsg->ControlFlowChangeBreak();
        etsg->LoadAccumulator(st, res);
    }
    CheckVoidReturnType(st, argument, etsg);
    etsg->ReturnAcc(st);
}

static void CompileImpl(const ir::SwitchStatement *self, ETSGen *etsg)
{
    compiler::LocalRegScope lrs(etsg, self->Scope());
    compiler::SwitchBuilder builder(etsg, self);
    compiler::VReg tag = etsg->AllocReg();

    builder.CompileTagOfSwitch(tag);
    int32_t defaultIndex = -1;

    for (size_t i = 0; i < self->Cases().size(); i++) {
        const auto *clause = self->Cases()[i];

        if (clause->Test() == nullptr) {
            defaultIndex = i;
            continue;
        }

        builder.JumpIfCase(tag, i);
    }

    if (defaultIndex >= 0) {
        builder.JumpToDefault(defaultIndex);
    } else {
        builder.Break();
    }

    for (size_t i = 0; i < self->Cases().size(); i++) {
        builder.SetCaseTarget(i);
        builder.CompileCaseStatements(i);
    }
}

void ETSCompiler::Compile(const ir::SwitchStatement *st) const
{
    ETSGen *etsg = GetETSGen();
    CompileImpl(st, etsg);
}

void ETSCompiler::Compile(const ir::ThrowStatement *st) const
{
    ETSGen *etsg = GetETSGen();
    etsg->ThrowException(st->Argument());
}

void ETSCompiler::Compile(const ir::TryStatement *st) const
{
    ETSGen *etsg = GetETSGen();

    compiler::ETSTryContext tryCtx(etsg, etsg->Allocator(), st, st->FinallyBlock() != nullptr);

    compiler::LabelPair tryLabelPair(etsg->AllocLabel(), etsg->AllocLabel());

    for (ir::CatchClause *clause : st->CatchClauses()) {
        tryCtx.AddNewCathTable(clause->TsType()->AsETSObjectType()->AssemblerName(), tryLabelPair);
    }

    compiler::Label *statementEnd = etsg->AllocLabel();
    auto catchTables = tryCtx.GetETSCatchTable();

    etsg->SetLabel(st, tryLabelPair.Begin());
    st->Block()->Compile(etsg);
    etsg->Branch(st, statementEnd);
    etsg->SetLabel(st, tryLabelPair.End());

    ES2PANDA_ASSERT(st->CatchClauses().size() == catchTables.size());

    for (uint32_t i = 0; i < st->CatchClauses().size(); i++) {
        etsg->SetLabel(st, catchTables.at(i)->LabelSet().CatchBegin());

        st->CatchClauses().at(i)->Compile(etsg);

        etsg->Branch(st, statementEnd);
    }

    etsg->SetLabel(st, statementEnd);

    auto trycatchLabelPair = compiler::LabelPair(tryLabelPair.Begin(), statementEnd);

    tryCtx.EmitFinalizer(trycatchLabelPair, st->finalizerInsertions_);
}

void ETSCompiler::Compile(const ir::VariableDeclarator *st) const
{
    ETSGen *etsg = GetETSGen();
    auto lref = compiler::ETSLReference::Create(etsg, st->Id(), true);
    auto ttctx = compiler::TargetTypeContext(etsg, st->TsType());

    if (st->Init() != nullptr) {
        st->Init()->Compile(etsg);
        etsg->ApplyConversion(st->Init(), st->Id()->AsIdentifier()->Variable()->TsType());
    } else {
        etsg->LoadDefaultValue(st, st->Id()->AsIdentifier()->Variable()->TsType());
    }

    // ????????????????????????????????????? is this meaningfull????
    etsg->ApplyConversion(st, st->TsType());
    lref.SetValue();
}

void ETSCompiler::Compile(const ir::VariableDeclaration *st) const
{
    ETSGen *etsg = GetETSGen();
    for (const auto *it : st->Declarators()) {
        it->Compile(etsg);
    }
}

template <typename CodeGen>
void CompileImpl(const ir::WhileStatement *whileStmt, [[maybe_unused]] CodeGen *cg)
{
    compiler::LabelTarget labelTarget(cg);

    cg->SetLabel(whileStmt, labelTarget.ContinueTarget());
    compiler::Condition::Compile(cg, whileStmt->Test(), labelTarget.BreakTarget());

    {
        compiler::LocalRegScope regScope(cg, whileStmt->Scope());
        compiler::LabelContext labelCtx(cg, labelTarget);
        whileStmt->Body()->Compile(cg);
    }

    cg->Branch(whileStmt, labelTarget.ContinueTarget());
    cg->SetLabel(whileStmt, labelTarget.BreakTarget());
}

void ETSCompiler::Compile(const ir::WhileStatement *st) const
{
    ETSGen *etsg = GetETSGen();
    CompileImpl(st, etsg);
}

void ETSCompiler::Compile(const ir::TSArrayType *node) const
{
    ETSGen *etsg = GetETSGen();
    etsg->LoadAccumulatorPoison(node, node->TsType());
}

void ETSCompiler::CompileCastPrimitives(const ir::Expression *expr, checker::Type const *targetType) const
{
    ETSGen *etsg = GetETSGen();

    switch (checker::ETSChecker::TypeKind(targetType)) {
        case checker::TypeFlag::ETS_BOOLEAN: {
            etsg->CastToBoolean(expr);
            break;
        }
        case checker::TypeFlag::CHAR: {
            etsg->CastToChar(expr);
            break;
        }
        case checker::TypeFlag::BYTE: {
            etsg->CastToByte(expr);
            break;
        }
        case checker::TypeFlag::SHORT: {
            etsg->CastToShort(expr);
            break;
        }
        case checker::TypeFlag::INT: {
            etsg->CastToInt(expr);
            break;
        }
        case checker::TypeFlag::LONG: {
            etsg->CastToLong(expr);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            etsg->CastToFloat(expr);
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            etsg->CastToDouble(expr);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
    etsg->SetAccumulatorType(targetType);
}

// NOTE(gogabr): will be needed once we forbid as conversions
/*
static void CastIfDynamic(ETSGen *etsg, ir::Expression const *expr, checker::TypeFlag typeFlag)
{
    // CC-OFFNXT(redundant_code[C++]) tmp code
    if (checker::ETSChecker::TypeKind(expr->TsType()) == checker::TypeFlag::ETS_DYNAMIC_TYPE) {
        etsg->CastDynamicToe(expr, typeFlag);
        return;
    }
    ES2PANDA_UNREACHABLE();
}
*/

void ETSCompiler::CompileCast(const ir::TSAsExpression *expr, checker::Type const *targetType) const
{
    ETSGen *etsg = GetETSGen();

    switch (checker::ETSChecker::TypeKind(targetType)) {
        case checker::TypeFlag::ETS_ARRAY:
        case checker::TypeFlag::ETS_TUPLE:
        case checker::TypeFlag::FUNCTION:
        case checker::TypeFlag::ETS_OBJECT:
        case checker::TypeFlag::ETS_TYPE_PARAMETER:
        case checker::TypeFlag::ETS_NONNULLISH:
        case checker::TypeFlag::ETS_PARTIAL_TYPE_PARAMETER:
        case checker::TypeFlag::ETS_UNION:
        case checker::TypeFlag::ETS_ANY:
        case checker::TypeFlag::ETS_NULL:
        case checker::TypeFlag::ETS_UNDEFINED:
        case checker::TypeFlag::ETS_VOID: {
            etsg->CastToReftype(expr, targetType, expr->isUncheckedCast_);
            break;
        }
        // NOTE(gogabr): will be needed once we forbid as conversion
        /*
        // CC-OFFNXT(redundant_code[C++]) tmp code
        case checker::TypeFlag::DOUBLE:
        case checker::TypeFlag::STRING:
        case checker::TypeFlag::ETS_BOOLEAN: {
            CastIfDynamic(etsg, expr->Expr(), targetType->TypeFlags());
            break;  // no further conversion
        }
        case checker::TypeFlag::BYTE:
        case checker::TypeFlag::SHORT:
        case checker::TypeFlag::INT:
        case checker::TypeFlag::LONG:
        case checker::TypeFlag::FLOAT:
        case checker::TypeFlag::CHAR: {
            CastIfDynamic(etsg, expr->Expr(), targetType->TypeFlags());
            CompileCastPrimitives(expr, targetType);
            break;
        }
        */
        default: {
            CompileCastPrimitives(expr, targetType);
            break;
        }
    }
}

void ETSCompiler::Compile(const ir::TSAsExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    expr->Expr()->Compile(etsg);

    const auto *const targetType = etsg->Checker()->GetApparentType(expr->TsType());

    if (!etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), targetType)) {
        auto ttctx = compiler::TargetTypeContext(etsg, nullptr);
        CompileCast(expr, targetType);
        ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), targetType));
    }
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSInterfaceDeclaration *st) const {}

void ETSCompiler::Compile(const ir::TSNonNullExpression *expr) const
{
    ETSGen *etsg = GetETSGen();
    compiler::RegScope rs(etsg);

    expr->Expr()->Compile(etsg);

    if (etsg->GetAccumulatorType()->PossiblyETSNullish()) {
        if (!etsg->GetAccumulatorType()->PossiblyETSNull()) {
            etsg->EmitNullcheck(expr);
            etsg->SetAccumulatorType(expr->OriginalType());
        } else {
            auto arg = etsg->AllocReg();
            etsg->StoreAccumulator(expr, arg);

            auto endLabel = etsg->AllocLabel();

            etsg->BranchIfNotNullish(expr, endLabel);
            etsg->EmitNullishException(expr);

            etsg->SetLabel(expr, endLabel);
            etsg->LoadAccumulator(expr, arg);
            etsg->AssumeNonNullish(expr, expr->OriginalType());
        }
    }

    ES2PANDA_ASSERT(etsg->Checker()->Relation()->IsIdenticalTo(etsg->GetAccumulatorType(), expr->OriginalType()));
}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSTypeAliasDeclaration *st) const {}

void ETSCompiler::Compile([[maybe_unused]] const ir::TSQualifiedName *expr) const
{
    ES2PANDA_UNREACHABLE();
}

}  // namespace ark::es2panda::compiler
