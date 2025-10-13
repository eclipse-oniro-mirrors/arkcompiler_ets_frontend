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

#ifndef ES2PANDA_IR_ETS_TYPE_REFERENCE_PART_H
#define ES2PANDA_IR_ETS_TYPE_REFERENCE_PART_H

#include "ir/typeNode.h"

namespace ark::es2panda::ir {

class ETSTypeReferencePart : public TypeNode {
public:
    explicit ETSTypeReferencePart(ir::Expression *name, ir::TSTypeParameterInstantiation *typeParams,
                                  ir::ETSTypeReferencePart *prev, ArenaAllocator *const allocator)
        : TypeNode(AstNodeType::ETS_TYPE_REFERENCE_PART, allocator), name_(name), typeParams_(typeParams), prev_(prev)
    {
        InitHistory();
    }

    explicit ETSTypeReferencePart(ir::Expression *name, ArenaAllocator *const allocator)
        : TypeNode(AstNodeType::ETS_TYPE_REFERENCE_PART, allocator), name_(name)
    {
        InitHistory();
    }

    explicit ETSTypeReferencePart(ir::Expression *name, ArenaAllocator *const allocator, AstNodeHistory *history)
        : TypeNode(AstNodeType::ETS_TYPE_REFERENCE_PART, allocator), name_(name)
    {
        if (history != nullptr) {
            history_ = history;
        } else {
            InitHistory();
        }
    }

    explicit ETSTypeReferencePart(ir::Expression *name, ir::TSTypeParameterInstantiation *typeParams,
                                  ir::ETSTypeReferencePart *prev, ArenaAllocator *const allocator,
                                  AstNodeHistory *history)
        : TypeNode(AstNodeType::ETS_TYPE_REFERENCE_PART, allocator), name_(name), typeParams_(typeParams), prev_(prev)
    {
        if (history != nullptr) {
            history_ = history;
        } else {
            InitHistory();
        }
    }

    ir::ETSTypeReferencePart *Previous()
    {
        return GetHistoryNodeAs<ETSTypeReferencePart>()->prev_;
    }

    const ir::ETSTypeReferencePart *Previous() const
    {
        return GetHistoryNodeAs<ETSTypeReferencePart>()->prev_;
    }

    void SetPrevious(ir::ETSTypeReferencePart *prev);

    ir::Expression *Name()
    {
        return GetHistoryNodeAs<ETSTypeReferencePart>()->name_;
    }

    void SetName(ir::Expression *name);

    ir::TSTypeParameterInstantiation *TypeParams()
    {
        return GetHistoryNodeAs<ETSTypeReferencePart>()->typeParams_;
    }

    void SetTypeParams(ir::TSTypeParameterInstantiation *typeParams);

    const ir::TSTypeParameterInstantiation *TypeParams() const
    {
        return GetHistoryNodeAs<ETSTypeReferencePart>()->typeParams_;
    }

    const ir::Expression *Name() const
    {
        return GetHistoryNodeAs<ETSTypeReferencePart>()->name_;
    }

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile(compiler::PandaGen *pg) const override;
    void Compile(compiler::ETSGen *etsg) const override;
    checker::Type *Check(checker::TSChecker *checker) override;
    checker::VerifiedType Check(checker::ETSChecker *checker) override;
    checker::Type *GetType([[maybe_unused]] checker::ETSChecker *checker) override;
    ir::Identifier *GetIdent();

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    [[nodiscard]] ETSTypeReferencePart *Clone(ArenaAllocator *allocator, AstNode *parent) override;

    ETSTypeReferencePart *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

private:
    checker::Type *HandleInternalTypes(checker::ETSChecker *checker);

    friend class SizeOfNodeTest;

    ir::Expression *name_;
    ir::TSTypeParameterInstantiation *typeParams_ {};
    ir::ETSTypeReferencePart *prev_ {};
};
}  // namespace ark::es2panda::ir

#endif
