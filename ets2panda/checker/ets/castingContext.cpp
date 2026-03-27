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

#include "castingContext.h"
#include "compiler/lowering/checkerPhase.h"

namespace ark::es2panda::checker {
CastingContext::CastingContext(TypeRelation *relation, const diagnostic::DiagnosticKind &diagKind,
                               const util::DiagnosticMessageParams &list, ConstructorData &&data)
{
    flags_ |= data.extraFlags;

    const SavedTypeRelationFlagsContext savedTypeRelationFlags(relation, flags_);
    relation->SetNode(data.node);

    relation->Result(false);
    if (!relation->IsSupertypeOf(data.target, data.source)) {
        if (!relation->IsCastableTo(data.source, data.target)) {
            relation->RaiseError(diagKind, list, data.pos);
        }
    } else {
        trivialCast_ = true;
        if (!data.node->IsArrayExpression() && !data.node->IsObjectExpression() && !data.node->IsLiteral() &&
            compiler::GetPhaseManager()->CurrentPhase()->Name() == compiler::CheckerPhase::NAME) {
            relation->RaiseError(diagnostic::TRIVIAL_CAST, {}, data.pos);
        }
    }

    uncheckedCast_ = relation->UncheckedCast();
    relation->SetNode(nullptr);
}

bool CastingContext::UncheckedCast() const noexcept
{
    return uncheckedCast_;
}

bool CastingContext::TrivialCast() const noexcept
{
    return trivialCast_;
}
}  // namespace ark::es2panda::checker
