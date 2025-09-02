/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_IR_TYPED_H
#define ES2PANDA_IR_TYPED_H

#include "ir/astNode.h"
#include "ir/statement.h"

namespace ark::es2panda::checker {
// NOLINTBEGIN(readability-redundant-declaration)
bool IsTypeError(Type const *tp);
// NOLINTEND(readability-redundant-declaration)
}  // namespace ark::es2panda::checker

namespace ark::es2panda::ir {

template <typename T>
class Typed : public T {
public:
    Typed() = delete;
    ~Typed() override = default;

    NO_COPY_OPERATOR(Typed);
    NO_MOVE_SEMANTIC(Typed);

    [[nodiscard]] checker::Type const *TsType() const
    {
        return AstNode::GetHistoryNodeAs<Typed<T>>()->tsType_;
    }

    [[nodiscard]] checker::Type *TsType()
    {
        return AstNode::GetHistoryNodeAs<Typed<T>>()->tsType_;
    }

    checker::Type *SetTsType(checker::Type *tsType) noexcept
    {
        auto nowNode = AstNode::GetHistoryNodeAs<Typed<T>>();
        if (nowNode->tsType_ != tsType) {
            AstNode::GetOrCreateHistoryNodeAs<Typed<T>>()->tsType_ = tsType;
        }
        return tsType;
    }

    [[nodiscard]] checker::Type *PreferredType() const noexcept
    {
        return preferredType_;
    }

    checker::Type *SetPreferredType(checker::Type *type) noexcept
    {
        return (preferredType_ = type);
    }

    bool IsTyped() const override
    {
        return true;
    }

protected:
    explicit Typed(AstNodeType const type) : T(type) {}
    explicit Typed(AstNodeType const type, ModifierFlags const flags) : T(type, flags) {}

    // NOTE: when cloning node its type is not copied but removed empty so that it can be re-checked further.
    Typed(Typed const &other) : T(static_cast<T const &>(other)) {}

    void CopyTo(AstNode *other) const override
    {
        auto otherImpl = static_cast<Typed<T> *>(other);
        otherImpl->tsType_ = tsType_;
        otherImpl->preferredType_ = preferredType_;
        T::CopyTo(other);
    }

private:
    friend class SizeOfNodeTest;
    checker::Type *tsType_ {};
    checker::Type *preferredType_ {};  // used by the checker to supply information from context
};

class TypedAstNode : public Typed<AstNode> {
public:
    TypedAstNode() = delete;
    ~TypedAstNode() override = default;

    NO_COPY_OPERATOR(TypedAstNode);
    NO_MOVE_SEMANTIC(TypedAstNode);

    void CopyTo(AstNode *other) const override
    {
        Typed<AstNode>::CopyTo(other);
    };

protected:
    explicit TypedAstNode(AstNodeType const type) : Typed<AstNode>(type) {}
    explicit TypedAstNode(AstNodeType const type, ModifierFlags const flags) : Typed<AstNode>(type, flags) {}

    TypedAstNode(TypedAstNode const &other) : Typed<AstNode>(static_cast<Typed<AstNode> const &>(other)) {}
};

class AnnotatedAstNode : public Annotated<AstNode> {
public:
    AnnotatedAstNode() = delete;
    ~AnnotatedAstNode() override = default;

    NO_COPY_OPERATOR(AnnotatedAstNode);
    NO_MOVE_SEMANTIC(AnnotatedAstNode);

protected:
    explicit AnnotatedAstNode(AstNodeType const type, TypeNode *const typeAnnotation)
        : Annotated<AstNode>(type, typeAnnotation)
    {
    }
    explicit AnnotatedAstNode(AstNodeType const type) : Annotated<AstNode>(type) {}
    explicit AnnotatedAstNode(AstNodeType const type, ModifierFlags const flags) : Annotated<AstNode>(type, flags) {}

    AnnotatedAstNode(AnnotatedAstNode const &other) : Annotated<AstNode>(static_cast<Annotated<AstNode> const &>(other))
    {
    }
};

class TypedStatement : public Typed<Statement> {
public:
    TypedStatement() = delete;
    ~TypedStatement() override = default;

    NO_COPY_OPERATOR(TypedStatement);
    NO_MOVE_SEMANTIC(TypedStatement);

    void CopyTo(AstNode *other) const override
    {
        Typed<Statement>::CopyTo(other);
    };

protected:
    explicit TypedStatement(AstNodeType type) : Typed<Statement>(type) {};
    explicit TypedStatement(AstNodeType type, ModifierFlags flags) : Typed<Statement>(type, flags) {};

    TypedStatement(TypedStatement const &other) : Typed<Statement>(static_cast<Typed<Statement> const &>(other)) {}

    inline static checker::Type *const CHECKED = reinterpret_cast<checker::Type *>(0x01);
};

class AnnotatedStatement : public Annotated<Statement> {
public:
    AnnotatedStatement() = delete;
    ~AnnotatedStatement() override = default;

    NO_COPY_OPERATOR(AnnotatedStatement);
    NO_MOVE_SEMANTIC(AnnotatedStatement);

    void CopyTo(AstNode *other) const override
    {
        Annotated<Statement>::CopyTo(other);
    }

protected:
    explicit AnnotatedStatement(AstNodeType type, TypeNode *typeAnnotation) : Annotated<Statement>(type, typeAnnotation)
    {
    }

    explicit AnnotatedStatement(AstNodeType type) : Annotated<Statement>(type) {}
    explicit AnnotatedStatement(AstNodeType type, ModifierFlags flags) : Annotated<Statement>(type, flags) {}

    AnnotatedStatement(AnnotatedStatement const &other)
        : Annotated<Statement>(static_cast<Annotated<Statement> const &>(other))
    {
    }
};

}  // namespace ark::es2panda::ir

#endif
