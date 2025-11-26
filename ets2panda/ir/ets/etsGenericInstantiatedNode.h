/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_IR_EXPRESSION_ETS_GENERIC_INSTANTIATED_NODE_H
#define ES2PANDA_IR_EXPRESSION_ETS_GENERIC_INSTANTIATED_NODE_H

#include "ir/expression.h"

namespace ark::es2panda::checker {
class ETSAnalyzer;
}  // namespace ark::es2panda::checker

namespace ark::es2panda::ir {

class ETSGenericInstantiatedNode final : public Expression {
public:
    ETSGenericInstantiatedNode() = delete;
    NO_COPY_SEMANTIC(ETSGenericInstantiatedNode);
    NO_MOVE_SEMANTIC(ETSGenericInstantiatedNode);
    ~ETSGenericInstantiatedNode() override = default;

    explicit ETSGenericInstantiatedNode(Expression *expression, TSTypeParameterInstantiation *typeParams)
        : Expression(AstNodeType::ETS_GENERIC_INSTANTIATED_NODE), expression_(expression), typeParams_(typeParams)
    {
        ES2PANDA_ASSERT(expression->IsIdentifier() || expression->IsMemberExpression());
    }

    TSTypeParameterInstantiation *TypeParams()
    {
        return typeParams_;
    }

    Expression *GetExpression()
    {
        return expression_;
    }

    void Iterate(const NodeTraverser &cb) const override;
    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile(compiler::PandaGen *pg) const override;
    void Compile(compiler::ETSGen *etsg) const override;
    checker::Type *Check(checker::TSChecker *checker) override;
    checker::VerifiedType Check(checker::ETSChecker *checker) override;
    [[nodiscard]] ETSGenericInstantiatedNode *Clone(ArenaAllocator *allocator, AstNode *parent) override;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

private:
    Expression *expression_;
    TSTypeParameterInstantiation *typeParams_;
};
}  // namespace ark::es2panda::ir

#endif
