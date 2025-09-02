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

#ifndef ES2PANDA_IR_EXPRESSION_ETS_PARAMETER_EXPRESSION_H
#define ES2PANDA_IR_EXPRESSION_ETS_PARAMETER_EXPRESSION_H

#include "ir/annotationAllowed.h"
#include "ir/expression.h"
#include "ir/expressions/identifier.h"
#include "ir/statements/annotationUsage.h"

namespace ark::es2panda::checker {
class ETSAnalyzer;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::ir {

class ETSParameterExpression final : public AnnotationAllowed<Expression> {
public:
    ETSParameterExpression() = delete;
    ~ETSParameterExpression() override = default;

    NO_COPY_SEMANTIC(ETSParameterExpression);
    NO_MOVE_SEMANTIC(ETSParameterExpression);

    explicit ETSParameterExpression(AnnotatedExpression *identOrSpread, bool isOptional,
                                    ArenaAllocator *const allocator);

    explicit ETSParameterExpression(AnnotatedExpression *identOrSpread, bool isOptional,
                                    ArenaAllocator *const allocator, AstNodeHistory *history);

    explicit ETSParameterExpression(AnnotatedExpression *identOrSpread, ir::Expression *initializer,
                                    ArenaAllocator *const allocator);

    explicit ETSParameterExpression(AnnotatedExpression *identOrSpread, ir::Expression *initializer,
                                    ArenaAllocator *const allocator, AstNodeHistory *history);

    [[nodiscard]] const util::StringView &Name() const noexcept;

    [[nodiscard]] const Identifier *Ident() const noexcept;
    [[nodiscard]] Identifier *Ident() noexcept;

    void SetIdent(Identifier *ident) noexcept
    {
        this->GetOrCreateHistoryNodeAs<ETSParameterExpression>()->ident_ = ident;
        ES2PANDA_ASSERT(ident);
        ident->SetParent(this);
    }

    SpreadElement *Spread() noexcept
    {
        return GetHistoryNodeAs<ETSParameterExpression>()->spread_;
    }

    const SpreadElement *Spread() const noexcept
    {
        return GetHistoryNodeAs<ETSParameterExpression>()->spread_;
    }

    void SetSpread(SpreadElement *spread);

    [[nodiscard]] const SpreadElement *RestParameter() const noexcept;
    [[nodiscard]] SpreadElement *RestParameter() noexcept;

    [[nodiscard]] const Expression *Initializer() const noexcept;
    [[nodiscard]] Expression *Initializer() noexcept;

    void SetLexerSaved(util::StringView savedLexer);
    [[nodiscard]] util::StringView LexerSaved() const noexcept;

    [[nodiscard]] varbinder::Variable *Variable() const noexcept;
    void SetVariable(varbinder::Variable *variable) noexcept;

    [[nodiscard]] TypeNode const *TypeAnnotation() const noexcept;
    [[nodiscard]] TypeNode *TypeAnnotation() noexcept;

    void SetTypeAnnotation(TypeNode *typeNode) noexcept;

    [[nodiscard]] bool IsOptional() const noexcept
    {
        return GetHistoryNodeAs<ETSParameterExpression>()->isOptional_;
    }

    void SetOptional(bool value) noexcept
    {
        this->GetOrCreateHistoryNodeAs<ETSParameterExpression>()->isOptional_ = value;
        ES2PANDA_ASSERT(IsOptional() || Initializer() == nullptr);
    }

    void SetInitializer(Expression *initExpr) noexcept
    {
        this->GetOrCreateHistoryNodeAs<ETSParameterExpression>()->initializer_ = initExpr;
        ES2PANDA_ASSERT(IsOptional() || Initializer() == nullptr);
    }

    [[nodiscard]] bool IsRestParameter() const noexcept
    {
        return Spread() != nullptr;
    }

    [[nodiscard]] std::size_t GetRequiredParams() const noexcept
    {
        return GetHistoryNodeAs<ETSParameterExpression>()->extraValue_;
    }

    void SetRequiredParams(std::size_t const extraValue);

    [[nodiscard]] ETSParameterExpression *Clone(ArenaAllocator *allocator, AstNode *parent) override;

    void Iterate(const NodeTraverser &cb) const override;
    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
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

    ETSParameterExpression *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

private:
    friend class SizeOfNodeTest;

    Identifier *ident_;
    Expression *initializer_ = nullptr;
    SpreadElement *spread_ = nullptr;
    util::StringView savedLexer_ = "";
    std::size_t extraValue_ = 0U;
    bool isOptional_ = false;
};
}  // namespace ark::es2panda::ir

#endif
