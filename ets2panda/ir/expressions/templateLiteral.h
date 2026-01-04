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

#ifndef ES2PANDA_PARSER_INCLUDE_AST_TEMPLATE_LITERAL_H
#define ES2PANDA_PARSER_INCLUDE_AST_TEMPLATE_LITERAL_H

#include "ir/base/templateElement.h"
#include "ir/expression.h"

namespace ark::es2panda::ir {
class TemplateLiteral : public Expression {
private:
    struct Tag {};

public:
    TemplateLiteral() = delete;
    ~TemplateLiteral() override = default;

    NO_COPY_SEMANTIC(TemplateLiteral);
    NO_MOVE_SEMANTIC(TemplateLiteral);

    explicit TemplateLiteral(ArenaVector<TemplateElement *> &&quasis, ArenaVector<Expression *> &&expressions,
                             util::StringView multilineString)
        : Expression(AstNodeType::TEMPLATE_LITERAL),
          quasis_(std::move(quasis)),
          expressions_(std::move(expressions)),
          multilineString_(multilineString)
    {
    }

    explicit TemplateLiteral(Tag tag, TemplateLiteral const &other, ArenaAllocator *allocator);

    [[nodiscard]] const ArenaVector<TemplateElement *> &Quasis() const noexcept
    {
        return quasis_;
    }

    void SetQuasis(ArenaVector<TemplateElement *> &&quasisList)
    {
        quasis_ = std::move(quasisList);

        for (auto &quasis : quasis_) {
            quasis->SetParent(this);
        }
    }

    [[nodiscard]] const ArenaVector<Expression *> &Expressions() const noexcept
    {
        return expressions_;
    }

    void SetExpressions(ArenaVector<Expression *> &&expressionsList)
    {
        expressions_ = std::move(expressionsList);

        for (auto &expression : expressions_) {
            expression->SetParent(this);
        }
    }

    [[nodiscard]] TemplateLiteral *Clone(ArenaAllocator *allocator, AstNode *parent) override;

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile([[maybe_unused]] compiler::PandaGen *pg) const override;
    void Compile([[maybe_unused]] compiler::ETSGen *etsg) const override;
    checker::Type *Check([[maybe_unused]] checker::TSChecker *checker) override;
    checker::VerifiedType Check([[maybe_unused]] checker::ETSChecker *checker) override;
    util::StringView GetMultilineString() const;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    void CleanCheckInformation() override;

private:
    ArenaVector<TemplateElement *> quasis_;
    ArenaVector<Expression *> expressions_;
    util::StringView multilineString_;
};
}  // namespace ark::es2panda::ir

#endif
