/**
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

#include <cstddef>

#include "checker/types/ets/etsResizableArrayType.h"
#include "checker/types/ets/etsTupleType.h"
#include "generated/signatures.h"
#include "checker/ets/wideningConverter.h"
#include "ir/astNodeFlags.h"
#include "varbinder/ETSBinder.h"
#include "checker/ETSchecker.h"
#include "checker/ets/function_helpers.h"
#include "checker/ets/typeRelationContext.h"
#include "checker/types/ets/etsAwaitedType.h"
#include "checker/types/ets/etsObjectType.h"
#include "checker/types/ets/etsPartialTypeParameter.h"
#include "checker/types/gradualType.h"
#include "checker/types/typeError.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "ir/base/catchClause.h"
#include "ir/base/classDefinition.h"
#include "ir/base/classProperty.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/base/spreadElement.h"
#include "ir/ets/etsFunctionType.h"
#include "ir/ets/etsParameterExpression.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/memberExpression.h"
#include "ir/expressions/objectExpression.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/doWhileStatement.h"
#include "ir/statements/expressionStatement.h"
#include "ir/statements/forInStatement.h"
#include "ir/statements/forOfStatement.h"
#include "ir/statements/forUpdateStatement.h"
#include "ir/statements/returnStatement.h"
#include "ir/statements/switchStatement.h"
#include "ir/statements/whileStatement.h"
#include "ir/ts/tsTypeAliasDeclaration.h"
#include "ir/ts/tsTypeParameter.h"
#include "ir/ts/tsTypeParameterInstantiation.h"
#include "parser/program/program.h"
#include "util/helpers.h"
#include "util/nameMangler.h"

namespace ark::es2panda::checker {

// NOTE: #14993 merge with InstantiationContext::ValidateTypeArg
bool ETSChecker::IsCompatibleTypeArgument(ETSTypeParameter *typeParam, Type *typeArgument,
                                          const Substitution *substitution)
{
    if (typeArgument->IsWildcardType()) {
        return true;
    }
    if (typeArgument->IsTypeError()) {
        return true;
    }
    // NOTE(vpukhov): #19701 void refactoring
    if (typeArgument->IsETSVoidType()) {
        typeArgument = GlobalETSUndefinedType();
    }
    ES2PANDA_ASSERT(IsReferenceType(typeArgument));
    auto constraint = typeParam->GetConstraintType()->Substitute(Relation(), substitution);
    return Relation()->IsSupertypeOf(constraint, typeArgument);
}

bool ETSChecker::EnhanceSubstitutionForReadonly(const ArenaVector<Type *> &typeParams, ETSReadonlyType *paramType,
                                                Type *argumentType, Substitution *substitution)
{
    return EnhanceSubstitutionForType(typeParams, paramType->GetUnderlying()->MaybeBaseTypeOfGradualType(),
                                      GetReadonlyType(argumentType), substitution);
}

/* A very rough and imprecise partial type inference */
bool ETSChecker::EnhanceSubstitutionForType(const ArenaVector<Type *> &typeParams, Type *paramType, Type *argumentType,
                                            Substitution *substitution)
{
    ES2PANDA_ASSERT(argumentType != nullptr);
    if (argumentType->IsETSPrimitiveType()) {
        argumentType = MaybeBoxInRelation(argumentType);
    }
    if (paramType->IsETSTypeParameter()) {
        auto *const originalTparam = paramType->AsETSTypeParameter()->GetOriginal();
        if (std::find(typeParams.begin(), typeParams.end(), originalTparam) != typeParams.end() &&
            substitution->count(originalTparam) == 0) {
            return EnhanceSubstitutionTypeParameter(paramType->AsETSTypeParameter(), argumentType, substitution);
        }
    }
    if (paramType->IsETSNonNullishType()) {
        return EnhanceSubstitutionForNonNullish(typeParams, paramType->AsETSNonNullishType(), argumentType,
                                                substitution);
    }
    if (paramType->IsETSFunctionType()) {
        return EnhanceSubstitutionForFunction(typeParams, paramType->AsETSFunctionType(), argumentType, substitution);
    }
    if (paramType->IsETSReadonlyType()) {
        return EnhanceSubstitutionForReadonly(typeParams, paramType->AsETSReadonlyType(), argumentType, substitution);
    }
    if (paramType->IsETSPartialTypeParameter()) {
        return EnhanceSubstitutionForPartialTypeParam(typeParams, paramType->AsETSPartialTypeParameter(), argumentType,
                                                      substitution);
    }
    if (paramType->IsGradualType()) {
        return EnhanceSubstitutionForType(typeParams, paramType->AsGradualType()->GetBaseType(), argumentType,
                                          substitution);
    }
    if (paramType->IsETSUnionType()) {
        return EnhanceSubstitutionForUnion(typeParams, paramType->AsETSUnionType(), argumentType, substitution);
    }
    if (paramType->IsETSResizableArrayType()) {
        return EnhanceSubstitutionForResizableArray(typeParams, paramType->AsETSResizableArrayType(), argumentType,
                                                    substitution);
    }
    if (paramType->IsETSObjectType()) {
        return EnhanceSubstitutionForObject(typeParams, paramType->AsETSObjectType(), argumentType, substitution);
    }
    if (paramType->IsETSArrayType()) {
        return EnhanceSubstitutionForArray(typeParams, paramType->AsETSArrayType(), argumentType, substitution);
    }
    if (paramType->IsETSAwaitedType()) {
        return EnhanceSubstitutionForAwaited(typeParams, paramType->AsETSAwaitedType(), argumentType, substitution);
    }

    return true;
}

bool ETSChecker::ValidateTypeSubstitution(const ArenaVector<Type *> &typeParams, Type *ctype, Type *argumentType,
                                          Substitution *substitution)
{
    if (!EnhanceSubstitutionForType(typeParams, ctype, argumentType, substitution)) {
        return false;
    }
    return !ctype->IsETSTypeParameter() ||
           (substitution->count(ctype->AsETSTypeParameter()) > 0 &&
            Relation()->IsAssignableTo(argumentType, substitution->at(ctype->AsETSTypeParameter())));
}

bool ETSChecker::EnhanceSubstitutionForUnion(const ArenaVector<Type *> &typeParams, ETSUnionType *paramUn,
                                             Type *argumentType, Substitution *substitution)
{
    if (!argumentType->IsETSUnionType()) {
        bool foundValid = false;
        for (Type *ctype : paramUn->ConstituentTypes()) {
            foundValid |=
                ValidateTypeSubstitution(typeParams, ctype->MaybeBaseTypeOfGradualType(), argumentType, substitution);
        }
        return foundValid;
    }
    auto *const argUn = argumentType->AsETSUnionType();

    ArenaVector<Type *> paramWlist(ProgramAllocator()->Adapter());
    ArenaVector<Type *> argWlist(ProgramAllocator()->Adapter());

    for (auto *pc : paramUn->ConstituentTypes()) {
        for (auto *ac : argUn->ConstituentTypes()) {
            if (ETSChecker::GetOriginalBaseType(pc) != ETSChecker::GetOriginalBaseType(ac)) {
                paramWlist.push_back(pc);
                argWlist.push_back(ac);
                continue;
            }
            if (!EnhanceSubstitutionForType(typeParams, pc, ac, substitution)) {
                return false;
            }
        }
    }
    auto *const newArg = CreateETSUnionType(std::move(argWlist));

    for (auto *pc : paramWlist) {
        if (!EnhanceSubstitutionForType(typeParams, pc, newArg, substitution)) {
            return false;
        }
    }
    return true;
}

bool ETSChecker::ProcessUntypedParameter(ir::AstNode *declNode, size_t paramIndex, Signature *paramSig,
                                         Signature *argSig, Substitution *substitution)
{
    if (!declNode->IsETSParameterExpression() || !HasStatus(CheckerStatus::IN_TYPE_INFER)) {
        return false;
    }

    auto *paramExpr = declNode->AsETSParameterExpression();
    if (paramExpr->Ident()->TypeAnnotation() != nullptr) {
        return false;
    }

    Type *paramType = paramSig->Params()[paramIndex]->TsType();
    Type *inferredType = paramType->Substitute(Relation(), substitution);

    varbinder::Variable *argParam = argSig->Params()[paramIndex];
    argParam->SetTsType(inferredType);
    paramExpr->Ident()->SetTsType(inferredType);

    return true;
}

static void RemoveInvalidTypeMarkers(ir::AstNode *node) noexcept
{
    std::function<void(ir::AstNode *)> doNode = [&](ir::AstNode *nn) {
        if (nn->IsTyped() && !(nn->IsExpression() && nn->AsExpression()->IsTypeNode()) &&
            nn->AsTyped()->TsType() != nullptr && nn->AsTyped()->TsType()->IsTypeError()) {
            nn->AsTyped()->SetTsType(nullptr);
        }
        if (nn->IsIdentifier() && nn->AsIdentifier()->TsType() != nullptr &&
            nn->AsIdentifier()->TsType()->IsTypeError()) {
            nn->AsIdentifier()->SetVariable(nullptr);
        }
        if (!nn->IsETSTypeReference()) {
            nn->Iterate([&](ir::AstNode *child) { doNode(child); });
        }
    };

    doNode(node);
}

static void ResetInferredNode(ETSChecker *checker)
{
    auto relation = checker->Relation();
    auto resetFuncState = [](ir::ArrowFunctionExpression *expr) {
        auto *func = expr->Function();
        func->SetSignature(nullptr);
        func->ClearReturnStatements();
        expr->SetTsType(nullptr);
    };

    const bool hasValidNode = relation->GetNode() != nullptr && relation->GetNode()->IsArrowFunctionExpression();
    if (!checker->HasStatus(CheckerStatus::IN_TYPE_INFER) || !hasValidNode) {
        return;
    }

    auto *arrowFunc = relation->GetNode()->AsArrowFunctionExpression();
    relation->SetNode(nullptr);

    RemoveInvalidTypeMarkers(arrowFunc);
    resetFuncState(arrowFunc);
    arrowFunc->Check(checker);
}

bool ETSChecker::EnhanceSubstitutionForNonNullish(const ArenaVector<Type *> &typeParams, ETSNonNullishType *paramType,
                                                  Type *argumentType, Substitution *substitution)
{
    if (argumentType->IsETSNonNullishType()) {
        ES2PANDA_ASSERT(argumentType->AsETSNonNullishType()->GetUnderlying() != nullptr);
        return EnhanceSubstitutionForType(typeParams, paramType->GetUnderlying(),
                                          argumentType->AsETSNonNullishType()->GetUnderlying(), substitution);
    }
    return EnhanceSubstitutionForType(typeParams, paramType->GetUnderlying(), argumentType, substitution);
}

bool ETSChecker::EnhanceSubstitutionTypeParameter(ETSTypeParameter *paramType, Type *argumentType,
                                                  Substitution *substitution)
{
    auto *const originalTparam = paramType->GetOriginal();
    if (!IsReferenceType(argumentType)) {
        LogError(diagnostic::INFERENCE_TYPE_INCOMPAT, {paramType, argumentType}, paramType->GetDeclNode()->Start());
        return false;
    }

    // #23068 substitution happens before the constraint check, should be restored
    EmplaceSubstituted(substitution, originalTparam, argumentType);
    return IsCompatibleTypeArgument(paramType, argumentType, substitution);
}

bool ETSChecker::EnhanceSubstitutionForFunction(const ArenaVector<Type *> &typeParams, ETSFunctionType *paramType,
                                                Type *argumentType, Substitution *substitution)
{
    auto const enhance = [this, typeParams, substitution](Type *ptype, Type *atype) {
        return EnhanceSubstitutionForType(typeParams, ptype, atype, substitution);
    };

    if (!argumentType->IsETSFunctionType()) {
        return true;
    }

    auto *paramSig = paramType->ArrowSignature();
    auto *argSig = argumentType->AsETSFunctionType()->ArrowSignature();

    if (paramSig->MinArgCount() < argSig->MinArgCount()) {
        return false;
    }

    bool res = true;
    const size_t commonArity = std::min(argSig->ArgCount(), paramSig->ArgCount());

    for (size_t idx = 0; idx < commonArity; idx++) {
        auto *declNode = argSig->Params()[idx]->Declaration()->Node();
        if (ProcessUntypedParameter(declNode, idx, paramSig, argSig, substitution)) {
            continue;
        }
        res &= enhance(paramSig->Params()[idx]->TsType(), argSig->Params()[idx]->TsType());
    }

    ResetInferredNode(this);

    if (argSig->HasRestParameter() && paramSig->HasRestParameter()) {
        res &= enhance(paramSig->RestVar()->TsType(), argSig->RestVar()->TsType());
    }
    res &= enhance(paramSig->ReturnType(), argSig->ReturnType());

    return res;
}

bool ETSChecker::EnhanceSubstitutionForAwaited(const ArenaVector<Type *> &typeParams, ETSAwaitedType *paramType,
                                               Type *argumentType, Substitution *substitution)
{
    auto *argumentAwaitedType =
        argumentType->IsETSAwaitedType() ? argumentType->AsETSAwaitedType()->GetUnderlying() : argumentType;
    auto *paramAwaitedType = paramType->GetUnderlying();
    return EnhanceSubstitutionForType(typeParams, paramAwaitedType, argumentAwaitedType, substitution);
}

bool ETSChecker::EnhanceSubstitutionForPartialTypeParam(const ArenaVector<Type *> &typeParams,
                                                        ETSPartialTypeParameter *paramType, Type *argumentType,
                                                        Substitution *substitution)
{
    if (!argumentType->IsETSObjectType() || !argumentType->AsETSObjectType()->IsPartial()) {
        return false;
    }
    ES2PANDA_ASSERT(argumentType->AsETSObjectType()->GetBaseType() != nullptr);
    return EnhanceSubstitutionForType(typeParams, paramType->GetUnderlying(),
                                      argumentType->AsETSObjectType()->GetBaseType(), substitution);
}

// Try to find the base type somewhere in object subtypes. Incomplete, yet safe
static ETSObjectType *FindEnhanceTargetInSupertypes(ETSObjectType *object, ETSObjectType *base)
{
    ES2PANDA_ASSERT(base == base->GetOriginalBaseType());
    if (object->GetConstOriginalBaseType() == base) {
        return object;
    }
    auto const traverse = [base](ETSObjectType *v) { return FindEnhanceTargetInSupertypes(v, base); };

    for (auto itf : object->Interfaces()) {
        auto res = traverse(itf);
        if (res != nullptr) {
            return res;
        }
    }

    if (object->SuperType() != nullptr) {
        auto res = traverse(object->SuperType());
        if (res != nullptr) {
            return res;
        }
    }
    return nullptr;
}

bool ETSChecker::EnhanceSubstitutionForObject(const ArenaVector<Type *> &typeParams, ETSObjectType *paramType,
                                              Type *argumentType, Substitution *substitution)
{
    auto const enhance = [this, typeParams, substitution](Type *ptype, Type *atype) {
        return EnhanceSubstitutionForType(typeParams, ptype, atype, substitution);
    };

    if (!argumentType->IsETSObjectType()) {
        return true;
    }
    auto enhanceType = FindEnhanceTargetInSupertypes(argumentType->AsETSObjectType(), paramType->GetOriginalBaseType());
    if (enhanceType == nullptr) {
        return true;
    }
    ES2PANDA_ASSERT(enhanceType->GetOriginalBaseType() == paramType->GetOriginalBaseType());
    bool res = true;
    for (size_t i = 0; i < enhanceType->TypeArguments().size(); i++) {
        res &= enhance(paramType->TypeArguments()[i], enhanceType->TypeArguments()[i]);
    }
    return res;
}

bool ETSChecker::EnhanceSubstitutionForArray(const ArenaVector<Type *> &typeParams, ETSArrayType *const paramType,
                                             Type *const argumentType, Substitution *const substitution)
{
    auto *const elementType =
        argumentType->IsETSArrayType() ? argumentType->AsETSArrayType()->ElementType() : argumentType;

    return EnhanceSubstitutionForType(typeParams, paramType->ElementType(), elementType, substitution);
}

bool ETSChecker::EnhanceSubstitutionForResizableArray(const ArenaVector<Type *> &typeParams,
                                                      ETSResizableArrayType *const paramType, Type *const argumentType,
                                                      Substitution *const substitution)
{
    auto *const elementType =
        argumentType->IsETSResizableArrayType() ? argumentType->AsETSResizableArrayType()->ElementType() : argumentType;

    return EnhanceSubstitutionForType(typeParams, paramType->ElementType(), elementType, substitution);
}

Signature *ETSChecker::ValidateParameterlessConstructor(Signature *signature, const lexer::SourcePosition &pos,
                                                        bool throwError)
{
    if (signature->MinArgCount() != 0) {
        if (throwError) {
            LogError(diagnostic::NO_SUCH_PARAMLESS_CTOR_2, {signature->MinArgCount()}, pos);
        }
        return nullptr;
    }
    return signature;
}

// #22952: remove optional arrow leftovers
bool ETSChecker::CheckOptionalLambdaFunction(ir::Expression *argument, Signature *substitutedSig, std::size_t index)
{
    if (argument->IsArrowFunctionExpression()) {
        auto *const arrowFuncExpr = argument->AsArrowFunctionExpression();

        if (ir::ScriptFunction *const lambda = arrowFuncExpr->Function();
            CheckLambdaAssignable(substitutedSig->Params()[index]->Declaration()->Node()->AsExpression(), lambda)) {
            return true;
        }
    }

    return false;
}

static bool IsInvalidArgumentAsIdentifier(varbinder::Scope *scope, const ir::Identifier *identifier)
{
    auto result = scope->Find(identifier->Name());
    return result.variable != nullptr && (result.variable->HasFlag(varbinder::VariableFlags::CLASS_OR_INTERFACE |
                                                                   varbinder::VariableFlags::TYPE_ALIAS));
}

static void ClearPreferredTypeForArray(checker::ETSChecker *checker, ir::Expression *argument, Type *paramType,
                                       TypeRelationFlag flags, bool needRecheck)
{
    if (argument->IsArrayExpression()) {
        // fixed array and resizeable array will cause problem here, so clear it.
        argument->AsArrayExpression()->CleanCheckInformation();
        argument->AsArrayExpression()->SetPreferredTypeBasedOnFuncParam(checker, paramType, flags);
    } else if (argument->IsETSNewArrayInstanceExpression()) {
        argument->AsETSNewArrayInstanceExpression()->CleanCheckInformation();
        argument->AsETSNewArrayInstanceExpression()->SetPreferredTypeBasedOnFuncParam(checker, paramType, flags);
    } else if (argument->IsETSNewMultiDimArrayInstanceExpression()) {
        argument->AsETSNewMultiDimArrayInstanceExpression()->CleanCheckInformation();
        argument->AsETSNewMultiDimArrayInstanceExpression()->SetPreferredTypeBasedOnFuncParam(checker, paramType,
                                                                                              flags);
    } else {
        return;
    }
    if (needRecheck) {
        argument->Check(checker);
    }
}

static bool CheckArrowFunctionParamIfNeeded(ETSChecker *checker, Signature *substitutedSig,
                                            const ArenaVector<ir::Expression *> &arguments, TypeRelationFlag flags)
{
    if ((flags & TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA) != 0 && arguments.back()->IsArrowFunctionExpression()) {
        ir::ScriptFunction *const lambda = arguments.back()->AsArrowFunctionExpression()->Function();
        auto targetParm = substitutedSig->GetSignatureInfo()->params.back()->Declaration()->Node();
        if (!checker->CheckLambdaAssignable(targetParm->AsETSParameterExpression(), lambda)) {
            return false;
        }
    }
    return true;
}

// Note: (Issue27688) if lambda is trailing lambda transferred, it must be in recheck.
// in signature matching, foo(()=>void) should be the same with foo() {}
static bool HasTransferredTrailingLambda(const ArenaVector<ir::Expression *> &arguments)
{
    return !arguments.empty() && arguments.back()->IsArrowFunctionExpression() &&
           arguments.back()->AsArrowFunctionExpression()->Function()->IsTrailingLambda();
}

bool ValidateRestParameter(ETSChecker *checker, Signature *signature, const ArenaVector<ir::Expression *> &arguments,
                           const lexer::SourcePosition &pos, TypeRelationFlag flags)
{
    size_t const argCount = arguments.size();
    size_t compareCount = argCount;
    auto const hasRestParameter = signature->HasRestParameter();
    auto const reportError = (flags & TypeRelationFlag::NO_THROW) == 0;
    if ((flags & TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA) != 0 && !signature->Params().empty() &&
        signature->Params().back()->Declaration()->Node()->AsETSParameterExpression()->IsOptional()) {
        compareCount = compareCount - 1;
    }

    if (!hasRestParameter && argCount > 0 && arguments[argCount - 1]->IsSpreadElement()) {
        if (reportError) {
            checker->LogError(diagnostic::ERROR_ARKTS_SPREAD_ONLY_WITH_REST, {}, pos);
        }
        return false;
    }
    if (compareCount < signature->MinArgCount() || (argCount > signature->ArgCount() && !hasRestParameter)) {
        if (reportError) {
            checker->LogError(diagnostic::PARAM_COUNT_MISMATCH, {signature->MinArgCount(), argCount}, pos);
        }
        return false;
    }
    if (hasRestParameter &&
        (((flags & TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA) != 0) || HasTransferredTrailingLambda(arguments))) {
        return false;
    }
    return !(argCount > signature->ArgCount() && hasRestParameter &&
             (flags & TypeRelationFlag::IGNORE_REST_PARAM) != 0);
}

// NOTE(dkofanov): Mimics type inferrence for integer literals. Also relies on the implicit widening which occurs
// later in checker and 'CheckCastLiteral' during 'ConstantExpressionLowering'.
static void InferTypeForNumberLiteral(ETSChecker *checker, ir::NumberLiteral *argumentLiteral, Type *paramType)
{
    argumentLiteral->SetTsType(nullptr);
    argumentLiteral->SetPreferredType(paramType);
    auto &number = argumentLiteral->AsNumberLiteral()->Number();

    auto *typeRel = checker->Relation();
    if (typeRel->IsSupertypeOf(checker->GlobalLongBuiltinType(), paramType)) {
        number.TryNarrowTo<int64_t>();
    } else if (typeRel->IsSupertypeOf(checker->GlobalIntBuiltinType(), paramType)) {
        number.TryNarrowTo<int32_t>();
    } else if (typeRel->IsSupertypeOf(checker->GlobalShortBuiltinType(), paramType)) {
        number.TryNarrowTo<int16_t>();
    } else if (typeRel->IsSupertypeOf(checker->GlobalByteBuiltinType(), paramType)) {
        number.TryNarrowTo<int8_t>();
    }
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP, G.FUD.05) solid logic
bool ETSChecker::ValidateSignatureRequiredParams(Signature *substitutedSig,
                                                 const ArenaVector<ir::Expression *> &arguments, TypeRelationFlag flags,
                                                 const std::vector<bool> &argTypeInferenceRequired, bool reportError)
{
    auto commonArity = std::min(arguments.size(), substitutedSig->ArgCount());
    if ((flags & TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA) != 0 || HasTransferredTrailingLambda(arguments)) {
        if (commonArity == 0) {
            ES2PANDA_ASSERT(substitutedSig->GetSignatureInfo()->params.empty());
            return true;
        }
        commonArity = commonArity - 1;
    }
    for (size_t index = 0; index < commonArity; ++index) {
        auto &argument = arguments[index];

        // #22952: infer optional parameter heuristics
        auto const paramType =
            GetNonNullishType(substitutedSig->Params()[index]->TsType())->MaybeBaseTypeOfGradualType();
        if (argument->IsObjectExpression()) {
            ES2PANDA_ASSERT(paramType != nullptr);
            if (!paramType->IsETSObjectType()) {
                return false;
            }
            if (paramType->AsETSObjectType()->IsBoxedPrimitive()) {
                return false;
            }
            argument->SetPreferredType(paramType);
        }

        if (argument->IsMemberExpression()) {
            SetArrayPreferredTypeForNestedMemberExpressions(argument->AsMemberExpression(), paramType);
        } else if (argument->IsSpreadElement()) {
            if (reportError) {
                LogError(diagnostic::SPREAD_ONTO_SINGLE_PARAM, {}, argument->Start());
            }
            return false;
        } else if (argument->IsNumberLiteral()) {
            InferTypeForNumberLiteral(this, argument->AsNumberLiteral(), paramType);
        }

        if (argTypeInferenceRequired[index]) {
            ES2PANDA_ASSERT(argument->IsArrowFunctionExpression());
            // Note: If the signatures are from lambdas, then they have no `Function`.
            ir::ScriptFunction *const lambda = argument->AsArrowFunctionExpression()->Function();
            auto targetParm = substitutedSig->GetSignatureInfo()->params[index]->Declaration()->Node();
            ERROR_SANITY_CHECK(this, targetParm->IsETSParameterExpression(), return false);
            if (CheckLambdaAssignable(targetParm->AsETSParameterExpression(), lambda)) {
                continue;
            }
            return false;
        }

        ClearPreferredTypeForArray(this, argument, paramType, flags, false);

        if (argument->IsIdentifier() && IsInvalidArgumentAsIdentifier(Scope(), argument->AsIdentifier())) {
            LogError(diagnostic::ARG_IS_CLASS_ID, {}, argument->Start());
            return false;
        }

        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        if (!ValidateSignatureInvocationContext(substitutedSig, argument, index, flags)) {
            return false;
        }
    }

    return CheckArrowFunctionParamIfNeeded(this, substitutedSig, arguments, flags);
}

bool ETSChecker::ValidateSignatureInvocationContext(Signature *substitutedSig, ir::Expression *argument,
                                                    std::size_t index, TypeRelationFlag flags)
{
    Type *targetType = substitutedSig->Params()[index]->TsType();
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    Type *argumentType = argument->Check(this);

    flags |= (TypeRelationFlag::ONLY_CHECK_WIDENING);

    auto const invocationCtx =
        checker::InvocationContext(Relation(), argument, argumentType, targetType, argument->Start(),
                                   {{diagnostic::TYPE_MISMATCH_AT_IDX, {argumentType, targetType, index + 1}}}, flags);

    return invocationCtx.IsInvocable() || CheckOptionalLambdaFunction(argument, substitutedSig, index);
}

bool ETSChecker::IsValidRestArgument(ir::Expression *const argument, Signature *const substitutedSig,
                                     const TypeRelationFlag flags, const std::size_t index)
{
    auto *restParamType = substitutedSig->RestVar()->TsType();
    if (restParamType->IsETSTupleType()) {
        return false;
    }
    if (argument->IsObjectExpression()) {
        argument->SetPreferredType(GetElementTypeOfArray(restParamType));
        // Object literals should be checked separately afterwards after call resolution
        return true;
    }

    // Set preferred type for array expressions before checking, similar to spread elements
    if (argument->IsArrayExpression()) {
        if (!SetPreferredTypeForArrayArgument(argument->AsArrayExpression(), substitutedSig)) {
            return false;
        }
    }

    const auto argumentType = argument->Check(this);
    if (argument->HasAstNodeFlags(ir::AstNodeFlags::RESIZABLE_REST)) {
        return true;
    }

    auto targetType = GetElementTypeOfArray(restParamType);
    if (substitutedSig->OwnerVar() == nullptr) {
        targetType = MaybeBoxType(targetType);
    }
    auto const invocationCtx = checker::InvocationContext(
        Relation(), argument, argumentType, targetType, argument->Start(),
        {{diagnostic::REST_PARAM_INCOMPAT_AT, {argumentType, targetType, index + 1}}}, flags);

    bool result = invocationCtx.IsInvocable();
    // Clear preferred type if invocation fails, similar to spread elements
    if (!result && argument->IsArrayExpression()) {
        ModifyPreferredType(argument->AsArrayExpression(), nullptr);
    }

    return result;
}

bool ETSChecker::SetPreferredTypeForArrayArgument(ir::ArrayExpression *arrayExpr, Signature *substitutedSig)
{
    auto *const restVarType = substitutedSig->RestVar()->TsType();
    if (!restVarType->IsETSArrayType() && !restVarType->IsETSResizableArrayType()) {
        return true;
    }
    auto targetType = GetElementTypeOfArray(restVarType);
    if (substitutedSig->OwnerVar() == nullptr) {
        targetType = MaybeBoxType(targetType);
    }
    // Validate tuple size before setting preferred type
    if (targetType->IsETSTupleType()) {
        auto *tupleType = targetType->AsETSTupleType();
        if (tupleType->GetTupleSize() != arrayExpr->Elements().size()) {
            // Size mismatch - don't set preferred type, this will cause a type error
            return false;
        }
    }
    arrayExpr->SetPreferredType(targetType);
    return true;
}

bool ETSChecker::ValidateSignatureRestParams(Signature *substitutedSig, const ArenaVector<ir::Expression *> &arguments,
                                             TypeRelationFlag flags, bool reportError,
                                             [[maybe_unused]] const bool unique)
{
    size_t const argumentCount = arguments.size();
    auto const commonArity = std::min(substitutedSig->ArgCount(), argumentCount);
    auto const restCount = argumentCount - commonArity;

    if (argumentCount == commonArity && substitutedSig->RestVar()->TsType()->IsETSTupleType()) {
        return false;
    }
    for (size_t index = commonArity; index < argumentCount; ++index) {
        auto &argument = arguments[index];

        if (!argument->IsSpreadElement()) {
            if (!IsValidRestArgument(argument, substitutedSig, flags, index)) {
                return false;
            }
            continue;
        }

        if (restCount > 1U) {
            if (reportError) {
                LogError(diagnostic::MULTIPLE_SPREADS, {}, argument->Start());
            }
            return false;
        }

        auto *const restArgument = argument->AsSpreadElement()->Argument();
        Type *targetType = substitutedSig->RestVar()->TsType();
        // backing out of check that results in a signature mismatch would be difficult
        // so only attempt it if there is only one candidate signature
        restArgument->SetPreferredType(targetType);
        argument->Check(this);
        auto const argumentType = restArgument->TsType();

        auto const invocationCtx = checker::InvocationContext(
            Relation(), restArgument, argumentType, substitutedSig->RestVar()->TsType(), argument->Start(),
            {{diagnostic::REST_PARAM_INCOMPAT_AT, {argumentType, substitutedSig->RestVar()->TsType(), index + 1}}},
            flags);
        if (!invocationCtx.IsInvocable()) {
            if (restArgument->IsArrayExpression()) {
                ModifyPreferredType(restArgument->AsArrayExpression(), nullptr);
            }
            return false;
        }
    }

    return true;
}

Signature *ETSChecker::ValidateSignature(
    std::tuple<Signature *, const ir::TSTypeParameterInstantiation *, TypeRelationFlag> info,
    const ArenaVector<ir::Expression *> &arguments, const lexer::SourcePosition &pos,
    const std::vector<bool> &argTypeInferenceRequired, const bool unique)
{
    auto [baseSignature, typeArguments, flags] = info;
    // In case of overloads, it is necessary to iterate through the compatible signatures again,
    // setting the boxing/unboxing flag for the arguments if needed.
    // So handle substitution arguments only in the case of unique function or collecting signature phase.
    Signature *const signature =
        ((flags & TypeRelationFlag::NO_SUBSTITUTION_NEEDED) != 0U)
            ? baseSignature
            : MaybeSubstituteTypeParameters(this, baseSignature, typeArguments, arguments, pos, flags);
    if (signature == nullptr) {
        return nullptr;
    }

    size_t const argCount = arguments.size();
    auto const hasRestParameter = signature->HasRestParameter();
    auto const reportError = (flags & TypeRelationFlag::NO_THROW) == 0;

    if (!ValidateRestParameter(this, signature, arguments, pos, flags)) {
        return nullptr;
    }
    auto count = std::min(signature->ArgCount(), argCount);
    // Check all required formal parameter(s) first
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    if (!ValidateSignatureRequiredParams(signature, arguments, flags, argTypeInferenceRequired, reportError)) {
        return nullptr;
    }

    // Check rest parameter(s) if any exists
    if (!hasRestParameter || (count >= argCount && !signature->RestVar()->TsType()->IsETSTupleType())) {
        return signature;
    }
    if (!ValidateSignatureRestParams(signature, arguments, flags, reportError, unique)) {
        return nullptr;
    }

    return signature;
}

Signature *ETSChecker::CollectParameterlessConstructor(ArenaVector<Signature *> &signatures,
                                                       const lexer::SourcePosition &pos)
{
    // We are able to provide more specific error messages.
    bool throwError = signatures.size() == 1;
    for (auto *sig : signatures) {
        if (auto *concreteSig = ValidateParameterlessConstructor(sig, pos, throwError); concreteSig != nullptr) {
            return concreteSig;
        }
    }
    LogError(diagnostic::NO_SUCH_PARAMLESS_CTOR, {}, pos);
    return nullptr;
}

bool IsSignatureAccessible(Signature *sig, ETSObjectType *containingClass, TypeRelation *relation)
{
    // NOTE(vivienvoros): this check can be removed if signature is implicitly declared as public according to the spec.
    if (!sig->HasSignatureFlag(SignatureFlags::PUBLIC | SignatureFlags::PROTECTED | SignatureFlags::PRIVATE |
                               SignatureFlags::INTERNAL)) {
        return true;
    }

    // NOTE(vivienvoros): take care of SignatureFlags::INTERNAL and SignatureFlags::INTERNAL_PROTECTED
    if (sig->HasSignatureFlag(SignatureFlags::INTERNAL) && !sig->HasSignatureFlag(SignatureFlags::PROTECTED)) {
        return true;
    }

    if (sig->HasSignatureFlag(SignatureFlags::PUBLIC) || sig->Owner() == containingClass ||
        (sig->HasSignatureFlag(SignatureFlags::PROTECTED) && relation->IsSupertypeOf(sig->Owner(), containingClass))) {
        return true;
    }

    return false;
}

// NOLINTNEXTLINE(readability-magic-numbers)
std::array<TypeRelationFlag, 9U> GetFlagVariants()
{
    // NOTE(boglarkahaag): Not in sync with specification, but solves the issues with rest params for now (#17483)
    return {
        TypeRelationFlag::NO_THROW | TypeRelationFlag::NO_UNBOXING | TypeRelationFlag::NO_BOXING |
            TypeRelationFlag::IGNORE_REST_PARAM | TypeRelationFlag::NO_WIDENING,
        TypeRelationFlag::NO_THROW | TypeRelationFlag::NO_UNBOXING | TypeRelationFlag::NO_BOXING,
        TypeRelationFlag::NO_THROW | TypeRelationFlag::IGNORE_REST_PARAM | TypeRelationFlag::NO_WIDENING,
        TypeRelationFlag::NO_THROW | TypeRelationFlag::NO_WIDENING,
        TypeRelationFlag::NO_THROW | TypeRelationFlag::WIDENING | TypeRelationFlag::NO_UNBOXING |
            TypeRelationFlag::NO_BOXING | TypeRelationFlag::IGNORE_REST_PARAM,
        TypeRelationFlag::NO_THROW | TypeRelationFlag::WIDENING | TypeRelationFlag::NO_UNBOXING |
            TypeRelationFlag::NO_BOXING,
        TypeRelationFlag::NO_THROW | TypeRelationFlag::WIDENING | TypeRelationFlag::IGNORE_REST_PARAM,
        TypeRelationFlag::NO_THROW | TypeRelationFlag::WIDENING,
    };
}

// CC-OFFNXT(huge_method) solid logic
ArenaVector<Signature *> ETSChecker::CollectSignatures(ArenaVector<Signature *> &signatures,
                                                       const ir::TSTypeParameterInstantiation *typeArguments,
                                                       const ArenaVector<ir::Expression *> &arguments,
                                                       const lexer::SourcePosition &pos, TypeRelationFlag resolveFlags)
{
    ArenaVector<Signature *> compatibleSignatures(ProgramAllocator()->Adapter());
    std::vector<bool> argTypeInferenceRequired = FindTypeInferenceArguments(arguments);
    Signature *notVisibleSignature = nullptr;

    if (signatures.size() > 1) {
        resolveFlags |= TypeRelationFlag::OVERLOADING_CONTEXT;
    }

    auto collectSignatures = [&](TypeRelationFlag relationFlags) {
        for (auto *sig : signatures) {
            if (notVisibleSignature != nullptr &&
                !IsSignatureAccessible(sig, Context().ContainingClass(), Relation())) {
                continue;
            }
            if (sig->HasSignatureFlag(SignatureFlags::BRIDGE)) {
                // Bridges are never invoked direcly
                continue;
            }
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            auto *concreteSig = ValidateSignature(std::make_tuple(sig, typeArguments, relationFlags), arguments, pos,
                                                  argTypeInferenceRequired, signatures.size() == 1);
            if (concreteSig == nullptr) {
                continue;
            }
            if (notVisibleSignature == nullptr &&
                !IsSignatureAccessible(sig, Context().ContainingClass(), Relation())) {
                notVisibleSignature = concreteSig;
            } else {
                compatibleSignatures.push_back(concreteSig);
            }
        }
    };

    // If there's only one signature, we don't need special checks for boxing/unboxing/widening.
    // We are also able to provide more specific error messages.
    if (signatures.size() == 1) {
        TypeRelationFlag flags = TypeRelationFlag::WIDENING | resolveFlags;
        collectSignatures(flags);
    } else {
        for (auto flags : GetFlagVariants()) {
            flags = flags | resolveFlags;
            collectSignatures(flags);
            if (!compatibleSignatures.empty()) {
                break;
            }
        }
    }

    if (compatibleSignatures.empty() && notVisibleSignature != nullptr &&
        ((resolveFlags & TypeRelationFlag::NO_THROW) == 0)) {
        LogError(diagnostic::SIG_INVISIBLE, {notVisibleSignature->Function()->Id()->Name(), notVisibleSignature}, pos);
    }
    return compatibleSignatures;
}

static void UpdateArrayArgsAndUnboxingFlags(ETSChecker *checker, Signature *sig,
                                            const ArenaVector<ir::Expression *> &arguments)
{
    auto const commonArity = std::min(arguments.size(), sig->ArgCount());
    for (size_t index = 0; index < commonArity; ++index) {
        auto argument = arguments[index];
        auto const paramType = checker->GetNonNullishType(sig->Params()[index]->TsType());
        auto flags = TypeRelationFlag::NO_THROW | TypeRelationFlag::BOXING | TypeRelationFlag::UNBOXING |
                     TypeRelationFlag::WIDENING;
        ClearPreferredTypeForArray(checker, argument, paramType, flags, true);
    }
}

Signature *ETSChecker::GetMostSpecificSignature(ArenaVector<Signature *> &compatibleSignatures,
                                                const ArenaVector<ir::Expression *> &arguments,
                                                const lexer::SourcePosition &pos, TypeRelationFlag resolveFlags)
{
    std::vector<bool> argTypeInferenceRequired = FindTypeInferenceArguments(arguments);
    Signature *mostSpecificSignature =
        ChooseMostSpecificSignature(compatibleSignatures, argTypeInferenceRequired, arguments, pos);

    if (mostSpecificSignature == nullptr) {
        LogError(diagnostic::AMBIGUOUS_FUNC_REF, {compatibleSignatures.front()->Function()->Id()->Name()}, pos);
        return nullptr;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    if (!TypeInference(mostSpecificSignature, arguments, resolveFlags)) {
        return nullptr;
    }

    // revalidate signature for arrays
    UpdateArrayArgsAndUnboxingFlags(this, mostSpecificSignature, arguments);

    return mostSpecificSignature;
}

void ETSChecker::ThrowSignatureMismatch(ArenaVector<Signature *> const &signatures,
                                        const ArenaVector<ir::Expression *> &arguments,
                                        const lexer::SourcePosition &pos, std::string_view signatureKind)
{
    if (!arguments.empty() && !signatures.empty()) {
        std::string msg {};
        auto someSignature = signatures[0];

        if (someSignature->HasFunction()) {
            if (someSignature->Function()->IsConstructor()) {
                msg.append(util::Helpers::GetClassDefinition(someSignature->Function())->InternalName().Mutf8());
            } else {
                msg.append(someSignature->Function()->Id()->Name().Mutf8());
            }
        }

        msg += "(";

        for (std::size_t index = 0U; index < arguments.size(); ++index) {
            auto const &argument = arguments[index];
            Type const *const argumentType = argument->Check(this);
            if (!argumentType->IsTypeError()) {
                msg += argumentType->ToString();
            } else {
                //  NOTE (DZ): extra cases for some specific nodes can be added here (as for 'ArrowFunctionExpression')
                msg += argument->ToString();
            }

            if (index == arguments.size() - 1U) {
                msg += ")";
                LogError(diagnostic::NO_MATCHING_SIG, {signatureKind, msg.c_str()}, pos);
                return;
            }

            msg += ", ";
        }
    }

    LogError(diagnostic::NO_MATCHING_SIG_2, {signatureKind}, pos);
}

Signature *ETSChecker::ValidateSignatures(ArenaVector<Signature *> &signatures,
                                          const ir::TSTypeParameterInstantiation *typeArguments,
                                          const ArenaVector<ir::Expression *> &arguments,
                                          const lexer::SourcePosition &pos, std::string_view signatureKind,
                                          TypeRelationFlag resolveFlags)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto compatibleSignatures = CollectSignatures(signatures, typeArguments, arguments, pos, resolveFlags);
    if (!compatibleSignatures.empty()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *sig = GetMostSpecificSignature(compatibleSignatures, arguments, pos, resolveFlags);
        // NOTE (DZ):   skip Promise<T> constructor/then/catch check -
        //              temporary solution, need to be removed after fixing OHOS code!
        if (sig == nullptr || !sig->HasFunction() ||
            !(sig->Function()->IsConstructor() || sig->Function()->Id()->Name().Is("then") ||
              sig->Function()->Id()->Name().Is("catch")) ||
            !sig->Owner()->Name().Is("Promise")) {
            // May need to re-check the arguments now that we know the particular signature to call.
            ValidateSignature({sig, nullptr, TypeRelationFlag::WIDENING | TypeRelationFlag::NO_SUBSTITUTION_NEEDED},
                              arguments, pos, FindTypeInferenceArguments(arguments), true);
        }
        return sig;
    }

    if ((resolveFlags & TypeRelationFlag::NO_THROW) == 0) {
        ThrowSignatureMismatch(signatures, arguments, pos, signatureKind);
    }

    return nullptr;
}

//  Excluded from 'FindMostSpecificSignature' to reduce its size due to code-style check
static std::size_t GetParameterNumber(Signature const *const sig)
{
    if (sig->HasFunction()) {
        return sig->Function()->Params().size();
    }
    auto num = sig->Params().size();
    return !sig->HasRestParameter() ? num : ++num;
}

Signature *ETSChecker::FindMostSpecificSignature(const ArenaVector<Signature *> &signatures,
                                                 const ArenaMultiMap<size_t, Signature *> &bestSignaturesForParameter,
                                                 size_t paramCount)
{
    auto isMostSpecificForAllParams = [&](const Signature *sig) {
        for (size_t paramIdx = 0; paramIdx < paramCount; ++paramIdx) {
            const auto [begin, end] = bestSignaturesForParameter.equal_range(paramIdx);
            if (std::none_of(begin, end, [sig](auto &entry) { return entry.second == sig; })) {
                return false;
            }
        }
        return true;
    };

    auto isGeneric = [](const Signature *sig) { return sig->TypeParams().empty(); };

    Signature *result = nullptr;
    size_t currentMinLength = SIZE_MAX;

    for (auto *candidate : signatures) {
        if (!isMostSpecificForAllParams(candidate)) {
            continue;
        }

        const auto candidateLength = GetParameterNumber(candidate);
        if (candidateLength > currentMinLength && !candidate->HasRestParameter()) {
            continue;
        }

        if (result == nullptr) {
            result = candidate;  // First valid candidate
            currentMinLength = GetParameterNumber(result);
            continue;
        }

        const auto currentLength = GetParameterNumber(result);
        if (candidate->HasRestParameter() && result->HasRestParameter()) {
            if (result->Owner() == candidate->Owner()) {
                result = nullptr;
            }
        } else if (candidateLength < currentLength) {
            result = candidate;  // Shorter parameter count wins
            currentMinLength = GetParameterNumber(result);
        } else if (candidateLength >= currentLength) {
            continue;
            // NOTE (smartin): all other cases below are unreachable code
        } else if (!isGeneric(candidate) && isGeneric(result)) {
            result = candidate;
        } else if (isGeneric(candidate) && !isGeneric(result)) {
            continue;
        } else {
            // Ambiguous resolution for same-length params, same genericity
            if (result->Owner() == candidate->Owner()) {
                result = nullptr;
            }
        }
    }

    return result;
}

static Type *GetParameterTypeOrRestAtIdx(checker::ETSChecker *checker, Signature *sig, const size_t idx)
{
    return idx < sig->ArgCount()
               ? sig->Params().at(idx)->TsType()->MaybeBaseTypeOfGradualType()
               : checker->GetElementTypeOfArray(sig->RestVar()->TsType())->MaybeBaseTypeOfGradualType();
}

static void InitMostSpecificType(TypeRelation *relation, const ArenaVector<Signature *> &signatures,
                                 Type *&mostSpecificType, Signature *&prevSig, const size_t idx)
{
    // Attempt to choose the widest type of available ones
    SavedTypeRelationFlagsContext ctx {relation, TypeRelationFlag::WIDENING | TypeRelationFlag::ONLY_CHECK_WIDENING};
    auto checker = relation->GetChecker()->AsETSChecker();
    for (auto *sig : signatures) {
        Type *sigType = GetParameterTypeOrRestAtIdx(checker, sig, idx);
        relation->Result(false);

        if (sigType->IsETSObjectType()) {
            if (sigType->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::INTERFACE)) {
                continue;
            }
            if (!sigType->AsETSObjectType()->IsBoxedPrimitive()) {
                // Found "non-primitive" ref type
                mostSpecificType = sigType;
                prevSig = sig;
                return;
            }
            relation->SetNode(prevSig->Function()->Params()[idx]->AsETSParameterExpression());
            if (relation->IsLegalBoxedPrimitiveConversion(sigType, mostSpecificType)) {
                mostSpecificType = sigType;
                prevSig = sig;
                continue;
            }
        }
        if (sigType->IsETSFunctionType() && relation->IsSupertypeOf(sigType, mostSpecificType)) {
            mostSpecificType = sigType;
            prevSig = sig;
            continue;
        }
        relation->Result(false);
        WideningConverter(checker, relation, sigType, mostSpecificType);
        if (relation->IsTrue()) {
            mostSpecificType = sigType;
            prevSig = sig;
            continue;
        }
    }
}

void ETSChecker::CheckAmbiguousCall(Type *&mostSpecificType, Type *sigType, Signature *prevSig, Signature *sig,
                                    const lexer::SourcePosition &pos)
{
    if (((sigType->IsETSObjectType() && mostSpecificType->IsETSObjectType()) ||
         (sigType->IsETSUnionType() && mostSpecificType->IsETSUnionType() &&
          ((Relation()->IsSupertypeOf(sigType->AsETSUnionType(), GlobalETSNullType()) &&
            Relation()->IsSupertypeOf(mostSpecificType->AsETSUnionType(), GlobalETSNullType())) ||
           // CC-OFFNXT(G.FMT.02-CPP) project code style
           (Relation()->IsSupertypeOf(sigType->AsETSUnionType(), GlobalETSUndefinedType()) &&
            Relation()->IsSupertypeOf(mostSpecificType->AsETSUnionType(), GlobalETSUndefinedType()))))) &&
        !Relation()->IsAssignableTo(mostSpecificType, sigType) &&
        !Relation()->IsLegalBoxedPrimitiveConversion(sigType, mostSpecificType)) {
        auto funcName = sig->Function()->Id()->Name();
        LogError(diagnostic::AMBIGUOUS_CALL, {funcName, funcName, funcName, prevSig, funcName, sig}, pos);
    }
}

void ETSChecker::SearchAmongMostSpecificTypes(Type *&mostSpecificType, Signature *&prevSig,
                                              std::tuple<const lexer::SourcePosition &, size_t, Signature *> info,
                                              bool lookForClassType)
{
    auto [pos, idx, sig] = info;
    Type *sigType = GetParameterTypeOrRestAtIdx(this, sig, idx);
    if (prevSig->HasFunction() && prevSig->Function()->Params()[idx]->IsETSParameterExpression()) {
        Relation()->SetNode(prevSig->Function()->Params()[idx]->AsETSParameterExpression());
    }
    const bool isClassType =
        sigType->IsETSObjectType() && !sigType->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::INTERFACE);
    if (isClassType == lookForClassType) {
        if (Relation()->IsIdenticalTo(sigType, mostSpecificType)) {
            Relation()->SetNode(nullptr);
            return;
        }

        if (idx >= prevSig->MinArgCount() && idx < sig->MinArgCount()) {
            // NOTE (smartin): prefer non-optional parameters over optional ones
            Relation()->Result(true);
            mostSpecificType = sigType;
            prevSig = sig;
            return;
        }

        if (isClassType && sigType->AsETSObjectType()->IsBoxedPrimitive() && mostSpecificType->IsETSObjectType() &&
            mostSpecificType->AsETSObjectType()->IsBoxedPrimitive()) {
            // NOTE (smartin): when a param with type int is available, make it more specific than other primitive
            // types. The making of correct rules for this is still in progress in spec, so this is a temp solution.
            if (mostSpecificType->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::BUILTIN_INT)) {
                return;
            }

            TypeRelationFlag flags = TypeRelationFlag::NO_THROW | TypeRelationFlag::UNBOXING |
                                     TypeRelationFlag::BOXING | TypeRelationFlag::WIDENING;
            Relation()->SetFlags(flags);
            if (sigType->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::BUILTIN_INT) ||
                Relation()->IsLegalBoxedPrimitiveConversion(mostSpecificType, sigType)) {
                Relation()->Result(true);
                mostSpecificType = sigType;
                prevSig = sig;
                return;
            }
        }
        if (Relation()->IsAssignableTo(sigType, mostSpecificType)) {
            mostSpecificType = sigType;
            prevSig = sig;
        } else {
            CheckAmbiguousCall(mostSpecificType, sigType, prevSig, sig, pos);
        }
    }
}

void ETSChecker::CollectSuitableSignaturesForTypeInference(
    size_t paramIdx, ArenaVector<Signature *> &signatures,
    ArenaMultiMap<size_t, Signature *> &bestSignaturesForParameter, const ArenaVector<ir::Expression *> &arguments)
{
    // For lambda parameters, attempt to obtain the most matching signature through the number of lambda parameters
    ES2PANDA_ASSERT(arguments.at(paramIdx)->IsArrowFunctionExpression());
    [[maybe_unused]] size_t paramCount =
        arguments.at(paramIdx)->AsArrowFunctionExpression()->Function()->Params().size();
    size_t minMatchArgCount = SIZE_MAX;

    for (auto *sig : signatures) {
        auto *sigParamType = GetNonNullishType(sig->Params().at(paramIdx)->TsType());
        ES2PANDA_ASSERT(sigParamType != nullptr);
        if (!sigParamType->IsETSFunctionType()) {
            continue;
        }

        auto sigParamArgCount = sigParamType->AsETSFunctionType()->ArrowSignature()->ArgCount();
        ES2PANDA_ASSERT(sigParamArgCount >= paramCount);

        minMatchArgCount = std::min(minMatchArgCount, sigParamArgCount);
    }

    for (auto *sig : signatures) {
        auto *sigParamType = GetNonNullishType(sig->Params().at(paramIdx)->TsType());
        ES2PANDA_ASSERT(sigParamType != nullptr);
        if (!sigParamType->IsETSFunctionType()) {
            continue;
        }

        if (sigParamType->AsETSFunctionType()->ArrowSignature()->ArgCount() == minMatchArgCount) {
            bestSignaturesForParameter.insert({paramIdx, sig});
        }
    }

    if (bestSignaturesForParameter.find(paramIdx) != bestSignaturesForParameter.end()) {
        return;
    }

    for (auto *sig : signatures) {
        auto paramType = sig->Params().at(paramIdx)->TsType()->MaybeBaseTypeOfGradualType();
        if (paramIdx >= sig->Params().size() || !paramType->IsETSObjectType() ||
            !paramType->AsETSObjectType()->IsGlobalETSObjectType()) {
            bestSignaturesForParameter.insert({paramIdx, sig});
        }
    }
}

ArenaMultiMap<size_t, Signature *> ETSChecker::GetSuitableSignaturesForParameter(
    const std::vector<bool> &argTypeInferenceRequired, size_t paramCount, ArenaVector<Signature *> &signatures,
    const ArenaVector<ir::Expression *> &arguments, const lexer::SourcePosition &pos)
{
    // Collect which signatures are most specific for each parameter.
    ArenaMultiMap<size_t /* parameter index */, Signature *> bestSignaturesForParameter(ProgramAllocator()->Adapter());

    const checker::SavedTypeRelationFlagsContext savedTypeRelationFlagCtx(Relation(),
                                                                          TypeRelationFlag::ONLY_CHECK_WIDENING);

    for (size_t i = 0; i < paramCount; ++i) {
        if (i >= argTypeInferenceRequired.size()) {
            for (auto *sig : signatures) {
                bestSignaturesForParameter.insert({i, sig});
            }
            continue;
        }
        if (argTypeInferenceRequired[i]) {
            CollectSuitableSignaturesForTypeInference(i, signatures, bestSignaturesForParameter, arguments);
            continue;
        }
        // 1st step: check which is the most specific parameter type for i. parameter.
        Type *mostSpecificType = signatures.front()->Params().at(i)->TsType();
        Signature *prevSig = signatures.front();

        // NOTE: first we choose the some signature with possibly widest argumetns' types
        // Then we search for the most specific signature
        InitMostSpecificType(Relation(), signatures, mostSpecificType, prevSig, i);
        for (auto *sig : signatures) {
            SearchAmongMostSpecificTypes(mostSpecificType, prevSig, std::make_tuple(pos, i, sig), true);
        }
        for (auto *sig : signatures) {
            SearchAmongMostSpecificTypes(mostSpecificType, prevSig, std::make_tuple(pos, i, sig), false);
        }

        for (auto *sig : signatures) {
            Type *sigType = GetParameterTypeOrRestAtIdx(this, sig, i);
            if (Relation()->IsIdenticalTo(sigType, mostSpecificType) ||
                (sigType->IsETSFunctionType() && Relation()->IsSupertypeOf(sigType, mostSpecificType))) {
                bestSignaturesForParameter.insert({i, sig});
            }
        }
    }
    return bestSignaturesForParameter;
}

Signature *ETSChecker::ChooseMostSpecificSignature(ArenaVector<Signature *> &signatures,
                                                   const std::vector<bool> &argTypeInferenceRequired,
                                                   const ArenaVector<ir::Expression *> &arguments,
                                                   const lexer::SourcePosition &pos, size_t argumentsSize)
{
    ES2PANDA_ASSERT(signatures.empty() == false);

    if (signatures.size() == 1) {
        return signatures.front();
    }

    std::sort(signatures.begin(), signatures.end(),
              [](Signature *sig1, Signature *sig2) { return sig1->ArgCount() > sig2->ArgCount(); });

    size_t paramCount = signatures.front()->ArgCount();
    if (argumentsSize != ULONG_MAX) {
        paramCount = argumentsSize;
    }
    // Multiple signatures with zero parameter because of inheritance.
    // Return the closest one in inheritance chain that is defined at the beginning of the vector.
    if (paramCount == 0) {
        auto zeroParamSignature = std::find_if(signatures.begin(), signatures.end(),
                                               [](auto *signature) { return signature->RestVar() == nullptr; });
        // If there is a zero parameter signature, return that
        if (zeroParamSignature != signatures.end()) {
            return *zeroParamSignature;
        }
        // If there are multiple rest parameter signatures with different argument types, throw error
        if (signatures.size() > 1 &&
            std::any_of(signatures.begin(), signatures.end(), [signatures, this](const auto *param) {
                auto left = MaybeBoxType(GetElementTypeOfArray(param->RestVar()->TsType()));
                auto right = MaybeBoxType(GetElementTypeOfArray(signatures.front()->RestVar()->TsType()));
                Relation()->IsIdenticalTo(left, right);
                return !Relation()->IsTrue();
            })) {
            LogError(diagnostic::AMBIGUOUS_CALL_2, {signatures.front()->Function()->Id()->Name()}, pos);
            return nullptr;
        }
        // Else return the signature with the rest parameter
        auto restParamSignature = std::find_if(signatures.begin(), signatures.end(),
                                               [](auto *signature) { return signature->RestVar() != nullptr; });
        return *restParamSignature;
    }

    ArenaMultiMap<size_t /* parameter index */, Signature *> bestSignaturesForParameter =
        GetSuitableSignaturesForParameter(argTypeInferenceRequired, paramCount, signatures, arguments, pos);
    // Find the signature that are most specific for all parameters.
    Signature *mostSpecificSignature = FindMostSpecificSignature(signatures, bestSignaturesForParameter, paramCount);

    return mostSpecificSignature;
}

static bool IsLastParameterLambdaWithReceiver(Signature const *sig)
{
    auto const &params = sig->Function()->Params();

    return !params.empty() && (params.back()->AsETSParameterExpression()->TypeAnnotation() != nullptr) &&
           params.back()->AsETSParameterExpression()->TypeAnnotation()->IsETSFunctionType() &&
           params.back()->AsETSParameterExpression()->TypeAnnotation()->AsETSFunctionType()->IsExtensionFunction();
}

Signature *ETSChecker::ResolvePotentialTrailingLambdaWithReceiver(ir::CallExpression *callExpr,
                                                                  ArenaVector<Signature *> const &signatures,
                                                                  ArenaVector<ir::Expression *> &arguments)
{
    auto *trailingLambda = arguments.back()->AsArrowFunctionExpression();
    ArenaVector<Signature *> normalSig(ProgramAllocator()->Adapter());
    ArenaVector<Signature *> sigContainLambdaWithReceiverAsParam(ProgramAllocator()->Adapter());
    Signature *signature = nullptr;
    for (auto sig : signatures) {
        if (!sig->HasFunction()) {
            continue;
        }

        if (!IsLastParameterLambdaWithReceiver(sig)) {
            normalSig.emplace_back(sig);
            continue;
        }

        auto *candidateFunctionType =
            sig->Function()->Params().back()->AsETSParameterExpression()->TypeAnnotation()->AsETSFunctionType();
        auto *currentReceiver = candidateFunctionType->Params()[0];
        trailingLambda->Function()->EmplaceParams(currentReceiver);
        sigContainLambdaWithReceiverAsParam.emplace_back(sig);
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        signature = ValidateSignatures(sigContainLambdaWithReceiverAsParam, callExpr->TypeParams(), arguments,
                                       callExpr->Start(), "call",
                                       TypeRelationFlag::NO_THROW | TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA);
        if (signature != nullptr) {
            return signature;
        }
        sigContainLambdaWithReceiverAsParam.clear();
        trailingLambda->Function()->ClearParams();
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return ValidateSignatures(normalSig, callExpr->TypeParams(), arguments, callExpr->Start(), "call",
                              TypeRelationFlag::NO_THROW | TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA);
}

void ETSChecker::UpdateDeclarationFromSignature(ir::CallExpression *expr, checker::Signature *signature)
{
    if (signature == nullptr) {
        return;
    }

    ir::AstNode *callIdentifier = expr->Callee();
    while (callIdentifier != nullptr && callIdentifier->IsMemberExpression()) {
        callIdentifier = callIdentifier->AsMemberExpression()->Property();
    }
    if (callIdentifier == nullptr || !callIdentifier->IsIdentifier()) {
        return;
    }

    auto signatureVar = callIdentifier->Variable();
    if (signatureVar == nullptr || !signatureVar->HasFlag(varbinder::VariableFlags::METHOD) ||
        !signature->HasFunction() || signature->Function()->IsDynamic()) {
        return;
    }

    auto sigName = signature->Function()->Id()->Name();
    if (callIdentifier->AsIdentifier()->Name() != sigName) {
        return;
    }

    ir::AstNode *declNode = signature->Function();
    while (!declNode->IsMethodDefinition()) {
        declNode = declNode->Parent();
    }
    auto allocator = ProgramAllocator();
    auto newDecl = allocator->New<varbinder::FunctionDecl>(allocator, sigName, declNode);
    auto newVar = allocator->New<varbinder::LocalVariable>(newDecl, varbinder::VariableFlags::METHOD |
                                                                        varbinder::VariableFlags::SYNTHETIC);
    newVar->SetTsType(declNode->AsMethodDefinition()->TsType()->Clone(this));
    callIdentifier->SetVariable(newVar);
}

Signature *ETSChecker::MakeSignatureInvocable(Signature *sig, ir::CallExpression *callExpr)
{
    if (sig == nullptr) {
        return nullptr;
    }
    std::size_t const argumentCount = callExpr->Arguments().size();
    std::size_t const parameterCount = sig->Params().size();
    auto count = std::min(parameterCount, argumentCount);
    for (std::size_t idx = 0; idx < count; ++idx) {
        // Kludge to make promise code compile
        if (callExpr->Arguments().at(idx)->IsArrowFunctionExpression()) {
            continue;
        }

        auto ctx = checker::AssignmentContext(
            Relation(), callExpr->Arguments().at(idx), callExpr->Arguments().at(idx)->TsType(),
            sig->Params().at(idx)->TsType(), callExpr->Arguments().at(idx)->Start(),
            {{diagnostic::INVALID_ASSIGNMNENT,
              {callExpr->Arguments().at(idx)->TsType(), sig->Params().at(idx)->TsType()}}});
        if (!ctx.IsAssignable()) {
            return nullptr;
        }
    }
    return sig;
}

Signature *ETSChecker::ResolveCallExpressionAndTrailingLambda(ArenaVector<Signature *> &signatures,
                                                              ir::CallExpression *callExpr,
                                                              const lexer::SourcePosition &pos,
                                                              const TypeRelationFlag reportFlag)
{
    if (callExpr->TrailingBlock() == nullptr) {
        auto sig =
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            ValidateSignatures(signatures, callExpr->TypeParams(), callExpr->Arguments(), pos, "call", reportFlag);
        sig = MakeSignatureInvocable(sig, callExpr);
        UpdateDeclarationFromSignature(callExpr, sig);
        return sig;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto arguments = ExtendArgumentsWithFakeLamda(callExpr);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto sig = ResolvePotentialTrailingLambdaWithReceiver(callExpr, signatures, arguments);
    if (sig != nullptr) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        TransformTraillingLambda(callExpr, sig);
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        TrailingLambdaTypeInference(sig, callExpr->Arguments());
        UpdateDeclarationFromSignature(callExpr, sig);
        callExpr->SetIsTrailingCall(true);
        return sig;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    sig = ValidateSignatures(signatures, callExpr->TypeParams(), callExpr->Arguments(), pos, "call", reportFlag);
    sig = MakeSignatureInvocable(sig, callExpr);
    if (sig != nullptr) {
        EnsureValidCurlyBrace(callExpr);
    }

    UpdateDeclarationFromSignature(callExpr, sig);
    return sig;
}

Signature *ETSChecker::ResolveConstructExpression(ETSObjectType *type, const ArenaVector<ir::Expression *> &arguments,
                                                  const lexer::SourcePosition &pos)
{
    auto *var = type->GetProperty(compiler::Signatures::CONSTRUCTOR_NAME, PropertySearchFlags::SEARCH_STATIC_METHOD);
    if (var != nullptr && var->TsType()->IsETSFunctionType()) {
        auto sig = MatchOrderSignatures(var->TsType()->AsETSFunctionType()->CallSignatures(), nullptr, arguments, pos,
                                        TypeRelationFlag::NONE);
        if (sig == nullptr) {
            ThrowOverloadMismatch(type->Name(), arguments, pos, "construct");
        }
        return sig;
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return ValidateSignatures(type->ConstructSignatures(), nullptr, arguments, pos, "construct");
}

/*
 * Object literals do not get checked in the process of call resolution; we need to check them separately
 * afterwards.
 */
void ETSChecker::CheckObjectLiteralArguments(Signature *signature, ArenaVector<ir::Expression *> const &arguments)
{
    for (uint32_t index = 0; index < arguments.size(); index++) {
        if (!arguments[index]->IsObjectExpression()) {
            continue;
        }

        Type *tp;
        if (index >= signature->Params().size()) {
            ES2PANDA_ASSERT(signature->RestVar());
            // Use element type as rest object literal type
            tp = GetElementTypeOfArray(signature->RestVar()->TsType());
        } else {
            // #22952: infer optional parameter heuristics
            tp = GetNonNullishType(signature->Params()[index]->TsType());
        }

        arguments[index]->SetPreferredType(tp);
        arguments[index]->Check(this);
    }
}

// Note: this function is extracted to reduce the size of `BuildMethodSignature`
static bool CollectOverload(checker::ETSChecker *checker, ir::MethodDefinition *method, ETSFunctionType *funcType)
{
    ir::OverloadInfo &ldInfo = method->GetOverloadInfoForUpdate();
    ArenaVector<ETSFunctionType *> overloads(checker->ProgramAllocator()->Adapter());

    for (ir::MethodDefinition *const currentFunc : method->Overloads()) {
        if (currentFunc->IsDeclare() != ldInfo.isDeclare) {
            checker->LogError(diagnostic::AMBIGUOUS_AMBIENT, {currentFunc->Id()->Name()}, currentFunc->Start());
            method->Id()->Variable()->SetTsType(checker->GlobalTypeError());
            return false;
        }
        ES2PANDA_ASSERT(currentFunc->Function() != nullptr);
        ES2PANDA_ASSERT(currentFunc->Id() != nullptr);
        currentFunc->Function()->Id()->SetVariable(currentFunc->Id()->Variable());
        checker->BuildFunctionSignature(currentFunc->Function(), method->IsConstructor());
        if (currentFunc->Function()->Signature() == nullptr) {
            auto *methodId = method->Id();
            ES2PANDA_ASSERT(methodId != nullptr);
            methodId->Variable()->SetTsType(checker->GlobalTypeError());
            return false;
        }

        auto *const overloadType = currentFunc->TsType() != nullptr ? currentFunc->TsType()->AsETSFunctionType()
                                                                    : checker->BuildMethodType(currentFunc->Function());
        ldInfo.needHelperOverload |=
            checker->CheckIdenticalOverloads(funcType, overloadType, currentFunc, ldInfo.isDeclare);

        if (currentFunc->TsType() == nullptr) {
            currentFunc->SetTsType(overloadType);
        }

        auto overloadSig = currentFunc->Function()->Signature();
        funcType->AddCallSignature(overloadSig);
        if (overloadSig->IsExtensionAccessor()) {
            funcType->GetExtensionAccessorSigs().emplace_back(overloadSig);
        } else if (overloadSig->IsExtensionFunction()) {
            funcType->GetExtensionFunctionSigs().emplace_back(overloadSig);
        }
        overloads.push_back(overloadType);

        ldInfo.minArg = std::min(ldInfo.minArg, currentFunc->Function()->Signature()->MinArgCount());
        ldInfo.maxArg = std::max(ldInfo.maxArg, currentFunc->Function()->Signature()->ArgCount());
        ldInfo.hasRestVar |= (currentFunc->Function()->Signature()->RestVar() != nullptr);
        ldInfo.returnVoid |= currentFunc->Function()->Signature()->ReturnType()->IsETSVoidType();
    }

    for (size_t baseFuncCounter = 0; baseFuncCounter < overloads.size(); ++baseFuncCounter) {
        auto *overloadType = overloads.at(baseFuncCounter);
        for (size_t compareFuncCounter = baseFuncCounter + 1; compareFuncCounter < overloads.size();
             compareFuncCounter++) {
            auto *compareOverloadType = overloads.at(compareFuncCounter);
            ldInfo.needHelperOverload |= checker->CheckIdenticalOverloads(
                overloadType, compareOverloadType, method->Overloads()[compareFuncCounter], ldInfo.isDeclare);
        }
    }
    return true;
}

checker::Type *ETSChecker::BuildMethodSignature(ir::MethodDefinition *method)
{
    if (method->TsType() != nullptr) {
        return method->TsType()->AsETSFunctionType();
    }
    auto *methodId = method->Id();
    ES2PANDA_ASSERT(methodId != nullptr);
    ES2PANDA_ASSERT(method->Function() != nullptr);
    if (methodId->AsIdentifier()->IsErrorPlaceHolder()) {
        return methodId->Variable()->SetTsType(GlobalTypeError());
    }
    method->Function()->Id()->SetVariable(methodId->Variable());
    BuildFunctionSignature(method->Function(), method->IsConstructor());
    if (method->Function()->Signature() == nullptr) {
        return methodId->Variable()->SetTsType(GlobalTypeError());
    }
    auto *funcType = BuildMethodType(method->Function());
    method->InitializeOverloadInfo();
    if (!CollectOverload(this, method, funcType)) {
        return GlobalTypeError();
    }
    ir::OverloadInfo &ldInfo = method->GetOverloadInfoForUpdate();

    ldInfo.needHelperOverload &= ldInfo.isDeclare;
    if (ldInfo.needHelperOverload) {
        LogDiagnostic(diagnostic::FUNCTION_ASM_SIG_COLLISION, {std::string(funcType->Name())}, method->Start());
    }

    return methodId->Variable()->SetTsType(funcType);
}

bool ETSChecker::CheckIdenticalOverloads(ETSFunctionType *func, ETSFunctionType *overload,
                                         const ir::MethodDefinition *const currentFunc, bool omitSameAsm,
                                         TypeRelationFlag relationFlags)
{
    // Don't necessary to check overload for invalid functions
    if (func->Name().Is(ERROR_LITERAL)) {
        ES2PANDA_ASSERT(IsAnyError());
        return false;
    }

    SavedTypeRelationFlagsContext savedFlagsCtx(Relation(), relationFlags);

    Relation()->SignatureIsIdenticalTo(func->CallSignatures()[0], overload->CallSignatures()[0]);
    if (Relation()->IsTrue() && func->CallSignatures()[0]->GetSignatureInfo()->restVar ==
                                    overload->CallSignatures()[0]->GetSignatureInfo()->restVar) {
        LogError(diagnostic::FUNCTION_REDECL_BY_TYPE_SIG, {func->Name().Mutf8()}, currentFunc->Start());
        return false;
    }

    if (!HasSameAssemblySignatures(func, overload)) {
        return false;
    }

    if (!omitSameAsm) {
        LogError(diagnostic::FUNCTION_REDECL_BY_ASM_SIG, {func->Name().Mutf8()}, currentFunc->Start());
        return false;
    }

    func->CallSignatures()[0]->AddSignatureFlag(SignatureFlags::DUPLICATE_ASM);
    overload->CallSignatures()[0]->AddSignatureFlag(SignatureFlags::DUPLICATE_ASM);

    return true;
}

Signature *ETSChecker::ComposeSignature(ir::ScriptFunction *func, SignatureInfo *signatureInfo, Type *returnType,
                                        varbinder::Variable *nameVar)
{
    auto *signature = CreateSignature(signatureInfo, returnType, func);
    if (signature == nullptr) {  // #23134
        ES2PANDA_ASSERT(IsAnyError());
        return nullptr;
    }
    signature->SetOwner(Context().ContainingClass());
    signature->SetOwnerVar(nameVar);

    const auto *returnTypeAnnotation = func->ReturnTypeAnnotation();
    if (returnTypeAnnotation == nullptr && ((func->Flags() & ir::ScriptFunctionFlags::HAS_RETURN) != 0)) {
        signature->AddSignatureFlag(SignatureFlags::NEED_RETURN_TYPE);
    }

    if (returnTypeAnnotation != nullptr && returnTypeAnnotation->IsTSThisType()) {
        // #22951: the original signature retains the arbitrary this type
        // (sometimes ETSGLOBAL). should be resolved woth proper `this` functions support
        signature->AddSignatureFlag(SignatureFlags::THIS_RETURN_TYPE);
    }

    if (signature->Owner() != nullptr && signature->Owner()->GetDeclNode()->IsFinal()) {
        signature->AddSignatureFlag(SignatureFlags::FINAL);
    }
    return signature;
}

Type *ETSChecker::ComposeReturnType(ir::TypeNode *typeAnnotation, bool isAsync)
{
    if (typeAnnotation != nullptr) {
        return typeAnnotation->GetType(this);
    }
    return isAsync ? CreatePromiseOf(GlobalVoidType()) : GlobalVoidType();
}

static varbinder::LocalVariable *SetupSignatureParameter(ir::ETSParameterExpression *param, Type *type)
{
    auto *const variable = param->Ident()->Variable();  // #23134
    if (variable == nullptr) {
        return nullptr;
    }
    param->Ident()->SetTsType(type);
    variable->SetTsType(type);
    return variable->AsLocalVariable();
}

// Should be moved to original ComposeSignatureInfo after AST fix
static bool AppendSignatureInfoParam(ETSChecker *checker, SignatureInfo *sigInfo, ir::ETSParameterExpression *param)
{
    auto variable = SetupSignatureParameter(param, [checker, param]() {
        if (param->TypeAnnotation() != nullptr) {
            auto type = param->TypeAnnotation()->GetType(checker);
            return param->IsOptional() ? checker->CreateETSUnionType({type, checker->GlobalETSUndefinedType()}) : type;
        }
        if (param->Ident()->TsType() != nullptr) {
            return param->Ident()->TsType();
        }

        if (!param->Ident()->IsErrorPlaceHolder() && !checker->HasStatus(checker::CheckerStatus::IN_TYPE_INFER)) {
            checker->LogError(diagnostic::INFER_FAILURE_FUNC_PARAM, {param->Ident()->Name()}, param->Start());
        }

        return checker->GlobalTypeError();
    }());
    if (variable == nullptr) {  // #23134
        return false;
    }
    if (param->IsRestParameter()) {
        return true;
    }

    sigInfo->params.push_back(variable);
    if (!param->IsOptional()) {
        ++sigInfo->minArgCount;
    }
    ERROR_SANITY_CHECK(
        checker,
        !param->IsOptional() || param->Ident()->TsType()->IsTypeError() ||
            checker->Relation()->IsSupertypeOf(param->Ident()->TsType(), checker->GlobalETSUndefinedType()),
        return false);
    return true;
}

SignatureInfo *ETSChecker::ComposeSignatureInfo(ir::TSTypeParameterDeclaration *typeParams,
                                                ArenaVector<ir::Expression *> const &params)
{
    auto *const signatureInfo = CreateSignatureInfo();

    if (typeParams != nullptr) {
        auto [typeParamTypes, ok] = CreateUnconstrainedTypeParameters(typeParams);
        ES2PANDA_ASSERT(signatureInfo != nullptr);
        signatureInfo->typeParams = std::move(typeParamTypes);
        if (ok) {
            AssignTypeParameterConstraints(typeParams);
        }
    }

    for (auto *const p : params) {
        if (!p->IsETSParameterExpression() ||
            !AppendSignatureInfoParam(this, signatureInfo, p->AsETSParameterExpression())) {  // #23134
            ES2PANDA_ASSERT(IsAnyError());
            return nullptr;
        }
    }

    if (!params.empty()) {
        if (auto param = params.back()->AsETSParameterExpression(); param->IsRestParameter()) {
            checker::Type *restParamType = nullptr;
            if (param->TypeAnnotation() != nullptr) {
                restParamType = param->RestParameter()->TypeAnnotation()->GetType(this);
            } else if (param->Ident()->TsType() != nullptr) {
                restParamType = param->Ident()->TsType();
            } else {
                ES2PANDA_ASSERT(IsAnyError());  // #23134
                return nullptr;
            }
            ES2PANDA_ASSERT(restParamType != nullptr);
            if (!restParamType->IsAnyETSArrayOrTupleType()) {
                LogError(diagnostic::ONLY_ARRAY_OR_TUPLE_FOR_REST, {}, param->Start());
                restParamType = GlobalTypeError();
            }
            signatureInfo->restVar = SetupSignatureParameter(param, restParamType);
            ES2PANDA_ASSERT(signatureInfo->restVar != nullptr);
        }
    }

    return signatureInfo;
}

void ETSChecker::ValidateMainSignature(ir::ScriptFunction *func)
{
    if (func->Params().size() >= 2U) {
        LogError(diagnostic::MAIN_INVALID_ARG_COUNT, {}, func->Start());
        return;
    }

    if (func->Params().size() == 1) {
        auto const *const param = func->Params()[0]->AsETSParameterExpression();

        if (param->IsRestParameter()) {
            LogError(diagnostic::MAIN_WITH_REST, {}, param->Start());
        }

        const auto paramType = param->Variable()->TsType();
        if (!paramType->IsETSArrayType() || !paramType->AsETSArrayType()->ElementType()->IsETSStringType()) {
            LogError(diagnostic::MAIN_PARAM_NOT_ARR_OF_STRING, {}, param->Start());
        }
    }
}

void ETSChecker::BuildFunctionSignature(ir::ScriptFunction *func, bool isConstructSig)
{
    ES2PANDA_ASSERT(func != nullptr);
    bool isArrow = func->IsArrow();
    // note(Ekko): For extenal function overload, need to not change ast tree, for arrow type, need perferred type.
    if (func->Signature() != nullptr && !isArrow) {
        return;
    }
    auto *nameVar = isArrow ? nullptr : func->Id()->Variable();
    auto funcName = nameVar == nullptr ? util::StringView() : nameVar->Name();

    if (func->IsConstructor() && func->IsStatic()) {
        LogError(diagnostic::INVALID_DECORATOR_CONSTRUCTOR, {}, func->Start());
        return;
    }

    if ((func->IsConstructor() || !func->IsStatic()) && !func->IsArrow()) {
        auto thisVar = func->Scope()->ParamScope()->Params().front();
        thisVar->SetTsType(Context().ContainingClass());
    }
    auto *signatureInfo = ComposeSignatureInfo(func->TypeParams(), func->Params());
    auto *returnType = func->GetPreferredReturnType() != nullptr
                           ? func->GetPreferredReturnType()
                           : ComposeReturnType(func->ReturnTypeAnnotation(), func->IsAsyncFunc());
    auto *signature = ComposeSignature(func, signatureInfo, returnType, nameVar);
    if (signature == nullptr) {  // #23134
        ES2PANDA_ASSERT(IsAnyError());
        return;
    }

    func->SetSignature(signature);

    if (isConstructSig) {
        signature->AddSignatureFlag(SignatureFlags::CONSTRUCT);
    } else {
        signature->AddSignatureFlag(SignatureFlags::CALL);
    }

    if (funcName.Is(compiler::Signatures::MAIN) &&
        func->Scope()->Name().Utf8().find(compiler::Signatures::ETS_GLOBAL) != std::string::npos) {
        func->AddFlag(ir::ScriptFunctionFlags::ENTRY_POINT);
    }
    if (func->IsEntryPoint()) {
        ValidateMainSignature(func);
    }

    VarBinder()->AsETSBinder()->BuildFunctionName(func);
    Program()->AddToFunctionScopes(func->Scope());
}

checker::ETSFunctionType *ETSChecker::BuildMethodType(ir::ScriptFunction *func)
{
    ES2PANDA_ASSERT(!func->IsArrow());
    ES2PANDA_ASSERT(func != nullptr);
    auto *nameVar = func->Id()->Variable();
    ETSFunctionType *funcType =
        CreateETSMethodType(nameVar->Name(), {{func->Signature()}, ProgramAllocator()->Adapter()});
    funcType->SetVariable(nameVar);
    return funcType;
}

Signature *ETSChecker::CheckEveryAbstractSignatureIsOverridden(ETSFunctionType *target, ETSFunctionType *source)
{
    for (auto targetSig = target->CallSignatures().begin(); targetSig != target->CallSignatures().end();) {
        if (!(*targetSig)->HasSignatureFlag(SignatureFlags::ABSTRACT)) {
            continue;
        }

        bool isOverridden = false;
        for (auto sourceSig : source->CallSignatures()) {
            if ((*targetSig)->Function()->Id()->Name() == sourceSig->Function()->Id()->Name() &&
                Relation()->SignatureIsSupertypeOf(*targetSig, sourceSig)) {
                targetSig = target->CallSignatures().erase(targetSig);
                isOverridden = true;
                break;
            }
            sourceSig++;
        }

        if (!isOverridden) {
            return *targetSig;
        }
    }

    return nullptr;
}

bool ETSChecker::IsOverridableIn(Signature *signature)
{
    if (signature->HasSignatureFlag(SignatureFlags::PRIVATE)) {
        return false;
    }

    // NOTE: #15095 workaround, separate internal visibility check
    if (signature->HasSignatureFlag(SignatureFlags::PUBLIC | SignatureFlags::INTERNAL)) {
        return true;
    }

    return signature->HasSignatureFlag(SignatureFlags::PROTECTED);
}

bool ETSChecker::IsMethodOverridesOther(Signature *base, Signature *derived)
{
    if (derived->Function()->IsConstructor()) {
        return false;
    }

    if (base == derived) {
        return true;
    }

    if (derived->HasSignatureFlag(SignatureFlags::STATIC) != base->HasSignatureFlag(SignatureFlags::STATIC)) {
        return false;
    }

    if (IsOverridableIn(base)) {
        SavedTypeRelationFlagsContext savedFlagsCtx(Relation(), TypeRelationFlag::NO_RETURN_TYPE_CHECK |
                                                                    TypeRelationFlag::OVERRIDING_CONTEXT);
        if (Relation()->SignatureIsSupertypeOf(base, derived)) {
            if (derived->HasSignatureFlag(SignatureFlags::STATIC)) {
                return false;
            }

            derived->Function()->SetOverride();
            return true;
        }
    }

    return false;
}

OverrideErrorCode ETSChecker::CheckOverride(Signature *signature, Signature *other)
{
    if (other->HasSignatureFlag(SignatureFlags::STATIC)) {
        ES2PANDA_ASSERT(signature->HasSignatureFlag(SignatureFlags::STATIC));
        return OverrideErrorCode::NO_ERROR;
    }

    if (other->IsFinal()) {
        return OverrideErrorCode::OVERRIDDEN_FINAL;
    }

    auto *ownerNode = signature->Owner()->GetDeclNode();
    auto *superNode = other->Owner()->GetDeclNode();
    bool bothRealClasses = (ownerNode != nullptr) && ownerNode->IsClassDefinition() && (superNode != nullptr) &&
                           superNode->IsClassDefinition() && signature->Owner()->HasObjectFlag(ETSObjectFlags::CLASS) &&
                           other->Owner()->HasObjectFlag(ETSObjectFlags::CLASS);
    if (bothRealClasses && signature->HasSignatureFlag(SignatureFlags::ABSTRACT) &&
        !other->HasSignatureFlag(SignatureFlags::ABSTRACT)) {
        return OverrideErrorCode::ABSTRACT_OVERRIDES_CONCRETE;
    }

    if (!other->ReturnType()->IsETSTypeParameter()) {
        if (!IsReturnTypeSubstitutable(signature, other)) {
            return OverrideErrorCode::INCOMPATIBLE_RETURN;
        }
    } else {
        // We need to have this branch to allow generic overriding of the form:
        // foo<T>(x: T): T -> foo<someClass>(x: someClass): someClass
        if (!signature->ReturnType()->IsETSReferenceType()) {
            return OverrideErrorCode::INCOMPATIBLE_RETURN;
        }
    }

    if (signature->ProtectionFlag() > other->ProtectionFlag()) {
        return OverrideErrorCode::OVERRIDDEN_WEAKER;
    }
    if (signature->HasProtectionFlagInternal() != other->HasProtectionFlagInternal()) {
        return OverrideErrorCode::OVERRIDDEN_INTERNAL;
    }

    return OverrideErrorCode::NO_ERROR;
}

Signature *ETSChecker::AdjustForTypeParameters(Signature *source, Signature *target)
{
    auto &sourceTypeParams = source->GetSignatureInfo()->typeParams;
    auto &targetTypeParams = target->GetSignatureInfo()->typeParams;
    if (sourceTypeParams.size() != targetTypeParams.size()) {
        return nullptr;
    }
    if (sourceTypeParams.empty()) {
        return target;
    }
    auto substitution = Substitution {};
    for (size_t ix = 0; ix < sourceTypeParams.size(); ix++) {
        if (!targetTypeParams[ix]->IsETSTypeParameter()) {
            continue;
        }
        EmplaceSubstituted(&substitution, targetTypeParams[ix]->AsETSTypeParameter(), sourceTypeParams[ix]);
    }
    return target->Substitute(Relation(), &substitution);
}

void ETSChecker::ReportOverrideError(Signature *signature, Signature *overriddenSignature,
                                     const OverrideErrorCode &errorCode)
{
    const char *reason {};
    switch (errorCode) {
        case OverrideErrorCode::OVERRIDDEN_FINAL: {
            reason = "overridden method is final.";
            break;
        }
        case OverrideErrorCode::INCOMPATIBLE_RETURN: {
            reason = "overriding return type is not compatible with the other return type.";
            break;
        }
        case OverrideErrorCode::OVERRIDDEN_WEAKER: {
            reason = "overridden method has weaker access privilege.";
            break;
        }
        case OverrideErrorCode::OVERRIDDEN_INTERNAL: {
            reason =
                "internal members can only be overridden by internal members, "
                "and non-internal members cannot be overridden by internal members.";
            break;
        }
        case OverrideErrorCode::INCOMPATIBLE_TYPEPARAM: {
            reason =
                "overriding type parameter's conatraints are not compatible with type parameter constraints of the "
                "overridden method.";
            break;
        }
        case OverrideErrorCode::ABSTRACT_OVERRIDES_CONCRETE: {
            reason = "an abstract method cannot override a non-abstract instance method.";
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    LogError(diagnostic::CANNOT_OVERRIDE,
             {signature->Function()->Id()->Name(), signature, signature->Owner(),
              overriddenSignature->Function()->Id()->Name(), overriddenSignature, overriddenSignature->Owner(), reason},
             signature->Function()->Start());
}

bool CheckTypeParameterConstraints(ArenaVector<Type *> typeParamList1, ArenaVector<Type *> typeParamList2,
                                   TypeRelation *relation)
{
    if (!typeParamList1.empty() || !typeParamList2.empty()) {
        if (typeParamList1.size() != typeParamList2.size()) {
            return false;
        }
        for (size_t i = 0; i < typeParamList1.size(); i++) {
            auto c1 = typeParamList1[i]->AsETSTypeParameter()->GetConstraintType();
            auto c2 = typeParamList2[i]->AsETSTypeParameter()->GetConstraintType();
            if (!relation->IsSupertypeOf(c1, c2)) {  // contravariance check
                return false;
            }
        }
    }
    return true;
}

bool ETSChecker::CheckOverride(Signature *signature, ETSObjectType *site)
{
    PropertySearchFlags flags =
        PropertySearchFlags::SEARCH_METHOD | PropertySearchFlags::DISALLOW_SYNTHETIC_METHOD_CREATION;
    auto *target = site->GetProperty(signature->Function()->Id()->Name(), flags);
    bool isOverridingAnySignature = false;

    if (target == nullptr || target->TsType() == nullptr || target->TsType()->IsTypeError()) {
        return isOverridingAnySignature;
    }

    for (auto *it : target->TsType()->AsETSFunctionType()->CallSignatures()) {
        bool typeParamError = false;
        if (!CheckTypeParameterConstraints(signature->TypeParams(), it->TypeParams(), Relation())) {
            typeParamError = true;
        }

        auto *itSubst = AdjustForTypeParameters(signature, it);

        if (itSubst == nullptr) {
            continue;
        }

        if (itSubst->HasSignatureFlag(SignatureFlags::ABSTRACT) || site->HasObjectFlag(ETSObjectFlags::INTERFACE)) {
            if ((itSubst->Function()->IsSetter() && !signature->Function()->IsSetter()) ||
                (itSubst->Function()->IsGetter() && !signature->Function()->IsGetter())) {
                continue;
            }
        }

        if (!IsMethodOverridesOther(itSubst, signature)) {
            continue;
        }

        if (typeParamError) {
            ReportOverrideError(signature, it, OverrideErrorCode::INCOMPATIBLE_TYPEPARAM);
            return false;
        }

        if (auto err = CheckOverride(signature, itSubst); err != OverrideErrorCode::NO_ERROR) {
            ReportOverrideError(signature, it, err);
            return false;
        }

        isOverridingAnySignature = true;
    }

    return isOverridingAnySignature;
}

void ETSChecker::CheckOverride(Signature *signature)
{
    ES2PANDA_ASSERT(signature != nullptr);
    auto *owner = signature->Owner();
    ES2PANDA_ASSERT(owner != nullptr);
    bool isOverriding = false;

    if (!owner->HasObjectFlag(ETSObjectFlags::CLASS | ETSObjectFlags::INTERFACE)) {
        return;
    }

    for (auto *const interface : owner->Interfaces()) {
        isOverriding |= CheckInterfaceOverride(this, interface, signature);
    }

    ETSObjectType *iter = owner->SuperType();
    while (iter != nullptr) {
        isOverriding |= CheckOverride(signature, iter);

        for (auto *const interface : iter->Interfaces()) {
            isOverriding |= CheckInterfaceOverride(this, interface, signature);
        }

        iter = iter->SuperType();
    }
    lexer::SourcePosition ownerPos = signature->Owner()->GetDeclNode()->Start();
    lexer::SourcePosition signaturePos = signature->Function()->Start();
    lexer::SourcePosition pos = signaturePos.line == 0 && signaturePos.index == 0 ? ownerPos : signaturePos;
    if (!isOverriding && signature->Function()->IsOverride()) {
        LogError(diagnostic::OVERRIDE_DOESNT_OVERRIDE,
                 {signature->Function()->Id()->Name(), signature, signature->Owner()}, pos);
    }
}

Signature *ETSChecker::GetSignatureFromMethodDefinition(const ir::MethodDefinition *methodDef)
{
    if (methodDef->TsType()->IsTypeError()) {
        return nullptr;
    }
    ES2PANDA_ASSERT_POS(methodDef->TsType() && methodDef->TsType()->IsETSFunctionType(), methodDef->Start());
    for (auto *it : methodDef->TsType()->AsETSFunctionType()->CallSignatures()) {
        if (it->Function() == methodDef->Function()) {
            return it;
        }
    }

    return nullptr;
}

bool ETSChecker::NeedToVerifySignatureVisibility(Signature *signature, const lexer::SourcePosition &pos)
{
    if (signature == nullptr) {
        LogError(diagnostic::SIG_UNAVAILABLE, {}, pos);
        return false;
    }

    return (Context().Status() & CheckerStatus::IGNORE_VISIBILITY) == 0U &&
           (signature->HasSignatureFlag(SignatureFlags::PRIVATE) ||
            signature->HasSignatureFlag(SignatureFlags::PROTECTED));
}

void ETSChecker::ValidateSignatureAccessibility(ETSObjectType *callee, Signature *signature,
                                                const lexer::SourcePosition &pos,
                                                const MaybeDiagnosticInfo &maybeErrorInfo)
{
    if (!NeedToVerifySignatureVisibility(signature, pos)) {
        return;
    }
    const auto *declNode = callee->GetDeclNode();
    auto *containingClass = Context().ContainingClass();
    bool isContainingSignatureInherited = containingClass->IsSignatureInherited(signature);
    ES2PANDA_ASSERT(declNode && (declNode->IsClassDefinition() || declNode->IsTSInterfaceDeclaration()));

    if (declNode->IsTSInterfaceDeclaration()) {
        if (containingClass == declNode->AsTSInterfaceDeclaration()->TsType() && isContainingSignatureInherited) {
            return;
        }
    }
    if (containingClass == declNode->AsClassDefinition()->TsType() && isContainingSignatureInherited) {
        return;
    }

    bool isSignatureInherited = callee->IsSignatureInherited(signature);
    const auto *currentOutermost = containingClass->OutermostClass();
    if (!signature->HasSignatureFlag(SignatureFlags::PRIVATE) &&
        ((signature->HasSignatureFlag(SignatureFlags::PROTECTED) && containingClass->IsDescendantOf(callee)) ||
         (currentOutermost != nullptr && currentOutermost == callee->OutermostClass())) &&
        isSignatureInherited) {
        return;
    }

    if (!maybeErrorInfo.has_value()) {
        LogError(diagnostic::SIG_INVISIBLE, {signature->Function()->Id()->Name(), signature}, pos);
        return;
    }
    const auto [diagnostic, diagnosticParams] = *maybeErrorInfo;
    LogError(diagnostic, diagnosticParams, pos);
}

void ETSChecker::CheckCapturedVariable(ir::AstNode *const node, varbinder::Variable *const var)
{
    if (node->IsIdentifier()) {
        const auto *const parent = node->Parent();

        if (parent->IsUpdateExpression() ||
            (parent->IsAssignmentExpression() && parent->AsAssignmentExpression()->Left() == node)) {
            const auto *const identNode = node->AsIdentifier();

            const auto *resolved = identNode->Variable();

            if (resolved == nullptr) {
                resolved = FindVariableInFunctionScope(identNode->Name());
            }

            if (resolved == nullptr) {
                resolved = FindVariableInGlobal(identNode);
            }

            if (resolved == var) {
                // For mutable captured variable [possible] smart-cast is senseless (or even erroneous)
                Context().RemoveSmartCast(var);
            }
        }
    }

    CheckCapturedVariableInSubnodes(node, var);
}

void ETSChecker::CheckCapturedVariableInSubnodes(ir::AstNode *node, varbinder::Variable *var)
{
    if (!node->IsClassDefinition()) {
        node->Iterate([this, var](ir::AstNode *childNode) { CheckCapturedVariable(childNode, var); });
    }
}

void ETSChecker::CheckCapturedVariables()
{
    // If we want to capture non constant local variables, we should wrap them in a generic reference class
    for (auto [var, _] : Context().CapturedVars()) {
        (void)_;
        if ((var->Declaration() == nullptr) || var->Declaration()->IsConstDecl() ||
            !var->HasFlag(varbinder::VariableFlags::LOCAL) || var->GetScope()->Node()->IsArrowFunctionExpression()) {
            continue;
        }

        auto *searchNode = var->Declaration()->Node()->Parent();

        if (searchNode->IsVariableDeclarator()) {
            searchNode = searchNode->Parent()->Parent();
        }

        CheckCapturedVariableInSubnodes(searchNode, var);
    }
}

bool ETSChecker::AreOverrideCompatible(Signature *const s1, Signature *const s2)
{
    // Two functions, methods or constructors M and N have the same signature if
    // their names and type parameters (if any) are the same, and their formal parameter
    // types are also the same (after the formal parameter types of N are adapted to the type parameters of M).
    // Signatures s1 and s2 are override-equivalent only if s1 and s2 are the same.
    if (s1->Function()->Id()->Name() != s2->Function()->Id()->Name()) {
        return false;
    }

    SavedTypeRelationFlagsContext savedFlagsCtx(Relation(), TypeRelationFlag::OVERRIDING_CONTEXT);
    return Relation()->SignatureIsSupertypeOf(s1, s2);
}

bool ETSChecker::IsReturnTypeSubstitutable(Signature *const s1, Signature *const s2)
{
    if (s2->HasSignatureFlag(checker::SignatureFlags::NEED_RETURN_TYPE)) {
        s2->Function()->Parent()->Parent()->Check(this);
    }
    auto *const r1 = s1->ReturnType();
    auto *const r2 = s2->ReturnType();

    // A method declaration d1 with return type R1 is return-type-substitutable for another method d2 with return
    // type R2 if any of the following is true:

    // NOTE(vpukhov): void type leaks into type arguments, so we have to check the original signature if the return type
    // is parametrized or not to use a proper subtyping check. To be replaced with IsETSPrimitiveType after #19701.
    auto const hasPrimitiveReturnType = [](Signature *s) {
        bool origIsRef = s->Function()->Signature()->ReturnType()->IsETSReferenceType();
        ES2PANDA_ASSERT_POS(origIsRef == s->ReturnType()->IsETSReferenceType(), s->Function()->Start());
        return !origIsRef;
    };
    // - If R1 is a primitive type then R2 is identical to R1.
    if (hasPrimitiveReturnType(s1) || hasPrimitiveReturnType(s2)) {
        return Relation()->IsIdenticalTo(r2, r1);
    }

    auto const hasThisReturnType = [](Signature *s) {
        auto *retAnn = s->Function()->ReturnTypeAnnotation();
        return retAnn != nullptr && retAnn->IsTSThisType();
    };
    // - If S2 is a 'this' type(polymorphic) and S1 must be also 'this'
    // If the overridden method (s2) has a 'this' return type, then the overriding method (s1) must also have it.
    bool s1HasThisType = hasThisReturnType(s1);
    bool s2HasThisType = hasThisReturnType(s2);
    if (!s1HasThisType && s2HasThisType) {
        return false;
    }

    // - If R1 is a reference type then R1, adapted to the type parameters of d2 (link to generic methods), is a
    //   subtype of R2.
    ES2PANDA_ASSERT(IsReferenceType(r1));
    return Relation()->IsSupertypeOf(r2, r1);
}

std::string ETSChecker::GetAsyncImplName(const util::StringView &name)
{
    std::string newName =
        util::NameMangler::GetInstance()->CreateMangledNameByTypeAndName(util::NameMangler::ASYNC, name);
    return newName;
}

std::string ETSChecker::GetAsyncImplName(ir::MethodDefinition *asyncMethod)
{
    ir::ScriptFunction *scriptFunc = asyncMethod->Function();
    CHECK_NOT_NULL(scriptFunc);
    ir::Identifier *asyncName = scriptFunc->Id();
    ES2PANDA_ASSERT_POS(asyncName != nullptr, asyncMethod->Start());
    return GetAsyncImplName(asyncName->Name());
}

ir::MethodDefinition *ETSChecker::CreateMethod(const util::StringView &name, ir::ModifierFlags modifiers,
                                               ir::ScriptFunctionFlags flags, ArenaVector<ir::Expression *> &&params,
                                               varbinder::FunctionParamScope *paramScope, ir::TypeNode *returnType,
                                               ir::AstNode *body)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *nameId = ProgramAllocNode<ir::Identifier>(name, ProgramAllocator());
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *scope = ProgramAllocator()->New<varbinder::FunctionScope>(ProgramAllocator(), paramScope);
    // clang-format off
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *const func = ProgramAllocNode<ir::ScriptFunction>(
        ProgramAllocator(), ir::ScriptFunction::ScriptFunctionData {
            // CC-OFFNXT(G.FMT.05-CPP) project codestyle clang format off
            body, ir::FunctionSignature(nullptr, std::move(params), returnType), flags, modifiers});
    // clang-format on
    ES2PANDA_ASSERT(func != nullptr);
    func->SetScope(scope);
    func->SetIdent(nameId);
    if (body != nullptr && body->IsBlockStatement()) {
        body->AsBlockStatement()->SetScope(scope);
    }
    ES2PANDA_ASSERT(scope != nullptr);
    scope->BindNode(func);
    paramScope->BindNode(func);
    scope->BindParamScope(paramScope);
    paramScope->BindFunctionScope(scope);

    if (!func->IsStatic()) {
        auto classDef = VarBinder()->GetScope()->AsClassScope()->Node()->AsClassDefinition();
        VarBinder()->AsETSBinder()->AddFunctionThisParam(func);
        func->Scope()->Find(varbinder::VarBinder::MANDATORY_PARAM_THIS).variable->SetTsType(classDef->TsType());
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *funcExpr = ProgramAllocNode<ir::FunctionExpression>(func);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *nameClone = nameId->Clone(ProgramAllocator(), nullptr);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *method = util::NodeAllocator::ForceSetParent<ir::MethodDefinition>(
        ProgramAllocator(), ir::MethodDefinitionKind::METHOD, nameClone, funcExpr, modifiers, ProgramAllocator(),
        false);

    return method;
}

varbinder::FunctionParamScope *ETSChecker::CopyParams(
    const ArenaVector<ir::Expression *> &params, ArenaVector<ir::Expression *> &outParams,
    ArenaUnorderedMap<varbinder::Variable *, varbinder::Variable *> *paramVarMap)
{
    auto paramCtx = varbinder::LexicalScope<varbinder::FunctionParamScope>(VarBinder());

    for (auto *const it : params) {
        auto *const paramOld = it->AsETSParameterExpression();
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *typeOld = paramOld->Clone(ProgramAllocator(), paramOld->Parent());
        ES2PANDA_ASSERT(typeOld != nullptr);
        auto *const paramNew = typeOld->AsETSParameterExpression();

        varbinder::Variable *var = VarBinder()->AddParamDecl(paramNew);
        Type *paramType = paramOld->Variable()->TsType();
        var->SetTsType(paramType);
        var->SetScope(paramCtx.GetScope());

        paramNew->SetVariable(var);
        paramNew->SetTsType(paramType);

        if (auto *newTypeAnno = paramNew->TypeAnnotation(); newTypeAnno != nullptr) {
            newTypeAnno->SetTsType(paramOld->TypeAnnotation()->TsType());
            compiler::InitScopesPhaseETS::RunExternalNode(newTypeAnno, VarBinder()->AsETSBinder());
        }

        if (paramVarMap != nullptr) {
            paramVarMap->insert({paramOld->Ident()->Variable(), var});
        }
        outParams.emplace_back(paramNew);
    }

    return paramCtx.GetScope();
}

void ETSChecker::ReplaceScope(ir::AstNode *root, ir::AstNode *oldNode, varbinder::Scope *newScope)
{
    if (root == nullptr) {
        return;
    }

    root->Iterate([this, oldNode, newScope](ir::AstNode *child) {
        auto *scope = NodeScope(child);
        if (scope != nullptr) {
            while (scope->Parent() != nullptr && scope->Parent()->Node() != oldNode) {
                scope = scope->Parent();
            }
            scope->SetParent(newScope);
        } else {
            ReplaceScope(child, oldNode, newScope);
        }
    });
}

void ETSChecker::MoveTrailingBlockToEnclosingBlockStatement(ir::CallExpression *callExpr)
{
    if (callExpr == nullptr) {
        return;
    }

    ir::AstNode *parent = callExpr->Parent();
    ir::AstNode *current = callExpr;
    while (parent != nullptr) {
        if (!parent->IsBlockStatement()) {
            current = parent;
            parent = parent->Parent();
        } else {
            // Collect trailing block, insert it only when block statements traversal ends to avoid order mismatch.
            parent->AsBlockStatement()->AddTrailingBlock(current, callExpr->TrailingBlock());
            callExpr->TrailingBlock()->SetParent(parent);
            callExpr->SetTrailingBlock(nullptr);
            break;
        }
    }
}

using SFunctionData = ir::ScriptFunction::ScriptFunctionData;
ir::ScriptFunction *ETSChecker::CreateLambdaFunction(ir::BlockStatement *trailingBlock, Signature *sig)
{
    auto *funcParamScope = varbinder::LexicalScope<varbinder::FunctionParamScope>(VarBinder()).GetScope();
    auto paramCtx = varbinder::LexicalScope<varbinder::FunctionParamScope>::Enter(VarBinder(), funcParamScope, false);

    auto funcCtx = varbinder::LexicalScope<varbinder::FunctionScope>(VarBinder());
    auto *funcScope = funcCtx.GetScope();
    funcScope->BindParamScope(funcParamScope);
    funcParamScope->BindFunctionScope(funcScope);
    funcParamScope->SetParent(trailingBlock->Scope()->Parent());

    for (auto [_, var] : trailingBlock->Scope()->Bindings()) {
        (void)_;
        if (var->GetScope() == trailingBlock->Scope()) {
            var->SetScope(funcScope);
            funcScope->InsertBinding(var->Name(), var);
        }
    }

    ArenaVector<ir::Expression *> params(ProgramAllocator()->Adapter());
    ir::ScriptFunctionFlags flags = ir::ScriptFunctionFlags::ARROW;
    bool trailingLambdaHasReceiver = false;
    if (IsLastParameterLambdaWithReceiver(sig)) {
        auto *actualLambdaType =
            sig->Function()->Params().back()->AsETSParameterExpression()->TypeAnnotation()->AsETSFunctionType();
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *receiverOfTrailingBlock =
            actualLambdaType->Params()[0]->Clone(ProgramAllocator(), nullptr)->AsExpression();
        auto *receiverVar = receiverOfTrailingBlock->AsETSParameterExpression()->Ident()->Variable();
        auto *receiverVarClone =
            ProgramAllocator()->New<varbinder::LocalVariable>(receiverVar->Declaration(), receiverVar->Flags());
        receiverVarClone->SetTsType(receiverVar->TsType());
        receiverVarClone->SetScope(funcParamScope);
        funcScope->InsertBinding(receiverVarClone->Name(), receiverVarClone);
        receiverOfTrailingBlock->AsETSParameterExpression()->Ident()->SetVariable(receiverVarClone);
        params.emplace_back(receiverOfTrailingBlock);
        trailingLambdaHasReceiver = true;
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *funcNode = ProgramAllocNode<ir::ScriptFunction>(
        ProgramAllocator(),
        SFunctionData {trailingBlock,
                       ir::FunctionSignature(nullptr, std::move(params), nullptr, trailingLambdaHasReceiver), flags});
    funcNode->SetScope(funcScope);
    funcScope->BindNode(funcNode);
    funcParamScope->BindNode(funcNode);

    trailingBlock->SetScope(funcScope);

    return funcNode;
}

void ETSChecker::TransformTraillingLambda(ir::CallExpression *callExpr, Signature *sig)
{
    auto *trailingBlock = callExpr->TrailingBlock();
    ES2PANDA_ASSERT(trailingBlock != nullptr);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *funcNode = CreateLambdaFunction(trailingBlock, sig);
    funcNode->AddFlag(ir::ScriptFunctionFlags::TRAILING_LAMBDA);
    ReplaceScope(funcNode->Body(), trailingBlock, funcNode->Scope());
    callExpr->SetTrailingBlock(nullptr);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *arrowFuncNode = ProgramAllocNode<ir::ArrowFunctionExpression>(funcNode, ProgramAllocator());
    arrowFuncNode->SetRange(trailingBlock->Range());
    arrowFuncNode->SetParent(callExpr);
    callExpr->Arguments().push_back(arrowFuncNode);
}

ArenaVector<ir::Expression *> ETSChecker::ExtendArgumentsWithFakeLamda(ir::CallExpression *callExpr)
{
    auto funcCtx = varbinder::LexicalScope<varbinder::FunctionScope>(VarBinder());
    auto *funcScope = funcCtx.GetScope();
    ArenaVector<ir::Expression *> params(ProgramAllocator()->Adapter());

    ArenaVector<ir::Statement *> statements(ProgramAllocator()->Adapter());
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *body = ProgramAllocNode<ir::BlockStatement>(ProgramAllocator(), std::move(statements));
    ES2PANDA_ASSERT(body != nullptr);
    body->SetScope(funcScope);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *funcNode = ProgramAllocNode<ir::ScriptFunction>(
        ProgramAllocator(),
        ir::ScriptFunction::ScriptFunctionData {body, ir::FunctionSignature(nullptr, std::move(params), nullptr),
                                                ir::ScriptFunctionFlags::ARROW});
    ES2PANDA_ASSERT(funcNode != nullptr);
    funcNode->SetScope(funcScope);
    funcScope->BindNode(funcNode);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *arrowFuncNode = ProgramAllocNode<ir::ArrowFunctionExpression>(funcNode, ProgramAllocator());
    ES2PANDA_ASSERT(arrowFuncNode != nullptr);
    arrowFuncNode->SetParent(callExpr);

    ArenaVector<ir::Expression *> fakeArguments = callExpr->Arguments();
    fakeArguments.push_back(arrowFuncNode);
    return fakeArguments;
}

void ETSChecker::EnsureValidCurlyBrace(ir::CallExpression *callExpr)
{
    if (callExpr->TrailingBlock() == nullptr) {
        return;
    }

    if (callExpr->IsTrailingBlockInNewLine()) {
        MoveTrailingBlockToEnclosingBlockStatement(callExpr);
        return;
    }

    LogError(diagnostic::NO_SUCH_SIG_WITH_TRAILING_LAMBDA, {}, callExpr->Start());
}

ETSObjectType *ETSChecker::GetCachedFunctionalInterface(ir::ETSFunctionType *type)
{
    auto hash = GetHashFromFunctionType(type);
    auto it = functionalInterfaceCache_.find(hash);
    if (it == functionalInterfaceCache_.cend()) {
        return nullptr;
    }
    return it->second;
}

void ETSChecker::CacheFunctionalInterface(ir::ETSFunctionType *type, ETSObjectType *ifaceType)
{
    auto hash = GetHashFromFunctionType(type);
    ES2PANDA_ASSERT(functionalInterfaceCache_.find(hash) == functionalInterfaceCache_.cend());
    functionalInterfaceCache_.emplace(hash, ifaceType);
}

void ETSChecker::CollectReturnStatements(ir::AstNode *parent)  // NOTE: remove with #28178
{
    parent->Iterate([this](ir::AstNode *childNode) -> void {
        if (childNode->IsScriptFunction()) {
            return;
        }

        auto scope = Scope();
        if (childNode->IsBlockStatement()) {
            scope = childNode->AsBlockStatement()->Scope();
        }
        checker::ScopeContext scopeCtx(this, scope);

        if (childNode->IsReturnStatement()) {
            ir::ReturnStatement *returnStmt = childNode->AsReturnStatement();
            returnStmt->Check(this);
        }

        CollectReturnStatements(childNode);
    });
}

std::vector<ConstraintCheckRecord> &ETSChecker::PendingConstraintCheckRecords()
{
    return pendingConstraintCheckRecords_;
}

size_t &ETSChecker::ConstraintCheckScopesCount()
{
    return constraintCheckScopesCount_;
}

bool ETSChecker::HasSameAssemblySignature(Signature const *const sig1, Signature const *const sig2) noexcept
{
    if (sig1->ReturnType()->ToAssemblerTypeWithRank() != sig2->ReturnType()->ToAssemblerTypeWithRank()) {
        return false;
    }

    if (sig1->ArgCount() != sig2->ArgCount()) {
        return false;
    }

    for (size_t ix = 0U; ix < sig1->Params().size(); ++ix) {
        if (sig1->Params()[ix]->TsType()->ToAssemblerTypeWithRank() !=
            sig2->Params()[ix]->TsType()->ToAssemblerTypeWithRank()) {
            return false;
        }
    }

    auto *rv1 = sig1->RestVar();
    auto *rv2 = sig2->RestVar();
    if (rv1 == nullptr && rv2 == nullptr) {
        return true;
    }
    if (rv1 == nullptr || rv2 == nullptr) {  // exactly one of them is null
        return false;
    }

    return (rv1->TsType()->ToAssemblerTypeWithRank() == rv2->TsType()->ToAssemblerTypeWithRank());
}

bool ETSChecker::HasSameAssemblySignatures(ETSFunctionType const *const func1,
                                           ETSFunctionType const *const func2) noexcept
{
    for (auto const *sig1 : func1->CallSignatures()) {
        for (auto const *sig2 : func2->CallSignatures()) {
            if (HasSameAssemblySignature(sig1, sig2)) {
                return true;
            }
        }
    }
    return false;
}

Signature *ETSChecker::FirstMatchSignatures(ir::CallExpression *expr, checker::Type *calleeType)
{
    if (expr->TrailingBlock() == nullptr) {
        auto *signature =
            MatchOrderSignatures(calleeType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow(), expr->TypeParams(),
                                 expr->Arguments(), expr->Start(), TypeRelationFlag::NONE);
        if (signature == nullptr) {
            ThrowOverloadMismatch(calleeType->AsETSFunctionType()->Name(), expr->Arguments(), expr->Start(), "call");
            return nullptr;
        }
        UpdateDeclarationFromSignature(expr, signature);
        return signature;
    }

    auto &signatures = expr->IsETSConstructorCall() ? calleeType->AsETSObjectType()->ConstructSignatures()
                                                    : calleeType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow();
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return ResolveTrailingLambda(signatures, expr, expr->Start());
}

Signature *ETSChecker::MatchOrderSignatures(ArenaVector<Signature *> &signatures,
                                            const ir::TSTypeParameterInstantiation *typeArguments,
                                            const ArenaVector<ir::Expression *> &arguments,
                                            const lexer::SourcePosition &pos, TypeRelationFlag resolveFlags)
{
    Signature *compatibleSignatures = nullptr;
    Signature *notVisibleSignature = nullptr;
    std::vector<bool> argTypeInferenceRequired = FindTypeInferenceArguments(arguments);

    auto collectSignatures = [&](TypeRelationFlag relationFlags) {
        for (auto *sig : signatures) {
            if (notVisibleSignature != nullptr &&
                !IsSignatureAccessible(sig, Context().ContainingClass(), Relation())) {
                continue;
            }
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            auto *concreteSig = ValidateOrderSignature(std::make_tuple(sig, typeArguments, relationFlags), arguments,
                                                       pos, argTypeInferenceRequired, signatures.size() == 1);

            if (concreteSig == nullptr) {
                CleanArgumentsInformation(arguments);
                continue;
            }
            if (notVisibleSignature == nullptr &&
                !IsSignatureAccessible(sig, Context().ContainingClass(), Relation())) {
                CleanArgumentsInformation(arguments);
                notVisibleSignature = concreteSig;
            } else {
                compatibleSignatures = concreteSig;
                return;
            }
        }
    };

    collectSignatures(signatures.size() == 1 ? TypeRelationFlag::WIDENING | resolveFlags
                                             : TypeRelationFlag::WIDENING | TypeRelationFlag::NO_THROW | resolveFlags);

    if (compatibleSignatures != nullptr) {
        return compatibleSignatures;
    }

    if (notVisibleSignature != nullptr && ((resolveFlags & TypeRelationFlag::NO_THROW) == 0)) {
        LogError(diagnostic::SIG_INVISIBLE, {notVisibleSignature->Function()->Id()->Name(), notVisibleSignature}, pos);
    }

    return nullptr;
}

Signature *ETSChecker::ValidateOrderSignature(
    std::tuple<Signature *, const ir::TSTypeParameterInstantiation *, TypeRelationFlag> info,
    const ArenaVector<ir::Expression *> &arguments, const lexer::SourcePosition &pos,
    const std::vector<bool> &argTypeInferenceRequired, const bool unique)
{
    auto [baseSignature, typeArguments, flags] = info;
    // In case of overloads, it is necessary to iterate through the compatible signatures again,
    // setting the boxing/unboxing flag for the arguments if needed.
    // So handle substitution arguments only in the case of unique function or collecting signature phase.
    Signature *const signature =
        MaybeSubstituteTypeParameters(this, baseSignature, typeArguments, arguments, pos, flags);
    if (signature == nullptr) {
        return nullptr;
    }

    // When process first match, if current signature is not matched, do not log TypeError
    SignatureMatchContext signatureMatchContext(this, util::DiagnosticType::SEMANTIC,
                                                (flags & TypeRelationFlag::NO_THROW) == 0);

    size_t const argCount = arguments.size();
    auto const hasRestParameter = signature->RestVar() != nullptr;
    size_t compareCount = argCount;
    if ((flags & TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA) != 0 && !signature->Params().empty() &&
        signature->Params().back()->Declaration()->Node()->AsETSParameterExpression()->IsOptional()) {
        compareCount = compareCount - 1;
    }

    if (compareCount < signature->MinArgCount() || (argCount > signature->ArgCount() && !hasRestParameter)) {
        LogError(diagnostic::PARAM_COUNT_MISMATCH, {signature->MinArgCount(), argCount}, pos);
        return nullptr;
    }

    if (argCount > signature->ArgCount() && hasRestParameter && (flags & TypeRelationFlag::IGNORE_REST_PARAM) != 0) {
        return nullptr;
    }

    auto count = std::min(signature->ArgCount(), argCount);
    // Check all required formal parameter(s) first
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    if (!ValidateOrderSignatureRequiredParams(signature, arguments, flags, argTypeInferenceRequired) ||
        !signatureMatchContext.ValidSignatureMatchStatus()) {
        return nullptr;
    }

    // Check rest parameter(s) if any exists
    if (!hasRestParameter || (count >= argCount && !signature->RestVar()->TsType()->IsETSTupleType())) {
        return signature;
    }
    if (!ValidateSignatureRestParams(signature, arguments, flags, true, unique)) {
        return nullptr;
    }

    return signature;
}

bool ETSChecker::SetPreferredTypeBeforeValidate(Signature *substitutedSig, ir::Expression *argument, size_t index,
                                                TypeRelationFlag flags,
                                                const std::vector<bool> &argTypeInferenceRequired)
{
    auto const paramType = GetNonNullishType(substitutedSig->Params()[index]->TsType());
    if (argument->IsObjectExpression()) {
        argument->AsObjectExpression()->SetPreferredType(paramType);
    }

    if (argument->IsMemberExpression()) {
        SetArrayPreferredTypeForNestedMemberExpressions(argument->AsMemberExpression(), paramType);
    }

    if (argTypeInferenceRequired[index]) {
        if (!paramType->IsETSFunctionType()) {
            return false;
        }
        ES2PANDA_ASSERT(argument->IsArrowFunctionExpression());
        auto *param = substitutedSig->Params()[index]->Declaration()->Node()->AsETSParameterExpression();
        auto *lambda = argument->AsArrowFunctionExpression();
        if (lambda->Function()->Params().size() >
            paramType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow().front()->Params().size()) {
            return false;
        }
        return CheckLambdaInfer(param->TypeAnnotation(), argument->AsArrowFunctionExpression(), paramType);
    }

    if (argument->IsArrayExpression()) {
        argument->AsArrayExpression()->SetPreferredTypeBasedOnFuncParam(this, paramType, flags);
    }

    if (argument->IsETSNewArrayInstanceExpression()) {
        argument->AsETSNewArrayInstanceExpression()->SetPreferredTypeBasedOnFuncParam(this, paramType, flags);
    }

    if (argument->IsETSNewMultiDimArrayInstanceExpression()) {
        argument->AsETSNewMultiDimArrayInstanceExpression()->SetPreferredTypeBasedOnFuncParam(this, paramType, flags);
    }

    return true;
}

bool ETSChecker::ValidateOrderSignatureRequiredParams(Signature *substitutedSig,
                                                      const ArenaVector<ir::Expression *> &arguments,
                                                      TypeRelationFlag flags,
                                                      const std::vector<bool> &argTypeInferenceRequired)
{
    auto commonArity = std::min(arguments.size(), substitutedSig->ArgCount());
    if ((flags & TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA) != 0) {
        if (commonArity == 0) {
            ES2PANDA_ASSERT(substitutedSig->GetSignatureInfo()->params.empty());
            return false;
        }
        commonArity = commonArity - 1;
    }
    for (size_t index = 0; index < commonArity; ++index) {
        auto &argument = arguments[index];
        if (!SetPreferredTypeBeforeValidate(substitutedSig, argument, index, flags, argTypeInferenceRequired)) {
            return false;
        }

        if (argument->IsSpreadElement()) {
            LogError(diagnostic::SPREAD_ONTO_SINGLE_PARAM, {}, argument->Start());
            return false;
        }

        if (argument->IsIdentifier() && IsInvalidArgumentAsIdentifier(Scope(), argument->AsIdentifier())) {
            LogError(diagnostic::ARG_IS_CLASS_ID, {}, argument->Start());
            return false;
        }

        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        if (!ValidateOrderSignatureInvocationContext(substitutedSig, argument, index, flags)) {
            return false;
        }
    }

    if ((flags & TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA) != 0 && arguments.back()->IsArrowFunctionExpression()) {
        ir::ScriptFunction *const lambda = arguments.back()->AsArrowFunctionExpression()->Function();
        auto targetParm = substitutedSig->GetSignatureInfo()->params.back()->Declaration()->Node();
        if (!CheckLambdaAssignable(targetParm->AsETSParameterExpression(), lambda)) {
            return false;
        }
    }
    return true;
}

void ETSChecker::CleanArgumentsInformation(const ArenaVector<ir::Expression *> &arguments)
{
    if (arguments.empty()) {
        return;
    }
    for (auto *argument : arguments) {
        argument->CleanCheckInformation();
    }
}

bool ETSChecker::ValidateOrderSignatureInvocationContext(Signature *substitutedSig, ir::Expression *argument,
                                                         std::size_t index, TypeRelationFlag flags)
{
    Type *targetType = substitutedSig->Params()[index]->TsType();
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    Type *argumentType = argument->Check(this);

    flags |= TypeRelationFlag::ONLY_CHECK_WIDENING;

    auto const invocationCtx =
        checker::InvocationContext(Relation(), argument, argumentType, targetType, argument->Start(),
                                   {{diagnostic::TYPE_MISMATCH_AT_IDX, {argumentType, targetType, index + 1}}}, flags);

    return invocationCtx.IsInvocable();
}

Signature *ETSChecker::ResolveTrailingLambda(ArenaVector<Signature *> &signatures, ir::CallExpression *callExpr,
                                             const lexer::SourcePosition &pos, const TypeRelationFlag reportFlag)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto arguments = ExtendArgumentsWithFakeLamda(callExpr);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto sig = ResolvePotentialTrailingLambda(callExpr, signatures, arguments);
    if (sig != nullptr) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        TransformTraillingLambda(callExpr, sig);
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        TrailingLambdaTypeInference(sig, callExpr->Arguments());
        UpdateDeclarationFromSignature(callExpr, sig);
        callExpr->SetIsTrailingCall(true);
        return sig;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    sig = MatchOrderSignatures(signatures, callExpr->TypeParams(), callExpr->Arguments(), pos, reportFlag);
    if (sig != nullptr) {
        EnsureValidCurlyBrace(callExpr);
    }

    UpdateDeclarationFromSignature(callExpr, sig);
    return sig;
}

Signature *ETSChecker::ResolvePotentialTrailingLambda(ir::CallExpression *callExpr,
                                                      ArenaVector<Signature *> const &signatures,
                                                      ArenaVector<ir::Expression *> &arguments)
{
    auto *trailingLambda = arguments.back()->AsArrowFunctionExpression();
    ArenaVector<Signature *> normalSig(ProgramAllocator()->Adapter());
    ArenaVector<Signature *> sigContainLambdaWithReceiverAsParam(ProgramAllocator()->Adapter());
    Signature *signature = nullptr;
    for (auto sig : signatures) {
        if (!IsLastParameterLambdaWithReceiver(sig)) {
            normalSig.emplace_back(sig);
            continue;
        }

        auto *candidateFunctionType =
            sig->Function()->Params().back()->AsETSParameterExpression()->TypeAnnotation()->AsETSFunctionType();
        auto *currentReceiver = candidateFunctionType->Params()[0];
        trailingLambda->Function()->EmplaceParams(currentReceiver);
        sigContainLambdaWithReceiverAsParam.emplace_back(sig);
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        signature = MatchOrderSignatures(sigContainLambdaWithReceiverAsParam, callExpr->TypeParams(), arguments,
                                         callExpr->Start(),
                                         TypeRelationFlag::NO_THROW | TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA);
        if (signature != nullptr) {
            return signature;
        }
        sigContainLambdaWithReceiverAsParam.clear();
        trailingLambda->Function()->ClearParams();
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return MatchOrderSignatures(normalSig, callExpr->TypeParams(), arguments, callExpr->Start(),
                                TypeRelationFlag::NO_THROW | TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA);
}

void ETSChecker::ThrowOverloadMismatch(util::StringView callName, const ArenaVector<ir::Expression *> &arguments,
                                       const lexer::SourcePosition &pos, std::string_view signatureKind)
{
    std::string msg {};
    msg.append(callName.Mutf8());
    msg += "(";

    for (std::size_t index = 0U; index < arguments.size(); ++index) {
        auto const &argument = arguments[index];
        Type const *const argumentType = argument->Check(this);
        if (!argumentType->IsTypeError()) {
            msg += argumentType->ToString();
        } else {
            //  NOTE (DZ): extra cases for some specific nodes can be added here (as for 'ArrowFunctionExpression')
            msg += argument->ToString();
        }

        if (index != arguments.size() - 1U) {
            msg += ", ";
        }
    }
    msg += ")";
    LogError(diagnostic::NO_MATCHING_SIG, {signatureKind, msg.c_str()}, pos);
}

}  // namespace ark::es2panda::checker
