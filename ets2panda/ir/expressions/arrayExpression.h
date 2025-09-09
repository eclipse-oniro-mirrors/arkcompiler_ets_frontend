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

#ifndef ES2PANDA_IR_EXPRESSION_ARRAY_EXPRESSION_H
#define ES2PANDA_IR_EXPRESSION_ARRAY_EXPRESSION_H

#include "ir/expression.h"
#include "ir/validationInfo.h"
#include "checker/types/ets/etsArrayType.h"

namespace ark::es2panda::checker {
class ETSAnalyzer;
}  // namespace ark::es2panda::checker
namespace ark::es2panda::compiler {
class ETSCompiler;
}  // namespace ark::es2panda::compiler
namespace ark::es2panda::ir {
class ArrayExpression : public AnnotatedExpression {
private:
    struct Tag {};

public:
    ArrayExpression() = delete;
    ~ArrayExpression() override = default;

    NO_COPY_SEMANTIC(ArrayExpression);
    NO_MOVE_SEMANTIC(ArrayExpression);

public:
    explicit ArrayExpression(ArenaVector<Expression *> &&elements, ArenaAllocator *const allocator)
        : ArrayExpression(AstNodeType::ARRAY_EXPRESSION, std::move(elements), allocator, false)
    {
    }

    explicit ArrayExpression(AstNodeType nodeType, ArenaVector<Expression *> &&elements,
                             [[maybe_unused]] ArenaAllocator *const allocator, bool const trailingComma)
        : AnnotatedExpression(nodeType), elements_(std::move(elements)), trailingComma_(trailingComma)
    {
    }

    explicit ArrayExpression(Tag tag, ArrayExpression const &other, ArenaAllocator *allocator);

    // NOTE (vivienvoros): these friend relationships can be removed once there are getters for private fields
    friend class checker::ETSAnalyzer;
    friend class compiler::ETSCompiler;

    [[nodiscard]] Expression *GetElementNodeAtIdx(const size_t idx) const noexcept
    {
        return elements_[idx];
    }

    [[nodiscard]] const ArenaVector<Expression *> &Elements() const noexcept
    {
        return elements_;
    }

    [[nodiscard]] ArenaVector<Expression *> &Elements() noexcept
    {
        return elements_;
    }

    void SetElements(ArenaVector<Expression *> &&elements) noexcept
    {
        elements_ = std::move(elements);
    }

    [[nodiscard]] bool IsDeclaration() const noexcept
    {
        return isDeclaration_;
    }

    [[nodiscard]] bool IsOptional() const noexcept
    {
        return optional_;
    }

    void SetDeclaration() noexcept
    {
        isDeclaration_ = true;
    }

    void SetOptional(bool optional) noexcept
    {
        optional_ = optional;
    }

    void CleanUp() override
    {
        AstNode::CleanUp();
        SetPreferredType(nullptr);
    }

    void ClearPreferredType();
    void CleanCheckInformation() override;

    [[nodiscard]] ArrayExpression *Clone(ArenaAllocator *allocator, AstNode *parent) override;

    [[nodiscard]] bool ConvertibleToArrayPattern();
    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile(compiler::PandaGen *pg) const override;
    void Compile(compiler::ETSGen *etsg) const override;
    checker::Type *Check(checker::TSChecker *checker) override;
    checker::VerifiedType Check(checker::ETSChecker *checker) override;
    checker::Type *CheckPattern(checker::TSChecker *checker);
    bool TrySetPreferredTypeForNestedArrayExpr(checker::ETSChecker *checker, ArrayExpression *nestedArrayExpr,
                                               std::size_t idx) const;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }
    static std::optional<checker::Type *> ExtractPossiblePreferredType(checker::Type *type);

    void SetPreferredTypeBasedOnFuncParam(checker::ETSChecker *checker, checker::Type *param,
                                          checker::TypeRelationFlag flags);

private:
    ArenaVector<Expression *> elements_;
    bool isDeclaration_ {};
    bool trailingComma_ {};
    bool optional_ {};
};
}  // namespace ark::es2panda::ir

#endif
