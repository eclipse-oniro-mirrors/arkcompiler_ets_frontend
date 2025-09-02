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

#ifndef ES2PANDA_IR_TS_ENUM_DECLARATION_H
#define ES2PANDA_IR_TS_ENUM_DECLARATION_H

#include "ir/statement.h"
#include "ir/statements/annotationUsage.h"
#include "varbinder/scope.h"

namespace ark::es2panda::varbinder {
class EnumVariable;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::ir {
class Identifier;
class TSEnumMember;

class TSEnumDeclaration : public TypedStatement {
public:
    // NOLINTBEGIN(cppcoreguidelines-pro-type-member-init)
    struct ConstructorFlags {
        bool isConst;
        bool isStatic = false;
        bool isDeclare = false;
    };
    // NOLINTEND(cppcoreguidelines-pro-type-member-init)

    explicit TSEnumDeclaration(ArenaAllocator *allocator, Identifier *key, ArenaVector<AstNode *> &&members,
                               ConstructorFlags &&flags, Language lang)
        : TypedStatement(AstNodeType::TS_ENUM_DECLARATION),
          decorators_(allocator->Adapter()),
          key_(key),
          typeNode_(nullptr),
          members_(std::move(members)),
          isConst_(flags.isConst),
          lang_(lang)
    {
        if (flags.isStatic) {
            AddModifier(ModifierFlags::STATIC);
        }
        if (flags.isDeclare) {
            AddModifier(ModifierFlags::DECLARE);
        }
    }

    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    explicit TSEnumDeclaration(ArenaAllocator *allocator, Identifier *key, ArenaVector<AstNode *> &&members,
                               ConstructorFlags &&flags, ir::TypeNode *typeNode, Language lang)
        : TypedStatement(AstNodeType::TS_ENUM_DECLARATION),
          decorators_(allocator->Adapter()),
          key_(key),
          typeNode_(typeNode),
          members_(std::move(members)),
          isConst_(flags.isConst),
          lang_(lang)
    {
        if (flags.isStatic) {
            AddModifier(ModifierFlags::STATIC);
        }
        if (flags.isDeclare) {
            AddModifier(ModifierFlags::DECLARE);
        }
        InitHistory();
    }

    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    explicit TSEnumDeclaration(ArenaAllocator *allocator, Identifier *key, ArenaVector<AstNode *> &&members,
                               ConstructorFlags &&flags, Language lang, AstNodeHistory *history)
        : TypedStatement(AstNodeType::TS_ENUM_DECLARATION),
          decorators_(allocator->Adapter()),
          key_(key),
          typeNode_(nullptr),
          members_(std::move(members)),
          isConst_(flags.isConst),
          lang_(lang)
    {
        if (flags.isStatic) {
            AddModifier(ModifierFlags::STATIC);
        }
        if (flags.isDeclare) {
            AddModifier(ModifierFlags::DECLARE);
        }
        if (history != nullptr) {
            history_ = history;
        } else {
            InitHistory();
        }
    }

    [[nodiscard]] bool IsScopeBearer() const noexcept override
    {
        return true;
    }

    [[nodiscard]] varbinder::LocalScope *Scope() const noexcept override
    {
        return GetHistoryNodeAs<TSEnumDeclaration>()->scope_;
    }

    void SetScope(varbinder::LocalScope *scope)
    {
        ES2PANDA_ASSERT(Scope() == nullptr);
        GetOrCreateHistoryNode()->AsTSEnumDeclaration()->scope_ = scope;
    }

    void ClearScope() noexcept override
    {
        SetScope(nullptr);
    }

    const Identifier *Key() const
    {
        return GetHistoryNodeAs<TSEnumDeclaration>()->key_;
    }

    TypeNode *TypeNodes()
    {
        return typeNode_;
    }

    Identifier *Key()
    {
        return GetHistoryNodeAs<TSEnumDeclaration>()->key_;
    }

    const ArenaVector<AstNode *> &Members() const
    {
        return GetHistoryNodeAs<TSEnumDeclaration>()->members_;
    }

    const util::StringView &InternalName() const
    {
        return GetHistoryNodeAs<TSEnumDeclaration>()->internalName_;
    }

    void SetInternalName(util::StringView internalName);

    ir::ClassDefinition *BoxedClass() const
    {
        return GetHistoryNodeAs<TSEnumDeclaration>()->boxedClass_;
    }

    void SetBoxedClass(ir::ClassDefinition *boxedClass);

    bool IsConst() const
    {
        return GetHistoryNodeAs<TSEnumDeclaration>()->isConst_;
    }

    const ArenaVector<Decorator *> &Decorators() const
    {
        return GetHistoryNodeAs<TSEnumDeclaration>()->decorators_;
    }

    void AddDecorators([[maybe_unused]] ArenaVector<ir::Decorator *> &&decorators) override
    {
        auto newNode = GetOrCreateHistoryNodeAs<TSEnumDeclaration>();
        newNode->decorators_ = std::move(decorators);
    }

    bool CanHaveDecorator([[maybe_unused]] bool inTs) const override
    {
        return !inTs;
    }

    [[nodiscard]] es2panda::Language Language() const noexcept
    {
        return GetHistoryNodeAs<TSEnumDeclaration>()->lang_;
    }

    static varbinder::EnumMemberResult EvaluateEnumMember(checker::TSChecker *checker, varbinder::EnumVariable *enumVar,
                                                          const ir::AstNode *expr);
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

    TSEnumDeclaration *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

    void EmplaceDecorators(Decorator *source);
    void ClearDecorators();
    void SetValueDecorators(Decorator *source, size_t index);
    [[nodiscard]] ArenaVector<Decorator *> &DecoratorsForUpdate();

    void EmplaceMembers(AstNode *source);
    void ClearMembers();
    void SetValueMembers(AstNode *source, size_t index);
    [[nodiscard]] ArenaVector<AstNode *> &MembersForUpdate();

private:
    bool RegisterUnexportedForDeclGen(ir::SrcDumper *dumper) const;
    friend class SizeOfNodeTest;
    void SetKey(Identifier *key);

    varbinder::LocalScope *scope_ {nullptr};
    ArenaVector<ir::Decorator *> decorators_;
    Identifier *key_;
    ir::TypeNode *typeNode_;
    ArenaVector<AstNode *> members_;
    util::StringView internalName_;
    ir::ClassDefinition *boxedClass_ {nullptr};
    bool isConst_;
    es2panda::Language lang_;
};
}  // namespace ark::es2panda::ir

#endif
