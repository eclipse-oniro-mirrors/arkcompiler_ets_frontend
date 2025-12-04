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

#ifndef ES2PANDA_IR_STATEMENT_RETURN_STATEMENT_H
#define ES2PANDA_IR_STATEMENT_RETURN_STATEMENT_H

#include "ir/statement.h"

namespace ark::es2panda::checker {
class ETSAnalyzer;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::compiler {
class ETSCompiler;
}  // namespace ark::es2panda::compiler

namespace ark::es2panda::compiler {
class JSCompiler;
class ETSCompiler;
}  // namespace ark::es2panda::compiler

namespace ark::es2panda::ir {
class ReturnStatement : public Statement {
public:
    explicit ReturnStatement() : ReturnStatement(nullptr) {}
    explicit ReturnStatement(Expression *argument) : Statement(AstNodeType::RETURN_STATEMENT), argument_(argument) {}

    // NOTE: csabahurton. these friend relationships can be removed once there are getters for private fields
    friend class checker::ETSAnalyzer;
    friend class compiler::ETSCompiler;

    Expression *Argument() noexcept
    {
        return argument_;
    }

    const Expression *Argument() const noexcept
    {
        return argument_;
    }

    checker::Type *ReturnType() noexcept
    {
        return returnType_;
    }

    const checker::Type *ReturnType() const noexcept
    {
        return returnType_;
    }

    void CleanUp() override
    {
        AstNode::CleanUp();
        returnType_ = nullptr;
    }

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void SetReturnType(checker::ETSChecker *checker, checker::Type *type) override;
    void SetArgument(Expression *arg);

    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile(compiler::PandaGen *pg) const override;
    void Compile(compiler::ETSGen *etsg) const override;
    checker::Type *Check(checker::TSChecker *checker) override;
    checker::VerifiedType Check(checker::ETSChecker *checker) override;
    bool IsAsyncImplReturn() const;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    ReturnStatement *Clone(ArenaAllocator *const allocator, AstNode *const parent) override;

private:
    EPtr<Expression> argument_ {};
    EPtr<checker::Type> returnType_ {};
};
}  // namespace ark::es2panda::ir

#endif
