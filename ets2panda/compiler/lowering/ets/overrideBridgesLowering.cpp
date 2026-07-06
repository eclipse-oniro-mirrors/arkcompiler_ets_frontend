/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "overrideBridgesLowering.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <ostream>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include "checker/ETSchecker.h"
#include "checker/types/ets/etsFunctionType.h"
#include "checker/types/ets/etsObjectType.h"
#include "checker/types/signature.h"
#include "checker/types/type.h"
#include "checker/types/typeRelation.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "ir/astNode.h"
#include "ir/base/classDefinition.h"
#include "ir/base/methodDefinition.h"
#include "libarkbase/utils/logger.h"
#include "parser/ETSparser.h"
#include "util/eheap.h"
#include "util/es2pandaMacros.h"
#include "util/ustring.h"
#include "varbinder/ETSBinder.h"
#include "varbinder/variable.h"

namespace ark::es2panda::compiler {

namespace {

using namespace checker;

class Locator {
public:
    explicit Locator(public_lib::Context *ctx) : ctx_(ctx)
    {
        ES2PANDA_ASSERT(ctx != nullptr);
        ES2PANDA_ASSERT(ctx_->parser->IsETSParser());
        ES2PANDA_ASSERT(ctx_->GetChecker()->IsETSChecker());
        ES2PANDA_ASSERT(ctx_->GetChecker()->VarBinder()->IsETSBinder());
    }

    DEFAULT_COPY_SEMANTIC(Locator);
    DEFAULT_MOVE_SEMANTIC(Locator);

    [[nodiscard]] parser::ETSParser *Parser() const
    {
        return ctx_->parser->AsETSParser();
    }

    [[nodiscard]] ETSChecker *Checker() const
    {
        return ctx_->GetChecker()->AsETSChecker();
    }

    [[nodiscard]] varbinder::ETSBinder *Binder() const
    {
        return Checker()->VarBinder()->AsETSBinder();
    }

    [[nodiscard]] ArenaAllocator *Allocator() const
    {
        return Checker()->Allocator();
    }

    [[nodiscard]] TypeRelation *Relation() const
    {
        return ctx_->GetChecker()->Relation();
    }

    template <typename T, typename... Args>
    [[nodiscard]] T *AllocNode(Args &&...args) const
    {
        return ctx_->AllocNode<T>(std::forward<Args>(args)...);
    }

    [[nodiscard]] ir::OpaqueTypeNode *AllocOpaqueTypeNode(Type const *type) const
    {
        return ctx_->AllocNode<ir::OpaqueTypeNode>(const_cast<checker::Type *>(type), Allocator());
    }

    [[nodiscard]] auto SaveContextForOverrideRelation(ETSObjectType const *classType) const
    {
        return std::make_pair(
            SavedCheckerContext(ctx_->GetChecker(), ctx_->GetChecker()->Context().Status(), classType),
            SavedTypeRelationFlagsContext(Relation(), TypeRelationFlag::OVERRIDING_CONTEXT));
    }

private:
    public_lib::Context *ctx_;
};

template <auto GetGlobalType>
bool IsTypeEqualToGlobal(Locator const &locator, Type const *type)
{
    return locator.Relation()->IsIdenticalTo(type, (locator.Checker()->*GetGlobalType)());
}

bool IsBridgeCandidateType(Type const *type)
{
    ES2PANDA_ASSERT(type != nullptr);

    return (type->IsETSObjectType() && type->AsETSObjectType()->IsBoxedPrimitive()) || type->IsETSUndefinedType() ||
           type->IsETSVoidType();
}

bool RequiresBridgeConversion(Locator const &locator, Type const *sourceType, Type const *targetType)
{
    ES2PANDA_ASSERT(sourceType != nullptr);
    ES2PANDA_ASSERT(targetType != nullptr);
    ES2PANDA_ASSERT(locator.Relation() != nullptr);

    auto const *effectiveSourceType =
        sourceType->IsETSVoidType() ? locator.Checker()->GlobalETSUndefinedType() : sourceType;

    if (targetType->IsETSAnyType() && effectiveSourceType->IsETSUndefinedType()) {
        return true;
    }

    if (!IsBridgeCandidateType(sourceType) && !effectiveSourceType->IsETSUndefinedType()) {
        return false;
    }

    if (locator.Relation()->IsIdenticalTo(effectiveSourceType, targetType)) {
        return false;
    }

    if (locator.Relation()->IsSupertypeOf(targetType, effectiveSourceType)) {
        return true;
    }

    return false;
}

bool IsNonIdenticalOverride(Locator const &locator, Signature const *super, Signature const *sub)
{
    ES2PANDA_ASSERT(super != nullptr);
    ES2PANDA_ASSERT(sub != nullptr);
    using SF = SignatureFlags;
    if ((super->Flags() & (SF::CONSTRUCT | SF::PRIVATE | SF::STATIC | SF::FINAL)) != 0U) {
        return false;
    }
    if ((sub->Flags() & (SF::CONSTRUCT | SF::PRIVATE | SF::STATIC)) != 0U) {
        return false;
    }
    if (super->Params().size() != sub->Params().size()) {
        return false;
    }
    auto const savedContext = locator.SaveContextForOverrideRelation(super->Owner());
    if (locator.Relation()->SignatureIsIdenticalTo(super, sub)) {
        return false;
    }

    return locator.Relation()->SignatureIsSupertypeOf(super, sub);
}

bool IsVoidOrUndefinedToAnyBridgeOverride(Locator const &locator, Signature const *superSign, Signature const *subSign)
{
    ES2PANDA_ASSERT(superSign != nullptr);
    ES2PANDA_ASSERT(subSign != nullptr);

    if (superSign->Params().size() != subSign->Params().size()) {
        return false;
    }

    if (!superSign->ReturnType()->IsETSAnyType()) {
        return false;
    }

    auto const *subReturn = subSign->ReturnType();
    if (!(subReturn->IsETSVoidType() || subReturn->IsETSUndefinedType())) {
        return false;
    }

    auto const savedContext = std::make_pair(
        SavedCheckerContext(locator.Checker(), locator.Checker()->Context().Status(), superSign->Owner()),
        SavedTypeRelationFlagsContext(locator.Relation(),
                                      TypeRelationFlag::OVERRIDING_CONTEXT | TypeRelationFlag::NO_RETURN_TYPE_CHECK));

    return locator.Relation()->SignatureIsSupertypeOf(superSign, subSign);
}

class SignatureOverrideAnalyzer {
public:
    explicit SignatureOverrideAnalyzer(Locator const &locator, Signature const *signature)
        : locator_(locator), signature_(signature)
    {
    }

    DEFAULT_COPY_SEMANTIC(SignatureOverrideAnalyzer);
    DEFAULT_MOVE_SEMANTIC(SignatureOverrideAnalyzer);

    void Process(ETSFunctionType const *method)
    {
        std::vector<Signature const *> foundInClass;
        for (auto const *superSign : method->CallSignatures()) {
            if (IsRequiresBridgeConversion(superSign)) {
                auto const savedContext = locator_.SaveContextForOverrideRelation(superSign->Owner());
                auto const pred = [this, superSign](Signature const *sign) {
                    return locator_.Relation()->SignatureIsIdenticalTo(superSign, sign);
                };
                if (std::none_of(foundSignatures_.begin(), foundSignatures_.end(), pred)) {
                    foundInClass.push_back(superSign);
                }
            }
        }
        foundSignatures_.insert(foundSignatures_.end(), foundInClass.begin(), foundInClass.end());
    }

    bool IsRequiresBridgeConversion(Signature const *superSign)
    {
        ES2PANDA_ASSERT(superSign != nullptr);
        if (!IsNonIdenticalOverride(locator_, superSign, signature_) &&
            !IsVoidOrUndefinedToAnyBridgeOverride(locator_, superSign, signature_)) {
            return false;
        }

        // Check parameter types
        ES2PANDA_ASSERT(superSign->Params().size() == signature_->Params().size());

        for (size_t i = 0; i < superSign->Params().size() && i < signature_->Params().size(); ++i) {
            auto const superType = superSign->Params()[i]->TsType();
            auto const subType = signature_->Params()[i]->TsType();
            if (RequiresBridgeConversion(locator_, superType, subType)) {
                return true;
            }
        }

        // Check return type
        auto const subType = signature_->ReturnType();
        auto const superType = superSign->ReturnType();
        return RequiresBridgeConversion(locator_, subType, superType);
    }

    std::vector<Signature const *> const &FoundSignatures() const
    {
        return foundSignatures_;
    }

private:
    Locator locator_;
    Signature const *signature_;
    std::vector<Signature const *> foundSignatures_ = {};
};

using MethodDefinitionHandler = std::function<bool(ir::MethodDefinition *)>;

// Iterate over non-static, non-private instance methods in class definition
void ForEachByMethod(ir::ClassDefinition *classDefinition, ArenaAllocator *allocator, MethodDefinitionHandler func)
{
    // Copy method pointers to avoid iterator invalidation when bridges are added
    ArenaVector<ir::MethodDefinition *> methods(allocator->Adapter());
    for (auto *member : classDefinition->Body()) {
        if (!member->IsMethodDefinition()) {
            continue;
        }
        auto *method = member->AsMethodDefinition();
        if ((!method->IsMethod() && !method->IsGetter() && !method->IsSetter()) || method->IsStatic() ||
            method->IsPrivate()) {
            continue;
        }
        methods.push_back(method);
    }

    for (auto *method : methods) {
        if (func(method)) {
            return;
        }
    }
}

// Builder for generating bridge method signature and body
class BridgeMethodBuilder {
public:
    BridgeMethodBuilder(Locator const &locator, checker::Signature const *superSign, checker::Signature const *subSign,
                        ir::ClassDefinition *classDefinition)
        : locator_(locator),
          superSign_(superSign),
          subSign_(subSign),
          functionName_(subSign->Function()->Id()->Name().Utf8())
    {
        ES2PANDA_ASSERT(superSign_ != nullptr);
        ES2PANDA_ASSERT(subSign_ != nullptr);
        ES2PANDA_ASSERT(classDefinition != nullptr);
        classType_ = classDefinition->TsType();
    }

    std::vector<ir::AstNode *> Build(std::ostream &out)
    {
        std::vector<ir::AstNode *> typeNodes {};
        std::swap(typeNodes_, typeNodes);
        typeNodes_.reserve(2U * superSign_->Params().size() + 2U);
        typeNodes_.emplace_back(locator_.AllocOpaqueTypeNode(classType_));
        classTypeIndex_ = typeNodes_.size();
        BuildSignature(out);
        BuildBody(out);
        std::swap(typeNodes_, typeNodes);
        return typeNodes;
    }

private:
    // Build method signature (name, parameters, return type)
    void BuildSignature(std::ostream &signature)
    {
        AppendMethodPrefix(signature);
        signature << functionName_ << '(';
        AppendParamListWithTypes(signature);
        signature << ")";
        AppendReturnType(signature);
        signature << " ";
    }

    // Build method body (implementation that casts this and calls subclass method)
    void BuildBody(std::ostream &body)
    {
        if (superSign_->Function()->IsGetter()) {
            AppendGetterBody(body);
        } else if (superSign_->Function()->IsSetter()) {
            AppendSetterBody(body);
        } else {
            AppendRegularBody(body);
        }
    }

    // Append method type prefix (get/set for accessors)
    void AppendMethodPrefix(std::ostream &out) const
    {
        if (superSign_->Function()->IsGetter()) {
            out << "get ";
        } else if (superSign_->Function()->IsSetter()) {
            out << "set ";
        }
    }

    // Append parameter list with types from super signature
    void AppendParamListWithTypes(std::ostream &out)
    {
        auto const &superParams = superSign_->Params();
        for (size_t i = 0; i < superParams.size(); ++i) {
            if (i > 0) {
                out << ", ";
            }

            out << superParams[i]->Name().Utf8();

            // Add super parameter type
            typeNodes_.emplace_back(locator_.AllocOpaqueTypeNode(superParams[i]->TsType()));
            out << ": @@T" << typeNodes_.size();
        }
    }

    // Append return type (not for setters)
    void AppendReturnType(std::ostream &out)
    {
        if (!superSign_->Function()->IsSetter()) {
            typeNodes_.emplace_back(locator_.AllocOpaqueTypeNode(superSign_->ReturnType()));
            out << ": @@T" << typeNodes_.size();
        }
    }

    // Generate getter body
    void AppendGetterBody(std::ostream &out)
    {
        out << "{ return ";
        AppendReturnConversion(
            out, [this](std::ostream &os) { os << "(this as @@T" << classTypeIndex_ << ")." << functionName_; });
        out << "; }";
    }

    // Generate setter body
    void AppendSetterBody(std::ostream &out)
    {
        out << "{ (this as @@T" << classTypeIndex_ << ")." << functionName_;
        AppendParamListForSetter(out);
        out << "; }";
    }

    // Generate regular method body
    void AppendRegularBody(std::ostream &out)
    {
        out << "{ return ";
        AppendReturnConversion(out, [this](std::ostream &os) {
            os << "(this as @@T" << classTypeIndex_ << ")." << functionName_ << "(";
            AppendParamListForRegular(os);
            os << ")";
        });
        out << "; }";
    }

    // Generate parameter conversion expression
    void AppendParamConversion(std::ostream &out, varbinder::Variable const *superParam, Type const *subParamType)
    {
        auto const *superParamType = superParam->TsType();
        if (RequiresBridgeConversion(locator_, superParamType, subParamType)) {
            ES2PANDA_ASSERT(IsBridgeCandidateType(superParamType));
            typeNodes_.emplace_back(locator_.AllocOpaqueTypeNode(subParamType));
            out << superParam->Name().Utf8() << " as @@T" << typeNodes_.size();
        } else {
            // No conversion needed
            out << superParam->Name().Utf8();
        }
    }

    // Generate return value conversion expression
    template <typename ExprWriter>
    void AppendReturnConversion(std::ostream &out, ExprWriter writeExpr)
    {
        auto const *superReturn = superSign_->ReturnType();
        auto const *subReturn = subSign_->ReturnType();
        if (RequiresBridgeConversion(locator_, subReturn, superReturn)) {
            ES2PANDA_ASSERT(IsBridgeCandidateType(subReturn));
            typeNodes_.emplace_back(locator_.AllocOpaqueTypeNode(superReturn));
            writeExpr(out);
            out << " as @@T" << typeNodes_.size();
        } else {
            writeExpr(out);
        }
    }

    // Append parameter assignments for setter
    void AppendParamListForSetter(std::ostream &out)
    {
        auto const &superParams = superSign_->Params();
        auto const &subParams = subSign_->Params();

        for (size_t i = 0; i < superParams.size(); ++i) {
            out << " = ";
            ES2PANDA_ASSERT(i < subParams.size());
            AppendParamConversion(out, superParams[i], subParams[i]->TsType());
            if (i < superParams.size() - 1) {
                out << ", ";
            }
        }
    }

    // Append parameter list for regular method call
    void AppendParamListForRegular(std::ostream &out)
    {
        auto const &superParams = superSign_->Params();
        auto const &subParams = subSign_->Params();

        for (size_t i = 0; i < superParams.size(); ++i) {
            if (i > 0) {
                out << ", ";
            }
            ES2PANDA_ASSERT(i < subParams.size());
            AppendParamConversion(out, superParams[i], subParams[i]->TsType());
        }
    }

    // Data members
    Locator const &locator_;
    Type *classType_ {};
    checker::Signature const *const superSign_;
    checker::Signature const *const subSign_;
    std::vector<ir::AstNode *> typeNodes_ {};
    std::string const functionName_;
    size_t classTypeIndex_ {0};
};

// for LOG(DEBUG)
[[maybe_unused]] std::ostream &operator<<(std::ostream &s, ir::MethodDefinition const &method)
{
    s << method.Id()->Name();
    std::stringstream ss;
    method.Function()->Signature()->ToString(ss, nullptr, true);
    return s << ss.rdbuf();
}

bool ShouldSkipBridgeCreation([[maybe_unused]] Locator const &locator, ir::ClassDefinition const *classDefinition,
                              ir::MethodDefinition const *originalMethod, checker::Signature const *superSign)
{
    auto const &methodName = superSign->Function()->Id()->Name();
    for (auto const *member : classDefinition->Body()) {
        if (!member->IsMethodDefinition()) {
            continue;
        }
        auto const *method = member->AsMethodDefinition();
        if (method == originalMethod) {
            continue;
        }
        if (method->Id()->Name() != methodName) {
            continue;
        }
        auto const *methodType = method->TsType();
        if (methodType == nullptr || !methodType->IsETSFunctionType()) {
            continue;
        }
        auto const *funcType = methodType->AsETSFunctionType();
        for (auto const *sign : funcType->CallSignatures()) {
            if (ETSChecker::HasSameAssemblySignature(sign, superSign)) {
                return true;
            }
        }
    }

    return false;
}

[[nodiscard]] ir::MethodDefinition *BuildBridgeMethod(Locator const &locator, ir::ClassDefinition *classDefinition,
                                                      checker::Signature const *superSign,
                                                      checker::Signature const *subSign)
{
    std::ostringstream sourceCode;
    BridgeMethodBuilder builder(locator, superSign, subSign, classDefinition);
    auto typeNodes = builder.Build(sourceCode);

    std::string const sourceStr = sourceCode.str();
    auto *const bridgeMethodDefinition = locator.Parser()->CreateFormattedClassMethodDefinition(sourceStr, typeNodes);
    ES2PANDA_ASSERT(bridgeMethodDefinition != nullptr);
    return bridgeMethodDefinition->AsMethodDefinition();
}

void ConfigureBridgeMethod(ir::MethodDefinition *bridgeMethod, ir::ClassDefinition const *classDefinition,
                           checker::Signature const *subSign)
{
    auto const configureModifiersAndFlags = [](auto *target, auto *source) {
        target->AddModifier(source->Modifiers());
        target->ClearModifier(ir::ModifierFlags::NATIVE | ir::ModifierFlags::ABSTRACT);
        target->AddAstNodeFlags(source->GetAstNodeFlags());
    };

    ir::MethodDefinition const *sourceMethod = nullptr;
    for (auto const *member : classDefinition->Body()) {
        if (!member->IsMethodDefinition()) {
            continue;
        }
        auto const *method = member->AsMethodDefinition();
        if (method->Id()->Name() != subSign->Function()->Id()->Name()) {
            continue;
        }
        auto const *methodType = method->TsType();
        if (methodType == nullptr || !methodType->IsETSFunctionType()) {
            continue;
        }
        auto const *funcType = methodType->AsETSFunctionType();
        for (auto *sign : funcType->CallSignatures()) {
            if (sign->HasSignatureFlag(checker::SignatureFlags::BRIDGE)) {
                continue;
            }
            sourceMethod = method;
            break;
        }
        if (sourceMethod != nullptr) {
            break;
        }
    }

    if (sourceMethod != nullptr) {
        configureModifiersAndFlags(bridgeMethod, sourceMethod);
        configureModifiersAndFlags(bridgeMethod->Function(), sourceMethod->Function());
    }

    bridgeMethod->SetParent(const_cast<ir::ClassDefinition *>(classDefinition));
    bridgeMethod->Function()->AddModifier(ir::ModifierFlags::PUBLIC);
}

void InitializeBridgeMethod(Locator const &locator, ir::ClassDefinition *classDefinition,
                            ir::MethodDefinition *bridgeMethod, ir::MethodDefinition *originalMethod)
{
    ES2PANDA_ASSERT(originalMethod != nullptr);

    auto *const checker = locator.Checker();
    auto *const varBinder = locator.Binder();

    auto *const scope = classDefinition->Scope();
    auto scopeGuard = varbinder::LexicalScope<varbinder::Scope>::Enter(varBinder, scope);
    InitScopesPhaseETS::RunExternalNode(bridgeMethod, varBinder);

    varbinder::BoundContext boundCtx {varBinder->GetRecordTable(), classDefinition, true};
    varBinder->AsETSBinder()->ResolveReferencesForScopeWithContext(bridgeMethod, scope);

    auto const savedCtx = checker::SavedCheckerContext(checker,
                                                       checker->Context().Status() | checker::CheckerStatus::IN_CLASS |
                                                           checker::CheckerStatus::IN_BRIDGE_TEST,
                                                       classDefinition->TsType()->AsETSObjectType());
    auto const scopeCtx = checker::ScopeContext(checker, scope);

    checker->BuildFunctionSignature(bridgeMethod->Function());
    bridgeMethod->Function()->Signature()->AddSignatureFlag(checker::SignatureFlags::BRIDGE);

    auto *const bridgeMethodType = checker->BuildMethodType(bridgeMethod->Function());

    // Get original method type
    auto *originalMethodType = const_cast<checker::ETSFunctionType *>(originalMethod->TsType()->AsETSFunctionType());
    auto const *bridgeSignature = bridgeMethod->Function()->Signature();
    auto const hasExistingAssemblySignature =
        std::any_of(originalMethodType->CallSignatures().begin(), originalMethodType->CallSignatures().end(),
                    [bridgeSignature](checker::Signature const *existing) {
                        return ETSChecker::HasSameAssemblySignature(existing, bridgeSignature);
                    });
    if (!hasExistingAssemblySignature) {
        // Check for identical overloads as in genericBridgesLowering
        checker->CheckIdenticalOverloads(originalMethodType, bridgeMethodType, bridgeMethod, false,
                                         checker::TypeRelationFlag::NONE);
    }

    bridgeMethod->SetTsType(bridgeMethodType);

    bridgeMethod->Function()->Body()->Check(checker);

    if (!hasExistingAssemblySignature) {
        // Add bridge signature to original method's type when it is not already tracked there.
        originalMethodType->AddCallSignature(bridgeMethod->Function()->Signature());
    }

    LOG(DEBUG, ES2PANDA) << "[CreateOverrideBridges] Bridge created: `" << classDefinition->Ident()->Name() << "."
                         << *bridgeMethod << "` for `" << *originalMethod << "`";
}

void CreateBridge(Locator const &locator, ir::ClassDefinition *classDefinition, ir::MethodDefinition *originalMethod,
                  checker::Signature const *superSign, checker::Signature const *subSign)
{
    if (ShouldSkipBridgeCreation(locator, classDefinition, originalMethod, superSign)) {
        return;
    }

    std::vector<ir::AstNode *> typeNodes;
    auto *bridgeMethod = BuildBridgeMethod(locator, classDefinition, superSign, subSign);
    ES2PANDA_ASSERT(bridgeMethod != nullptr);
    ConfigureBridgeMethod(bridgeMethod, classDefinition, subSign);

    InitializeBridgeMethod(locator, classDefinition, bridgeMethod, originalMethod);
}

void AllSignaturesNeedingBridge(Locator const &locator, ETSObjectType const *subType, Signature const *subSign,
                                std::function<void(Signature const *)> func)
{
    ES2PANDA_ASSERT(subSign->Function() != nullptr);
    SignatureOverrideAnalyzer analyzer(locator, subSign);
    std::vector<Signature const *> fallbackSigns;
    auto appendUniqueBridgeSign = [](std::vector<Signature const *> &target, Signature const *candidate) {
        if (std::any_of(target.begin(), target.end(), [candidate](Signature const *existing) {
                return existing == candidate || ETSChecker::HasSameAssemblySignature(existing, candidate);
            })) {
            return;
        }
        target.push_back(candidate);
    };

    // Helper lambda to extract and process method signatures from a type
    // Looks up the method by name and processes all its signatures through the analyzer
    auto processMethodSignaturesFrom = [&analyzer, &fallbackSigns, &appendUniqueBridgeSign, &locator,
                                        subSign](ETSObjectType const *type) noexcept -> void {
        auto const &it = type->InstanceMethods().find(subSign->Function()->Id()->Name());
        if (it == type->InstanceMethods().end()) {
            return;
        }
        if (!it->second->TsType()->IsETSFunctionType()) {
            return;
        }
        auto const *funcType = it->second->TsType()->AsETSFunctionType();
        analyzer.Process(funcType);
        for (auto const *superSign : funcType->CallSignatures()) {
            if (!IsVoidOrUndefinedToAnyBridgeOverride(locator, superSign, subSign)) {
                continue;
            }
            appendUniqueBridgeSign(fallbackSigns, superSign);
        }
    };

    // Helper lambda to process interface methods from a type's interfaces
    auto processInterfaces = [&processMethodSignaturesFrom](ETSObjectType const *type) noexcept -> void {
        if (type->Interfaces().empty()) {
            return;  // Early exit if no interfaces
        }
        for (auto const *interfaceType : type->Interfaces()) {
            ES2PANDA_ASSERT(interfaceType->IsInterface());
            processMethodSignaturesFrom(interfaceType);
        }
    };

    // Process superclass hierarchy AND their interfaces
    for (auto super = subType->SuperType(); super != nullptr; super = super->SuperType()) {
        // Process methods in the superclass
        processMethodSignaturesFrom(super);

        // Process interfaces implemented by this superclass
        processInterfaces(super);
    }

    // Process interfaces implemented directly by the current class
    processInterfaces(subType);

    std::vector<Signature const *> bridgeSigns;
    bridgeSigns.reserve(analyzer.FoundSignatures().size() + fallbackSigns.size());
    for (auto const *sign : analyzer.FoundSignatures()) {
        appendUniqueBridgeSign(bridgeSigns, sign);
    }
    for (auto const *sign : fallbackSigns) {
        appendUniqueBridgeSign(bridgeSigns, sign);
    }

    for (auto const *sign : bridgeSigns) {
        func(sign);
    }
}

// Process all signatures of a method and create bridges for overridden methods
void ProcessFunction(Locator const &locator, ir::ClassDefinition *classDefinition, ETSObjectType *clsType,
                     [[maybe_unused]] ir::MethodDefinition *methodDef, ETSFunctionType *method)
{
    for (auto const *const subSign : method->CallSignatures()) {
        if (subSign->HasSignatureFlag(checker::SignatureFlags::BRIDGE)) {
            continue;
        }

        AllSignaturesNeedingBridge(locator, clsType, subSign,
                                   [&locator, classDefinition, methodDef, subSign](Signature const *superSign) -> void {
                                       CreateBridge(locator, classDefinition, methodDef, superSign, subSign);
                                   });
    }
}

// Process a single method definition for override bridge creation
void Process(Locator const &locator, ir::ClassDefinition *classDefinition, ir::MethodDefinition *method)
{
    ES2PANDA_ASSERT(classDefinition != nullptr);
    ES2PANDA_ASSERT(classDefinition->TsType() != nullptr);
    ES2PANDA_ASSERT(classDefinition->TsType()->IsETSObjectType());

    ES2PANDA_ASSERT(method != nullptr);
    ES2PANDA_ASSERT(method->TsType() != nullptr);
    ES2PANDA_ASSERT(method->TsType()->IsETSFunctionType());

    ProcessFunction(locator, classDefinition, classDefinition->TsType()->AsETSObjectType(), method,
                    method->TsType()->AsETSFunctionType());
}

// Process all methods in a class definition for override bridge creation
// Returns the class definition for chaining
ir::ClassDefinition *Process(Locator const &locator, ir::ClassDefinition *classDefinition)
{
    ForEachByMethod(classDefinition, locator.Allocator(), [&locator, classDefinition](ir::MethodDefinition *method) {
        auto const &name = method->Id()->Name();
        if (name.Utf8().find("lambda_invoke-") != std::string_view::npos) {
            return false;
        }
        Process(locator, classDefinition, method);
        return false;
    });

    return classDefinition;
}

}  // namespace

bool OverrideBridgesPhase::PerformForProgram(parser::Program *program)
{
    Locator const locator(Context());
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [&locator](ir::AstNode *node) -> ir::AstNode * {
            if (node->IsClassDefinition()) {
                return Process(locator, node->AsClassDefinition());
            }
            return node;
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
