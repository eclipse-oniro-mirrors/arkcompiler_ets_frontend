/**
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_IR_EXPRESSION_UNARY_EXPRESSION_H
#define ES2PANDA_IR_EXPRESSION_UNARY_EXPRESSION_H

#include <ir/expression.h>
#include <lexer/token/tokenType.h>

namespace panda::es2panda::compiler {
class PandaGen;
}  // namespace panda::es2panda::compiler

namespace panda::es2panda::checker {
class Checker;
class Type;
}  // namespace panda::es2panda::checker

namespace panda::es2panda::ir {

class UnaryExpression : public Expression {
public:
    explicit UnaryExpression(Expression *argument, lexer::TokenType unaryOperator)
        : Expression(AstNodeType::UNARY_EXPRESSION), argument_(argument), operator_(unaryOperator)
    {
    }

    lexer::TokenType OperatorType() const
    {
        return operator_;
    }

    const Expression *Argument() const
    {
        return argument_;
    }

    bool IsNegativeNumber() const
    {
        return operator_ == lexer::TokenType::PUNCTUATOR_MINUS && argument_->IsNumberLiteral();
    }

    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Compile(compiler::PandaGen *pg) const override;
    checker::Type *Check(checker::Checker *checker) const override;
    void UpdateSelf(const NodeUpdater &cb, [[maybe_unused]] binder::Binder *binder) override;

private:
    Expression *argument_;
    lexer::TokenType operator_;
};

}  // namespace panda::es2panda::ir

#endif
