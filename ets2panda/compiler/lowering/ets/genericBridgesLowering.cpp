/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "genericBridgesLowering.h"

#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "compiler/lowering/util.h"
#include <sstream>

namespace ark::es2panda::compiler {

std::string GenericBridgesPhase::BuildMethodSignature(ir::ScriptFunction const *derivedFunction,
                                                      checker::Signature const *baseSignature,
                                                      std::vector<ir::AstNode *> &typeNodes) const noexcept
{
    std::ostringstream signature {};
    auto const &functionName = derivedFunction->Id()->Name().Mutf8();

    // Add method type prefix (get/set for accessors)
    if (derivedFunction->IsGetter()) {
        signature << "get ";
    } else if (derivedFunction->IsSetter()) {
        signature << "set ";
    }

    signature << functionName << '(';

    // Add parameters
    auto const &baseParameters = baseSignature->Params();
    auto const &derivedParameters = derivedFunction->Signature()->Params();
    auto const parameterNumber = baseParameters.size();

    for (std::size_t i = 0U; i < parameterNumber; ++i) {
        if (i != 0U) {
            signature << ", ";
        }

        signature << GetAdjustedParameterName(derivedFunction, derivedParameters[i]->Name().Utf8());

        // Add base parameter type
        typeNodes.emplace_back(
            Context()->AllocNode<ir::OpaqueTypeNode>(baseParameters[i]->TsType(), Context()->Allocator()));
        signature << ": @@T" << typeNodes.size();
    }

    signature << ")";

    // Add return type (not for setters)
    if (!derivedFunction->IsSetter()) {
        typeNodes.emplace_back(Context()->AllocNode<ir::OpaqueTypeNode>(
            const_cast<checker::Type *>(baseSignature->ReturnType()), Context()->Allocator()));
        signature << ": @@T" << typeNodes.size();
    }

    signature << " ";
    return signature.str();
}

std::string GenericBridgesPhase::BuildMethodBody(ir::ClassDefinition const *classDefinition,
                                                 ir::ScriptFunction const *derivedFunction,
                                                 std::vector<ir::AstNode *> &typeNodes) const noexcept
{
    std::ostringstream body {};
    auto const &functionName = derivedFunction->Id()->Name().Mutf8();

    // Add class type for casting
    typeNodes.emplace_back(Context()->AllocNode<ir::OpaqueTypeNode>(
        const_cast<checker::Type *>(classDefinition->TsType()), Context()->Allocator()));
    auto const classTypeIndex = typeNodes.size();

    if (derivedFunction->IsGetter()) {
        body << "{ return (this as @@T" << classTypeIndex << ")." << functionName << "; }";
    } else if (derivedFunction->IsSetter()) {
        body << "{ (this as @@T" << classTypeIndex << ")." << functionName
             << BuildSetterAssignment(derivedFunction, typeNodes) << "; }";
    } else {
        body << "{ return (this as @@T" << classTypeIndex << ")." << functionName
             << BuildMethodCall(derivedFunction, typeNodes) << "; }";
    }

    return body.str();
}

std::string GenericBridgesPhase::GetAdjustedParameterName(ir::ScriptFunction const *derivedFunction,
                                                          std::string_view parameterName) const noexcept
{
    // For setters, remove property prefix if present
    if (derivedFunction->IsSetter() && parameterName.rfind(compiler::Signatures::PROPERTY, 0) == 0) {
        return std::string(parameterName.substr(compiler::Signatures::PROPERTY.size()));
    }
    return std::string(parameterName);
}

std::string GenericBridgesPhase::BuildSetterAssignment(ir::ScriptFunction const *derivedFunction,
                                                       std::vector<ir::AstNode *> &typeNodes) const noexcept
{
    std::ostringstream assignment {};
    auto const &derivedParameters = derivedFunction->Signature()->Params();

    for (std::size_t i = 0U; i < derivedParameters.size(); ++i) {
        if (i != 0U) {
            assignment << ", ";
        }

        assignment << " = ";
        auto const &parameterName = derivedParameters[i]->Name().Utf8();
        auto const adjustedParameterName = GetAdjustedParameterName(derivedFunction, parameterName);
        assignment << adjustedParameterName;

        // Add derived parameter type for casting
        typeNodes.emplace_back(
            Context()->AllocNode<ir::OpaqueTypeNode>(derivedParameters[i]->TsType(), Context()->Allocator()));
        assignment << " as @@T" << typeNodes.size();
    }

    return assignment.str();
}

std::string GenericBridgesPhase::BuildMethodCall(ir::ScriptFunction const *derivedFunction,
                                                 std::vector<ir::AstNode *> &typeNodes) const noexcept
{
    std::ostringstream call {};
    call << "(";
    auto const &derivedParameters = derivedFunction->Signature()->Params();

    for (std::size_t i = 0U; i < derivedParameters.size(); ++i) {
        if (i != 0U) {
            call << ", ";
        }

        auto const &parameterName = derivedParameters[i]->Name().Utf8();
        auto const adjustedParameterName = GetAdjustedParameterName(derivedFunction, parameterName);
        call << adjustedParameterName;

        // Add derived parameter type for casting
        typeNodes.emplace_back(
            Context()->AllocNode<ir::OpaqueTypeNode>(derivedParameters[i]->TsType(), Context()->Allocator()));
        call << " as @@T" << typeNodes.size();
    }

    call << ")";
    return call.str();
}

std::string GenericBridgesPhase::CreateMethodDefinitionString(ir::ClassDefinition const *classDefinition,
                                                              checker::Signature const *baseSignature,
                                                              ir::ScriptFunction const *derivedFunction,
                                                              std::vector<ir::AstNode *> &typeNodes) const noexcept
{
    constexpr std::size_t SOURCE_CODE_LENGTH = 128U;
    std::string result {};
    result.reserve(2U * SOURCE_CODE_LENGTH);

    // Build method signature (name, parameters, return type)
    std::string signature = BuildMethodSignature(derivedFunction, baseSignature, typeNodes);

    // Build method body (implementation)
    std::string body = BuildMethodBody(classDefinition, derivedFunction, typeNodes);

    result = signature + body;
    return result;
}

void GenericBridgesPhase::AddGenericBridge(ir::ClassDefinition const *const classDefinition,
                                           ir::MethodDefinition *const methodDefinition,
                                           checker::Signature const *baseSignature,
                                           ir::ScriptFunction *const derivedFunction) const
{
    auto *parser = Context()->parser->AsETSParser();
    std::vector<ir::AstNode *> typeNodes {};
    ES2PANDA_ASSERT(baseSignature);
    typeNodes.reserve(2U * baseSignature->Params().size() + 2U);

    auto const sourceCode = CreateMethodDefinitionString(classDefinition, baseSignature, derivedFunction, typeNodes);

    auto *const bridgeMethodDefinition = parser->CreateFormattedClassMethodDefinition(sourceCode, typeNodes);
    ES2PANDA_ASSERT(bridgeMethodDefinition != nullptr);
    auto *const bridgeMethod = bridgeMethodDefinition->AsMethodDefinition();
    ES2PANDA_ASSERT(bridgeMethod != nullptr && methodDefinition->Id() != nullptr);

    auto configureModifiersAndFlags = [](auto *target, auto *source) {
        target->AddModifier(source->Modifiers());
        target->ClearModifier(ir::ModifierFlags::NATIVE | ir::ModifierFlags::ABSTRACT);
        target->AddAstNodeFlags(source->GetAstNodeFlags());
    };

    configureModifiersAndFlags(bridgeMethod, methodDefinition);
    bridgeMethod->SetParent(const_cast<ir::ClassDefinition *>(classDefinition));
    configureModifiersAndFlags(bridgeMethod->Function(), methodDefinition->Function());

    auto *varBinder = Context()->GetChecker()->VarBinder()->AsETSBinder();
    auto *scope = NearestScope(methodDefinition);
    auto scopeGuard = varbinder::LexicalScope<varbinder::Scope>::Enter(varBinder, scope);
    InitScopesPhaseETS::RunExternalNode(bridgeMethod, varBinder);

    varbinder::BoundContext boundCtx {varBinder->GetRecordTable(), const_cast<ir::ClassDefinition *>(classDefinition),
                                      true};
    varBinder->AsETSBinder()->ResolveReferencesForScopeWithContext(bridgeMethod, scope);

    auto *checker = Context()->GetChecker()->AsETSChecker();
    auto const checkerCtx =
        checker::SavedCheckerContext(checker,
                                     checker::CheckerStatus::IN_CLASS | checker::CheckerStatus::IGNORE_VISIBILITY |
                                         checker::CheckerStatus::IN_BRIDGE_TEST,
                                     classDefinition->TsType()->AsETSObjectType());
    auto scopeCtx = checker::ScopeContext(checker, scope);

    //  Note: we need to create and set function/method type here because the general method `BuildMethodSignature(...)`
    //  is not suitable for this case. Moreover, we have to save and restore proper type for `methodDefinition` because
    //  call to `BuildFunctionSignature(...)` breaks it!
    auto *methodType = methodDefinition->Id()->Variable()->TsType()->AsETSFunctionType();

    checker->BuildFunctionSignature(bridgeMethod->Function());
    bridgeMethod->Function()->Signature()->AddSignatureFlag(checker::SignatureFlags::BRIDGE);

    auto *const bridgeMethodType = checker->BuildMethodType(bridgeMethod->Function());
    checker->CheckIdenticalOverloads(methodType, bridgeMethodType, bridgeMethod, false,
                                     checker::TypeRelationFlag::NONE);
    bridgeMethod->SetTsType(bridgeMethodType);
    methodType->AddCallSignature(bridgeMethod->Function()->Signature());
    methodDefinition->Id()->Variable()->SetTsType(methodType);

    bridgeMethod->Function()->Body()->Check(
        checker);  // avoid checking overriding, this may fail if only return type is different.
}

void GenericBridgesPhase::ProcessScriptFunction(ir::ClassDefinition const *const classDefinition,
                                                ir::ScriptFunction *const baseFunction,
                                                ir::MethodDefinition *const derivedMethod,
                                                Substitutions const &substitutions) const
{
    auto *const checker = Context()->GetChecker()->AsETSChecker();
    auto *const relation = checker->Relation();

    auto const overrides = [checker, relation, classDefinition](checker::Signature const *source,
                                                                checker::Signature const *target) -> bool {
        checker::SavedCheckerContext const checkerCtx(
            checker, checker->Context().Status() | checker::CheckerStatus::IN_BRIDGE_TEST,
            classDefinition->TsType()->AsETSObjectType());
        checker::SavedTypeRelationFlagsContext const savedFlags(relation, checker::TypeRelationFlag::BRIDGE_CHECK);
        return relation->SignatureIsSupertypeOf(const_cast<checker::Signature *>(target),
                                                const_cast<checker::Signature *>(source));
    };

    //  We are not interested in functions that either don't have type parameters at all
    //  or have type parameters that are not modified in the derived class
    ES2PANDA_ASSERT(baseFunction);
    auto const *baseSignature1 = baseFunction->Signature()->Substitute(relation, &substitutions.baseConstraints);
    if (baseSignature1 == baseFunction->Signature()) {
        return;
    }

    auto *baseSignature2 = baseFunction->Signature()->Substitute(relation, &substitutions.derivedSubstitutions);
    if (baseSignature2 == baseFunction->Signature()) {
        return;
    }
    baseSignature2 = baseSignature2->Substitute(relation, &substitutions.derivedConstraints);

    ir::ScriptFunction *derivedFunction = nullptr;
    checker::ETSFunctionType const *methodType = derivedMethod->Id()->Variable()->TsType()->AsETSFunctionType();
    for (auto *signature : methodType->CallSignatures()) {
        signature = signature->Substitute(relation, &substitutions.derivedConstraints);
        // A special case is when the overriding function's return type is going to be unboxed.
        if ((overrides(signature, baseSignature1) || checker->HasSameAssemblySignature(baseSignature1, signature)) &&
            baseSignature1->ReturnType()->IsETSUnboxableObject() == signature->ReturnType()->IsETSUnboxableObject()) {
            //  NOTE: we already have custom-implemented method with the required bridge signature.
            //  Probably sometimes we will issue warning notification here...
            return;
        }

        if (overrides(baseSignature1, signature) && overrides(baseSignature2, baseSignature1)) {
            // This derived overload already handles the base union signature.
            return;
        }
        if ((derivedFunction == nullptr && overrides(signature, baseSignature2))) {
            //  NOTE: we don't care the possible case of mapping several derived function to the same bridge
            //  signature. Probably sometimes we will process it correctly or issue warning notification here...
            derivedFunction = signature->Function();
        }
    }

    if (derivedFunction != nullptr && derivedFunction != baseFunction) {
        AddGenericBridge(classDefinition, derivedMethod, baseSignature1, derivedFunction);
    }
}

void GenericBridgesPhase::MaybeAddGenericBridges(ir::ClassDefinition const *const classDefinition,
                                                 ir::MethodDefinition *const baseMethod,
                                                 ir::MethodDefinition *const derivedMethod,
                                                 Substitutions const &substitutions) const
{
    ProcessScriptFunction(classDefinition, baseMethod->Function(), derivedMethod, substitutions);
    for (auto *const overload : baseMethod->Overloads()) {
        ProcessScriptFunction(classDefinition, overload->Function(), derivedMethod, substitutions);
    }
}

static ir::MethodDefinition *FindBridgeCandidate(ir::ClassDefinition const *const classDefinition,
                                                 ir::MethodDefinition *baseMethod)
{
    auto const &classBody = classDefinition->Body();

    // Skip `static`, `final` and special methods...
    bool const isSpecialMethodKind =
        (baseMethod->Kind() != ir::MethodDefinitionKind::METHOD &&
         baseMethod->Kind() != ir::MethodDefinitionKind::GET && baseMethod->Kind() != ir::MethodDefinitionKind::SET);
    if (isSpecialMethodKind || baseMethod->IsStatic() || baseMethod->IsFinal() ||
        baseMethod->Id()->Name().Utf8().find("lambda_invoke-") != std::string_view::npos) {
        return nullptr;
    }

    // Check if the derived class has any possible overrides of this method
    auto isOverridePred = [&name = baseMethod->Id()->Name()](ir::AstNode const *node) -> bool {
        return node->IsMethodDefinition() && !node->IsStatic() && node->AsMethodDefinition()->Id()->Name() == name;
    };
    auto it = std::find_if(classBody.cbegin(), classBody.end(), isOverridePred);
    return it == classBody.cend() ? nullptr : (*it)->AsMethodDefinition();
}

static bool HasBridgeCandidates(ir::ClassDefinition const *const classDefinition,
                                ArenaVector<ir::AstNode *> const &items)
{
    for (auto *item : items) {
        if (item->IsMethodDefinition()) {
            auto method = item->AsMethodDefinition();
            auto derivedMethod = FindBridgeCandidate(classDefinition, method);
            if (derivedMethod != nullptr) {
                return true;
            }
        }
    }
    return false;
}

void GenericBridgesPhase::CreateGenericBridges(ir::ClassDefinition const *const classDefinition,
                                               Substitutions &substitutions,
                                               ArenaVector<ir::AstNode *> const &items) const
{
    //  Collect type parameters defaults/constraints in the derived class
    auto *checker = Context()->GetChecker()->AsETSChecker();
    substitutions.derivedConstraints = checker::Substitution {};

    auto const *const classType = classDefinition->TsType()->AsETSObjectType();
    auto const &typeParameters = classType->GetConstOriginalBaseType()->AsETSObjectType()->TypeArguments();
    for (auto *const parameter : typeParameters) {
        auto *const typeParameter = parameter->AsETSTypeParameter();
        checker->EmplaceSubstituted(&substitutions.derivedConstraints, typeParameter,
                                    typeParameter->GetConstraintType());
    }

    for (auto *item : items) {
        if (item->IsMethodDefinition()) {
            // Skip `static`, `final` and special methods...
            auto *const method = item->AsMethodDefinition();
            auto derivedMethod = FindBridgeCandidate(classDefinition, method);
            if (derivedMethod != nullptr) {
                MaybeAddGenericBridges(classDefinition, method, derivedMethod, substitutions);
            }
        }
    }
}

GenericBridgesPhase::Substitutions GenericBridgesPhase::GetSubstitutions(
    checker::ETSObjectType const *const objectType, ArenaVector<checker::Type *> const &typeParameters) const
{
    auto const &typeArguments = objectType->TypeArguments();
    auto const parameterNumber = typeParameters.size();
    ES2PANDA_ASSERT(parameterNumber == typeArguments.size());

    auto *checker = Context()->GetChecker()->AsETSChecker();
    Substitutions substitutions {};

    //  We need to check if the class derived from base generic class (or implementing generic interface)
    //  has either explicit class type substitutions or type parameters with narrowing constraints.
    for (std::size_t i = 0U; i < parameterNumber; ++i) {
        auto *const typeParameter = typeParameters[i]->AsETSTypeParameter();
        checker::Type *const typeArgument = typeArguments[i];

        //  Collect type parameters defaults/constraints in the base class
        //  and type argument substitutions in the derived class
        checker->EmplaceSubstituted(&substitutions.derivedSubstitutions, typeParameter, typeArgument);
        if (auto *const defaultType = typeParameter->GetDefaultType(); defaultType != nullptr) {
            checker->EmplaceSubstituted(&substitutions.baseConstraints, typeParameter, defaultType);
        } else {
            checker->EmplaceSubstituted(&substitutions.baseConstraints, typeParameter,
                                        typeParameter->GetConstraintType());
        }
    }

    return substitutions;
}

static std::unordered_set<checker::ETSObjectType *> CollectInterfacesTransitive(checker::ETSObjectType *type)
{
    std::unordered_set<checker::ETSObjectType *> collected;

    auto traverse = [&collected](auto &&self, checker::ETSObjectType *t) {
        if (t->TypeArguments().empty() || !collected.insert(t).second) {
            return;
        }
        for (auto itf : t->Interfaces()) {
            self(self, itf);
        }
    };
    for (auto itf : type->Interfaces()) {
        traverse(traverse, itf);
    }
    return collected;
}

void GenericBridgesPhase::ProcessInterfaces(ir::ClassDefinition *const classDefinition) const
{
    auto interfaces = CollectInterfacesTransitive(classDefinition->TsType()->AsETSObjectType());

    for (auto const *interfaceType : interfaces) {
        auto const &typeParameters = interfaceType->GetConstOriginalBaseType()->AsETSObjectType()->TypeArguments();
        if (!typeParameters.empty()) {
            auto const &interfaceBody = interfaceType->GetDeclNode()->AsTSInterfaceDeclaration()->Body()->Body();
            if (HasBridgeCandidates(classDefinition, interfaceBody)) {
                Substitutions substitutions = GetSubstitutions(interfaceType, typeParameters);
                ES2PANDA_ASSERT(interfaceType->GetDeclNode()->IsTSInterfaceDeclaration());
                CreateGenericBridges(classDefinition, substitutions, interfaceBody);
            }
        }
    }
}

void GenericBridgesPhase::ProcessClassWithGenericSupertype(const ir::ClassDefinition *const classDefinition,
                                                           const checker::ETSObjectType *const superType,
                                                           const ArenaVector<checker::Type *> &typeParameters) const
{
    //  Check if the class derived from base generic class has either explicit class type substitutions
    //  or type parameters with narrowing constraints.
    Substitutions substitutions = GetSubstitutions(superType, typeParameters);
    // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
    if (substitutions.derivedSubstitutions.empty()) {
        return;
    }

    // If it has, then probably the generic bridges should be created.
    auto const &superClassBody = superType->GetDeclNode()->AsClassDefinition()->Body();
    CreateGenericBridges(classDefinition, substitutions, superClassBody);
    const ArenaVector<checker::ETSObjectType *> &interfaces = superType->Interfaces();
    for (const checker::ETSObjectType *const interface : interfaces) {
        Substitutions interfaceSubstitutions =
            GetSubstitutions(interface, interface->GetConstOriginalBaseType()->AsETSObjectType()->TypeArguments());
        const auto &interfaceBody = interface->GetDeclNode()->AsTSInterfaceDeclaration()->Body()->Body();
        CreateGenericBridges(classDefinition, interfaceSubstitutions, interfaceBody);
    }
}

ir::ClassDefinition *GenericBridgesPhase::ProcessClassDefinition(ir::ClassDefinition *const classDefinition) const
{
    //  Check class interfaces.
    ProcessInterfaces(classDefinition);

    //  Check if the base class is a generic class.
    if (classDefinition->Super() == nullptr || classDefinition->Super()->TsType() == nullptr ||
        !classDefinition->Super()->TsType()->IsETSObjectType()) {
        return classDefinition;
    }

    const auto *superType = classDefinition->Super()->TsType()->AsETSObjectType();
    while (superType != nullptr) {
        auto const &typeParameters = superType->GetConstOriginalBaseType()->AsETSObjectType()->TypeArguments();
        if (typeParameters.empty()) {
            return classDefinition;
        }

        ProcessClassWithGenericSupertype(classDefinition, superType, typeParameters);

        superType = superType->SuperType();
    }

    return classDefinition;
}

bool GenericBridgesPhase::PerformForProgram(parser::Program *program)
{
    program->Ast()->TransformChildrenRecursively(
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [this](ir::AstNode *ast) -> ir::AstNode * {
            if (ast->IsClassDefinition()) {
                return ProcessClassDefinition(ast->AsClassDefinition());
            }
            return ast;
        },
        Name());

    return true;
}

}  // namespace ark::es2panda::compiler
