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

#ifndef ES2PANDA_COMPILER_CORE_ETSGEN_H
#define ES2PANDA_COMPILER_CORE_ETSGEN_H

#include "ir/astNode.h"
#include "compiler/core/codeGen.h"
#include "compiler/core/ETSfunction.h"
#include "compiler/core/targetTypeContext.h"
#include "checker/ETSchecker.h"
#include "ir/expressions/identifier.h"
#include "util/helpers.h"
#include <variant>

namespace ark::es2panda::compiler {

class ETSGen final : public CodeGen {
public:
    explicit ETSGen(ArenaAllocator *allocator, RegSpiller *spiller, public_lib::Context *context,
                    std::tuple<varbinder::FunctionScope *, ProgramElement *, AstCompiler *> toCompile) noexcept;

    [[nodiscard]] const checker::ETSChecker *Checker() const noexcept;
    [[nodiscard]] const varbinder::ETSBinder *VarBinder() const noexcept;
    [[nodiscard]] const checker::Type *ReturnType() const noexcept;
    [[nodiscard]] const checker::ETSObjectType *ContainingObjectType() const noexcept;

    [[nodiscard]] VReg &Acc() noexcept;
    [[nodiscard]] VReg Acc() const noexcept;

    void SetAccumulatorType(const checker::Type *type);
    [[nodiscard]] const checker::Type *GetAccumulatorType() const;
    void CompileAndCheck(const ir::Expression *expr);

    [[nodiscard]] VReg StoreException(const ir::AstNode *node);
    void ApplyConversionAndStoreAccumulator(const ir::AstNode *node, VReg vreg, const checker::Type *targetType);
    void StoreAccumulator(const ir::AstNode *node, VReg vreg);
    void LoadAccumulator(const ir::AstNode *node, VReg vreg);
    [[nodiscard]] IRNode *AllocMov(const ir::AstNode *node, VReg vd, VReg vs) override;
    [[nodiscard]] IRNode *AllocMov(const ir::AstNode *node, OutVReg vd, VReg vs) override;
    void MoveVreg(const ir::AstNode *node, VReg vd, VReg vs);

    [[nodiscard]] checker::Type const *TypeForVar(varbinder::Variable const *var) const noexcept override;

    void LoadVar(const ir::Identifier *node, varbinder::Variable const *var);
    void LoadDynamicNamespaceVariable(const ir::AstNode *node, varbinder::Variable const *var);
    void StoreVar(const ir::Identifier *node, const varbinder::ConstScopeFindResult &result);

    void LoadStaticProperty(const ir::AstNode *node, const checker::Type *propType, const util::StringView &fullName);
    void StoreStaticProperty(const ir::AstNode *node, const checker::Type *propType, const util::StringView &fullName);

    void StoreStaticOwnProperty(const ir::AstNode *node, const checker::Type *propType, const util::StringView &name);
    [[nodiscard]] util::StringView FormClassPropReference(const checker::ETSObjectType *classType,
                                                          const util::StringView &name);

    void StoreProperty(const ir::AstNode *node, const checker::Type *propType, VReg objReg,
                       const util::StringView &name);
    void LoadProperty(const ir::AstNode *node, const checker::Type *propType, VReg objReg,
                      const util::StringView &fullName);

    void StoreByIndexAny(const ir::MemberExpression *node, VReg objectReg, VReg index);
    void LoadByIndexAny(const ir::MemberExpression *node, VReg objectReg);

    void StoreByValueAny(const ir::MemberExpression *node, VReg objectReg, VReg value);
    void LoadByValueAny(const ir::MemberExpression *node, VReg objectReg);

    void StorePropertyByNameAny(const ir::AstNode *const node, const VReg objReg, const util::StringView &fullName);
    void LoadPropertyByNameAny(const ir::AstNode *const node, const VReg objReg, const util::StringView &fullName);

    void StorePropertyByName(const ir::AstNode *node, VReg objReg,
                             checker::ETSChecker::NamedAccessMeta const &fieldMeta);
    void LoadPropertyByName(const ir::AstNode *node, VReg objReg,
                            checker::ETSChecker::NamedAccessMeta const &fieldMeta);

    void LoadUndefinedDynamic(const ir::AstNode *node, Language lang);

    void LoadThis(const ir::AstNode *node);
    [[nodiscard]] VReg GetThisReg() const;

    const checker::Type *LoadDefaultValue(const ir::AstNode *node, const checker::Type *type);
    void EmitReturnVoid(const ir::AstNode *node);
    void ReturnAcc(const ir::AstNode *node);

    void BranchIfIsInstance(const ir::AstNode *node, VReg srcReg, const checker::Type *target, Label *ifTrue);
    void BranchIfIsInstanceUnion(const ir::AstNode *node, VReg srcReg, const checker::Type *target, Label *ifTrue);
    void IsInstance(const ir::AstNode *node, VReg srcReg, checker::Type const *target);
    void EmitFailedTypeCastException(const ir::AstNode *node, const VReg src, checker::Type const *target,
                                     bool isUndef = false);

    void EmitNullcheck([[maybe_unused]] const ir::AstNode *node)
    {
#ifdef PANDA_WITH_ETS
        Sa().Emit<EtsNullcheck>(node);
#else
        ES2PANDA_UNREACHABLE();
#endif  // PANDA_WITH_ETS
    }

    void BinaryLogic(const ir::AstNode *node, lexer::TokenType op, VReg lhs);
    void BinaryArithmLogic(const ir::AstNode *node, lexer::TokenType op, VReg lhs);
    void Binary(const ir::AstNode *node, lexer::TokenType op, VReg lhs);
    void Unary(const ir::AstNode *node, lexer::TokenType op);
    void Update(const ir::AstNode *node, lexer::TokenType op);
    void UpdateBigInt(const ir::Expression *node, VReg arg, lexer::TokenType op);

    void Condition(const ir::AstNode *node, lexer::TokenType op, VReg lhs, Label *ifFalse);

    template <typename CondCompare, bool BEFORE_LOGICAL_NOT>
    void ResolveConditionalResultFloat(const ir::AstNode *node, Label *realEndLabel);

    template <typename CondCompare, bool BEFORE_LOGICAL_NOT, bool USE_FALSE_LABEL>
    void ResolveConditionalResultNumeric(const ir::AstNode *node, [[maybe_unused]] Label *ifFalse, Label **end);

    template <typename CondCompare, bool BEFORE_LOGICAL_NOT>
    void ResolveConditionalResultReference(const ir::AstNode *node);

    template <typename CondCompare, bool BEFORE_LOGICAL_NOT, bool USE_FALSE_LABEL>
    void ResolveConditionalResult(const ir::AstNode *node, [[maybe_unused]] Label *ifFalse);

    template <bool BEFORE_LOGICAL_NOT = false, bool FALSE_LABEL_EXISTED = true>
    void ResolveConditionalResultIfFalse(const ir::AstNode *node, Label *ifFalse = nullptr);

    template <bool BEFORE_LOGICAL_NOT = false, bool FALSE_LABEL_EXISTED = true>
    void ResolveConditionalResultIfTrue(const ir::AstNode *node, Label *ifFalse = nullptr);

    template <typename CondCompare, typename NegCondCompare>
    void BranchConditional(const ir::AstNode *node, Label *endLabel);

    void ConditionalFloat(const ir::AstNode *node);

    void BranchConditionalIfFalse(const ir::AstNode *node, Label *endLabel);

    void BranchConditionalIfTrue(const ir::AstNode *node, Label *endLabel);

    void BranchIfFalse(const ir::AstNode *node, Label *ifFalse)
    {
        Sa().Emit<Jeqz>(node, ifFalse);
    }

    void BranchIfTrue(const ir::AstNode *node, Label *ifTrue)
    {
        Sa().Emit<Jnez>(node, ifTrue);
    }

    void BranchIfUndefined(const ir::AstNode *node, Label *ifNull)
    {
        Sa().Emit<JeqzObj>(node, ifNull);
    }

    void BranchIfNotUndefined(const ir::AstNode *node, Label *ifNotNull)
    {
        Sa().Emit<JnezObj>(node, ifNotNull);
    }

    void BranchIfNull(const ir::AstNode *node, Label *ifTaken)
    {
        EmitIsNull(node);
        BranchIfTrue(node, ifTaken);
    }

    void BranchIfNotNull(const ir::AstNode *node, Label *ifTaken)
    {
        EmitIsNull(node);
        BranchIfFalse(node, ifTaken);
    }

    void BranchIfNullish(const ir::AstNode *node, Label *ifNullish);
    void BranchIfNotNullish(const ir::AstNode *node, Label *ifNotNullish);
    void AssumeNonNullish(const ir::AstNode *node, checker::Type const *targetType);

    void JumpTo(const ir::AstNode *node, Label *labelTo)
    {
        Sa().Emit<Jmp>(node, labelTo);
    }

    void EmitThrow(const ir::AstNode *node, VReg err)
    {
        Ra().Emit<Throw>(node, err);
    }

    void EmitNullishException(const ir::AstNode *node);
    void ThrowException(const ir::Expression *expr);
    bool ExtendWithFinalizer(ir::AstNode const *node, const ir::AstNode *originalNode, Label *prevFinnaly = nullptr);

    void Negate(const ir::AstNode *node);
    void LogicalNot(const ir::AstNode *node);

    void LoadAccumulatorByte(const ir::AstNode *node, int8_t number);

    void LoadAccumulatorShort(const ir::AstNode *node, int16_t number);

    void LoadAccumulatorInt(const ir::AstNode *node, int32_t number);

    void LoadAccumulatorWideInt(const ir::AstNode *node, int64_t number);

    void LoadAccumulatorFloat(const ir::AstNode *node, float number);

    void LoadAccumulatorDouble(const ir::AstNode *node, double number);

    void LoadAccumulatorBoolean(const ir::AstNode *node, bool value);

    void LoadAccumulatorString(const ir::AstNode *node, util::StringView str);

    void LoadAccumulatorBigInt(const ir::AstNode *node, util::StringView str);

    void LoadAccumulatorUndefined(const ir::AstNode *node);

    void LoadAccumulatorNull([[maybe_unused]] const ir::AstNode *node);

    void LoadAccumulatorPoison(const ir::AstNode *node, const checker::Type *type);

    void LoadAccumulatorChar(const ir::AstNode *node, char16_t value);

    void ApplyConversion(const ir::AstNode *node)
    {
        if (targetType_ != nullptr) {
            ApplyConversion(node, targetType_);
        }
    }
    void ApplyConversionCast(const ir::AstNode *node, const checker::Type *targetType);
    void ApplyConversion(const ir::AstNode *node, const checker::Type *targetType);
    void ApplyCast(const ir::AstNode *node, const checker::Type *targetType);
    void SwapBinaryOpArgs(const ir::AstNode *node, VReg lhs);
    VReg MoveAccToReg(const ir::AstNode *node);

    void LoadResizableArrayLength(const ir::AstNode *node);
    void LoadResizableArrayElement(const ir::AstNode *node, const VReg arrObj, const VReg arrIndex);
    void LoadArrayLength(const ir::AstNode *node, VReg arrayReg);
    void LoadArrayElement(const ir::AstNode *node, VReg objectReg);
    void StoreArrayElement(const ir::AstNode *node, VReg objectReg, VReg index, const checker::Type *elementType);

    util::StringView GetTupleMemberNameForIndex(std::size_t index) const;
    void LoadTupleElement(const ir::AstNode *node, VReg objectReg, const checker::Type *elementType, std::size_t index);
    void StoreTupleElement(const ir::AstNode *node, VReg objectReg, const checker::Type *elementType,
                           std::size_t index);

    template <typename T>
    void MoveImmediateToRegister(const ir::AstNode *node, VReg reg, const checker::TypeFlag valueType, T const value);

    template <typename T>
    void IncrementImmediateRegister(const ir::AstNode *node, VReg reg, const checker::TypeFlag valueType,
                                    T const value);

    template <typename IntCompare>
    void JumpCompareRegister(const ir::AstNode *node, VReg lhs, Label *ifFalse)
    {
        Ra().Emit<IntCompare>(node, lhs, ifFalse);
    }

    void LoadStringLength(const ir::AstNode *node);
    void LoadStringChar(const ir::AstNode *node, VReg stringObj, VReg charIndex, bool needBox = false);

    void FloatIsNaN(const ir::AstNode *node);
    void DoubleIsNaN(const ir::AstNode *node);

    void CompileStatements(const ArenaVector<ir::Statement *> &statements);

    // Cast
    void CastToBoolean(const ir::AstNode *node);
    void CastToByte(const ir::AstNode *node);
    void CastToChar(const ir::AstNode *node);
    void CastToShort(const ir::AstNode *node);
    void CastToDouble(const ir::AstNode *node);
    void CastToFloat(const ir::AstNode *node);
    void CastToLong(const ir::AstNode *node);
    void CastToInt(const ir::AstNode *node);
    void CastToReftype(const ir::AstNode *node, const checker::Type *targetType, bool unchecked);

    void InternalIsInstance(const ir::AstNode *node, const checker::Type *target);
    void InternalCheckCast(const ir::AstNode *node, const checker::Type *target);
    void EmitAnyCheckCast(const ir::AstNode *node, const checker::Type *target);
    void CheckedReferenceNarrowing(const ir::AstNode *node, const checker::Type *target);
    void GuardUncheckedType(const ir::AstNode *node, const checker::Type *unchecked, const checker::Type *target);

    // Call, Construct
    void NewArray(const ir::AstNode *node, VReg arr, VReg dim, const checker::Type *arrType);
    void NewObject(const ir::AstNode *node, util::StringView name, VReg athis);
    void BuildString(const ir::BinaryExpression *node, VReg lhs);
    void ConcatStrings(const ir::BinaryExpression *node, VReg lhs);
    void ToString(const ir::Expression *node, VReg arg);
    void CallBigIntUnaryOperator(const ir::Expression *node, VReg arg, util::StringView signature);
    void CallBigIntBinaryOperator(const ir::Expression *node, VReg lhs, VReg rhs, util::StringView signature);
    void CallBigIntBinaryComparison(const ir::Expression *node, VReg lhs, VReg rhs, util::StringView signature);
    void BuildTemplateString(const ir::TemplateLiteral *node);
    void InitObject(const ir::AstNode *node, checker::Signature const *signature,
                    const ArenaVector<ir::Expression *> &arguments)
    {
        CallImpl<InitobjShort, Initobj, InitobjRange>(node, signature, arguments);
    }

    bool IsDevirtualizedSignature(const checker::Signature *signature)
    {
        ES2PANDA_ASSERT(signature != nullptr && !signature->HasSignatureFlag(checker::SignatureFlags::STATIC));
        return signature->HasSignatureFlag(checker::SignatureFlags::FINAL | checker::SignatureFlags::PRIVATE |
                                           checker::SignatureFlags::CONSTRUCTOR);
    }

    void EmitEtsTypeof([[maybe_unused]] const ir::AstNode *node, [[maybe_unused]] const VReg reg)
    {
#ifdef PANDA_WITH_ETS
        Ra().Emit<EtsTypeof>(node, reg);
#else
        ES2PANDA_UNREACHABLE();
#endif  // PANDA_WITH_ETS
    }

    void EmitEtsIstrue([[maybe_unused]] const ir::AstNode *node, [[maybe_unused]] const VReg reg)
    {
#ifdef PANDA_WITH_ETS
        Ra().Emit<EtsIstrue>(node, reg);
#else
        ES2PANDA_UNREACHABLE();
#endif  // PANDA_WITH_ETS
    }

    void EmitAnyIsInstance(const ir::AstNode *node, VReg objReg)
    {
        Sa().Emit<AnyIsinstance>(node, objReg);
    }

    void CallExact(const ir::AstNode *node, checker::Signature *signature,
                   const ArenaVector<ir::Expression *> &arguments)
    {
        CallImpl<CallShort, Call, CallRange>(node, signature, arguments);
    }

    void CallExact(const ir::AstNode *const node, const checker::Signature *signature, const VReg arg0,
                   const ArenaVector<ir::Expression *> &arguments)
    {
        CallArgStart<CallShort, Call, CallRange>(node, signature, arg0, arguments);
    }

    void CallExact(const ir::AstNode *const node, const util::StringView name)
    {
        Ra().Emit<CallShort, 0>(node, name, dummyReg_, dummyReg_);
    }

    void CallExact(const ir::AstNode *const node, const util::StringView name, const VReg arg0)
    {
        Ra().Emit<CallShort, 1>(node, name, arg0, dummyReg_);
    }

    void CallExactDevirtual(const ir::AstNode *const node, const util::StringView name, const VReg arg0)
    {
        Ra().EmitDevirtual<CallShort, 1>(node, name, arg0, dummyReg_);
    }

    void CallExact(const ir::AstNode *const node, const util::StringView name, const VReg arg0, const VReg arg1)
    {
        Ra().Emit<CallShort>(node, name, arg0, arg1);
    }

    void CallExactDevirtual(const ir::AstNode *const node, const util::StringView name, const VReg arg0,
                            const VReg arg1)
    {
        Ra().EmitDevirtual<CallShort>(node, name, arg0, arg1);
    }

    void CallExact(const ir::AstNode *const node, const util::StringView name, const VReg arg0, const VReg arg1,
                   const VReg arg2)
    {
        Ra().Emit<Call, 3U>(node, name, arg0, arg1, arg2, dummyReg_);
    }

    void CallByName([[maybe_unused]] const ir::AstNode *const node,
                    [[maybe_unused]] const checker::Signature *signature, [[maybe_unused]] const VReg arg0,
                    [[maybe_unused]] const ArenaVector<ir::Expression *> &arguments)
    {
#ifdef PANDA_WITH_ETS
        CallArgStart<EtsCallNameShort, EtsCallName, EtsCallNameRange>(node, signature, arg0, arguments);
#else
        ES2PANDA_UNREACHABLE();
#endif
    }

    void CallVirtual(const ir::AstNode *const node, const checker::Signature *signature, const VReg athis,
                     const ArenaVector<ir::Expression *> &arguments)
    {
        ES2PANDA_ASSERT(!signature->HasSignatureFlag(checker::SignatureFlags::STATIC));
        ES2PANDA_ASSERT(!signature->Owner()->GetDeclNode()->IsFinal() || signature->IsFinal());
        if (IsDevirtualizedSignature(signature)) {
            CallArgStartDevirtual<CallShort, Call, CallRange>(node, signature, athis, arguments);
        } else {
            CallArgStart<CallVirtShort, CallVirt, CallVirtRange>(node, signature, athis, arguments);
        }
    }

    void CallVirtual(const ir::AstNode *const node, const checker::Signature *signature, const VReg athis)
    {
        if (IsDevirtualizedSignature(signature)) {
            CallExactDevirtual(node, signature->InternalName(), athis);
        } else {
            CallVirtual(node, signature->InternalName(), athis);
        }
    }

    void CallVirtual(const ir::AstNode *const node, const checker::Signature *signature, const VReg athis,
                     const VReg arg0)
    {
        if (IsDevirtualizedSignature(signature)) {
            CallExactDevirtual(node, signature->InternalName(), athis, arg0);
        } else {
            CallVirtual(node, signature->InternalName(), athis, arg0);
        }
    }

    void CallVirtual(const ir::AstNode *const node, const util::StringView name, const VReg athis)
    {
        Ra().Emit<CallVirtShort, 1>(node, name, athis, dummyReg_);
    }

    void CallVirtual(const ir::AstNode *const node, const util::StringView name, const VReg athis, const VReg arg0)
    {
        Ra().Emit<CallVirtShort>(node, name, athis, arg0);
    }

    struct CallDynamicData {
        const ir::AstNode *node = nullptr;
        VReg obj;
        VReg param2;
    };

    void CallDynamic(CallDynamicData data, checker::Signature *signature,
                     const ArenaVector<ir::Expression *> &arguments)
    {
        CallDynamicImpl<CallShort, Call, CallRange>(data, signature, arguments);
    }

    void CallDynamic(CallDynamicData data, VReg param3, checker::Signature *signature,
                     const ArenaVector<ir::Expression *> &arguments)
    {
        CallDynamicImpl<CallShort, Call, CallRange>(data, param3, signature, arguments);
    }

    void CallAnyNew(const ir::AstNode *const node, const ArenaVector<ir::Expression *> &arguments, const VReg athis)
    {
        CallAnyImpl<AnyCallNew0, AnyCallNewShort, AnyCallNewRange>(node, arguments, athis);
    }

    void CallAnyThis(const ir::AstNode *const node, const ir::Identifier *ident,
                     const ArenaVector<ir::Expression *> &arguments, const VReg athis)
    {
        CallAnyImpl<AnyCallThis0, AnyCallThisShort, AnyCallThisRange>(node, ident, arguments, athis);
    }

    void CallAny(const ir::AstNode *const node, const ArenaVector<ir::Expression *> &arguments, const VReg athis)
    {
        CallAnyImpl<AnyCall0, AnyCallShort, AnyCallRange>(node, arguments, athis);
    }

    // until a lowering for implicit super is available
    void CallRangeFillUndefined(const ir::AstNode *const node, checker::Signature *const signature, const VReg thisReg);

    void CreateBigIntObject(const ir::AstNode *node, VReg arg0,
                            std::string_view signature = Signatures::BUILTIN_BIGINT_CTOR);

    void GetType(const ir::AstNode *node, bool isEtsPrimitive)
    {
        if (isEtsPrimitive) {
            // NOTE: SzD. LoadStaticProperty if ETS stdlib has static TYPE constants otherwise fallback to LdaType
        } else {
            ES2PANDA_ASSERT(GetAccumulatorType() != nullptr);
            auto classRef = GetAccumulatorType()->AsETSObjectType()->AssemblerName();
            Sa().Emit<LdaType>(node, classRef);
        }
    }

    void EmitLdaType(const ir::AstNode *node, util::StringView sv)
    {
        Sa().Emit<LdaType>(node, sv);
    }

    ~ETSGen() override = default;
    NO_COPY_SEMANTIC(ETSGen);
    NO_MOVE_SEMANTIC(ETSGen);

private:
    const VReg dummyReg_ = VReg::RegStart();

    void LoadConstantObject(const ir::Expression *node, const checker::Type *type);
    void CreateStringBuilder(const ir::Expression *node);
    void StringBuilderAppend(const ir::AstNode *node, VReg builder);
    void AppendString(const ir::Expression *binExpr, VReg builder);
    void StringBuilder(const ir::Expression *left, const ir::Expression *right, VReg builder);
    void AppendTemplateString(const ir::TemplateLiteral *node);
    void ConcatTemplateString(const ir::TemplateLiteral *node);
    util::StringView FormClassPropReference(varbinder::Variable const *var);
    void UnaryMinus(const ir::AstNode *node);
    void UnaryTilde(const ir::AstNode *node);

    util::StringView ToAssemblerType(const es2panda::checker::Type *type) const;
    void TestIsInstanceType(const ir::AstNode *const node, std::tuple<Label *, Label *> label,
                            checker::Type const *target, const VReg srcReg, bool acceptNull);
    void CheckedReferenceNarrowingObject(const ir::AstNode *node, const checker::Type *target);

    template <bool IS_SRTICT = false>
    void HandleDefinitelyNullishEquality(const ir::AstNode *node, VReg lhs, VReg rhs, Label *ifFalse);
    template <bool IS_SRTICT = false>
    void HandlePossiblyNullishEquality(const ir::AstNode *node, VReg lhs, VReg rhs, Label *ifFalse, Label *ifTrue);

    bool IsNullUnsafeObjectType(checker::Type const *type) const;

    void EmitIsNull([[maybe_unused]] const ir::AstNode *node)
    {
#ifdef PANDA_WITH_ETS
        Sa().Emit<EtsIsnullvalue>(node);
#else
        ES2PANDA_UNREACHABLE();
#endif  // PANDA_WITH_ETS
    }

    void EmitCheckCast(const ir::AstNode *node, util::StringView target)
    {
        if (target != Signatures::BUILTIN_OBJECT) {
            Sa().Emit<Checkcast>(node, target);
        }
    }

    void EmitIsInstance(const ir::AstNode *node, util::StringView target)
    {
        if (target != Signatures::BUILTIN_OBJECT) {
            Sa().Emit<Isinstance>(node, target);
        } else {
            LoadAccumulatorBoolean(node, true);
        }
    }

    template <bool IS_SRTICT = false>
    void EmitEtsEquals([[maybe_unused]] const ir::AstNode *node, [[maybe_unused]] const VReg lhs,
                       [[maybe_unused]] const VReg rhs)
    {
#ifdef PANDA_WITH_ETS
        if constexpr (IS_SRTICT) {
            Ra().Emit<EtsStrictequals>(node, lhs, rhs);
        } else {
            Ra().Emit<EtsEquals>(node, lhs, rhs);
        }
#else
        ES2PANDA_UNREACHABLE();
#endif  // PANDA_WITH_ETS
    }

    template <typename T>
    void StoreValueIntoArray(const ir::AstNode *const node, const VReg arr, const VReg index)
    {
        Ra().Emit<T>(node, arr, index);
    }

    template <typename LongOp, typename IntOp, typename DoubleOp, typename FloatOp>
    void UpdateOperator(const ir::AstNode *node);

    template <typename Br>
    void InverseCondition(const ir::AstNode *node, Br const &br, Label *target, bool inverse = true)
    {
        if (!inverse) {
            br(target);
            return;
        }
        Label *loc = AllocLabel();
        br(loc);
        JumpTo(node, target);
        SetLabel(node, loc);
    }

    template <bool IS_SRTICT = false>
    void RefEqualityLoose(const ir::AstNode *node, VReg lhs, VReg rhs, Label *ifFalse);

    template <typename Compare, typename Cond>
    void BinaryNumberComparison(const ir::AstNode *node, VReg lhs, Label *ifFalse)
    {
        Ra().Emit<Compare>(node, lhs);
        Sa().Emit<Cond>(node, ifFalse);
    }

    template <typename IntCompare, typename CondCompare, typename DynCompare, bool IS_STRICT = false>
    void BinaryEquality(const ir::AstNode *node, VReg lhs, Label *ifFalse);

    template <typename IntCompare, typename CondCompare, bool IS_STRICT = false>
    void BinaryEqualityCondition(const ir::AstNode *node, VReg lhs, Label *ifFalse);

    template <typename IntCompare, typename CondCompare>
    void BinaryRelation(const ir::AstNode *node, VReg lhs, Label *ifFalse);

    template <typename IntCompare, typename CondCompare>
    void BinaryRelationCondition(const ir::AstNode *node, VReg lhs, Label *ifFalse);

    template <typename CompareGreater, typename CompareLess, typename CondCompare>
    void BinaryFloatingPointComparison(const ir::AstNode *node, VReg lhs, Label *ifFalse)
    {
        if constexpr (std::is_same_v<CondCompare, Jgez> || std::is_same_v<CondCompare, Jgtz>) {
            BinaryNumberComparison<CompareGreater, CondCompare>(node, lhs, ifFalse);
        } else {
            BinaryNumberComparison<CompareLess, CondCompare>(node, lhs, ifFalse);
        }
    }

    template <typename IntOp, typename LongOp, typename FloatOp, typename DoubleOp>
    void BinaryArithmetic(const ir::AstNode *node, VReg lhs);

    template <typename IntOp, typename LongOp>
    void BinaryBitwiseArithmetic(const ir::AstNode *node, VReg lhs);

// NOLINTBEGIN(cppcoreguidelines-macro-usage, readability-container-size-empty, modernize-loop-convert)
#define COMPILE_ARG(idx)                                                                                       \
    ES2PANDA_ASSERT((idx) < arguments.size());                                                                 \
    ES2PANDA_ASSERT((idx) < signature->Params().size() || signature->RestVar() != nullptr);                    \
    auto *param##idx = (idx) < signature->Params().size() ? signature->Params()[(idx)] : signature->RestVar(); \
    auto *paramType##idx = param##idx->TsType();                                                               \
    auto ttctx##idx = TargetTypeContext(this, paramType##idx);                                                 \
    arguments[idx]->Compile(this);                                                                             \
    VReg arg##idx = AllocReg();                                                                                \
    ApplyConversion(arguments[idx], nullptr);                                                                  \
    ApplyConversionAndStoreAccumulator(arguments[idx], arg##idx, paramType##idx)

    template <typename Short, typename General, typename Range>
    void CallArgStart(const ir::AstNode *const node, const checker::Signature *signature, const VReg argStart,
                      const ArenaVector<ir::Expression *> &arguments)
    {
        RegScope rs(this);
        const auto name = signature->InternalName();

        switch (arguments.size()) {
            case 0U: {
                Ra().Emit<Short, 1>(node, name, argStart, dummyReg_);
                break;
            }
            case 1U: {
                COMPILE_ARG(0);
                Ra().Emit<Short>(node, name, argStart, arg0);
                break;
            }
            case 2U: {
                COMPILE_ARG(0);
                COMPILE_ARG(1);
                Ra().Emit<General, 3U>(node, name, argStart, arg0, arg1, dummyReg_);
                break;
            }
            case 3U: {
                COMPILE_ARG(0);
                COMPILE_ARG(1);
                COMPILE_ARG(2);
                Ra().Emit<General>(node, name, argStart, arg0, arg1, arg2);
                break;
            }
            default: {
                for (size_t idx = 0; idx < arguments.size(); idx++) {
                    COMPILE_ARG(idx);
                }

                Rra().Emit<Range>(node, argStart, arguments.size() + 1, name, argStart);
                break;
            }
        }
    }

    template <typename Short, typename General, typename Range>
    void CallArgStartDevirtual(const ir::AstNode *const node, const checker::Signature *signature, const VReg argStart,
                               const ArenaVector<ir::Expression *> &arguments)
    {
        RegScope rs(this);
        const auto name = signature->InternalName();
        switch (arguments.size()) {
            case 0U: {
                Ra().EmitDevirtual<Short, 1>(node, name, argStart, dummyReg_);
                break;
            }
            case 1U: {
                COMPILE_ARG(0);
                Ra().EmitDevirtual<Short>(node, name, argStart, arg0);
                break;
            }
            case 2U: {
                COMPILE_ARG(0);
                COMPILE_ARG(1);
                Ra().EmitDevirtual<General, 3U>(node, name, argStart, arg0, arg1, dummyReg_);
                break;
            }
            case 3U: {
                COMPILE_ARG(0);
                COMPILE_ARG(1);
                COMPILE_ARG(2);
                Ra().EmitDevirtual<General>(node, name, argStart, arg0, arg1, arg2);
                break;
            }
            default: {
                for (size_t idx = 0; idx < arguments.size(); idx++) {
                    COMPILE_ARG(idx);
                }

                Rra().EmitDevirtual<Range>(node, argStart, arguments.size() + 1, name, argStart);
                break;
            }
        }
    }

    template <typename Short, typename General, typename Range>
    void CallImpl(const ir::AstNode *node, checker::Signature const *signature,
                  const ArenaVector<ir::Expression *> &arguments)
    {
        ES2PANDA_ASSERT(signature != nullptr);
        RegScope rs(this);

        switch (arguments.size()) {
            case 0U: {
                Ra().Emit<Short, 0U>(node, signature->InternalName(), dummyReg_, dummyReg_);
                break;
            }
            case 1U: {
                COMPILE_ARG(0);
                Ra().Emit<Short, 1U>(node, signature->InternalName(), arg0, dummyReg_);
                break;
            }
            case 2U: {
                COMPILE_ARG(0);
                COMPILE_ARG(1);
                Ra().Emit<Short, 2U>(node, signature->InternalName(), arg0, arg1);
                break;
            }
            case 3U: {
                COMPILE_ARG(0);
                COMPILE_ARG(1);
                COMPILE_ARG(2);
                Ra().Emit<General, 3U>(node, signature->InternalName(), arg0, arg1, arg2, dummyReg_);
                break;
            }
            case 4U: {
                COMPILE_ARG(0);
                COMPILE_ARG(1);
                COMPILE_ARG(2);
                COMPILE_ARG(3);
                Ra().Emit<General, 4U>(node, signature->InternalName(), arg0, arg1, arg2, arg3);
                break;
            }
            default: {
                VReg argStart = NextReg();

                for (size_t idx = 0; idx < arguments.size(); idx++) {
                    COMPILE_ARG(idx);
                }

                Rra().Emit<Range>(node, argStart, arguments.size(), signature->InternalName(), argStart);
                break;
            }
        }
    }
#undef COMPILE_ARG

#define COMPILE_ARG(idx, shift)                                                                       \
    ES2PANDA_ASSERT((idx) < arguments.size());                                                        \
    ES2PANDA_ASSERT((idx) + (shift) < signature->Params().size() || signature->RestVar() != nullptr); \
    auto *paramType##idx = (idx) + (shift) < signature->Params().size()                               \
                               ? signature->Params()[(idx) + (shift)]->TsType()                       \
                               : signature->RestVar()->TsType();                                      \
    auto ttctx##idx = TargetTypeContext(this, paramType##idx);                                        \
    VReg arg##idx = AllocReg();                                                                       \
    arguments[idx]->Compile(this);                                                                    \
    ApplyConversionAndStoreAccumulator(arguments[idx], arg##idx, paramType##idx)

    template <typename Short, typename General, typename Range>
    void CallDynamicImpl(CallDynamicData data, checker::Signature *signature,
                         const ArenaVector<ir::Expression *> &arguments)
    {
        RegScope rs(this);
        const auto name = signature->InternalName();

        switch (arguments.size()) {
            case 0U: {
                Ra().Emit<Short>(data.node, name, data.obj, data.param2);
                break;
            }
            case 1U: {
                COMPILE_ARG(0, 2U);
                Ra().Emit<General, 3U>(data.node, name, data.obj, data.param2, arg0, dummyReg_);
                break;
            }
            case 2U: {
                COMPILE_ARG(0, 2U);
                COMPILE_ARG(1, 2U);
                Ra().Emit<General>(data.node, name, data.obj, data.param2, arg0, arg1);
                break;
            }
            default: {
                for (size_t idx = 0; idx < arguments.size(); idx++) {
                    COMPILE_ARG(idx, 2U);
                }

                Rra().Emit<Range>(data.node, data.obj, arguments.size() + 2U, name, data.obj);
                break;
            }
        }
    }

    template <typename Short, typename General, typename Range>
    void CallDynamicImpl(CallDynamicData data, VReg param3, checker::Signature *signature,
                         const ArenaVector<ir::Expression *> &arguments)
    {
        RegScope rs(this);
        const auto name = signature->InternalName();

        switch (arguments.size()) {
            case 0U: {
                Ra().Emit<General, 3U>(data.node, name, data.obj, data.param2, param3, dummyReg_);
                break;
            }
            case 1U: {
                COMPILE_ARG(0, 3U);
                Ra().Emit<General>(data.node, name, data.obj, data.param2, param3, arg0);
                break;
            }
            default: {
                for (size_t idx = 0; idx < arguments.size(); idx++) {
                    COMPILE_ARG(idx, 3U);
                }

                Rra().Emit<Range>(data.node, data.obj, arguments.size() + 3U, name, data.obj);
                break;
            }
        }
    }

#undef COMPILE_ARG

#define COMPILE_ANY_ARG(idx)                                   \
    ES2PANDA_ASSERT((idx) < arguments.size());                 \
    auto *param##idx = arguments[idx];                         \
    auto *paramType##idx = param##idx->TsType();               \
    auto ttctx##idx = TargetTypeContext(this, paramType##idx); \
    arguments[idx]->Compile(this);                             \
    VReg arg##idx = AllocReg();                                \
    ApplyConversion(arguments[idx], nullptr);                  \
    ApplyConversionAndStoreAccumulator(arguments[idx], arg##idx, paramType##idx)

    template <typename Zero, typename Short, typename Range>
    void CallAnyImpl(const ir::AstNode *node, const ir::Identifier *ident,
                     const ArenaVector<ir::Expression *> &arguments, const VReg athis)
    {
        ES2PANDA_ASSERT(ident != nullptr);
        RegScope rs(this);

        switch (arguments.size()) {
            case 0U: {
                Ra().Emit<Zero>(node, ident->Name(), athis);
                break;
            }
            case 1U: {
                COMPILE_ANY_ARG(0);
                Ra().Emit<Short>(node, ident->Name(), athis, arg0);
                break;
            }
            default: {
                VReg argStart = NextReg();
                for (size_t idx = 0; idx < arguments.size(); idx++) {
                    COMPILE_ANY_ARG(idx);
                }

                Rra().Emit<Range>(node, argStart, arguments.size(), ident->Name(), athis, argStart, arguments.size());
            }
        }
    }

    template <typename Zero, typename Short, typename Range>
    void CallAnyImpl(const ir::AstNode *node, const ArenaVector<ir::Expression *> &arguments, const VReg athis)
    {
        RegScope rs(this);

        switch (arguments.size()) {
            case 0U: {
                Ra().Emit<Zero>(node, athis);
                break;
            }
            case 1U: {
                COMPILE_ANY_ARG(0);
                Ra().Emit<Short>(node, athis, arg0);
                break;
            }
            default: {
                VReg argStart = NextReg();
                for (size_t idx = 0; idx < arguments.size(); idx++) {
                    COMPILE_ANY_ARG(idx);
                }

                Rra().Emit<Range>(node, argStart, arguments.size(), athis, argStart, arguments.size());
            }
        }
    }
#undef COMPILE_ANY_ARG
    // NOLINTEND(cppcoreguidelines-macro-usage, readability-container-size-empty, modernize-loop-convert)

    void ToBinaryResult(const ir::AstNode *node, Label *ifFalse);

    template <typename T>
    void LoadAccumulatorNumber(const ir::AstNode *node, T number, checker::TypeFlag targetType);
    template <typename T>
    void SetAccumulatorTargetType(const ir::AstNode *node, checker::TypeFlag typeKind, T number);
    void InitializeContainingClass();

    friend class TargetTypeContext;

    VReg acc_ {};
    const checker::Type *targetType_ {};
    const checker::ETSObjectType *containingObjectType_ {};
};

}  // namespace ark::es2panda::compiler

#endif
