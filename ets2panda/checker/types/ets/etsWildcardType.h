/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_WILDCARD_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_WILDCARD_TYPE_H

#include "checker/types/type.h"

namespace ark::es2panda::checker {

class ETSWildcardType : public Type {
public:
    ETSWildcardType() = delete;
    ~ETSWildcardType() override = default;

    NO_COPY_SEMANTIC(ETSWildcardType);
    NO_MOVE_SEMANTIC(ETSWildcardType);

    explicit ETSWildcardType(ETSTypeParameter *tparam) : Type(TypeFlag::ETS_WILDCARD), tparam_(tparam) {}

    ETSTypeParameter *GetUnderlying() const
    {
        return tparam_;
    }

    void ToString(std::stringstream &ss, bool precise) const override;
    void Identical(TypeRelation *relation, Type *other) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    Type *Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes) override;
    void Cast(TypeRelation *relation, Type *target) override;

    void ToAssemblerType(std::stringstream &ss) const override
    {
        ss << "wildcard";
    }

private:
    ETSTypeParameter *const tparam_;
};

}  // namespace ark::es2panda::checker

#endif
