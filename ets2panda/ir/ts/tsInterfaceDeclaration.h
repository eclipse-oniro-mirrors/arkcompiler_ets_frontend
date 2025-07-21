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

#ifndef ES2PANDA_IR_TS_INTERFACE_DECLARATION_H
#define ES2PANDA_IR_TS_INTERFACE_DECLARATION_H

#include "varbinder/scope.h"
#include "ir/annotationAllowed.h"
#include "ir/statement.h"
#include "ir/statements/annotationUsage.h"

namespace ark::es2panda::varbinder {
class Variable;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::ir {
class Identifier;
class TSInterfaceBody;
class TSInterfaceHeritage;
class TSTypeParameterDeclaration;

class TSInterfaceDeclaration : public AnnotationAllowed<TypedStatement> {
public:
    // NOLINTBEGIN(cppcoreguidelines-pro-type-member-init)
    struct ConstructorData {
        Identifier *id;
        TSTypeParameterDeclaration *typeParams;
        TSInterfaceBody *body;
        bool isStatic;
        bool isExternal;
        es2panda::Language lang;
    };
    // NOLINTEND(cppcoreguidelines-pro-type-member-init)

    explicit TSInterfaceDeclaration([[maybe_unused]] ArenaAllocator *allocator,
                                    ArenaVector<TSInterfaceHeritage *> &&extends, ConstructorData &&data)
        : AnnotationAllowed<TypedStatement>(AstNodeType::TS_INTERFACE_DECLARATION, allocator),
          id_(data.id),
          typeParams_(data.typeParams),
          body_(data.body),
          extends_(std::move(extends)),
          isStatic_(data.isStatic),
          isExternal_(data.isExternal),
          lang_(data.lang)
    {
        if (isStatic_) {
            AddModifier(ir::ModifierFlags::STATIC);
        }
        InitHistory();
    }

    explicit TSInterfaceDeclaration([[maybe_unused]] ArenaAllocator *allocator,
                                    ArenaVector<TSInterfaceHeritage *> &&extends, ConstructorData &&data,
                                    AstNodeHistory *history)
        : AnnotationAllowed<TypedStatement>(AstNodeType::TS_INTERFACE_DECLARATION, allocator),
          id_(data.id),
          typeParams_(data.typeParams),
          body_(data.body),
          extends_(std::move(extends)),
          isStatic_(data.isStatic),
          isExternal_(data.isExternal),
          lang_(data.lang)
    {
        if (isStatic_) {
            AddModifier(ir::ModifierFlags::STATIC);
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
        return GetHistoryNodeAs<TSInterfaceDeclaration>()->scope_;
    }

    void SetScope(varbinder::LocalScope *scope)
    {
        ES2PANDA_ASSERT(Scope() == nullptr);
        GetOrCreateHistoryNode()->AsTSInterfaceDeclaration()->scope_ = scope;
    }

    void ClearScope() noexcept override
    {
        GetOrCreateHistoryNode()->AsTSInterfaceDeclaration()->scope_ = nullptr;
    }

    TSInterfaceBody *Body()
    {
        return GetHistoryNodeAs<TSInterfaceDeclaration>()->body_;
    }

    const TSInterfaceBody *Body() const
    {
        return GetHistoryNodeAs<TSInterfaceDeclaration>()->body_;
    }

    Identifier *Id()
    {
        return GetHistoryNodeAs<TSInterfaceDeclaration>()->id_;
    }

    const Identifier *Id() const
    {
        return GetHistoryNodeAs<TSInterfaceDeclaration>()->id_;
    }

    const util::StringView &InternalName() const
    {
        return GetHistoryNodeAs<TSInterfaceDeclaration>()->internalName_;
    }

    void SetInternalName(util::StringView internalName);

    bool IsStatic() const
    {
        return GetHistoryNodeAs<TSInterfaceDeclaration>()->isStatic_;
    }

    bool IsFromExternal() const
    {
        return GetHistoryNodeAs<TSInterfaceDeclaration>()->isExternal_;
    }

    const TSTypeParameterDeclaration *TypeParams() const
    {
        return GetHistoryNodeAs<TSInterfaceDeclaration>()->typeParams_;
    }

    TSTypeParameterDeclaration *TypeParams()
    {
        return GetHistoryNodeAs<TSInterfaceDeclaration>()->typeParams_;
    }

    [[nodiscard]] const ArenaVector<TSInterfaceHeritage *> &Extends();
    [[nodiscard]] ArenaVector<TSInterfaceHeritage *> &ExtendsForUpdate();

    const ArenaVector<TSInterfaceHeritage *> &Extends() const
    {
        return GetHistoryNodeAs<TSInterfaceDeclaration>()->extends_;
    }

    void TransformChildren(const NodeTransformer &cb, std::string_view transformationName) override;

    [[nodiscard]] es2panda::Language Language() const noexcept
    {
        return GetHistoryNodeAs<TSInterfaceDeclaration>()->lang_;
    }

    ClassDeclaration *GetAnonClass() noexcept
    {
        return GetHistoryNodeAs<TSInterfaceDeclaration>()->anonClass_;
    }

    ClassDeclaration *GetAnonClass() const noexcept
    {
        return GetHistoryNodeAs<TSInterfaceDeclaration>()->anonClass_;
    }

    void SetAnonClass(ClassDeclaration *anonClass);

    void Iterate(const NodeTraverser &cb) const override;
    void Dump(ir::AstDumper *dumper) const override;
    void Dump(ir::SrcDumper *dumper) const override;
    void Compile([[maybe_unused]] compiler::PandaGen *pg) const override;
    void Compile(compiler::ETSGen *etsg) const override;
    checker::Type *Check([[maybe_unused]] checker::TSChecker *checker) override;
    checker::VerifiedType Check([[maybe_unused]] checker::ETSChecker *checker) override;
    checker::Type *InferType(checker::TSChecker *checker, varbinder::Variable *bindingVar) const;

    void Accept(ASTVisitorT *v) override
    {
        v->Accept(this);
    }

    TSInterfaceDeclaration *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

    void EmplaceExtends(TSInterfaceHeritage *extends);
    void ClearExtends();
    void SetValueExtends(TSInterfaceHeritage *extends, size_t index);

private:
    bool RegisterUnexportedForDeclGen(ir::SrcDumper *dumper) const;
    friend class SizeOfNodeTest;
    void SetId(Identifier *id);
    void SetTypeParams(TSTypeParameterDeclaration *typeParams);
    void SetBody(TSInterfaceBody *body);

    varbinder::LocalScope *scope_ {nullptr};
    Identifier *id_;
    TSTypeParameterDeclaration *typeParams_;
    TSInterfaceBody *body_;
    ArenaVector<TSInterfaceHeritage *> extends_;
    util::StringView internalName_ {};
    bool isStatic_;
    bool isExternal_;
    es2panda::Language lang_;
    ClassDeclaration *anonClass_ {nullptr};
};
}  // namespace ark::es2panda::ir

#endif
