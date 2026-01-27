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

#include <utility>
#include "ETSGen-inl.h"

#include "assembler/mangling.h"
#include "compiler/core/ETSemitter.h"
#include "compiler/core/codeGen.h"
#include "compiler/core/emitter.h"
#include "compiler/core/regScope.h"
#include "generated/isa.h"
#include "generated/signatures.h"
#include "ir/base/scriptFunction.h"
#include "ir/base/classDefinition.h"
#include "ir/expression.h"
#include "ir/statement.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/binaryExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/expressions/memberExpression.h"
#include "ir/expressions/templateLiteral.h"
#include "ir/statements/breakStatement.h"
#include "ir/statements/continueStatement.h"
#include "ir/statements/tryStatement.h"
#include "ir/ts/tsInterfaceDeclaration.h"
#include "compiler/base/lreference.h"
#include "compiler/base/catchTable.h"
#include "compiler/core/dynamicContext.h"
#include "libarkbase/macros.h"
#include "util/es2pandaMacros.h"
#include "varbinder/ETSBinder.h"
#include "varbinder/variable.h"
#include "checker/types/type.h"
#include "checker/types/signature.h"
#include "checker/checker.h"
#include "checker/ETSchecker.h"
#include "checker/types/ets/etsObjectType.h"
#include "checker/types/ets/etsTupleType.h"
#include "checker/types/ets/etsAsyncFuncReturnType.h"
#include "parser/program/program.h"
#include "checker/types/globalTypesHolder.h"
#include "public/public.h"

namespace ark::es2panda::compiler {

static inline bool IsWidePrimitiveType(checker::Type const *type)
{
    return type->IsLongType() || type->IsDoubleType();
}

ETSGen::ETSGen(SArenaAllocator *allocator, RegSpiller *spiller, public_lib::Context *context,
               std::tuple<varbinder::FunctionScope *, ProgramElement *, AstCompiler *> toCompile) noexcept
    : CodeGen(allocator, spiller, context, toCompile),
      containingObjectType_(util::Helpers::GetContainingObjectType(RootNode()))
{
    ETSFunction::Compile(this);
}

static util::StringView MakeView(ETSGen const *etsg, std::string const &str)
{
    auto alloc = etsg->Allocator();
    return util::StringView(std::string_view(*alloc->New<SArenaString>(str, alloc->Adapter())));
}

void ETSGen::SetAccumulatorType(const checker::Type *type)
{
    SetVRegType(acc_, type);
}

const checker::Type *ETSGen::GetAccumulatorType() const
{
    return GetVRegType(acc_);
}

void ETSGen::CompileAndCheck(const ir::Expression *expr)
{
    // NOTE: vpukhov. bad accumulator type leads to terrible bugs in codegen
    // make exact types match mandatory
    expr->Compile(this);

    auto const *const accType = GetAccumulatorType();
    auto const *const exprType = expr->TsType();

    if (Checker()->Relation()->IsIdenticalTo(accType, exprType) || exprType->IsETSTypeParameter() ||
        exprType->IsETSPartialTypeParameter() || exprType->IsETSNonNullishType()) {
        return;
    }

    ES2PANDA_ASSERT(accType != nullptr);
    if (accType->IsETSPrimitiveType() &&
        ((accType->TypeFlags() ^ exprType->TypeFlags()) & ~checker::TypeFlag::CONSTANT) == 0) {
        return;
    }

    ASSERT_PRINT(false, std::string("Type mismatch after Expression::Compile: ") + accType->ToString() +
                            " instead of " + exprType->ToString());
}

const checker::ETSChecker *ETSGen::Checker() const noexcept
{
    return Context()->GetChecker()->AsETSChecker();
}

const varbinder::ETSBinder *ETSGen::VarBinder() const noexcept
{
    return Context()->parserProgram->VarBinder()->AsETSBinder();
}

const checker::Type *ETSGen::ReturnType() const noexcept
{
    return RootNode()->AsScriptFunction()->Signature()->ReturnType();
}

const checker::ETSObjectType *ETSGen::ContainingObjectType() const noexcept
{
    return containingObjectType_;
}

ETSEmitter *ETSGen::Emitter() const
{
    return static_cast<ETSEmitter *>(Context()->emitter);
}

VReg &ETSGen::Acc() noexcept
{
    return acc_;
}

VReg ETSGen::Acc() const noexcept
{
    return acc_;
}

void ETSGen::ApplyConversionAndStoreAccumulator(const ir::AstNode *const node, const VReg vreg,
                                                const checker::Type *const targetType)
{
    ApplyConversion(node, targetType);
    StoreAccumulator(node, vreg);
}

VReg ETSGen::StoreError(const ir::AstNode *node)
{
    VReg error = AllocReg();
    Ra().Emit<StaObj>(node, error);

    SetAccumulatorType(Checker()->GlobalBuiltinErrorType());
    SetVRegType(error, GetAccumulatorType());
    return error;
}

void ETSGen::StoreAccumulator(const ir::AstNode *const node, const VReg vreg)
{
    const auto *const accType = GetAccumulatorType();

    ES2PANDA_ASSERT(accType != nullptr);
    if (accType->IsETSReferenceType()) {
        Ra().Emit<StaObj>(node, vreg);
    } else if (IsWidePrimitiveType(accType)) {
        Ra().Emit<StaWide>(node, vreg);
    } else {
        Ra().Emit<Sta>(node, vreg);
    }

    SetVRegType(vreg, accType);
}

void ETSGen::LoadAccumulator(const ir::AstNode *node, VReg vreg)
{
    const auto *const vregType = GetVRegType(vreg);

    ES2PANDA_ASSERT(vregType != nullptr);
    if (vregType->IsETSReferenceType()) {
        Ra().Emit<LdaObj>(node, vreg);
    } else if (IsWidePrimitiveType(vregType)) {
        Ra().Emit<LdaWide>(node, vreg);
    } else {
        Ra().Emit<Lda>(node, vreg);
    }

    SetAccumulatorType(vregType);
}

IRNode *ETSGen::AllocMov(const ir::AstNode *const node, const VReg vd, const VReg vs)
{
    const auto *const sourceType = GetVRegType(vs);
    // CC-OFFNXT(G.FMT.14-CPP) project code style
    auto *const mov = [this, sourceType, node, vd, vs]() -> IRNode * {
        if (sourceType->IsETSReferenceType()) {
            return Allocator()->New<MovObj>(node, vd, vs);
        }
        if (IsWidePrimitiveType(sourceType)) {
            return Allocator()->New<MovWide>(node, vd, vs);
        }
        return Allocator()->New<Mov>(node, vd, vs);
    }();

    SetVRegType(vd, sourceType);
    return mov;
}

IRNode *ETSGen::AllocMov(const ir::AstNode *const node, OutVReg vd, const VReg vs)
{
    return AllocSpillMov(node, *vd.reg, vs, vd.type);
}

checker::Type const *ETSGen::TypeForVar(varbinder::Variable const *var) const noexcept
{
    return var->TsType();
}

IRNode *ETSGen::AllocSpillMov(const ir::AstNode *node, VReg vd, VReg vs, OperandType type)
{
    ES2PANDA_ASSERT(type != OperandType::ANY);
    ES2PANDA_ASSERT(type != OperandType::NONE);

    switch (type) {
        case OperandType::REF:
            return Allocator()->New<MovObj>(node, vd, vs);
        case OperandType::B64:
            return Allocator()->New<MovWide>(node, vd, vs);
        case OperandType::B32:
            return Allocator()->New<Mov>(node, vd, vs);
        default:
            ES2PANDA_UNREACHABLE();
            break;
    }

    return Allocator()->New<Mov>(node, vd, vs);
}

void ETSGen::MoveVreg(const ir::AstNode *const node, const VReg vd, const VReg vs)
{
    const auto *const sourceType = GetVRegType(vs);
    ES2PANDA_ASSERT(sourceType != nullptr);

    if (sourceType->IsETSReferenceType()) {
        Ra().Emit<MovObj>(node, vd, vs);
    } else if (IsWidePrimitiveType(sourceType)) {
        Ra().Emit<MovWide>(node, vd, vs);
    } else {
        Ra().Emit<Mov>(node, vd, vs);
    }

    SetVRegType(vd, sourceType);
}

// indicates that initializer is meaningless and may lead to NPEs
void ETSGen::LoadAccumulatorPoison(const ir::AstNode *node, const checker::Type *type)
{
    ES2PANDA_ASSERT(type->IsETSReferenceType());
    LoadAccumulatorUndefined(node);
    SetAccumulatorType(type);
}

void ETSGen::LoadVar(const ir::Identifier *node, varbinder::Variable const *const var)
{
    auto *local = var->AsLocalVariable();

    switch (ETSLReference::ResolveReferenceKind(var)) {
        case ReferenceKind::STATIC_FIELD: {
            LoadStaticProperty(node, var->TsType(), FormClassPropReference(var));
            break;
        }
        case ReferenceKind::FIELD: {
            ES2PANDA_ASSERT(GetVRegType(GetThisReg()) != nullptr);
            LoadProperty(node, var->TsType(), GetThisReg(), FormClassPropReference(var));
            break;
        }
        case ReferenceKind::METHOD:
        case ReferenceKind::STATIC_METHOD:
        case ReferenceKind::CLASS:
        case ReferenceKind::STATIC_CLASS: {
            SetAccumulatorType(var->TsType());
            break;
        }
        case ReferenceKind::LOCAL: {
            LoadAccumulator(node, local->Vreg());
            SetAccumulatorType(GetVRegType(local->Vreg()));
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

void ETSGen::StoreVar(const ir::Identifier *node, const varbinder::ConstScopeFindResult &result)
{
    auto *local = result.variable->AsLocalVariable();
    ApplyConversion(node, local->TsType());

    switch (ETSLReference::ResolveReferenceKind(result.variable)) {
        case ReferenceKind::STATIC_FIELD: {
            StoreStaticProperty(node, result.variable->TsType(), FormClassPropReference(result.variable));
            break;
        }
        case ReferenceKind::FIELD: {
            StoreProperty(node, result.variable->TsType(), GetThisReg(), FormClassPropReference(result.variable));
            break;
        }
        case ReferenceKind::LOCAL: {
            StoreAccumulator(node, local->Vreg());
            SetVRegType(local->Vreg(), GetAccumulatorType());
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

util::StringView ETSGen::AssemblerReference(util::StringView ref)
{
    Emitter()->AddDependence(ref.Mutf8());
    return ref;
}

util::StringView ETSGen::AssemblerSignatureReference(util::StringView ref)
{
    auto const funcName = pandasm::GetFunctionNameFromSignature(std::string(ref));
    ES2PANDA_ASSERT(!funcName.empty());
    auto const className = funcName.substr(0, funcName.rfind('.'));
    AssemblerReference(util::StringView(className));

    return AssemblerReference(ref);
}

util::StringView ETSGen::AssemblerReference(checker::Signature const *sig)
{
    return AssemblerSignatureReference(sig->InternalName());  // simplify
}

util::StringView ETSGen::FormClassOwnPropReference(const checker::ETSObjectType *classType,
                                                   const util::StringView &name)
{
    std::stringstream ss;
    ES2PANDA_ASSERT(classType != nullptr);
    ss << ToAssemblerType(classType) << Signatures::METHOD_SEPARATOR << name;
    return util::StringView(*ProgElement()->Strings().emplace(Emitter()->AddDependence(ss.str())).first);
}

util::StringView ETSGen::FormClassPropReference(varbinder::Variable const *const var)
{
    auto *node = var->Declaration()->Node();
    ES2PANDA_ASSERT(node->IsClassProperty());
    if (node->IsOverride()) {
        ES2PANDA_ASSERT(node->AsClassProperty()->BasePropertyVar() != nullptr);
        return FormClassPropReference(node->AsClassProperty()->BasePropertyVar());
    }
    auto containingObjectType = util::Helpers::GetContainingObjectType(var->Declaration()->Node());
    return FormClassOwnPropReference(containingObjectType, var->Name());
}

void ETSGen::StoreStaticProperty(const ir::AstNode *const node, const checker::Type *propType,
                                 const util::StringView &fullName)
{
    if (propType->IsETSReferenceType()) {
        Sa().Emit<StstaticObj>(node, fullName);
    } else if (IsWidePrimitiveType(propType)) {
        Sa().Emit<StstaticWide>(node, fullName);
    } else {
        Sa().Emit<Ststatic>(node, fullName);
    }
}

static bool StaticAccessRequiresReferenceSafetyCheck(const ir::AstNode *const node, const checker::Type *propType)
{
    if (propType->PossiblyETSUndefined()) {
        return false;
    }
    auto parent = node->Parent();
    if (parent->IsMemberExpression()) {
        return false;
    }
    if (parent->IsCallExpression() && parent->AsCallExpression()->Callee() == node) {
        return false;
    }
    return true;
}

void ETSGen::LoadStaticProperty(const ir::AstNode *const node, const checker::Type *propType,
                                const util::StringView &fullName)
{
    ES2PANDA_ASSERT(propType != nullptr);
    if (propType->IsETSReferenceType()) {
        Sa().Emit<LdstaticObj>(node, fullName);
        if (StaticAccessRequiresReferenceSafetyCheck(node, propType)) {
            EmitNullcheck(node);
        }
    } else if (IsWidePrimitiveType(propType)) {
        Sa().Emit<LdstaticWide>(node, fullName);
    } else {
        Sa().Emit<Ldstatic>(node, fullName);
    }

    SetAccumulatorType(propType);
}

void ETSGen::StoreProperty(const ir::AstNode *const node, const checker::Type *propType, const VReg objReg,
                           const util::StringView &fullName)
{
    if (propType->IsETSReferenceType()) {
        Ra().Emit<StobjObj>(node, objReg, fullName);
    } else if (IsWidePrimitiveType(propType)) {
        Ra().Emit<StobjWide>(node, objReg, fullName);
    } else {
        Ra().Emit<Stobj>(node, objReg, fullName);
    }
}

void ETSGen::StorePropertyByNameAny(const ir::AstNode *const node, const VReg objReg, const util::StringView &fullName)
{
    ES2PANDA_ASSERT(node->IsMemberExpression() &&
                    Checker()->GetApparentType(node->AsMemberExpression()->Object()->TsType())->IsETSAnyType());
    Ra().Emit<AnyStbyname>(node, objReg, fullName);
    SetAccumulatorType(node->AsMemberExpression()->TsType());
}

void ETSGen::LoadPropertyByNameAny(const ir::AstNode *const node, const VReg objReg, const util::StringView &fullName)
{
    ES2PANDA_ASSERT(node->IsMemberExpression() &&
                    Checker()->GetApparentType(node->AsMemberExpression()->Object()->TsType())->IsETSAnyType());
    Ra().Emit<AnyLdbyname>(node, objReg, fullName);
    SetAccumulatorType(node->AsMemberExpression()->TsType());
}

void ETSGen::LoadProperty(const ir::AstNode *const node, const checker::Type *propType, const VReg objReg,
                          const util::StringView &fullName)
{
    if (propType->IsETSReferenceType()) {
        Ra().Emit<LdobjObj>(node, objReg, fullName);
    } else if (IsWidePrimitiveType(propType)) {
        Ra().Emit<LdobjWide>(node, objReg, fullName);
    } else {
        Ra().Emit<Ldobj>(node, objReg, fullName);
    }

    SetAccumulatorType(propType);
}

void ETSGen::StorePropertyByName([[maybe_unused]] const ir::AstNode *node, [[maybe_unused]] VReg objReg,
                                 [[maybe_unused]] checker::ETSChecker::NamedAccessMeta const &fieldMeta)
{
#ifdef PANDA_WITH_ETS
    auto [metaObj, propType, propName] = fieldMeta;
    const auto fullName = FormClassOwnPropReference(metaObj, propName);

    if (propType->IsETSReferenceType()) {
        Ra().Emit<EtsStobjNameObj>(node, objReg, fullName);
    } else if (IsWidePrimitiveType(propType)) {
        Ra().Emit<EtsStobjNameWide>(node, objReg, fullName);
    } else {
        Ra().Emit<EtsStobjName>(node, objReg, fullName);
    }
#else
    ES2PANDA_UNREACHABLE();
#endif  // PANDA_WITH_ETS
}

void ETSGen::LoadPropertyByName([[maybe_unused]] const ir::AstNode *const node, [[maybe_unused]] VReg objReg,
                                [[maybe_unused]] checker::ETSChecker::NamedAccessMeta const &fieldMeta)
{
#ifdef PANDA_WITH_ETS
    auto [metaObj, propType, propName] = fieldMeta;
    const auto fullName = FormClassOwnPropReference(metaObj, propName);

    if (propType->IsETSReferenceType()) {
        Ra().Emit<EtsLdobjNameObj>(node, objReg, fullName);
    } else if (IsWidePrimitiveType(propType)) {
        Ra().Emit<EtsLdobjNameWide>(node, objReg, fullName);
    } else {
        Ra().Emit<EtsLdobjName>(node, objReg, fullName);
    }
    SetAccumulatorType(propType);
#else
    ES2PANDA_UNREACHABLE();
#endif  // PANDA_WITH_ETS
}

void ETSGen::StoreByIndexAny(const ir::MemberExpression *node, VReg objectReg, VReg index)
{
    RegScope rs(this);

    // Store property by index
    Ra().Emit<AnyStbyidx>(node, objectReg, index);
    SetAccumulatorType(Checker()->GlobalVoidType());
}

void ETSGen::LoadByIndexAny(const ir::MemberExpression *node, VReg objectReg)
{
    RegScope rs(this);

    VReg indexReg = AllocReg();
    StoreAccumulator(node, indexReg);

    // Get property by index
    Ra().Emit<AnyLdbyidx>(node, objectReg);
    SetAccumulatorType(node->TsType());
}

void ETSGen::StoreByValueAny(const ir::MemberExpression *node, VReg objectReg, VReg value)
{
    RegScope rs(this);

    // Store property by value
    Ra().Emit<AnyStbyval>(node, objectReg, value);
    SetAccumulatorType(Checker()->GlobalVoidType());
}

void ETSGen::LoadByValueAny(const ir::MemberExpression *node, VReg objectReg)
{
    RegScope rs(this);

    VReg valueReg = AllocReg();
    StoreAccumulator(node, valueReg);

    // Get property by value
    Ra().Emit<AnyLdbyval>(node, objectReg, valueReg);
    SetAccumulatorType(node->TsType());
}

void ETSGen::CallRangeFillUndefined(const ir::AstNode *const node, checker::Signature *const signature,
                                    const VReg thisReg)
{
    RegScope rs(this);
    ES2PANDA_ASSERT(signature->MinArgCount() == 0);

    auto undef = AllocReg();
    LoadAccumulatorUndefined(node);
    StoreAccumulator(node, undef);

    VReg const argStart = NextReg();
    Ra().Emit<MovObj>(node, AllocReg(), thisReg);

    for (size_t idx = 0; idx < signature->ArgCount(); idx++) {
        Ra().Emit<MovObj>(node, AllocReg(), undef);
    }
    Rra().Emit<CallRange>(node, argStart, signature->ArgCount() + 1, AssemblerReference(signature), argStart);
}

void ETSGen::LoadThis(const ir::AstNode *node)
{
    LoadAccumulator(node, GetThisReg());
}

void ETSGen::CreateBigIntObject(const ir::AstNode *node, VReg arg0, std::string_view signature)
{
    Ra().Emit<InitobjShort>(node, AssemblerSignatureReference(signature), arg0, dummyReg_);
}

VReg ETSGen::GetThisReg() const
{
    const auto res = Scope()->Find(varbinder::VarBinder::MANDATORY_PARAM_THIS);
    return res.variable->AsLocalVariable()->Vreg();
}

const checker::Type *ETSGen::LoadDefaultValue(const ir::AstNode *node, const checker::Type *type)
{
    if (type->IsETSAsyncFuncReturnType()) {
        LoadDefaultValue(node, type->AsETSAsyncFuncReturnType()->GetPromiseTypeArg());
        return type;
    }

    auto const checker = const_cast<checker::ETSChecker *>(Checker());

    if (type->IsETSReferenceType()) {
        if (checker->Relation()->IsSupertypeOf(type, checker->GlobalETSUndefinedType())) {
            LoadAccumulatorUndefined(node);
        } else if (type->IsETSObjectType() && type->AsETSObjectType()->IsBoxedPrimitive()) {
            //  Call default constructor for boxed primitive types.
            static auto const DUMMY_ARGS = ArenaVector<ir::Expression *>(checker->Allocator()->Adapter());
            auto const &signatures = type->AsETSObjectType()->ConstructSignatures();
            auto const it = std::find_if(signatures.cbegin(), signatures.cend(), [](checker::Signature *signature) {
                return signature->ArgCount() == 0U && !signature->HasRestParameter();
            });
            if (it != signatures.cend()) {
                InitObject(node, *it, DUMMY_ARGS);
            } else {
                LoadAccumulatorPoison(node, type);
            }
        } else {
            LoadAccumulatorPoison(node, type);
        }
        return type;
    }

    if (type->IsETSBooleanType()) {
        LoadAccumulatorBoolean(node, type->AsETSBooleanType()->GetValue());
    } else {
        const auto ttctx = TargetTypeContext(this, type);
        LoadAccumulatorInt(node, 0);
    }
    return type;
}

void ETSGen::EmitReturnVoid(const ir::AstNode *node)
{
    Sa().Emit<ReturnVoid>(node);
}

void ETSGen::ReturnAcc(const ir::AstNode *node)
{
    const auto *const accType = GetAccumulatorType();
    ES2PANDA_ASSERT(accType != nullptr);

    if (accType->IsETSReferenceType()) {
        Sa().Emit<ReturnObj>(node);
    } else if (IsWidePrimitiveType(accType)) {
        Sa().Emit<ReturnWide>(node);
    } else {
        Sa().Emit<Return>(node);
    }
}

bool ETSGen::IsNullUnsafeObjectType(checker::Type const *type) const
{
    ES2PANDA_ASSERT(type != nullptr);
    auto const checker = const_cast<checker::ETSChecker *>(Checker());
    return checker->Relation()->IsSupertypeOf(checker->GetApparentType(type), checker->GlobalETSObjectType());
}

// Implemented on top of the runtime type system, do not relax checks, do not introduce new types
// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP, G.FUD.05) solid logic
void ETSGen::IsInstance(const ir::AstNode *const node, const VReg srcReg, const checker::Type *target)
{
    target = Checker()->GetApparentType(target);
    ES2PANDA_ASSERT(target != nullptr && target->IsETSReferenceType() && GetAccumulatorType() != nullptr);

    if (target->IsETSAnyType()) {
        LoadAccumulatorBoolean(node, true);
        return;
    }
    if (target->IsETSNeverType()) {
        LoadAccumulatorBoolean(node, false);
        return;
    }

    LoadAccumulator(node, srcReg);
    if (target->IsETSArrayType() || (target->IsETSObjectType() &&
                                     !(IsNullUnsafeObjectType(target) && GetAccumulatorType()->PossiblyETSNullish()))) {
        if (ToAssemblerType(target) != Signatures::BUILTIN_OBJECT) {
            EmitIsInstance(node, ToAssemblerType(target));
        } else {
            LoadAccumulatorBoolean(node, true);
        }
        SetAccumulatorType(Checker()->GlobalETSBooleanType());
        return;
    }

    auto ifTrue = AllocLabel();

    if (target->IsETSVoidType()) {
        BranchIfUndefined(node, ifTrue);
    } else if (target->DefinitelyETSNullish()) {
        if (target->PossiblyETSUndefined()) {
            BranchIfUndefined(node, ifTrue);
        }
        if (target->PossiblyETSNull()) {
            BranchIfNull(node, ifTrue);
        }
    } else {
        auto ifFalse = AllocLabel();
        if (target->PossiblyETSUndefined()) {
            BranchIfUndefined(node, ifTrue);
        }
        ES2PANDA_ASSERT(!target->IsETSTypeAliasType());
        if (!target->PossiblyETSNull() && IsNullUnsafeObjectType(target)) {
            BranchIfNull(node, ifFalse);
            LoadAccumulator(node, srcReg);
        }
        if (ToAssemblerType(target) == Signatures::BUILTIN_OBJECT) {
            if (!target->PossiblyETSUndefined()) {
                BranchIfUndefined(node, ifFalse);
            }
            JumpTo(node, ifTrue);
        } else {
            EmitIsInstance(node, ToAssemblerType(target));
            BranchIfTrue(node, ifTrue);
        }
        SetLabel(node, ifFalse);
    }

    auto end = AllocLabel();
    LoadAccumulatorBoolean(node, false);
    JumpTo(node, end);
    SetLabel(node, ifTrue);
    LoadAccumulatorBoolean(node, true);
    SetLabel(node, end);
}

// Implemented on top of the runtime type system, do not relax checks, do not introduce new types
// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP, G.FUD.05) solid logic
void ETSGen::CheckedReferenceNarrowing(const ir::AstNode *node, const checker::Type *target)
{
    target = Checker()->GetApparentType(target);
    auto const source = GetAccumulatorType();
    ES2PANDA_ASSERT(target != nullptr && source != nullptr);
    ES2PANDA_ASSERT(target->IsETSReferenceType());

    const RegScope rs(this);
    const auto srcReg = AllocReg();
    auto end = AllocLabel();

    StoreAccumulator(node, srcReg);
    if (target->IsETSVoidType() || target->IsETSAnyType()) {  // NOTE(vpukhov): #19701 void refactoring
        SetAccumulatorType(target);
        return;
    }
    if (target->IsETSNeverType()) {
        EmitFailedTypeCastException(node, srcReg, target);
        SetAccumulatorType(target);
        return;
    }
    if (target->DefinitelyETSNullish()) {
        if (target->PossiblyETSUndefined()) {
            BranchIfUndefined(node, end);
        }
        if (target->PossiblyETSNull()) {
            BranchIfNull(node, end);
        }
        EmitFailedTypeCastException(node, srcReg, target);
        SetLabel(node, end);
        LoadAccumulator(node, srcReg);
        if (!target->IsETSUndefinedType()) {
            EmitCheckCast(node, ToAssemblerType(target));
        }
        SetAccumulatorType(target);
        return;
    }

    auto isNullish = AllocLabel();
    bool nullishCheck = false;

    if (source->PossiblyETSUndefined() && !target->PossiblyETSUndefined()) {
        nullishCheck = true;
        BranchIfUndefined(node, isNullish);
    }
    if (source->PossiblyETSNull() && !target->PossiblyETSNull() && IsNullUnsafeObjectType(target)) {
        nullishCheck = true;
        BranchIfNull(node, isNullish);
    }

    if (!nullishCheck) {
        EmitCheckCast(node, ToAssemblerType(target));
    } else {
        LoadAccumulator(node, srcReg);
        EmitCheckCast(node, ToAssemblerType(target));
        JumpTo(node, end);

        SetLabel(node, isNullish);
        EmitFailedTypeCastException(node, srcReg, target);

        SetLabel(node, end);
    }
    SetAccumulatorType(target);
}

void ETSGen::GuardUncheckedType(const ir::AstNode *node, const checker::Type *unchecked, const checker::Type *target)
{
    if (unchecked != nullptr) {
        SetAccumulatorType(unchecked);
        // this check guards possible type violations, **do not relax it**
        CheckedReferenceNarrowing(node, Checker()->MaybeBoxType(target));
        // Because on previous step accumulator type may be set in CheckerReferenceNarrowing to boxed counterpart of
        // target We need to apply unbox conversion if needed to avoid RTE
        ES2PANDA_ASSERT(GetAccumulatorType() != nullptr);
        if (target->IsETSPrimitiveType() && GetAccumulatorType()->IsETSUnboxableObject()) {
            ApplyConversion(node, target);
        }
    }
    SetAccumulatorType(target);
}

void ETSGen::EmitFailedTypeCastException(const ir::AstNode *node, const VReg src, checker::Type const *target)
{
    const RegScope rs(this);
    const auto typeReg = AllocReg();

    bool useExclUndefinedStub;

    if (target->IsETSUndefinedType() || target->IsETSNeverType()) {
        useExclUndefinedStub = false;
        LoadAccumulatorUndefined(node);
    } else {
        useExclUndefinedStub = !const_cast<checker::ETSChecker *>(Checker())->Relation()->IsSupertypeOf(
            target, Checker()->GlobalETSUndefinedType());
        EmitLdaType(node, ToAssemblerType(target));
    }
    StoreAccumulator(node, typeReg);

    const auto stubSignature = useExclUndefinedStub
                                   ? compiler::Signatures::BUILTIN_RUNTIME_FAILED_TYPE_CAST_EXCL_UNDEFINED_STUB
                                   : compiler::Signatures::BUILTIN_RUNTIME_FAILED_TYPE_CAST_INCL_UNDEFINED_STUB;
    Ra().Emit<CallShort>(node, AssemblerSignatureReference(stubSignature), src, typeReg);

    const auto errorReg = AllocReg();
    StoreAccumulator(node, errorReg);
    EmitThrow(node, errorReg);
    SetAccumulatorType(nullptr);
}

void ETSGen::LoadConstantObject(const ir::Expression *node, const checker::Type *type)
{
    if (type->IsETSBigIntType()) {
        LoadAccumulatorBigInt(node, type->AsETSObjectType()->AsETSBigIntType()->GetValue());
        const VReg value = AllocReg();
        StoreAccumulator(node, value);
        CreateBigIntObject(node, value);
    } else if (type->IsETSStringType()) {
        LoadAccumulatorString(node, type->AsETSObjectType()->AsETSStringType()->GetValue());
        SetAccumulatorType(node->TsType());
    } else {
        ES2PANDA_UNREACHABLE();
    }
}

void ETSGen::ApplyConversionCast(const ir::AstNode *node, const checker::Type *targetType)
{
    switch (checker::ETSChecker::TypeKind(targetType)) {
        case checker::TypeFlag::DOUBLE: {
            CastToDouble(node);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            CastToFloat(node);
            break;
        }
        case checker::TypeFlag::LONG: {
            CastToLong(node);
            break;
        }
        case checker::TypeFlag::INT: {
            CastToInt(node);
            break;
        }
        case checker::TypeFlag::CHAR: {
            CastToChar(node);
            break;
        }
        case checker::TypeFlag::SHORT: {
            CastToShort(node);
            break;
        }
        case checker::TypeFlag::BYTE: {
            CastToByte(node);
            break;
        }
        case checker::TypeFlag::ETS_ARRAY:
        case checker::TypeFlag::ETS_OBJECT:
        default: {
            break;
        }
    }
}

void ETSGen::ApplyConversion(const ir::AstNode *node, const checker::Type *targetType)
{
    if (targetType == nullptr) {
        return;
    }

    auto ttctx = TargetTypeContext(this, targetType);

    ApplyConversionCast(node, targetType);
}

void ETSGen::ApplyCast(const ir::AstNode *node, const checker::Type *targetType)
{
    auto typeKind = checker::ETSChecker::TypeKind(targetType);

    switch (typeKind) {
        case checker::TypeFlag::DOUBLE: {
            CastToDouble(node);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            CastToFloat(node);
            break;
        }
        case checker::TypeFlag::LONG: {
            CastToLong(node);
            break;
        }
        case checker::TypeFlag::INT: {
            CastToInt(node);
            break;
        }
        case checker::TypeFlag::SHORT: {
            CastToShort(node);
            break;
        }
        case checker::TypeFlag::BYTE: {
            CastToByte(node);
            break;
        }
        case checker::TypeFlag::CHAR: {
            CastToChar(node);
            break;
        }
        default: {
            break;
        }
    }
}

void ETSGen::SwapBinaryOpArgs(const ir::AstNode *const node, const VReg lhs)
{
    const RegScope rs(this);
    const auto tmp = AllocReg();

    StoreAccumulator(node, tmp);
    LoadAccumulator(node, lhs);
    MoveVreg(node, lhs, tmp);
}

VReg ETSGen::MoveAccToReg(const ir::AstNode *const node)
{
    const auto newReg = AllocReg();
    StoreAccumulator(node, newReg);
    return newReg;
}

void ETSGen::CastToBoolean([[maybe_unused]] const ir::AstNode *node)
{
    auto typeKind = checker::ETSChecker::TypeKind(GetAccumulatorType());
    switch (typeKind) {
        case checker::TypeFlag::ETS_BOOLEAN: {
            return;
        }
        case checker::TypeFlag::CHAR: {
            Sa().Emit<U32tou1>(node);
            break;
        }
        case checker::TypeFlag::BYTE:
        case checker::TypeFlag::SHORT:
        case checker::TypeFlag::INT: {
            Sa().Emit<I32tou1>(node);
            return;
        }
        case checker::TypeFlag::LONG: {
            Sa().Emit<I64tou1>(node);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            Sa().Emit<F32toi32>(node);
            Sa().Emit<I32tou1>(node);
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            Sa().Emit<F64toi32>(node);
            Sa().Emit<I32tou1>(node);
            break;
        }
        case checker::TypeFlag::ETS_OBJECT:
        case checker::TypeFlag::ETS_NEVER: {
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    SetAccumulatorType(Checker()->GlobalETSBooleanType());
}

void ETSGen::CastToByte([[maybe_unused]] const ir::AstNode *node)
{
    auto typeKind = checker::ETSChecker::TypeKind(GetAccumulatorType());
    switch (typeKind) {
        case checker::TypeFlag::BYTE: {
            return;
        }
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::CHAR: {
            Sa().Emit<U32toi8>(node);
            break;
        }
        case checker::TypeFlag::SHORT:
        case checker::TypeFlag::INT: {
            Sa().Emit<I32toi8>(node);
            break;
        }
        case checker::TypeFlag::LONG: {
            Sa().Emit<I64toi32>(node);
            Sa().Emit<I32toi8>(node);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            Sa().Emit<F32toi32>(node);
            Sa().Emit<I32toi8>(node);
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            Sa().Emit<F64toi32>(node);
            Sa().Emit<I32toi8>(node);
            break;
        }
        case checker::TypeFlag::ETS_NEVER:
        case checker::TypeFlag::ETS_OBJECT: {
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    SetAccumulatorType(Checker()->GlobalByteType());
}

void ETSGen::CastToChar([[maybe_unused]] const ir::AstNode *node)
{
    auto typeKind = checker::ETSChecker::TypeKind(GetAccumulatorType());
    switch (typeKind) {
        case checker::TypeFlag::CHAR: {
            if (node->IsCharLiteral()) {
                auto type = node->AsCharLiteral()->TsType();
                if (type->TypeFlags() == (checker::TypeFlag::CONSTANT | checker::TypeFlag::BYTE)) {
                    SetAccumulatorType(type);
                }
            }
            return;
        }
        case checker::TypeFlag::ETS_BOOLEAN: {
            break;
        }
        case checker::TypeFlag::BYTE:
        case checker::TypeFlag::SHORT:
        case checker::TypeFlag::INT: {
            Sa().Emit<I32tou16>(node);
            break;
        }
        case checker::TypeFlag::LONG: {
            Sa().Emit<I64toi32>(node);
            Sa().Emit<I32tou16>(node);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            Sa().Emit<F32toi32>(node);
            Sa().Emit<I32tou16>(node);
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            Sa().Emit<F64toi32>(node);
            Sa().Emit<I32tou16>(node);
            break;
        }
        case checker::TypeFlag::ETS_NEVER:
        case checker::TypeFlag::ETS_OBJECT: {
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    SetAccumulatorType(Checker()->GlobalCharType());
}

void ETSGen::CastToShort([[maybe_unused]] const ir::AstNode *node)
{
    auto typeKind = checker::ETSChecker::TypeKind(GetAccumulatorType());
    switch (typeKind) {
        case checker::TypeFlag::SHORT: {
            return;
        }
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::CHAR: {
            Sa().Emit<U32toi16>(node);
            break;
        }
        case checker::TypeFlag::BYTE: {
            break;
        }
        case checker::TypeFlag::INT: {
            Sa().Emit<I32toi16>(node);
            break;
        }
        case checker::TypeFlag::LONG: {
            Sa().Emit<I64toi32>(node);
            Sa().Emit<I32toi16>(node);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            Sa().Emit<F32toi32>(node);
            Sa().Emit<I32toi16>(node);
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            Sa().Emit<F64toi32>(node);
            Sa().Emit<I32toi16>(node);
            break;
        }
        case checker::TypeFlag::ETS_NEVER:
        case checker::TypeFlag::ETS_OBJECT: {
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    SetAccumulatorType(Checker()->GlobalShortType());
}

void ETSGen::CastToDouble(const ir::AstNode *node)
{
    auto typeKind = checker::ETSChecker::TypeKind(GetAccumulatorType());
    switch (typeKind) {
        case checker::TypeFlag::DOUBLE: {
            return;
        }
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::CHAR: {
            Sa().Emit<U32tof64>(node);
            break;
        }
        case checker::TypeFlag::BYTE:
        case checker::TypeFlag::SHORT:
        case checker::TypeFlag::INT: {
            Sa().Emit<I32tof64>(node);
            break;
        }
        case checker::TypeFlag::LONG: {
            Sa().Emit<I64tof64>(node);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            Sa().Emit<F32tof64>(node);
            break;
        }
        case checker::TypeFlag::ETS_NEVER:
        case checker::TypeFlag::ETS_OBJECT: {
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    SetAccumulatorType(Checker()->GlobalDoubleType());
}

void ETSGen::CastToFloat(const ir::AstNode *node)
{
    auto typeKind = checker::ETSChecker::TypeKind(GetAccumulatorType());
    switch (typeKind) {
        case checker::TypeFlag::FLOAT: {
            return;
        }
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::CHAR: {
            Sa().Emit<U32tof32>(node);
            break;
        }
        case checker::TypeFlag::BYTE:
        case checker::TypeFlag::SHORT:
        case checker::TypeFlag::INT: {
            Sa().Emit<I32tof32>(node);
            break;
        }
        case checker::TypeFlag::LONG: {
            Sa().Emit<I64tof32>(node);
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            Sa().Emit<F64tof32>(node);
            break;
        }
        case checker::TypeFlag::ETS_NEVER:
        case checker::TypeFlag::ETS_OBJECT: {
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    SetAccumulatorType(Checker()->GlobalFloatType());
}

void ETSGen::CastToLong(const ir::AstNode *node)
{
    auto typeKind = checker::ETSChecker::TypeKind(GetAccumulatorType());
    switch (typeKind) {
        case checker::TypeFlag::LONG: {
            return;
        }
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::CHAR: {
            Sa().Emit<U32toi64>(node);
            break;
        }
        case checker::TypeFlag::BYTE:
        case checker::TypeFlag::SHORT:
        case checker::TypeFlag::INT: {
            Sa().Emit<I32toi64>(node);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            Sa().Emit<F32toi64>(node);
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            Sa().Emit<F64toi64>(node);
            break;
        }
        case checker::TypeFlag::ETS_NEVER:
        case checker::TypeFlag::ETS_ARRAY:
        case checker::TypeFlag::ETS_OBJECT: {
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    SetAccumulatorType(Checker()->GlobalLongType());
}

void ETSGen::CastToInt(const ir::AstNode *node)
{
    auto typeKind = checker::ETSChecker::TypeKind(GetAccumulatorType());
    switch (typeKind) {
        case checker::TypeFlag::INT: {
            return;
        }
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::CHAR:
        case checker::TypeFlag::BYTE:
        case checker::TypeFlag::SHORT: {
            break;
        }
        case checker::TypeFlag::LONG: {
            Sa().Emit<I64toi32>(node);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            Sa().Emit<F32toi32>(node);
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            Sa().Emit<F64toi32>(node);
            break;
        }
        case checker::TypeFlag::ETS_NEVER:
        case checker::TypeFlag::ETS_OBJECT: {
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    SetAccumulatorType(Checker()->GlobalIntType());
}

void ETSGen::CastToReftype(const ir::AstNode *const node, const checker::Type *const targetType, const bool unchecked)
{
    ES2PANDA_ASSERT(GetAccumulatorType() != nullptr);
    ES2PANDA_ASSERT(GetAccumulatorType()->IsETSReferenceType());

    if (!unchecked) {
        CheckedReferenceNarrowing(node, targetType);
        return;
    }

    ES2PANDA_ASSERT(!targetType->IsETSTypeParameter() && !targetType->IsETSNonNullishType() &&
                    !targetType->IsETSPartialTypeParameter());
    CheckedReferenceNarrowing(node, targetType);
    SetAccumulatorType(targetType);
}

void ETSGen::ToBinaryResult(const ir::AstNode *node, Label *ifFalse)
{
    Label *end = AllocLabel();
    Sa().Emit<Ldai>(node, 1);
    Sa().Emit<Jmp>(node, end);
    SetLabel(node, ifFalse);
    Sa().Emit<Ldai>(node, 0);
    SetLabel(node, end);
    SetAccumulatorType(Checker()->GlobalETSBooleanType());
}

void ETSGen::BinaryLogic(const ir::AstNode *node, lexer::TokenType op, VReg lhs)
{
    switch (op) {
        case lexer::TokenType::PUNCTUATOR_MOD:
        case lexer::TokenType::PUNCTUATOR_MOD_EQUAL: {
            SwapBinaryOpArgs(node, lhs);
            BinaryArithmetic<Mod2, Mod2Wide, Fmod2, Fmod2Wide>(node, lhs);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_LEFT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_LEFT_SHIFT_EQUAL: {
            SwapBinaryOpArgs(node, lhs);
            BinaryBitwiseArithmetic<Shl2, Shl2Wide>(node, lhs);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT_EQUAL: {
            SwapBinaryOpArgs(node, lhs);
            BinaryBitwiseArithmetic<Ashr2, Ashr2Wide>(node, lhs);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT:
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT_EQUAL: {
            SwapBinaryOpArgs(node, lhs);
            BinaryBitwiseArithmetic<Shr2, Shr2Wide>(node, lhs);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_BITWISE_AND:
        case lexer::TokenType::PUNCTUATOR_BITWISE_AND_EQUAL: {
            BinaryBitwiseArithmetic<And2, And2Wide>(node, lhs);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_BITWISE_OR:
        case lexer::TokenType::PUNCTUATOR_BITWISE_OR_EQUAL: {
            BinaryBitwiseArithmetic<Or2, Or2Wide>(node, lhs);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_BITWISE_XOR:
        case lexer::TokenType::PUNCTUATOR_BITWISE_XOR_EQUAL: {
            BinaryBitwiseArithmetic<Xor2, Xor2Wide>(node, lhs);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    ES2PANDA_ASSERT(node->IsAssignmentExpression() || node->IsBinaryExpression());
    ES2PANDA_ASSERT(Checker()->Relation()->IsIdenticalTo(GetAccumulatorType(), node->AsExpression()->TsType()));
}

void ETSGen::BinaryArithmLogic(const ir::AstNode *node, lexer::TokenType op, VReg lhs)
{
    switch (op) {
        case lexer::TokenType::PUNCTUATOR_PLUS:
        case lexer::TokenType::PUNCTUATOR_PLUS_EQUAL: {
            SwapBinaryOpArgs(node, lhs);
            BinaryArithmetic<Add2, Add2Wide, Fadd2, Fadd2Wide>(node, lhs);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_MINUS:
        case lexer::TokenType::PUNCTUATOR_MINUS_EQUAL: {
            SwapBinaryOpArgs(node, lhs);
            BinaryArithmetic<Sub2, Sub2Wide, Fsub2, Fsub2Wide>(node, lhs);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_MULTIPLY:
        case lexer::TokenType::PUNCTUATOR_MULTIPLY_EQUAL: {
            SwapBinaryOpArgs(node, lhs);
            BinaryArithmetic<Mul2, Mul2Wide, Fmul2, Fmul2Wide>(node, lhs);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_DIVIDE:
        case lexer::TokenType::PUNCTUATOR_DIVIDE_EQUAL: {
            SwapBinaryOpArgs(node, lhs);
            BinaryArithmetic<Div2, Div2Wide, Fdiv2, Fdiv2Wide>(node, lhs);
            break;
        }
        default: {
            BinaryLogic(node, op, lhs);
            break;
        }
    }

    ES2PANDA_ASSERT(node->IsAssignmentExpression() || node->IsBinaryExpression());
    ES2PANDA_ASSERT(Checker()->Relation()->IsIdenticalTo(GetAccumulatorType(), node->AsExpression()->TsType()));
}

void ETSGen::Binary(const ir::AstNode *node, lexer::TokenType op, VReg lhs)
{
    Label *ifFalse = AllocLabel();
    switch (op) {
        case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL: {
            BinaryEquality<Jne, Jnez, Jeqz, true>(node, lhs, ifFalse);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_EQUAL: {
            BinaryEquality<Jne, Jnez, Jeqz>(node, lhs, ifFalse);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL: {
            BinaryEquality<Jeq, Jeqz, Jnez, true>(node, lhs, ifFalse);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_NOT_EQUAL: {
            BinaryEquality<Jeq, Jeqz, Jnez>(node, lhs, ifFalse);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_LESS_THAN: {
            BinaryRelation<Jle, Jlez>(node, lhs, ifFalse);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL: {
            BinaryRelation<Jlt, Jltz>(node, lhs, ifFalse);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN: {
            BinaryRelation<Jge, Jgez>(node, lhs, ifFalse);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL: {
            BinaryRelation<Jgt, Jgtz>(node, lhs, ifFalse);
            break;
        }
        default: {
            BinaryArithmLogic(node, op, lhs);
            break;
        }
    }

    ES2PANDA_ASSERT(node->IsAssignmentExpression() || node->IsBinaryExpression());
    ES2PANDA_ASSERT(Checker()->Relation()->IsIdenticalTo(GetAccumulatorType(), node->AsExpression()->TsType()));
}

void ETSGen::Condition(const ir::AstNode *node, lexer::TokenType op, VReg lhs, Label *ifFalse)
{
    switch (op) {
        case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL: {
            BinaryEqualityCondition<Jne, Jnez, true>(node, lhs, ifFalse);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_EQUAL: {
            BinaryEqualityCondition<Jne, Jnez>(node, lhs, ifFalse);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL: {
            BinaryEqualityCondition<Jeq, Jeqz, true>(node, lhs, ifFalse);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_NOT_EQUAL: {
            BinaryEqualityCondition<Jeq, Jeqz>(node, lhs, ifFalse);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_LESS_THAN: {
            BinaryRelationCondition<Jle, Jlez>(node, lhs, ifFalse);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_LESS_THAN_EQUAL: {
            BinaryRelationCondition<Jlt, Jltz>(node, lhs, ifFalse);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN: {
            BinaryRelationCondition<Jge, Jgez>(node, lhs, ifFalse);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_GREATER_THAN_EQUAL: {
            BinaryRelationCondition<Jgt, Jgtz>(node, lhs, ifFalse);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

template <typename CondCompare, bool BEFORE_LOGICAL_NOT>
void ETSGen::ResolveConditionalResultFloat(const ir::AstNode *node, Label *realEndLabel)
{
    auto type = GetAccumulatorType();
    ES2PANDA_ASSERT(type != nullptr);
    VReg tmpReg = AllocReg();
    StoreAccumulator(node, tmpReg);
    if (type->IsFloatType()) {
        FloatIsNaN(node);
    } else {
        DoubleIsNaN(node);
    }
    Sa().Emit<Xori>(node, 1);

    BranchIfFalse(node, realEndLabel);
    LoadAccumulator(node, tmpReg);
    VReg zeroReg = AllocReg();

    if (type->IsFloatType()) {
        MoveImmediateToRegister(node, zeroReg, checker::TypeFlag::FLOAT, 0);
        BinaryNumberComparison<Fcmpl, Jeqz>(node, zeroReg, realEndLabel);
    } else {
        MoveImmediateToRegister(node, zeroReg, checker::TypeFlag::DOUBLE, 0);
        BinaryNumberComparison<FcmplWide, Jeqz>(node, zeroReg, realEndLabel);
    }
}

template <typename CondCompare, bool BEFORE_LOGICAL_NOT, bool USE_FALSE_LABEL>
void ETSGen::ResolveConditionalResultNumeric(const ir::AstNode *node, [[maybe_unused]] Label *ifFalse, Label **end)
{
    auto type = GetAccumulatorType();
    ES2PANDA_ASSERT(type != nullptr);
    auto realEndLabel = [end, ifFalse, this](bool useFalseLabel) {
        if (useFalseLabel) {
            return ifFalse;
        }
        if ((*end) == nullptr) {
            (*end) = AllocLabel();
        }
        return (*end);
    }(USE_FALSE_LABEL);
    if (type->IsDoubleType() || type->IsFloatType()) {
        ResolveConditionalResultFloat<CondCompare, BEFORE_LOGICAL_NOT>(node, realEndLabel);
    }
    if (type->IsLongType()) {
        VReg zeroReg = AllocReg();
        MoveImmediateToRegister(node, zeroReg, checker::TypeFlag::LONG, 0);
        BinaryNumberComparison<CmpWide, CondCompare>(node, zeroReg, realEndLabel);
    }
    if constexpr (BEFORE_LOGICAL_NOT) {
        Label *zeroPrimitive = AllocLabel();
        BranchIfFalse(node, zeroPrimitive);
        ToBinaryResult(node, zeroPrimitive);
    }
}

template <typename CondCompare, bool BEFORE_LOGICAL_NOT>
void ETSGen::ResolveConditionalResultReference(const ir::AstNode *node)
{
    auto const testString = [this, node]() {
        LoadStringLength(node);
        if constexpr (BEFORE_LOGICAL_NOT) {
            Label *zeroLenth = AllocLabel();
            BranchIfFalse(node, zeroLenth);
            ToBinaryResult(node, zeroLenth);
        }
    };

    auto type = GetAccumulatorType();
    ES2PANDA_ASSERT(type != nullptr);
    if (!type->PossiblyETSString()) {
        Sa().Emit<Ldai>(node, 1);
        return;
    }
    if (type->IsETSStringType()) {  // should also be valid for string|null|undefined
        testString();
        return;
    }

    Label *isString = AllocLabel();
    Label *end = AllocLabel();
    compiler::VReg objReg = AllocReg();
    StoreAccumulator(node, objReg);

    ES2PANDA_ASSERT(Checker()->GlobalBuiltinETSStringType() != nullptr);
    EmitIsInstance(node, ToAssemblerType(Checker()->GlobalBuiltinETSStringType()));
    BranchIfTrue(node, isString);
    Sa().Emit<Ldai>(node, 1);
    Branch(node, end);
    SetLabel(node, isString);
    LoadAccumulator(node, objReg);
    EmitCheckCast(node, Signatures::BUILTIN_STRING);  // help verifier
    testString();
    SetLabel(node, end);
}

template <typename CondCompare, bool BEFORE_LOGICAL_NOT, bool USE_FALSE_LABEL>
void ETSGen::ResolveConditionalResult(const ir::AstNode *node, [[maybe_unused]] Label *ifFalse)
{
    auto type = GetAccumulatorType();
    ES2PANDA_ASSERT(type != nullptr);
#ifdef PANDA_WITH_ETS
    if (type->IsETSReferenceType()) {
        VReg valReg = AllocReg();
        StoreAccumulator(node, valReg);
        EmitEtsIstrue(node, valReg);
        return;
    }
#endif  // PANDA_WITH_ETS
    if (type->IsETSBooleanType()) {
        return;
    }
    Label *ifNullish {nullptr};
    Label *end {nullptr};
    if (type->PossiblyETSNullish()) {
        if constexpr (USE_FALSE_LABEL) {
            BranchIfNullish(node, ifFalse);
        } else {
            ifNullish = AllocLabel();
            end = AllocLabel();
            BranchIfNullish(node, ifNullish);
        }
    }
    if (type->DefinitelyETSNullish()) {
        // skip
    } else if (type->IsETSReferenceType()) {
        ResolveConditionalResultReference<CondCompare, BEFORE_LOGICAL_NOT>(node);
    } else {
        ResolveConditionalResultNumeric<CondCompare, BEFORE_LOGICAL_NOT, USE_FALSE_LABEL>(node, ifFalse, &end);
    }
    if (ifNullish != nullptr) {
        Branch(node, end);
        SetLabel(node, ifNullish);
        Sa().Emit<Ldai>(node, 0);
    }
    if (end != nullptr) {
        SetLabel(node, end);
    }
}

template <bool BEFORE_LOGICAL_NOT, bool FALSE_LABEL_EXISTED>
void ETSGen::ResolveConditionalResultIfFalse(const ir::AstNode *node, Label *ifFalse)
{
    ResolveConditionalResult<Jeqz, BEFORE_LOGICAL_NOT, FALSE_LABEL_EXISTED>(node, ifFalse);
}

template void ETSGen::ResolveConditionalResultIfFalse<false, true>(const ir::AstNode *node, Label *ifFalse);
template void ETSGen::ResolveConditionalResultIfFalse<true, false>(const ir::AstNode *node, Label *ifFalse);
template void ETSGen::ResolveConditionalResultIfFalse<false, false>(const ir::AstNode *node, Label *ifFalse);

template <bool BEFORE_LOGICAL_NOT, bool FALSE_LABEL_EXISTED>
void ETSGen::ResolveConditionalResultIfTrue(const ir::AstNode *node, Label *ifFalse)
{
    ResolveConditionalResult<Jnez, BEFORE_LOGICAL_NOT, FALSE_LABEL_EXISTED>(node, ifFalse);
}

template void ETSGen::ResolveConditionalResultIfTrue<false, true>(const ir::AstNode *node, Label *ifFalse);
template void ETSGen::ResolveConditionalResultIfTrue<false, false>(const ir::AstNode *node, Label *ifFalse);

template <typename CondCompare, typename NegCondCompare>
void ETSGen::BranchConditional(const ir::AstNode *node, Label *endLabel)
{
    auto type = GetAccumulatorType();
    ES2PANDA_ASSERT(type != nullptr);
    if (type->IsETSReferenceType()) {
        VReg valReg = AllocReg();
        StoreAccumulator(node, valReg);
        EmitEtsIstrue(node, valReg);
    } else if (type->IsDoubleType() || type->IsFloatType()) {
        ConditionalFloat(node);
    } else if (type->IsLongType()) {
        VReg zeroReg = AllocReg();
        MoveImmediateToRegister(node, zeroReg, checker::TypeFlag::LONG, 0);
        Ra().Emit<CmpWide>(node, zeroReg);
    }

    Sa().Emit<CondCompare>(node, endLabel);
}

void ETSGen::ConditionalFloat(const ir::AstNode *node)
{
    auto type = GetAccumulatorType();
    ES2PANDA_ASSERT(type != nullptr);
    VReg tmpReg = AllocReg();
    VReg isNaNReg = AllocReg();

    StoreAccumulator(node, tmpReg);
    if (type->IsFloatType()) {
        FloatIsNaN(node);
    } else {
        DoubleIsNaN(node);
    }
    Sa().Emit<Xori>(node, 1);
    StoreAccumulator(node, isNaNReg);
    LoadAccumulator(node, tmpReg);

    VReg zeroReg = AllocReg();

    if (type->IsFloatType()) {
        MoveImmediateToRegister(node, zeroReg, checker::TypeFlag::FLOAT, 0);
        Ra().Emit<Fcmpl>(node, zeroReg);
    } else {
        MoveImmediateToRegister(node, zeroReg, checker::TypeFlag::DOUBLE, 0);
        Ra().Emit<FcmplWide>(node, zeroReg);
    }
    Sa().Emit<Xori>(node, 0);
    Ra().Emit<And2>(node, isNaNReg);
}

void ETSGen::BranchConditionalIfFalse(const ir::AstNode *node, Label *endLabel)
{
    BranchConditional<Jeqz, Jnez>(node, endLabel);
}

void ETSGen::BranchConditionalIfTrue(const ir::AstNode *node, Label *endLabel)
{
    BranchConditional<Jnez, Jeqz>(node, endLabel);
}

void ETSGen::BranchIfNullish(const ir::AstNode *node, Label *ifNullish)
{
    auto *const type = GetAccumulatorType();
    ES2PANDA_ASSERT(type != nullptr);

    if (type->IsETSVoidType()) {
        // NOTE(): #19701 need void refactoring
        Sa().Emit<Jmp>(node, ifNullish);
    } else if (type->DefinitelyNotETSNullish()) {
        // no action
    } else if (type->DefinitelyETSNullish()) {
        Sa().Emit<Jmp>(node, ifNullish);
    } else if (!type->PossiblyETSNull()) {
        Sa().Emit<JeqzObj>(node, ifNullish);
    } else {
        RegScope rs(this);
        auto tmpObj = AllocReg();
        auto notTaken = AllocLabel();

        if (type->PossiblyETSUndefined()) {
            Sa().Emit<JeqzObj>(node, ifNullish);
        }

        StoreAccumulator(node, tmpObj);
        EmitIsNull(node);
        Sa().Emit<Jeqz>(node, notTaken);

        LoadAccumulator(node, tmpObj);
        Sa().Emit<Jmp>(node, ifNullish);

        SetLabel(node, notTaken);
        LoadAccumulator(node, tmpObj);
    }
}

void ETSGen::BranchIfNotNullish(const ir::AstNode *node, Label *ifNotNullish)
{
    auto notTaken = AllocLabel();
    BranchIfNullish(node, notTaken);
    JumpTo(node, ifNotNullish);
    SetLabel(node, notTaken);
}

void ETSGen::AssumeNonNullish(const ir::AstNode *node, checker::Type const *targetType)
{
    auto const *nullishType = GetAccumulatorType();
    ES2PANDA_ASSERT(nullishType != nullptr);
    if (nullishType->PossiblyETSNull()) {
        // clear 'null' dataflow
        EmitCheckCast(node, ToAssemblerType(targetType));
    }
    SetAccumulatorType(targetType);
}

void ETSGen::EmitNullishException(const ir::AstNode *node)
{
    RegScope ra(this);
    VReg exception = AllocReg();
    VReg messageReg = AllocReg();
    VReg optionsReg = AllocReg();

    LoadAccumulatorUndefined(node);
    StoreAccumulator(node, messageReg);
    LoadAccumulatorUndefined(node);
    StoreAccumulator(node, optionsReg);

    Ra().Emit<InitobjShort>(node, AssemblerSignatureReference(Signatures::BUILTIN_NULLPOINTER_ERROR_CTOR), messageReg,
                            optionsReg);
    SetAccumulatorType(Checker()->GlobalETSObjectType());
    StoreAccumulator(node, exception);

    EmitThrow(node, exception);
    SetAccumulatorType(nullptr);
}

template <typename IntCompare, typename CondCompare, typename DynCompare, bool IS_STRICT>
void ETSGen::BinaryEquality(const ir::AstNode *node, VReg lhs, Label *ifFalse)
{
    BinaryEqualityCondition<IntCompare, CondCompare, IS_STRICT>(node, lhs, ifFalse);
    ToBinaryResult(node, ifFalse);
    SetAccumulatorType(Checker()->GlobalETSBooleanType());
}

template <typename IntCompare, typename CondCompare, bool IS_STRICT>
void ETSGen::BinaryEqualityCondition(const ir::AstNode *node, VReg lhs, Label *ifFalse)
{
    if (targetType_->IsETSReferenceType()) {
        RegScope rs(this);
        VReg arg0 = AllocReg();
        StoreAccumulator(node, arg0);
        InverseCondition(
            node, [this, node, lhs, arg0](Label *tgt) { RefEqualityLoose<IS_STRICT>(node, lhs, arg0, tgt); }, ifFalse,
            std::is_same_v<CondCompare, Jeqz>);
        SetAccumulatorType(Checker()->GlobalETSBooleanType());
        return;
    }

    auto typeKind = checker::ETSChecker::TypeKind(targetType_);

    switch (typeKind) {
        case checker::TypeFlag::DOUBLE: {
            BinaryFloatingPointComparison<FcmpgWide, FcmplWide, CondCompare>(node, lhs, ifFalse);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            BinaryFloatingPointComparison<Fcmpg, Fcmpl, CondCompare>(node, lhs, ifFalse);
            break;
        }
        case checker::TypeFlag::LONG: {
            BinaryNumberComparison<CmpWide, CondCompare>(node, lhs, ifFalse);
            break;
        }
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::BYTE:
        case checker::TypeFlag::CHAR:
        case checker::TypeFlag::SHORT:
        case checker::TypeFlag::INT: {
            Ra().Emit<IntCompare>(node, lhs, ifFalse);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    SetAccumulatorType(Checker()->GlobalETSBooleanType());
}

template <typename IntCompare, typename CondCompare>
void ETSGen::BinaryRelation(const ir::AstNode *node, VReg lhs, Label *ifFalse)
{
    BinaryRelationCondition<IntCompare, CondCompare>(node, lhs, ifFalse);
    ToBinaryResult(node, ifFalse);
}

template <typename IntCompare, typename CondCompare>
void ETSGen::BinaryRelationCondition(const ir::AstNode *node, VReg lhs, Label *ifFalse)
{
    auto typeKind = checker::ETSChecker::TypeKind(targetType_);

    switch (typeKind) {
        case checker::TypeFlag::DOUBLE: {
            BinaryFloatingPointComparison<FcmpgWide, FcmplWide, CondCompare>(node, lhs, ifFalse);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            BinaryFloatingPointComparison<Fcmpg, Fcmpl, CondCompare>(node, lhs, ifFalse);
            break;
        }
        case checker::TypeFlag::LONG: {
            BinaryNumberComparison<CmpWide, CondCompare>(node, lhs, ifFalse);
            break;
        }
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::BYTE:
        case checker::TypeFlag::SHORT:
        case checker::TypeFlag::CHAR:
        case checker::TypeFlag::INT: {
            Ra().Emit<IntCompare>(node, lhs, ifFalse);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    SetAccumulatorType(Checker()->GlobalETSBooleanType());
}

template <typename IntOp, typename LongOp, typename FloatOp, typename DoubleOp>
void ETSGen::BinaryArithmetic(const ir::AstNode *node, VReg lhs)
{
    auto typeKind = checker::ETSChecker::TypeKind(targetType_);

    switch (typeKind) {
        case checker::TypeFlag::DOUBLE: {
            Ra().Emit<DoubleOp>(node, lhs);
            SetAccumulatorType(Checker()->GlobalDoubleType());
            break;
        }
        case checker::TypeFlag::FLOAT: {
            Ra().Emit<FloatOp>(node, lhs);
            SetAccumulatorType(Checker()->GlobalFloatType());
            break;
        }
        default: {
            BinaryBitwiseArithmetic<IntOp, LongOp>(node, lhs);
            break;
        }
    }

    ApplyCast(node, node->AsExpression()->TsType());
}

template <typename IntOp, typename LongOp>
void ETSGen::BinaryBitwiseArithmetic(const ir::AstNode *node, VReg lhs)
{
    auto typeKind = checker::ETSChecker::TypeKind(targetType_);

    switch (typeKind) {
        case checker::TypeFlag::LONG: {
            Ra().Emit<LongOp>(node, lhs);
            SetAccumulatorType(Checker()->GlobalLongType());
            break;
        }
        case checker::TypeFlag::BYTE:
        case checker::TypeFlag::SHORT:
        case checker::TypeFlag::CHAR:
        case checker::TypeFlag::INT: {
            Ra().Emit<IntOp>(node, lhs);
            SetAccumulatorType(Checker()->GlobalIntType());
            break;
        }
        case checker::TypeFlag::ETS_BOOLEAN: {
            Ra().Emit<IntOp>(node, lhs);
            SetAccumulatorType(Checker()->GlobalETSBooleanType());
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    ApplyCast(node, node->AsExpression()->TsType());
}

template <bool IS_STRICT>
void ETSGen::HandleDefinitelyNullishEquality(const ir::AstNode *node, VReg lhs, VReg rhs, Label *ifFalse)
{
    if constexpr (IS_STRICT) {
        LoadAccumulator(node, lhs);
        Ra().Emit<JneObj>(node, rhs, ifFalse);
    } else {
        auto *checker = const_cast<checker::ETSChecker *>(Checker());
        auto ltype = checker->GetNonConstantType(const_cast<checker::Type *>(GetVRegType(lhs)));
        ES2PANDA_ASSERT(ltype != nullptr);
        LoadAccumulator(node, ltype->DefinitelyETSNullish() ? rhs : lhs);
        BranchIfNotNullish(node, ifFalse);
    }
}

template <bool IS_STRICT>
void ETSGen::HandlePossiblyNullishEquality(const ir::AstNode *node, VReg lhs, VReg rhs, Label *ifFalse, Label *ifTrue)
{
    Label *ifLhsNullish = AllocLabel();
    Label *out = AllocLabel();

    LoadAccumulator(node, lhs);
    BranchIfNullish(node, ifLhsNullish);

    LoadAccumulator(node, rhs);
    BranchIfNullish(node, ifFalse);
    JumpTo(node, out);

    SetLabel(node, ifLhsNullish);
    if constexpr (IS_STRICT) {
        Ra().Emit<JneObj>(node, rhs, ifFalse);
    } else {
        LoadAccumulator(node, rhs);
        BranchIfNotNullish(node, ifFalse);
    }
    JumpTo(node, ifTrue);

    SetLabel(node, out);
    SetAccumulatorType(nullptr);
}

static std::optional<std::tuple<checker::Type const *, util::StringView, bool>> SelectLooseObjComparator(
    ETSGen *etsg, checker::Type *lhs, checker::Type *rhs)
{
    auto checker = const_cast<checker::ETSChecker *>(etsg->Checker());
    auto alhs = checker->GetApparentType(checker->GetNonNullishType(lhs));
    auto arhs = checker->GetApparentType(checker->GetNonNullishType(rhs));
    ES2PANDA_ASSERT(alhs != nullptr && arhs != nullptr);
    alhs = alhs->IsETSStringType() ? checker->GlobalBuiltinETSStringType() : alhs;
    arhs = arhs->IsETSStringType() ? checker->GlobalBuiltinETSStringType() : arhs;
    ES2PANDA_ASSERT(alhs != nullptr && arhs != nullptr);
    if (!alhs->IsETSObjectType() || !arhs->IsETSObjectType()) {
        return std::nullopt;
    }
    if (!checker->Relation()->IsIdenticalTo(alhs, arhs)) {
        return std::nullopt;
    }
    auto obj = alhs->AsETSObjectType();
    if (!obj->HasObjectFlag(checker::ETSObjectFlags::VALUE_TYPED)) {
        return std::nullopt;
    }
    // NOTE(vpukhov): emit faster code
    bool isFinal = obj->GetDeclNode()->IsFinal();
    auto methodSig =
        MakeView(etsg, etsg->Emitter()->AddDependence(obj->AssemblerName().Mutf8()) + ".equals:std.core.Object;u1;");
    return std::make_tuple(checker->GetNonConstantType(obj), methodSig, isFinal);
}

template <typename LongOp, typename IntOp, typename DoubleOp, typename FloatOp>
void ETSGen::UpdateOperator(const ir::AstNode *node)
{
    switch (checker::ETSChecker::ETSType(GetAccumulatorType())) {
        case checker::TypeFlag::LONG: {
            RegScope scope(this);
            VReg reg = AllocReg();
            Ra().Emit<MoviWide>(node, reg, 1LL);
            Ra().Emit<LongOp>(node, reg);
            break;
        }
        case checker::TypeFlag::INT: {
            Sa().Emit<IntOp>(node, 1);
            break;
        }
        case checker::TypeFlag::CHAR: {
            Sa().Emit<IntOp>(node, 1);
            Sa().Emit<I32tou16>(node);
            break;
        }
        case checker::TypeFlag::SHORT: {
            Sa().Emit<IntOp>(node, 1);
            Sa().Emit<I32toi16>(node);
            break;
        }
        case checker::TypeFlag::BYTE: {
            Sa().Emit<IntOp>(node, 1);
            Sa().Emit<I32toi8>(node);
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            RegScope scope(this);
            VReg reg = AllocReg();
            Ra().Emit<FmoviWide>(node, reg, 1.0);
            Ra().Emit<DoubleOp>(node, reg);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            RegScope scope(this);
            VReg reg = AllocReg();
            Ra().Emit<Fmovi>(node, reg, 1.0F);
            Ra().Emit<FloatOp>(node, reg);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

template <bool IS_STRICT>
void ETSGen::RefEqualityLoose(const ir::AstNode *node, VReg lhs, VReg rhs, Label *ifFalse)
{
    auto *checker = const_cast<checker::ETSChecker *>(Checker());
    auto ltype = checker->GetNonConstantType(const_cast<checker::Type *>(GetVRegType(lhs)));
    auto rtype = checker->GetNonConstantType(const_cast<checker::Type *>(GetVRegType(rhs)));
    ES2PANDA_ASSERT(ltype != nullptr && rtype != nullptr);
    if (ltype->DefinitelyETSNullish() || rtype->DefinitelyETSNullish()) {
        HandleDefinitelyNullishEquality<IS_STRICT>(node, lhs, rhs, ifFalse);
    } else if (auto spec = SelectLooseObjComparator(  // try to select specific type
                                                      // CC-OFFNXT(G.FMT.06-CPP) project code style
                   this, const_cast<checker::Type *>(ltype),
                   const_cast<checker::Type *>(rtype));  // CC-OFF(G.FMT.02) project code style
               spec.has_value()) {                       // CC-OFF(G.FMT.02-CPP) project code style
        auto &[assumeType, methodSig, isDevirtual] = *spec;
        auto ifTrue = AllocLabel();
        if (ltype->PossiblyETSNullish() || rtype->PossiblyETSNullish()) {
            HandlePossiblyNullishEquality<IS_STRICT>(node, lhs, rhs, ifFalse, ifTrue);
        }
        LoadAccumulator(node, rhs);
        AssumeNonNullish(node, assumeType);
        StoreAccumulator(node, rhs);
        LoadAccumulator(node, lhs);
        AssumeNonNullish(node, assumeType);
        if (!isDevirtual) {
            CallExact(node, methodSig, lhs, rhs);
        } else {
            CallExactDevirtual(node, methodSig, lhs, rhs);
        }
        BranchIfFalse(node, ifFalse);
        SetLabel(node, ifTrue);
    } else {
        EmitEtsEquals<IS_STRICT>(node, lhs, rhs);
        BranchIfFalse(node, ifFalse);
    }
    SetAccumulatorType(nullptr);
}

void ETSGen::CompileStatements(const ArenaVector<ir::Statement *> &statements)
{
    for (const auto *stmt : statements) {
        stmt->Compile(this);
    }
}

void ETSGen::Negate(const ir::AstNode *node)
{
    auto typeKind = checker::ETSChecker::TypeKind(GetAccumulatorType());

    switch (typeKind) {
        case checker::TypeFlag::BYTE:
        case checker::TypeFlag::SHORT:
        case checker::TypeFlag::CHAR:
        case checker::TypeFlag::INT: {
            Sa().Emit<Neg>(node);
            return;
        }
        case checker::TypeFlag::LONG: {
            Sa().Emit<NegWide>(node);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            Sa().Emit<Fneg>(node);
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            Sa().Emit<FnegWide>(node);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

void ETSGen::LogicalNot(const ir::AstNode *node)
{
    ResolveConditionalResultIfFalse<true, false>(node);
    Sa().Emit<Xori>(node, 1);
    SetAccumulatorType(Checker()->GlobalETSBooleanType());
}

void ETSGen::LoadAccumulatorByte(const ir::AstNode *node, int8_t number)
{
    LoadAccumulatorNumber<int8_t>(node, number, checker::TypeFlag::BYTE);
}

void ETSGen::LoadAccumulatorShort(const ir::AstNode *node, int16_t number)
{
    LoadAccumulatorNumber<int16_t>(node, number, checker::TypeFlag::SHORT);
}

void ETSGen::LoadAccumulatorInt(const ir::AstNode *node, int32_t number)
{
    LoadAccumulatorNumber<int32_t>(node, number, checker::TypeFlag::INT);
}

void ETSGen::LoadAccumulatorWideInt(const ir::AstNode *node, int64_t number)
{
    LoadAccumulatorNumber<int64_t>(node, number, checker::TypeFlag::LONG);
}

void ETSGen::LoadAccumulatorFloat(const ir::AstNode *node, float number)
{
    LoadAccumulatorNumber<float>(node, number, checker::TypeFlag::FLOAT);
}

void ETSGen::LoadAccumulatorDouble(const ir::AstNode *node, double number)
{
    LoadAccumulatorNumber<double>(node, number, checker::TypeFlag::DOUBLE);
}

void ETSGen::LoadAccumulatorBoolean(const ir::AstNode *node, bool value)
{
    Sa().Emit<Ldai>(node, value ? 1 : 0);
    SetAccumulatorType(Checker()->GlobalETSBooleanType());
    ApplyConversion(node, Checker()->GlobalETSBooleanType());
}

void ETSGen::LoadAccumulatorString(const ir::AstNode *node, util::StringView str)
{
    Sa().Emit<LdaStr>(node, str);
    SetAccumulatorType(Checker()->GlobalETSStringLiteralType());
}

void ETSGen::LoadAccumulatorBigInt(const ir::AstNode *node, util::StringView str)
{
    Sa().Emit<LdaStr>(node, str);
    SetAccumulatorType(Checker()->GlobalETSBigIntType());
}

void ETSGen::LoadAccumulatorUndefined(const ir::AstNode *node)
{
    Sa().Emit<LdaNull>(node);
    SetAccumulatorType(Checker()->GlobalETSUndefinedType());
}

void ETSGen::LoadAccumulatorNull([[maybe_unused]] const ir::AstNode *node)
{
#ifdef PANDA_WITH_ETS
    Sa().Emit<EtsLdnullvalue>(node);
    SetAccumulatorType(Checker()->GlobalETSNullType());
#else
    ES2PANDA_UNREACHABLE();
#endif  // PANDA_WITH_ETS
}

void ETSGen::LoadAccumulatorChar(const ir::AstNode *node, char16_t value)
{
    Sa().Emit<Ldai>(node, value);
    SetAccumulatorType(Checker()->GlobalCharType());
    ApplyConversion(node, Checker()->GlobalCharType());
}

void ETSGen::Unary(const ir::AstNode *node, lexer::TokenType op)
{
    switch (op) {
        case lexer::TokenType::PUNCTUATOR_PLUS:
            break;  // NOP -> Unary numeric promotion is performed
        case lexer::TokenType::PUNCTUATOR_MINUS:
            UnaryMinus(node);
            break;
        case lexer::TokenType::PUNCTUATOR_TILDE:
            UnaryTilde(node);
            break;
        case lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK:
            LogicalNot(node);
            break;
        default:
            ES2PANDA_UNREACHABLE();
    }
}

void ETSGen::UnaryMinus(const ir::AstNode *node)
{
    ES2PANDA_ASSERT(GetAccumulatorType() != nullptr);
    if (GetAccumulatorType()->IsETSBigIntType()) {
        const VReg value = AllocReg();
        StoreAccumulator(node, value);
        CallExact(node, Signatures::BUILTIN_BIGINT_NEGATE, value);
        return;
    }

    switch (checker::ETSChecker::ETSType(GetAccumulatorType())) {
        case checker::TypeFlag::LONG: {
            Sa().Emit<NegWide>(node);
            break;
        }
        case checker::TypeFlag::INT:
        case checker::TypeFlag::SHORT:
        case checker::TypeFlag::CHAR:
        case checker::TypeFlag::BYTE: {
            Sa().Emit<Neg>(node);
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            Sa().Emit<FnegWide>(node);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            Sa().Emit<Fneg>(node);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

void ETSGen::UnaryTilde(const ir::AstNode *node)
{
    ES2PANDA_ASSERT(GetAccumulatorType() != nullptr);
    if (GetAccumulatorType()->IsETSBigIntType()) {
        const VReg value = AllocReg();
        StoreAccumulator(node, value);
        CallExact(node, Signatures::BUILTIN_BIGINT_OPERATOR_BITWISE_NOT, value);
        SetAccumulatorType(Checker()->GlobalETSBigIntType());
        return;
    }

    switch (checker::ETSChecker::ETSType(GetAccumulatorType())) {
        case checker::TypeFlag::LONG: {
            Sa().Emit<NotWide>(node);
            break;
        }
        case checker::TypeFlag::INT:
        case checker::TypeFlag::SHORT:
        case checker::TypeFlag::CHAR:
        case checker::TypeFlag::BYTE: {
            Sa().Emit<Not>(node);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

void ETSGen::Update(const ir::AstNode *node, lexer::TokenType op)
{
    switch (op) {
        case lexer::TokenType::PUNCTUATOR_PLUS_PLUS: {
            UpdateOperator<Add2Wide, Addi, Fadd2Wide, Fadd2>(node);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_MINUS_MINUS: {
            UpdateOperator<Sub2Wide, Subi, Fsub2Wide, Fsub2>(node);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

void ETSGen::UpdateBigInt(const ir::Expression *node, VReg arg, lexer::TokenType op)
{
    switch (op) {
        case lexer::TokenType::PUNCTUATOR_PLUS_PLUS: {
            CallBigIntUnaryOperator(node, arg, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_INCREMENT);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_MINUS_MINUS: {
            CallBigIntUnaryOperator(node, arg, compiler::Signatures::BUILTIN_BIGINT_OPERATOR_DECREMENT);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

void ETSGen::ConcatStrings(const ir::BinaryExpression *node, VReg lhs)
{
    ES2PANDA_ASSERT(node->OperationType()->IsETSStringType());
    node->CompileOperands(this, lhs);
    RegScope rs(this);
    auto rhs = AllocReg();
    StoreAccumulator(node->Right(), rhs);

    ToString(node->Left(), lhs);
    StoreAccumulator(node->Left(), lhs);

    ToString(node->Right(), rhs);
    StoreAccumulator(node->Right(), rhs);

    ES2PANDA_ASSERT(GetVRegType(lhs)->IsETSStringType());
    ES2PANDA_ASSERT(GetVRegType(rhs)->IsETSStringType());
    CallExact(node, Signatures::BUILTIN_STRING_BUILDER_CONCAT_STRING, lhs, rhs);
    SetAccumulatorType(node->TsType());
}

void ETSGen::ToString(const ir::Expression *node, VReg arg)
{
    const auto regType = GetVRegType(arg);
    if (regType->IsETSReferenceType()) {
        if (regType->PossiblyETSUndefined()) {
            const auto ifUndefined = AllocLabel();
            const auto end = AllocLabel();
            LoadAccumulator(node, arg);
            BranchIfUndefined(node, ifUndefined);
            CallVirtual(node, Signatures::BUILTIN_OBJECT_TO_STRING, arg);
            SetAccumulatorType(Checker()->GlobalBuiltinETSStringType());
            JumpTo(node, end);

            SetLabel(node, ifUndefined);
            LoadAccumulatorString(node, "undefined");

            SetLabel(node, end);
            return;
        }
        if (regType->IsETSStringType()) {
            LoadAccumulator(node, arg);
        } else {
            CallVirtual(node, Signatures::BUILTIN_OBJECT_TO_STRING, arg);
            SetAccumulatorType(Checker()->GlobalBuiltinETSStringType());
        }
        return;
    }

    using TSign = std::pair<checker::TypeFlag, std::string_view>;
    constexpr std::array TO_STRING_METHODS {
        TSign {checker::TypeFlag::ETS_BOOLEAN, Signatures::BUILTIN_STRING_BUILDER_TO_STRING_BOOLEAN},
        TSign {checker::TypeFlag::CHAR, Signatures::BUILTIN_STRING_BUILDER_TO_STRING_CHAR},
        TSign {checker::TypeFlag::SHORT, Signatures::BUILTIN_STRING_BUILDER_TO_STRING_INT},
        TSign {checker::TypeFlag::BYTE, Signatures::BUILTIN_STRING_BUILDER_TO_STRING_INT},
        TSign {checker::TypeFlag::INT, Signatures::BUILTIN_STRING_BUILDER_TO_STRING_INT},
        TSign {checker::TypeFlag::LONG, Signatures::BUILTIN_STRING_BUILDER_TO_STRING_LONG},
        TSign {checker::TypeFlag::FLOAT, Signatures::BUILTIN_STRING_BUILDER_TO_STRING_FLOAT},
        TSign {checker::TypeFlag::DOUBLE, Signatures::BUILTIN_STRING_BUILDER_TO_STRING_DOUBLE},
    };

    const auto typeFlag = checker::ETSChecker::ETSType(regType);
    const auto iter = std::find_if(TO_STRING_METHODS.begin(), TO_STRING_METHODS.end(),
                                   [typeFlag](TSign p) { return p.first == typeFlag; });
    if (iter != TO_STRING_METHODS.end()) {
        CallExact(node, iter->second, arg);
        SetAccumulatorType(Checker()->GlobalBuiltinETSStringType());
        return;
    }
    ES2PANDA_UNREACHABLE();
}

void ETSGen::StringBuilderAppend(const ir::AstNode *node, VReg builder)
{
    RegScope rs(this);
    util::StringView signature {};

    node->Compile(this);

    std::unordered_map<checker::TypeFlag, std::string_view> typeFlagToSignaturesMap {
        {checker::TypeFlag::ETS_BOOLEAN, Signatures::BUILTIN_STRING_BUILDER_APPEND_BOOLEAN},
        {checker::TypeFlag::CHAR, Signatures::BUILTIN_STRING_BUILDER_APPEND_CHAR},
        {checker::TypeFlag::SHORT, Signatures::BUILTIN_STRING_BUILDER_APPEND_INT},
        {checker::TypeFlag::BYTE, Signatures::BUILTIN_STRING_BUILDER_APPEND_INT},
        {checker::TypeFlag::INT, Signatures::BUILTIN_STRING_BUILDER_APPEND_INT},
        {checker::TypeFlag::LONG, Signatures::BUILTIN_STRING_BUILDER_APPEND_LONG},
        {checker::TypeFlag::FLOAT, Signatures::BUILTIN_STRING_BUILDER_APPEND_FLOAT},
        {checker::TypeFlag::DOUBLE, Signatures::BUILTIN_STRING_BUILDER_APPEND_DOUBLE},
    };

    auto search = typeFlagToSignaturesMap.find(checker::ETSChecker::ETSType(GetAccumulatorType()));
    if (search != typeFlagToSignaturesMap.end()) {
        signature = search->second;
    } else {
        signature = Signatures::BUILTIN_STRING_BUILDER_APPEND_BUILTIN_STRING;
    }

    ES2PANDA_ASSERT(GetAccumulatorType() != nullptr);
    if (GetAccumulatorType()->IsETSReferenceType() && !GetAccumulatorType()->IsETSStringType()) {
        if (GetAccumulatorType()->PossiblyETSUndefined()) {
            Label *ifUndefined = AllocLabel();
            Label *end = AllocLabel();
            BranchIfUndefined(node, ifUndefined);
            Ra().Emit<CallVirtAccShort, 0>(node, AssemblerSignatureReference(Signatures::BUILTIN_OBJECT_TO_STRING),
                                           dummyReg_, 0);
            JumpTo(node, end);

            SetLabel(node, ifUndefined);
            LoadAccumulatorString(node, "undefined");

            SetLabel(node, end);
        } else {
            Ra().Emit<CallVirtAccShort, 0>(node, AssemblerSignatureReference(Signatures::BUILTIN_OBJECT_TO_STRING),
                                           dummyReg_, 0);
        }
    }

    VReg arg0 = AllocReg();
    StoreAccumulator(node, arg0);

    CallExactDevirtual(node, signature, builder, arg0);
    SetAccumulatorType(Checker()->GetGlobalTypesHolder()->GlobalStringBuilderBuiltinType());
}

void ETSGen::AppendString(const ir::Expression *const expr, const VReg builder)
{
    ES2PANDA_ASSERT((expr->IsBinaryExpression() &&
                     expr->AsBinaryExpression()->OperatorType() == lexer::TokenType::PUNCTUATOR_PLUS) ||
                    (expr->IsAssignmentExpression() &&
                     expr->AsAssignmentExpression()->OperatorType() == lexer::TokenType::PUNCTUATOR_PLUS_EQUAL));

    if (expr->IsBinaryExpression()) {
        StringBuilder(expr->AsBinaryExpression()->Left(), expr->AsBinaryExpression()->Right(), builder);
    } else {
        StringBuilder(expr->AsAssignmentExpression()->Left(), expr->AsAssignmentExpression()->Right(), builder);
    }
}

void ETSGen::StringBuilder(const ir::Expression *const left, const ir::Expression *const right, const VReg builder)
{
    if (left->IsBinaryExpression() && left->TsType()->IsETSStringType()) {
        AppendString(left->AsBinaryExpression(), builder);
    } else {
        StringBuilderAppend(left, builder);
    }

    StringBuilderAppend(right, builder);
}

void ETSGen::BuildString(const ir::BinaryExpression *node, VReg lhs)
{
    // #26986 use concat instead of append
    if (Context()->config->options->IsEtsStringsConcat()) {
        ConcatStrings(node, lhs);
        return;
    }
    auto builder = AllocReg();
    CreateStringBuilder(node, lhs, builder);
    CallExact(node, Signatures::BUILTIN_STRING_BUILDER_TO_STRING, builder);
    SetAccumulatorType(node->TsType());
}

void ETSGen::CreateStringBuilder(const ir::BinaryExpression *node, VReg lhs, VReg builder)
{
    ES2PANDA_ASSERT(node->OperationType()->IsETSStringType());

    if (node->Left()->IsBinaryExpression()) {
        CreateStringBuilder(node->Left()->AsBinaryExpression(), lhs, builder);
        StringBuilderAppend(node->Right(), builder);
    } else {
        node->Left()->Compile(this);
        ApplyConversionAndStoreAccumulator(node->Left(), lhs, node->OperationType());
        ES2PANDA_ASSERT(GetVRegType(lhs)->IsETSStringType());

        RegScope rs(this);
        Ra().Emit<InitobjShort>(node, AssemblerSignatureReference(Signatures::BUILTIN_STRING_BUILDER_CTOR_STRING), lhs,
                                dummyReg_);
        SetAccumulatorType(Checker()->GlobalStringBuilderBuiltinType());
        StoreAccumulator(node, builder);
        StringBuilderAppend(node->Right(), builder);
    }
}

void ETSGen::CallBigIntUnaryOperator(const ir::Expression *node, VReg arg, const util::StringView signature)
{
    LoadAccumulator(node, arg);
    CallExact(node, signature, arg);
    SetAccumulatorType(Checker()->GlobalETSBigIntType());
}

void ETSGen::CallBigIntBinaryOperator(const ir::Expression *node, VReg lhs, VReg rhs, const util::StringView signature)
{
    LoadAccumulator(node, lhs);
    CallExact(node, signature, lhs, rhs);
    SetAccumulatorType(Checker()->GlobalETSBigIntType());
}

void ETSGen::CallBigIntBinaryComparison(const ir::Expression *node, VReg lhs, VReg rhs,
                                        const util::StringView signature)
{
    LoadAccumulator(node, lhs);
    CallExact(node, signature, lhs, rhs);
    SetAccumulatorType(Checker()->GlobalETSBooleanType());
}

void ETSGen::BuildTemplateString(const ir::TemplateLiteral *node)
{
    // #26986 use concat instead of append
    if (Context()->config->options->IsEtsStringsConcat()) {
        ConcatTemplateString(node);
    } else {
        AppendTemplateString(node);
    }
}

void ETSGen::AppendTemplateString(const ir::TemplateLiteral *node)
{
    RegScope rs(this);

    Ra().Emit<InitobjShort, 0>(node, AssemblerSignatureReference(Signatures::BUILTIN_STRING_BUILDER_CTOR), dummyReg_,
                               dummyReg_);
    SetAccumulatorType(Checker()->GlobalStringBuilderBuiltinType());

    auto builder = AllocReg();
    StoreAccumulator(node, builder);
    // Just to reduce extra nested level(s):
    auto const appendExpressions = [this, &builder](ArenaVector<ir::Expression *> const &expressions,
                                                    ArenaVector<ir::TemplateElement *> const &quasis) -> void {
        ES2PANDA_ASSERT(quasis.size() == expressions.size() + 1);
        for (size_t i = 0; i < expressions.size();) {
            StringBuilderAppend(expressions[i], builder);
            if (!quasis[++i]->Raw().Empty()) {
                StringBuilderAppend(quasis[i], builder);
            }
        }
    };
    if (auto const &quasis = node->Quasis(); !quasis.empty()) {
        if (!quasis[0]->Raw().Empty()) {
            StringBuilderAppend(quasis[0], builder);
        }

        if (auto const &expressions = node->Expressions(); !expressions.empty()) {
            appendExpressions(expressions, quasis);
        }
    }
    CallExact(node, Signatures::BUILTIN_STRING_BUILDER_TO_STRING, builder);
    SetAccumulatorType(node->TsType());
}

void ETSGen::ConcatTemplateString(const ir::TemplateLiteral *node)
{
    const auto &quasis = node->Quasis();
    const auto &expressions = node->Expressions();
    if (quasis.empty()) {
        LoadAccumulatorString(node, "");
        return;
    }
    ES2PANDA_ASSERT(quasis.size() == expressions.size() + 1);
    const RegScope rs {this};
    const auto result = AllocReg();
    const auto reg = AllocReg();
    // collect expressions
    std::vector<const ir::Expression *> buffer {};
    buffer.reserve(quasis.size() + expressions.size());
    if (!quasis[0]->Raw().Empty()) {
        buffer.push_back(quasis[0]);
    }
    for (size_t i = 0; i < expressions.size(); ++i) {
        buffer.push_back(expressions[i]);
        if (!quasis[i + 1]->Raw().Empty()) {
            buffer.push_back(quasis[i + 1]);
        }
    }
    // concat buffered expressions
    if (buffer.empty()) {
        LoadAccumulatorString(node, "");
        return;
    }
    const auto node2str = [this, reg](const ir::Expression *expr) {
        expr->Compile(this);
        StoreAccumulator(expr, reg);
        ToString(expr, reg);
        ES2PANDA_ASSERT(GetAccumulatorType()->IsETSStringType());
    };
    auto iter = buffer.begin();
    node2str(*iter);
    StoreAccumulator(node, result);
    for (++iter; iter != buffer.end(); ++iter) {
        node2str(*iter);
        StoreAccumulator(*iter, reg);
        CallExact(node, Signatures::BUILTIN_STRING_BUILDER_CONCAT_STRING, result, reg);
        SetAccumulatorType(Checker()->GlobalBuiltinETSStringType());
        StoreAccumulator(node, result);
    }
    LoadAccumulator(node, result);
    SetAccumulatorType(node->TsType());
}

void ETSGen::NewObject(const ir::AstNode *const node, const util::StringView name, VReg athis)
{
    Ra().Emit<Newobj>(node, athis, AssemblerReference(name));
    SetVRegType(athis, Checker()->GlobalETSObjectType());
}

void ETSGen::NewArray(const ir::AstNode *const node, const VReg arr, const VReg dim, const checker::Type *const arrType)
{
    auto str = ToAssemblerType(arrType);
    const auto res = ProgElement()->Strings().emplace(str);

    Ra().Emit<Newarr>(node, arr, dim, AssemblerReference(util::StringView(*res.first)));
    SetVRegType(arr, arrType);
}

void ETSGen::LoadResizableArrayLength(const ir::AstNode *node)
{
    Ra().Emit<CallAccShort, 0>(node, AssemblerSignatureReference(Signatures::BUILTIN_ARRAY_LENGTH), dummyReg_, 0);
    SetAccumulatorType(Checker()->GlobalIntType());
}

void ETSGen::LoadResizableArrayElement(const ir::AstNode *node, const VReg arrObj, const VReg arrIndex)
{
    Ra().Emit<CallVirtShort>(node, AssemblerSignatureReference(Signatures::BUILTIN_ARRAY_GET_ELEMENT), arrObj,
                             arrIndex);
    // #32345 - GuardUncheckedType is missing
}

void ETSGen::LoadArrayLength(const ir::AstNode *node, VReg arrayReg)
{
    Ra().Emit<Lenarr>(node, arrayReg);
    SetAccumulatorType(Checker()->GlobalIntType());
}

void ETSGen::LoadArrayElement(const ir::AstNode *node, VReg objectReg)
{
    ES2PANDA_ASSERT(GetVRegType(objectReg) != nullptr);
    auto *elementType = GetVRegType(objectReg)->AsETSArrayType()->ElementType();
    if (elementType->IsETSReferenceType()) {
        Ra().Emit<LdarrObj>(node, objectReg);
        SetAccumulatorType(elementType);
        return;
    }
    switch (checker::ETSChecker::ETSType(elementType)) {
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::BYTE: {
            Ra().Emit<Ldarr8>(node, objectReg);
            break;
        }
        case checker::TypeFlag::CHAR: {
            Ra().Emit<Ldarru16>(node, objectReg);
            break;
        }
        case checker::TypeFlag::SHORT: {
            Ra().Emit<Ldarr16>(node, objectReg);
            break;
        }
        case checker::TypeFlag::INT: {
            Ra().Emit<Ldarr>(node, objectReg);
            break;
        }
        case checker::TypeFlag::LONG: {
            Ra().Emit<LdarrWide>(node, objectReg);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            Ra().Emit<Fldarr32>(node, objectReg);
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            Ra().Emit<FldarrWide>(node, objectReg);
            break;
        }

        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    SetAccumulatorType(elementType);
}

void ETSGen::StoreArrayElement(const ir::AstNode *node, VReg objectReg, VReg index, const checker::Type *elementType)
{
    if (elementType->IsETSReferenceType()) {
        Ra().Emit<StarrObj>(node, objectReg, index);
        SetAccumulatorType(elementType);
        return;
    }
    switch (checker::ETSChecker::ETSType(elementType)) {
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::BYTE: {
            Ra().Emit<Starr8>(node, objectReg, index);
            break;
        }
        case checker::TypeFlag::CHAR:
        case checker::TypeFlag::SHORT: {
            Ra().Emit<Starr16>(node, objectReg, index);
            break;
        }
        case checker::TypeFlag::INT: {
            Ra().Emit<Starr>(node, objectReg, index);
            break;
        }
        case checker::TypeFlag::LONG: {
            Ra().Emit<StarrWide>(node, objectReg, index);
            break;
        }
        case checker::TypeFlag::FLOAT: {
            Ra().Emit<Fstarr32>(node, objectReg, index);
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            Ra().Emit<FstarrWide>(node, objectReg, index);
            break;
        }

        default: {
            ES2PANDA_UNREACHABLE();
        }
    }

    SetAccumulatorType(elementType);
}

util::StringView ETSGen::GetTupleMemberNameForIndex(const std::size_t index) const
{
    return MakeView(this, "$" + std::to_string(index));
}

void ETSGen::LoadTupleElement(const ir::AstNode *node, VReg objectReg, const checker::Type *elementType,
                              std::size_t index)
{
    ES2PANDA_ASSERT(GetVRegType(objectReg) != nullptr && GetVRegType(objectReg)->IsETSTupleType());
    const auto propName = FormClassOwnPropReference(GetVRegType(objectReg)->AsETSTupleType()->GetWrapperType(),
                                                    GetTupleMemberNameForIndex(index));

    // NOTE (smartin): remove after generics without type erasure is possible
    const auto *const boxedElementType = Checker()->MaybeBoxType(elementType);
    LoadProperty(node, boxedElementType, objectReg, propName);
}

void ETSGen::StoreTupleElement(const ir::AstNode *node, VReg objectReg, const checker::Type *elementType,
                               std::size_t index)
{
    ES2PANDA_ASSERT(GetVRegType(objectReg) != nullptr && GetVRegType(objectReg)->IsETSTupleType());
    const auto *const tupleType = GetVRegType(objectReg)->AsETSTupleType();
    SetVRegType(objectReg, tupleType->GetWrapperType());
    const auto fullName = FormClassOwnPropReference(tupleType->GetWrapperType(), GetTupleMemberNameForIndex(index));

    // NOTE (smartin): remove after generics without type erasure is possible
    const auto *const boxedElementType = Checker()->MaybeBoxType(elementType);
    StoreProperty(node, boxedElementType, objectReg, fullName);
}

template <typename T>
void ETSGen::IncrementImmediateRegister(const ir::AstNode *node, VReg reg, const checker::TypeFlag valueType,
                                        T const value)
{
    switch (valueType) {
        // NOTE: operand of increment instruction (INCI) is defined in spec as 32-bit integer,
        // but its current implementation actually can work with 64-bit integers as well.
        case checker::TypeFlag::INT: {
            Ra().Emit<Inci>(node, reg, static_cast<checker::IntType::UType>(value));
            break;
        }
        case checker::TypeFlag::CHAR: {
            Ra().Emit<Inci>(node, reg, static_cast<checker::CharType::UType>(value));
            break;
        }
        case checker::TypeFlag::SHORT: {
            Ra().Emit<Inci>(node, reg, static_cast<checker::ShortType::UType>(value));
            break;
        }
        case checker::TypeFlag::ETS_BOOLEAN:
            [[fallthrough]];
        case checker::TypeFlag::BYTE: {
            Ra().Emit<Inci>(node, reg, static_cast<checker::ByteType::UType>(value));
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

template void ETSGen::IncrementImmediateRegister<int32_t>(const ir::AstNode *node, VReg reg,
                                                          const checker::TypeFlag valueType, int32_t const value);

void ETSGen::LoadStringLength(const ir::AstNode *node)
{
    Ra().Emit<CallAccShort, 0>(node, AssemblerSignatureReference(Signatures::BUILTIN_STRING_LENGTH), dummyReg_, 0);
    SetAccumulatorType(Checker()->GlobalIntType());
}

void ETSGen::FloatIsNaN(const ir::AstNode *node)
{
    Ra().Emit<CallAccShort, 0>(node, AssemblerSignatureReference(Signatures::BUILTIN_FLOAT_IS_NAN), dummyReg_, 0);
    SetAccumulatorType(Checker()->GlobalETSBooleanType());
}

void ETSGen::DoubleIsNaN(const ir::AstNode *node)
{
    Ra().Emit<CallAccShort, 0>(node, AssemblerSignatureReference(Signatures::BUILTIN_DOUBLE_IS_NAN), dummyReg_, 0);
    SetAccumulatorType(Checker()->GlobalETSBooleanType());
}

void ETSGen::LoadStringChar(const ir::AstNode *node, const VReg stringObj, const VReg charIndex, bool needBox)
{
    Ra().Emit<CallShort>(node, AssemblerSignatureReference(Signatures::BUILTIN_STRING_CHAR_AT), stringObj, charIndex);
    SetAccumulatorType(Checker()->GlobalCharType());
    if (needBox) {
        Ra().Emit<CallAccShort, 0>(node, AssemblerSignatureReference(Signatures::BUILTIN_CHAR_VALUE_OF), dummyReg_, 0);
        SetAccumulatorType(Checker()->GlobalCharBuiltinType());
    }
}

void ETSGen::ThrowException(const ir::Expression *expr)
{
    RegScope rs(this);

    expr->Compile(this);
    VReg arg = AllocReg();
    StoreAccumulator(expr, arg);
    EmitThrow(expr, arg);
}

bool ETSGen::ExtendWithFinalizer(ir::AstNode const *node, const ir::AstNode *originalNode, Label *prevFinnaly)
{
    ES2PANDA_ASSERT(originalNode != nullptr);

    if (node == nullptr || !node->IsStatement()) {
        return false;
    }

    if ((originalNode->IsContinueStatement() && originalNode->AsContinueStatement()->Target() == node) ||
        (originalNode->IsBreakStatement() && originalNode->AsBreakStatement()->Target() == node)) {
        return false;
    }

    if (node->IsTryStatement() && node->AsTryStatement()->HasFinalizer()) {
        Label *beginLabel = nullptr;

        if (prevFinnaly == nullptr) {
            beginLabel = AllocLabel();
            Branch(originalNode, beginLabel);
        } else {
            beginLabel = prevFinnaly;
        }

        Label *endLabel = AllocLabel();

        if (node->Parent() != nullptr && node->Parent()->IsStatement()) {
            if (!ExtendWithFinalizer(node->Parent(), originalNode, endLabel)) {
                endLabel = nullptr;
            }
        } else {
            endLabel = nullptr;
        }

        LabelPair insertion = compiler::LabelPair(beginLabel, endLabel);

        auto *tryStatement = const_cast<ir::AstNode *>(node)->AsTryStatement();
        tryStatement->AddFinalizerInsertion(insertion, originalNode->AsStatement());

        return true;
    }

    auto *parent = node->Parent();

    if (parent == nullptr || !parent->IsStatement()) {
        return false;
    }

    if (parent->IsTryStatement() && node->IsBlockStatement() &&
        parent->AsTryStatement()->FinallyBlock() == node->AsBlockStatement()) {
        parent = parent->Parent();
    }

    return ExtendWithFinalizer(parent, originalNode, prevFinnaly);
}

util::StringView ETSGen::ToAssemblerType(const es2panda::checker::Type *type) const
{
    ES2PANDA_ASSERT(type != nullptr && type->IsETSReferenceType());

    std::stringstream ss;
    type->ToAssemblerTypeWithRank(ss);
    auto const str = ss.str();
    return MakeView(this, Emitter()->AddDependence(str));
}

template <typename T>
void ETSGen::SetAccumulatorTargetType(const ir::AstNode *node, checker::TypeFlag typeKind, T number)
{
    switch (typeKind) {
        case checker::TypeFlag::ETS_BOOLEAN:
        case checker::TypeFlag::BYTE: {
            Sa().Emit<Ldai>(node, static_cast<checker::ByteType::UType>(number));
            SetAccumulatorType(Checker()->GlobalByteType());
            break;
        }
        case checker::TypeFlag::CHAR: {
            Sa().Emit<Ldai>(node, static_cast<checker::CharType::UType>(number));
            SetAccumulatorType(Checker()->GlobalCharType());
            break;
        }
        case checker::TypeFlag::SHORT: {
            Sa().Emit<Ldai>(node, static_cast<checker::ShortType::UType>(number));
            SetAccumulatorType(Checker()->GlobalShortType());
            break;
        }
        case checker::TypeFlag::INT: {
            Sa().Emit<Ldai>(node, static_cast<checker::IntType::UType>(number));
            SetAccumulatorType(Checker()->GlobalIntType());
            break;
        }
        case checker::TypeFlag::LONG: {
            Sa().Emit<LdaiWide>(node, static_cast<checker::LongType::UType>(number));
            SetAccumulatorType(Checker()->GlobalLongType());
            break;
        }
        case checker::TypeFlag::FLOAT: {
            Sa().Emit<Fldai>(node, static_cast<checker::FloatType::UType>(number));
            SetAccumulatorType(Checker()->GlobalFloatType());
            break;
        }
        case checker::TypeFlag::DOUBLE: {
            Sa().Emit<FldaiWide>(node, static_cast<checker::DoubleType::UType>(number));
            SetAccumulatorType(Checker()->GlobalDoubleType());
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

template <typename T>
void ETSGen::LoadAccumulatorNumber(const ir::AstNode *node, T number, checker::TypeFlag targetType)
{
    // NOTE (smartin): refactor this to do at a more appropriate place
    const bool isContextTargetTypeValid =
        targetType_ != nullptr &&
        (!targetType_->IsETSObjectType() && !targetType_->IsETSUnionType() && !targetType_->IsETSArrayType() &&
         !targetType_->IsETSTupleType() && !targetType_->IsETSTypeParameter());

    auto typeKind = isContextTargetTypeValid ? checker::ETSChecker::TypeKind(targetType_) : targetType;

    SetAccumulatorTargetType(node, typeKind, number);

    if (targetType_ && (targetType_->IsETSObjectType() || targetType_->IsETSUnionType())) {
        ApplyConversion(node, targetType_);
    }
}
}  // namespace ark::es2panda::compiler
