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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_NULLISH_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_NULLISH_TYPE_H

#include "checker/types/type.h"
#include "ir/astNode.h"

namespace ark::es2panda::checker {

class ETSNullType : public Type {
public:
    ETSNullType() : Type(TypeFlag::ETS_NULL) {}

    void Identical(TypeRelation *relation, Type *other) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    void Compare(TypeRelation *relation, Type *other) override;
    void Cast(TypeRelation *relation, Type *target) override;
    void CastTarget(TypeRelation *relation, Type *source) override;

    void ToString(std::stringstream &ss, bool precise) const override;
    void ToAssemblerType(std::stringstream &ss) const override;
    void ToDebugInfoType([[maybe_unused]] std::stringstream &ss) const override;

    Type *Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes) override;
};

class ETSUndefinedType : public Type {
public:
    ETSUndefinedType() : Type(TypeFlag::ETS_UNDEFINED) {}

    void Identical(TypeRelation *relation, Type *other) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    void Compare(TypeRelation *relation, Type *other) override;
    void Cast(TypeRelation *relation, Type *target) override;
    void CastTarget(TypeRelation *relation, Type *source) override;

    void ToString(std::stringstream &ss, bool precise) const override;
    void ToAssemblerType(std::stringstream &ss) const override;
    void ToDebugInfoType([[maybe_unused]] std::stringstream &ss) const override;

    Type *Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes) override;
};

}  // namespace ark::es2panda::checker

#endif
