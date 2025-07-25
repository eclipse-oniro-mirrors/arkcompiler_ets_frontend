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

#ifndef ES2PANDA_COMPILER_CHECKER_ETS_CASTING_CONTEXT_H
#define ES2PANDA_COMPILER_CHECKER_ETS_CASTING_CONTEXT_H

#include "checker/types/typeRelation.h"

namespace ark::es2panda::checker {
class CastingContext {
public:
    // NOLINTBEGIN(cppcoreguidelines-pro-type-member-init)
    struct ConstructorData {
        ir::Expression *node;
        checker::Type *source;
        checker::Type *target;
        lexer::SourcePosition const &pos;
        TypeRelationFlag extraFlags = TypeRelationFlag::NONE;
    };
    // NOLINTEND(cppcoreguidelines-pro-type-member-init)

    CastingContext(TypeRelation *relation, const diagnostic::DiagnosticKind &diagKind,
                   const util::DiagnosticMessageParams &list, ConstructorData &&data);

    [[nodiscard]] bool UncheckedCast() const noexcept;

private:
    TypeRelationFlag flags_ {TypeRelationFlag::CASTING_CONTEXT | TypeRelationFlag::IN_CASTING_CONTEXT};
    bool uncheckedCast_ {true};
};

}  // namespace ark::es2panda::checker

#endif
