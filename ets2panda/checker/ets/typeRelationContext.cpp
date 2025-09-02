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

#include "typeRelationContext.h"

namespace ark::es2panda::checker {
bool AssignmentContext::ValidateArrayTypeInitializerByElement(TypeRelation *relation, ir::ArrayExpression *node,
                                                              ETSArrayType *target)
{
    bool ok = true;
    ES2PANDA_ASSERT(node != nullptr);
    ES2PANDA_ASSERT(target != nullptr);
    if (target->IsETSTupleType()) {
        return true;
    }

    for (uint32_t index = 0; index < node->Elements().size(); index++) {
        ir::Expression *currentArrayElem = node->Elements()[index];
        auto const currentArrayElementType = currentArrayElem->Check(relation->GetChecker()->AsETSChecker());

        if (!AssignmentContext(relation, currentArrayElem,
                               currentArrayElem->Check(relation->GetChecker()->AsETSChecker()), target->ElementType(),
                               currentArrayElem->Start(), std::nullopt, TypeRelationFlag::NO_THROW)
                 // CC-OFFNXT(G.FMT.06-CPP,G.FMT.02-CPP) project code style
                 .IsAssignable()) {
            relation->GetChecker()->LogError(diagnostic::ARRAY_ELEMENT_INIT_TYPE_INCOMPAT,
                                             {index, currentArrayElementType, target->ElementType()},
                                             currentArrayElem->Start());
            ok = false;
        }
    }
    return ok;
}

bool InstantiationContext::ValidateTypeArguments(ETSObjectType *type, ir::TSTypeParameterInstantiation *typeArgs,
                                                 const lexer::SourcePosition &pos)
{
    if (checker_->HasStatus(CheckerStatus::IN_INSTANCEOF_CONTEXT)) {
        if (typeArgs != nullptr) {
            checker_->LogDiagnostic(diagnostic::INSTANCEOF_ERASED, {type->Name()}, pos);
        }
        result_ = type;
        return true;
    }
    if (!checker_->CheckNumberOfTypeArguments(type, typeArgs, pos)) {
        result_ = checker_->GlobalTypeError();
        return true;
        // the return value 'true' of this function let Instantiationcontext constructor return immediately.
    }
    if (type->TypeArguments().empty()) {
        result_ = type;
        return true;
    }
    return false;
}

// CC-OFFNXT(huge_depth[C++]) solid logic
void InstantiationContext::InstantiateType(ETSObjectType *type, ir::TSTypeParameterInstantiation *typeArgs)
{
    ArenaVector<Type *> typeArgTypes(checker_->Allocator()->Adapter());
    typeArgTypes.reserve(type->TypeArguments().size());

    if (typeArgs != nullptr) {
        for (auto *const it : typeArgs->Params()) {
            auto *paramType = it->GetType(checker_);
            ES2PANDA_ASSERT(paramType != nullptr);
            if (paramType->IsTypeError()) {
                result_ = paramType;
                return;
            }
            ES2PANDA_ASSERT(!paramType->IsETSPrimitiveType());
            typeArgTypes.push_back(paramType);
        }
    }

    while (typeArgTypes.size() < type->TypeArguments().size()) {
        Type *defaultType = nullptr;
        if (type->TypeArguments().at(typeArgTypes.size())->IsETSTypeParameter()) {
            defaultType = type->TypeArguments().at(typeArgTypes.size())->AsETSTypeParameter()->GetDefaultType();
        } else {
            defaultType = type->TypeArguments().at(typeArgTypes.size());
        }

        if (defaultType != nullptr && !defaultType->IsTypeError()) {
            typeArgTypes.emplace_back(defaultType);
        } else {
            ES2PANDA_ASSERT(checker_->IsAnyError());
            typeArgTypes.emplace_back(checker_->GlobalETSObjectType());
        }
    }

    auto pos = (typeArgs == nullptr) ? type->Variable()->Declaration()->Node()->Range().start : typeArgs->Range().start;
    InstantiateType(type, std::move(typeArgTypes), pos);
    ES2PANDA_ASSERT(result_->IsETSObjectType());
    result_->AsETSObjectType()->AddObjectFlag(ETSObjectFlags::NO_OPTS);
}

static void CheckInstantiationConstraints(ETSChecker *checker, ArenaVector<Type *> const &typeParams,
                                          const Substitution *substitution, lexer::SourcePosition pos)
{
    auto relation = checker->Relation();

    for (auto type : typeParams) {
        type = type->MaybeBaseTypeOfGradualType();
        if (!type->IsETSTypeParameter()) {
            continue;
        }
        auto typeParam = type->AsETSTypeParameter();
        auto typeArg = typeParam->Substitute(relation, substitution);
        if (typeArg->IsWildcardType()) {
            continue;
        }
        if (typeArg->IsTypeError()) {
            continue;
        }
        // NOTE(vpukhov): #19701 void refactoring
        ES2PANDA_ASSERT(typeArg->IsETSReferenceType() || typeArg->IsETSVoidType());
        auto maybeIrrelevantTypeArg = typeArg->IsETSVoidType() ? checker->GlobalETSUndefinedType() : typeArg;
        auto constraint = typeParam->GetConstraintType()->Substitute(relation, substitution);
        if (!relation->IsSupertypeOf(constraint, maybeIrrelevantTypeArg)) {
            checker->LogError(diagnostic::TYPEARG_TYPEPARAM_SUBTYPING, {typeArg, constraint}, pos);
        }
    }
}

void ConstraintCheckScope::TryCheckConstraints()
{
    if (Unlock()) {
        auto &records = checker_->PendingConstraintCheckRecords();
        for (auto const &[typeParams, substitution, pos] : records) {
            CheckInstantiationConstraints(checker_, *typeParams, &substitution, pos);
        }
        records.clear();
    }
}

void InstantiationContext::InstantiateType(ETSObjectType *type, ArenaVector<Type *> &&typeArgTypes,
                                           const lexer::SourcePosition &pos)
{
    util::StringView hash = checker_->GetHashFromTypeArguments(typeArgTypes);
    auto const &typeParams = type->TypeArguments();

    while (typeArgTypes.size() < typeParams.size()) {
        typeArgTypes.push_back(typeParams.at(typeArgTypes.size()));
    }

    auto substitution = Substitution {};
    for (size_t idx = 0; idx < typeParams.size(); idx++) {
        if (!typeParams[idx]->IsETSTypeParameter()) {
            continue;
        }
        checker_->EmplaceSubstituted(&substitution, typeParams[idx]->AsETSTypeParameter(), typeArgTypes[idx]);
    }

    ConstraintCheckScope ctScope(checker_);
    result_ = type->Substitute(checker_->Relation(), &substitution)->AsETSObjectType();
    if (!checker_->Relation()->NoThrowGenericTypeAlias()) {
        checker_->PendingConstraintCheckRecords().emplace_back(&typeParams, std::move(substitution), pos);
    }

    type->InsertInstantiationMap(hash, result_->AsETSObjectType());
    result_->AddTypeFlag(TypeFlag::GENERIC);

    ctScope.TryCheckConstraints();
}

}  // namespace ark::es2panda::checker
