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

#ifndef ES2PANDA_IR_ETS_INTRINSIC_NODE_H
#define ES2PANDA_IR_ETS_INTRINSIC_NODE_H

#include "ir/expression.h"
#include "ir/visitor/AstVisitor.h"
#include "utils/arena_containers.h"

namespace ark::es2panda::ir {

class EtsIntrinsicInfo;
extern EtsIntrinsicInfo const *GetIntrinsicInfoFor(ETSIntrinsicNode const *node);

class ETSIntrinsicNode : public Expression {
public:
    ETSIntrinsicNode() = delete;
    ~ETSIntrinsicNode() override = default;

    NO_COPY_SEMANTIC(ETSIntrinsicNode);
    NO_MOVE_SEMANTIC(ETSIntrinsicNode);

    explicit ETSIntrinsicNode(ETSIntrinsicNode const &other, ArenaAllocator *const allocator);
    explicit ETSIntrinsicNode(util::StringView id, ArenaVector<ir::Expression *> &&arguments);

    util::StringView Id() const;
    checker::Type *ExpectedTypeAt(checker::ETSChecker *checker, size_t idx) const;

    ArenaVector<ir::Expression *> const &Arguments() const
    {
        return arguments_;
    }

    ArenaVector<ir::Expression *> &Arguments()
    {
        return arguments_;
    }

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile([[maybe_unused]] compiler::PandaGen *pg) const override;
    void Compile(compiler::ETSGen *etsg) const override;
    checker::Type *Check([[maybe_unused]] checker::TSChecker *checker) override;
    checker::VerifiedType Check([[maybe_unused]] checker::ETSChecker *checker) override;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    [[nodiscard]] ETSIntrinsicNode *Clone(ArenaAllocator *allocator, AstNode *parent) override;

private:
    friend EtsIntrinsicInfo const *GetIntrinsicInfoFor(ETSIntrinsicNode const *node);

    EtsIntrinsicInfo const *info_;
    ArenaVector<ir::Expression *> arguments_;
};
}  // namespace ark::es2panda::ir

#endif