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

#ifndef ES2PANDA_IR_EXPRESSION_IDENTIFIER_H
#define ES2PANDA_IR_EXPRESSION_IDENTIFIER_H

#include "ir/expression.h"
#include "ir/validationInfo.h"
#include <varbinder/varbinder.h>

namespace ark::es2panda::varbinder {
class Variable;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::ir {

using ENUMBITOPS_OPERATORS;

enum class IdentifierFlags : uint32_t {
    NONE = 0U,
    OPTIONAL = 1U << 0U,
    TDZ = 1U << 1U,
    PRIVATE = 1U << 2U,
    GET = 1U << 3U,
    SET = 1U << 4U,
    IGNORE_BOX = 1U << 5U,
    ANNOTATIONDECL = 1U << 6U,
    ANNOTATIONUSAGE = 1U << 7U,
    ERROR_PLACEHOLDER = 1U << 8U,
};

}  // namespace ark::es2panda::ir

template <>
struct enumbitops::IsAllowedType<ark::es2panda::ir::IdentifierFlags> : std::true_type {
};

namespace ark::es2panda::ir {

class Identifier : public AnnotatedExpression {
private:
    struct Tag {};

public:
    Identifier() = delete;
    ~Identifier() override = default;

    NO_COPY_SEMANTIC(Identifier);
    NO_MOVE_SEMANTIC(Identifier);

public:
    explicit Identifier(ArenaAllocator *const allocator);
    explicit Identifier(util::StringView const name, ArenaAllocator *const allocator);
    explicit Identifier(util::StringView const name, TypeNode *const typeAnnotation, ArenaAllocator *const allocator);
    explicit Identifier(Tag tag, Identifier const &other, ArenaAllocator *allocator);

    [[nodiscard]] const util::StringView &Name() const noexcept
    {
        return GetHistoryNodeAs<Identifier>()->name_;
    }

    [[nodiscard]] util::StringView &Name() noexcept
    {
        return GetHistoryNodeAs<Identifier>()->name_;
    }

    void SetName(const util::StringView &newName) noexcept;

    bool IsErrorPlaceHolder() const noexcept
    {
        return (IdFlags() & IdentifierFlags::ERROR_PLACEHOLDER) != 0;
    }

    [[nodiscard]] bool IsOptional() const noexcept
    {
        return (IdFlags() & IdentifierFlags::OPTIONAL) != 0;
    }

    void SetOptional(bool const optional) noexcept
    {
        if (optional) {
            AddIdFlags(IdentifierFlags::OPTIONAL);
        } else {
            ClearIdFlags(IdentifierFlags::OPTIONAL);
        }
    }

    [[nodiscard]] bool IsReference(ScriptExtension ext) const noexcept
    {
        return !IsDeclaration(ext);
    }

    [[nodiscard]] bool IsTdz() const noexcept
    {
        return (IdFlags() & IdentifierFlags::TDZ) != 0;
    }

    void SetTdz() noexcept
    {
        AddIdFlags(IdentifierFlags::TDZ);
    }

    void SetAccessor() noexcept
    {
        AddIdFlags(IdentifierFlags::GET);
    }

    [[nodiscard]] bool IsAccessor() const noexcept
    {
        return (IdFlags() & IdentifierFlags::GET) != 0;
    }

    void SetMutator() noexcept
    {
        AddIdFlags(IdentifierFlags::SET);
    }

    [[nodiscard]] bool IsMutator() const noexcept
    {
        return (IdFlags() & IdentifierFlags::SET) != 0;
    }

    [[nodiscard]] bool IsReceiver() const noexcept
    {
        return Name() == varbinder::VarBinder::MANDATORY_PARAM_THIS;
    }

    [[nodiscard]] bool IsPrivateIdent() const noexcept
    {
        return (IdFlags() & IdentifierFlags::PRIVATE) != 0;
    }

    void SetPrivate(bool const isPrivate) noexcept
    {
        if (isPrivate) {
            AddIdFlags(IdentifierFlags::PRIVATE);
        } else {
            ClearIdFlags(IdentifierFlags::PRIVATE);
        }
    }

    [[nodiscard]] bool IsIgnoreBox() const noexcept
    {
        return (IdFlags() & IdentifierFlags::IGNORE_BOX) != 0;
    }

    void SetIgnoreBox() noexcept
    {
        AddIdFlags(IdentifierFlags::IGNORE_BOX);
    }

    [[nodiscard]] bool IsAnnotationDecl() const noexcept
    {
        return (IdFlags() & IdentifierFlags::ANNOTATIONDECL) != 0;
    }

    void SetAnnotationDecl() noexcept
    {
        AddIdFlags(IdentifierFlags::ANNOTATIONDECL);
    }

    [[nodiscard]] bool IsAnnotationUsage() const noexcept
    {
        return (IdFlags() & IdentifierFlags::ANNOTATIONUSAGE) != 0;
    }

    void SetAnnotationUsage() noexcept
    {
        AddIdFlags(IdentifierFlags::ANNOTATIONUSAGE);
    }

    [[nodiscard]] Identifier *Clone(ArenaAllocator *allocator, AstNode *parent) override;
    [[nodiscard]] Identifier *CloneReference(ArenaAllocator *allocator, AstNode *parent);

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile(compiler::PandaGen *pg) const override;
    void Compile(compiler::ETSGen *etsg) const override;
    checker::Type *Check(checker::TSChecker *checker) override;
    checker::VerifiedType Check(checker::ETSChecker *checker) override;

    std::string ToString() const override;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    Identifier *Construct(ArenaAllocator *allocator) override
    {
        return allocator->New<Identifier>(allocator);
    }

    void CopyTo(AstNode *other) const override
    {
        auto otherImpl = other->AsIdentifier();

        otherImpl->name_ = name_;
        otherImpl->flags_ = flags_;

        AnnotatedExpression::CopyTo(other);
    };

    IdentifierFlags IdFlags() const
    {
        return GetHistoryNodeAs<Identifier>()->flags_;
    }

private:
    void AddIdFlags(IdentifierFlags const flags) noexcept
    {
        if (!All(IdFlags(), flags)) {
            GetOrCreateHistoryNodeAs<Identifier>()->flags_ |= flags;
        }
    }

    void ClearIdFlags(IdentifierFlags const flags) noexcept
    {
        if (Any(IdFlags(), flags)) {
            GetOrCreateHistoryNodeAs<Identifier>()->flags_ &= ~flags;
        }
    }

    bool CheckDeclarationsPart2(const ir::AstNode *parent, ScriptExtension ext) const;
    bool CheckDeclarationsPart1(const ir::AstNode *parent, ScriptExtension ext) const;
    bool CheckNotDeclarations(const ir::AstNode *parent, ScriptExtension ext) const;
    bool CheckDefinitions(const ir::AstNode *parent, ScriptExtension ext) const;
    bool IsDeclaration(ScriptExtension ext) const;

    util::StringView name_;
    IdentifierFlags flags_ {IdentifierFlags::NONE};
};
}  // namespace ark::es2panda::ir

#endif
