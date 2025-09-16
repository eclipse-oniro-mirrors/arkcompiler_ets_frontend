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

#include "classElement.h"

#include "ir/base/methodDefinition.h"
#include "ir/expressions/identifier.h"

namespace ark::es2panda::ir {

void ClassElement::SetOrigEnumMember(TSEnumMember *enumMember)
{
    this->GetOrCreateHistoryNodeAs<ClassElement>()->enumMember_ = enumMember;
}

void ClassElement::SetKey(Expression *key)
{
    this->GetOrCreateHistoryNodeAs<ClassElement>()->key_ = key;
}

void ClassElement::EmplaceDecorators(Decorator *decorators)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ClassElement>();
    newNode->decorators_.emplace_back(decorators);
}

void ClassElement::ClearDecorators()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ClassElement>();
    newNode->decorators_.clear();
}

void ClassElement::SetValueDecorators(Decorator *decorators, size_t index)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ClassElement>();
    auto &arenaVector = newNode->decorators_;
    ES2PANDA_ASSERT(arenaVector.size() > index);
    arenaVector[index] = decorators;
}

[[nodiscard]] const ArenaVector<Decorator *> &ClassElement::Decorators()
{
    auto newNode = this->GetHistoryNodeAs<ClassElement>();
    return newNode->decorators_;
}

[[nodiscard]] ArenaVector<Decorator *> &ClassElement::DecoratorsForUpdate()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ClassElement>();
    return newNode->decorators_;
}

void ClassElement::SetValue(Expression *value) noexcept
{
    if (Value() == value) {
        return;
    }

    if (value != nullptr) {
        value->SetParent(this);
    }
    this->GetOrCreateHistoryNodeAs<ClassElement>()->value_ = value;
}

Identifier *ClassElement::Id() noexcept
{
    auto const key = GetHistoryNode()->AsClassElement()->key_;
    return key != nullptr && key->IsIdentifier() ? key->AsIdentifier() : nullptr;
}

const Identifier *ClassElement::Id() const noexcept
{
    auto const key = GetHistoryNode()->AsClassElement()->key_;
    return key != nullptr && key->IsIdentifier() ? key->AsIdentifier() : nullptr;
}

bool ClassElement::IsPrivateElement() const noexcept
{
    if (IsClassStaticBlock()) {
        return false;
    }

    auto const key = GetHistoryNode()->AsClassElement()->key_;
    return key->IsIdentifier() && key->AsIdentifier()->IsPrivateIdent();
}

void ClassElement::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsClassElement();

    otherImpl->key_ = key_;
    otherImpl->value_ = value_;
    otherImpl->decorators_ = decorators_;
    otherImpl->isComputed_ = isComputed_;
    otherImpl->enumMember_ = enumMember_;

    TypedStatement::CopyTo(other);
}

}  // namespace ark::es2panda::ir
