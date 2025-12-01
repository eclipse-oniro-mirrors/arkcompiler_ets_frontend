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

#ifndef ES2PANDA_COMPILER_CHECKER_ETS_FUNCTION_HELPERS_H
#define ES2PANDA_COMPILER_CHECKER_ETS_FUNCTION_HELPERS_H

#include "checker/ETSchecker.h"
#include "checker/ets/typeRelationContext.h"
#include "checker/types/ets/etsObjectType.h"
#include "checker/types/type.h"
#include "ir/base/catchClause.h"
#include "ir/base/classDefinition.h"
#include "ir/base/classProperty.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/base/spreadElement.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/memberExpression.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/doWhileStatement.h"
#include "ir/statements/expressionStatement.h"
#include "ir/statements/forInStatement.h"
#include "ir/statements/forOfStatement.h"
#include "ir/statements/forUpdateStatement.h"
#include "ir/statements/switchStatement.h"
#include "ir/statements/whileStatement.h"
#include "ir/ts/tsTypeParameterInstantiation.h"
#include "parser/program/program.h"
#include "util/helpers.h"
#include "util/language.h"
#include "varbinder/declaration.h"
#include "varbinder/ETSBinder.h"
#include "varbinder/scope.h"
#include "varbinder/varbinder.h"
#include "varbinder/variable.h"
#include "varbinder/variableFlags.h"

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
        if (!checker->IsCompatibleTypeArgument(sigParams[ix]->AsETSTypeParameter(), instArgs[ix],
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

static Signature *MaybeSubstituteTypeParameters(ETSChecker *checker, Signature *signature,
                                                const ir::TSTypeParameterInstantiation *typeArguments,
                                                const ArenaVector<ir::Expression *> &arguments,
                                                const lexer::SourcePosition &pos, TypeRelationFlag flags)
{
    if (typeArguments == nullptr && signature->GetSignatureInfo()->typeParams.empty()) {
        return signature;
    }

    const std::optional<Substitution> substitution =
        (typeArguments != nullptr)
            ? BuildExplicitSubstitutionForArguments(checker, signature, typeArguments->Params(), pos, flags)
            : BuildImplicitSubstitutionForArguments(checker, signature, arguments);

    return (!substitution.has_value()) ? nullptr : signature->Substitute(checker->Relation(), &substitution.value());
}

static bool CheckInterfaceOverride(ETSChecker *const checker, ETSObjectType *const interface,
                                   Signature *const signature)
{
    bool isOverriding = checker->CheckOverride(signature, interface);

    for (auto *const superInterface : interface->Interfaces()) {
        isOverriding |= CheckInterfaceOverride(checker, superInterface, signature);
    }

    return isOverriding;
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

}  // namespace ark::es2panda::checker

#endif
