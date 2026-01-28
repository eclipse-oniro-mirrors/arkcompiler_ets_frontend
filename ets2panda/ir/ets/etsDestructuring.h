/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_IR_ETS_DESTRUCTURING_H
#define ES2PANDA_IR_ETS_DESTRUCTURING_H

#include <utility>

#include "ir/expression.h"

namespace ark::es2panda::checker {
class ETSAnalyzer;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::ir {

class ETSDestructuring : public AnnotatedExpression {
public:
    explicit ETSDestructuring(ArenaAllocator *allocator) noexcept
        : AnnotatedExpression(AstNodeType::ETS_DESTRUCTURING), elements_(allocator->Adapter())
    {
        InitHistory();
    }

    explicit ETSDestructuring(ArenaVector<Expression *> elements) noexcept
        : AnnotatedExpression(AstNodeType::ETS_DESTRUCTURING), elements_(std::move(elements))
    {
        // Strip OmittedExpressions from the end
        while (!elements_.empty() && elements_.back()->IsOmittedExpression()) {
            elements_.pop_back();
        }
        InitHistory();
    }

    Expression *GetExpressionAtIndex(std::size_t index)
    {
        ES2PANDA_ASSERT(index < Size());
        return elements_.at(index);
    }

    ArenaVector<Expression *> Elements()
    {
        return elements_;
    }

    ArenaVector<Expression *> Elements() const
    {
        return elements_;
    }

    std::size_t Size() const
    {
        return elements_.size();
    }

    void SetValueTypes(Expression *expr, size_t index) const
    {
        GetOrCreateHistoryNodeAs<ETSDestructuring>()->elements_[index] = expr;
    }

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile(compiler::PandaGen *pg) const override;
    void Compile(compiler::ETSGen *etsg) const override;
    checker::Type *Check(checker::TSChecker *checker) override;
    checker::VerifiedType Check(checker::ETSChecker *checker) override;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    [[nodiscard]] ETSDestructuring *Clone(ArenaAllocator *allocator, AstNode *parent) override;

protected:
    ETSDestructuring *Construct(ArenaAllocator *allocator) override
    {
        ArenaVector<Expression *> elements(allocator->Adapter());
        return allocator->New<ETSDestructuring>(std::move(elements));
    }

    void CopyTo(AstNode *other) const override
    {
        auto otherImpl = other->AsETSDestructuring();
        otherImpl->elements_ = elements_;
        AnnotatedExpression::CopyTo(other);
    }

private:
    ArenaVector<Expression *> elements_;
};
}  // namespace ark::es2panda::ir

#endif
