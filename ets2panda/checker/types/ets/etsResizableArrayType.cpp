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

#include "etsResizableArrayType.h"

namespace ark::es2panda::checker {

ETSResizableArrayType *ETSResizableArrayType::Substitute(TypeRelation *relation, const Substitution *substitution)
{
    auto copiedType = ETSObjectType::Substitute(relation, substitution)->AsETSResizableArrayType();
    element_ = TypeArguments()[0];
    return copiedType;
}

void ETSResizableArrayType::ToString(std::stringstream &ss, [[maybe_unused]] bool precise) const
{
    if (ElementType() != nullptr) {
        if (HasTypeFlag(TypeFlag::READONLY)) {
            ss << "readonly ";
        }
        ss << "Array<";
        ElementType()->ToString(ss, precise);
        ss << ">";
    }
}

void ETSResizableArrayType::Iterate(const TypeTraverser &func) const
{
    func(element_);
}

}  // namespace ark::es2panda::checker
