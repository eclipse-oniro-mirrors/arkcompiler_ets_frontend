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

#ifndef ES2PANDA_PARSER_INCLUDE_AST_CLASS_STATIC_BLOCK_H
#define ES2PANDA_PARSER_INCLUDE_AST_CLASS_STATIC_BLOCK_H

#include "ir/base/classElement.h"

namespace ark::es2panda::ir {
class Expression;

class ClassStaticBlock : public ClassElement {
public:
    explicit ClassStaticBlock(Expression *value, [[maybe_unused]] ArenaAllocator *allocator)
        : ClassElement(AstNodeType::CLASS_STATIC_BLOCK, nullptr, value, ModifierFlags::NONE, allocator, false)
    {
        InitHistory();
    }

    PrivateFieldKind ToPrivateFieldKind([[maybe_unused]] bool isStatic) const override
    {
        return PrivateFieldKind::FIELD;
    }

    ir::ScriptFunction *Function();
    const ir::ScriptFunction *Function() const;
    const util::StringView &Name() const;

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
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

    ClassStaticBlock *Construct(ArenaAllocator *allocator) override
    {
        return allocator->New<ClassStaticBlock>(nullptr, allocator);
    }

    void CopyTo(AstNode *other) const override
    {
        ClassElement::CopyTo(other);
    };

private:
};
}  // namespace ark::es2panda::ir

#endif
