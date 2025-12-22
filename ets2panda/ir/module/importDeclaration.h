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

#ifndef ES2PANDA_IR_MODULE_IMPORT_DECLARATION_H
#define ES2PANDA_IR_MODULE_IMPORT_DECLARATION_H

#include "ir/statement.h"

namespace ark::es2panda::ir {
class StringLiteral;

enum class ImportKinds { ALL, TYPES };
using ExportKinds = ImportKinds;

class ImportDeclaration : public Statement {
public:
    void EmplaceSpecifiers(AstNode *source);
    void ClearSpecifiers();
    void SetValueSpecifiers(AstNode *source, size_t index);
    [[nodiscard]] ArenaVector<AstNode *> &SpecifiersForUpdate();

    explicit ImportDeclaration(StringLiteral *source, ArenaVector<AstNode *> &&specifiers,
                               const ImportKinds importKinds = ImportKinds::ALL)
        : Statement(AstNodeType::IMPORT_DECLARATION),
          source_(source),
          specifiers_(std::move(specifiers)),
          importKinds_(importKinds)
    {
        InitHistory();
    }

    explicit ImportDeclaration(StringLiteral *source, ArenaVector<AstNode *> &&specifiers,
                               const ImportKinds importKinds, AstNodeHistory *history)
        : Statement(AstNodeType::IMPORT_DECLARATION),
          source_(source),
          specifiers_(std::move(specifiers)),
          importKinds_(importKinds)
    {
        if (history != nullptr) {
            SetHistoryInternal(history);
        } else {
            InitHistory();
        }
    }

    const StringLiteral *Source() const
    {
        return GetHistoryNodeAs<ImportDeclaration>()->source_;
    }

    StringLiteral *Source()
    {
        return GetHistoryNodeAs<ImportDeclaration>()->source_;
    }

    void SetSource(StringLiteral *source);

    const ArenaVector<AstNode *> &Specifiers() const
    {
        return GetHistoryNodeAs<ImportDeclaration>()->specifiers_;
    }

    void SetSpecifiers(ArenaVector<AstNode *> &&specifiersList)
    {
        auto newNode = GetOrCreateHistoryNodeAs<ImportDeclaration>();
        auto &specifiers = newNode->specifiers_;
        specifiers = std::move(specifiersList);

        for (auto *specifier : specifiers) {
            specifier->SetParent(newNode);
        }
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

    bool IsTypeKind() const
    {
        return GetHistoryNodeAs<ImportDeclaration>()->importKinds_ == ImportKinds::TYPES;
    }

    ImportDeclaration *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

private:
    friend class SizeOfNodeTest;

    StringLiteral *source_;
    ArenaVector<AstNode *> specifiers_;
    ImportKinds importKinds_;
};
}  // namespace ark::es2panda::ir

#endif
