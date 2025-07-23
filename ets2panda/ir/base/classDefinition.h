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

#ifndef ES2PANDA_PARSER_INCLUDE_AST_CLASS_DEFINITION_H
#define ES2PANDA_PARSER_INCLUDE_AST_CLASS_DEFINITION_H

#include "varbinder/scope.h"
#include "varbinder/variable.h"
#include "ir/srcDump.h"
#include "ir/annotationAllowed.h"
#include "ir/astNode.h"
#include "ir/astNodeHistory.h"
#include "ir/expressions/identifier.h"
#include "ir/statements/annotationUsage.h"
#include "ir/statements/classDeclaration.h"
#include "util/language.h"

namespace ark::es2panda::ir {
class ClassElement;
class Identifier;
class MethodDefinition;
class TSTypeParameterDeclaration;
class TSTypeParameterInstantiation;
class TSClassImplements;
class TSIndexSignature;

using ENUMBITOPS_OPERATORS;

enum class ClassDefinitionModifiers : uint32_t {
    NONE = 0,
    DECLARATION = 1U << 0U,
    ID_REQUIRED = 1U << 1U,
    GLOBAL = 1U << 2U,
    HAS_SUPER = 1U << 3U,
    SET_CTOR_ID = 1U << 4U,
    EXTERN = 1U << 5U,
    ANONYMOUS = 1U << 6U,
    GLOBAL_INITIALIZED = 1U << 7U,
    CLASS_DECL = 1U << 8U,
    INNER = 1U << 9U,
    FROM_EXTERNAL = 1U << 10U,
    LOCAL = 1U << 11U,
    CLASSDEFINITION_CHECKED = 1U << 12U,
    NAMESPACE_TRANSFORMED = 1U << 13U,
    STRING_ENUM_TRANSFORMED = 1U << 14U,
    INT_ENUM_TRANSFORMED = 1U << 15U,
    FROM_STRUCT = 1U << 16U,
    FUNCTIONAL_REFERENCE = 1U << 17U,
    LAZY_IMPORT_OBJECT_CLASS = 1U << 18U,
    INIT_IN_CCTOR = 1U << 19U,
    DECLARATION_ID_REQUIRED = DECLARATION | ID_REQUIRED,
    ETS_MODULE = NAMESPACE_TRANSFORMED | GLOBAL
};

}  // namespace ark::es2panda::ir

template <>
struct enumbitops::IsAllowedType<ark::es2panda::ir::ClassDefinitionModifiers> : std::true_type {
};

namespace ark::es2panda::ir {

class ClassDefinition : public AnnotationAllowed<TypedAstNode> {
public:
    ClassDefinition() = delete;
    ~ClassDefinition() override = default;

    NO_COPY_SEMANTIC(ClassDefinition);
    NO_MOVE_SEMANTIC(ClassDefinition);
    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    explicit ClassDefinition(ArenaAllocator *allocator, Identifier *ident, TSTypeParameterDeclaration *typeParams,
                             TSTypeParameterInstantiation *superTypeParams,
                             ArenaVector<TSClassImplements *> &&implements, MethodDefinition *ctor,
                             Expression *superClass, ArenaVector<AstNode *> &&body, ClassDefinitionModifiers modifiers,
                             ModifierFlags flags, Language lang)
        : AnnotationAllowed<TypedAstNode>(AstNodeType::CLASS_DEFINITION, flags, allocator),
          ident_(ident),
          typeParams_(typeParams),
          superTypeParams_(superTypeParams),
          implements_(std::move(implements)),
          ctor_(ctor),
          superClass_(superClass),
          body_(std::move(body)),
          modifiers_(modifiers),
          lang_(lang),
          capturedVars_(body_.get_allocator()),
          localVariableIsNeeded_(body_.get_allocator()),
          localIndex_(classCounter_++),
          localPrefix_("$" + std::to_string(localIndex_)),
          exportedClasses_(body_.get_allocator())
    {
        InitHistory();
    }
    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    explicit ClassDefinition(ArenaAllocator *allocator, Identifier *ident, ArenaVector<AstNode *> &&body,
                             ClassDefinitionModifiers modifiers, ModifierFlags flags, Language lang)
        : AnnotationAllowed<TypedAstNode>(AstNodeType::CLASS_DEFINITION, flags, allocator),
          ident_(ident),
          implements_(allocator->Adapter()),
          body_(std::move(body)),
          modifiers_(modifiers),
          lang_(lang),
          capturedVars_(allocator->Adapter()),
          localVariableIsNeeded_(allocator->Adapter()),
          localIndex_(classCounter_++),
          localPrefix_("$" + std::to_string(localIndex_)),
          exportedClasses_(body_.get_allocator())
    {
        InitHistory();
    }

    explicit ClassDefinition(ArenaAllocator *allocator, Identifier *ident, ClassDefinitionModifiers modifiers,
                             ModifierFlags flags, Language lang)
        : AnnotationAllowed<TypedAstNode>(AstNodeType::CLASS_DEFINITION, flags, allocator),
          ident_(ident),
          implements_(allocator->Adapter()),
          body_(allocator->Adapter()),
          modifiers_(modifiers),
          lang_(lang),
          capturedVars_(allocator->Adapter()),
          localVariableIsNeeded_(allocator->Adapter()),
          localIndex_(classCounter_++),
          localPrefix_("$" + std::to_string(localIndex_)),
          exportedClasses_(body_.get_allocator())
    {
        InitHistory();
    }

    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    explicit ClassDefinition(ArenaAllocator *allocator, Identifier *ident, ArenaVector<AstNode *> &&body,
                             ClassDefinitionModifiers modifiers, ModifierFlags flags, Language lang,
                             AstNodeHistory *history)
        : AnnotationAllowed<TypedAstNode>(AstNodeType::CLASS_DEFINITION, flags, allocator),
          ident_(ident),
          implements_(allocator->Adapter()),
          body_(std::move(body)),
          modifiers_(modifiers),
          lang_(lang),
          capturedVars_(allocator->Adapter()),
          localVariableIsNeeded_(allocator->Adapter()),
          localIndex_(classCounter_++),
          localPrefix_("$" + std::to_string(localIndex_)),
          exportedClasses_(body_.get_allocator())
    {
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
        return GetHistoryNodeAs<ClassDefinition>()->scope_;
    }

    void ClearScope() noexcept override
    {
        SetScope(nullptr);
    }

    [[nodiscard]] const Identifier *Ident() const noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->ident_;
    }

    [[nodiscard]] Identifier *Ident() noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->ident_;
    }

    void SetIdent(ir::Identifier *ident) noexcept;

    [[nodiscard]] const util::StringView &InternalName() const noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->internalName_;
    }

    [[nodiscard]] Expression *Super() noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->superClass_;
    }

    [[nodiscard]] const Expression *Super() const noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->superClass_;
    }

    void SetSuper(Expression *superClass)
    {
        auto newNode = this->GetOrCreateHistoryNodeAs<ClassDefinition>();
        newNode->superClass_ = superClass;
        if (newNode->superClass_ != nullptr) {
            newNode->superClass_->SetParent(this);
        }
    }

    [[nodiscard]] bool IsGlobal() const noexcept
    {
        return (Modifiers() & ClassDefinitionModifiers::GLOBAL) != 0;
    }

    [[nodiscard]] bool IsLocal() const noexcept
    {
        return (Modifiers() & ClassDefinitionModifiers::LOCAL) != 0;
    }

    [[nodiscard]] bool IsExtern() const noexcept
    {
        return (Modifiers() & ClassDefinitionModifiers::EXTERN) != 0;
    }

    [[nodiscard]] bool IsFromExternal() const noexcept
    {
        return (Modifiers() & ClassDefinitionModifiers::FROM_EXTERNAL) != 0;
    }
    [[nodiscard]] bool IsInner() const noexcept
    {
        return (Modifiers() & ClassDefinitionModifiers::INNER) != 0;
    }

    [[nodiscard]] bool IsGlobalInitialized() const noexcept
    {
        return (Modifiers() & ClassDefinitionModifiers::GLOBAL_INITIALIZED) != 0;
    }

    [[nodiscard]] bool IsClassDefinitionChecked() const noexcept
    {
        return (Modifiers() & ClassDefinitionModifiers::CLASSDEFINITION_CHECKED) != 0;
    }

    [[nodiscard]] bool IsAnonymous() const noexcept
    {
        return (Modifiers() & ClassDefinitionModifiers::ANONYMOUS) != 0;
    }

    [[nodiscard]] bool IsIntEnumTransformed() const noexcept
    {
        return (Modifiers() & ClassDefinitionModifiers::INT_ENUM_TRANSFORMED) != 0;
    }

    [[nodiscard]] bool IsStringEnumTransformed() const noexcept
    {
        return (Modifiers() & ClassDefinitionModifiers::STRING_ENUM_TRANSFORMED) != 0;
    }

    [[nodiscard]] bool IsEnumTransformed() const noexcept
    {
        return IsIntEnumTransformed() || IsStringEnumTransformed();
    }

    [[nodiscard]] bool IsNamespaceTransformed() const noexcept
    {
        return (Modifiers() & ClassDefinitionModifiers::NAMESPACE_TRANSFORMED) != 0;
    }

    [[nodiscard]] bool IsLazyImportObjectClass() const noexcept
    {
        return (Modifiers() & ClassDefinitionModifiers::LAZY_IMPORT_OBJECT_CLASS) != 0;
    }

    [[nodiscard]] bool IsFromStruct() const noexcept
    {
        return (Modifiers() & ClassDefinitionModifiers::FROM_STRUCT) != 0;
    }

    [[nodiscard]] bool IsInitInCctor() const noexcept
    {
        return (Modifiers() & ClassDefinitionModifiers::INIT_IN_CCTOR) != 0;
    }

    [[nodiscard]] bool IsModule() const noexcept
    {
        return IsGlobal() || IsNamespaceTransformed();
    }

    [[nodiscard]] es2panda::Language Language() const noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->lang_;
    }

    void SetGlobalInitialized() noexcept
    {
        AddClassModifiers(ClassDefinitionModifiers::GLOBAL_INITIALIZED);
    }

    void SetInnerModifier() noexcept
    {
        AddClassModifiers(ClassDefinitionModifiers::INNER);
    }

    void SetClassDefinitionChecked() noexcept
    {
        AddClassModifiers(ClassDefinitionModifiers::CLASSDEFINITION_CHECKED);
    }

    void SetAnonymousModifier() noexcept
    {
        AddClassModifiers(ClassDefinitionModifiers::ANONYMOUS);
    }

    void SetNamespaceTransformed() noexcept
    {
        AddClassModifiers(ClassDefinitionModifiers::NAMESPACE_TRANSFORMED);
    }

    void SetLazyImportObjectClass() noexcept
    {
        AddClassModifiers(ClassDefinitionModifiers::LAZY_IMPORT_OBJECT_CLASS);
    }

    void SetFromStructModifier() noexcept
    {
        AddClassModifiers(ClassDefinitionModifiers::FROM_STRUCT);
    }

    void SetInitInCctor()
    {
        AddClassModifiers(ClassDefinitionModifiers::INIT_IN_CCTOR);
    }

    [[nodiscard]] ClassDefinitionModifiers Modifiers() const noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->modifiers_;
    }

    void AddProperties(ArenaVector<AstNode *> &&body)
    {
        for (auto *prop : body) {
            prop->SetParent(this);
        }

        auto newNode = GetOrCreateHistoryNode()->AsClassDefinition();
        newNode->body_.insert(newNode->body_.end(), body.begin(), body.end());
    }

    [[nodiscard]] const ArenaVector<AstNode *> &Body() const noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->body_;
    }

    [[nodiscard]] MethodDefinition *Ctor() noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->ctor_;
    }

    [[nodiscard]] const ArenaVector<ir::TSClassImplements *> &Implements() const noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->implements_;
    }

    [[nodiscard]] const ir::TSTypeParameterDeclaration *TypeParams() const noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->typeParams_;
    }

    [[nodiscard]] ir::TSTypeParameterDeclaration *TypeParams() noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->typeParams_;
    }

    const TSTypeParameterInstantiation *SuperTypeParams() const
    {
        return GetHistoryNodeAs<ClassDefinition>()->superTypeParams_;
    }

    TSTypeParameterInstantiation *SuperTypeParams()
    {
        return GetHistoryNodeAs<ClassDefinition>()->superTypeParams_;
    }

    // ekkoruse: dangerous count for cache here
    [[nodiscard]] static int LocalTypeCounter() noexcept
    {
        return classCounter_;
    }

    [[nodiscard]] int LocalIndex() const noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->localIndex_;
    }

    [[nodiscard]] MethodDefinition *FunctionalReferenceReferencedMethod() const noexcept
    {
        return functionalReferenceReferencedMethod_;
    }

    void SetFunctionalReferenceReferencedMethod(MethodDefinition *functionalReferenceReferencedMethod)
    {
        functionalReferenceReferencedMethod_ = functionalReferenceReferencedMethod;
    }

    [[nodiscard]] const std::string &LocalPrefix() const noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->localPrefix_;
    }

    bool CaptureVariable(varbinder::Variable *var)
    {
        auto newNode = GetOrCreateHistoryNode()->AsClassDefinition();
        return newNode->capturedVars_.insert(var).second;
    }

    bool AddToLocalVariableIsNeeded(varbinder::Variable *var)
    {
        auto newNode = GetOrCreateHistoryNode()->AsClassDefinition();
        return newNode->localVariableIsNeeded_.insert(var).second;
    }

    bool IsLocalVariableNeeded(varbinder::Variable *var) const
    {
        auto const newNode = GetHistoryNode()->AsClassDefinition();
        return newNode->localVariableIsNeeded_.find(var) != newNode->localVariableIsNeeded_.end();
    }

    [[nodiscard]] const ArenaSet<varbinder::Variable *> &CapturedVariables() const noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->capturedVars_;
    }

    bool EraseCapturedVariable(varbinder::Variable *var)
    {
        auto newNode = GetOrCreateHistoryNode()->AsClassDefinition();
        return newNode->capturedVars_.erase(var) != 0;
    }

    ir::TSEnumDeclaration *OrigEnumDecl() const
    {
        return GetHistoryNodeAs<ClassDefinition>()->origEnumDecl_;
    }

    ClassDeclaration *GetAnonClass() noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->anonClass_;
    }

    const FunctionExpression *Ctor() const;
    bool HasPrivateMethod() const;
    bool HasNativeMethod() const;
    bool HasComputedInstanceField() const;
    bool HasMatchingPrivateKey(const util::StringView &name) const;

    void TransformBase(const NodeTransformer &cb, std::string_view transformationName);
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

    template <typename T>
    static void DumpItems(ir::SrcDumper *dumper, const std::string &prefix, const ArenaVector<T *> &items)
    {
        if (items.empty()) {
            return;
        }
        dumper->Add(prefix);
        for (size_t i = 0; i < items.size(); ++i) {
            items[i]->Dump(dumper);
            if (i < items.size() - 1) {
                dumper->Add(", ");
            }
        }
    }

    void CleanUp() override
    {
        AstNode::CleanUp();
        ClearClassModifiers(ClassDefinitionModifiers::CLASSDEFINITION_CHECKED);
    }

    void AddToExportedClasses(const ir::ClassDeclaration *cls)
    {
        ES2PANDA_ASSERT(cls->IsExported() || cls->Definition()->IsGlobal());
        auto newNode = reinterpret_cast<ClassDefinition *>(this->GetOrCreateHistoryNode());
        newNode->exportedClasses_.emplace_back(cls);
    }

    void BatchAddToExportedClasses(const ArenaVector<const ir::ClassDeclaration *> &classes)
    {
        for (const auto cls : classes) {
            AddToExportedClasses(cls);
        }
    }

    [[nodiscard]] const ArenaVector<const ir::ClassDeclaration *> &ExportedClasses() const noexcept
    {
        return GetHistoryNodeAs<ClassDefinition>()->exportedClasses_;
    }
    void SetScope(varbinder::LocalScope *scope);
    void SetModifiers(ClassDefinitionModifiers modifiers);

    void EmplaceBody(AstNode *body);
    void ClearBody();
    void SetValueBody(AstNode *body, size_t index);
    const ArenaVector<AstNode *> &Body();
    [[nodiscard]] ArenaVector<AstNode *> &BodyForUpdate();

    void EmplaceImplements(TSClassImplements *implements);
    void ClearImplements();
    void SetValueImplements(TSClassImplements *implements, size_t index);
    const ArenaVector<TSClassImplements *> &Implements();
    ArenaVector<TSClassImplements *> &ImplementsForUpdate();

    void SetCtor(MethodDefinition *ctor);
    void SetTypeParams(TSTypeParameterDeclaration *typeParams);
    void SetOrigEnumDecl(TSEnumDeclaration *origEnumDecl);
    void SetAnonClass(ClassDeclaration *anonClass);
    void SetInternalName(util::StringView internalName);

protected:
    ClassDefinition *Construct(ArenaAllocator *allocator) override;

    void AddClassModifiers(ClassDefinitionModifiers const flags) noexcept
    {
        if (!All(Modifiers(), flags)) {
            GetOrCreateHistoryNodeAs<ClassDefinition>()->modifiers_ |= flags;
        }
    }

    void ClearClassModifiers(ClassDefinitionModifiers const flags) noexcept
    {
        if (Any(Modifiers(), flags)) {
            GetOrCreateHistoryNodeAs<ClassDefinition>()->modifiers_ &= ~flags;
        }
    }

    void CopyTo(AstNode *other) const override;

private:
    void SetSuperClass(Expression *superClass);
    void SetSuperTypeParams(TSTypeParameterInstantiation *superTypeParams);

    [[nodiscard]] Expression *SuperClass()
    {
        return GetHistoryNodeAs<ClassDefinition>()->superClass_;
    }

    [[nodiscard]] const Expression *SuperClass() const
    {
        return GetHistoryNodeAs<ClassDefinition>()->superClass_;
    }

    void CompileStaticFieldInitializers(compiler::PandaGen *pg, compiler::VReg classReg,
                                        const std::vector<compiler::VReg> &staticComputedFieldKeys) const;

    // This method is needed by OHOS CI code checker
    void DumpBody(ir::SrcDumper *dumper) const;
    void DumpGlobalClass(ir::SrcDumper *dumper) const;
    void DumpPrefix(ir::SrcDumper *dumper) const;
    bool RegisterUnexportedForDeclGen(ir::SrcDumper *dumper) const;

    friend class SizeOfNodeTest;
    varbinder::LocalScope *scope_ {nullptr};
    util::StringView internalName_ {};
    Identifier *ident_ {};
    TSTypeParameterDeclaration *typeParams_ {};
    TSTypeParameterInstantiation *superTypeParams_ {};
    ArenaVector<TSClassImplements *> implements_;
    MethodDefinition *ctor_ {};
    Expression *superClass_ {};
    ArenaVector<AstNode *> body_;
    ClassDefinitionModifiers modifiers_;
    es2panda::Language lang_;
    ArenaSet<varbinder::Variable *> capturedVars_;
    ArenaSet<varbinder::Variable *> localVariableIsNeeded_;
    TSEnumDeclaration *origEnumDecl_ {};
    ClassDeclaration *anonClass_ {nullptr};
    static std::atomic<int> classCounter_;
    int localIndex_ {};
    std::string localPrefix_ {};
    MethodDefinition *functionalReferenceReferencedMethod_ {};
    ArenaVector<const ir::ClassDeclaration *> exportedClasses_;
};
}  // namespace ark::es2panda::ir

#endif
