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

#include "checker/ETSchecker.h"

#include "checker/types/ets/etsAsyncFuncReturnType.h"
#include "checker/types/ets/etsEnumType.h"
#include "checker/types/ets/etsResizableArrayType.h"
#include "checker/types/globalTypesHolder.h"
#include "checker/types/type.h"
#include "ir/statements/annotationDeclaration.h"

#include <compiler/lowering/phase.h>

namespace ark::es2panda::checker {

ByteType *ETSChecker::CreateByteType(int8_t value)
{
    return ProgramAllocator()->New<ByteType>(value);
}

ETSBooleanType *ETSChecker::CreateETSBooleanType(bool value)
{
    return ProgramAllocator()->New<ETSBooleanType>(value);
}

DoubleType *ETSChecker::CreateDoubleType(double value)
{
    return ProgramAllocator()->New<DoubleType>(value);
}

FloatType *ETSChecker::CreateFloatType(float value)
{
    return ProgramAllocator()->New<FloatType>(value);
}

IntType *ETSChecker::CreateIntType(int32_t value)
{
    return ProgramAllocator()->New<IntType>(value);
}

LongType *ETSChecker::CreateLongType(int64_t value)
{
    return ProgramAllocator()->New<LongType>(value);
}

ShortType *ETSChecker::CreateShortType(int16_t value)
{
    return ProgramAllocator()->New<ShortType>(value);
}

CharType *ETSChecker::CreateCharType(char16_t value)
{
    return ProgramAllocator()->New<CharType>(value);
}

ETSBigIntType *ETSChecker::CreateETSBigIntLiteralType(util::StringView value)
{
    return ProgramAllocator()->New<ETSBigIntType>(ProgramAllocator(), GlobalBuiltinETSBigIntType(), Relation(), value);
}

ETSStringType *ETSChecker::CreateETSStringLiteralType(util::StringView value)
{
    return ProgramAllocator()->New<ETSStringType>(ProgramAllocator(), GlobalBuiltinETSStringType(), Relation(), value);
}

ETSResizableArrayType *ETSChecker::CreateETSMultiDimResizableArrayType(Type *element, size_t dimSize)
{
    ETSObjectType *type = GlobalBuiltinETSResizableArrayType();
    ES2PANDA_ASSERT(type != nullptr);
    ETSResizableArrayType *const arrayType = type->AsETSResizableArrayType();
    ES2PANDA_ASSERT(arrayType->TypeArguments().size() == 1U);

    Type *baseArrayType = element;

    for (size_t dim = 0; dim < dimSize; ++dim) {
        auto tmpSubstitution = Substitution {};
        EmplaceSubstituted(&tmpSubstitution, arrayType->TypeArguments()[0]->AsETSTypeParameter()->GetOriginal(),
                           MaybeBoxType(baseArrayType));
        baseArrayType = arrayType->Substitute(Relation(), &tmpSubstitution);
    }
    return baseArrayType->AsETSResizableArrayType();
}

ETSResizableArrayType *ETSChecker::CreateETSResizableArrayType(Type *element)
{
    ETSObjectType *type = GlobalBuiltinETSResizableArrayType();
    ES2PANDA_ASSERT(type != nullptr);
    ETSResizableArrayType *arrayType = type->AsETSResizableArrayType();
    ES2PANDA_ASSERT(arrayType->TypeArguments().size() == 1U);

    auto substitution = Substitution {};
    EmplaceSubstituted(&substitution, arrayType->TypeArguments()[0]->AsETSTypeParameter()->GetOriginal(),
                       MaybeBoxType(element));
    return arrayType->Substitute(Relation(), &substitution);
}

ETSArrayType *ETSChecker::CreateETSArrayType(Type *elementType, bool isCachePolluting)
{
    auto res = arrayTypes_.find({elementType, isCachePolluting});
    if (res != arrayTypes_.end()) {
        return res->second;
    }

    auto *arrayType = ProgramAllocator()->New<ETSArrayType>(elementType);

    ES2PANDA_ASSERT(arrayType != nullptr);
    std::stringstream ss;
    arrayType->ToAssemblerTypeWithRank(ss);
    // arrayType->SetAssemblerName(util::UString(ss.str(), ProgramAllocator()).View());

    auto it = arrayTypes_.insert({{elementType, isCachePolluting}, arrayType});
    if (it.second && (!elementType->IsTypeParameter() || !elementType->IsETSTypeParameter())) {
        CreateBuiltinArraySignature(arrayType, arrayType->Rank());
    }

    return arrayType;
}

Type *ETSChecker::CreateETSUnionType(Span<Type *const> constituentTypes)
{
    if (constituentTypes.empty()) {
        return nullptr;
    }

    ArenaVector<Type *> newConstituentTypes(ProgramAllocator()->Adapter());
    newConstituentTypes.assign(constituentTypes.begin(), constituentTypes.end());

    ETSUnionType::NormalizeTypes(Relation(), newConstituentTypes);
    if (newConstituentTypes.size() == 1) {
        return newConstituentTypes[0];
    }
    auto *un = ProgramAllocator()->New<ETSUnionType>(this, std::move(newConstituentTypes));
    auto ut = un->GetAssemblerType().Mutf8();
    if (std::count_if(ut.begin(), ut.end(), [](char c) { return c == ','; }) > 0) {
        unionAssemblerTypes_.insert(un->GetAssemblerType());
    }
    return un;
}

ETSTypeAliasType *ETSChecker::CreateETSTypeAliasType(util::StringView name, const ir::AstNode *declNode,
                                                     bool isRecursive)
{
    return ProgramAllocator()->New<ETSTypeAliasType>(this, name, declNode, isRecursive);
}

// the "inference" routine of the generic type inference _mutates_ arrow function signature
// even in the case when the arrow signature is computed from the arrow function expression
// Conservatively disable the type caching in that case
static bool WorkaroundForSignatureMutationsInTypeInference(Signature *signature)
{
    if (!signature->HasFunction()) {
        return false;
    }
    auto ast = signature->Function();
    if (!(ast->Parent()->IsArrowFunctionExpression() && ast->Parent()->Parent()->IsCallExpression())) {
        return false;
    }

    auto callee = ast->Parent()->Parent()->AsCallExpression()->Callee()->TsType();
    if (callee == nullptr || !callee->IsETSMethodType()) {
        return false;
    }
    for (auto sig : callee->AsETSFunctionType()->CallSignatures()) {
        if (!sig->TypeParams().empty()) {
            return true;
        }
    }
    return false;
}

ETSFunctionType *ETSChecker::CreateETSArrowType(Signature *signature)
{
    bool noCaching = false;
    auto const typeRef = [&noCaching](Type *t) {
        noCaching |= t->IsTypeError();  // "inference" has not succeeded yet
        return t;
    };
    noCaching |= WorkaroundForSignatureMutationsInTypeInference(signature);

    std::stringstream ss;
    ss << "/" << helpers::ToUnderlying(signature->Flags());
    ss << "/";
    for (auto p : signature->GetSignatureInfo()->typeParams) {
        ss << ":" << p;
    }
    ss << "/" << signature->GetSignatureInfo()->minArgCount;
    ss << "/r" << typeRef(signature->ReturnType());
    ss << "/";
    for (auto p : signature->GetSignatureInfo()->params) {
        ss << ":" << typeRef(p->TsType());
    }
    auto restVar = signature->GetSignatureInfo()->restVar;
    if (restVar != nullptr) {
        ss << "/" << typeRef(restVar->TsType());
    }
    auto hash = ss.str();

    auto &cache = functionTypeInstantiationMap_;
    if (auto it = cache.find(hash); !noCaching && it != cache.end()) {
        return it->second;
    }
    auto type = ProgramAllocator()->New<ETSFunctionType>(this, signature);
    cache.insert({hash, type});
    return type;
}

ETSFunctionType *ETSChecker::CreateETSMethodType(util::StringView name, ArenaVector<Signature *> &&signatures)
{
    return ProgramAllocator()->New<ETSFunctionType>(this, name, std::move(signatures));
}

static SignatureFlags ConvertToSignatureFlags(ir::ModifierFlags inModifiers, ir::ScriptFunctionFlags inFunctionFlags)
{
    SignatureFlags outFlags = SignatureFlags::NO_OPTS;

    const auto convertModifier = [&outFlags, inModifiers](ir::ModifierFlags astFlag, SignatureFlags sigFlag) {
        if ((inModifiers & astFlag) != 0U) {
            outFlags |= sigFlag;
        }
    };
    const auto convertFlag = [&outFlags, inFunctionFlags](ir::ScriptFunctionFlags funcFlag, SignatureFlags sigFlag) {
        if ((inFunctionFlags & funcFlag) != 0U) {
            outFlags |= sigFlag;
        }
    };

    convertFlag(ir::ScriptFunctionFlags::CONSTRUCTOR, SignatureFlags::CONSTRUCTOR);
    convertFlag(ir::ScriptFunctionFlags::SETTER, SignatureFlags::SETTER);
    convertFlag(ir::ScriptFunctionFlags::GETTER, SignatureFlags::GETTER);

    convertModifier(ir::ModifierFlags::ABSTRACT, SignatureFlags::ABSTRACT);
    convertModifier(ir::ModifierFlags::PROTECTED, SignatureFlags::PROTECTED);
    convertModifier(ir::ModifierFlags::FINAL, SignatureFlags::FINAL);
    convertModifier(ir::ModifierFlags::STATIC, SignatureFlags::STATIC);
    convertModifier(ir::ModifierFlags::PUBLIC, SignatureFlags::PUBLIC);
    convertModifier(ir::ModifierFlags::PRIVATE, SignatureFlags::PRIVATE);
    convertModifier(ir::ModifierFlags::INTERNAL, SignatureFlags::INTERNAL);
    convertModifier(ir::ModifierFlags::DEFAULT, SignatureFlags::DEFAULT);

    return outFlags;
}

Signature *ETSChecker::CreateSignature(SignatureInfo *info, Type *returnType, ir::ScriptFunction *func)
{
    if (info == nullptr) {  // #23134
        ES2PANDA_ASSERT(IsAnyError());
        return nullptr;
    }
    auto signature = ProgramAllocator()->New<Signature>(info, returnType, func);
    auto convertedFlag = ConvertToSignatureFlags(func->Modifiers(), func->Flags());
    ES2PANDA_ASSERT(signature != nullptr);
    func->HasReceiver() ? signature->AddSignatureFlag(SignatureFlags::EXTENSION_FUNCTION | convertedFlag)
                        : signature->AddSignatureFlag(convertedFlag);
    return signature;
}

Signature *ETSChecker::CreateSignature(SignatureInfo *info, Type *returnType, ir::ScriptFunctionFlags sff,
                                       bool hasReceiver)
{
    if (info == nullptr) {  // #23134
        ES2PANDA_ASSERT(IsAnyError());
        return nullptr;
    }
    auto signature = ProgramAllocator()->New<Signature>(info, returnType, nullptr);
    ES2PANDA_ASSERT(signature != nullptr);
    signature->AddSignatureFlag(ConvertToSignatureFlags(ir::ModifierFlags::NONE, sff));
    // synthetic arrow type signature flags
    auto extraFlags = SignatureFlags::ABSTRACT | SignatureFlags::CALL | SignatureFlags::PUBLIC;
    hasReceiver ? signature->AddSignatureFlag(SignatureFlags::EXTENSION_FUNCTION | extraFlags)
                : signature->AddSignatureFlag(extraFlags);
    return signature;
}

SignatureInfo *ETSChecker::CreateSignatureInfo()
{
    return ProgramAllocator()->New<SignatureInfo>(ProgramAllocator());
}

ETSTypeParameter *ETSChecker::CreateTypeParameter()
{
    return ProgramAllocator()->New<ETSTypeParameter>();
}

ETSExtensionFuncHelperType *ETSChecker::CreateETSExtensionFuncHelperType(ETSFunctionType *classMethodType,
                                                                         ETSFunctionType *extensionFunctionType)
{
    return ProgramAllocator()->New<ETSExtensionFuncHelperType>(classMethodType, extensionFunctionType);
}

static std::pair<util::StringView, util::StringView> GetObjectTypeDeclNames(ir::AstNode *node)
{
    if (node->IsClassDefinition()) {
        return {node->AsClassDefinition()->Ident()->Name(), node->AsClassDefinition()->InternalName()};
    }
    if (node->IsTSInterfaceDeclaration()) {
        return {node->AsTSInterfaceDeclaration()->Id()->Name(), node->AsTSInterfaceDeclaration()->InternalName()};
    }
    return {node->AsAnnotationDeclaration()->GetBaseName()->Name(), node->AsAnnotationDeclaration()->InternalName()};
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP) solid logic, big switch case
static ETSObjectType *InitializeGlobalBuiltinObjectType(ETSChecker *checker, GlobalTypeId globalId,
                                                        ir::AstNode *declNode, ETSObjectFlags flags)
{
    auto const create = [checker, declNode, flags](ETSObjectFlags addFlags = ETSObjectFlags::NO_OPTS) {
        return checker->CreateETSObjectType(declNode, flags | addFlags);
    };

    auto const setType = [checker](GlobalTypeId slotId, Type *type) {
        auto &slot = checker->GetGlobalTypesHolder()->GlobalTypes()[helpers::ToUnderlying(slotId)];
        if (slot == nullptr) {
            slot = type;
        }
        return slot;
    };

    auto *const programAllocator = checker->ProgramAllocator();

    switch (globalId) {
        case GlobalTypeId::ETS_OBJECT_BUILTIN: {
            auto *objType = setType(GlobalTypeId::ETS_OBJECT_BUILTIN, create())->AsETSObjectType();
            auto null = checker->GlobalETSNullType();
            auto undef = checker->GlobalETSUndefinedType();
            setType(GlobalTypeId::ETS_UNION_UNDEFINED_NULL_OBJECT, checker->CreateETSUnionType({objType, null, undef}));
            setType(GlobalTypeId::ETS_UNION_UNDEFINED_NULL, checker->CreateETSUnionType({null, undef}));
            return objType;
        }
        case GlobalTypeId::ETS_STRING_BUILTIN: {
            auto *stringObj = setType(GlobalTypeId::ETS_STRING_BUILTIN,
                                      create(ETSObjectFlags::BUILTIN_STRING | ETSObjectFlags::STRING))
                                  ->AsETSObjectType();
            setType(GlobalTypeId::ETS_STRING,
                    programAllocator->New<ETSStringType>(programAllocator, stringObj, checker->Relation()));
            return stringObj;
        }
        case GlobalTypeId::ETS_BIG_INT_BUILTIN: {
            auto *bigIntObj =
                setType(GlobalTypeId::ETS_BIG_INT_BUILTIN, create(ETSObjectFlags::BUILTIN_BIGINT))->AsETSObjectType();
            setType(GlobalTypeId::ETS_BIG_INT, programAllocator->New<ETSBigIntType>(programAllocator, bigIntObj));
            return bigIntObj;
        }
        case GlobalTypeId::ETS_ARRAY_BUILTIN: {
            if (declNode->AsClassDefinition()->InternalName().Utf8() != compiler::Signatures::ESCOMPAT_ARRAY) {
                return checker->CreateETSObjectType(declNode, flags);
            }
            auto *arrayObj =
                setType(GlobalTypeId::ETS_ARRAY_BUILTIN, create(ETSObjectFlags::BUILTIN_ARRAY))->AsETSObjectType();
            setType(GlobalTypeId::ETS_ARRAY, programAllocator->New<ETSResizableArrayType>(programAllocator, arrayObj));
            return arrayObj;
        }
        case GlobalTypeId::ETS_READONLY_ARRAY:
            if (declNode->IsClassDefinition() || declNode->AsTSInterfaceDeclaration()->InternalName().Utf8() !=
                                                     compiler::Signatures::ESCOMPAT_READONLYARRAY) {
                return checker->CreateETSObjectType(declNode, flags);
            }
            return setType(globalId, create(ETSObjectFlags::BUILTIN_READONLY_ARRAY))->AsETSObjectType();
        case GlobalTypeId::ETS_BOOLEAN_BUILTIN:
            return create(ETSObjectFlags::BUILTIN_BOOLEAN);
        case GlobalTypeId::ETS_BYTE_BUILTIN:
            return create(ETSObjectFlags::BUILTIN_BYTE);
        case GlobalTypeId::ETS_CHAR_BUILTIN:
            return create(ETSObjectFlags::BUILTIN_CHAR);
        case GlobalTypeId::ETS_SHORT_BUILTIN:
            return create(ETSObjectFlags::BUILTIN_SHORT);
        case GlobalTypeId::ETS_INT_BUILTIN:
            return create(ETSObjectFlags::BUILTIN_INT);
        case GlobalTypeId::ETS_LONG_BUILTIN:
            return create(ETSObjectFlags::BUILTIN_LONG);
        case GlobalTypeId::ETS_FLOAT_BUILTIN:
            return create(ETSObjectFlags::BUILTIN_FLOAT);
        case GlobalTypeId::ETS_DOUBLE_BUILTIN:
            return create(ETSObjectFlags::BUILTIN_DOUBLE);
        default:
            return create();
    }
}

ETSObjectType *ETSChecker::CreateETSObjectTypeOrBuiltin(ir::AstNode *declNode, ETSObjectFlags flags)
{
    if (LIKELY(HasStatus(CheckerStatus::BUILTINS_INITIALIZED))) {
        return CreateETSObjectType(declNode, flags);
    }
    // Note (zengran): should be determined whether is a builtin type through InternalName instead of DeclName.
    auto const globalId = GetGlobalTypesHolder()->NameToId(GetObjectTypeDeclNames(declNode).first);
    if (!globalId.has_value()) {
        return CreateETSObjectType(declNode, flags);
    }
    return InitializeGlobalBuiltinObjectType(this, globalId.value(), declNode, flags);
}

ETSObjectType *ETSChecker::CreateETSObjectType(
    ir::AstNode *declNode, ETSObjectFlags flags,
    /* this parameter maintanis the behavior of the broken ast-cache logic, avoid it whenever possible */
    std::optional<std::pair<ThreadSafeArenaAllocator *, TypeRelation *>> caches)
{
    auto const allocator = caches.has_value() ? caches->first : ProgramAllocator();
    auto const relation = caches.has_value() ? caches->second : Relation();

    auto const [name, internalName] = GetObjectTypeDeclNames(declNode);

    if (declNode->IsClassDefinition() && (declNode->AsClassDefinition()->IsEnumTransformed())) {
        if (declNode->AsClassDefinition()->IsIntEnumTransformed()) {
            return allocator->New<ETSIntEnumType>(allocator, name, internalName, declNode, relation);
        }
        if (declNode->AsClassDefinition()->IsDoubleEnumTransformed()) {
            return ProgramAllocator()->New<ETSDoubleEnumType>(ProgramAllocator(), name, internalName, declNode,
                                                              Relation());
        }
        ES2PANDA_ASSERT(declNode->AsClassDefinition()->IsStringEnumTransformed());
        return allocator->New<ETSStringEnumType>(allocator, name, internalName, declNode, relation);
    }

    if (internalName == compiler::Signatures::BUILTIN_ARRAY) {
        return allocator->New<ETSResizableArrayType>(allocator, name, std::make_tuple(declNode, flags, relation));
    }
    return allocator->New<ETSObjectType>(allocator, name, internalName, std::make_tuple(declNode, flags, relation));
}

std::tuple<util::StringView, SignatureInfo *> ETSChecker::CreateBuiltinArraySignatureInfo(const ETSArrayType *arrayType,
                                                                                          size_t dim)
{
    std::stringstream ss;
    arrayType->ToAssemblerTypeWithRank(ss);
    ss << compiler::Signatures::METHOD_SEPARATOR << compiler::Signatures::CTOR << compiler::Signatures::MANGLE_BEGIN;
    arrayType->ToAssemblerTypeWithRank(ss);

    auto *info = CreateSignatureInfo();
    ES2PANDA_ASSERT(info != nullptr);
    info->minArgCount = dim;

    for (size_t i = 0; i < dim; i++) {
        util::UString param(std::to_string(i), ProgramAllocator());
        auto *paramVar =
            varbinder::Scope::CreateVar(ProgramAllocator(), param.View(), varbinder::VariableFlags::NONE, nullptr);
        ES2PANDA_ASSERT(paramVar != nullptr);
        paramVar->SetTsType(GlobalIntType());

        info->params.push_back(paramVar);

        ss << compiler::Signatures::MANGLE_SEPARATOR << compiler::Signatures::PRIMITIVE_INT;
    }

    ss << compiler::Signatures::MANGLE_SEPARATOR << compiler::Signatures::PRIMITIVE_VOID
       << compiler::Signatures::MANGLE_SEPARATOR;
    auto internalName = util::UString(ss.str(), ProgramAllocator()).View();

    return {internalName, info};
}

Signature *ETSChecker::CreateBuiltinArraySignature(const ETSArrayType *arrayType, size_t dim)
{
    auto currentChecker =
        compiler::GetPhaseManager()->Context() != nullptr ? compiler::GetPhaseManager()->Context()->GetChecker() : this;
    auto &globalArraySignatures = currentChecker->AsETSChecker()->globalArraySignatures_;
    auto res = globalArraySignatures.find(arrayType);
    if (res != globalArraySignatures.end()) {
        return res->second;
    }

    auto [internalName, info] = CreateBuiltinArraySignatureInfo(arrayType, dim);
    auto *signature = CreateSignature(info, GlobalVoidType(), ir::ScriptFunctionFlags::NONE, false);
    ES2PANDA_ASSERT(signature != nullptr);
    signature->SetInternalName(internalName);
    globalArraySignatures.insert({arrayType, signature});

    return signature;
}

ETSObjectType *ETSChecker::CreatePromiseOf(Type *type)
{
    ETSObjectType *const promiseType = GlobalBuiltinPromiseType();
    ES2PANDA_ASSERT(promiseType->TypeArguments().size() == 1U);

    auto substitution = Substitution {};
    ES2PANDA_ASSERT(promiseType != nullptr);
    EmplaceSubstituted(&substitution, promiseType->TypeArguments()[0]->AsETSTypeParameter()->GetOriginal(), type);

    return promiseType->Substitute(Relation(), &substitution);
}

static bool IsInValidKeyofTypeNode(ir::AstNode *node)
{
    return (node->Modifiers() & ir::ModifierFlags::PRIVATE) != 0 ||
           (node->Modifiers() & ir::ModifierFlags::PROTECTED) != 0;
}

void ETSChecker::ProcessTypeMembers(ETSObjectType *type, ArenaVector<Type *> &literals)
{
    if (type == GlobalETSObjectType()) {
        return;
    }

    for (auto *method : type->Methods()) {
        auto *methodDef = method->Declaration()->Node()->AsMethodDefinition();
        for (auto *overload : methodDef->Overloads()) {
            if (IsInValidKeyofTypeNode(overload)) {
                continue;
            }
            literals.push_back(CreateETSStringLiteralType(overload->Key()->Variable()->Name()));
        }
        if (!IsInValidKeyofTypeNode(method->Declaration()->Node())) {
            literals.push_back(CreateETSStringLiteralType(method->Name()));
        }
    }

    for (auto *field : type->Fields()) {
        if (IsInValidKeyofTypeNode(field->Declaration()->Node())) {
            continue;
        }
        literals.push_back(CreateETSStringLiteralType(field->Name()));
    }
}

Type *ETSChecker::CreateUnionFromKeyofType(ETSObjectType *const type)
{
    ArenaVector<Type *> stringLiterals(ProgramAllocator()->Adapter());
    std::deque<ETSObjectType *> superTypes;
    superTypes.push_back(type);
    auto enqueueSupers = [&](ETSObjectType *currentType) {
        if (currentType->SuperType() != nullptr) {
            superTypes.push_back(currentType->SuperType());
        }
        for (auto interface : currentType->Interfaces()) {
            superTypes.push_back(interface);
        }
    };

    while (!superTypes.empty()) {
        auto *currentType = superTypes.front();
        superTypes.pop_front();

        ProcessTypeMembers(currentType, stringLiterals);
        enqueueSupers(currentType);
    }

    return stringLiterals.empty() ? GlobalETSNeverType() : CreateETSUnionType(std::move(stringLiterals));
}

ETSAsyncFuncReturnType *ETSChecker::CreateETSAsyncFuncReturnTypeFromPromiseType(ETSObjectType *promiseType)
{
    return ProgramAllocator()->New<ETSAsyncFuncReturnType>(ProgramAllocator(), Relation(), promiseType);
}

ETSAsyncFuncReturnType *ETSChecker::CreateETSAsyncFuncReturnTypeFromBaseType(Type *baseType)
{
    auto const promiseType = CreatePromiseOf(MaybeBoxType(baseType));
    return ProgramAllocator()->New<ETSAsyncFuncReturnType>(ProgramAllocator(), Relation(), promiseType);
}

}  // namespace ark::es2panda::checker
