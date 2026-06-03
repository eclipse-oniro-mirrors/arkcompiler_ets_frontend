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

#ifndef ES2PANDA_PARSER_INCLUDE_AST_SPREAD_ELEMENT_H
#define ES2PANDA_PARSER_INCLUDE_AST_SPREAD_ELEMENT_H

#include "ir/expression.h"
#include "ir/validationInfo.h"

namespace ark::es2panda::checker {
class Type;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::ir {
class SpreadElement : public AnnotatedExpression {
private:
    struct Tag {};

public:
    enum class ResolvedSpreadKind { INVALID, INDEXABLE, TUPLE, ITERABLE };

    SpreadElement() = delete;
    ~SpreadElement() override = default;

    NO_COPY_SEMANTIC(SpreadElement);
    NO_MOVE_SEMANTIC(SpreadElement);

    explicit SpreadElement(AstNodeType const nodeType, [[maybe_unused]] ArenaAllocator *const allocator,
                           Expression *const argument)
        : AnnotatedExpression(nodeType), argument_(argument)
    {
        ES2PANDA_ASSERT(argument_ != nullptr);
    }

    explicit SpreadElement(Tag tag, SpreadElement const &other, ArenaAllocator *allocator);

    [[nodiscard]] const Expression *Argument() const noexcept
    {
        return argument_;
    }

    [[nodiscard]] Expression *Argument() noexcept
    {
        return argument_;
    }

    [[nodiscard]] bool IsOptional() const noexcept
    {
        return optional_;
    }

    void SetOptional(bool optional) noexcept
    {
        optional_ = optional;
    }

    void SetResolvedSpread(ResolvedSpreadKind kind, checker::Type *sourceType, checker::Type *elementType) noexcept
    {
        resolvedSpreadKind_ = kind;
        resolvedSpreadSourceType_ = sourceType;
        resolvedSpreadElementType_ = elementType;
    }

    [[nodiscard]] ResolvedSpreadKind GetResolvedSpreadKind() const noexcept
    {
        return resolvedSpreadKind_;
    }

    [[nodiscard]] checker::Type *GetResolvedSpreadElementType() const noexcept
    {
        return resolvedSpreadElementType_;
    }

    [[nodiscard]] checker::Type *GetResolvedSpreadSourceType() const noexcept
    {
        return resolvedSpreadSourceType_;
    }

    [[nodiscard]] SpreadElement *Clone(ArenaAllocator *allocator, AstNode *parent) override;

    [[nodiscard]] bool ConvertibleToRest(bool isDeclaration, bool allowPattern = true);

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile([[maybe_unused]] compiler::PandaGen *pg) const override;
    void Compile([[maybe_unused]] compiler::ETSGen *etsg) const override;
    checker::Type *Check([[maybe_unused]] checker::TSChecker *checker) override;
    checker::VerifiedType Check([[maybe_unused]] checker::ETSChecker *checker) override;

    std::string ToString() const override;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

private:
    Expression *argument_ = nullptr;
    bool optional_ {false};
    ResolvedSpreadKind resolvedSpreadKind_ {ResolvedSpreadKind::INVALID};
    checker::Type *resolvedSpreadSourceType_ {nullptr};
    checker::Type *resolvedSpreadElementType_ {nullptr};
};
}  // namespace ark::es2panda::ir

#endif
