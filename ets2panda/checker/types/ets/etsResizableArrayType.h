/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_RESIZABLE_ARRAY_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_RESIZABLE_ARRAY_TYPE_H

#include "checker/types/ets/etsObjectType.h"
#include "checker/types/ets/etsObjectTypeConstants.h"

namespace ark::es2panda::checker {

class ETSResizableArrayType : public ETSObjectType {
public:
    explicit ETSResizableArrayType(ArenaAllocator *allocator, ETSObjectType *super)
        : ETSObjectType(allocator, "", compiler::Signatures::BUILTIN_ARRAY, nullptr,
                        ETSObjectFlags::CLASS | ETSObjectFlags::BUILTIN_ARRAY | ETSObjectFlags::RESOLVED_SUPER),
          element_(nullptr)
    {
        SetSuperType(super);
    }

    explicit ETSResizableArrayType(ArenaAllocator *allocator, util::StringView name,
                                   std::tuple<ir::AstNode *, ETSObjectFlags, TypeRelation *> info)
        : ETSObjectType(allocator, name, compiler::Signatures::BUILTIN_ARRAY, info), element_(nullptr)
    {
    }

    explicit ETSResizableArrayType(ArenaAllocator *allocator, ETSObjectType *super, TypeRelation *relation,
                                   Type *element)
        : ETSObjectType(
              allocator, "", compiler::Signatures::BUILTIN_ARRAY,
              std::make_tuple(nullptr,
                              ETSObjectFlags::CLASS | ETSObjectFlags::BUILTIN_ARRAY | ETSObjectFlags::RESOLVED_SUPER,
                              relation)),
          element_(element)
    {
        SetSuperType(super);
        variable_ = super->Variable();
    }

    NO_COPY_SEMANTIC(ETSResizableArrayType);
    NO_MOVE_SEMANTIC(ETSResizableArrayType);

    ETSResizableArrayType() = delete;
    ~ETSResizableArrayType() override = default;

    Type *ElementType()
    {
        if (element_ == nullptr) {
            element_ = TypeArguments()[0];
        }
        return element_;
    }

    const Type *ElementType() const
    {
        return TypeArguments()[0];
    }

    void SetElementType(Type *element)
    {
        element_ = element;
    }

    ETSResizableArrayType *Substitute(TypeRelation *relation, const Substitution *substitution) override;

    void ToString(std::stringstream &ss, [[maybe_unused]] bool precise) const override;

private:
    Type *element_;
};

}  // namespace ark::es2panda::checker

#endif
