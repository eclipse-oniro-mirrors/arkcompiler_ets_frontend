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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_BIGINT_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_BIGINT_TYPE_H

#include "checker/types/ets/etsObjectType.h"

namespace ark::es2panda::checker {
class ETSBigIntType : public ETSObjectType {
public:
    explicit ETSBigIntType(ArenaAllocator *allocator, [[maybe_unused]] ETSObjectType *super)
        : ETSObjectType(allocator, "", compiler::Signatures::BUILTIN_BIGINT, nullptr,
                        ETSObjectFlags::CLASS | ETSObjectFlags::BUILTIN_BIGINT | ETSObjectFlags::RESOLVED_SUPER)
    {
        SetSuperType(super);
    }

    explicit ETSBigIntType(ArenaAllocator *allocator, ETSObjectType *super, TypeRelation *relation,
                           util::StringView value)
        : ETSObjectType(
              allocator, "", compiler::Signatures::BUILTIN_BIGINT,
              std::make_tuple(nullptr,
                              ETSObjectFlags::CLASS | ETSObjectFlags::BUILTIN_BIGINT | ETSObjectFlags::RESOLVED_SUPER,
                              relation)),
          value_(value)
    {
        SetSuperType(super);
        AddTypeFlag(TypeFlag::CONSTANT);
        AddTypeFlag(TypeFlag::BIGINT_LITERAL);
        variable_ = super->Variable();
    }

    void Identical(TypeRelation *relation, Type *other) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    Type *Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes) override;

    void ToString(std::stringstream &ss, [[maybe_unused]] bool precise) const override
    {
        ss << lexer::TokenToString(lexer::TokenType::KEYW_BIGINT);
    }

    void ToAssemblerType([[maybe_unused]] std::stringstream &ss) const override
    {
        ss << compiler::Signatures::BUILTIN_BIGINT;
    }

    util::StringView GetValue() const
    {
        return value_;
    }

private:
    util::StringView value_ {};
};
}  // namespace ark::es2panda::checker

#endif
