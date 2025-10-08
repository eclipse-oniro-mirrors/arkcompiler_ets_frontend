/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "overloadDeclaration.h"
#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::ir {
PrivateFieldKind OverloadDeclaration::ToPrivateFieldKind(bool const isStatic) const
{
    return isStatic ? PrivateFieldKind::STATIC_OVERLOAD : PrivateFieldKind::OVERLOAD;
}

void OverloadDeclaration::ResolveReferences(const NodeTraverser &cb) const
{
    cb(key_);

    for (auto *it : VectorIterationGuard(overloadedList_)) {
        cb(it);
    }
}

void OverloadDeclaration::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    cb(key_);
    for (auto *&it : VectorIterationGuard(overloadedList_)) {
        if (auto *transformedNode = cb(it); it != transformedNode) {
            it->SetTransformedNode(transformationName, transformedNode);
            it = transformedNode->AsIdentifier();
        }
    }
}

void OverloadDeclaration::Iterate(const NodeTraverser &cb) const
{
    cb(key_);
    for (auto *it : VectorIterationGuard(overloadedList_)) {
        cb(it);
    }
}

void OverloadDeclaration::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "OverloadDeclaration"},
                 {"key", key_},
                 {"optional", AstDumper::Optional(AstDumper::ModifierToString(Modifiers()))},
                 {"static", IsStatic()},
                 {"overloadedList", overloadedList_}});
}

void OverloadDeclaration::DumpModifier(ir::SrcDumper *dumper) const
{
    if (compiler::HasGlobalClassParent(this) || Parent()->IsETSModule()) {
        if (IsExported()) {
            dumper->Add("export ");
        } else if (IsDefaultExported()) {
            dumper->Add("export default ");
        }
    }

    if (dumper->IsDeclgen()) {
        dumper->GetDeclgen()->TryDeclareAmbientContext(dumper);
    } else if (IsDeclare() && (compiler::HasGlobalClassParent(this) || Parent()->IsETSModule())) {
        dumper->Add("declare ");
    }

    if (Parent() != nullptr && Parent()->IsClassDefinition() && !Parent()->AsClassDefinition()->IsLocal() &&
        !compiler::HasGlobalClassParent(this)) {
        if (IsPrivate()) {
            dumper->Add("private ");
        } else if (IsProtected()) {
            dumper->Add("protected ");
        } else {
            dumper->Add("public ");
        }

        if (IsStatic()) {
            dumper->Add("static ");
        }
    }

    if (IsAsync()) {
        dumper->Add("async ");
    }
}

void OverloadDeclaration::Dump(ir::SrcDumper *dumper) const
{
    DumpModifier(dumper);
    dumper->Add("overload ");
    dumper->Add(IsConstructor() ? "constructor " : key_->AsIdentifier()->Name().Mutf8());
    dumper->Add("{ ");
    for (size_t i = 0; i < overloadedList_.size(); i++) {
        if (i != 0) {
            dumper->Add(", ");
        }
        overloadedList_[i]->Dump(dumper);
    }
    dumper->Add(" };\n");
}

void OverloadDeclaration::Compile([[maybe_unused]] compiler::PandaGen *pg) const {}

checker::Type *OverloadDeclaration::Check([[maybe_unused]] checker::TSChecker *checker)
{
    return nullptr;
}

checker::VerifiedType OverloadDeclaration::Check([[maybe_unused]] checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

OverloadDeclaration *OverloadDeclaration::Construct(ArenaAllocator *allocator)
{
    return allocator->New<OverloadDeclaration>(nullptr, ModifierFlags::NONE, allocator);
}

void OverloadDeclaration::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsOverloadDeclaration();

    otherImpl->overloadFlags_ = overloadFlags_;
    otherImpl->overloadedList_ = overloadedList_;
    ClassElement::CopyTo(other);
}

}  // namespace ark::es2panda::ir
