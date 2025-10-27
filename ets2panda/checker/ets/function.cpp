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
#include "checker/ETSAnalyzerHelpers.h"
#include "checker/ETSchecker.h"
#include "checker/ets/typeRelationContext.h"
#include "checker/types/ets/etsAwaitedType.h"
#include "checker/types/ets/etsObjectType.h"
#include "checker/types/ets/etsPartialTypeParameter.h"
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

static Type *MaybeBoxedType(ETSChecker *checker, Type *type, ir::Expression *expr)
{
    ES2PANDA_ASSERT(type != nullptr);
    if (!type->IsETSPrimitiveType()) {
        return type;
    }
    auto *relation = checker->Relation();
    auto *oldNode = relation->GetNode();
    relation->SetNode(expr);
    auto *res = checker->MaybeBoxInRelation(type);
    relation->SetNode(oldNode);
    return res;
}

static void InferUntilFail(Signature const *const signature, const ArenaVector<ir::Expression *> &arguments,
                           ETSChecker *checker, Substitution *substitution)
{
    auto *sigInfo = signature->GetSignatureInfo();
    auto &sigParams = signature->GetSignatureInfo()->typeParams;
    ArenaVector<bool> inferStatus(checker->Allocator()->Adapter());
    inferStatus.assign(arguments.size(), false);
    bool anyChange = true;
    size_t lastSubsititutionSize = 0;

    checker->AddStatus(checker::CheckerStatus::IN_TYPE_INFER);
    // some ets lib files require type infer from arg index 0,1,... , not fit to build graph
    ES2PANDA_ASSERT(substitution != nullptr);
    while (anyChange && substitution->size() < sigParams.size()) {
        anyChange = false;
        for (size_t ix = 0; ix < arguments.size(); ++ix) {
            if (inferStatus[ix]) {
                continue;
            }

            auto *arg = arguments[ix];
            if (arg->IsObjectExpression()) {
                continue;
            }

            auto *const argType = arg->IsSpreadElement()
                                      ? MaybeBoxedType(checker, arg->AsSpreadElement()->Argument()->Check(checker),
                                                       arg->AsSpreadElement()->Argument())
                                      : MaybeBoxedType(checker, arg->Check(checker), arg);
            auto *const paramType = (ix < signature->ArgCount())  ? sigInfo->params[ix]->TsType()
                                    : sigInfo->restVar != nullptr ? sigInfo->restVar->TsType()
                                                                  : nullptr;

            if (paramType == nullptr) {
                continue;
            }
            if (arg->IsArrowFunctionExpression()) {
                checker->Relation()->SetNode(arg);
            }

            if (checker->EnhanceSubstitutionForType(sigInfo->typeParams, paramType, argType, substitution)) {
                inferStatus[ix] = true;
            }
            if (lastSubsititutionSize != substitution->size()) {
                lastSubsititutionSize = substitution->size();
                anyChange = true;
            }
        }
    }
    checker->RemoveStatus(checker::CheckerStatus::IN_TYPE_INFER);
}

static std::optional<Substitution> BuildImplicitSubstitutionForArguments(ETSChecker *checker, Signature *signature,
                                                                         const ArenaVector<ir::Expression *> &arguments)
{
    auto substitution = Substitution {};
    auto *sigInfo = signature->GetSignatureInfo();
    auto &sigParams = signature->GetSignatureInfo()->typeParams;

    InferUntilFail(signature, arguments, checker, &substitution);

    if (substitution.size() != sigParams.size()) {
        for (const auto typeParam : sigParams) {
            auto newTypeParam = typeParam->AsETSTypeParameter();
            if (auto it = substitution.find(newTypeParam); it != substitution.cend()) {
                continue;
            }
            if (newTypeParam->GetDefaultType() == nullptr) {
                checker->EmplaceSubstituted(&substitution, newTypeParam, checker->GlobalETSNeverType());
                continue;
            }
            auto dflt = newTypeParam->GetDefaultType()->Substitute(checker->Relation(), &substitution);
            if (!checker->EnhanceSubstitutionForType(sigInfo->typeParams, newTypeParam, dflt, &substitution)) {
                return std::nullopt;
            }
        }
    }
    if (substitution.size() != sigParams.size() &&
        (signature->Function()->ReturnTypeAnnotation() == nullptr ||
         !checker->EnhanceSubstitutionForType(sigInfo->typeParams,
                                              signature->Function()->ReturnTypeAnnotation()->TsType(),
                                              signature->ReturnType(), &substitution))) {
        return std::nullopt;
    }

    return substitution;
}

static bool IsCompatibleTypeArgument(ETSChecker *checker, ETSTypeParameter *typeParam, Type *typeArgument,
                                     const Substitution *substitution);

static std::optional<Substitution> BuildExplicitSubstitutionForArguments(ETSChecker *checker, Signature *signature,
                                                                         const ArenaVector<ir::TypeNode *> &params,
                                                                         const lexer::SourcePosition &pos,
                                                                         TypeRelationFlag flags)
{
    auto &sigParams = signature->GetSignatureInfo()->typeParams;
    auto substitution = Substitution {};
    auto constraintsSubstitution = Substitution {};
    ArenaVector<Type *> instArgs {checker->Allocator()->Adapter()};

    for (size_t ix = 0; ix < params.size(); ++ix) {
        instArgs.push_back(MaybeBoxedType(checker, params[ix]->GetType(checker), params[ix]));
        if (ix < sigParams.size()) {
            checker->EmplaceSubstituted(&constraintsSubstitution, sigParams[ix]->AsETSTypeParameter(), instArgs[ix]);
        }
    }
    for (size_t ix = instArgs.size(); ix < sigParams.size(); ++ix) {
        auto typeParam = sigParams[ix]->AsETSTypeParameter();
        auto *dflt = typeParam->GetDefaultType();
        if (dflt == nullptr) {
            break;
        }

        dflt = dflt->Substitute(checker->Relation(), &constraintsSubstitution);
        instArgs.push_back(dflt);
        checker->EmplaceSubstituted(&constraintsSubstitution, typeParam, instArgs[ix]);
    }
    if (sigParams.size() != instArgs.size()) {
        if ((flags & TypeRelationFlag::NO_THROW) == 0U) {
            checker->LogError(diagnostic::RTYPE_PARAM_COUNT_MISMATCH, {sigParams.size(), instArgs.size()}, pos);
        }
        return std::nullopt;
    }

    for (size_t ix = 0; ix < sigParams.size(); ix++) {
        if (!IsCompatibleTypeArgument(checker, sigParams[ix]->AsETSTypeParameter(), instArgs[ix],
                                      &constraintsSubstitution)) {
            if ((flags & TypeRelationFlag::NO_THROW) == 0U) {
                auto *constraintType = sigParams[ix]->AsETSTypeParameter()->GetConstraintType()->Substitute(
                    checker->Relation(), &constraintsSubstitution);
                checker->LogError(diagnostic::TYPEARG_TYPEPARAM_SUBTYPING, {instArgs[ix], constraintType}, pos);
            }

            return std::nullopt;
        }
        checker->EmplaceSubstituted(&substitution, sigParams[ix]->AsETSTypeParameter(), instArgs[ix]);
    }
    return substitution;
}

static Signature *MaybeSubstituteTypeParameters(
    ETSChecker *checker, std::tuple<Signature *, const ir::TSTypeParameterInstantiation *, TypeRelationFlag> info,
    const ArenaVector<ir::Expression *> &arguments, const lexer::SourcePosition &pos)
{
    auto [signature, typeArguments, flags] = info;
    if (typeArguments == nullptr && signature->GetSignatureInfo()->typeParams.empty()) {
        return signature;
    }

    const std::optional<Substitution> substitution =
        (typeArguments != nullptr)
            ? BuildExplicitSubstitutionForArguments(checker, signature, typeArguments->Params(), pos, flags)
            : BuildImplicitSubstitutionForArguments(checker, signature, arguments);

    return (!substitution.has_value()) ? nullptr : signature->Substitute(checker->Relation(), &substitution.value());
}

static varbinder::Scope *NodeScope(ir::AstNode *ast)
{
    if (ast->IsBlockStatement()) {
        return ast->AsBlockStatement()->Scope();
    }
    if (ast->IsBlockExpression()) {
        return ast->AsBlockExpression()->Scope();
    }
    if (ast->IsDoWhileStatement()) {
        return ast->AsDoWhileStatement()->Scope();
    }
    if (ast->IsForInStatement()) {
        return ast->AsForInStatement()->Scope();
    }
    if (ast->IsForOfStatement()) {
        return ast->AsForOfStatement()->Scope();
    }
    if (ast->IsForUpdateStatement()) {
        return ast->AsForUpdateStatement()->Scope();
    }
    if (ast->IsSwitchStatement()) {
        return ast->AsSwitchStatement()->Scope();
    }
    if (ast->IsWhileStatement()) {
        return ast->AsWhileStatement()->Scope();
    }
    if (ast->IsCatchClause()) {
        return ast->AsCatchClause()->Scope();
    }
    if (ast->IsClassDefinition()) {
        return ast->AsClassDefinition()->Scope();
    }
    if (ast->IsScriptFunction()) {
        return ast->AsScriptFunction()->Scope()->ParamScope();
    }
    return nullptr;
}

// NOTE: #14993 merge with InstantiationContext::ValidateTypeArg
static bool IsCompatibleTypeArgument(ETSChecker *checker, ETSTypeParameter *typeParam, Type *typeArgument,
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
        typeArgument = checker->GlobalETSUndefinedType();
    }
    ES2PANDA_ASSERT(ETSChecker::IsReferenceType(typeArgument));
    auto constraint = typeParam->GetConstraintType()->Substitute(checker->Relation(), substitution);
    return checker->Relation()->IsSupertypeOf(constraint, typeArgument);
}

static bool EnhanceSubstitutionForType(ETSChecker *checker, const ArenaVector<Type *> &typeParams, Type *paramType,
                                       Type *argumentType, Substitution *substitution);

static bool EnhanceSubstitutionForReadonly(ETSChecker *checker, const ArenaVector<Type *> &typeParams,
                                           ETSReadonlyType *paramType, Type *argumentType, Substitution *substitution)
{
    return EnhanceSubstitutionForType(checker, typeParams, paramType->GetUnderlying(),
                                      checker->GetReadonlyType(argumentType), substitution);
}

static bool ValidateTypeSubstitution(ETSChecker *checker, const ArenaVector<Type *> &typeParams, Type *ctype,
                                     Type *argumentType, Substitution *substitution)
{
    if (!EnhanceSubstitutionForType(checker, typeParams, ctype, argumentType, substitution)) {
        return false;
    }
    return !ctype->IsETSTypeParameter() ||
           (substitution->count(ctype->AsETSTypeParameter()) > 0 &&
            checker->Relation()->IsAssignableTo(argumentType, substitution->at(ctype->AsETSTypeParameter())));
}

static bool EnhanceSubstitutionForUnion(ETSChecker *checker, const ArenaVector<Type *> &typeParams,
                                        ETSUnionType *paramUn, Type *argumentType, Substitution *substitution)
{
    if (!argumentType->IsETSUnionType()) {
        bool foundValid = false;
        for (Type *ctype : paramUn->ConstituentTypes()) {
            foundValid |= ValidateTypeSubstitution(checker, typeParams, ctype, argumentType, substitution);
        }
        return foundValid;
    }
    auto *const argUn = argumentType->AsETSUnionType();

    std::vector<Type *> paramWlist {};
    ArenaVector<Type *> argWlist(checker->ProgramAllocator()->Adapter());

    bool isIdenticalUpToTypeParams = false;
    for (auto *pc : paramUn->ConstituentTypes()) {
        for (auto *ac : argUn->ConstituentTypes()) {
            {
                // Type parameters are inferred separately, here we check the equality of the base classes
                SavedTypeRelationFlagsContext savedFlagsCtx(checker->Relation(),
                                                            checker->Relation()->GetTypeRelationFlags() |
                                                                TypeRelationFlag::IGNORE_TYPE_PARAMETERS);
                isIdenticalUpToTypeParams = checker->Relation()->IsIdenticalTo(pc, ac);
            }

            if (!isIdenticalUpToTypeParams) {
                paramWlist.push_back(pc);
                argWlist.push_back(ac);
                continue;
            }

            if (!EnhanceSubstitutionForType(checker, typeParams, pc, ac, substitution)) {
                return false;
            }
        }
    }
    auto *const newArg = checker->CreateETSUnionType(std::move(argWlist));

    for (auto *pc : paramWlist) {
        if (!EnhanceSubstitutionForType(checker, typeParams, pc, newArg, substitution)) {
            return false;
        }
    }
    return true;
}

static bool ProcessUntypedParameter(ETSChecker *checker, size_t paramIndex, Signature *paramSig, Signature *argSig,
                                    Substitution *substitution)
{
    auto declNode = argSig->Params()[paramIndex]->Declaration()->Node();
    if (!declNode->IsETSParameterExpression() || !checker->HasStatus(CheckerStatus::IN_TYPE_INFER)) {
        return false;
    }

    auto *paramExpr = declNode->AsETSParameterExpression();
    if (paramExpr->Ident()->TypeAnnotation() != nullptr) {
        return false;
    }

    Type *paramType = paramSig->Params()[paramIndex]->TsType();
    Type *inferredType = paramType->Substitute(checker->Relation(), substitution);

    varbinder::Variable *argParam = argSig->Params()[paramIndex];
    argParam->SetTsType(inferredType);
    paramExpr->Ident()->SetTsType(inferredType);
    paramExpr->Ident()->Variable()->SetTsType(inferredType);

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

static void ResetInferredTypeInArrowBody(ir::AstNode *body, ETSChecker *checker,
                                         std::unordered_set<varbinder::Variable *> &inferredVarSet)
{
    checker::ScopeContext scopeCtx(checker, body->Parent()->Scope());
    std::function<void(ir::AstNode *)> doNode = [&](ir::AstNode *node) {
        if (node->IsIdentifier()) {
            auto *id = node->AsIdentifier();
            if (inferredVarSet.count(id->Variable()) == 0U) {
                return;
            }

            ir::AstNode *checkNode = id;
            while (checkNode->Parent()->IsTyped() && checkNode->Parent()->AsTyped()->TsType() == nullptr &&
                   checkNode->Parent() != body) {
                checkNode = checkNode->Parent();
            }
            checkNode->Check(checker);
        }
        if (node->IsVariableDeclarator()) {
            auto *id = node->AsVariableDeclarator()->Id();
            inferredVarSet.emplace(id->Variable());
            node->Check(checker);
        }
    };
    body->IterateRecursively(doNode);
}

static void ResetInferredNode(ETSChecker *checker, std::unordered_set<varbinder::Variable *> &inferredVarSet)
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
    ResetInferredTypeInArrowBody(arrowFunc->Function()->Body(), checker, inferredVarSet);
    resetFuncState(arrowFunc);
    arrowFunc->Check(checker);
}

static bool EnhanceSubstitutionForNonNullish(ETSChecker *checker, const ArenaVector<Type *> &typeParams,
                                             ETSNonNullishType *paramType, Type *argumentType,
                                             Substitution *substitution)
{
    if (argumentType->IsETSNonNullishType()) {
        ES2PANDA_ASSERT(argumentType->AsETSNonNullishType()->GetUnderlying() != nullptr);
        return EnhanceSubstitutionForType(checker, typeParams, paramType->GetUnderlying(),
                                          argumentType->AsETSNonNullishType()->GetUnderlying(), substitution);
    }
    return EnhanceSubstitutionForType(checker, typeParams, paramType->GetUnderlying(), argumentType, substitution);
}

static bool EnhanceSubstitutionTypeParameter(ETSChecker *checker, ETSTypeParameter *paramType, Type *argumentType,
                                             Substitution *substitution)
{
    auto *const originalTparam = paramType->GetOriginal();
    if (!ETSChecker::IsReferenceType(argumentType)) {
        checker->LogError(diagnostic::INFERENCE_TYPE_INCOMPAT, {paramType, argumentType},
                          paramType->GetDeclNode()->Start());
        return false;
    }

    // #23068 substitution happens before the constraint check, should be restored
    checker->EmplaceSubstituted(substitution, originalTparam, argumentType);
    return IsCompatibleTypeArgument(checker, paramType, argumentType, substitution);
}

static bool EnhanceSubstitutionForFunction(ETSChecker *checker, const ArenaVector<Type *> &typeParams,
                                           ETSFunctionType *paramType, Type *argumentType, Substitution *substitution)
{
    auto const enhance = [checker, typeParams, substitution](Type *ptype, Type *atype) {
        return EnhanceSubstitutionForType(checker, typeParams, ptype, atype, substitution);
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

    std::unordered_set<varbinder::Variable *> inferredVarSet;
    for (size_t idx = 0; idx < commonArity; idx++) {
        auto *declNode = argSig->Params()[idx]->Declaration()->Node();
        if (ProcessUntypedParameter(checker, idx, paramSig, argSig, substitution)) {
            inferredVarSet.emplace(declNode->AsETSParameterExpression()->Ident()->Variable());
            continue;
        }
        res &= enhance(paramSig->Params()[idx]->TsType(), argSig->Params()[idx]->TsType());
    }

    ResetInferredNode(checker, inferredVarSet);

    if (argSig->HasRestParameter() && paramSig->HasRestParameter()) {
        res &= enhance(paramSig->RestVar()->TsType(), argSig->RestVar()->TsType());
    }
    res &= enhance(paramSig->ReturnType(), argSig->ReturnType());

    return res;
}

static bool EnhanceSubstitutionForAwaited(ETSChecker *checker, const ArenaVector<Type *> &typeParams,
                                          ETSAwaitedType *paramType, Type *argumentType, Substitution *substitution)
{
    auto *argumentAwaitedType =
        argumentType->IsETSAwaitedType() ? argumentType->AsETSAwaitedType()->GetUnderlying() : argumentType;
    auto *paramAwaitedType = paramType->GetUnderlying();
    return EnhanceSubstitutionForType(checker, typeParams, paramAwaitedType, argumentAwaitedType, substitution);
}

static bool EnhanceSubstitutionForPartialTypeParam(ETSChecker *checker, const ArenaVector<Type *> &typeParams,
                                                   ETSPartialTypeParameter *paramType, Type *argumentType,
                                                   Substitution *substitution)
{
    if (!argumentType->IsETSObjectType() || !argumentType->AsETSObjectType()->IsPartial()) {
        return false;
    }
    ES2PANDA_ASSERT(argumentType->AsETSObjectType()->GetBaseType() != nullptr);
    return EnhanceSubstitutionForType(checker, typeParams, paramType->GetUnderlying(),
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

static bool EnhanceSubstitutionForObject(ETSChecker *checker, const ArenaVector<Type *> &typeParams,
                                         ETSObjectType *paramType, Type *argumentType, Substitution *substitution)
{
    auto const enhance = [checker, typeParams, substitution](Type *ptype, Type *atype) {
        return EnhanceSubstitutionForType(checker, typeParams, ptype, atype, substitution);
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

static bool EnhanceSubstitutionForArray(ETSChecker *checker, const ArenaVector<Type *> &typeParams,
                                        ETSArrayType *const paramType, Type *const argumentType,
                                        Substitution *const substitution)
{
    auto *const elementType =
        argumentType->IsETSArrayType() ? argumentType->AsETSArrayType()->ElementType() : argumentType;

    return EnhanceSubstitutionForType(checker, typeParams, paramType->ElementType(), elementType, substitution);
}

static bool EnhanceSubstitutionForResizableArray(ETSChecker *checker, const ArenaVector<Type *> &typeParams,
                                                 ETSResizableArrayType *const paramType, Type *const argumentType,
                                                 Substitution *const substitution)
{
    auto *const elementType =
        argumentType->IsETSResizableArrayType() ? argumentType->AsETSResizableArrayType()->ElementType() : argumentType;

    return EnhanceSubstitutionForType(checker, typeParams, paramType->ElementType(), elementType, substitution);
}

/* A very rough and imprecise partial type inference */
// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP) solid logic
static bool EnhanceSubstitutionForType(ETSChecker *checker, const ArenaVector<Type *> &typeParams, Type *paramType,
                                       Type *argumentType, Substitution *substitution)
{
    ES2PANDA_ASSERT(argumentType != nullptr);
    if (argumentType->IsETSPrimitiveType()) {
        argumentType = checker->MaybeBoxInRelation(argumentType);
    }
    if (paramType->IsETSTypeParameter()) {
        auto *const originalTparam = paramType->AsETSTypeParameter()->GetOriginal();
        if (std::find(typeParams.begin(), typeParams.end(), originalTparam) != typeParams.end() &&
            substitution->count(originalTparam) == 0) {
            return EnhanceSubstitutionTypeParameter(checker, paramType->AsETSTypeParameter(), argumentType,
                                                    substitution);
        }
    }
    if (paramType->IsETSNonNullishType()) {
        return EnhanceSubstitutionForNonNullish(checker, typeParams, paramType->AsETSNonNullishType(), argumentType,
                                                substitution);
    }
    if (paramType->IsETSFunctionType()) {
        return EnhanceSubstitutionForFunction(checker, typeParams, paramType->AsETSFunctionType(), argumentType,
                                              substitution);
    }
    if (paramType->IsETSReadonlyType()) {
        return EnhanceSubstitutionForReadonly(checker, typeParams, paramType->AsETSReadonlyType(), argumentType,
                                              substitution);
    }
    if (paramType->IsETSPartialTypeParameter()) {
        return EnhanceSubstitutionForPartialTypeParam(checker, typeParams, paramType->AsETSPartialTypeParameter(),
                                                      argumentType, substitution);
    }
    if (paramType->IsETSUnionType()) {
        return EnhanceSubstitutionForUnion(checker, typeParams, paramType->AsETSUnionType(), argumentType,
                                           substitution);
    }
    if (paramType->IsETSResizableArrayType()) {
        return EnhanceSubstitutionForResizableArray(checker, typeParams, paramType->AsETSResizableArrayType(),
                                                    argumentType, substitution);
    }
    if (paramType->IsETSObjectType()) {
        return EnhanceSubstitutionForObject(checker, typeParams, paramType->AsETSObjectType(), argumentType,
                                            substitution);
    }
    if (paramType->IsETSArrayType()) {
        return EnhanceSubstitutionForArray(checker, typeParams, paramType->AsETSArrayType(), argumentType,
                                           substitution);
    }
    if (paramType->IsETSAwaitedType()) {
        return EnhanceSubstitutionForAwaited(checker, typeParams, paramType->AsETSAwaitedType(), argumentType,
                                             substitution);
    }

    return true;
}

bool ETSChecker::EnhanceSubstitutionForType(const ArenaVector<Type *> &typeParams, Type *paramType, Type *argumentType,
                                            Substitution *substitution)
{
    return checker::EnhanceSubstitutionForType(this, typeParams, paramType, argumentType, substitution);
}

// #22952: optional arrow leftovers
static bool CheckLambdaAssignableUnion(ir::AstNode *typeAnn, ir::ScriptFunction *lambda)
{
    bool assignable = false;
    for (auto *type : typeAnn->AsETSUnionType()->Types()) {
        if (type->IsETSFunctionType()) {
            assignable |= lambda->Params().size() <= type->AsETSFunctionType()->Params().size();
            continue;
        }

        if (type->IsETSTypeReference()) {
            auto aliasType = util::Helpers::DerefETSTypeReference(type);
            assignable |= aliasType->IsETSFunctionType() &&
                          lambda->Params().size() <= aliasType->AsETSFunctionType()->Params().size();
        }
    }

    return assignable;
}

// #22952: optional arrow leftovers
static bool CheckLambdaAssignable(ETSChecker *checker, ir::Expression *param, ir::ScriptFunction *lambda)
{
    ES2PANDA_ASSERT(param->IsETSParameterExpression());
    ir::AstNode *typeAnn = param->AsETSParameterExpression()->Ident()->TypeAnnotation();
    if (typeAnn == nullptr) {
        return false;
    }
    if (typeAnn->IsETSTypeReference() && !typeAnn->AsETSTypeReference()->TsType()->IsETSArrayType()) {
        typeAnn = util::Helpers::DerefETSTypeReference(typeAnn);
    }
    if (typeAnn->IsTSTypeParameter()) {
        return true;
    }
    if (!typeAnn->IsETSFunctionType()) {
        // the surrounding function is made so we can *bypass* the typecheck in the "inference" context,
        // however the body of the function has to be checked in any case
        if (typeAnn->IsETSUnionType()) {
            return CheckLambdaAssignableUnion(typeAnn, lambda);
        }

        Type *paramType = param->AsETSParameterExpression()->Ident()->TsType();
        if (checker->Relation()->IsSupertypeOf(paramType, checker->GlobalBuiltinFunctionType())) {
            lambda->Parent()->Check(checker);
            return true;
        }
        return false;
    }

    ir::ETSFunctionType *calleeType = typeAnn->AsETSFunctionType();
    return lambda->Params().size() <= calleeType->Params().size();
}

// #22952: remove optional arrow leftovers
static bool CheckOptionalLambdaFunction(ETSChecker *checker, ir::Expression *argument, Signature *substitutedSig,
                                        size_t index)
{
    if (argument->IsArrowFunctionExpression()) {
        auto *const arrowFuncExpr = argument->AsArrowFunctionExpression();

        if (ir::ScriptFunction *const lambda = arrowFuncExpr->Function(); CheckLambdaAssignable(
                // CC-OFFNXT(G.FMT.06-CPP) project code style
                checker, substitutedSig->Params()[index]->Declaration()->Node()->AsExpression(), lambda)) {
            return true;
        }
    }

    return false;
}

static bool IsInvalidArgumentAsIdentifier(varbinder::Scope *scope, const ir::Identifier *identifier)
{
    auto result = scope->Find(identifier->Name());
    return result.variable != nullptr &&
           (result.variable->HasFlag(varbinder::VariableFlags::CLASS_OR_INTERFACE_OR_ENUM |
                                     varbinder::VariableFlags::TYPE_ALIAS));
}

static void ClearPreferredTypeForArray(checker::ETSChecker *checker, ir::Expression *argument, Type *paramType,
                                       TypeRelationFlag flags, bool needRecheck)
{
    if (argument->IsArrayExpression()) {
        // fixed array and resizeable array will cause problem here, so clear it.
        argument->CleanCheckInformation();
        argument->AsArrayExpression()->SetPreferredTypeBasedOnFuncParam(checker, paramType, flags);
    } else if (argument->IsETSNewArrayInstanceExpression()) {
        argument->CleanCheckInformation();
        argument->AsETSNewArrayInstanceExpression()->SetPreferredTypeBasedOnFuncParam(checker, paramType, flags);
    } else if (argument->IsETSNewMultiDimArrayInstanceExpression()) {
        argument->CleanCheckInformation();
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
        if (!CheckLambdaAssignable(checker, targetParm->AsETSParameterExpression(), lambda)) {
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
    if (argumentLiteral->IsFolded()) {
        return;
    }
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

static bool ValidateSignatureInvocationContext(ETSChecker *checker, Signature *substitutedSig, ir::Expression *argument,
                                               std::size_t index, TypeRelationFlag flags);

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP, G.FUD.05) solid logic
static bool ValidateSignatureRequiredParams(ETSChecker *checker, Signature *substitutedSig,
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
        auto const paramType = checker->GetNonNullishType(substitutedSig->Params()[index]->TsType());
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
            checker->SetArrayPreferredTypeForNestedMemberExpressions(argument->AsMemberExpression(), paramType);
        } else if (argument->IsSpreadElement()) {
            if (reportError) {
                checker->LogError(diagnostic::SPREAD_ONTO_SINGLE_PARAM, {}, argument->Start());
            }
            return false;
        } else if (argument->IsNumberLiteral()) {
            InferTypeForNumberLiteral(checker, argument->AsNumberLiteral(), paramType);
        }

        if (argTypeInferenceRequired[index]) {
            ES2PANDA_ASSERT(argument->IsArrowFunctionExpression());
            // Note: If the signatures are from lambdas, then they have no `Function`.
            ir::ScriptFunction *const lambda = argument->AsArrowFunctionExpression()->Function();
            auto targetParm = substitutedSig->GetSignatureInfo()->params[index]->Declaration()->Node();
            ERROR_SANITY_CHECK(checker, targetParm->IsETSParameterExpression(), return false);
            if (CheckLambdaAssignable(checker, targetParm->AsETSParameterExpression(), lambda)) {
                continue;
            }
            return false;
        }

        ClearPreferredTypeForArray(checker, argument, paramType, flags, false);

        if (argument->IsIdentifier() && IsInvalidArgumentAsIdentifier(checker->Scope(), argument->AsIdentifier())) {
            checker->LogError(diagnostic::ARG_IS_CLASS_ID, {}, argument->Start());
            return false;
        }

        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        if (!ValidateSignatureInvocationContext(checker, substitutedSig, argument, index, flags)) {
            return false;
        }
    }

    return CheckArrowFunctionParamIfNeeded(checker, substitutedSig, arguments, flags);
}

static bool ValidateSignatureInvocationContext(ETSChecker *checker, Signature *substitutedSig, ir::Expression *argument,
                                               std::size_t index, TypeRelationFlag flags)
{
    Type *targetType = substitutedSig->Params()[index]->TsType();
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    // NOTE (smartin): remove these flag hacks after the overload resolution is completely reworked
    if ((flags & TypeRelationFlag::NO_THROW) != 0) {
        argument->IterateRecursively([](ir::AstNode *node) { node->AddAstNodeFlags(ir::AstNodeFlags::NO_THROW); });
    }
    Type *argumentType = argument->Check(checker);
    if ((flags & TypeRelationFlag::NO_THROW) != 0) {
        argument->IterateRecursively([](ir::AstNode *node) { node->RemoveAstNodeFlags(ir::AstNodeFlags::NO_THROW); });
    }

    flags |= (TypeRelationFlag::ONLY_CHECK_WIDENING);

    auto const invocationCtx =
        checker::InvocationContext(checker->Relation(), argument, argumentType, targetType, argument->Start(),
                                   {{diagnostic::TYPE_MISMATCH_AT_IDX, {argumentType, targetType, index + 1}}}, flags);

    auto sig = substitutedSig;
    if (invocationCtx.HasError() && sig && sig->HasFunction()) {
        if ((sig->Function()->IsConstructor() || sig->Function()->Id()->Name().Is("then") ||
             sig->Function()->Id()->Name().Is("catch")) &&
            sig->Owner()->Name().Is("Promise")) {
            checker->LogDiagnostic(diagnostic::PROMISE_NO_MATCHING_SIG, {argumentType}, argument->Start());
        }
    }
    return invocationCtx.IsInvocable() || CheckOptionalLambdaFunction(checker, argument, substitutedSig, index);
}

static bool SetPreferredTypeForArrayArgument(ETSChecker *checker, ir::ArrayExpression *arrayExpr,
                                             Signature *substitutedSig);

static bool IsValidRestArgument(ETSChecker *checker, ir::Expression *const argument, Signature *const substitutedSig,
                                const TypeRelationFlag flags, const std::size_t index)
{
    auto *restParamType = substitutedSig->RestVar()->TsType();
    if (restParamType->IsETSTupleType()) {
        return false;
    }
    if (argument->IsObjectExpression()) {
        argument->SetPreferredType(checker->GetElementTypeOfArray(restParamType));
        // Object literals should be checked separately afterwards after call resolution
        return true;
    }

    // Set preferred type for array expressions before checking, similar to spread elements
    if (argument->IsArrayExpression()) {
        if (!SetPreferredTypeForArrayArgument(checker, argument->AsArrayExpression(), substitutedSig)) {
            return false;
        }
    }

    const auto argumentType = argument->Check(checker);
    if (argument->HasAstNodeFlags(ir::AstNodeFlags::RESIZABLE_REST)) {
        return true;
    }

    auto targetType = checker->GetElementTypeOfArray(restParamType);
    if (substitutedSig->OwnerVar() == nullptr) {
        targetType = checker->MaybeBoxType(targetType);
    }
    auto const invocationCtx = checker::InvocationContext(
        checker->Relation(), argument, argumentType, targetType, argument->Start(),
        {{diagnostic::REST_PARAM_INCOMPAT_AT, {argumentType, targetType, index + 1}}}, flags);

    bool result = invocationCtx.IsInvocable();
    // Clear preferred type if invocation fails, similar to spread elements
    if (!result && argument->IsArrayExpression()) {
        checker->ModifyPreferredType(argument->AsArrayExpression(), nullptr);
    }

    return result;
}

static bool SetPreferredTypeForArrayArgument(ETSChecker *checker, ir::ArrayExpression *arrayExpr,
                                             Signature *substitutedSig)
{
    auto *const restVarType = substitutedSig->RestVar()->TsType();
    if (!restVarType->IsETSArrayType() && !restVarType->IsETSResizableArrayType()) {
        return true;
    }
    auto targetType = checker->GetElementTypeOfArray(restVarType);
    if (substitutedSig->OwnerVar() == nullptr) {
        targetType = checker->MaybeBoxType(targetType);
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

static bool ValidateSignatureRestParams(ETSChecker *checker, Signature *substitutedSig,
                                        const ArenaVector<ir::Expression *> &arguments, TypeRelationFlag flags,
                                        bool reportError)
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
            if (!IsValidRestArgument(checker, argument, substitutedSig, flags, index)) {
                return false;
            }
            continue;
        }

        if (restCount > 1U) {
            if (reportError) {
                checker->LogError(diagnostic::MULTIPLE_SPREADS, {}, argument->Start());
            }
            return false;
        }

        auto *const restArgument = argument->AsSpreadElement()->Argument();
        Type *targetType = substitutedSig->RestVar()->TsType();
        // backing out of check that results in a signature mismatch would be difficult
        // so only attempt it if there is only one candidate signature
        restArgument->SetPreferredType(targetType);
        argument->Check(checker);
        auto const argumentType = restArgument->TsType();

        auto const invocationCtx = checker::InvocationContext(
            checker->Relation(), restArgument, argumentType, substitutedSig->RestVar()->TsType(), argument->Start(),
            {{diagnostic::REST_PARAM_INCOMPAT_AT, {argumentType, substitutedSig->RestVar()->TsType(), index + 1}}},
            flags);
        if (!invocationCtx.IsInvocable()) {
            if (restArgument->IsArrayExpression()) {
                checker->ModifyPreferredType(restArgument->AsArrayExpression(), nullptr);
                argument->SetTsType(nullptr);
            }
            return false;
        }
    }

    return true;
}

static Signature *ValidateSignature(
    ETSChecker *checker, std::tuple<Signature *, const ir::TSTypeParameterInstantiation *, TypeRelationFlag> info,
    const ArenaVector<ir::Expression *> &arguments, const lexer::SourcePosition &pos,
    const std::vector<bool> &argTypeInferenceRequired)
{
    auto [baseSignature, typeArguments, flags] = info;
    // In case of overloads, it is necessary to iterate through the compatible signatures again,
    // setting the boxing/unboxing flag for the arguments if needed.
    // So handle substitution arguments only in the case of unique function or collecting signature phase.
    Signature *const signature = ((flags & TypeRelationFlag::NO_SUBSTITUTION_NEEDED) != 0U)
                                     ? baseSignature
                                     : MaybeSubstituteTypeParameters(checker, info, arguments, pos);
    if (signature == nullptr) {
        return nullptr;
    }

    size_t const argCount = arguments.size();
    auto const hasRestParameter = signature->HasRestParameter();
    auto const reportError = (flags & TypeRelationFlag::NO_THROW) == 0;

    if (!ValidateRestParameter(checker, signature, arguments, pos, flags)) {
        return nullptr;
    }
    auto count = std::min(signature->ArgCount(), argCount);
    // Check all required formal parameter(s) first
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    if (!ValidateSignatureRequiredParams(checker, signature, arguments, flags, argTypeInferenceRequired, reportError)) {
        return nullptr;
    }

    // Check rest parameter(s) if any exists
    if (!hasRestParameter || (count >= argCount && !signature->RestVar()->TsType()->IsETSTupleType())) {
        return signature;
    }
    if (!ValidateSignatureRestParams(checker, signature, arguments, flags, reportError)) {
        return nullptr;
    }

    return signature;
}

bool IsSignatureAccessible(Signature *sig, ETSObjectType *containingClass, TypeRelation *relation)
{
    // NOTE(vivienvoros): this check can be removed if signature is implicitly declared as public according to the
    // spec.
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

static std::vector<bool> FindTypeInferenceArguments(const ArenaVector<ir::Expression *> &arguments)
{
    std::vector<bool> argTypeInferenceRequired(arguments.size());
    size_t index = 0;
    for (ir::Expression *arg : arguments) {
        if (arg->IsArrowFunctionExpression()) {
            ir::ScriptFunction *const lambda = arg->AsArrowFunctionExpression()->Function();
            if (ETSChecker::NeedTypeInference(lambda)) {
                argTypeInferenceRequired[index] = true;
            }
        }
        ++index;
    }
    return argTypeInferenceRequired;
}

static Signature *ValidateSignature(
    ETSChecker *checker, std::tuple<Signature *, const ir::TSTypeParameterInstantiation *, TypeRelationFlag> info,
    const ArenaVector<ir::Expression *> &arguments, const lexer::SourcePosition &pos,
    const std::vector<bool> &argTypeInferenceRequired);

// CC-OFFNXT(huge_method) solid logic
static ArenaVector<Signature *> CollectSignatures(ETSChecker *checker, ArenaVector<Signature *> &signatures,
                                                  const ir::TSTypeParameterInstantiation *typeArguments,
                                                  const ArenaVector<ir::Expression *> &arguments,
                                                  const lexer::SourcePosition &pos, TypeRelationFlag resolveFlags)
{
    ArenaVector<Signature *> compatibleSignatures(checker->ProgramAllocator()->Adapter());
    std::vector<bool> argTypeInferenceRequired = FindTypeInferenceArguments(arguments);
    Signature *notVisibleSignature = nullptr;

    if (signatures.size() > 1) {
        resolveFlags |= TypeRelationFlag::OVERLOADING_CONTEXT;
    }

    // CC-OFFNXT(G.RES.06-CPP) solid logic
    auto collectSignatures = [&](TypeRelationFlag relationFlags) {
        for (auto *sig : signatures) {
            if (notVisibleSignature != nullptr &&
                !IsSignatureAccessible(sig, checker->Context().ContainingClass(), checker->Relation())) {
                continue;
            }
            if (sig->HasSignatureFlag(SignatureFlags::BRIDGE)) {
                // Bridges are never invoked direcly
                continue;
            }
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            auto *concreteSig = ValidateSignature(checker, std::make_tuple(sig, typeArguments, relationFlags),
                                                  arguments, pos, argTypeInferenceRequired);
            if (concreteSig == nullptr) {
                continue;
            }
            if (notVisibleSignature == nullptr &&
                !IsSignatureAccessible(sig, checker->Context().ContainingClass(), checker->Relation())) {
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
        checker->LogError(diagnostic::SIG_INVISIBLE,
                          {notVisibleSignature->Function()->Id()->Name(), notVisibleSignature}, pos);
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

static bool CheckLambdaInfer(ETSChecker *checker, ir::AstNode *typeAnnotation,
                             ir::ArrowFunctionExpression *const arrowFuncExpr, Type *const subParameterType)
{
    if (typeAnnotation->IsETSTypeReference()) {
        typeAnnotation = util::Helpers::DerefETSTypeReference(typeAnnotation);
    }

    if (typeAnnotation->IsTSTypeParameter()) {
        return true;
    }

    if (!typeAnnotation->IsETSFunctionType()) {
        return false;
    }

    ir::ScriptFunction *const lambda = arrowFuncExpr->Function();
    auto calleeType = typeAnnotation->AsETSFunctionType();
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    checker->InferTypesForLambda(lambda, calleeType, subParameterType->AsETSFunctionType()->ArrowSignature());

    return true;
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP) solid logic
static bool CheckLambdaTypeAnnotation(ETSChecker *checker, ir::ETSParameterExpression *param,
                                      ir::ArrowFunctionExpression *const arrowFuncExpr, Type *const parameterType,
                                      TypeRelationFlag flags)
{
    ir::AstNode *typeAnnotation = param->Ident()->TypeAnnotation();
    if (typeAnnotation->IsETSTypeReference()) {
        typeAnnotation = util::Helpers::DerefETSTypeReference(typeAnnotation);
    }
    auto checkInvocable = [&arrowFuncExpr, &parameterType, checker](TypeRelationFlag functionFlags) {
        Type *const argumentType = arrowFuncExpr->Check(checker);
        functionFlags |= TypeRelationFlag::NO_THROW;

        checker::InvocationContext invocationCtx(checker->Relation(), arrowFuncExpr, argumentType, parameterType,
                                                 arrowFuncExpr->Start(), std::nullopt, functionFlags);
        return invocationCtx.IsInvocable();
    };

    //  process `single` type as usual.
    if (!typeAnnotation->IsETSUnionType()) {
        // #22952: infer optional parameter heuristics
        auto nonNullishParam = param->IsOptional() ? checker->GetNonNullishType(parameterType) : parameterType;
        ES2PANDA_ASSERT(nonNullishParam != nullptr);
        if (!nonNullishParam->IsETSFunctionType()) {
            arrowFuncExpr->Check(checker);
            return true;
        }
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return CheckLambdaInfer(checker, typeAnnotation, arrowFuncExpr, nonNullishParam) && checkInvocable(flags);
    }

    // Preserve actual lambda types
    ir::ScriptFunction *const lambda = arrowFuncExpr->Function();
    std::vector<ir::TypeNode *> lambdaParamTypes {};
    for (auto *const lambdaParam : lambda->Params()) {
        lambdaParamTypes.emplace_back(lambdaParam->AsETSParameterExpression()->Ident()->TypeAnnotation());
    }
    auto *const lambdaReturnTypeAnnotation = lambda->ReturnTypeAnnotation();

    if (!parameterType->IsETSUnionType() || parameterType->AsETSUnionType()->ConstituentTypes().size() !=
                                                typeAnnotation->AsETSUnionType()->Types().size()) {
        Type *const argumentType = arrowFuncExpr->Check(checker);
        return checker->Relation()->IsSupertypeOf(parameterType, argumentType);
    }

    const auto typeAnnsOfUnion = typeAnnotation->AsETSUnionType()->Types();
    const auto typeParamOfUnion = parameterType->AsETSUnionType()->ConstituentTypes();
    for (size_t ix = 0; ix < typeAnnsOfUnion.size(); ++ix) {
        auto *typeNode = typeAnnsOfUnion[ix];
        auto *paramNode = typeParamOfUnion[ix];
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        if (CheckLambdaInfer(checker, typeNode, arrowFuncExpr, paramNode) && checkInvocable(flags)) {
            return true;
        }

        //  Restore inferring lambda types:
        for (std::size_t i = 0U; i < lambda->Params().size(); ++i) {
            if (lambdaParamTypes[i] == nullptr) {
                lambda->Params()[i]->AsETSParameterExpression()->Ident()->SetTsTypeAnnotation(nullptr);
            }
        }
        if (lambdaReturnTypeAnnotation == nullptr) {
            lambda->SetReturnTypeAnnotation(nullptr);
        }
    }

    return false;
}

static bool ResolveLambdaArgumentType(ETSChecker *checker, Signature *signature, size_t paramPosition,
                                      std::pair<ir::Expression *, size_t> argumentInfo,
                                      TypeRelationFlag resolutionFlags)
{
    auto [argument, argumentPosition] = argumentInfo;
    if (!argument->IsArrowFunctionExpression()) {
        return true;
    }

    auto arrowFuncExpr = argument->AsArrowFunctionExpression();
    bool typeValid = true;
    ir::ScriptFunction *const lambda = arrowFuncExpr->Function();
    // Note: (Issue27688) if lambda is trailing lambda transferred, it must be in recheck.
    // its type was cleared before the check, so here we need recheck it.
    if (!checker->NeedTypeInference(lambda) && !lambda->IsTrailingLambda()) {
        return typeValid;
    }

    arrowFuncExpr->SetTsType(nullptr);
    auto *const param =
        signature->GetSignatureInfo()->params[paramPosition]->Declaration()->Node()->AsETSParameterExpression();
    Type *const parameterType = signature->Params()[paramPosition]->TsType();

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    bool rc = CheckLambdaTypeAnnotation(checker, param, arrowFuncExpr, parameterType, resolutionFlags);
    if (!rc) {
        if ((resolutionFlags & TypeRelationFlag::NO_THROW) == 0) {
            Type *const argumentType = arrowFuncExpr->Check(checker);
            checker->LogError(diagnostic::TYPE_MISMATCH_AT_IDX, {argumentType, parameterType, argumentPosition + 1},
                              arrowFuncExpr->Start());
        }
        rc = false;
    } else if ((lambda->Signature() != nullptr) && !lambda->HasReturnStatement()) {
        //  Need to check void return type here if there are no return statement(s) in the body.
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        if (!AssignmentContext(
                 // CC-OFFNXT(G.FMT.06-CPP) project code style
                 checker->Relation(), checker->ProgramAllocNode<ir::Identifier>(checker->ProgramAllocator()),
                 checker->GlobalVoidType(), lambda->Signature()->ReturnType(), lambda->Start(), std::nullopt,
                 checker::TypeRelationFlag::DIRECT_RETURN | checker::TypeRelationFlag::NO_THROW)
                 .IsAssignable()) {  // CC-OFF(G.FMT.02-CPP) project code style
            checker->LogError(diagnostic::ARROW_TYPE_MISMATCH,
                              {checker->GlobalVoidType(), lambda->Signature()->ReturnType()}, lambda->Body()->Start());
            rc = false;
        }
    }

    typeValid &= rc;

    return typeValid;
}

static bool TypeInference(ETSChecker *checker, Signature *signature, const ArenaVector<ir::Expression *> &arguments,
                          TypeRelationFlag inferenceFlags)
{
    bool typeConsistent = true;
    auto const argumentCount = arguments.size();
    auto const minArity = std::min(signature->ArgCount(), argumentCount);

    for (size_t idx = 0U; idx < minArity; ++idx) {
        auto const &argument = arguments[idx];

        if (idx == argumentCount - 1 && (inferenceFlags & TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA) != 0) {
            continue;
        }
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        const bool valid = ResolveLambdaArgumentType(checker, signature, idx, {argument, idx}, inferenceFlags);
        typeConsistent &= valid;
    }

    return typeConsistent;
}

static Signature *ChooseMostSpecificSignature(ETSChecker *checker, ArenaVector<Signature *> &signatures,
                                              const std::vector<bool> &argTypeInferenceRequired,
                                              const ArenaVector<ir::Expression *> &arguments,
                                              const lexer::SourcePosition &pos);

Signature *ETSChecker::GetMostSpecificSignature(ArenaVector<Signature *> &compatibleSignatures,
                                                const ArenaVector<ir::Expression *> &arguments,
                                                const lexer::SourcePosition &pos, TypeRelationFlag resolveFlags)
{
    std::vector<bool> argTypeInferenceRequired = FindTypeInferenceArguments(arguments);
    Signature *mostSpecificSignature =
        ChooseMostSpecificSignature(this, compatibleSignatures, argTypeInferenceRequired, arguments, pos);

    if (mostSpecificSignature == nullptr) {
        LogError(diagnostic::AMBIGUOUS_FUNC_REF, {compatibleSignatures.front()->Function()->Id()->Name()}, pos);
        return nullptr;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    if (!TypeInference(this, mostSpecificSignature, arguments, resolveFlags)) {
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
                //  NOTE (DZ): extra cases for some specific nodes can be added here (as for
                //  'ArrowFunctionExpression')
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

static void RemoveEnumTypeFlagIfNeed(ark::es2panda::checker::Signature *signature,
                                     const ArenaVector<ir::Expression *> &arguments)
{
    if (signature == nullptr) {
        return;
    }
    for (size_t index = 0; index < arguments.size(); ++index) {
        auto &argument = arguments[index];
        argument->RemoveAstNodeFlags(ir::AstNodeFlags::GENERATE_VALUE_OF);
    }
}

Signature *ETSChecker::ValidateSignatures(ArenaVector<Signature *> &signatures,
                                          const ir::TSTypeParameterInstantiation *typeArguments,
                                          const ArenaVector<ir::Expression *> &arguments,
                                          const lexer::SourcePosition &pos, std::string_view signatureKind,
                                          TypeRelationFlag resolveFlags)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto compatibleSignatures = CollectSignatures(this, signatures, typeArguments, arguments, pos, resolveFlags);
    if (!compatibleSignatures.empty()) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *sig = GetMostSpecificSignature(compatibleSignatures, arguments, pos, resolveFlags);
        // NOTE (DZ):   skip Promise<T> constructor/then/catch check -
        //              temporary solution, need to be removed after fixing OHOS code!
        if (sig == nullptr || !sig->HasFunction() ||
            !(sig->Function()->IsConstructor() || sig->Function()->Id()->Name().Is("then") ||
              sig->Function()->Id()->Name().Is("catch")) ||
            !sig->Owner()->Name().Is("Promise")) {
            RemoveEnumTypeFlagIfNeed(sig, arguments);
            // May need to re-check the arguments now that we know the particular signature to call.
            ValidateSignature(this,
                              {sig, nullptr, TypeRelationFlag::WIDENING | TypeRelationFlag::NO_SUBSTITUTION_NEEDED},
                              arguments, pos, FindTypeInferenceArguments(arguments));
        } else {
            ValidateSignature(
                this,
                {sig, nullptr,
                 TypeRelationFlag::WIDENING | TypeRelationFlag::NO_SUBSTITUTION_NEEDED | TypeRelationFlag::NO_THROW},
                arguments, pos, FindTypeInferenceArguments(arguments));
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

static Signature *FindMostSpecificSignature(const ArenaVector<Signature *> &signatures,
                                            const ArenaMultiMap<size_t, Signature *> &bestSignaturesForParameter,
                                            size_t paramCount)
{
    auto isMostSpecificForAllParams = [paramCount, &bestSignaturesForParameter](const Signature *sig) {
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
    return idx < sig->ArgCount() ? sig->Params().at(idx)->TsType()
                                 : checker->GetElementTypeOfArray(sig->RestVar()->TsType());
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

static void SearchAmongMostSpecificTypes(ETSChecker *checker, Type *&mostSpecificType, Signature *&prevSig,
                                         std::tuple<const lexer::SourcePosition &, size_t, Signature *> info,
                                         bool lookForClassType)
{
    auto [pos, idx, sig] = info;
    Type *sigType = GetParameterTypeOrRestAtIdx(checker, sig, idx);
    if (prevSig->HasFunction() && prevSig->Function()->Params()[idx]->IsETSParameterExpression()) {
        checker->Relation()->SetNode(prevSig->Function()->Params()[idx]->AsETSParameterExpression());
    }
    const bool isClassType =
        sigType->IsETSObjectType() && !sigType->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::INTERFACE);
    if (isClassType == lookForClassType) {
        if (checker->Relation()->IsIdenticalTo(sigType, mostSpecificType)) {
            checker->Relation()->SetNode(nullptr);
            return;
        }

        if (idx >= prevSig->MinArgCount() && idx < sig->MinArgCount()) {
            // NOTE (smartin): prefer non-optional parameters over optional ones
            checker->Relation()->Result(true);
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
            checker->Relation()->SetFlags(flags);
            if (sigType->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::BUILTIN_INT) ||
                checker->Relation()->IsLegalBoxedPrimitiveConversion(mostSpecificType, sigType)) {
                checker->Relation()->Result(true);
                mostSpecificType = sigType;
                prevSig = sig;
                return;
            }
        }
        if (checker->Relation()->IsAssignableTo(sigType, mostSpecificType)) {
            mostSpecificType = sigType;
            prevSig = sig;
        } else if (sigType->IsETSObjectType() && mostSpecificType->IsETSObjectType() &&
                   !checker->Relation()->IsAssignableTo(mostSpecificType, sigType) &&
                   !checker->Relation()->IsLegalBoxedPrimitiveConversion(sigType, mostSpecificType)) {
            auto funcName = sig->Function()->Id()->Name();
            checker->LogError(diagnostic::AMBIGUOUS_CALL, {funcName, funcName, funcName, prevSig, funcName, sig}, pos);
        }
    }
}

static void CollectSuitableSignaturesForTypeInference(ETSChecker *checker, size_t paramIdx,
                                                      ArenaVector<Signature *> &signatures,
                                                      ArenaMultiMap<size_t, Signature *> &bestSignaturesForParameter,
                                                      const ArenaVector<ir::Expression *> &arguments)
{
    // For lambda parameters, attempt to obtain the most matching signature through the number of lambda parameters
    ES2PANDA_ASSERT(arguments.at(paramIdx)->IsArrowFunctionExpression());
    [[maybe_unused]] size_t paramCount =
        arguments.at(paramIdx)->AsArrowFunctionExpression()->Function()->Params().size();
    size_t minMatchArgCount = SIZE_MAX;

    for (auto *sig : signatures) {
        auto *sigParamType = checker->GetNonNullishType(sig->Params().at(paramIdx)->TsType());
        ES2PANDA_ASSERT(sigParamType != nullptr);
        if (!sigParamType->IsETSFunctionType()) {
            continue;
        }

        auto sigParamArgCount = sigParamType->AsETSFunctionType()->ArrowSignature()->ArgCount();
        ES2PANDA_ASSERT(sigParamArgCount >= paramCount);

        minMatchArgCount = std::min(minMatchArgCount, sigParamArgCount);
    }

    for (auto *sig : signatures) {
        auto *sigParamType = checker->GetNonNullishType(sig->Params().at(paramIdx)->TsType());
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
        auto paramType = sig->Params().at(paramIdx)->TsType();
        if (paramIdx >= sig->Params().size() || !paramType->IsETSObjectType() ||
            !paramType->AsETSObjectType()->IsGlobalETSObjectType()) {
            bestSignaturesForParameter.insert({paramIdx, sig});
        }
    }
}

static ArenaMultiMap<size_t, Signature *> GetSuitableSignaturesForParameter(
    ETSChecker *checker, const std::vector<bool> &argTypeInferenceRequired, size_t paramCount,
    ArenaVector<Signature *> &signatures, const ArenaVector<ir::Expression *> &arguments,
    const lexer::SourcePosition &pos)
{
    // Collect which signatures are most specific for each parameter.
    ArenaMultiMap<size_t /* parameter index */, Signature *> bestSignaturesForParameter(
        checker->ProgramAllocator()->Adapter());

    const checker::SavedTypeRelationFlagsContext savedTypeRelationFlagCtx(checker->Relation(),
                                                                          TypeRelationFlag::ONLY_CHECK_WIDENING);

    for (size_t i = 0; i < paramCount; ++i) {
        if (i >= argTypeInferenceRequired.size()) {
            for (auto *sig : signatures) {
                bestSignaturesForParameter.insert({i, sig});
            }
            continue;
        }
        if (argTypeInferenceRequired[i]) {
            CollectSuitableSignaturesForTypeInference(checker, i, signatures, bestSignaturesForParameter, arguments);
            continue;
        }
        // 1st step: check which is the most specific parameter type for i. parameter.
        Type *mostSpecificType = signatures.front()->Params().at(i)->TsType();
        Signature *prevSig = signatures.front();

        // NOTE: first we choose the some signature with possibly widest argumetns' types
        // Then we search for the most specific signature
        InitMostSpecificType(checker->Relation(), signatures, mostSpecificType, prevSig, i);
        for (auto *sig : signatures) {
            SearchAmongMostSpecificTypes(checker, mostSpecificType, prevSig, std::make_tuple(pos, i, sig), true);
        }
        for (auto *sig : signatures) {
            SearchAmongMostSpecificTypes(checker, mostSpecificType, prevSig, std::make_tuple(pos, i, sig), false);
        }

        for (auto *sig : signatures) {
            Type *sigType = GetParameterTypeOrRestAtIdx(checker, sig, i);
            if (checker->Relation()->IsIdenticalTo(sigType, mostSpecificType) ||
                (sigType->IsETSFunctionType() && checker->Relation()->IsSupertypeOf(sigType, mostSpecificType))) {
                bestSignaturesForParameter.insert({i, sig});
            }
        }
    }
    return bestSignaturesForParameter;
}

static Signature *ChooseMostSpecificSignature(ETSChecker *checker, ArenaVector<Signature *> &signatures,
                                              const std::vector<bool> &argTypeInferenceRequired,
                                              const ArenaVector<ir::Expression *> &arguments,
                                              const lexer::SourcePosition &pos)
{
    ES2PANDA_ASSERT(signatures.empty() == false);

    if (signatures.size() == 1) {
        return signatures.front();
    }

    std::sort(signatures.begin(), signatures.end(),
              [](Signature *sig1, Signature *sig2) { return sig1->ArgCount() > sig2->ArgCount(); });

    size_t const paramCount = signatures.front()->ArgCount();
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
            std::any_of(signatures.begin(), signatures.end(), [signatures, checker](const auto *param) {
                auto left = checker->MaybeBoxType(checker->GetElementTypeOfArray(param->RestVar()->TsType()));
                auto right =
                    // CC-OFFNXT(G.FMT.02) project code style
                    checker->MaybeBoxType(checker->GetElementTypeOfArray(signatures.front()->RestVar()->TsType()));
                return !checker->Relation()->IsIdenticalTo(left, right);
            })) {
            checker->LogError(diagnostic::AMBIGUOUS_CALL_2, {signatures.front()->Function()->Id()->Name()}, pos);
            return nullptr;
        }
        // Else return the signature with the rest parameter
        auto restParamSignature = std::find_if(signatures.begin(), signatures.end(),
                                               [](auto *signature) { return signature->RestVar() != nullptr; });
        return *restParamSignature;
    }

    ArenaMultiMap<size_t /* parameter index */, Signature *> bestSignaturesForParameter =
        GetSuitableSignaturesForParameter(checker, argTypeInferenceRequired, paramCount, signatures, arguments, pos);
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

static Signature *ResolvePotentialTrailingLambdaWithReceiver(ETSChecker *checker, ir::CallExpression *callExpr,
                                                             ArenaVector<Signature *> const &signatures,
                                                             ArenaVector<ir::Expression *> &arguments)
{
    auto *trailingLambda = arguments.back()->AsArrowFunctionExpression();
    ArenaVector<Signature *> normalSig(checker->ProgramAllocator()->Adapter());
    ArenaVector<Signature *> sigContainLambdaWithReceiverAsParam(checker->ProgramAllocator()->Adapter());
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
        signature = checker->ValidateSignatures(
            sigContainLambdaWithReceiverAsParam, callExpr->TypeParams(), arguments, callExpr->Start(), "call",
            TypeRelationFlag::NO_THROW | TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA);
        if (signature != nullptr) {
            return signature;
        }
        sigContainLambdaWithReceiverAsParam.clear();
        trailingLambda->Function()->ClearParams();
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return checker->ValidateSignatures(normalSig, callExpr->TypeParams(), arguments, callExpr->Start(), "call",
                                       TypeRelationFlag::NO_THROW | TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA);
}

static Signature *MakeSignatureInvocable(ETSChecker *checker, Signature *sig, ir::CallExpression *callExpr)
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
            checker->Relation(), callExpr->Arguments().at(idx), callExpr->Arguments().at(idx)->TsType(),
            sig->Params().at(idx)->TsType(), callExpr->Arguments().at(idx)->Start(),
            {{diagnostic::INVALID_ASSIGNMNENT,
              {callExpr->Arguments().at(idx)->TsType(), sig->Params().at(idx)->TsType()}}});
        if (!ctx.IsAssignable()) {
            return nullptr;
        }
    }
    return sig;
}

static bool TrailingLambdaTypeInference(ETSChecker *checker, Signature *signature,
                                        const ArenaVector<ir::Expression *> &arguments)
{
    if (arguments.empty() || signature->GetSignatureInfo()->params.empty()) {
        return false;
    }
    ES2PANDA_ASSERT(arguments.back()->IsArrowFunctionExpression());
    const size_t lastParamPos = signature->GetSignatureInfo()->params.size() - 1;
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return ResolveLambdaArgumentType(checker, signature, lastParamPos, {arguments.back(), arguments.size() - 1},
                                     TypeRelationFlag::NONE);
}

static ArenaVector<ir::Expression *> ExtendArgumentsWithFakeLamda(ETSChecker *checker, ir::CallExpression *callExpr);
static void TransformTraillingLambda(ETSChecker *checker, ir::CallExpression *callExpr, Signature *sig);
static void EnsureValidCurlyBrace(ETSChecker *checker, ir::CallExpression *callExpr);

Signature *ETSChecker::ResolveCallExpressionAndTrailingLambda(ArenaVector<Signature *> &signatures,
                                                              ir::CallExpression *callExpr,
                                                              const lexer::SourcePosition &pos,
                                                              const TypeRelationFlag reportFlag)
{
    if (callExpr->TrailingBlock() == nullptr) {
        auto sig =
            // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
            ValidateSignatures(signatures, callExpr->TypeParams(), callExpr->Arguments(), pos, "call", reportFlag);
        sig = MakeSignatureInvocable(this, sig, callExpr);
        UpdateDeclarationFromSignature(this, callExpr, sig);
        return sig;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto arguments = ExtendArgumentsWithFakeLamda(this, callExpr);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto sig = ResolvePotentialTrailingLambdaWithReceiver(this, callExpr, signatures, arguments);
    if (sig != nullptr) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        TransformTraillingLambda(this, callExpr, sig);
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        TrailingLambdaTypeInference(this, sig, callExpr->Arguments());
        UpdateDeclarationFromSignature(this, callExpr, sig);
        callExpr->SetIsTrailingCall(true);
        return sig;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    sig = ValidateSignatures(signatures, callExpr->TypeParams(), callExpr->Arguments(), pos, "call", reportFlag);
    sig = MakeSignatureInvocable(this, sig, callExpr);
    if (sig != nullptr) {
        EnsureValidCurlyBrace(this, callExpr);
    }

    UpdateDeclarationFromSignature(this, callExpr, sig);
    return sig;
}

static Signature *MatchOrderSignatures(
    ETSChecker *checker,
    std::tuple<ArenaVector<Signature *> &, const ir::TSTypeParameterInstantiation *, TypeRelationFlag> info,
    const ArenaVector<ir::Expression *> &arguments, const lexer::SourcePosition &pos);

static void ThrowOverloadMismatch(ETSChecker *checker, util::StringView callName,
                                  const ArenaVector<ir::Expression *> &arguments, const lexer::SourcePosition &pos,
                                  std::string_view signatureKind);

Signature *ETSChecker::ResolveConstructExpression(ETSObjectType *type, const ArenaVector<ir::Expression *> &arguments,
                                                  const lexer::SourcePosition &pos)
{
    auto *var = type->GetProperty(compiler::Signatures::CONSTRUCTOR_NAME, PropertySearchFlags::SEARCH_STATIC_METHOD);
    if (var != nullptr && var->TsType()->IsETSFunctionType()) {
        auto sig = MatchOrderSignatures(
            this, {var->TsType()->AsETSFunctionType()->CallSignatures(), nullptr, TypeRelationFlag::NONE}, arguments,
            pos);
        if (sig == nullptr) {
            ThrowOverloadMismatch(this, type->Name(), arguments, pos, "construct");
        }
        return sig;
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return ValidateSignatures(type->ConstructSignatures(), nullptr, arguments, pos, "construct");
}

// Note: this function is extracted to reduce the size of `BuildMethodSignature`
static bool CollectOverload(checker::ETSChecker *checker, ir::MethodDefinition *method, ETSFunctionType *funcType)
{
    ir::OverloadInfo &ldInfo = method->GetOverloadInfoForUpdate();
    std::vector<ETSFunctionType *> overloads {};

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

static bool CheckRestParamOverload(Signature *funcSig, Signature *overloadSig, TypeRelation *relation)
{
    if (std::abs(static_cast<int32_t>(funcSig->ArgCount() - overloadSig->ArgCount())) != 1) {
        return true;
    }
    if (!relation->NoReturnTypeCheck() && !relation->IsIdenticalTo(funcSig->ReturnType(), overloadSig->ReturnType())) {
        return true;
    }

    for (size_t idx = 0; idx < std::min(funcSig->ArgCount(), overloadSig->ArgCount()); ++idx) {
        if (!relation->IsIdenticalTo(funcSig->Params()[idx]->TsType(), overloadSig->Params()[idx]->TsType())) {
            return true;
        }
    }

    auto isLastParamIdentical = [relation](Signature *withRest, Signature *withArray) {
        if (withArray->Params().empty()) {
            return false;
        }
        auto *lastParamType = withArray->Params().back()->TsType();
        bool isArray = lastParamType->IsETSArrayType() || lastParamType->IsETSResizableArrayType();
        return isArray && relation->IsIdenticalTo(lastParamType, withRest->RestVar()->TsType());
    };
    if ((funcSig->HasRestParameter() && isLastParamIdentical(funcSig, overloadSig)) ||
        (overloadSig->HasRestParameter() && isLastParamIdentical(overloadSig, funcSig))) {
        return false;
    }
    return true;
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

    auto *funcSig = func->CallSignatures()[0];
    auto *overloadSig = overload->CallSignatures()[0];
    Relation()->SignatureIsIdenticalTo(funcSig, overloadSig);
    if (Relation()->IsTrue() && funcSig->GetSignatureInfo()->restVar == overloadSig->GetSignatureInfo()->restVar) {
        LogError(diagnostic::FUNCTION_REDECL_BY_TYPE_SIG, {overload->Name().Mutf8()}, currentFunc->Start());
        return false;
    }

    if (funcSig->HasRestParameter() != overloadSig->HasRestParameter() &&
        !CheckRestParamOverload(funcSig, overloadSig, Relation())) {
        LogError(diagnostic::FUNCTION_REDECL_BY_TYPE_SIG, {overload->Name().Mutf8()}, currentFunc->Start());
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

            // NOTE(muhammet): Have to add optional arguments again so it doesn't break the assertion for rest tuples
            size_t nOpt = std::count_if(signatureInfo->params.begin(), signatureInfo->params.end(),
                                        [](varbinder::LocalVariable *var) {
                                            return var->Declaration()->Node()->AsETSParameterExpression()->IsOptional();
                                        });
            if (signatureInfo->restVar->TsType()->IsETSTupleType()) {
                signatureInfo->minArgCount += nOpt;
            }
        }
    }

    return signatureInfo;
}

static void ValidateMainSignature(ETSChecker *checker, ir::ScriptFunction *func)
{
    if (func->Params().size() >= 2U) {
        checker->LogError(diagnostic::MAIN_INVALID_ARG_COUNT, {}, func->Start());
        return;
    }

    if (func->Params().size() == 1) {
        auto const *const param = func->Params()[0]->AsETSParameterExpression();

        if (param->IsRestParameter()) {
            checker->LogError(diagnostic::MAIN_WITH_REST, {}, param->Start());
        }

        const auto paramType = param->Variable()->TsType();
        if (!paramType->IsETSArrayType() || !paramType->AsETSArrayType()->ElementType()->IsETSStringType()) {
            checker->LogError(diagnostic::MAIN_PARAM_NOT_ARR_OF_STRING, {}, param->Start());
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
        ValidateMainSignature(this, func);
    }

    VarBinder()->AsETSBinder()->BuildFunctionName(func);
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

static bool IsOverridableIn(Signature *signature)
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

static bool IsMethodOverridesOther(ETSChecker *checker, Signature *base, Signature *derived)
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
        SavedTypeRelationFlagsContext savedFlagsCtx(checker->Relation(), TypeRelationFlag::NO_RETURN_TYPE_CHECK |
                                                                             TypeRelationFlag::OVERRIDING_CONTEXT);
        if (checker->Relation()->SignatureIsSupertypeOf(base, derived)) {
            if (derived->HasSignatureFlag(SignatureFlags::STATIC)) {
                return false;
            }

            derived->Function()->SetOverride();
            return true;
        }
    }

    return false;
}

enum class OverrideErrorCode {
    NO_ERROR,
    OVERRIDDEN_FINAL,
    INCOMPATIBLE_RETURN,
    INCOMPATIBLE_TYPEPARAM,
    OVERRIDDEN_WEAKER,
    OVERRIDDEN_INTERNAL,
    ABSTRACT_OVERRIDES_CONCRETE,
};

static OverrideErrorCode CheckOverride(ETSChecker *checker, Signature *signature, Signature *other)
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
        if (!checker->IsReturnTypeSubstitutable(signature, other)) {
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

static void ReportOverrideError(ETSChecker *checker, Signature *signature, Signature *overriddenSignature,
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

    checker->LogError(diagnostic::CANNOT_OVERRIDE,
                      {signature->Function()->Id()->Name(), signature, signature->Owner(),
                       overriddenSignature->Function()->Id()->Name(), overriddenSignature, overriddenSignature->Owner(),
                       reason},
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

static bool CheckOverride(ETSChecker *checker, Signature *signature, ETSObjectType *site)
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
        if (!CheckTypeParameterConstraints(signature->TypeParams(), it->TypeParams(), checker->Relation())) {
            typeParamError = true;
        }

        auto *itSubst = checker->AdjustForTypeParameters(signature, it);

        if (itSubst == nullptr) {
            continue;
        }

        if (itSubst->HasSignatureFlag(SignatureFlags::ABSTRACT) || site->HasObjectFlag(ETSObjectFlags::INTERFACE)) {
            if ((itSubst->Function()->IsSetter() && !signature->Function()->IsSetter()) ||
                (itSubst->Function()->IsGetter() && !signature->Function()->IsGetter())) {
                continue;
            }
        }

        if (!IsMethodOverridesOther(checker, itSubst, signature)) {
            continue;
        }

        if (typeParamError) {
            ReportOverrideError(checker, signature, it, OverrideErrorCode::INCOMPATIBLE_TYPEPARAM);
            return false;
        }

        if (auto err = CheckOverride(checker, signature, itSubst); err != OverrideErrorCode::NO_ERROR) {
            ReportOverrideError(checker, signature, it, err);
            return false;
        }

        isOverridingAnySignature = true;
    }

    return isOverridingAnySignature;
}

static bool CheckInterfaceOverride(ETSChecker *const checker, ETSObjectType *const interface,
                                   Signature *const signature)
{
    bool isOverriding = CheckOverride(checker, signature, interface);

    for (auto *const superInterface : interface->Interfaces()) {
        isOverriding |= CheckInterfaceOverride(checker, superInterface, signature);
    }

    return isOverriding;
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
        isOverriding |= checker::CheckOverride(this, signature, iter);

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

static bool NeedToVerifySignatureVisibility(ETSChecker *checker, Signature *signature, const lexer::SourcePosition &pos)
{
    if (signature == nullptr) {
        checker->LogError(diagnostic::SIG_UNAVAILABLE, {}, pos);
        return false;
    }

    return (checker->Context().Status() & CheckerStatus::IGNORE_VISIBILITY) == 0U &&
           (signature->HasSignatureFlag(SignatureFlags::PRIVATE) ||
            signature->HasSignatureFlag(SignatureFlags::PROTECTED));
}

void ETSChecker::ValidateSignatureAccessibility(ETSObjectType *callee, Signature *signature,
                                                const lexer::SourcePosition &pos,
                                                const MaybeDiagnosticInfo &maybeErrorInfo)
{
    if (!NeedToVerifySignatureVisibility(this, signature, pos)) {
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

static void MoveTrailingBlockToEnclosingBlockStatement(ir::CallExpression *callExpr)
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

static ir::ScriptFunction *CreateLambdaFunction(ETSChecker *checker, ir::BlockStatement *trailingBlock, Signature *sig)
{
    auto *funcParamScope = varbinder::LexicalScope<varbinder::FunctionParamScope>(checker->VarBinder()).GetScope();
    auto paramCtx =
        varbinder::LexicalScope<varbinder::FunctionParamScope>::Enter(checker->VarBinder(), funcParamScope, false);

    auto funcCtx = varbinder::LexicalScope<varbinder::FunctionScope>(checker->VarBinder());
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

    ArenaVector<ir::Expression *> params(checker->ProgramAllocator()->Adapter());
    ir::ScriptFunctionFlags flags = ir::ScriptFunctionFlags::ARROW;
    bool trailingLambdaHasReceiver = false;
    if (IsLastParameterLambdaWithReceiver(sig)) {
        auto *actualLambdaType =
            sig->Function()->Params().back()->AsETSParameterExpression()->TypeAnnotation()->AsETSFunctionType();
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *receiverOfTrailingBlock =
            actualLambdaType->Params()[0]->Clone(checker->ProgramAllocator(), nullptr)->AsExpression();
        auto *receiverVar = receiverOfTrailingBlock->AsETSParameterExpression()->Ident()->Variable();
        auto *receiverVarClone = checker->ProgramAllocator()->New<varbinder::LocalVariable>(receiverVar->Declaration(),
                                                                                            receiverVar->Flags());
        receiverVarClone->SetTsType(receiverVar->TsType());
        receiverVarClone->SetScope(funcParamScope);
        funcScope->InsertBinding(receiverVarClone->Name(), receiverVarClone);
        receiverOfTrailingBlock->AsETSParameterExpression()->Ident()->SetVariable(receiverVarClone);
        params.emplace_back(receiverOfTrailingBlock);
        trailingLambdaHasReceiver = true;
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *funcNode = checker->ProgramAllocNode<ir::ScriptFunction>(
        checker->ProgramAllocator(),
        ir::ScriptFunction::ScriptFunctionData {
            trailingBlock, ir::FunctionSignature(nullptr, std::move(params), nullptr, trailingLambdaHasReceiver),
            flags});
    funcNode->SetScope(funcScope);
    funcScope->BindNode(funcNode);
    funcParamScope->BindNode(funcNode);

    trailingBlock->SetScope(funcScope);

    return funcNode;
}

static ir::ScriptFunction *CreateLambdaFunction(ETSChecker *checker, ir::BlockStatement *trailingBlock, Signature *sig);

static void TransformTraillingLambda(ETSChecker *checker, ir::CallExpression *callExpr, Signature *sig)
{
    auto *trailingBlock = callExpr->TrailingBlock();
    ES2PANDA_ASSERT(trailingBlock != nullptr);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *funcNode = CreateLambdaFunction(checker, trailingBlock, sig);
    funcNode->AddFlag(ir::ScriptFunctionFlags::TRAILING_LAMBDA);
    checker->ReplaceScope(funcNode->Body(), trailingBlock, funcNode->Scope());
    callExpr->SetTrailingBlock(nullptr);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *arrowFuncNode = checker->ProgramAllocNode<ir::ArrowFunctionExpression>(funcNode, checker->ProgramAllocator());
    arrowFuncNode->SetRange(trailingBlock->Range());
    arrowFuncNode->SetParent(callExpr);
    callExpr->Arguments().push_back(arrowFuncNode);
}

static ArenaVector<ir::Expression *> ExtendArgumentsWithFakeLamda(ETSChecker *checker, ir::CallExpression *callExpr)
{
    auto funcCtx = varbinder::LexicalScope<varbinder::FunctionScope>(checker->VarBinder());
    auto *funcScope = funcCtx.GetScope();
    ArenaVector<ir::Expression *> params(checker->ProgramAllocator()->Adapter());

    ArenaVector<ir::Statement *> statements(checker->ProgramAllocator()->Adapter());
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *body = checker->ProgramAllocNode<ir::BlockStatement>(checker->ProgramAllocator(), std::move(statements));
    ES2PANDA_ASSERT(body != nullptr);
    body->SetScope(funcScope);

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *funcNode = checker->ProgramAllocNode<ir::ScriptFunction>(
        checker->ProgramAllocator(),
        ir::ScriptFunction::ScriptFunctionData {body, ir::FunctionSignature(nullptr, std::move(params), nullptr),
                                                ir::ScriptFunctionFlags::ARROW});
    ES2PANDA_ASSERT(funcNode != nullptr);
    funcNode->SetScope(funcScope);
    funcScope->BindNode(funcNode);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto *arrowFuncNode = checker->ProgramAllocNode<ir::ArrowFunctionExpression>(funcNode, checker->ProgramAllocator());
    ES2PANDA_ASSERT(arrowFuncNode != nullptr);
    arrowFuncNode->SetParent(callExpr);

    ArenaVector<ir::Expression *> fakeArguments = callExpr->Arguments();
    fakeArguments.push_back(arrowFuncNode);
    return fakeArguments;
}

static void EnsureValidCurlyBrace(ETSChecker *checker, ir::CallExpression *callExpr)
{
    if (callExpr->TrailingBlock() == nullptr) {
        return;
    }

    if (callExpr->IsTrailingBlockInNewLine()) {
        MoveTrailingBlockToEnclosingBlockStatement(callExpr);
        return;
    }

    checker->LogError(diagnostic::NO_SUCH_SIG_WITH_TRAILING_LAMBDA, {}, callExpr->Start());
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

static Signature *ResolveTrailingLambda(ETSChecker *checker, ArenaVector<Signature *> &signatures,
                                        ir::CallExpression *callExpr, const lexer::SourcePosition &pos,
                                        const TypeRelationFlag reportFlag = TypeRelationFlag::NONE);

Signature *ETSChecker::FirstMatchSignatures(ir::CallExpression *expr, checker::Type *calleeType)
{
    if (expr->TrailingBlock() == nullptr) {
        auto *signature = MatchOrderSignatures(this,
                                               {calleeType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow(),
                                                expr->TypeParams(), TypeRelationFlag::NONE},
                                               expr->Arguments(), expr->Start());
        if (signature == nullptr) {
            ThrowOverloadMismatch(this, calleeType->AsETSFunctionType()->Name(), expr->Arguments(), expr->Start(),
                                  "call");
            return nullptr;
        }
        UpdateDeclarationFromSignature(this, expr, signature);
        return signature;
    }

    auto &signatures = expr->IsETSConstructorCall() ? calleeType->AsETSObjectType()->ConstructSignatures()
                                                    : calleeType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow();
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return ResolveTrailingLambda(this, signatures, expr, expr->Start());
}

static Signature *ValidateOrderSignature(
    ETSChecker *checker, std::tuple<Signature *, const ir::TSTypeParameterInstantiation *, TypeRelationFlag> info,
    const ArenaVector<ir::Expression *> &arguments, const lexer::SourcePosition &pos,
    const std::vector<bool> &argTypeInferenceRequired);

static void CleanArgumentsInformation(const ArenaVector<ir::Expression *> &arguments)
{
    if (arguments.empty()) {
        return;
    }
    for (auto *argument : arguments) {
        argument->CleanCheckInformation();
    }
}

static Signature *MatchOrderSignatures(
    ETSChecker *checker,
    std::tuple<ArenaVector<Signature *> &, const ir::TSTypeParameterInstantiation *, TypeRelationFlag> info,
    const ArenaVector<ir::Expression *> &arguments, const lexer::SourcePosition &pos)
{
    auto [signatures, typeArguments, resolveFlags] = info;
    Signature *notVisibleSignature = nullptr;
    std::vector<bool> argTypeInferenceRequired = FindTypeInferenceArguments(arguments);

    auto const validateFlags = signatures.size() == 1
                                   ? TypeRelationFlag::WIDENING | resolveFlags
                                   : TypeRelationFlag::WIDENING | TypeRelationFlag::NO_THROW | resolveFlags;

    for (auto *sig : signatures) {
        if (notVisibleSignature != nullptr &&
            !IsSignatureAccessible(sig, checker->Context().ContainingClass(), checker->Relation())) {
            continue;
        }
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *concreteSig = ValidateOrderSignature(checker, std::make_tuple(sig, typeArguments, validateFlags),
                                                   arguments, pos, argTypeInferenceRequired);
        if (concreteSig == nullptr) {
            CleanArgumentsInformation(arguments);
            continue;
        }
        if (notVisibleSignature == nullptr &&
            !IsSignatureAccessible(sig, checker->Context().ContainingClass(), checker->Relation())) {
            CleanArgumentsInformation(arguments);
            notVisibleSignature = concreteSig;
        } else {
            return concreteSig;
        }
    }

    if (notVisibleSignature != nullptr && ((resolveFlags & TypeRelationFlag::NO_THROW) == 0)) {
        checker->LogError(diagnostic::SIG_INVISIBLE,
                          {notVisibleSignature->Function()->Id()->Name(), notVisibleSignature}, pos);
    }

    return nullptr;
}

static bool ValidateOrderSignatureRequiredParams(ETSChecker *checker, Signature *substitutedSig,
                                                 const ArenaVector<ir::Expression *> &arguments, TypeRelationFlag flags,
                                                 const std::vector<bool> &argTypeInferenceRequired);

static Signature *ValidateOrderSignature(
    ETSChecker *checker, std::tuple<Signature *, const ir::TSTypeParameterInstantiation *, TypeRelationFlag> info,
    const ArenaVector<ir::Expression *> &arguments, const lexer::SourcePosition &pos,
    const std::vector<bool> &argTypeInferenceRequired)
{
    auto [baseSignature, typeArguments, flags] = info;
    // In case of overloads, it is necessary to iterate through the compatible signatures again,
    // setting the boxing/unboxing flag for the arguments if needed.
    // So handle substitution arguments only in the case of unique function or collecting signature phase.
    Signature *const signature = MaybeSubstituteTypeParameters(checker, info, arguments, pos);
    if (signature == nullptr) {
        return nullptr;
    }

    size_t const argCount = arguments.size();
    auto const hasRestParameter = signature->RestVar() != nullptr;
    size_t compareCount = argCount;
    if ((flags & TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA) != 0 && !signature->Params().empty() &&
        signature->Params().back()->Declaration()->Node()->AsETSParameterExpression()->IsOptional()) {
        compareCount = compareCount - 1;
    }

    const bool throwError = (flags & TypeRelationFlag::NO_THROW) == 0;
    if (compareCount < signature->MinArgCount() || (argCount > signature->ArgCount() && !hasRestParameter)) {
        if (throwError) {
            checker->LogError(diagnostic::PARAM_COUNT_MISMATCH, {signature->MinArgCount(), argCount}, pos);
        }
        return nullptr;
    }

    if (argCount > signature->ArgCount() && hasRestParameter && (flags & TypeRelationFlag::IGNORE_REST_PARAM) != 0) {
        return nullptr;
    }

    auto count = std::min(signature->ArgCount(), argCount);
    // Check all required formal parameter(s) first
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    if (!ValidateOrderSignatureRequiredParams(checker, signature, arguments, flags, argTypeInferenceRequired)) {
        return nullptr;
    }

    // Check rest parameter(s) if any exists
    if (!hasRestParameter || (count >= argCount && !signature->RestVar()->TsType()->IsETSTupleType())) {
        return signature;
    }

    if (!ValidateSignatureRestParams(checker, signature, arguments, flags, true)) {
        return nullptr;
    }

    return signature;
}

static bool SetPreferredTypeBeforeValidate(ETSChecker *checker, Signature *substitutedSig, ir::Expression *argument,
                                           size_t index, const std::vector<bool> &argTypeInferenceRequired)
{
    auto const paramType = checker->GetNonNullishType(substitutedSig->Params()[index]->TsType());
    if (argument->IsObjectExpression()) {
        if (!paramType->IsETSObjectType()) {
            return false;
        }
        if (paramType->AsETSObjectType()->IsBoxedPrimitive()) {
            return false;
        }
        argument->SetPreferredType(paramType);
    }

    if (argument->IsMemberExpression()) {
        checker->SetArrayPreferredTypeForNestedMemberExpressions(argument->AsMemberExpression(), paramType);
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
        return CheckLambdaInfer(checker, param->TypeAnnotation(), argument->AsArrowFunctionExpression(), paramType);
    }

    return true;
}

static bool ValidateOrderSignatureInvocationContext(ETSChecker *checker, Signature *substitutedSig,
                                                    ir::Expression *argument, std::size_t index,
                                                    TypeRelationFlag flags);

static bool ValidateOrderSignatureRequiredParams(ETSChecker *checker, Signature *substitutedSig,
                                                 const ArenaVector<ir::Expression *> &arguments, TypeRelationFlag flags,
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
    bool throwError = (flags & TypeRelationFlag::NO_THROW) == 0;
    for (size_t index = 0; index < commonArity; ++index) {
        const auto &argument = arguments[index];
        auto *const paramType = checker->GetNonNullishType(substitutedSig->Params()[index]->TsType());
        if (!SetPreferredTypeBeforeValidate(checker, substitutedSig, argument, index, argTypeInferenceRequired)) {
            return false;
        }

        if (argument->IsSpreadElement()) {
            if (throwError) {
                checker->LogError(diagnostic::SPREAD_ONTO_SINGLE_PARAM, {}, argument->Start());
            }
            return false;
        }

        ClearPreferredTypeForArray(checker, argument, paramType, flags, false);

        if (argument->IsIdentifier() && IsInvalidArgumentAsIdentifier(checker->Scope(), argument->AsIdentifier())) {
            if (throwError) {
                checker->LogError(diagnostic::ARG_IS_CLASS_ID, {}, argument->Start());
            }
            return false;
        }

        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        if (!ValidateOrderSignatureInvocationContext(checker, substitutedSig, argument, index, flags)) {
            return false;
        }
    }

    if ((flags & TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA) != 0 && arguments.back()->IsArrowFunctionExpression()) {
        ir::ScriptFunction *const lambda = arguments.back()->AsArrowFunctionExpression()->Function();
        auto targetParm = substitutedSig->GetSignatureInfo()->params.back()->Declaration()->Node();
        if (!CheckLambdaAssignable(checker, targetParm->AsETSParameterExpression(), lambda)) {
            return false;
        }
    }
    return true;
}

static bool ValidateOrderSignatureInvocationContext(ETSChecker *checker, Signature *substitutedSig,
                                                    ir::Expression *argument, std::size_t index, TypeRelationFlag flags)
{
    Type *targetType = substitutedSig->Params()[index]->TsType();
    // NOTE (smartin): remove these flag hacks after the overload resolution is completely reworked
    if ((flags & TypeRelationFlag::NO_THROW) != 0) {
        argument->AddAstNodeFlags(ir::AstNodeFlags::NO_THROW);
        argument->IterateRecursively([](ir::AstNode *node) { node->AddAstNodeFlags(ir::AstNodeFlags::NO_THROW); });
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    Type *argumentType = argument->Check(checker);

    if ((flags & TypeRelationFlag::NO_THROW) != 0) {
        argument->RemoveAstNodeFlags(ir::AstNodeFlags::NO_THROW);
        argument->IterateRecursively([](ir::AstNode *node) { node->RemoveAstNodeFlags(ir::AstNodeFlags::NO_THROW); });
    }

    bool isAnyErroneousNodeInArgument = false;
    argument->IterateRecursively([checker, &isAnyErroneousNodeInArgument](ir::AstNode *node) {
        if (node->IsExpression() && node->AsExpression()->TsType() == checker->GlobalTypeError()) {
            isAnyErroneousNodeInArgument = true;
            return;
        }
    });
    if (isAnyErroneousNodeInArgument || argument->TsType()->IsTypeError()) {
        // Some checks invalidate the type of argument node (if the type is erroneous), but won't log an error into
        // diagnostics. This is needed as overload signatures try to match a signature one after another, and a not
        // matching signature doesn't necessarily indicate wrong code (as an overload later in the overload list may
        // match with the argument type). This can happen with expressions that needs their type inferred (eg. array
        // expressions, object literals ...). When we set a 'preferred type' to a non-matching type to the expression
        // (eg. we set wrong arity tuple), we don't want to throw an error during the check of the expression (as said,
        // because another overload may set the correct 'preferred type' for it). Type error type is assignable to
        // anything (so the invocation ctx check on next line will pass), but we don't want to allow a call with
        // incorrect arguments. In this case a signature with bad parameter type and argument with 'ErrorType' will be
        // chosen, which may cause errors in later phases (as a phase terminates if an error happened).
        // This solution is brittle (but better than deleting all diagnostics thrown during argument check), so when
        // overload resolution will be reworked, it'll need to be deleted.
        return false;
    }

    flags |= TypeRelationFlag::ONLY_CHECK_WIDENING;

    auto const invocationCtx =
        checker::InvocationContext(checker->Relation(), argument, argumentType, targetType, argument->Start(),
                                   {{diagnostic::TYPE_MISMATCH_AT_IDX, {argumentType, targetType, index + 1}}}, flags);

    return invocationCtx.IsInvocable();
}

static Signature *ResolvePotentialTrailingLambda(ETSChecker *checker, ir::CallExpression *callExpr,
                                                 ArenaVector<Signature *> const &signatures,
                                                 ArenaVector<ir::Expression *> &arguments);

static Signature *ResolveTrailingLambda(ETSChecker *checker, ArenaVector<Signature *> &signatures,
                                        ir::CallExpression *callExpr, const lexer::SourcePosition &pos,
                                        const TypeRelationFlag reportFlag)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto arguments = ExtendArgumentsWithFakeLamda(checker, callExpr);
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    auto sig = ResolvePotentialTrailingLambda(checker, callExpr, signatures, arguments);
    if (sig != nullptr) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        TransformTraillingLambda(checker, callExpr, sig);
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        TrailingLambdaTypeInference(checker, sig, callExpr->Arguments());
        UpdateDeclarationFromSignature(checker, callExpr, sig);
        callExpr->SetIsTrailingCall(true);
        return sig;
    }

    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    sig = MatchOrderSignatures(checker, {signatures, callExpr->TypeParams(), reportFlag}, callExpr->Arguments(), pos);
    if (sig != nullptr) {
        EnsureValidCurlyBrace(checker, callExpr);
    }

    UpdateDeclarationFromSignature(checker, callExpr, sig);
    return sig;
}

static Signature *ResolvePotentialTrailingLambda(ETSChecker *checker, ir::CallExpression *callExpr,
                                                 ArenaVector<Signature *> const &signatures,
                                                 ArenaVector<ir::Expression *> &arguments)
{
    auto *trailingLambda = arguments.back()->AsArrowFunctionExpression();
    ArenaVector<Signature *> normalSig(checker->ProgramAllocator()->Adapter());
    ArenaVector<Signature *> sigContainLambdaWithReceiverAsParam(checker->ProgramAllocator()->Adapter());
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
        auto signature = MatchOrderSignatures(checker,
                                              {sigContainLambdaWithReceiverAsParam, callExpr->TypeParams(),
                                               TypeRelationFlag::NO_THROW | TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA},
                                              arguments, callExpr->Start());
        if (signature != nullptr) {
            return signature;
        }
        sigContainLambdaWithReceiverAsParam.clear();
        trailingLambda->Function()->ClearParams();
    }
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    return MatchOrderSignatures(
        checker,
        {normalSig, callExpr->TypeParams(), TypeRelationFlag::NO_THROW | TypeRelationFlag::NO_CHECK_TRAILING_LAMBDA},
        arguments, callExpr->Start());
}

std::optional<Substitution> ETSChecker::CheckTypeParamsAndBuildSubstitutionIfValid(
    Signature *signature, const ArenaVector<ir::TypeNode *> &params, const lexer::SourcePosition &pos)
{
    return BuildExplicitSubstitutionForArguments(this, signature, params, pos, TypeRelationFlag::NONE);
}

static void ThrowOverloadMismatch(ETSChecker *checker, util::StringView callName,
                                  const ArenaVector<ir::Expression *> &arguments, const lexer::SourcePosition &pos,
                                  std::string_view signatureKind)
{
    std::string msg {};
    msg.append(callName.Mutf8());
    msg += "(";

    for (std::size_t index = 0U; index < arguments.size(); ++index) {
        auto const &argument = arguments[index];
        Type const *const argumentType = argument->Check(checker);
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
    checker->LogError(diagnostic::NO_MATCHING_SIG, {signatureKind, msg.c_str()}, pos);
}

}  // namespace ark::es2panda::checker
