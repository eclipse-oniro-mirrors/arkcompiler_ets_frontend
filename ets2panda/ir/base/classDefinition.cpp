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

#include "classDefinition.h"

#include "checker/TSchecker.h"
#include "checker/ETSchecker.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/pandagen.h"
#include "ir/astDump.h"
#include "ir/srcDump.h"
#include "ir/base/classStaticBlock.h"
#include "ir/base/methodDefinition.h"
#include "ir/base/scriptFunction.h"
#include "ir/expressions/identifier.h"
#include "ir/ts/tsClassImplements.h"

namespace ark::es2panda::ir {

void ClassDefinition::SetCtor(MethodDefinition *ctor)
{
    this->GetOrCreateHistoryNodeAs<ClassDefinition>()->ctor_ = ctor;
}

void ClassDefinition::SetTypeParams(TSTypeParameterDeclaration *typeParams)
{
    this->GetOrCreateHistoryNodeAs<ClassDefinition>()->typeParams_ = typeParams;
}

void ClassDefinition::SetOrigEnumDecl(TSEnumDeclaration *origEnumDecl)
{
    this->GetOrCreateHistoryNodeAs<ClassDefinition>()->origEnumDecl_ = origEnumDecl;
}

void ClassDefinition::SetAnonClass(ClassDeclaration *anonClass)
{
    this->GetOrCreateHistoryNodeAs<ClassDefinition>()->anonClass_ = anonClass;
}

void ClassDefinition::SetSuperClass(Expression *superClass)
{
    this->GetOrCreateHistoryNodeAs<ClassDefinition>()->superClass_ = superClass;
}

void ClassDefinition::SetSuperTypeParams(TSTypeParameterInstantiation *superTypeParams)
{
    this->GetOrCreateHistoryNodeAs<ClassDefinition>()->superTypeParams_ = superTypeParams;
}

void ClassDefinition::SetScope(varbinder::LocalScope *scope)
{
    this->GetOrCreateHistoryNodeAs<ClassDefinition>()->scope_ = scope;
}

void ClassDefinition::SetModifiers(ClassDefinitionModifiers modifiers)
{
    this->GetOrCreateHistoryNodeAs<ClassDefinition>()->modifiers_ = modifiers;
}

void ClassDefinition::SetInternalName(util::StringView internalName)
{
    this->GetOrCreateHistoryNodeAs<ClassDefinition>()->internalName_ = internalName;
}

void ClassDefinition::EmplaceBody(AstNode *body)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ClassDefinition>();
    newNode->body_.emplace_back(body);
}

void ClassDefinition::ClearBody()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ClassDefinition>();
    newNode->body_.clear();
}

void ClassDefinition::SetValueBody(AstNode *body, size_t index)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ClassDefinition>();
    auto &arenaVector = newNode->body_;
    ES2PANDA_ASSERT(arenaVector.size() > index);
    arenaVector[index] = body;
}

[[nodiscard]] const ArenaVector<AstNode *> &ClassDefinition::Body()
{
    auto newNode = this->GetHistoryNodeAs<ClassDefinition>();
    return newNode->body_;
}

[[nodiscard]] ArenaVector<AstNode *> &ClassDefinition::BodyForUpdate()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ClassDefinition>();
    return newNode->body_;
}

void ClassDefinition::EmplaceImplements(TSClassImplements *implements)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ClassDefinition>();
    newNode->implements_.emplace_back(implements);
}

void ClassDefinition::ClearImplements()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ClassDefinition>();
    newNode->implements_.clear();
}

void ClassDefinition::SetValueImplements(TSClassImplements *implements, size_t index)
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ClassDefinition>();
    auto &arenaVector = newNode->implements_;
    ES2PANDA_ASSERT(arenaVector.size() > index);
    arenaVector[index] = implements;
}

[[nodiscard]] const ArenaVector<TSClassImplements *> &ClassDefinition::Implements()
{
    auto newNode = this->GetHistoryNodeAs<ClassDefinition>();
    return newNode->implements_;
}

[[nodiscard]] ArenaVector<TSClassImplements *> &ClassDefinition::ImplementsForUpdate()
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ClassDefinition>();
    return newNode->implements_;
}

const FunctionExpression *ClassDefinition::Ctor() const
{
    auto const newNode = GetHistoryNode()->AsClassDefinition();
    return newNode->ctor_ != nullptr ? newNode->ctor_->Value()->AsFunctionExpression() : nullptr;
}

bool ClassDefinition::HasPrivateMethod() const
{
    auto const body = Body();
    return std::any_of(body.cbegin(), body.cend(), [](auto const *element) {
        return element->IsMethodDefinition() && element->AsClassElement()->IsPrivateElement();
    });
}

bool ClassDefinition::HasNativeMethod() const
{
    auto const body = Body();
    return std::any_of(body.cbegin(), body.cend(), [](auto const *element) {
        return element->IsMethodDefinition() && element->AsMethodDefinition()->IsNative();
    });
}

bool ClassDefinition::HasComputedInstanceField() const
{
    auto const body = Body();
    return std::any_of(body.cbegin(), body.cend(), [](auto *element) {
        return element->IsClassProperty() && element->AsClassElement()->IsComputed() &&
               !(element->AsClassElement()->Modifiers() & ir::ModifierFlags::STATIC);
    });
}

bool ClassDefinition::HasMatchingPrivateKey(const util::StringView &name) const
{
    auto const body = Body();
    return std::any_of(body.cbegin(), body.cend(), [&name](auto *element) {
        return element->AsClassElement()->IsPrivateElement() && element->AsClassElement()->Id()->Name() == name;
    });
}

void ClassDefinition::TransformBase(const NodeTransformer &cb, std::string_view transformationName)
{
    auto const ident = Ident();
    if (ident != nullptr) {
        if (auto *transformedNode = cb(ident); ident != transformedNode) {
            ident->SetTransformedNode(transformationName, transformedNode);
            SetIdent(transformedNode->AsIdentifier());
        }
    }

    auto const typeParam = TypeParams();
    if (typeParam != nullptr) {
        if (auto *transformedNode = cb(typeParam); typeParam != transformedNode) {
            typeParam->SetTransformedNode(transformationName, transformedNode);
            SetTypeParams(transformedNode->AsTSTypeParameterDeclaration());
        }
    }

    auto const superClass = SuperClass();
    if (superClass != nullptr) {
        if (auto *transformedNode = cb(superClass); superClass != transformedNode) {
            superClass->SetTransformedNode(transformationName, transformedNode);
            SetSuperClass(transformedNode->AsExpression());
        }
    }

    auto const superTypeParam = SuperTypeParams();
    if (superTypeParam != nullptr) {
        if (auto *transformedNode = cb(superTypeParam); superTypeParam != transformedNode) {
            superTypeParam->SetTransformedNode(transformationName, transformedNode);
            SetSuperTypeParams(transformedNode->AsTSTypeParameterInstantiation());
        }
    }
}

void ClassDefinition::TransformChildren(const NodeTransformer &cb, std::string_view transformationName)
{
    TransformBase(cb, transformationName);

    auto const &implement = Implements();
    for (size_t ix = 0; ix < implement.size(); ix++) {
        if (auto *transformedNode = cb(implement[ix]); implement[ix] != transformedNode) {
            implement[ix]->SetTransformedNode(transformationName, transformedNode);
            SetValueImplements(transformedNode->AsTSClassImplements(), ix);
        }
    }

    TransformAnnotations(cb, transformationName);

    auto const &ctor = Ctor();
    if (ctor != nullptr) {
        if (auto *transformedNode = cb(ctor); ctor != transformedNode) {
            ctor->SetTransformedNode(transformationName, transformedNode);
            SetCtor(transformedNode->AsMethodDefinition());
        }
    }

    // Survives adding new elements to the end
    // NOLINTNEXTLINE(modernize-loop-convert)
    auto const &body = Body();
    for (size_t ix = 0; ix < body.size(); ix++) {
        if (auto *transformedNode = cb(body[ix]); body[ix] != transformedNode) {
            body[ix]->SetTransformedNode(transformationName, transformedNode);
            SetValueBody(transformedNode, ix);
        }
    }
}

void ClassDefinition::Iterate(const NodeTraverser &cb) const
{
    auto const ident = GetHistoryNodeAs<ClassDefinition>()->ident_;
    if (ident != nullptr) {
        cb(ident);
    }

    auto const typeParams = GetHistoryNodeAs<ClassDefinition>()->typeParams_;
    if (typeParams != nullptr) {
        cb(typeParams);
    }

    auto const superClass = GetHistoryNodeAs<ClassDefinition>()->superClass_;
    if (superClass != nullptr) {
        cb(superClass);
    }

    auto const superTypeParams = GetHistoryNodeAs<ClassDefinition>()->superTypeParams_;
    if (superTypeParams != nullptr) {
        cb(superTypeParams);
    }

    // Survives adding new elements to the end
    // NOLINTNEXTLINE(modernize-loop-convert)
    auto const &implements = GetHistoryNodeAs<ClassDefinition>()->implements_;
    for (auto implement : implements) {
        cb(implement);
    }

    for (auto *it : VectorIterationGuard(Annotations())) {
        cb(it);
    }

    auto const ctor = GetHistoryNodeAs<ClassDefinition>()->ctor_;
    if (ctor != nullptr) {
        cb(ctor);
    }

    auto const &body = GetHistoryNodeAs<ClassDefinition>()->body_;
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (size_t ix = 0; ix < body.size(); ix++) {
        cb(body[ix]);
    }
}

void ClassDefinition::SetIdent(ir::Identifier *ident) noexcept
{
    auto newNode = this->GetOrCreateHistoryNodeAs<ClassDefinition>();
    newNode->ident_ = ident;
    if (ident != nullptr) {
        ident->SetParent(this);
    }
}

void ClassDefinition::Dump(ir::AstDumper *dumper) const
{
    auto propFilter = [](AstNode *prop) -> bool {
        return !prop->IsClassStaticBlock() || !prop->AsClassStaticBlock()->Function()->IsHidden();
    };
    auto ctor = GetHistoryNodeAs<ClassDefinition>()->ctor_;
    dumper->Add({{"id", AstDumper::Nullish(Ident())},
                 {"typeParameters", AstDumper::Optional(TypeParams())},
                 {"superClass", AstDumper::Nullish(SuperClass())},
                 {"superTypeParameters", AstDumper::Optional(SuperTypeParams())},
                 {"implements", Implements()},
                 {"annotations", AstDumper::Optional(Annotations())},
                 {"constructor", AstDumper::Optional(ctor)},
                 {"body", Body(), propFilter}});
}

void ClassDefinition::DumpGlobalClass(ir::SrcDumper *dumper) const
{
    ES2PANDA_ASSERT(IsGlobal());
    ir::ClassStaticBlock *classStaticBlock = nullptr;
    for (auto elem : Body()) {
        if (elem->IsClassProperty()) {
            elem->Dump(dumper);
            dumper->Endl();
        }

        if (elem->IsClassStaticBlock()) {
            classStaticBlock = elem->AsClassStaticBlock();
        }
    }
    for (auto elem : Body()) {
        if (elem->IsMethodDefinition()) {
            elem->Dump(dumper);
            dumper->Endl();
        }
    }

    if (classStaticBlock == nullptr) {
        return;
    }

    auto bodyStmts =
        classStaticBlock->Value()->AsFunctionExpression()->Function()->Body()->AsBlockStatement()->Statements();
    for (auto statement : bodyStmts) {
        if (statement->IsExpressionStatement() &&
            statement->AsExpressionStatement()->GetExpression()->IsAssignmentExpression() &&
            statement->AsExpressionStatement()->GetExpression()->AsAssignmentExpression()->IsIgnoreConstAssign()) {
            // skip the dummy assignment expression created for const variable decl in the class static block.
            continue;
        }
        statement->Dump(dumper);
        if (statement != bodyStmts.back()) {
            dumper->Endl();
        }
    }
}

// This method is needed by OHOS CI code checker
void ClassDefinition::DumpBody(ir::SrcDumper *dumper) const
{
    auto const body = Body();
    dumper->Add(" {");
    if (!body.empty()) {
        dumper->IncrIndent();
        dumper->Endl();
        for (auto elem : body) {
            elem->Dump(dumper);
            if (elem == body.back()) {
                dumper->DecrIndent();
            }
            dumper->Endl();
        }
    }
    dumper->Add("}");
}

void ClassDefinition::DumpPrefix(ir::SrcDumper *dumper) const
{
    if (IsExported()) {
        dumper->Add("export ");
    } else if (IsDefaultExported()) {
        dumper->Add("export default ");
    }

    if (IsDeclare() || dumper->IsDeclgen()) {
        dumper->Add("declare ");
    }

    if (IsFinal()) {
        dumper->Add("final ");
    }

    if (IsAbstract() && !IsNamespaceTransformed()) {
        dumper->Add("abstract ");
    }

    if (parent_->IsETSStructDeclaration() || IsFromStruct()) {
        dumper->Add("struct ");
    } else if (IsNamespaceTransformed()) {
        dumper->Add("namespace ");
    } else {
        dumper->Add("class ");
    }
}

bool ClassDefinition::RegisterUnexportedForDeclGen(ir::SrcDumper *dumper) const
{
    if (!dumper->IsDeclgen()) {
        return false;
    }

    if (dumper->IsIndirectDepPhase()) {
        return false;
    }

    if (IsExported() || IsDefaultExported()) {
        return false;
    }

    const auto className = ident_->Name().Mutf8();
    dumper->AddNode(className, this);
    return true;
}

void ClassDefinition::Dump(ir::SrcDumper *dumper) const
{
    // NOTE: plugin API fails
    auto const ident = Ident();
    if ((ident->Name().StartsWith("$dynmodule")) || (ident->Name().StartsWith("$jscall"))) {
        return;
    }

    if (IsGlobal()) {
        DumpGlobalClass(dumper);
        return;
    }

    ES2PANDA_ASSERT(ident_ != nullptr);

    if (RegisterUnexportedForDeclGen(dumper)) {
        return;
    }

    for (auto *anno : Annotations()) {
        anno->Dump(dumper);
    }

    DumpPrefix(dumper);
    ident_->Dump(dumper);

    if (TypeParams() != nullptr) {
        dumper->Add("<");
        TypeParams()->Dump(dumper);
        dumper->Add("> ");
    }

    if (SuperClass() != nullptr) {
        dumper->Add(" extends ");
        SuperClass()->Dump(dumper);
    }

    DumpItems(dumper, " implements ", Implements());

    if (!IsDeclare() || !Body().empty()) {
        DumpBody(dumper);
    }
    if (IsLocal()) {
        dumper->Add(";");
    }
    dumper->Endl();
}

void ClassDefinition::Compile(compiler::PandaGen *pg) const
{
    pg->GetAstCompiler()->Compile(this);
}

void ClassDefinition::Compile(compiler::ETSGen *etsg) const
{
    etsg->GetAstCompiler()->Compile(this);
}

checker::Type *ClassDefinition::Check(checker::TSChecker *checker)
{
    return checker->GetAnalyzer()->Check(this);
}

checker::VerifiedType ClassDefinition::Check(checker::ETSChecker *checker)
{
    return {this, checker->GetAnalyzer()->Check(this)};
}

ClassDefinition *ClassDefinition::Construct(ArenaAllocator *allocator)
{
    ArenaVector<AstNode *> body {allocator->Adapter()};
    return allocator->New<ClassDefinition>(allocator, nullptr, std::move(body), ClassDefinitionModifiers::NONE,
                                           ModifierFlags::NONE, Language::Id::COUNT, history_);
}

void ClassDefinition::CopyTo(AstNode *other) const
{
    auto otherImpl = other->AsClassDefinition();

    otherImpl->scope_ = scope_;
    otherImpl->internalName_ = internalName_;
    otherImpl->ident_ = ident_;
    otherImpl->typeParams_ = typeParams_;
    otherImpl->superTypeParams_ = superTypeParams_;
    otherImpl->implements_ = implements_;
    otherImpl->ctor_ = ctor_;
    otherImpl->superClass_ = superClass_;
    otherImpl->body_ = body_;
    otherImpl->modifiers_ = modifiers_;
    otherImpl->lang_ = lang_;
    otherImpl->capturedVars_ = capturedVars_;
    otherImpl->localVariableIsNeeded_ = localVariableIsNeeded_;
    otherImpl->origEnumDecl_ = origEnumDecl_;
    otherImpl->anonClass_ = anonClass_;
    otherImpl->localIndex_ = localIndex_;
    otherImpl->localPrefix_ = localPrefix_;
    otherImpl->functionalReferenceReferencedMethod_ = functionalReferenceReferencedMethod_;
    otherImpl->exportedClasses_ = exportedClasses_;

    AnnotationAllowed<TypedAstNode>::CopyTo(other);
}

std::atomic<int> ClassDefinition::classCounter_ = 0;

}  // namespace ark::es2panda::ir
