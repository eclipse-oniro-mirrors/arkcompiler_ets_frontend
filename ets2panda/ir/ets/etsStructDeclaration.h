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

#ifndef ES2PANDA_IR_ETS_STRUCT_DECLARATION_H
#define ES2PANDA_IR_ETS_STRUCT_DECLARATION_H

#include "ir/statements/classDeclaration.h"

namespace ark::es2panda::ir {
class ETSStructDeclaration : public ClassDeclaration {
public:
    ETSStructDeclaration() = delete;
    ~ETSStructDeclaration() override = default;

    NO_COPY_SEMANTIC(ETSStructDeclaration);
    NO_MOVE_SEMANTIC(ETSStructDeclaration);

    explicit ETSStructDeclaration(ClassDefinition *const def, ArenaAllocator *const allocator)
        : ClassDeclaration(AstNodeType::STRUCT_DECLARATION, def, allocator)
    {
        InitHistory();
    }

    explicit ETSStructDeclaration(ClassDefinition *const def, ArenaAllocator *const allocator, AstNodeHistory *history)
        : ClassDeclaration(AstNodeType::STRUCT_DECLARATION, def, allocator)
    {
        if (history != nullptr) {
            history_ = history;
        } else {
            InitHistory();
        }
    }

    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;

    checker::Type *Check(checker::TSChecker *checker) override;
    checker::VerifiedType Check(checker::ETSChecker *checker) override;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    ETSStructDeclaration *Construct(ArenaAllocator *allocator) override
    {
        return allocator->New<ETSStructDeclaration>(nullptr, allocator);
    }

    void CopyTo(AstNode *other) const override
    {
        ClassDeclaration::CopyTo(other);
    };
};
}  // namespace ark::es2panda::ir

#endif
