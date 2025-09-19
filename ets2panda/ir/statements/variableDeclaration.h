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

#ifndef ES2PANDA_IR_STATEMENT_VARIABLE_DECLARATION_H
#define ES2PANDA_IR_STATEMENT_VARIABLE_DECLARATION_H

#include "ir/annotationAllowed.h"
#include "ir/statement.h"
#include "ir/expressions/identifier.h"
#include "ir/statements/annotationUsage.h"
#include "variableDeclarator.h"

namespace ark::es2panda::ir {
class VariableDeclarator;

class VariableDeclaration : public AnnotationAllowed<Statement> {
private:
    struct Tag {};

public:
    enum class VariableDeclarationKind { CONST, LET, VAR };

    explicit VariableDeclaration(VariableDeclarationKind kind, ArenaAllocator *allocator,
                                 ArenaVector<VariableDeclarator *> &&declarators)
        : AnnotationAllowed<Statement>(AstNodeType::VARIABLE_DECLARATION, allocator),
          kind_(kind),
          declarators_(std::move(declarators))
    {
        InitHistory();
    }

    explicit VariableDeclaration(VariableDeclarationKind kind, ArenaAllocator *allocator,
                                 ArenaVector<VariableDeclarator *> &&declarators, AstNodeHistory *history)
        : AnnotationAllowed<Statement>(AstNodeType::VARIABLE_DECLARATION, allocator),
          kind_(kind),
          declarators_(std::move(declarators))
    {
        if (history != nullptr) {
            history_ = history;
        } else {
            InitHistory();
        }
    }

    explicit VariableDeclaration(Tag tag, VariableDeclaration const &other, ArenaAllocator *allocator);

    explicit VariableDeclaration(Tag const tag, VariableDeclaration const &other, ArenaAllocator *const allocator,
                                 AstNodeHistory *history);

    const ArenaVector<VariableDeclarator *> &Declarators() const
    {
        return GetHistoryNodeAs<VariableDeclaration>()->declarators_;
    }

    void SetDeclarators(ArenaVector<VariableDeclarator *> &&declaratorsList)
    {
        auto newNode = this->GetOrCreateHistoryNodeAs<VariableDeclaration>();
        auto &declarators = newNode->declarators_;
        declarators = std::move(declaratorsList);

        for (auto *declarator : declarators) {
            declarator->SetParent(newNode);
        }
    }

    [[nodiscard]] const ArenaVector<VariableDeclarator *> &Declarators();

    [[nodiscard]] ArenaVector<VariableDeclarator *> &DeclaratorsForUpdate();

    VariableDeclarationKind Kind() const
    {
        return GetHistoryNodeAs<VariableDeclaration>()->kind_;
    }

    VariableDeclarator *GetDeclaratorByName(util::StringView name) const
    {
        for (VariableDeclarator *declarator : Declarators()) {
            if (declarator->Id()->AsIdentifier()->Name().Compare(name) == 0) {
                return declarator;
            }
        }
        return nullptr;
    }

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;
    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile([[maybe_unused]] compiler::PandaGen *pg) const override;
    void Compile([[maybe_unused]] compiler::ETSGen *etsg) const override;
    checker::Type *Check([[maybe_unused]] checker::TSChecker *checker) override;
    checker::VerifiedType Check([[maybe_unused]] checker::ETSChecker *checker) override;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    [[nodiscard]] VariableDeclaration *Clone(ArenaAllocator *allocator, AstNode *parent) override;

    VariableDeclaration *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

private:
    friend class SizeOfNodeTest;
    void SetValueDeclarators(VariableDeclarator *source, size_t index);
    void EmplaceDeclarators(VariableDeclarator *source);
    void ClearDeclarators();

    VariableDeclarationKind kind_;
    ArenaVector<VariableDeclarator *> declarators_;
};
}  // namespace ark::es2panda::ir

#endif
