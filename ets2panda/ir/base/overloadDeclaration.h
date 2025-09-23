/**
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

#ifndef ES2PANDA_IR_OVERLOAD_DEFINITION_H
#define ES2PANDA_IR_OVERLOAD_DEFINITION_H

#include "classDefinition.h"
#include "ir/typeNode.h"
#include "methodDefinition.h"

namespace ark::es2panda::ir {

using ENUMBITOPS_OPERATORS;

enum class OverloadDeclFlags : std::uint8_t {
    NONE = 0U,
    FUNCTION = 1U << 0U,          // Function Overload Declaration
    CLASS_METHOD = 1U << 1U,      // Class Method Overload Declaration
    INTERFACE_METHOD = 1U << 2U,  // Interface Method Overload Declaration
    // CONSTURCTOR = 1U << 3U,  // Constructor Overload Declarations
};
}  // namespace ark::es2panda::ir

namespace enumbitops {

template <>
struct IsAllowedType<ark::es2panda::ir::OverloadDeclFlags> : std::true_type {
};

}  // namespace enumbitops

namespace ark::es2panda::ir {
class OverloadDeclaration : public ClassElement {
public:
    OverloadDeclaration() = delete;
    ~OverloadDeclaration() override = default;

    NO_COPY_SEMANTIC(OverloadDeclaration);
    NO_MOVE_SEMANTIC(OverloadDeclaration);

    explicit OverloadDeclaration(Expression *const key, ModifierFlags const modifiers,
                                 [[maybe_unused]] ArenaAllocator *const allocator)
        : ClassElement(AstNodeType::OVERLOAD_DECLARATION, key, nullptr, modifiers, allocator, false),
          overloadedList_(allocator->Adapter()),
          overloadFlags_(OverloadDeclFlags::NONE)
    {
        InitHistory();
    }

    OverloadDeclFlags Flag() const
    {
        return GetHistoryNodeAs<OverloadDeclaration>()->overloadFlags_;
    }

    [[nodiscard]] ArenaVector<ir::Expression *> &OverloadedList() noexcept
    {
        return GetOrCreateHistoryNodeAs<OverloadDeclaration>()->overloadedList_;
    }

    [[nodiscard]] const ArenaVector<ir::Expression *> &OverloadedList() const noexcept
    {
        return GetOrCreateHistoryNodeAs<OverloadDeclaration>()->overloadedList_;
    }

    void SetOverloadedList(ArenaVector<Expression *> overloadedList)
    {
        auto newNode = GetOrCreateHistoryNodeAs<OverloadDeclaration>();
        newNode->overloadedList_ = std::move(overloadedList);
    }

    void PushFront(Identifier *const overloadedExpression)
    {
        auto newNode = GetOrCreateHistoryNodeAs<OverloadDeclaration>();
        newNode->overloadedList_.insert(newNode->overloadedList_.begin(), overloadedExpression);
    }

    void AddOverloadDeclFlag(OverloadDeclFlags overloadFlag)
    {
        auto newNode = GetOrCreateHistoryNodeAs<OverloadDeclaration>();
        newNode->overloadFlags_ |= overloadFlag;
    }

    [[nodiscard]] bool HasOverloadDeclFlag(OverloadDeclFlags overloadFlag) const
    {
        return (Flag() & overloadFlag) != 0;
    }

    [[nodiscard]] bool IsConstructorOverloadDeclaration()
    {
        return IsConstructor();
    }

    [[nodiscard]] bool IsFunctionOverloadDeclaration()
    {
        return (Flag() & OverloadDeclFlags::FUNCTION) != 0;
    }

    [[nodiscard]] bool IsClassMethodOverloadDeclaration()
    {
        return (Flag() & OverloadDeclFlags::CLASS_METHOD) != 0;
    }

    [[nodiscard]] bool IsInterfaceMethodOverloadDeclaration()
    {
        return (Flag() & OverloadDeclFlags::INTERFACE_METHOD) != 0;
    }

    void ResolveReferences(const NodeTraverser &cb) const;

    PrivateFieldKind ToPrivateFieldKind(bool isStatic) const override;
    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void DumpModifier(ir::SrcDumper *dumper) const;
    void Compile([[maybe_unused]] compiler::PandaGen *pg) const override;
    checker::Type *Check([[maybe_unused]] checker::TSChecker *checker) override;
    checker::VerifiedType Check([[maybe_unused]] checker::ETSChecker *checker) override;
    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

protected:
    OverloadDeclaration *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

private:
    ArenaVector<Expression *> overloadedList_;
    OverloadDeclFlags overloadFlags_;
};
}  // namespace ark::es2panda::ir

#endif
