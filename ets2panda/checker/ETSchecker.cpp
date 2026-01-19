/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include <mutex>
#include <public/public.h>
#include <utility>

#include "ETSchecker.h"

#include "es2panda.h"
#include "ir/base/classDefinition.h"
#include "ir/expression.h"
#include "ir/expressions/callExpression.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "ir/statements/blockStatement.h"
#include "types/type.h"
#include "varbinder/ETSBinder.h"
#include "parser/program/program.h"
#include "checker/ets/aliveAnalyzer.h"
#include "checker/ets/assignAnalyzer.h"
#include "checker/ets/etsWarningAnalyzer.h"
#include "checker/types/globalTypesHolder.h"
#include "ir/base/scriptFunction.h"
#include "util/helpers.h"
#include "evaluate/scopedDebugInfoPlugin.h"
#include "checker/types/ets/etsTupleType.h"

namespace ark::es2panda::checker {

void ETSChecker::ReputCheckerDataProgram(ETSChecker *eChecker)
{
    if (!HasStatus(CheckerStatus::BUILTINS_INITIALIZED)) {
        SetGlobalTypesHolder(eChecker->GetGlobalTypesHolder());
        AddStatus(CheckerStatus::BUILTINS_INITIALIZED);
    }

    if (auto it = readdedChecker_.find(eChecker); it != readdedChecker_.end()) {
        return;
    }
    readdedChecker_.insert(eChecker->readdedChecker_.begin(), eChecker->readdedChecker_.end());
    auto &computedAbstractMapToCopy = eChecker->GetCachedComputedAbstracts();
    for (auto &[key, value] : computedAbstractMapToCopy) {
        if (cachedComputedAbstracts_.find(key) != cachedComputedAbstracts_.end()) {
            return;
        }
        auto &[v1, v2] = value;
        std::vector<ETSFunctionType *> newV1;
        std::unordered_set<ETSObjectType *> newV2;
        newV1.assign(v1.cbegin(), v1.cend());
        newV2.insert(v2.cbegin(), v2.cend());
        cachedComputedAbstracts_.try_emplace(key, newV1, newV2);
    }

    auto &globalArraySigs = eChecker->globalArraySignatures_;
    globalArraySignatures_.insert(globalArraySigs.cbegin(), globalArraySigs.cend());

    auto &apparentTypes = eChecker->apparentTypes_;
    apparentTypes_.insert(apparentTypes.cbegin(), apparentTypes.cend());

    auto &objectInstantiationMap = eChecker->objectInstantiationMap_;
    for (auto &[key, value] : objectInstantiationMap) {
        if (objectInstantiationMap_.find(key) == objectInstantiationMap_.end()) {
            objectInstantiationMap_.insert(objectInstantiationMap.cbegin(), objectInstantiationMap.cend());
        }
    }

    auto &invokeToArrowSignatures = eChecker->invokeToArrowSignatures_;
    invokeToArrowSignatures_.insert(invokeToArrowSignatures.cbegin(), invokeToArrowSignatures.cend());
    auto &arrowToFuncInterfaces = eChecker->arrowToFuncInterfaces_;
    arrowToFuncInterfaces_.insert(arrowToFuncInterfaces.cbegin(), arrowToFuncInterfaces.cend());
    auto unionAssemblerTypes = eChecker->unionAssemblerTypes_;
    unionAssemblerTypes_.insert(unionAssemblerTypes.cbegin(), unionAssemblerTypes.cend());
}

void ETSChecker::ReputCheckerData()
{
    readdedChecker_.insert(this);
    for (auto &[_, extPrograms] : Program()->ExternalSources()) {
        (void)_;
        for (auto *extProg : extPrograms) {
            if (!extProg->IsASTLowered() && extProg->IsProgramModified()) {
                continue;
            }
            ReputCheckerDataProgram(extProg->Checker()->AsETSChecker());
        }
    }
}

static util::StringView InitBuiltin(ETSChecker *checker, std::string_view signature)
{
    const auto &varMap = checker->VarBinder()->TopScope()->Bindings();
    const auto iterator = varMap.find(signature);
    ES2PANDA_ASSERT(iterator != varMap.end());
    auto *var = iterator->second;
    Type *type {nullptr};
    if (var->HasFlag(varbinder::VariableFlags::BUILTIN_TYPE)) {
        if (var->Declaration()->Node()->IsClassDefinition()) {
            type = checker->BuildBasicClassProperties(var->Declaration()->Node()->AsClassDefinition());
        } else {
            ES2PANDA_ASSERT(var->Declaration()->Node()->IsTSInterfaceDeclaration());
            type = checker->BuildBasicInterfaceProperties(var->Declaration()->Node()->AsTSInterfaceDeclaration());
        }
        checker->GetGlobalTypesHolder()->InitializeBuiltin(iterator->first, type);
    }
    return iterator->first;
}

void ETSChecker::CheckObjectLiteralKeys(const ArenaVector<ir::Expression *> &properties)
{
    std::set<util::StringView> fieldNames {};
    std::set<util::StringView> methodNames {};

    for (auto property : properties) {
        if (!property->IsProperty()) {
            continue;
        }
        auto propertyDecl = property->AsProperty();
        auto propKey = propertyDecl->Key();
        if (!propKey->IsIdentifier() && !propKey->IsStringLiteral()) {
            continue;
        }

        // number kind only used here
        auto propName = propKey->IsIdentifier() ? propKey->AsIdentifier()->Name() : propKey->AsStringLiteral()->Str();
        if (fieldNames.find(propName) != fieldNames.end()) {
            LogError(diagnostic::OBJ_LIT_PROPERTY_REDECLARATION, {}, property->Start());
        }

        // Method names can duplicate because of possible overloading
        if (!propertyDecl->Value()->IsArrowFunctionExpression()) {
            if (methodNames.find(propName) != methodNames.end()) {
                LogError(diagnostic::OBJ_LIT_PROPERTY_REDECLARATION, {}, property->Start());
            }
            fieldNames.insert(propName);
        } else {
            methodNames.insert(propName);
        }
    }
}

static void SetupBuiltinMember(varbinder::Variable *var)
{
    auto *type = var->TsType();
    if (type == nullptr || !type->IsETSObjectType()) {
        return;
    }
}

// clang-format off
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
static constexpr std::string_view BUILTINS_TO_INIT[] = {
    compiler::Signatures::BUILTIN_OBJECT_CLASS,
    compiler::Signatures::BUILTIN_STRING_CLASS,
    compiler::Signatures::BUILTIN_BIGINT_CLASS,
    compiler::Signatures::BUILTIN_ERROR_CLASS,
    compiler::Signatures::BUILTIN_TYPE_CLASS,
    compiler::Signatures::BUILTIN_PROMISE_CLASS,
    compiler::Signatures::BUILTIN_BOOLEAN_CLASS,
    compiler::Signatures::BUILTIN_BYTE_CLASS,
    compiler::Signatures::BUILTIN_CHAR_CLASS,
    compiler::Signatures::BUILTIN_SHORT_CLASS,
    compiler::Signatures::BUILTIN_INT_CLASS,
    compiler::Signatures::BUILTIN_LONG_CLASS,
    compiler::Signatures::BUILTIN_FLOAT_CLASS,
    compiler::Signatures::BUILTIN_DOUBLE_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION0_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION1_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION2_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION3_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION4_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION5_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION6_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION7_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION8_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION9_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION10_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION11_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION12_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION13_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION14_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION15_CLASS,
    compiler::Signatures::BUILTIN_FUNCTION16_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA0_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA1_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA2_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA3_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA4_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA5_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA6_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA7_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA8_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA9_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA10_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA11_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA12_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA13_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA14_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA15_CLASS,
    compiler::Signatures::BUILTIN_LAMBDA16_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR0_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR1_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR2_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR3_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR4_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR5_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR6_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR7_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR8_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR9_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR10_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR11_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR12_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR13_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR14_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR15_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONR16_CLASS,
    compiler::Signatures::BUILTIN_FUNCTIONN_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR0_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR1_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR2_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR3_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR4_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR5_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR6_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR7_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR8_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR9_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR10_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR11_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR12_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR13_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR14_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR15_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAR16_CLASS,
    compiler::Signatures::BUILTIN_LAMBDAN_CLASS,
    compiler::Signatures::BUILTIN_TUPLE0_CLASS,
    compiler::Signatures::BUILTIN_TUPLE1_CLASS,
    compiler::Signatures::BUILTIN_TUPLE2_CLASS,
    compiler::Signatures::BUILTIN_TUPLE3_CLASS,
    compiler::Signatures::BUILTIN_TUPLE4_CLASS,
    compiler::Signatures::BUILTIN_TUPLE5_CLASS,
    compiler::Signatures::BUILTIN_TUPLE6_CLASS,
    compiler::Signatures::BUILTIN_TUPLE7_CLASS,
    compiler::Signatures::BUILTIN_TUPLE8_CLASS,
    compiler::Signatures::BUILTIN_TUPLE9_CLASS,
    compiler::Signatures::BUILTIN_TUPLE10_CLASS,
    compiler::Signatures::BUILTIN_TUPLE11_CLASS,
    compiler::Signatures::BUILTIN_TUPLE12_CLASS,
    compiler::Signatures::BUILTIN_TUPLE13_CLASS,
    compiler::Signatures::BUILTIN_TUPLE14_CLASS,
    compiler::Signatures::BUILTIN_TUPLE15_CLASS,
    compiler::Signatures::BUILTIN_TUPLE16_CLASS,
    compiler::Signatures::BUILTIN_TUPLEN_CLASS,
};
// clang-format on

static void IntializeFunctionInterfaces(GlobalTypesHolder *typeHolder)
{
    auto const getItf = [typeHolder](size_t arity, bool hasRest) {
        return typeHolder->GlobalFunctionBuiltinType(arity, hasRest)->AsETSObjectType();
    };

    for (size_t arity = 0; arity <= typeHolder->VariadicFunctionTypeThreshold(); arity++) {
        getItf(arity, false)->AddObjectFlag(ETSObjectFlags::FUNCTIONAL);
        getItf(arity, true)->AddObjectFlag(ETSObjectFlags::FUNCTIONAL);
    }
}

void ETSChecker::InitializeBuiltins(varbinder::ETSBinder *varbinder)
{
    if (HasStatus(CheckerStatus::BUILTINS_INITIALIZED)) {
        return;
    }

    const auto varMap = varbinder->TopScope()->Bindings();
    if (varMap.find(compiler::Signatures::BUILTIN_OBJECT_CLASS) == varMap.end()) {
        return;
    }

    auto const objectName = InitBuiltin(this, compiler::Signatures::BUILTIN_OBJECT_CLASS);

    for (auto sig : BUILTINS_TO_INIT) {
        InitBuiltin(this, sig);
    }

    IntializeFunctionInterfaces(GetGlobalTypesHolder());

    for (const auto &[name, var] : varMap) {
        (void)name;
        SetupBuiltinMember(var);
    }

    for (const auto &[name, var] : varMap) {
        if (name == objectName) {
            continue;
        }

        if (var->HasFlag(varbinder::VariableFlags::BUILTIN_TYPE)) {
            if (var->TsType() == nullptr) {
                InitializeBuiltin(var, name);
            } else {
                GetGlobalTypesHolder()->InitializeBuiltin(name, var->TsType());
            }
        }
    }

    AddStatus(CheckerStatus::BUILTINS_INITIALIZED);
}

void ETSChecker::InitializeBuiltin(varbinder::Variable *var, const util::StringView &name)
{
    Type *type {nullptr};
    if (var->Declaration()->Node()->IsClassDefinition()) {
        type = BuildBasicClassProperties(var->Declaration()->Node()->AsClassDefinition());
    } else {
        ES2PANDA_ASSERT(var->Declaration()->Node()->IsTSInterfaceDeclaration());
        type = BuildBasicInterfaceProperties(var->Declaration()->Node()->AsTSInterfaceDeclaration());
    }
    GetGlobalTypesHolder()->InitializeBuiltin(name, type);
}

bool ETSChecker::StartChecker(varbinder::VarBinder *varbinder, const util::Options &options)
{
    if (options.IsParseOnly()) {
        return false;
    }
    permitRelaxedAny_ = options.IsPermitRelaxedAny();

    auto *etsBinder = varbinder->AsETSBinder();
    InitializeBuiltins(etsBinder);

    bool isEvalMode = (debugInfoPlugin_ != nullptr);
    if (UNLIKELY(isEvalMode)) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        debugInfoPlugin_->PreCheck();
    }

    CheckProgram(Program(), true);

    if (UNLIKELY(isEvalMode)) {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        debugInfoPlugin_->PostCheck();
    }

#ifndef NDEBUG
    for (auto *func : varbinder->Functions()) {
        ES2PANDA_ASSERT(!func->Node()->AsScriptFunction()->Scope()->Name().Empty());
    }
#endif

    if (options.IsDumpDynamicAst()) {
        std::cout << Program()->Dump() << std::endl;
    }

    CheckWarnings(Program(), options);

    return !IsAnyError();
}

evaluate::ScopedDebugInfoPlugin *ETSChecker::GetDebugInfoPlugin()
{
    return debugInfoPlugin_;
}

const evaluate::ScopedDebugInfoPlugin *ETSChecker::GetDebugInfoPlugin() const
{
    return debugInfoPlugin_;
}

void ETSChecker::SetDebugInfoPlugin(evaluate::ScopedDebugInfoPlugin *debugInfo)
{
    debugInfoPlugin_ = debugInfo;
}

void ETSChecker::CheckProgram(parser::Program *program, bool runAnalysis)
{
    auto *savedProgram = Program();
    SetProgram(program);

    for (auto &[_, extPrograms] : program->ExternalSources()) {
        (void)_;
        for (auto *extProg : extPrograms) {
            if (!extProg->IsASTLowered() && extProg->IsProgramModified()) {
                extProg->PushChecker(this);
                auto *savedProgram2 = VarBinder()->AsETSBinder()->Program();
                varbinder::RecordTableContext recordTableCtx(VarBinder()->AsETSBinder(), extProg);
                VarBinder()->AsETSBinder()->SetProgram(extProg);
                VarBinder()->AsETSBinder()->ResetTopScope(extProg->GlobalScope());
                checker::SavedCheckerContext savedContext(this, Context().Status(), Context().ContainingClass());
                AddStatus(checker::CheckerStatus::IN_EXTERNAL);
                CheckProgram(extProg, VarBinder()->IsGenStdLib() || extProg->IsGenAbcForExternal());
                VarBinder()->AsETSBinder()->SetProgram(savedProgram2);
                VarBinder()->AsETSBinder()->ResetTopScope(savedProgram2->GlobalScope());
                extProg->SetProgramModified(false);
            }
        }
    }

    ES2PANDA_ASSERT(Program()->Ast()->IsProgram());

    if (runAnalysis) {
        Program()->Ast()->Check(this);
        if (!IsAnyError()) {
            AliveAnalyzer aliveAnalyzer(Program()->Ast(), this);
            AssignAnalyzer(this).Analyze(Program()->Ast());
        }
    } else if (!VarBinder()->GetContext()->lazyCheck) {
        Program()->Ast()->Check(this);
    }

    ES2PANDA_ASSERT(VarBinder()->AsETSBinder()->GetExternalRecordTable().find(program)->second);

    SetProgram(savedProgram);
}

void ETSChecker::CheckWarnings(parser::Program *program, const util::Options &options)
{
    const auto &etsWarningCollection = options.GetEtsWarningCollection();
    for (const auto warning : etsWarningCollection) {
        ETSWarningAnalyzer(Program()->Ast(), program, warning, DiagnosticEngine());
    }
}

Type *ETSChecker::CheckTypeCached(ir::Expression *expr)
{
    if (expr->TsType() == nullptr) {
        expr->SetTsType(expr->Check(this));
    }

    return expr->TsType();
}

bool ETSChecker::IsClassStaticMethod(checker::ETSObjectType *objType, checker::Signature *signature)
{
    return objType->HasObjectFlag(checker::ETSObjectFlags::CLASS) &&
           signature->HasSignatureFlag(checker::SignatureFlags::STATIC);
}

[[nodiscard]] TypeFlag ETSChecker::TypeKind(const Type *const type) noexcept
{
    // These types were not present in the ETS_TYPE list. Some of them are omitted intentionally, other are just bugs
    static constexpr auto TO_CLEAR = TypeFlag::CONSTANT | TypeFlag::GENERIC | TypeFlag::ETS_NUMERIC_ENUM |
                                     TypeFlag::ETS_STRING_ENUM | TypeFlag::READONLY | TypeFlag::BIGINT_LITERAL |
                                     TypeFlag::ETS_TYPE_ALIAS | TypeFlag::TYPE_ERROR | TypeFlag::STRING_LITERAL;

    CHECK_NOT_NULL(type);
    auto res = static_cast<checker::TypeFlag>(type->TypeFlags() & ~(TO_CLEAR));
    ES2PANDA_ASSERT_POS(res == TypeFlag::NONE || helpers::math::IsPowerOfTwo(res & ~(TypeFlag::NONE)),
                        ark::es2panda::GetPositionForDiagnostic());
    return res;
}

template <typename... Args>
ETSObjectType *ETSChecker::AsETSObjectType(Type *(GlobalTypesHolder::*typeFunctor)(Args...), Args... args) const
{
    auto *ret = (GetGlobalTypesHolder()->*typeFunctor)(args...);
    return ret != nullptr ? ret->AsETSObjectType() : nullptr;
}

Type *ETSChecker::GlobalByteType() const
{
    return GetGlobalTypesHolder()->GlobalByteType();
}

Type *ETSChecker::GlobalByteBuiltinType() const
{
    return GetGlobalTypesHolder()->GlobalByteBuiltinType();
}

Type *ETSChecker::GlobalShortType() const
{
    return GetGlobalTypesHolder()->GlobalShortType();
}

Type *ETSChecker::GlobalShortBuiltinType() const
{
    return GetGlobalTypesHolder()->GlobalShortBuiltinType();
}

Type *ETSChecker::GlobalIntType() const
{
    return GetGlobalTypesHolder()->GlobalIntType();
}

Type *ETSChecker::GlobalIntBuiltinType() const
{
    return GetGlobalTypesHolder()->GlobalIntegerBuiltinType();
}

Type *ETSChecker::GlobalLongType() const
{
    return GetGlobalTypesHolder()->GlobalLongType();
}

Type *ETSChecker::GlobalLongBuiltinType() const
{
    return GetGlobalTypesHolder()->GlobalLongBuiltinType();
}

Type *ETSChecker::GlobalFloatType() const
{
    return GetGlobalTypesHolder()->GlobalFloatType();
}

Type *ETSChecker::GlobalFloatBuiltinType() const
{
    return GetGlobalTypesHolder()->GlobalFloatBuiltinType();
}

Type *ETSChecker::GlobalDoubleType() const
{
    return GetGlobalTypesHolder()->GlobalDoubleType();
}

Type *ETSChecker::GlobalDoubleBuiltinType() const
{
    return GetGlobalTypesHolder()->GlobalDoubleBuiltinType();
}

Type *ETSChecker::GlobalCharType() const
{
    return GetGlobalTypesHolder()->GlobalCharType();
}
Type *ETSChecker::GlobalCharBuiltinType() const
{
    return GetGlobalTypesHolder()->GlobalCharBuiltinType();
}

Type *ETSChecker::GlobalETSBooleanType() const
{
    return GetGlobalTypesHolder()->GlobalETSBooleanType();
}

Type *ETSChecker::GlobalETSBooleanBuiltinType() const
{
    return GetGlobalTypesHolder()->GlobalETSBooleanBuiltinType();
}

Type *ETSChecker::GlobalVoidType() const
{
    return GetGlobalTypesHolder()->GlobalETSVoidType();
}

Type *ETSChecker::GlobalETSNullType() const
{
    return GetGlobalTypesHolder()->GlobalETSNullType();
}

Type *ETSChecker::GlobalETSUndefinedType() const
{
    return GetGlobalTypesHolder()->GlobalETSUndefinedType();
}

Type *ETSChecker::GlobalETSAnyType() const
{
    return GetGlobalTypesHolder()->GlobalETSAnyType();
}

Type *ETSChecker::GlobalETSRelaxedAnyType() const
{
    return GetGlobalTypesHolder()->GlobalETSRelaxedAnyType();
}

Type *ETSChecker::GlobalETSNeverType() const
{
    return GetGlobalTypesHolder()->GlobalETSNeverType();
}

Type *ETSChecker::GlobalETSStringLiteralType() const
{
    return GetGlobalTypesHolder()->GlobalETSStringLiteralType();
}

Type *ETSChecker::GlobalETSBigIntType() const
{
    return GetGlobalTypesHolder()->GlobalETSBigIntBuiltinType();
}

Type *ETSChecker::GlobalWildcardType() const
{
    return GetGlobalTypesHolder()->GlobalWildcardType();
}

ETSObjectType *ETSChecker::GlobalETSObjectType() const
{
    return AsETSObjectType(&GlobalTypesHolder::GlobalETSObjectType);
}

ETSUnionType *ETSChecker::GlobalETSUnionUndefinedNull() const
{
    auto *ret = (GetGlobalTypesHolder()->*&GlobalTypesHolder::GlobalETSUnionUndefinedNull)();
    return ret != nullptr ? ret->AsETSUnionType() : nullptr;
}

ETSUnionType *ETSChecker::GlobalETSUnionUndefinedNullObject() const
{
    auto *ret = (GetGlobalTypesHolder()->*&GlobalTypesHolder::GlobalETSUnionUndefinedNullObject)();
    return ret != nullptr ? ret->AsETSUnionType() : nullptr;
}

ETSObjectType *ETSChecker::GlobalBuiltinClassType() const
{
    return AsETSObjectType(&GlobalTypesHolder::GlobalClassBuiltinType);
}

ETSObjectType *ETSChecker::GlobalBuiltinETSResizableArrayType() const
{
    return AsETSObjectType(&GlobalTypesHolder::GlobalArrayBuiltinType);
}

ETSObjectType *ETSChecker::GlobalBuiltinETSStringType() const
{
    return AsETSObjectType(&GlobalTypesHolder::GlobalETSStringBuiltinType);
}

ETSObjectType *ETSChecker::GlobalBuiltinETSBigIntType() const
{
    return AsETSObjectType(&GlobalTypesHolder::GlobalETSBigIntBuiltinType);
}

ETSObjectType *ETSChecker::GlobalBuiltinTypeType() const
{
    return AsETSObjectType(&GlobalTypesHolder::GlobalTypeBuiltinType);
}

ETSObjectType *ETSChecker::GlobalBuiltinErrorType() const
{
    return AsETSObjectType(&GlobalTypesHolder::GlobalErrorBuiltinType);
}

ETSObjectType *ETSChecker::GlobalStringBuilderBuiltinType() const
{
    return AsETSObjectType(&GlobalTypesHolder::GlobalStringBuilderBuiltinType);
}

ETSObjectType *ETSChecker::GlobalBuiltinPromiseType() const
{
    return AsETSObjectType(&GlobalTypesHolder::GlobalPromiseBuiltinType);
}

ETSObjectType *ETSChecker::GlobalBuiltinFunctionType() const
{
    return AsETSObjectType(&GlobalTypesHolder::GlobalFunctionBuiltinType);
}

ETSObjectType *ETSChecker::GlobalBuiltinFunctionType(size_t nargs, bool hasRest) const
{
    return AsETSObjectType(&GlobalTypesHolder::GlobalFunctionBuiltinType, nargs, hasRest);
}

ETSObjectType *ETSChecker::GlobalBuiltinLambdaType(size_t nargs, bool hasRest) const
{
    return AsETSObjectType(&GlobalTypesHolder::GlobalLambdaBuiltinType, nargs, hasRest);
}

ETSObjectType *ETSChecker::GlobalBuiltinTupleType(size_t nargs) const
{
    return AsETSObjectType(&GlobalTypesHolder::GlobalTupleBuiltinType, nargs);
}

size_t ETSChecker::GlobalBuiltinFunctionTypeVariadicThreshold() const
{
    return GetGlobalTypesHolder()->VariadicFunctionTypeThreshold();
}

ETSObjectType *ETSChecker::GlobalBuiltinBoxType(Type *contents)
{
    ES2PANDA_ASSERT(contents->IsETSReferenceType());
    if (!contents->IsETSUnboxableObject()) {
        auto *base = AsETSObjectType(&GlobalTypesHolder::GlobalBoxBuiltinType);
        auto substitution = Substitution {};
        ES2PANDA_ASSERT(base != nullptr);
        substitution.emplace(base->TypeArguments()[0]->AsETSTypeParameter(), contents);
        return base->Substitute(Relation(), &substitution);
    }

    switch (contents->AsETSObjectType()->UnboxableKind()) {
        case ETSObjectFlags::BUILTIN_BOOLEAN:
            return AsETSObjectType(&GlobalTypesHolder::GlobalBooleanBoxBuiltinType);
        case ETSObjectFlags::BUILTIN_BYTE:
            return AsETSObjectType(&GlobalTypesHolder::GlobalByteBoxBuiltinType);
        case ETSObjectFlags::BUILTIN_SHORT:
            return AsETSObjectType(&GlobalTypesHolder::GlobalShortBoxBuiltinType);
        case ETSObjectFlags::BUILTIN_CHAR:
            return AsETSObjectType(&GlobalTypesHolder::GlobalCharBoxBuiltinType);
        case ETSObjectFlags::BUILTIN_INT:
            return AsETSObjectType(&GlobalTypesHolder::GlobalIntBoxBuiltinType);
        case ETSObjectFlags::BUILTIN_LONG:
            return AsETSObjectType(&GlobalTypesHolder::GlobalLongBoxBuiltinType);
        case ETSObjectFlags::BUILTIN_FLOAT:
            return AsETSObjectType(&GlobalTypesHolder::GlobalFloatBoxBuiltinType);
        case ETSObjectFlags::BUILTIN_DOUBLE:
            return AsETSObjectType(&GlobalTypesHolder::GlobalDoubleBoxBuiltinType);
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

GlobalArraySignatureMap &ETSChecker::GlobalArrayTypes()
{
    return globalArraySignatures_;
}

const GlobalArraySignatureMap &ETSChecker::GlobalArrayTypes() const
{
    return globalArraySignatures_;
}

Type *ETSChecker::GlobalTypeError() const
{
    return GetGlobalTypesHolder()->GlobalTypeError();
}

Type *ETSChecker::InvalidateType(ir::Typed<ir::AstNode> *node)
{
    return node->SetTsType(GlobalTypeError());
}

Type *ETSChecker::TypeError(ir::Typed<ir::AstNode> *node, const diagnostic::DiagnosticKind &diagKind,
                            const lexer::SourcePosition &at)
{
    return TypeError(node, diagKind, util::DiagnosticMessageParams {}, at);
}

Type *ETSChecker::TypeError(ir::Typed<ir::AstNode> *node, const diagnostic::DiagnosticKind &diagKind,
                            const util::DiagnosticMessageParams &list, const lexer::SourcePosition &at)
{
    LogError(diagKind, list, at);
    return InvalidateType(node);
}

Type *ETSChecker::TypeError(varbinder::Variable *var, const diagnostic::DiagnosticKind &diagKind,
                            const lexer::SourcePosition &at)
{
    return TypeError(var, diagKind, {}, at);
}

Type *ETSChecker::TypeError(varbinder::Variable *var, const diagnostic::DiagnosticKind &diagKind,
                            const util::DiagnosticMessageParams &list, const lexer::SourcePosition &at)
{
    LogError(diagKind, list, at);
    var->SetTsType(GlobalTypeError());
    return var->TsType();
}

void ETSChecker::HandleUpdatedCallExpressionNode(ir::CallExpression *callExpr)
{
    // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
    VarBinder()->AsETSBinder()->HandleCustomNodes(callExpr);
}

Type *ETSChecker::SelectGlobalIntegerTypeForNumeric(Type *type) const
{
    if (type->IsETSObjectType()) {
        auto const *objectType = type->AsETSObjectType();

        if (objectType->HasObjectFlag(ETSObjectFlags::BUILTIN_FLOAT)) {
            return GlobalIntBuiltinType();
        }

        if (objectType->HasObjectFlag(ETSObjectFlags::BUILTIN_DOUBLE)) {
            return GlobalLongBuiltinType();
        }
    }
    return type;
}

Signature *ETSChecker::FindExtensionSetterInMap(util::StringView name, ETSObjectType *type)
{
    return GetGlobalTypesHolder()->FindExtensionSetterInMap(name, type);
}

Signature *ETSChecker::FindExtensionGetterInMap(util::StringView name, ETSObjectType *type)
{
    return GetGlobalTypesHolder()->FindExtensionGetterInMap(name, type);
}

void ETSChecker::InsertExtensionSetterToMap(util::StringView name, ETSObjectType *type, Signature *sig)
{
    GetGlobalTypesHolder()->InsertExtensionSetterToMap(name, type, sig);
}

bool ETSChecker::HasParameterlessConstructor(checker::Type *type)
{
    if (!type->IsETSObjectType()) {
        return false;
    }

    auto *objType = type->AsETSObjectType();
    for (auto *ctorSig : objType->ConstructSignatures()) {
        if (ctorSig != nullptr && ctorSig->Params().empty() && !ctorSig->HasRestParameter()) {
            return true;
        }
    }

    return false;
}

void ETSChecker::InsertExtensionGetterToMap(util::StringView name, ETSObjectType *type, Signature *sig)
{
    GetGlobalTypesHolder()->InsertExtensionGetterToMap(name, type, sig);
}

bool ETSChecker::TypeHasDefaultValue(Type *tp) const
{
    return tp->IsBuiltinNumeric() || tp->IsETSBooleanType() || tp->IsETSCharType() ||
           Relation()->IsSupertypeOf(GlobalETSUndefinedType(), tp);
}

/* Invoke method name in functional interfaces */
std::string ETSChecker::FunctionalInterfaceInvokeName(size_t arity, bool hasRest)
{
    if (arity < GlobalBuiltinFunctionTypeVariadicThreshold()) {
        return (hasRest ? "invokeR" : "invoke") + std::to_string(arity);
    }
    return "unsafeCall";
}

}  // namespace ark::es2panda::checker
