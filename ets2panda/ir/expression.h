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

#ifndef ES2PANDA_IR_EXPRESSION_H
#define ES2PANDA_IR_EXPRESSION_H

#include "ir/typed.h"

namespace ark::es2panda::ir {
class Literal;
class TypeNode;
class AnnotatedExpression;

class Expression : public TypedAstNode {
public:
    Expression() = delete;
    ~Expression() override = default;

    Expression &operator=(const Expression &) = delete;
    NO_MOVE_SEMANTIC(Expression);

    [[nodiscard]] bool IsGrouped() const noexcept
    {
        return AstNode::GetHistoryNodeAs<Expression>()->HasAstNodeFlags(AstNodeFlags::IS_GROUPED);
    }

    void SetGrouped() noexcept
    {
        if (!IsGrouped()) {
            AstNode::GetOrCreateHistoryNodeAs<Expression>()->SetAstNodeFlags(AstNodeFlags::IS_GROUPED);
        }
    }

    [[nodiscard]] const Literal *AsLiteral() const
    {
        ES2PANDA_ASSERT(IsLiteral());
        return reinterpret_cast<const Literal *>(GetHistoryNodeAs<Expression>());
    }

    [[nodiscard]] Literal *AsLiteral()
    {
        ES2PANDA_ASSERT(IsLiteral());
        return reinterpret_cast<Literal *>(GetHistoryNodeAs<Expression>());
    }

    [[nodiscard]] virtual bool IsLiteral() const noexcept
    {
        return false;
    }

    [[nodiscard]] virtual bool IsTypeNode() const noexcept
    {
        return false;
    }

    [[nodiscard]] virtual bool IsAnnotatedExpression() const noexcept
    {
        return false;
    }

    [[nodiscard]] bool IsExpression() const noexcept override
    {
        return true;
    }

    [[nodiscard]] TypeNode *AsTypeNode()
    {
        ES2PANDA_ASSERT(IsTypeNode());
        return reinterpret_cast<TypeNode *>(GetHistoryNodeAs<Expression>());
    }

    [[nodiscard]] const TypeNode *AsTypeNode() const
    {
        ES2PANDA_ASSERT(IsTypeNode());
        return reinterpret_cast<const TypeNode *>(GetHistoryNodeAs<Expression>());
    }

    [[nodiscard]] AnnotatedExpression *AsAnnotatedExpression()
    {
        ES2PANDA_ASSERT(IsAnnotatedExpression());
        return reinterpret_cast<AnnotatedExpression *>(GetHistoryNodeAs<Expression>());
    }

    [[nodiscard]] const AnnotatedExpression *AsAnnotatedExpression() const
    {
        ES2PANDA_ASSERT(IsAnnotatedExpression());
        return reinterpret_cast<const AnnotatedExpression *>(GetHistoryNodeAs<Expression>());
    }

    bool IsBrokenExpression() const noexcept;

    [[nodiscard]] virtual std::string ToString() const;

    void CopyTo(AstNode *other) const override;

protected:
    explicit Expression(AstNodeType const type) : TypedAstNode(type) {}
    explicit Expression(AstNodeType const type, ModifierFlags const flags) : TypedAstNode(type, flags) {}

    Expression(Expression const &other) : TypedAstNode(static_cast<TypedAstNode const &>(other)) {}

private:
    friend class SizeOfNodeTest;
};

class AnnotatedExpression : public Annotated<Expression> {
public:
    AnnotatedExpression() = delete;
    ~AnnotatedExpression() override = default;

    NO_COPY_SEMANTIC(AnnotatedExpression);
    NO_MOVE_SEMANTIC(AnnotatedExpression);

    [[nodiscard]] bool IsAnnotatedExpression() const noexcept override
    {
        return true;
    }

protected:
    explicit AnnotatedExpression(AstNodeType const type, TypeNode *const typeAnnotation)
        : Annotated<Expression>(type, typeAnnotation)
    {
    }
    explicit AnnotatedExpression(AstNodeType const type) : Annotated<Expression>(type) {}

    explicit AnnotatedExpression(AnnotatedExpression const &other, ArenaAllocator *allocator);
};

class MaybeOptionalExpression : public Expression {
public:
    MaybeOptionalExpression() = delete;
    ~MaybeOptionalExpression() override = default;

    MaybeOptionalExpression &operator=(const MaybeOptionalExpression &) = delete;
    NO_MOVE_SEMANTIC(MaybeOptionalExpression);

    [[nodiscard]] bool IsOptional() const noexcept
    {
        return GetHistoryNodeAs<MaybeOptionalExpression>()->optional_;
    }

    void ClearOptional() noexcept
    {
        GetOrCreateHistoryNodeAs<MaybeOptionalExpression>()->optional_ = false;
    }

protected:
    explicit MaybeOptionalExpression(AstNodeType type, bool optional) : Expression(type), optional_(optional) {}
    explicit MaybeOptionalExpression(AstNodeType type, ModifierFlags flags, bool optional)
        : Expression(type, flags), optional_(optional)
    {
    }

    MaybeOptionalExpression(MaybeOptionalExpression const &other) : Expression(static_cast<Expression const &>(other))
    {
        optional_ = other.optional_;
    }

private:
    bool optional_;
};

}  // namespace ark::es2panda::ir

#endif /* ES2PANDA_IR_EXPRESSION_H */
