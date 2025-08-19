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

#include "tsInterfaceDeclaration.h"

#include "util/es2pandaMacros.h"
#include "utils/arena_containers.h"
#include "varbinder/declaration.h"
#include "varbinder/variable.h"
#include "checker/TSchecker.h"
#include "checker/ETSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"

#include "ir/expressions/identifier.h"
#include "ir/ts/tsInterfaceBody.h"
#include "ir/ts/tsInterfaceHeritage.h"
#include "ir/ts/tsTypeParameter.h"
#include "ir/ts/tsTypeParameterDeclaration.h"
#include "util/language.h"

namespace ark::es2panda::ir {

void TSInterfaceDeclaration::SetInternalName(util::StringView internalName)
{
    this->GetOrCreateHistoryNodeAs<TSInterfaceDeclaration>()->internalName_ = internalName;
}

void TSInterfaceDeclaration::SetAnonClass(ClassDeclaration *anonClass)
{
    this->GetOrCreateHistoryNodeAs<TSInterfaceDeclaration>()->anonClass_ = anonClass;
}

void TSInterfaceDeclaration::SetId(Identifier *id)
{
    this->GetOrCreateHistoryNodeAs<TSInterfaceDeclaration>()->id_ = id;
}

void TSInterfaceDeclaration::SetTypeParams(TSTypeParameterDeclaration *typeParams)
{
    this->GetOrCreateHistoryNodeAs<TSInterfaceDeclaration>()->typeParams_ = typeParams;
}

void TSInterfaceDeclaration::SetBody(TSInterfaceBody *body)
{
    this->GetOrCreateHistoryNodeAs<TSInterfaceDeclaration>()->body_ = body;
}

void TSInterfaceDeclaration::EmplaceExtends(TSInterfaceHeritage *extends)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSInterfaceDeclaration>();
    newNode->extends_.emplace_back(extends);
}

void TSInterfaceDeclaration::ClearExtends()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSInterfaceDeclaration>();
    newNode->extends_.clear();
}

void TSInterfaceDeclaration::SetValueExtends(TSInterfaceHeritage *extends, size_t index)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSInterfaceDeclaration>();
    auto &arenaVector = newNode->extends_;
    arenaVector[index] = extends;
}

[[nodiscard]] const ArenaVector<TSInterfaceHeritage *> &TSInterfaceDeclaration::Extends()
{
    auto newNode = this->GetHistoryNodeAs<TSInterfaceDeclaration>();
    return newNode->extends_;
}

[[nodiscard]] ArenaVector<TSInterfaceHeritage *> &TSInterfaceDeclaration::ExtendsForUpdate()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<TSInterfaceDeclaration>();
    return newNode->extends_;
}

void TSInterfaceDeclaration::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    TransformAnnotations(cb, transformationName);

    auto const id = Id();
    if (auto *transformedNode = cb(id); id != transformedNode) {
        id->SetTransformedNode(transformationName, transformedNode);
        SetId(transformedNode->AsIdentifier());
    }

    auto const typeParams = TypeParams();
    if (typeParams != nullptr) {
        if (auto *transformedNode = cb(typeParams); typeParams != transformedNode) {
            typeParams->SetTransformedNode(transformationName, transformedNode);
            SetTypeParams(transformedNode->AsTSTypeParameterDeclaration());
        }
    }

    auto const &extends = Extends();
    for (size_t ix = 0; ix < extends.size(); ix++) {
        if (auto *transformedNode = cb(extends[ix]); extends[ix] != transformedNode) {
            extends[ix]->SetTransformedNode(transformationName, transformedNode);
            SetValueExtends(transformedNode->AsTSInterfaceHeritage(), ix);
        }
    }

    auto const &body = Body();
    if (auto *transformedNode = cb(body); body != transformedNode) {
        body->SetTransformedNode(transformationName, transformedNode);
        SetBody(transformedNode->AsTSInterfaceBody());
    }
}

void TSInterfaceDeclaration::Iterate(const NodeTraverser &cb) const
{
    IterateAnnotations(cb);

    auto const id = GetHistoryNode()->AsTSInterfaceDeclaration()->id_;
    cb(id);

    auto const typeParams = GetHistoryNode()->AsTSInterfaceDeclaration()->typeParams_;
    if (typeParams != nullptr) {
        cb(typeParams);
    }

    for (auto *it : VectorIterationGuard(Extends())) {
        cb(it);
    }

    auto const body = GetHistoryNode()->AsTSInterfaceDeclaration()->body_;
    cb(body);
}

void TSInterfaceDeclaration::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "TSInterfaceDeclaration"},
                 {"annotations", AstDumper::Optional(Annotations())},
                 {"body", Body()},
                 {"id", Id()},
                 {"extends", Extends()},
                 {"typeParameters", AstDumper::Optional(TypeParams())}});
}

bool TSInterfaceDeclaration::RegisterUnexportedForDeclGen(ir::SrcDumper *dumper) const
{
    if (!dumper->IsDeclgen()) {
        return false;
    }

    if (dumper->IsIndirectDepPhase()) {
        return false;
    }

    if (id_->Parent()->IsDefaultExported() || id_->Parent()->IsExported()) {
        return false;
    }

    auto name = id_->Name().Mutf8();
    dumper->AddNode(name, this);
    return true;
}

void TSInterfaceDeclaration::Dump(ir::SrcDumper *dumper) const
{
    ES2PANDA_ASSERT(id_);
    if (!id_->Parent()->IsDefaultExported() && !id_->Parent()->IsExported() && dumper->IsDeclgen() &&
        !dumper->IsIndirectDepPhase()) {
        auto name = id_->Name().Mutf8();
        dumper->AddNode(name, this);
        return;
    }
    DumpAnnotations(dumper);
    if (id_->Parent()->IsExported()) {
        dumper->Add("export ");
    } else if (id_->Parent()->IsDefaultExported()) {
        dumper->Add("export default ");
    }
    if (IsDeclare() || dumper->IsDeclgen()) {
        dumper->Add("declare ");
    }
    dumper->Add("interface ");
    Id()->Dump(dumper);

    auto const typeParams = TypeParams();
    if (typeParams != nullptr) {
        dumper->Add("<");
        typeParams->Dump(dumper);
        dumper->Add(">");
    }

    auto const extends = Extends();
    if (!extends.empty()) {
        dumper->Add(" extends ");
        for (auto ext : extends) {
            ext->Dump(dumper);
            if (ext != extends.back()) {
                dumper->Add(", ");
            }
        }
    }

    auto body = Body();
    dumper->Add(" {");
    if (body != nullptr) {
        dumper->IncrIndent();
        dumper->Endl();
        body->Dump(dumper);
        dumper->DecrIndent();
        dumper->Endl();
    }
    dumper->Add("}");
    dumper->Endl();
}

void TSInterfaceDeclaration::Compile([[maybe_unused]] compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void TSInterfaceDeclaration::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *TSInterfaceDeclaration::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType TSInterfaceDeclaration::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

TSInterfaceDeclaration *TSInterfaceDeclaration::Construct(ArenaAllocator *allocator)
{
    ArenaVector<TSInterfaceHeritage *> extends(allocator->Adapter());
    return allocator->New<TSInterfaceDeclaration>(
        allocator, std::move(extends), ConstructorData {nullptr, nullptr, nullptr, false, false, Language::Id::COUNT});
}

void TSInterfaceDeclaration::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsTSInterfaceDeclaration();

    otherImpl->scope_ = scope_;
    otherImpl->id_ = id_;
    otherImpl->typeParams_ = typeParams_;
    otherImpl->body_ = body_;
    otherImpl->extends_ = extends_;
    otherImpl->internalName_ = internalName_;
    otherImpl->isStatic_ = isStatic_;
    otherImpl->isExternal_ = isExternal_;
    otherImpl->lang_ = lang_;
    otherImpl->anonClass_ = anonClass_;

    AnnotationAllowed<TypedStatement>::CopyTo(other);
}

}  // namespace ark::es2panda::ir
