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

#include "classProperty.h"

#include "checker/ETSchecker.h"
#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::ir {

void ClassProperty::SetTypeAnnotation(TypeNode *typeAnnotation)
{
    this->GetOrCreateHistoryNodeAs<ClassProperty>()->typeAnnotation_ = typeAnnotation;
}

void ClassProperty::SetDefaultAccessModifier(bool isDefault)
{
    this->GetOrCreateHistoryNodeAs<ClassProperty>()->isDefault_ = isDefault;
}

void ClassProperty::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    auto *key = Key();
    if (key != nullptr) {
        if (auto *transformedNode = cb(key); key != transformedNode) {
            key->SetTransformedNode(transformationName, transformedNode);
            SetKey(transformedNode->AsExpression());
        }
    }

    auto *value = Value();
    if (value != nullptr) {
        if (auto *transformedNode = cb(value); value != transformedNode) {
            value->SetTransformedNode(transformationName, transformedNode);
            SetValue(transformedNode->AsExpression());
        }
    }

    auto *typeAnnotation = TypeAnnotation();
    if (typeAnnotation != nullptr) {
        if (auto *transformedNode = cb(typeAnnotation); typeAnnotation != transformedNode) {
            typeAnnotation->SetTransformedNode(transformationName, transformedNode);
            SetTypeAnnotation(static_cast<TypeNode *>(transformedNode));
        }
    }

    TransformAnnotations(cb, transformationName);
}

void ClassProperty::Iterate(const NodeTraverser &cb) const
{
    auto const key = GetHistoryNode()->AsClassProperty()->key_;
    cb(key);

    auto const value = GetHistoryNode()->AsClassProperty()->value_;
    if (value != nullptr) {
        cb(value);
    }

    if (TypeAnnotation() != nullptr) {
        cb(TypeAnnotation());
    }

    IterateAnnotations(cb);
}

void ClassProperty::Dump(ir::AstDumper *dumper) const
{
    dumper->Add({{"type", "ClassProperty"},
                 {"key", Key()},
                 {"value", AstDumper::Optional(Value())},
                 {"accessibility", AstDumper::Optional(AstDumper::ModifierToString(Modifiers()))},
                 {"static", IsStatic()},
                 {"readonly", IsReadonly()},
                 {"declare", IsDeclare()},
                 {"optional", IsOptionalDeclaration()},
                 {"computed", IsComputed()},
                 {"typeAnnotation", AstDumper::Optional(TypeAnnotation())},
                 {"definite", IsDefinite()},
                 {"annotations", AstDumper::Optional(Annotations())}});
}

void ClassProperty::DumpModifiers(ir::SrcDumper *dumper) const
{
    ES2PANDA_ASSERT(key_);
    if (dumper->IsDeclgen()) {
        if (key_->Parent()->IsExported()) {
            dumper->Add("export declare ");
        } else if (key_->Parent()->IsDefaultExported()) {
            dumper->Add("export default declare ");
        }
    }

    if (compiler::HasGlobalClassParent(this)) {
        if (key_->Parent()->IsConst()) {
            dumper->Add("const ");
        } else {
            dumper->Add("let ");
        }
        return;
    }

    if (Parent() != nullptr && Parent()->IsClassDefinition() && !Parent()->AsClassDefinition()->IsLocal()) {
        if (IsPrivate()) {
            dumper->Add("private ");
        } else if (IsProtected()) {
            dumper->Add("protected ");
        } else if (IsInternal()) {
            dumper->Add("internal ");
        } else {
            dumper->Add("public ");
        }
    }

    if (IsStatic()) {
        dumper->Add("static ");
    }

    if (IsReadonly()) {
        dumper->Add("readonly ");
    }
}

bool ClassProperty::DumpNamespaceForDeclGen(ir::SrcDumper *dumper) const
{
    if (!dumper->IsDeclgen()) {
        return false;
    }

    if (Parent() == nullptr) {
        return false;
    }

    bool isNamespaceTransformed =
        Parent()->IsClassDefinition() && Parent()->AsClassDefinition()->IsNamespaceTransformed();
    if (isNamespaceTransformed) {
        dumper->Add("let ");
        return true;
    }
    return false;
}

void ClassProperty::DumpPrefix(ir::SrcDumper *dumper) const
{
    DumpAnnotations(dumper);
    if (DumpNamespaceForDeclGen(dumper)) {
        return;
    }
    DumpModifiers(dumper);
}

void ClassProperty::DumpCheckerTypeForDeclGen(ir::SrcDumper *dumper) const
{
    if (!dumper->IsDeclgen()) {
        return;
    }

    if (TsType() == nullptr) {
        return;
    }

    auto typeStr = TsType()->ToString();
    dumper->Add(": ");
    dumper->Add(typeStr);

    dumper->PushTask([dumper, typeStr] { dumper->DumpNode(typeStr); });
}

bool ClassProperty::RegisterUnexportedForDeclGen(ir::SrcDumper *dumper) const
{
    ES2PANDA_ASSERT(key_);
    if (!dumper->IsDeclgen()) {
        return false;
    }

    auto name = key_->AsIdentifier()->Name().Mutf8();
    if (name.rfind('#', 0) == 0) {
        return true;
    }

    if (IsPrivate()) {
        return true;
    }

    if (!compiler::HasGlobalClassParent(this)) {
        return false;
    }

    if (dumper->IsIndirectDepPhase()) {
        return false;
    }

    if (key_->Parent()->IsExported() || key_->Parent()->IsDefaultExported()) {
        return false;
    }

    dumper->AddNode(name, this);
    return true;
}

void ClassProperty::Dump(ir::SrcDumper *dumper) const
{
    if (RegisterUnexportedForDeclGen(dumper)) {
        return;
    }
    DumpPrefix(dumper);

    if (Key() != nullptr) {
        Key()->Dump(dumper);
    }

    if (IsOptionalDeclaration()) {
        dumper->Add("?");
    }

    if (IsDefinite()) {
        dumper->Add("!");
    }

    if (typeAnnotation_ != nullptr && !dumper->IsDeclgen()) {
        dumper->Add(": ");
        TypeAnnotation()->Dump(dumper);
    }

    DumpCheckerTypeForDeclGen(dumper);

    if (value_ != nullptr && !dumper->IsDeclgen()) {
        dumper->Add(" = ");
        Value()->Dump(dumper);
    }

    dumper->Add(";");
    dumper->Endl();
}

void ClassProperty::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ClassProperty::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ClassProperty::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ClassProperty::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

ClassProperty *ClassProperty::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const key = Key()->Clone(allocator, nullptr)->AsExpression();
    auto *const value = Value() != nullptr ? Value()->Clone(allocator, nullptr)->AsExpression() : nullptr;
    auto *const typeAnnotation = TypeAnnotation() != nullptr ? TypeAnnotation()->Clone(allocator, nullptr) : nullptr;

    auto *const clone = allocator->New<ClassProperty>(key, value, typeAnnotation, Modifiers(), allocator, IsComputed());

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    key->SetParent(clone);
    if (value != nullptr) {
        value->SetTsType(Value()->TsType());
        value->SetParent(clone);
    }
    if (typeAnnotation != nullptr) {
        typeAnnotation->SetTsType(typeAnnotation->TsType());
        typeAnnotation->SetParent(clone);
    }

    if (HasAnnotations()) {
        clone->SetAnnotations(Annotations());
    }

    clone->SetRange(Range());

    return clone;
}

ClassProperty *ClassProperty::Construct(ArenaAllocator *allocator)
{
    return allocator->New<ClassProperty>(nullptr, nullptr, nullptr, ModifierFlags::NONE, allocator, false);
}

void ClassProperty::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsClassProperty();

    otherImpl->typeAnnotation_ = typeAnnotation_;
    otherImpl->isDefault_ = isDefault_;
    otherImpl->initMode_ = initMode_;

    AnnotationAllowed<ClassElement>::CopyTo(other);
}

}  // namespace ark::es2panda::ir
