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

#ifndef ES2PANDA_IR_STATEMENT_FUNCTION_DECLARATION_H
#define ES2PANDA_IR_STATEMENT_FUNCTION_DECLARATION_H

#include "ir/annotationAllowed.h"
#include "ir/statement.h"
#include "ir/statements/annotationUsage.h"
#include "ir/base/scriptFunction.h"

namespace ark::es2panda::ir {
class ScriptFunction;
class AnnotationUsage;

class FunctionDeclaration : public AnnotationAllowed<Statement> {
public:
    explicit FunctionDeclaration(ArenaAllocator *allocator, ScriptFunction *func,
                                 ArenaVector<AnnotationUsage *> &&annotations, bool isAnonymous = false)
        : AnnotationAllowed<Statement>(AstNodeType::FUNCTION_DECLARATION, std::move(annotations)),
          decorators_(allocator->Adapter()),
          func_(func),
          isAnonymous_(isAnonymous)
    {
        InitHistory();
        if (func != nullptr) {
            flags_ = func->Modifiers();
        }
    }

    explicit FunctionDeclaration(ArenaAllocator *allocator, ScriptFunction *func, bool isAnonymous = false)
        : AnnotationAllowed<Statement>(AstNodeType::FUNCTION_DECLARATION, allocator),
          decorators_(allocator->Adapter()),
          func_(func),
          isAnonymous_(isAnonymous)
    {
        InitHistory();
        if (func != nullptr) {
            flags_ = func->Modifiers();
        }
    }

    explicit FunctionDeclaration(ArenaAllocator *allocator, ScriptFunction *func, bool isAnonymous,
                                 AstNodeHistory *history)
        : AnnotationAllowed<Statement>(AstNodeType::FUNCTION_DECLARATION, allocator),
          decorators_(allocator->Adapter()),
          func_(func),
          isAnonymous_(isAnonymous)
    {
        if (history != nullptr) {
            history_ = history;
        } else {
            InitHistory();
        }
    }

    ScriptFunction *Function()
    {
        return GetHistoryNodeAs<FunctionDeclaration>()->func_;
    }

    bool IsAnonymous() const
    {
        return GetHistoryNodeAs<FunctionDeclaration>()->isAnonymous_;
    }

    const ScriptFunction *Function() const
    {
        return GetHistoryNodeAs<FunctionDeclaration>()->func_;
    }

    void AddDecorators([[maybe_unused]] ArenaVector<ir::Decorator *> &&decorators) override
    {
        auto newNode = this->GetOrCreateHistoryNode()->AsFunctionDeclaration();
        newNode->decorators_ = std::move(decorators);
    }

    bool CanHaveDecorator([[maybe_unused]] bool inTs) const override
    {
        return !inTs;
    }

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

    FunctionDeclaration *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

    [[nodiscard]] const ArenaVector<Decorator *> &Decorators() const
    {
        return GetHistoryNodeAs<FunctionDeclaration>()->decorators_;
    };

private:
    friend class SizeOfNodeTest;
    void SetFunction(ScriptFunction *func);
    void EmplaceDecorators(Decorator *decorators);
    void ClearDecorators();
    void SetValueDecorators(Decorator *decorators, size_t index);
    [[nodiscard]] ArenaVector<Decorator *> &DecoratorsForUpdate();

    ArenaVector<Decorator *> decorators_;
    ScriptFunction *func_;
    bool isAnonymous_;
};
}  // namespace ark::es2panda::ir

#endif
