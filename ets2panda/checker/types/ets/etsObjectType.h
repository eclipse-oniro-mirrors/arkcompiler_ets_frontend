/*
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

#ifndef ES2PANDA_COMPILER_CHECKER_TYPES_ETS_OBJECT_TYPE_H
#define ES2PANDA_COMPILER_CHECKER_TYPES_ETS_OBJECT_TYPE_H

#include <memory>
#include <mutex>
#include <shared_mutex>

#include "checker/checker.h"
#include "checker/types/type.h"
#include "checker/types/ets/etsObjectTypeConstants.h"
#include "checker/types/signature.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/ts/tsTypeParameterDeclaration.h"
#include "ir/ts/tsEnumDeclaration.h"
#include "varbinder/scope.h"
#include "ir/base/classDefinition.h"

namespace ark::es2panda::checker {
using PropertyProcesser = std::function<varbinder::LocalVariable *(varbinder::LocalVariable *, Type *)>;

inline constexpr auto *PARTIAL_CLASS_SUFFIX = "$partial";

class ETSObjectType : public Type {
public:
    using PropertyMap = ArenaUnorderedMap<util::StringView, varbinder::LocalVariable *>;
    using InstantiationMap = ArenaUnorderedMap<util::StringView, ETSObjectType *>;
    using PropertyTraverser = std::function<void(const varbinder::LocalVariable *)>;
    using PropertyHolder = std::array<PropertyMap *, static_cast<size_t>(PropertyType::COUNT)>;

    explicit ETSObjectType(ThreadSafeArenaAllocator *allocator, util::StringView name, util::StringView internalName,
                           ir::AstNode *declNode, ETSObjectFlags flags)
        : ETSObjectType(allocator, name, internalName, std::make_tuple(declNode, flags, nullptr),
                        std::make_index_sequence<static_cast<size_t>(PropertyType::COUNT)> {})
    {
    }

    explicit ETSObjectType(ThreadSafeArenaAllocator *allocator, util::StringView name, util::StringView internalName,
                           std::tuple<ir::AstNode *, ETSObjectFlags, TypeRelation *> info)
        : ETSObjectType(allocator, name, internalName, info,
                        std::make_index_sequence<static_cast<size_t>(PropertyType::COUNT)> {})
    {
    }

    void AddConstructSignature(Signature *signature)
    {
        constructSignatures_.push_back(signature);
        propertiesInstantiated_ = true;
    }

    void AddConstructSignature(const ArenaVector<Signature *> &signatures) const
    {
        constructSignatures_.insert(constructSignatures_.end(), signatures.begin(), signatures.end());
        propertiesInstantiated_ = true;
    }

    void AddInterface(ETSObjectType *interfaceType);
    void SetSuperType(ETSObjectType *super);

    ETSChecker *GetETSChecker();

    void SetTypeArguments(ArenaVector<Type *> &&typeArgs)
    {
#ifndef NDEBUG
        for (auto const &t : typeArgs) {
            ES2PANDA_ASSERT(t->IsETSReferenceType());
        }
#endif
        typeArguments_ = std::move(typeArgs);
    }

    void SetEnclosingType(ETSObjectType *enclosingType)
    {
        enclosingType_ = enclosingType;
    }

    void SetRelation(TypeRelation *relation)
    {
        ES2PANDA_ASSERT(relation);
        relation_ = relation;
    }

    TypeRelation *GetRelation() const
    {
        return relation_;
    }

    PropertyMap &InstanceMethods() const
    {
        EnsurePropertiesInstantiated();
        EnsurePropertyMapInitialized(PropertyType::INSTANCE_METHOD);
        return *properties_[static_cast<size_t>(PropertyType::INSTANCE_METHOD)];
    }

    PropertyMap &InstanceFields() const
    {
        EnsurePropertiesInstantiated();
        EnsurePropertyMapInitialized(PropertyType::INSTANCE_FIELD);
        return *properties_[static_cast<size_t>(PropertyType::INSTANCE_FIELD)];
    }

    PropertyMap &InstanceDecls() const
    {
        EnsurePropertiesInstantiated();
        EnsurePropertyMapInitialized(PropertyType::INSTANCE_DECL);
        return *properties_[static_cast<size_t>(PropertyType::INSTANCE_DECL)];
    }

    PropertyMap &StaticMethods() const
    {
        EnsurePropertiesInstantiated();
        EnsurePropertyMapInitialized(PropertyType::STATIC_METHOD);
        return *properties_[static_cast<size_t>(PropertyType::STATIC_METHOD)];
    }

    PropertyMap &StaticFields() const
    {
        EnsurePropertiesInstantiated();
        EnsurePropertyMapInitialized(PropertyType::STATIC_FIELD);
        return *properties_[static_cast<size_t>(PropertyType::STATIC_FIELD)];
    }

    PropertyMap &StaticDecls() const
    {
        EnsurePropertiesInstantiated();
        EnsurePropertyMapInitialized(PropertyType::STATIC_DECL);
        return *properties_[static_cast<size_t>(PropertyType::STATIC_DECL)];
    }

    const ArenaVector<Type *> &TypeArguments() const
    {
        return typeArguments_;
    }

    const ArenaVector<Signature *> &ConstructSignatures() const
    {
        EnsurePropertiesInstantiated();
        return constructSignatures_;
    }

    ArenaVector<Signature *> &ConstructSignatures()
    {
        EnsurePropertiesInstantiated();
        return constructSignatures_;
    }

    const ArenaVector<ETSObjectType *> &Interfaces() const
    {
        EnsureInterfacesInitialized();
        return *interfaces_;
    }

    const ArenaVector<ETSObjectType *> &Interfaces()
    {
        EnsureInterfacesInitialized();
        return *interfaces_;
    }

    ir::AstNode *GetDeclNode() const
    {
        return declNode_;
    }

    const ETSObjectType *SuperType() const
    {
        return superType_;
    }

    ETSObjectType *SuperType()
    {
        return superType_;
    }

    const ETSObjectType *EnclosingType() const
    {
        return enclosingType_;
    }

    ETSObjectType *EnclosingType()
    {
        return enclosingType_;
    }

    ETSObjectType *OutermostClass()
    {
        auto *iter = enclosingType_;

        while (iter != nullptr && iter->EnclosingType() != nullptr) {
            iter = iter->EnclosingType();
        }

        return iter;
    }

    void SetBaseType(ETSObjectType *baseType)
    {
        baseType_ = baseType;
    }

    ETSObjectType *GetBaseType() noexcept
    {
        return baseType_;
    }

    const ETSObjectType *GetBaseType() const noexcept
    {
        return baseType_;
    }

    const ArenaSet<ETSObjectType *> &TransitiveSupertypes() const noexcept
    {
        return *transitiveSupertypes_;
    }

    ETSObjectType const *GetConstOriginalBaseType() const noexcept;

    ETSObjectType *GetOriginalBaseType() const noexcept
    {
        return const_cast<ETSObjectType *>(GetConstOriginalBaseType());
    }

    bool IsGlobalETSObjectType() const noexcept
    {
        return superType_ == nullptr && !IsGradual();
    }

    bool IsPropertyInherited(const varbinder::Variable *var);

    bool IsPropertyOfAscendant(const varbinder::Variable *var) const;

    bool IsSignatureInherited(Signature *signature);

    bool IsDescendantOf(const ETSObjectType *ascendant) const;

    util::StringView Name() const
    {
        return name_;
    }

    util::StringView AssemblerName() const;

    ETSObjectFlags ObjectFlags() const
    {
        return flags_;
    }

    void AddObjectFlag(ETSObjectFlags flag)
    {
        flags_ |= flag;
    }

    void RemoveObjectFlag(ETSObjectFlags flag)
    {
        flags_ &= ~flag;
    }

    bool HasObjectFlag(ETSObjectFlags flag) const
    {
        return (flags_ & flag) != 0;
    }

    bool IsInterface() const
    {
        return HasObjectFlag(ETSObjectFlags::INTERFACE);
    }

    bool IsGradual() const
    {
        return HasObjectFlag(ETSObjectFlags::GRADUAL);
    }

    bool IsETSStringLiteralType() const
    {
        return superType_ != nullptr && superType_->IsETSObjectType() &&
               superType_->HasObjectFlag(ETSObjectFlags::STRING);
    }

    ETSFunctionType *GetFunctionalInterfaceInvokeType() const;

    ETSObjectFlags BuiltInKind() const
    {
        return static_cast<checker::ETSObjectFlags>(flags_ & ETSObjectFlags::BUILTIN_TYPE);
    }

    ETSObjectFlags UnboxableKind() const
    {
        return static_cast<checker::ETSObjectFlags>(flags_ & ETSObjectFlags::UNBOXABLE_TYPE);
    }

    ETSObjectType *GetInstantiatedType(util::StringView hash);

    varbinder::Scope *GetTypeArgumentScope() const
    {
        auto *typeParams = GetTypeParams();
        if (typeParams == nullptr) {
            return nullptr;
        }
        return typeParams->Scope();
    }

    void InsertInstantiationMap(const util::StringView key, ETSObjectType *value);

    template <PropertyType TYPE>
    varbinder::LocalVariable *GetOwnProperty(const util::StringView name) const
    {
        EnsurePropertiesInstantiated();
        EnsurePropertyMapInitialized(TYPE);
        auto found = properties_[static_cast<size_t>(TYPE)]->find(name);
        if (found != properties_[static_cast<size_t>(TYPE)]->end()) {
            return found->second;
        }
        return nullptr;
    }

    template <PropertyType TYPE>
    void AddProperty(varbinder::LocalVariable *prop) const
    {
        EnsurePropertyMapInitialized(TYPE);
        properties_[static_cast<size_t>(TYPE)]->emplace(prop->Name(), prop);
        propertiesInstantiated_ = true;
    }

    template <PropertyType TYPE>
    void AddProperty(varbinder::LocalVariable *prop, util::StringView localName) const
    {
        util::StringView nameToAccess = prop->Name();

        if (!localName.Empty()) {
            nameToAccess = localName;
        }

        EnsurePropertyMapInitialized(TYPE);
        properties_[static_cast<size_t>(TYPE)]->emplace(nameToAccess, prop);
        propertiesInstantiated_ = true;
    }

    template <PropertyType TYPE>
    void RemoveProperty(varbinder::LocalVariable *prop)
    {
        EnsurePropertyMapInitialized(TYPE);
        properties_[static_cast<size_t>(TYPE)]->erase(prop->Name());
        propertiesInstantiated_ = true;
    }

    [[nodiscard]] bool IsGeneric() const noexcept
    {
        return !typeArguments_.empty();
    }

    [[nodiscard]] bool IsPartial() const noexcept
    {
        return name_.EndsWith(PARTIAL_CLASS_SUFFIX);
    }

    std::vector<const varbinder::LocalVariable *> ForeignProperties() const;
    varbinder::LocalVariable *GetProperty(util::StringView name, PropertySearchFlags flags) const;
    std::vector<varbinder::LocalVariable *> GetAllProperties() const;
    void ForEachAllOwnProperties(const PropertyTraverser &cb) const;
    void ForEachAllNonOwnProperties(const PropertyTraverser &cb) const;
    varbinder::LocalVariable *CopyProperty(varbinder::LocalVariable *prop, ArenaAllocator *allocator,
                                           TypeRelation *relation, GlobalTypesHolder *globalTypes);
    std::vector<varbinder::LocalVariable *> Methods() const;
    std::vector<varbinder::LocalVariable *> Fields() const;
    std::vector<varbinder::LocalVariable *> Overloads() const;
    varbinder::LocalVariable *CreateSyntheticVarFromEverySignature(const util::StringView &name,
                                                                   PropertySearchFlags flags) const;
    varbinder::LocalVariable *CollectSignaturesForSyntheticType(std::vector<Signature *> &signatures,
                                                                const util::StringView &name,
                                                                PropertySearchFlags flags) const;
    void AddSignatureFromFunction(std::vector<Signature *> &signatures, PropertySearchFlags flags, ETSChecker *checker,
                                  varbinder::LocalVariable *found) const;
    void AddSignatureFromOverload(std::vector<Signature *> &signatures, PropertySearchFlags flags,
                                  varbinder::LocalVariable *found) const;
    void AddSignatureFromConstructor(std::vector<Signature *> &signatures, varbinder::LocalVariable *found) const;
    bool ReplaceArgumentInSignature(std::vector<Signature *> &signatures, Signature *sigToInsert,
                                    TypeRelation *relation) const;
    bool CheckIdenticalFlags(ETSObjectType *other) const;
    void Iterate(const PropertyTraverser &cb) const;
    void ToString(std::stringstream &ss, bool precise) const override;
    void Identical(TypeRelation *relation, Type *other) override;
    bool AssignmentSource(TypeRelation *relation, Type *target) override;
    void AssignmentTarget(TypeRelation *relation, Type *source) override;
    bool IsBoxedPrimitive() const;
    Type *Instantiate(ArenaAllocator *allocator, TypeRelation *relation, GlobalTypesHolder *globalTypes) override;
    void UpdateTypeProperties(PropertyProcesser const &func);
    ETSObjectType *Substitute(TypeRelation *relation, const Substitution *substitution) override;
    ETSObjectType *Substitute(TypeRelation *relation, const Substitution *substitution, bool cache,
                              bool isExtensionFunctionType = false);
    ETSObjectType *SubstituteArguments(TypeRelation *relation, ArenaVector<Type *> const &arguments);
    void Cast(TypeRelation *relation, Type *target) override;
    bool CastNumericObject(TypeRelation *relation, Type *target);
    void IsSupertypeOf(TypeRelation *relation, Type *source) override;
    void IsSubtypeOf(TypeRelation *relation, Type *target) override;
    ETSObjectType *ProcessInterfaceBaseType(std::unordered_set<Type *> *extendsSet);
    Type *AsSuper(Checker *checker, varbinder::Variable *sourceVar) override;
    void ToAssemblerType([[maybe_unused]] std::stringstream &ss) const override;
    static std::string NameToDescriptor(util::StringView name);
    void ToDebugInfoType(std::stringstream &ss) const override;
    void ToDebugInfoSignatureType(std::stringstream &ss) const;
    void CheckVarianceRecursively(TypeRelation *relation, VarianceFlag varianceFlag) override;

    void AddReExports(ETSObjectType *reExport);
    void AddReExportAlias(util::StringView const &value, util::StringView const &key);
    util::StringView GetReExportAliasValue(util::StringView const &key) const;
    bool IsReExportHaveAliasValue(util::StringView const &key) const;
    const ArenaVector<ETSObjectType *> &ReExports() const;
    bool IsSameBasedGeneric(TypeRelation *relation, Type const *other) const;

    ThreadSafeArenaAllocator *Allocator() const
    {
        return allocator_;
    }

    [[nodiscard]] static std::uint32_t GetPrecedence(checker::ETSChecker *checker, ETSObjectType const *type) noexcept;

    bool IsPropertiesInstantiated() const
    {
        return propertiesInstantiated_;
    }

protected:
    virtual ETSFunctionType *CreateMethodTypeForProp(util::StringView name) const;

private:
    template <size_t... IS>
    explicit ETSObjectType(ThreadSafeArenaAllocator *allocator, util::StringView name, util::StringView assemblerName,
                           std::tuple<ir::AstNode *, ETSObjectFlags, TypeRelation *> info,
                           [[maybe_unused]] std::index_sequence<IS...> s)
        : Type(TypeFlag::ETS_OBJECT),
          allocator_(allocator),
          name_(name),
          internalName_(assemblerName),
          declNode_(std::get<ir::AstNode *>(info)),
          interfaces_(nullptr),
          reExports_(nullptr),
          reExportAlias_(nullptr),
          flags_(std::get<ETSObjectFlags>(info)),
          typeArguments_(allocator->Adapter()),
          transitiveSupertypes_(nullptr),
          relation_(std::get<TypeRelation *>(info)),
          constructSignatures_(allocator->Adapter()),
          properties_ {}
    {
    }

    template <typename MapFn>
    void CopyPropertyGroup(PropertyType type, MapFn &&mapFn, const Substitution &subst) const;

    /* Properties and construct signatures are instantiated lazily. */
    void InstantiateProperties() const;
    void CheckAndInstantiateProperties() const;
    void EnsurePropertiesInstantiated() const
    {
        if (!propertiesInstantiated_) {
            CheckAndInstantiateProperties();
            propertiesInstantiated_ = true;
        }
    }
    void EnsurePropertyMapInitialized(PropertyType type) const
    {
        auto index = static_cast<size_t>(type);
        if (properties_[index] == nullptr) {
            properties_[index] = allocator_->New<PropertyMap>(allocator_->Adapter());
        }
    }

    void EnsureInterfacesInitialized() const
    {
        if (interfaces_ == nullptr) {
            interfaces_ = allocator_->New<ArenaVector<ETSObjectType *>>(allocator_->Adapter());
        }
    }

    void EnsureReExportsInitialized() const
    {
        if (reExports_ == nullptr) {
            reExports_ = allocator_->New<ArenaVector<ETSObjectType *>>(allocator_->Adapter());
        }
    }

    void EnsureReExportAliasInitialized() const
    {
        if (reExportAlias_ == nullptr) {
            reExportAlias_ = allocator_->New<ArenaMap<util::StringView, util::StringView>>(allocator_->Adapter());
        }
    }

    void EnsureTransitiveSupertypesInitialized() const
    {
        if (transitiveSupertypes_ == nullptr) {
            transitiveSupertypes_ = allocator_->New<ArenaSet<ETSObjectType *>>(allocator_->Adapter());
        }
    }
    bool CastWidening(TypeRelation *relation, Type *target, TypeFlag unboxFlags, TypeFlag wideningFlags);
    void IdenticalUptoTypeArguments(TypeRelation *relation, Type *other);
    void SubstitutePartialTypes(TypeRelation *relation, Type *other);
    void IsGenericSupertypeOf(TypeRelation *relation, ETSObjectType *source);
    void UpdateTypeProperty(varbinder::LocalVariable *const prop, PropertyType fieldType,
                            PropertyProcesser const &func);

    varbinder::LocalVariable *SearchFieldsDecls(util::StringView name, PropertySearchFlags flags) const;

    void SetCopiedTypeProperties(TypeRelation *relation, ETSObjectType *copiedType, ArenaVector<Type *> &&newTypeArgs,
                                 ETSObjectType *base);
    bool SubstituteTypeArgs(TypeRelation *relation, ArenaVector<Type *> &newTypeArgs, const Substitution *substitution);

    bool TryCastByte(TypeRelation *const relation, Type *const target);
    bool TryCastIntegral(TypeRelation *const relation, Type *const target);
    bool TryCastFloating(TypeRelation *const relation, Type *const target);
    bool TryCastUnboxable(TypeRelation *const relation, Type *const target);

    void CacheSupertypeTransitive(ETSObjectType *type);

    ir::TSTypeParameterDeclaration *GetTypeParams() const;

    ThreadSafeArenaAllocator *const allocator_;
    util::StringView const name_;
    util::StringView const internalName_;
    ir::AstNode *const declNode_;
    mutable ArenaVector<ETSObjectType *> *interfaces_;
    mutable ArenaVector<ETSObjectType *> *reExports_;
    mutable ArenaMap<util::StringView, util::StringView> *reExportAlias_;
    ETSObjectFlags flags_;
    ArenaVector<Type *> typeArguments_;
    ETSObjectType *superType_ {};
    ETSObjectType *enclosingType_ {};
    ETSObjectType *baseType_ {};

    // optimized subtyping
    mutable ArenaSet<ETSObjectType *> *transitiveSupertypes_;

    // for lazy properties instantiation
    TypeRelation *relation_ = nullptr;
    const ArenaSubstitution *effectiveSubstitution_ = nullptr;
    mutable bool propertiesInstantiated_ = false;
    mutable ArenaVector<Signature *> constructSignatures_;
    mutable PropertyHolder properties_;
};
}  // namespace ark::es2panda::checker

#endif /* TYPESCRIPT_TYPES_FUNCTION_TYPE_H */
