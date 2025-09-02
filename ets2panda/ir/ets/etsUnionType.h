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

#ifndef ES2PANDA_IR_ETS_UNION_TYPE_H
#define ES2PANDA_IR_ETS_UNION_TYPE_H

#include "ir/typeNode.h"

namespace ark::es2panda::ir {
class ETSUnionType : public TypeNode {
public:
    explicit ETSUnionType(ArenaVector<TypeNode *> &&types, ArenaAllocator *const allocator)
        : TypeNode(AstNodeType::ETS_UNION_TYPE, allocator), types_(std::move(types))
    {
        InitHistory();
    }

    const ArenaVector<TypeNode *> &Types() const
    {
        return GetHistoryNodeAs<ETSUnionType>()->types_;
    }

    void SetValueTypes(TypeNode *type, size_t index) const
    {
        GetOrCreateHistoryNodeAs<ETSUnionType>()->types_[index] = type;
    }

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile([[maybe_unused]] compiler::PandaGen *pg) const override;
    checker::Type *Check([[maybe_unused]] checker::TSChecker *checker) override;
    checker::VerifiedType Check([[maybe_unused]] checker::ETSChecker *checker) override;
    checker::Type *GetType([[maybe_unused]] checker::ETSChecker *checker) override;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    [[nodiscard]] ETSUnionType *Clone(ArenaAllocator *allocator, AstNode *parent) override;

protected:
    ETSUnionType *Construct(ArenaAllocator *allocator) override
    {
        ArenaVector<TypeNode *> types(allocator->Adapter());
        return allocator->New<ETSUnionType>(std::move(types), allocator);
    }

    void CopyTo(AstNode *other) const override
    {
        auto otherImpl = reinterpret_cast<ETSUnionType *>(other);

        otherImpl->types_ = types_;

        TypeNode::CopyTo(other);
    }

private:
    ArenaVector<TypeNode *> types_;
};
}  // namespace ark::es2panda::ir

#endif
