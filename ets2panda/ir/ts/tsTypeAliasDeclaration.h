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

#ifndef ES2PANDA_IR_TS_TYPE_ALIAS_DECLARATION_H
#define ES2PANDA_IR_TS_TYPE_ALIAS_DECLARATION_H

#include "ir/statement.h"
#include "ir/typed.h"
#include "ir/statements/annotationUsage.h"

namespace ark::es2panda::varbinder {
class Variable;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::ir {
class Identifier;
class TSTypeParameterDeclaration;

class TSTypeAliasDeclaration : public AnnotatedStatement {
public:
    explicit TSTypeAliasDeclaration([[maybe_unused]] ArenaAllocator *allocator, Identifier *id,
                                    TSTypeParameterDeclaration *typeParams, TypeNode *typeAnnotation)
        : AnnotatedStatement(AstNodeType::TS_TYPE_ALIAS_DECLARATION, typeAnnotation),
          annotations_(allocator->Adapter()),
          id_(id),
          typeParams_(typeParams),
          typeParamTypes_(allocator->Adapter())
    {
        InitHistory();
    }

    explicit TSTypeAliasDeclaration([[maybe_unused]] ArenaAllocator *allocator, Identifier *id)
        : AnnotatedStatement(AstNodeType::TS_TYPE_ALIAS_DECLARATION),
          annotations_(allocator->Adapter()),
          id_(id),
          typeParams_(nullptr),
          typeParamTypes_(allocator->Adapter())
    {
        InitHistory();
    }

    Identifier *Id()
    {
        return GetHistoryNodeAs<TSTypeAliasDeclaration>()->id_;
    }

    const Identifier *Id() const
    {
        return GetHistoryNodeAs<TSTypeAliasDeclaration>()->id_;
    }

    TSTypeParameterDeclaration *TypeParams() const
    {
        return GetHistoryNodeAs<TSTypeAliasDeclaration>()->typeParams_;
    }

    void SetTypeParameters(ir::TSTypeParameterDeclaration *typeParams);

    void SetTypeParameterTypes(ArenaVector<checker::Type *> &&typeParamTypes)
    {
        auto newNode = reinterpret_cast<TSTypeAliasDeclaration *>(GetOrCreateHistoryNode());
        newNode->typeParamTypes_ = std::move(typeParamTypes);
    }

    ArenaVector<checker::Type *> const &TypeParameterTypes() const
    {
        return GetHistoryNodeAs<TSTypeAliasDeclaration>()->typeParamTypes_;
    }

    [[nodiscard]] const ArenaVector<ir::AnnotationUsage *> &Annotations() const noexcept
    {
        return GetHistoryNodeAs<TSTypeAliasDeclaration>()->annotations_;
    }

    void SetAnnotations(ArenaVector<ir::AnnotationUsage *> &&annotations)
    {
        auto newNode = reinterpret_cast<TSTypeAliasDeclaration *>(GetOrCreateHistoryNode());
        newNode->annotations_ = std::move(annotations);
        for (AnnotationUsage *anno : newNode->annotations_) {
            anno->SetParent(this);
        }
    }

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

    void CleanUp() override
    {
        AstNode::CleanUp();
        ClearTypeParamterTypes();
    }

    TSTypeAliasDeclaration *Construct(ArenaAllocator *allocator) override;
    void CopyTo(AstNode *other) const override;

    void EmplaceAnnotations(AnnotationUsage *annotations);
    void ClearAnnotations();
    void SetValueAnnotations(AnnotationUsage *annotations, size_t index);
    [[nodiscard]] ArenaVector<AnnotationUsage *> &AnnotationsForUpdate();

    void EmplaceTypeParamterTypes(checker::Type *typeParamTypes);
    void ClearTypeParamterTypes();
    void SetValueTypeParamterTypes(checker::Type *typeParamTypes, size_t index);
    [[nodiscard]] ArenaVector<checker::Type *> &TypeParamterTypesForUpdate();

private:
    bool RegisterUnexportedForDeclGen(ir::SrcDumper *dumper) const;
    friend class SizeOfNodeTest;

    void SetId(Identifier *id);

    ArenaVector<AnnotationUsage *> annotations_;
    Identifier *id_;
    TSTypeParameterDeclaration *typeParams_;
    ArenaVector<checker::Type *> typeParamTypes_;
};
}  // namespace ark::es2panda::ir

#endif
