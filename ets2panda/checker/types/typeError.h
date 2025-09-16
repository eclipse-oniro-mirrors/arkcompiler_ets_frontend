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

#ifndef ES2PANDA_CHECKER_TYPES_ERROR_TYPE_ERROR_H
#define ES2PANDA_CHECKER_TYPES_ERROR_TYPE_ERROR_H

#include "checker/types/type.h"
#include "es2panda.h"

namespace ark::es2panda::checker {

class TypeError : public Type {
public:
    explicit TypeError() : Type(TypeFlag::TYPE_ERROR) {}

    bool AssignmentSource(TypeRelation *relation, [[maybe_unused]] Type *target) override
    {
        // Don't spread the error
        relation->Result(RelationResult::TRUE);
        return true;
    }

    void AssignmentTarget(TypeRelation *relation, [[maybe_unused]] Type *source) override
    {
        // Don't spread the error
        relation->Result(RelationResult::TRUE);
    }

    void ToString(std::stringstream &ss, [[maybe_unused]] bool precise) const override
    {
        ss << ERROR_TYPE;
    }

    Type *Instantiate([[maybe_unused]] ArenaAllocator *allocator, [[maybe_unused]] TypeRelation *relation,
                      [[maybe_unused]] GlobalTypesHolder *globalTypes) override
    {
        return this;
    }
};

}  // namespace ark::es2panda::checker

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define EMPTY_VALUE

// CC-OFFNXT(G.PRE.02-CPP) TypeError handling macro definition
#define ERROR_SANITY_CHECK(etsChecker, test, whatIfFails) \
    if (!(test)) {                                        \
        ES2PANDA_ASSERT((etsChecker)->IsAnyError());      \
        whatIfFails;                                      \
    }

// CC-OFFNXT(G.PRE.02-CPP) TypeError handling macro definition
#define ERROR_TYPE_CHECK(etsChecker, testType, whatIfError) \
    ES2PANDA_ASSERT((testType) != nullptr);                 \
    if ((testType)->IsTypeError()) {                        \
        ES2PANDA_ASSERT((etsChecker)->IsAnyError());        \
        whatIfError;                                        \
    }

// CC-OFFNXT(G.PRE.02-CPP) TypeError handling macro definition
#define FORWARD_TYPE_ERROR(etsChecker, testType, target)             \
    ES2PANDA_ASSERT((testType) != nullptr);                          \
    if ((testType)->IsTypeError()) {                                 \
        ES2PANDA_ASSERT((etsChecker)->IsAnyError());                 \
        /* CC-OFFNXT(G.PRE.05) error handling. */                    \
        return (target)->SetTsType((etsChecker)->GlobalTypeError()); \
    }

// CC-OFFNXT(G.PRE.02-CPP) TypeError handling macro definition
#define FORWARD_VALUE_ON_TYPE_ERROR(etsChecker, testType, target, value) \
    ES2PANDA_ASSERT((testType) != nullptr);                              \
    if ((testType)->IsTypeError()) {                                     \
        ES2PANDA_ASSERT((etsChecker)->IsAnyError());                     \
        (target)->SetTsType((etsChecker)->GlobalTypeError());            \
        /* CC-OFFNXT(G.PRE.05) error handling. */                        \
        return value;                                                    \
    }
// NOLINTEND(cppcoreguidelines-macro-usage)

#endif
