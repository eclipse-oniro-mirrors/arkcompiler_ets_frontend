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

#ifndef ES2PANDA_IR_TS_TYPE_PARAMETER_H
#define ES2PANDA_IR_TS_TYPE_PARAMETER_H

#include "ir/expression.h"
#include "ir/annotationAllowed.h"

namespace ark::es2panda::ir {
class Identifier;

class TSTypeParameter : public AnnotationAllowed<Expression> {
public:
    explicit TSTypeParameter(Identifier *name, TypeNode *constraint, TypeNode *defaultType,
                             ArenaAllocator *const allocator)
        : AnnotationAllowed<Expression>(AstNodeType::TS_TYPE_PARAMETER, allocator),
          name_(name),
          constraint_(constraint),
          defaultType_(defaultType)
    {
        InitHistory();
    }

    explicit TSTypeParameter(Identifier *name, TypeNode *constraint, TypeNode *defaultType, ModifierFlags flags,
                             ArenaAllocator *const allocator)
        : AnnotationAllowed<Expression>(AstNodeType::TS_TYPE_PARAMETER, flags, allocator),
          name_(name),
          constraint_(constraint),
          defaultType_(defaultType)
    {
        ES2PANDA_ASSERT(flags == ModifierFlags::NONE || flags == ModifierFlags::IN || flags == ModifierFlags::OUT);
        InitHistory();
    }

    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    explicit TSTypeParameter(Identifier *name, TypeNode *constraint, TypeNode *defaultType, ModifierFlags flags,
                             ArenaAllocator *const allocator, AstNodeHistory *history)
        : AnnotationAllowed<Expression>(AstNodeType::TS_TYPE_PARAMETER, flags, allocator),
          name_(name),
          constraint_(constraint),
          defaultType_(defaultType)
    {
        ES2PANDA_ASSERT(flags == ModifierFlags::NONE || flags == ModifierFlags::IN || flags == ModifierFlags::OUT);
        if (history != nullptr) {
            SetHistoryInternal(history);
        } else {
            InitHistory();
        }
    }

    explicit TSTypeParameter(Identifier *name, TypeNode *constraint, TypeNode *defaultType,
                             ArenaAllocator *const allocator, AstNodeHistory *history)
        : AnnotationAllowed<Expression>(AstNodeType::TS_TYPE_PARAMETER, allocator),
          name_(name),
          constraint_(constraint),
          defaultType_(defaultType)
    {
        if (history != nullptr) {
            SetHistoryInternal(history);
        } else {
            InitHistory();
        }
    }

    const Identifier *Name() const
    {
        return GetHistoryNodeAs<TSTypeParameter>()->name_;
    }

    Identifier *Name()
    {
        return GetHistoryNodeAs<TSTypeParameter>()->name_;
    }

    TypeNode *Constraint()
    {
        return GetHistoryNodeAs<TSTypeParameter>()->constraint_;
    }

    const TypeNode *Constraint() const
    {
        return GetHistoryNodeAs<TSTypeParameter>()->constraint_;
    }

    void SetConstraint(TypeNode *constraint);

    TypeNode *DefaultType() const
    {
        return GetHistoryNodeAs<TSTypeParameter>()->defaultType_;
    }

    void SetDefaultType(TypeNode *defaultType);

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

    TSTypeParameter *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

private:
    friend class SizeOfNodeTest;
    void SetName(Identifier *name);

    Identifier *name_;
    TypeNode *constraint_;
    TypeNode *defaultType_;
};
}  // namespace ark::es2panda::ir

#endif
