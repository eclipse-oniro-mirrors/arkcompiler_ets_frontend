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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_BOOLEAN_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_BOOLEAN_TYPE_H

#include "checker/types/type.h"

namespace ark::es2panda::checker {
class ETSBooleanType : public Type {
public:
    using UType = bool;
    ETSBooleanType() : Type(TypeFlag::ETS_BOOLEAN) {}
    explicit ETSBooleanType(UType value) : Type(TypeFlag::ETS_BOOLEAN | TypeFlag::CONSTANT), value_(value) {}

    void Identical(TypeRelation *relation, Type *other) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    bool AssignmentSource([[maybe_unused]] TypeRelation *relation, [[maybe_unused]] Type *target) override;
    void Cast(TypeRelation *relation, Type *target) override;
    Type *Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes) override;

    UType GetValue() const
    {
        return value_;
    }

    void ToString(std::stringstream &ss, [[maybe_unused]] bool precise) const override
    {
        ss << "boolean";
    }

    void ToAssemblerType([[maybe_unused]] std::stringstream &ss) const override
    {
        ss << compiler::Signatures::PRIMITIVE_BOOLEAN;
    }

    void ToDebugInfoType(std::stringstream &ss) const override
    {
        ss << compiler::Signatures::TYPE_DESCRIPTOR_BOOLEAN;
    }

private:
    UType value_ {false};
};
}  // namespace ark::es2panda::checker

#endif
