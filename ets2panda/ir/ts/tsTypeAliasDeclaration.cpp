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

#include "tsTypeAliasDeclaration.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"
#include "ir/typeNode.h"
#include "ir/base/decorator.h"
#include "ir/expressions/identifier.h"
#include "ir/ts/tsTypeParameter.h"
#include "ir/ts/tsTypeParameterDeclaration.h"

namespace ark::es2panda::ir {

void TSTypeAliasDeclaration::SetTypeParameters(TSTypeParameterDeclaration *typeParams)
{
    this->GetOrCreateHistoryNodeAs<TSTypeAliasDeclaration>()->typeParams_ = typeParams;
}

void TSTypeAliasDeclaration::SetId(Identifier *id)
{
    this->GetOrCreateHistoryNodeAs<TSTypeAliasDeclaration>()->id_ = id;
}

void TSTypeAliasDeclaration::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    auto const &decorators = Decorators();
    for (size_t ix = 0; ix < decorators.size(); ix++) {
        if (auto *transformedNode = cb(decorators[ix]); decorators[ix] != transformedNode) {
            decorators[ix]->SetTransformedNode(transformationName, transformedNode);
            SetValueDecorators(transformedNode->AsDecorator(), ix);
        }
    }

    auto const &annotations = Annotations();
    for (size_t ix = 0; ix < annotations.size(); ix++) {
        if (auto *transformedNode = cb(annotations[ix]); annotations[ix] != transformedNode) {
            annotations[ix]->SetTransformedNode(transformationName, transformedNode);
            SetValueAnnotations(transformedNode->AsAnnotationUsage(), ix);
        }
    }

    auto const id = Id();
    if (auto *transformedNode = cb(id); id != transformedNode) {
        id->SetTransformedNode(transformationName, transformedNode);
        SetId(transformedNode->AsIdentifier());
    }

    auto const typeParams = TypeParams();
    if (typeParams != nullptr) {
        if (auto *transformedNode = cb(typeParams); typeParams != transformedNode) {
            typeParams->SetTransformedNode(transformationName, transformedNode);
            SetTypeParameters(transformedNode->AsTSTypeParameterDeclaration());
        }
    }

    if (auto *typeAnnotation = TypeAnnotation(); typeAnnotation != nullptr) {
        if (auto *transformedNode = cb(typeAnnotation); typeAnnotation != transformedNode) {
            typeAnnotation->SetTransformedNode(transformationName, transformedNode);
            SetTsTypeAnnotation(static_cast<TypeNode *>(transformedNode));
        }
    }
}

void TSTypeAliasDeclaration::Iterate(const NodeTraverser &cb) const
{
    for (auto *it : VectorIterationGuard(Decorators())) {
        cb(it);
    }

    for (auto *it : Annotations()) {
        cb(it);
    }

    auto const id = GetHistoryNode()->AsTSTypeAliasDeclaration()->id_;
    cb(id);

    auto typeParams = GetHistoryNode()->AsTSTypeAliasDeclaration()->typeParams_;
    if (typeParams != nullptr) {
        cb(typeParams);
    }

    if (TypeAnnotation() != nullptr) {
        cb(TypeAnnotation());
    }
}

void TSTypeAliasDeclaration::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TSTypeAliasDeclaration"},
                 {"decorators", AstDumper::Optional(Decorators())},
                 {"annotations", AstDumper::Optional(Annotations())},
                 {"id", Id()},
                 {"typeAnnotation", AstDumper::Optional(TypeAnnotation())},
                 {"typeParameters", AstDumper::Optional(TypeParams())}});
}

bool TSTypeAliasDeclaration::RegisterUnexportedForDeclGen(ir::SrcDumper *dumper) const
{
    if (!dumper->IsDeclgen()) {
        return false;
    }

    if (dumper->IsIndirectDepPhase()) {
        return false;
    }

    if (id_->Parent()->IsExported() || id_->Parent()->IsDefaultExported()) {
        return false;
    }

    auto name = id_->Name().Mutf8();
    dumper->AddNode(name, this);
    return true;
}

void TSTypeAliasDeclaration::Dump(ir::SrcDumper *dumper) const
{
    ES2PANDA_ASSERT(id_);
    if (RegisterUnexportedForDeclGen(dumper)) {
        return;
    }
    for (auto *anno : Annotations()) {
        anno->Dump(dumper);
    }
    if (id_->Parent()->IsExported()) {
        dumper->Add("export ");
    }
    dumper->Add("type ");
    Id()->Dump(dumper);
    auto const typeParams = TypeParams();
    if (typeParams != nullptr) {
        dumper->Add("<");
        typeParams->Dump(dumper);
        dumper->Add(">");
    }
    dumper->Add(" = ");
    if (Id()->IsAnnotatedExpression()) {
        auto type = TypeAnnotation();
        ES2PANDA_ASSERT(type);
        type->Dump(dumper);
    }
    dumper->Add(";");
    dumper->Endl();
}

void TSTypeAliasDeclaration::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void TSTypeAliasDeclaration::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSTypeAliasDeclaration::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType TSTypeAliasDeclaration::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

TSTypeAliasDeclaration *TSTypeAliasDeclaration::Construct(ArenaAllocator *allocator)
{
    return allocator->New<TSTypeAliasDeclaration>(allocator, nullptr, nullptr, nullptr);
}

void TSTypeAliasDeclaration::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsTSTypeAliasDeclaration();

    otherImpl->decorators_ = decorators_;
    otherImpl->annotations_ = annotations_;
    otherImpl->id_ = id_;
    otherImpl->typeParams_ = typeParams_;
    otherImpl->typeParamTypes_ = typeParamTypes_;

    AnnotatedStatement::CopyTo(other);
}

void TSTypeAliasDeclaration::EmplaceDecorators(Decorator *decorators)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSTypeAliasDeclaration>();
    newNode->decorators_.emplace_back(decorators);
}

void TSTypeAliasDeclaration::ClearDecorators()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSTypeAliasDeclaration>();
    newNode->decorators_.clear();
}

void TSTypeAliasDeclaration::SetValueDecorators(Decorator *decorators, size_t index)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSTypeAliasDeclaration>();
    auto &arenaVector = newNode->decorators_;
    ES2PANDA_ASSERT(arenaVector.size() > index);
    arenaVector[index] = decorators;
}

[[nodiscard]] ArenaVector<Decorator *> &TSTypeAliasDeclaration::DecoratorsForUpdate()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSTypeAliasDeclaration>();
    return newNode->decorators_;
}

void TSTypeAliasDeclaration::EmplaceTypeParamterTypes(checker::Type *typeParamTypes)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSTypeAliasDeclaration>();
    newNode->typeParamTypes_.emplace_back(typeParamTypes);
}

void TSTypeAliasDeclaration::ClearTypeParamterTypes()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSTypeAliasDeclaration>();
    newNode->typeParamTypes_.clear();
}

void TSTypeAliasDeclaration::SetValueTypeParamterTypes(checker::Type *typeParamTypes, size_t index)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSTypeAliasDeclaration>();
    auto &arenaVector = newNode->typeParamTypes_;
    ES2PANDA_ASSERT(arenaVector.size() > index);
    arenaVector[index] = typeParamTypes;
}

[[nodiscard]] ArenaVector<checker::Type *> &TSTypeAliasDeclaration::TypeParamterTypesForUpdate()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSTypeAliasDeclaration>();
    return newNode->typeParamTypes_;
}

void TSTypeAliasDeclaration::EmplaceAnnotations(AnnotationUsage *annotations)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSTypeAliasDeclaration>();
    newNode->annotations_.emplace_back(annotations);
}

void TSTypeAliasDeclaration::ClearAnnotations()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSTypeAliasDeclaration>();
    newNode->annotations_.clear();
}

void TSTypeAliasDeclaration::SetValueAnnotations(AnnotationUsage *annotations, size_t index)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSTypeAliasDeclaration>();
    auto &arenaVector = newNode->annotations_;
    ES2PANDA_ASSERT(arenaVector.size() > index);
    arenaVector[index] = annotations;
}

[[nodiscard]] ArenaVector<AnnotationUsage *> &TSTypeAliasDeclaration::AnnotationsForUpdate()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSTypeAliasDeclaration>();
    return newNode->annotations_;
}

}  // namespace ark::es2panda::ir
