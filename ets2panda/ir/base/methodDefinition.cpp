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

#include "methodDefinition.h"

#include "checker/TSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "classDefinition.h"
#include "ir/ts/tsInterfaceBody.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::ir {

void MethodDefinition::SetDefaultAccessModifier(bool isDefault)
{
    this->GetOrCreateHistoryNodeAs<MethodDefinition>()->isDefault_ = isDefault;
}

void MethodDefinition::SetBaseOverloadMethod(MethodDefinition *const baseOverloadMethod)
{
    this->GetOrCreateHistoryNodeAs<MethodDefinition>()->baseOverloadMethod_ = baseOverloadMethod;
}

void MethodDefinition::SetAsyncPairMethod(MethodDefinition *const asyncPairMethod)
{
    this->GetOrCreateHistoryNodeAs<MethodDefinition>()->asyncPairMethod_ = asyncPairMethod;
}

ScriptFunction *MethodDefinition::Function()
{
    auto const value = Value();
    return value->IsFunctionExpression() ? value->AsFunctionExpression()->Function() : nullptr;
}

const ScriptFunction *MethodDefinition::Function() const
{
    auto const value = Value();
    return value->IsFunctionExpression() ? value->AsFunctionExpression()->Function() : nullptr;
}

PrivateFieldKind MethodDefinition::ToPrivateFieldKind(bool const isStatic) const
{
    switch (Kind()) {
        case MethodDefinitionKind::METHOD: {
            return isStatic ? PrivateFieldKind::STATIC_METHOD : PrivateFieldKind::METHOD;
        }
        case MethodDefinitionKind::GET: {
            return isStatic ? PrivateFieldKind::STATIC_GET : PrivateFieldKind::GET;
        }
        case MethodDefinitionKind::SET: {
            return isStatic ? PrivateFieldKind::STATIC_SET : PrivateFieldKind::SET;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

void MethodDefinition::ResolveReferences(const NodeTraverser &cb) const
{
    auto key = GetHistoryNode()->AsMethodDefinition()->key_;
    auto value = GetHistoryNode()->AsMethodDefinition()->value_;
    cb(key);
    cb(value);

    for (auto *it : VectorIterationGuard(Overloads())) {
        cb(it);
    }
}

void MethodDefinition::Iterate(const NodeTraverser &cb) const
{
    auto key = GetHistoryNode()->AsMethodDefinition()->key_;
    auto value = GetHistoryNode()->AsMethodDefinition()->value_;
    cb(key);
    cb(value);

    for (auto *it : Overloads()) {
        if (it->Parent() == this) {
            cb(it);
        }
    }
}

void MethodDefinition::TransformChildren(const NodeTransformer &cb, std::string_view const transformationName)
{
    auto *key = Key();
    if (auto *transformedNode = cb(key); key != transformedNode) {
        key->SetTransformedNode(transformationName, transformedNode);
        SetKey(transformedNode->AsExpression());
    }

    auto *value = Value();
    if (auto *transformedNode = cb(value); value != transformedNode) {
        value->SetTransformedNode(transformationName, transformedNode);
        SetValue(transformedNode->AsExpression());
    }

    auto const &overloads = Overloads();
    for (size_t ix = 0; ix < overloads.size(); ix++) {
        if (auto *transformedNode = cb(overloads[ix]); overloads[ix] != transformedNode) {
            overloads[ix]->SetTransformedNode(transformationName, transformedNode);
            SetValueOverloads(transformedNode->AsMethodDefinition(), ix);
        }
    }
}

void MethodDefinition::Dump(ir::AstDumper *dumper) const
{
    const char *kind = nullptr;

    switch (Kind()) {
        case MethodDefinitionKind::CONSTRUCTOR: {
            kind = "constructor";
            break;
        }
        case MethodDefinitionKind::METHOD: {
            kind = "method";
            break;
        }
        case MethodDefinitionKind::EXTENSION_METHOD: {
            kind = "extensionmethod";
            break;
        }
        case MethodDefinitionKind::GET: {
            kind = "get";
            break;
        }
        case MethodDefinitionKind::SET: {
            kind = "set";
            break;
        }
        case MethodDefinitionKind::EXTENSION_GET: {
            kind = "extensionget";
            break;
        }
        case MethodDefinitionKind::EXTENSION_SET: {
            kind = "extensionset";
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    dumper->Add({{"type", "MethodDefinition"},
                 {"key", Key()},
                 {"kind", kind},
                 {"accessibility", AstDumper::Optional(AstDumper::ModifierToString(Modifiers()))},
                 {"static", IsStatic()},
                 {"optional", IsOptionalDeclaration()},
                 {"computed", IsComputed()},
                 {"value", Value()},
                 {"overloads", Overloads()}});
}

static void DumpModifierPrefix(const ir::MethodDefinition *m, ir::SrcDumper *dumper)
{
    ES2PANDA_ASSERT(!compiler::HasGlobalClassParent(m));
    if (m->IsStatic()) {
        dumper->Add("static ");
    }

    if (m->IsAbstract() &&
        !(m->Parent()->IsTSInterfaceBody() ||
          (m->BaseOverloadMethod() != nullptr && m->BaseOverloadMethod()->Parent()->IsTSInterfaceBody()))) {
        dumper->Add("abstract ");
    }
    if (m->IsFinal()) {
        dumper->Add("final ");
    }
    if (m->IsNative()) {
        dumper->Add("native ");
    }
    if (m->IsAsync() && !dumper->IsDeclgen()) {
        dumper->Add("async ");
    }
    if (m->IsOverride()) {
        dumper->Add("override ");
    }

    if (m->IsGetter()) {
        dumper->Add("get ");
    } else if (m->IsSetter()) {
        dumper->Add("set ");
    }
}

static bool IsNamespaceTransformed(const MethodDefinition *method)
{
    auto *parent = method->Parent();
    if (parent->IsMethodDefinition()) {
        // handle overloads
        parent = parent->Parent();
    }
    return parent->IsClassDefinition() && parent->AsClassDefinition()->IsNamespaceTransformed();
}

static void DumpAccessorPrefix(const ir::MethodDefinition *m, ir::SrcDumper *dumper)
{
    //  special processing for overloads
    auto const *parent = m->Parent();
    if (parent != nullptr && parent->IsMethodDefinition()) {
        parent = parent->Parent();
    }

    if (parent == nullptr) {
        return;
    }

    if (parent->IsClassDefinition() && !parent->AsClassDefinition()->IsLocal()) {
        if (m->IsPrivate()) {
            dumper->Add("private ");
        } else if (m->IsProtected()) {
            dumper->Add("protected ");
        } else if (!dumper->IsDeclgen()) {
            dumper->Add("public ");
        }
        return;
    }

    if (dumper->IsDeclgen() && parent->IsTSInterfaceBody()) {
        if (m->Value() != nullptr && m->Value()->IsFunctionExpression() &&
            m->Value()->AsFunctionExpression()->Function() != nullptr &&
            m->Value()->AsFunctionExpression()->Function()->HasBody() && !m->IsGetter() && !m->IsSetter()) {
            // Setter and Getter don't have 'default' modifier according to the language spec.
            dumper->Add("default ");
        }
    }
}

static void DumpPrefix(const ir::MethodDefinition *m, ir::SrcDumper *dumper)
{
    bool global = compiler::HasGlobalClassParent(m);
    if (global || IsNamespaceTransformed(m)) {
        if (m->IsExported()) {
            dumper->Add("export ");
        }
        if (m->IsDefaultExported()) {
            dumper->Add("export default ");
            dumper->SetDefaultExport();
        }
        if (dumper->IsDeclgen()) {
            if (global) {
                dumper->Add("declare ");
            } else {
                dumper->GetDeclgen()->TryDeclareAmbientContext(dumper);
            }
        }
        if (m->IsGetter()) {
            dumper->Add("get ");
        } else if (m->IsSetter()) {
            dumper->Add("set ");
        } else {
            dumper->Add("function ");
        }
        return;
    }

    DumpAccessorPrefix(m, dumper);
    DumpModifierPrefix(m, dumper);
}

bool MethodDefinition::FilterForDeclGen() const
{
    if (key_ == nullptr) {
        return false;
    }

    if (Function()->IsSynthetic()) {
        return true;
    }

    if (compiler::HasGlobalClassParent(this) && !key_->Parent()->IsExported() && !key_->Parent()->IsDefaultExported()) {
        return true;
    }

    ES2PANDA_ASSERT(Id() != nullptr);
    auto const name = Id()->Name().Utf8();
    if (name.find("%%async") != std::string_view::npos || name == compiler::Signatures::INITIALIZER_BLOCK_INIT ||
        name == compiler::Signatures::INIT_METHOD || name == compiler::Signatures::CCTOR) {
        return true;
    }

    if (name.rfind('#', 0U) == 0U) {
        return true;
    }

    return false;
}

static void DumpSingleOverload(const ir::MethodDefinition *m, ir::SrcDumper *dumper)
{
    if (compiler::HasGlobalClassParent(m) && m->Id() != nullptr &&
        m->Id()->Name().Is(compiler::Signatures::INIT_METHOD)) {
        m->Function()->Body()->Dump(dumper);
        return;
    }

    auto value = m->Value();
    if (value->AsFunctionExpression()->Function()->HasAnnotations()) {
        for (auto *anno : value->AsFunctionExpression()->Function()->Annotations()) {
            // NOTE(zhelyapov): workaround, see #26031
            if (anno->GetBaseName()->Name() != compiler::Signatures::DEFAULT_ANNO_FOR_FUNC) {
                anno->Dump(dumper);
            }
        }
    }

    DumpPrefix(m, dumper);

    if (m->IsConstructor() &&
        !(m->Key()->IsIdentifier() && m->Key()->AsIdentifier()->Name().Is(compiler::Signatures::CONSTRUCTOR_NAME))) {
        dumper->Add(std::string(compiler::Signatures::CONSTRUCTOR_NAME) + " ");
    }

    auto key = m->Key();
    if (key != nullptr) {
        key->Dump(dumper);
    }

    if (value != nullptr) {
        value->Dump(dumper);
    }
}

void MethodDefinition::Dump(ir::SrcDumper *dumper) const
{
    if (dumper->IsDeclgen()) {
        if (FilterForDeclGen()) {
            return;
        }
        if (Parent() != nullptr && (IsGetter() || IsSetter()) && IsOptionalDeclaration() &&
            Parent()->IsTSInterfaceBody() && OriginalNode() != nullptr && OriginalNode()->IsClassProperty()) {
            OriginalNode()->AsClassProperty()->ForceDump(dumper);
            dumper->Endl();
            return;
        }
    }

    dumper->DumpJsdocBeforeTargetNode(this);

    if (!dumper->IsDeclgen() || !IsPrivate()) {
        DumpSingleOverload(this, dumper);
    }

    for (auto method : Overloads()) {
        method->Dump(dumper);
    }
}

void MethodDefinition::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void MethodDefinition::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *MethodDefinition::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType MethodDefinition::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

MethodDefinition *MethodDefinition::Clone(ArenaAllocator *const allocator, AstNode *const parent)
{
    auto *const key = Key()->Clone(allocator, nullptr)->AsExpression();
    auto *const value = Value()->Clone(allocator, nullptr)->AsExpression();
    auto *const clone = allocator->New<MethodDefinition>(Kind(), key, value, Modifiers(), allocator, IsComputed());

    if (parent != nullptr) {
        clone->SetParent(parent);
    }

    key->SetParent(clone);
    value->SetParent(clone);

    clone->SetBaseOverloadMethod(BaseOverloadMethod());

    for (auto *const overloads : Overloads()) {
        clone->AddOverload(overloads->Clone(allocator, clone));
    }

    return clone;
}

void MethodDefinition::InitializeOverloadInfo()
{
    ES2PANDA_ASSERT(this->Function() != nullptr);

    SetOverloadInfo({this->Function()->Signature()->MinArgCount(), this->Function()->Signature()->ArgCount(), false,
                     this->IsDeclare(), (this->Function()->Signature()->RestVar() != nullptr),
                     this->Function()->Signature()->ReturnType()->IsETSVoidType()});
}

void MethodDefinition::ResetOverloads()
{
    auto baseOverloadMethod = BaseOverloadMethod();
    SetBaseOverloadMethod(nullptr);
    for (auto *overload : Overloads()) {
        overload->CleanUp();
    }
    ClearOverloads();

    if ((Function() == nullptr) || !Function()->IsOverload()) {
        return;
    }

    Function()->ClearFlag(ir::ScriptFunctionFlags::OVERLOAD);
    /*
     * if this method and it's baseOverloadMethod are in two different files,
     * no need to move it to the body of baseOverloadMethod's contianing class in cleanup.
     */
    if (GetTopStatement() != baseOverloadMethod->GetTopStatement()) {
        return;
    }

    auto parent = baseOverloadMethod->Parent();
    ES2PANDA_ASSERT(parent->IsClassDefinition() || parent->IsTSInterfaceBody());
    auto &body =
        parent->IsClassDefinition() ? parent->AsClassDefinition()->Body() : parent->AsTSInterfaceBody()->Body();

    for (auto *elem : body) {
        if (elem == this) {
            return;
        }
    }

    parent->IsClassDefinition() ? parent->AsClassDefinition()->EmplaceBody(this)
                                : parent->AsTSInterfaceBody()->Body().push_back(this);
}

void MethodDefinition::CleanUp()
{
    AstNode::CleanUp();
    ResetOverloads();
}

MethodDefinition *MethodDefinition::Construct(ArenaAllocator *allocator)
{
    return allocator->New<MethodDefinition>(MethodDefinitionKind::NONE, nullptr, nullptr, ModifierFlags::NONE,
                                            allocator, false);
}

void MethodDefinition::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsMethodDefinition();

    otherImpl->isDefault_ = isDefault_;
    otherImpl->kind_ = kind_;
    otherImpl->overloads_ = overloads_;
    otherImpl->baseOverloadMethod_ = baseOverloadMethod_;
    otherImpl->asyncPairMethod_ = asyncPairMethod_;
    otherImpl->overloadInfo_ = overloadInfo_;

    ClassElement::CopyTo(other);
}

void MethodDefinition::EmplaceOverloads(MethodDefinition *overloads)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<MethodDefinition>();
    newNode->overloads_.emplace_back(overloads);
}

void MethodDefinition::ClearOverloads()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<MethodDefinition>();
    newNode->overloads_.clear();
}

void MethodDefinition::SetValueOverloads(MethodDefinition *overloads, size_t index)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<MethodDefinition>();
    auto &arenaVector = newNode->overloads_;
    ES2PANDA_ASSERT(arenaVector.size() > index);
    arenaVector[index] = overloads;
}

[[nodiscard]] ArenaVector<MethodDefinition *> &MethodDefinition::OverloadsForUpdate()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<MethodDefinition>();
    return newNode->overloads_;
}

}  // namespace ark::es2panda::ir
