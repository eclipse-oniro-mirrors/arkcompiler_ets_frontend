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

#ifndef ES2PANDA_CHECKER_ETS_CHECKER_H
#define ES2PANDA_CHECKER_ETS_CHECKER_H

#include <cstddef>
#include <mutex>

#include "checker/checker.h"

#include "checker/types/globalTypesHolder.h"
#include "checker/types/ets/etsResizableArrayType.h"
#include "checker/types/ets/types.h"
#include "checker/resolveResult.h"
#include "ir/statements/annotationDeclaration.h"
#include "ir/expressions/callExpression.h"
#include "ir/visitor/AstVisitor.h"
#include "types/type.h"
#include "util/helpers.h"
#include "util/ustring.h"

namespace ark::es2panda::varbinder {
class VarBinder;
class Decl;
class EnumVariable;
class FunctionDecl;
class LocalVariable;
class Scope;
class Variable;
class ETSBinder;
class RecordTable;
class FunctionParamScope;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::evaluate {
class ScopedDebugInfoPlugin;
}  // namespace ark::es2panda::evaluate

namespace ark::es2panda::checker {

struct Accessor {
    bool isGetter {false};
    bool isSetter {false};
    bool isExternal {false};
};

struct PairHash {
    size_t operator()(const std::pair<Type *, bool> &p) const
    {
        size_t hash1 = std::hash<Type *> {}(p.first);
        size_t hash2 = std::hash<bool> {}(p.second);
        return hash1 ^ (hash2 << 1ULL);
    }
};

template <typename T>
std::vector<T> ArenaVectorToStdVector(ArenaVector<T> const &av)
{
    std::vector<T> res;
    res.assign(av.begin(), av.end());
    return res;
}

template <typename T>
ArenaVector<T> StdVectorToArenaVector(std::vector<T> const &av, ArenaAllocator *allocator)
{
    ArenaVector<T> res {allocator->Adapter()};
    res.assign(av.begin(), av.end());
    return res;
}

using ComputedAbstracts =
    std::unordered_map<ETSObjectType *, std::pair<std::vector<ETSFunctionType *>, std::unordered_set<ETSObjectType *>>>;
using ArrayMap = std::unordered_map<std::pair<Type *, bool>, ETSArrayType *, PairHash>;
using ObjectInstantiationMap = std::unordered_map<ETSObjectType *, std::unordered_map<std::string, ETSObjectType *>>;
template <typename T>
using TypeInstantiationCacheMap = std::unordered_map<std::string, T *>;
using GlobalArraySignatureMap = std::unordered_map<const ETSArrayType *, Signature *>;
using FunctionSignatureMap = std::unordered_map<ETSFunctionType *, ETSFunctionType *>;
using FunctionInterfaceMap = std::unordered_map<ETSFunctionType *, ETSObjectType *>;
using FunctionalInterfaceMap = std::unordered_map<util::StringView, ETSObjectType *>;
using TypeMapping = std::unordered_map<Type const *, Type *>;
using ConstraintCheckRecord = std::tuple<const ArenaVector<Type *> *, const Substitution, lexer::SourcePosition>;
// can't use util::DiagnosticWithParams because std::optional can't contain references
using MaybeDiagnosticInfo =
    std::optional<std::pair<const diagnostic::DiagnosticKind, const util::DiagnosticMessageParams>>;
using AstNodePtr = ir::AstNode *;
using TypePtr = Type *;

class ETSChecker final : public Checker {
public:
    explicit ETSChecker(ArenaAllocator *allocator, util::DiagnosticEngine &diagnosticEngine)
        // NOLINTNEXTLINE(readability-redundant-member-init)
        : Checker(allocator, diagnosticEngine)
    {
    }

    ~ETSChecker() override = default;

    NO_COPY_SEMANTIC(ETSChecker);
    NO_MOVE_SEMANTIC(ETSChecker);

    [[nodiscard]] static TypeFlag ETSType(const Type *const type) noexcept
    {
        return ETSChecker::TypeKind(type);
    }

    [[nodiscard]] static TypeFlag TypeKind(const Type *const type) noexcept;

    Type *GlobalByteType() const;
    Type *GlobalShortType() const;
    Type *GlobalIntType() const;
    Type *GlobalLongType() const;
    Type *GlobalFloatType() const;
    Type *GlobalDoubleType() const;
    Type *GlobalCharType() const;
    Type *GlobalETSBooleanType() const;
    Type *GlobalVoidType() const;
    Type *GlobalETSNullType() const;
    Type *GlobalETSUndefinedType() const;
    Type *GlobalETSAnyType() const;
    Type *GlobalETSRelaxedAnyType() const;
    Type *GlobalETSNeverType() const;
    Type *GlobalETSStringLiteralType() const;
    Type *GlobalETSBigIntType() const;
    Type *GlobalWildcardType() const;

    Type *GlobalByteBuiltinType() const;
    Type *GlobalShortBuiltinType() const;
    Type *GlobalIntBuiltinType() const;
    Type *GlobalLongBuiltinType() const;
    Type *GlobalFloatBuiltinType() const;
    Type *GlobalDoubleBuiltinType() const;
    Type *GlobalCharBuiltinType() const;
    Type *GlobalETSBooleanBuiltinType() const;

    ETSObjectType *GlobalETSObjectType() const;
    ETSUnionType *GlobalETSUnionUndefinedNull() const;
    ETSUnionType *GlobalETSUnionUndefinedNullObject() const;
    ETSObjectType *GlobalBuiltinClassType() const;
    ETSObjectType *GlobalBuiltinETSResizableArrayType() const;
    ETSObjectType *GlobalBuiltinETSStringType() const;
    ETSObjectType *GlobalBuiltinETSBigIntType() const;
    ETSObjectType *GlobalBuiltinTypeType() const;
    ETSObjectType *GlobalBuiltinErrorType() const;
    ETSObjectType *GlobalStringBuilderBuiltinType() const;
    ETSObjectType *GlobalBuiltinPromiseType() const;
    ETSObjectType *GlobalBuiltinFunctionType() const;
    ETSObjectType *GlobalBuiltinBoxType(Type *contents);

    ETSObjectType *GlobalBuiltinFunctionType(size_t nargs, bool hasRest) const;
    ETSObjectType *GlobalBuiltinLambdaType(size_t nargs, bool hasRest) const;
    size_t GlobalBuiltinFunctionTypeVariadicThreshold() const;

    ETSObjectType *GlobalBuiltinTupleType(size_t nargs) const;

    GlobalArraySignatureMap &GlobalArrayTypes();
    const GlobalArraySignatureMap &GlobalArrayTypes() const;

    const auto &UnionAssemblerTypes() const
    {
        return unionAssemblerTypes_;
    }

    auto &UnionAssemblerTypes()
    {
        return unionAssemblerTypes_;
    }

    bool IsRelaxedAnyTypeAnnotationAllowed() const
    {
        return permitRelaxedAny_;
    }

    Type *GlobalTypeError() const;
    [[nodiscard]] Type *InvalidateType(ir::Typed<ir::AstNode> *node);
    [[nodiscard]] Type *TypeError(ir::Typed<ir::AstNode> *node, const diagnostic::DiagnosticKind &diagKind,
                                  const lexer::SourcePosition &at);
    [[nodiscard]] Type *TypeError(ir::Typed<ir::AstNode> *node, const diagnostic::DiagnosticKind &diagKind,
                                  const util::DiagnosticMessageParams &list, const lexer::SourcePosition &at);
    [[nodiscard]] Type *TypeError(varbinder::Variable *var, const diagnostic::DiagnosticKind &diagKind,
                                  const lexer::SourcePosition &at);
    [[nodiscard]] Type *TypeError(varbinder::Variable *var, const diagnostic::DiagnosticKind &diagKind,
                                  const util::DiagnosticMessageParams &list, const lexer::SourcePosition &at);

    void InitializeBuiltins(varbinder::ETSBinder *varbinder);
    void InitializeBuiltin(varbinder::Variable *var, const util::StringView &name);
    bool StartChecker([[maybe_unused]] varbinder::VarBinder *varbinder, const util::Options &options) override;
    Type *CheckTypeCached(ir::Expression *expr) override;
    void ResolveStructuredTypeMembers([[maybe_unused]] Type *type) override {};
    Type *GetTypeFromVariableDeclaration(varbinder::Variable *const var);
    Type *GetTypeOfVariable([[maybe_unused]] varbinder::Variable *var) override;
    Type *GuaranteedTypeForUncheckedCast(Type *base, Type *substituted);
    Type *GuaranteedTypeForUncheckedCallReturn(Signature *sig);
    Type *GuaranteedTypeForUncheckedPropertyAccess(varbinder::Variable *prop);
    Type *GuaranteedTypeForUnionFieldAccess(ir::MemberExpression *memberExpression, ETSUnionType *etsUnionType);
    void ReputCheckerData();

    [[nodiscard]] bool IsETSChecker() const noexcept override
    {
        return true;
    }

    // Object
    void CheckObjectLiteralKeys(const ArenaVector<ir::Expression *> &properties);
    Type *BuildBasicClassProperties(ir::ClassDefinition *classDef);
    Type *BuildBasicInterfaceProperties(ir::TSInterfaceDeclaration *interfaceDecl);
    ETSObjectType *GetSuperType(ETSObjectType *type);
    ArenaVector<ETSObjectType *> const &GetInterfaces(ETSObjectType *type);
    void GetInterfacesOfClass(ETSObjectType *type);
    void GetInterfacesOfInterface(ETSObjectType *type);
    void ValidateImplementedInterface(ETSObjectType *type, Type *interface, std::unordered_set<Type *> *extendsSet,
                                      const lexer::SourcePosition &pos);
    void ResolveDeclaredMembersOfObject(const Type *type);
    std::optional<std::size_t> GetTupleElementAccessValue(const ir::Expression *expr);
    bool ValidateArrayIndex(ir::Expression *expr, bool relaxed = false);
    bool ValidateTupleIndex(const ETSTupleType *tuple, ir::MemberExpression *expr, bool reportError = true);
    bool ValidateTupleIndexFromEtsObject(const ETSTupleType *const tuple, ir::MemberExpression *expr);
    Type *CheckThisOrSuperAccess(ir::Expression *node, ETSObjectType *classType, std::string_view msg);
    void CreateTypeForClassOrInterfaceTypeParameters(ETSObjectType *type);
    ETSTypeParameter *SetUpParameterType(ir::TSTypeParameter *param);
    void GetInterfacesOfClass(ETSObjectType *type, ArenaVector<ETSObjectType *> &interfaces);
    void CheckIfOverrideIsValidInInterface(ETSObjectType *classType, Signature *sig, Signature *sigFunc);
    void CheckDynamicInheritanceAndImplement(ETSObjectType *const interfaceOrClassType);
    void CheckFunctionRedeclarationInInterface(ETSObjectType *classType, ArenaVector<Signature *> &similarSignatures,
                                               Signature *sigFunc);
    void ValidateAbstractMethodsToBeImplemented(std::vector<ETSFunctionType *> &abstractsToBeImplemented,
                                                ETSObjectType *classType,
                                                const std::vector<Signature *> &implementedSignatures);
    void ValidateOptionalPropOverriding(const std::vector<ETSFunctionType *> &optionalProps, ETSObjectType *classType);
    void ApplyModifiersAndRemoveImplementedAbstracts(std::vector<ETSFunctionType *>::iterator &it,
                                                     std::vector<ETSFunctionType *> &abstractsToBeImplemented,
                                                     ETSObjectType *classType, bool &functionOverridden,
                                                     const Accessor &isGetSetExternal);
    void ValidateAbstractSignature(std::vector<ETSFunctionType *>::iterator &it,
                                   std::vector<ETSFunctionType *> &abstractsToBeImplemented,
                                   const std::vector<Signature *> &implementedSignatures, bool &functionOverridden,
                                   Accessor &isGetSetExternal);
    void ValidateNonOverriddenFunction(ETSObjectType *classType, std::vector<ETSFunctionType *>::iterator &it,
                                       std::vector<ETSFunctionType *> &abstractsToBeImplemented,
                                       bool &functionOverridden, const Accessor &isGetSet);
    void MaybeReportErrorsForOverridingValidation(std::vector<ETSFunctionType *> &abstractsToBeImplemented,
                                                  ETSObjectType *classType, const lexer::SourcePosition &pos,
                                                  bool reportError);
    void AddAccessorFlagsForOptionalPropInterface(ETSObjectType *classType, ETSObjectType *interfaceType,
                                                  ir::MethodDefinition *ifaceMethod);
    void ValidateOverriding(ETSObjectType *classType, const lexer::SourcePosition &pos);
    void CheckInterfaceFunctions(ETSObjectType *classType);
    void CollectImplementedMethodsFromInterfaces(ETSObjectType *classType,
                                                 std::vector<Signature *> *implementedSignatures,
                                                 std::vector<ETSFunctionType *> *optionalProps,
                                                 const std::vector<ETSFunctionType *> &abstractsToBeImplemented);
    void AddImplementedSignature(std::vector<Signature *> *implementedSignatures, varbinder::LocalVariable *function,
                                 ETSFunctionType *it);
    void AddOptionalProps(std::vector<ETSFunctionType *> *optionalPropSignatures, varbinder::LocalVariable *function);
    void CheckInnerClassMembers(const ETSObjectType *classType);
    void CheckLocalClass(ir::ClassDefinition *classDef, CheckerStatus &checkerStatus);
    void CheckClassDefinition(ir::ClassDefinition *classDef);
    void CheckClassElement(ir::ClassDefinition *classDef);
    void CheckClassAnnotations(ir::ClassDefinition *classDef);
    void CheckInterfaceAnnotations(ir::TSInterfaceDeclaration *interfaceDecl);
    void CheckConstructors(ir::ClassDefinition *classDef, ETSObjectType *classType);
    void FindAssignment(const ir::AstNode *node, const varbinder::LocalVariable *classVar, bool &initialized);
    void FindAssignments(const ir::AstNode *node, const varbinder::LocalVariable *classVar, bool &initialized);
    void CheckConstFields(const ETSObjectType *classType);
    void CheckConstFieldInitialized(const ETSObjectType *classType, varbinder::LocalVariable *classVar);
    void CheckConstFieldInitialized(const Signature *signature, varbinder::LocalVariable *classVar);
    void ComputeAbstractsFromInterface(ETSObjectType *interfaceType);
    std::vector<ETSFunctionType *> &GetAbstractsForClass(ETSObjectType *classType);
    std::vector<Signature *> CollectAbstractSignaturesFromObject(const ETSObjectType *objType);
    void CreateFunctionTypesFromAbstracts(const std::vector<Signature *> &abstracts,
                                          std::vector<ETSFunctionType *> *target);
    void CheckCyclicConstructorCall(Signature *signature);
    void CheckAnnotationReference(const ir::MemberExpression *memberExpr, const varbinder::LocalVariable *prop);
    std::vector<ResolveResult *> HandlePropertyResolution(varbinder::LocalVariable *const prop,
                                                          ir::MemberExpression *const memberExpr,
                                                          varbinder::Variable *const globalFunctionVar,
                                                          PropertySearchFlags searchFlag);
    std::vector<ResolveResult *> ResolveMemberReference(const ir::MemberExpression *memberExpr, ETSObjectType *target);
    varbinder::LocalVariable *ResolveOverloadReference(const ir::Identifier *ident, ETSObjectType *objType,
                                                       PropertySearchFlags searchFlags);
    void WarnForEndlessLoopInGetterSetter(const ir::MemberExpression *const memberExpr);
    varbinder::Variable *GetExtensionFuncVarInGlobalFunction(const ir::MemberExpression *const memberExpr);
    varbinder::Variable *GetExtensionFuncVarInGlobalField(const ir::MemberExpression *const memberExpr);
    varbinder::Variable *GetExtensionFuncVarInFunctionScope(const ir::MemberExpression *const memberExpr);
    varbinder::Variable *ResolveInstanceExtension(const ir::MemberExpression *memberExpr);
    void CheckImplicitSuper(ETSObjectType *classType, Signature *ctorSig);
    void CheckThisOrSuperCallInConstructor(ETSObjectType *classType, Signature *ctorSig);
    void CheckExpressionsInConstructor(const ArenaVector<const ir::Expression *> &arguments);
    ArenaVector<const ir::Expression *> CheckMemberOrCallOrObjectExpressionInConstructor(const ir::Expression *arg);
    void CheckValidInheritance(ETSObjectType *classType, ir::ClassDefinition *classDef);
    void CheckProperties(ETSObjectType *classType, ir::ClassDefinition *classDef, varbinder::LocalVariable *it,
                         varbinder::LocalVariable *found, ETSObjectType *interfaceFound);
    void CheckReadonlyClassPropertyInImplementedInterface(ETSObjectType *classType, varbinder::LocalVariable *field);
    void TransformProperties(ETSObjectType *classType);
    void CheckGetterSetterProperties(ETSObjectType *classType);
    void ComputeApparentType(Type *type);
    [[nodiscard]] Type *GetApparentType(Type *type);
    [[nodiscard]] Type const *GetApparentType(Type const *type) const;
    Type *GetConstantBuiltinType(Type *type);

    void VariableTypeFromInitializer(varbinder::Variable *variable, Type *annotationType, Type *initType);

    bool TypeHasDefaultValue(Type *tp) const;

    // Type creation
    ByteType *CreateByteType(int8_t value);
    ETSBooleanType *CreateETSBooleanType(bool value);
    DoubleType *CreateDoubleType(double value);
    FloatType *CreateFloatType(float value);
    IntType *CreateIntType(int32_t value);
    LongType *CreateLongType(int64_t value);
    ShortType *CreateShortType(int16_t value);
    CharType *CreateCharType(char16_t value);
    ETSBigIntType *CreateETSBigIntLiteralType(util::StringView value);
    ETSStringType *CreateETSStringLiteralType(util::StringView value);
    ETSResizableArrayType *CreateETSMultiDimResizableArrayType(Type *element, size_t dimSize);
    ETSResizableArrayType *CreateETSResizableArrayType(Type *element);
    ETSArrayType *CreateETSArrayType(Type *elementType, bool isCachePolluting = false);

    Type *CreateETSUnionType(Span<Type *const> constituentTypes);
    template <size_t N>
    Type *CreateETSUnionType(Type *const (&arr)[N])  // NOLINT(modernize-avoid-c-arrays)
    {
        return CreateETSUnionType(Span(arr));
    }
    Type *CreateETSUnionType(std::vector<Type *> &&constituentTypes)
    {
        return CreateETSUnionType(Span<Type *const>(constituentTypes));
    }

    ETSTupleType *CreateETSTupleType(Span<Type *const> elements, bool readonly);
    ETSTupleType *CreateETSTupleType(std::vector<Type *> &&elements, bool readonly)
    {
        return CreateETSTupleType(Span<Type *const>(elements), readonly);
    }

    Type *CreateUnionFromKeyofType(ETSObjectType *const type);
    ETSAsyncFuncReturnType *CreateETSAsyncFuncReturnTypeFromPromiseType(ETSObjectType *promiseType);
    ETSAsyncFuncReturnType *CreateETSAsyncFuncReturnTypeFromBaseType(Type *baseType);
    ETSTypeAliasType *CreateETSTypeAliasType(util::StringView name, const ir::AstNode *declNode,
                                             bool isRecursive = false);
    ETSFunctionType *CreateETSArrowType(Signature *signature);
    ETSFunctionType *CreateETSMethodType(util::StringView name, ArenaVector<Signature *> &&signatures);
    ETSExtensionFuncHelperType *CreateETSExtensionFuncHelperType(ETSFunctionType *classMethodType,
                                                                 ETSFunctionType *extensionFunctionType);
    ETSTypeParameter *CreateTypeParameter();

    ETSObjectType *CreateETSObjectType(
        ir::AstNode *declNode, ETSObjectFlags flags,
        /* this parameter maintanis the behavior of the broken ast-cache logic, avoid it whenever possible */
        std::optional<std::pair<ArenaAllocator *, TypeRelation *>> caches = std::nullopt);
    ETSObjectType *CreateETSObjectTypeOrBuiltin(ir::AstNode *declNode, ETSObjectFlags flags);
    std::tuple<util::StringView, SignatureInfo *> CreateBuiltinArraySignatureInfo(const ETSArrayType *arrayType,
                                                                                  size_t dim);
    Signature *CreateBuiltinArraySignature(const ETSArrayType *arrayType, size_t dim);
    ETSObjectType *CreatePromiseOf(Type *type);

    Signature *CreateSignature(SignatureInfo *info, Type *returnType, ir::ScriptFunction *func);
    Signature *CreateSignature(SignatureInfo *info, Type *returnType, ir::ScriptFunctionFlags sff, bool hasReceiver);
    SignatureInfo *CreateSignatureInfo();

    // Arithmetic
    bool CheckBinaryOperatorForBigInt(Type *left, Type *right, lexer::TokenType op);
    [[nodiscard]] bool CheckBinaryPlusMultDivOperandsForUnionType(const Type *leftType, const Type *rightType,
                                                                  const ir::Expression *left,
                                                                  const ir::Expression *right);
    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    std::tuple<Type *, Type *> CheckBinaryOperator(ir::Expression *left, ir::Expression *right, ir::Expression *expr,
                                                   lexer::TokenType operationType, lexer::SourcePosition pos,
                                                   bool forcePromotion = false);
    std::tuple<Type *, Type *> CheckArithmeticOperations(
        ir::Expression *expr,
        std::tuple<ir::Expression *, ir::Expression *, lexer::TokenType, lexer::SourcePosition> op, bool isEqualOp,
        std::tuple<checker::Type *, checker::Type *, Type *, Type *> types);
    checker::Type *CheckBinaryOperatorMulDivMod(
        std::tuple<ir::Expression *, ir::Expression *, lexer::TokenType, lexer::SourcePosition> op, bool isEqualOp,
        std::tuple<checker::Type *, checker::Type *, Type *, Type *> types);

    checker::Type *CheckBinaryBitwiseOperatorForNumericEnums(checker::Type *const leftType,
                                                             checker::Type *const rightType);

    checker::Type *CheckBinaryOperatorExponentiation(
        std::tuple<ir::Expression *, ir::Expression *, lexer::TokenType, lexer::SourcePosition> op, bool isEqualOp,
        std::tuple<checker::Type *, checker::Type *, Type *, Type *> types);

    checker::Type *CheckBinaryOperatorPlus(
        std::tuple<ir::Expression *, ir::Expression *, lexer::TokenType, lexer::SourcePosition> op, bool isEqualOp,
        std::tuple<checker::Type *, checker::Type *, Type *, Type *> types);
    checker::Type *CheckBinaryOperatorShift(
        std::tuple<ir::Expression *, ir::Expression *, lexer::TokenType, lexer::SourcePosition> op, bool isEqualOp,
        std::tuple<checker::Type *, checker::Type *, Type *, Type *> types);
    checker::Type *CheckBinaryOperatorBitwise(
        std::tuple<ir::Expression *, ir::Expression *, lexer::TokenType, lexer::SourcePosition> op, bool isEqualOp,
        std::tuple<checker::Type *, checker::Type *, Type *, Type *> types);
    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    checker::Type *CheckBinaryOperatorLogical(ir::Expression *left, ir::Expression *right, checker::Type *leftType,
                                              checker::Type *rightType, Type *unboxedL, Type *unboxedR);
    std::tuple<Type *, Type *> CheckBinaryOperatorStrictEqual(ir::Expression *left, lexer::TokenType operationType,
                                                              lexer::SourcePosition pos, checker::Type *leftType,
                                                              checker::Type *rightType);
    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    std::tuple<Type *, Type *> CheckBinaryOperatorLessGreater(ir::Expression *left, ir::Expression *right,
                                                              lexer::TokenType operationType, lexer::SourcePosition pos,
                                                              bool isEqualOp, checker::Type *leftType,
                                                              checker::Type *rightType, Type *unboxedL, Type *unboxedR);
    std::tuple<Type *, Type *> CheckBinaryOperatorInstanceOf(const ir::Expression *right, checker::Type *leftType,
                                                             checker::Type *rightType);
    checker::Type *CheckBinaryOperatorNullishCoalescing(ir::Expression *left, ir::Expression *right,
                                                        lexer::SourcePosition pos);
    bool CheckIfNumeric(Type *type);
    bool CheckIfFloatingPoint(Type *type);

    bool ValidateArrayTypeInitializerByElement(ir::ArrayExpression *node, Type *target);
    Type *HandleArithmeticOperationOnTypes(Type *left, Type *right, lexer::TokenType operationType);
    void SetGenerateValueOfFlags(std::tuple<checker::Type *, checker::Type *, Type *, Type *> types,
                                 std::tuple<ir::Expression *, ir::Expression *> nodes);
    template <typename TargetType>
    Type *PerformRelationOperationOnTypes(Type *left, Type *right, lexer::TokenType operationType);

    // Function
    static bool NeedTypeInference(const ir::ScriptFunction *lambda);
    bool ContainsTypeParameter(checker::Type *type);
    void InferTypesForLambda(ir::ScriptFunction *lambda, ir::ETSFunctionType *calleeType,
                             Signature *maybeSubstitutedFunctionSig = nullptr);
    void InferTypesForLambda(ir::ScriptFunction *lambda, Signature *signature);
    void TryInferTypeForLambdaTypeAlias(ir::ArrowFunctionExpression *expr, ETSFunctionType *calleeType);

    ArenaSubstitution *NewArenaSubstitution()
    {
        return ProgramAllocator()->New<ArenaSubstitution>(ProgramAllocator()->Adapter());
    }

    static Substitution ArenaSubstitutionToSubstitution(const ArenaSubstitution *orig);
    void EmplaceSubstituted(Substitution *substitution, ETSTypeParameter *tparam, Type *typeArg);
    void EmplaceSubstituted(ArenaSubstitution *substitution, ETSTypeParameter *tparam, Type *typeArg);

    [[nodiscard]] bool EnhanceSubstitutionForType(const ArenaVector<Type *> &typeParams, Type *paramType,
                                                  Type *argumentType, Substitution *substitution);
    std::pair<ArenaVector<Type *>, bool> CreateUnconstrainedTypeParameters(
        ir::TSTypeParameterDeclaration const *typeParams);
    [[nodiscard]] std::optional<Substitution> CheckTypeParamsAndBuildSubstitutionIfValid(
        Signature *signature, const ArenaVector<ir::TypeNode *> &params, const lexer::SourcePosition &pos);
    void AssignTypeParameterConstraints(ir::TSTypeParameterDeclaration const *typeParams);
    void ThrowSignatureMismatch(ArenaVector<Signature *> const &signatures,
                                const ArenaVector<ir::Expression *> &arguments, const lexer::SourcePosition &pos,
                                std::string_view signatureKind);
    Signature *FirstMatchSignatures(ArenaVector<Signature *> &signatures, ir::CallExpression *expr);
    Signature *MatchOrderSignatures(ArenaVector<Signature *> &signatures,
                                    const ArenaVector<ir::Expression *> &arguments, const ir::Expression *expr,
                                    TypeRelationFlag resolveFlags, std::string_view signatureKind = "call");

    Signature *ResolveConstructExpression(ETSObjectType *type, ir::ETSNewClassInstanceExpression *expr);
    Signature *ComposeSignature(ir::ScriptFunction *func, SignatureInfo *signatureInfo, Type *returnType,
                                varbinder::Variable *nameVar);
    Type *ComposeReturnType(ir::TypeNode *typeAnnotation, bool isAsync);
    SignatureInfo *ComposeSignatureInfo(ir::TSTypeParameterDeclaration *typeParams,
                                        ArenaVector<ir::Expression *> const &params);
    void BuildFunctionSignature(ir::ScriptFunction *func, bool isConstructSig = false);
    ETSFunctionType *BuildMethodType(ir::ScriptFunction *func);
    Type *BuildMethodSignature(ir::MethodDefinition *method);
    static Signature *GetSignatureFromMethodDefinition(const ir::MethodDefinition *methodDef);
    bool CheckIdenticalOverloads(ETSFunctionType *func, ETSFunctionType *overload,
                                 const ir::MethodDefinition *currentFunc, bool omitSameAsm = false,
                                 TypeRelationFlag relationFlags = TypeRelationFlag::NO_RETURN_TYPE_CHECK);
    static bool HasSameAssemblySignature(Signature const *const sig1, Signature const *const sig2) noexcept;
    static bool HasSameAssemblySignatures(ETSFunctionType const *const func1,
                                          ETSFunctionType const *const func2) noexcept;
    static bool HasParameterlessConstructor(checker::Type *type);
    Signature *AdjustForTypeParameters(Signature *source, Signature *target);
    void CheckOverride(Signature *signature);
    [[nodiscard]] bool IsReturnTypeSubstitutable(Signature *s1, Signature *s2);
    void ValidateSignatureAccessibility(ETSObjectType *callee, Signature *signature, const lexer::SourcePosition &pos,
                                        const MaybeDiagnosticInfo &maybeErrorInfo = std::nullopt);

    // CC-OFFNXT(G.FUN.01-CPP) solid logic
    ir::MethodDefinition *CreateMethod(const util::StringView &name, ir::ModifierFlags modifiers,
                                       ir::ScriptFunctionFlags flags, ArenaVector<ir::Expression *> &&params,
                                       varbinder::FunctionParamScope *paramScope, ir::TypeNode *returnType,
                                       ir::AstNode *body);
    varbinder::FunctionParamScope *CopyParams(
        const ArenaVector<ir::Expression *> &params, ArenaVector<ir::Expression *> &outParams,
        ArenaUnorderedMap<varbinder::Variable *, varbinder::Variable *> *paramVarMap);
    void ReplaceScope(ir::AstNode *root, ir::AstNode *oldNode, varbinder::Scope *newScope);

    // Helpers
    std::string FunctionalInterfaceInvokeName(size_t arity, bool hasRest);
    static std::string GetAsyncImplName(const util::StringView &name);
    static std::string GetAsyncImplName(ir::MethodDefinition *asyncMethod);
    std::vector<util::StringView> GetNameForSynteticObjectType(const util::StringView &source);
    template <checker::PropertyType TYPE>
    void BindingsModuleObjectAddProperty(checker::ETSObjectType *moduleObjType, ir::ETSImportDeclaration *importDecl,
                                         const varbinder::Scope::VariableMap &bindings,
                                         const util::StringView &importPath);
    std::vector<util::StringView> FindPropNameForNamespaceImport(const util::StringView &originalName,
                                                                 const util::StringView &importPath);
    void SetPropertiesForModuleObject(checker::ETSObjectType *moduleObjType, const util::StringView &importPath,
                                      ir::ETSImportDeclaration *importDecl = nullptr);
    parser::Program *SelectEntryOrExternalProgram(varbinder::ETSBinder *etsBinder, const util::StringView &importPath);
    void SetrModuleObjectTsType(ir::Identifier *local, checker::ETSObjectType *moduleObjType);
    Type *GetReferencedTypeFromBase(Type *baseType, ir::Expression *name);
    Type *GetReferencedTypeBase(ir::Expression *name);
    Type *ResolveReferencedType(varbinder::LocalVariable *refVar, const ir::Expression *name);
    Type *GetTypeFromInterfaceReference(varbinder::Variable *var);
    Type *GetTypeFromTypeAliasReference(varbinder::Variable *var);
    Type *GetTypeFromClassReference(varbinder::Variable *var);
    void ValidateGenericTypeAliasForClonedNode(ir::TSTypeAliasDeclaration *typeAliasNode,
                                               const ir::TSTypeParameterInstantiation *exactTypeParams);
    Type *HandleTypeAlias(ir::Expression *name, const ir::TSTypeParameterInstantiation *typeParams,
                          ir::TSTypeAliasDeclaration *const typeAliasNode);
    bool CheckMinimumTypeArgsPresent(const ir::TSTypeAliasDeclaration *typeAliasNode,
                                     const ir::TSTypeParameterInstantiation *typeParams);
    static ir::TypeNode *ResolveTypeNodeForTypeArg(const ir::TSTypeAliasDeclaration *typeAliasNode,
                                                   const ir::TSTypeParameterInstantiation *typeParams, size_t idx);
    Type *GetTypeFromTypeParameterReference(varbinder::LocalVariable *var, const lexer::SourcePosition &pos);
    Type *GetNonConstantType(Type *type);
    checker::Type *GetElementTypeOfArray(checker::Type *type) const;
    const checker::Type *GetElementTypeOfArray(const checker::Type *type) const;
    bool IsNullLikeOrVoidExpression(const ir::Expression *expr) const;
    void ValidateUnaryOperatorOperand(varbinder::Variable *variable, ir::Expression *expr);
    void CheckFunctionSignatureAnnotations(const ArenaVector<ir::Expression *> &params,
                                           ir::TSTypeParameterDeclaration *typeParams,
                                           ir::TypeNode *returnTypeAnnotation);
    bool CheckAndLogInvalidThisUsage(const ir::TypeNode *type, const diagnostic::DiagnosticKind &diagnostic);
    bool IsFixedArray(ir::ETSTypeReferencePart *part);
    void ValidateThisUsage(const ir::TypeNode *returnTypeAnnotation);

    template <typename T>
    void CheckAnnotations(ir::AnnotationAllowed<T> *node)
    {
        if (node->HasAnnotations()) {
            CheckAnnotations(node->Annotations());
        }
    }

    void CheckAmbientAnnotation(ir::AnnotationDeclaration *annoImpl, ir::AnnotationDeclaration *annoDecl);
    bool CheckAmbientAnnotationFieldInitializerValue(ir::Expression *init, ir::Expression *expected);
    bool CheckAmbientAnnotationFieldInitializer(ir::Expression *init, ir::Expression *expected);
    void CheckAnnotationRetention(ir::AnnotationUsage *anno);
    void CheckAnnotationTarget(ir::AnnotationUsage *anno);
    std::optional<ir::AnnotationTargets> GetFunctionTarget(ir::ScriptFunction *func);
    std::optional<ir::AnnotationTargets> GetParentNodeTarget(ir::AstNode *parent);
    void HandleAnnotationRetention(ir::AnnotationUsage *anno, ir::AnnotationDeclaration *annoDecl);
    void HandleAnnotationTarget(ir::AnnotationUsage *anno, ir::AnnotationDeclaration *annoDecl);
    void CheckStandardAnnotation(ir::AnnotationUsage *anno);
    void CheckAnnotationPropertyType(ir::ClassProperty *property);
    void CheckSinglePropertyAnnotation(ir::AnnotationUsage *st, ir::AnnotationDeclaration *annoDecl);
    void CheckMultiplePropertiesAnnotation(ir::AnnotationUsage *st, util::StringView const &baseName,
                                           ArenaUnorderedMap<util::StringView, ir::ClassProperty *> &fieldMap);
    void InferLambdaInAssignmentExpression(ir::AssignmentExpression *const expr);
    void InferAliasLambdaType(ir::TypeNode *localTypeAnnotation, ir::ArrowFunctionExpression *init);
    checker::Type *ApplyConditionalOperatorPromotion(checker::ETSChecker *checker, checker::Type *unboxedL,
                                                     checker::Type *unboxedR);
    Type *ApplyUnaryOperatorPromotion(ir::Expression *expr, Type *type, bool isCondExpr = false);
    Type *GetUnaryOperatorPromotedType(Type *type, const bool doPromotion = true);
    Type *HandleBooleanLogicalOperators(Type *leftType, Type *rightType, lexer::TokenType tokenType);

    checker::Type *FixOptionalVariableType(varbinder::Variable *const bindingVar, ir::ModifierFlags flags);
    void CheckEnumType(ir::Expression *init, checker::Type *initType, const util::StringView &varName);
    checker::Type *CheckVariableDeclaration(ir::Identifier *ident, ir::TypeNode *typeAnnotation, ir::Expression *init,
                                            ir::ModifierFlags flags);
    void CheckTruthinessOfType(ir::Expression *expr);

    bool CheckNonNullish(ir::Expression const *expr);
    Type *GetNonNullishType(Type *type);
    Type *RemoveNullType(Type *type);
    Type *RemoveUndefinedType(Type *type);
    std::pair<Type *, Type *> RemoveNullishTypes(Type *type);

    void ConcatConstantString(util::UString &target, Type *type);
    Type *HandleStringConcatenation(Type *leftType, Type *rightType);
    Type *ResolveIdentifier(ir::Identifier *ident);
    ETSFunctionType *FindFunctionInVectorGivenByName(util::StringView name, std::vector<ETSFunctionType *> &list);
    void MergeComputedAbstracts(std::vector<ETSFunctionType *> &merged, std::vector<ETSFunctionType *> &current);
    void MergeSignatures(ETSFunctionType *target, ETSFunctionType *source);
    ir::AstNode *FindAncestorGivenByType(ir::AstNode *node, ir::AstNodeType type, const ir::AstNode *endNode = nullptr);
    util::StringView GetContainingObjectNameFromSignature(Signature *signature);
    bool IsFunctionContainsSignature(ETSFunctionType *funcType, Signature *signature);
    bool CheckFunctionContainsClashingSignature(const ETSFunctionType *funcType, Signature *signature);
    static bool IsReferenceType(const Type *type)
    {
        return type->IsETSReferenceType();
    }
    void ValidatePropertyAccess(varbinder::Variable *var, ETSObjectType *obj, ir::Expression const *expr);
    varbinder::VariableFlags GetAccessFlagFromNode(const ir::AstNode *node);
    Type *CheckSwitchDiscriminant(ir::Expression *discriminant);
    Type *MaybeUnboxInRelation(Type *objectType);
    Type *MaybeUnboxConditionalInRelation(Type *objectType);
    Type *MaybeBoxInRelation(Type *objectType);
    Type *MaybeBoxExpression(ir::Expression *expr);
    Type *MaybeBoxType(Type *type) const;
    Type *MaybeUnboxType(Type *type) const;
    Type const *MaybeBoxType(Type const *type) const;
    Type const *MaybeUnboxType(Type const *type) const;
    void CheckForSameSwitchCases(ArenaVector<ir::SwitchCaseStatement *> const &cases);
    std::string GetStringFromIdentifierValue(checker::Type *caseType) const;
    bool CompareIdentifiersValuesAreDifferent(ir::Expression *compareValue, const std::string &caseValue);
    varbinder::Variable *FindVariableInFunctionScope(
        util::StringView name, const varbinder::ResolveBindingOptions options = varbinder::ResolveBindingOptions::ALL);
    std::pair<varbinder::Variable *, const ETSObjectType *> FindVariableInClassOrEnclosing(
        util::StringView name, const ETSObjectType *classType);
    varbinder::Variable *FindVariableInGlobal(
        const ir::Identifier *identifier,
        const varbinder::ResolveBindingOptions options = varbinder::ResolveBindingOptions::ALL);
    varbinder::Variable *ExtraCheckForResolvedError(ir::Identifier *ident);
    void ValidateResolvedIdentifier(ir::Identifier *ident);
    static bool IsVariableStatic(const varbinder::Variable *var);
    static bool IsVariableGetterSetter(const varbinder::Variable *var);
    static bool IsVariableExtensionAccessor(const varbinder::Variable *var);
    static bool IsVariableOverloadDeclaration(const varbinder::Variable *var);
    bool IsOverloadDeclaration(ir::Expression *expr);
    bool IsSameDeclarationType(varbinder::LocalVariable *target, varbinder::LocalVariable *compare);
    void SaveCapturedVariable(varbinder::Variable *var, ir::Identifier *ident);
    bool SaveCapturedVariableInLocalClass(varbinder::Variable *var, ir::Identifier *ident);
    void CheckUnboxedTypeWidenable(TypeRelation *relation, Type *target, Type *self);
    void CheckUnboxedTypesAssignable(TypeRelation *relation, Type *source, Type *target);
    void CheckBoxedSourceTypeAssignable(TypeRelation *relation, Type *source, Type *target);
    void CheckUnboxedSourceTypeWithWideningAssignable(TypeRelation *relation, Type *source, Type *target);
    void CheckValidGenericTypeParameter(Type *argType, const lexer::SourcePosition &pos);
    void ValidateNamespaceProperty(varbinder::Variable *property, const ETSObjectType *target,
                                   const ir::Identifier *ident);
    void ValidateResolvedProperty(varbinder::LocalVariable **property, const ETSObjectType *target,
                                  const ir::Identifier *ident, PropertySearchFlags flags);
    bool CheckNumberOfTypeArguments(ETSObjectType *type, ir::TSTypeParameterInstantiation *typeArgs,
                                    const lexer::SourcePosition &pos);
    ir::BlockStatement *FindFinalizerOfTryStatement(ir::AstNode *startFrom, const ir::AstNode *p);
    void CheckExceptionClauseType(const std::vector<checker::ETSObjectType *> &exceptions, ir::CatchClause *catchClause,
                                  checker::Type *clauseType);

    void CheckConstructorOverloadDeclaration(ETSChecker *checker, ir::OverloadDeclaration *node) const;
    void CheckFunctionOverloadDeclaration(ETSChecker *checker, ir::OverloadDeclaration *node) const;
    void CheckClassMethodOverloadDeclaration(ETSChecker *checker, ir::OverloadDeclaration *node) const;
    void CheckInterfaceMethodOverloadDeclaration(ETSChecker *checker, ir::OverloadDeclaration *node) const;

    ETSObjectType *GetRelevantArgumentedTypeFromChild(ETSObjectType *child, ETSObjectType *target);
    util::StringView GetHashFromTypeArguments(const ArenaVector<Type *> &typeArgTypes);
    util::StringView GetHashFromFunctionType(ir::ETSFunctionType *type);
    static ETSObjectType *GetOriginalBaseType(Type *object);
    void SetArrayPreferredTypeForNestedMemberExpressions(ir::MemberExpression *expr, Type *annotationType);
    bool IsExtensionETSFunctionType(const checker::Type *type);
    bool IsExtensionAccessorFunctionType(const checker::Type *type);
    bool IsArrayExprSizeValidForTuple(const ir::ArrayExpression *arrayExpr, const ETSTupleType *tuple);
    void ModifyPreferredType(ir::ArrayExpression *arrayExpr, Type *newPreferredType);
    Type *SelectGlobalIntegerTypeForNumeric(Type *type) const;

    ir::ClassProperty *ClassPropToImplementationProp(ir::ClassProperty *classProp, varbinder::ClassScope *scope);
    ir::Expression *GenerateImplicitInstantiateArg(const std::string &className);
    void GenerateGetterSetterBody(ArenaVector<ir::Statement *> &stmts, ArenaVector<ir::Expression *> &params,
                                  ir::ClassProperty *field, varbinder::FunctionParamScope *paramScope, bool isSetter);
    static ir::MethodDefinition *GenerateDefaultGetterSetter(ir::ClassProperty *property, ir::ClassProperty *field,
                                                             varbinder::ClassScope *scope, bool isSetter,
                                                             ETSChecker *checker);
    void GenerateGetterSetterPropertyAndMethod(ir::ClassProperty *originalProp, ETSObjectType *classType);
    void SetupGetterSetterFlags(ir::ClassProperty *originalProp, ETSObjectType *classType, ir::MethodDefinition *getter,
                                ir::MethodDefinition *setter, const bool inExternal);
    Type *GetImportSpecifierObjectType(ir::ETSImportDeclaration *importDecl, ir::Identifier *ident,
                                       std::unordered_set<parser::Program *> *moduleStackCache = nullptr);
    void ImportNamespaceObjectTypeAddReExportType(ir::ETSImportDeclaration *importDecl,
                                                  checker::ETSObjectType *lastObjectType, ir::Identifier *ident,
                                                  std::unordered_set<parser::Program *> *moduleStackCache = nullptr);
    bool CheckValidEqualReferenceType(checker::Type *const leftType, checker::Type *const rightType);
    bool CheckVoidAnnotation(const ir::ETSPrimitiveType *typeAnnotation);
    void ETSObjectTypeDeclNode(ETSChecker *checker, ETSObjectType *const objectType);
    ir::CallExpression *CreateExtensionAccessorCall(ETSChecker *checker, ir::MemberExpression *expr,
                                                    ArenaVector<ir::Expression *> &&args);
    Signature *FindRelativeExtensionGetter(ir::MemberExpression *const expr, ETSFunctionType *funcType);
    Signature *FindRelativeExtensionSetter(ir::MemberExpression *const expr, ETSFunctionType *funcType);
    Type *GetExtensionAccessorReturnType(ir::MemberExpression *expr);
    // Utility type handler functions
    std::optional<ir::TypeNode *> GetUtilityTypeTypeParamNode(const ir::TSTypeParameterInstantiation *typeParams,
                                                              const std::string_view &utilityTypeName);
    Type *HandleUtilityTypeParameterNode(const ir::TSTypeParameterInstantiation *typeParams,
                                         const ir::Identifier *const ident);
    // Partial
    Type *CreatePartialType(Type *typeToBePartial);
    Type *HandlePartialInterface(ir::TSInterfaceDeclaration *interfaceDecl, ETSObjectType *typeToBePartial);

    ir::ClassProperty *CreateNullishProperty(ir::ClassProperty *prop, ir::ClassDefinition *newClassDefinition);
    ir::ClassProperty *CreateNullishProperty(ir::ClassProperty *const prop,
                                             ir::TSInterfaceDeclaration *const newTSInterfaceDefinition);
    ir::MethodDefinition *CreateNullishAccessor(ir::MethodDefinition *const accessor,
                                                ir::TSInterfaceDeclaration *interface);
    ir::ClassProperty *CreateNullishPropertyFromAccessor(ir::MethodDefinition *const accessor,
                                                         ir::ClassDefinition *newClassDefinition);
    void CreatePartialClassDeclaration(ir::ClassDefinition *newClassDefinition, ir::ClassDefinition *classDef);
    ir::ETSTypeReference *BuildSuperPartialTypeReference(Type *superPartialType,
                                                         ir::TSTypeParameterInstantiation *superPartialRefTypeParams);
    ir::TSInterfaceDeclaration *CreateInterfaceProto(util::StringView name, parser::Program *const interfaceDeclProgram,
                                                     const ir::TSInterfaceDeclaration *interfaceDecl);
    ir::TSTypeParameterInstantiation *CreateNewSuperPartialRefTypeParamsDecl(
        ArenaMap<ir::TSTypeParameter *, ir::TSTypeParameter *> *likeSubstitution, const Type *const superPartialType,
        ir::Expression *superRef);
    ir::TSTypeParameterDeclaration *ProcessTypeParamAndGenSubstitution(
        ir::TSTypeParameterDeclaration const *const thisTypeParams,
        ArenaMap<ir::TSTypeParameter *, ir::TSTypeParameter *> *likeSubstitution,
        ir::TSTypeParameterDeclaration *newTypeParams);
    ir::AstNode *CreateGetterOrSetterBodyForOptional(bool isSetter, bool isOptional);
    Type *CreatePartialTypeInterfaceDecl(ir::TSInterfaceDeclaration *const interfaceDecl,
                                         ETSObjectType *const typeToBePartial,
                                         ir::TSInterfaceDeclaration *partialInterface);
    void CreatePartialTypeInterfaceMethods(ir::TSInterfaceDeclaration *const interfaceDecl,
                                           ir::TSInterfaceDeclaration *partialInterface);
    ir::ClassDefinition *CreateClassPrototype(util::StringView name, parser::Program *classDeclProgram);
    varbinder::Variable *SearchNamesInMultiplePrograms(const std::set<const parser::Program *> &programs,
                                                       const std::set<util::StringView> &classNamesToFind);
    std::pair<ir::ScriptFunction *, ir::Identifier *> CreateScriptFunctionForConstructor(
        varbinder::FunctionScope *scope);
    ir::MethodDefinition *CreateNonStaticClassInitializer(varbinder::ClassScope *classScope,
                                                          varbinder::RecordTable *recordTable);
    bool SetPreferredTypeForExpression(ir::Expression *expr, ir::TypeNode *typeAnnotation, ir::Expression *init,
                                       checker::Type *annotationType);
    // Readonly
    Type *GetReadonlyType(Type *type);
    void MakePropertiesReadonly(ETSObjectType *classType);
    // Awaited<T>
    Type *HandleAwaitedUtilityType(Type *typeToBeAwaited);
    Type *HandleAwaitExpression(Type *typeToBeAwaited, ir::AwaitExpression *expr);
    Type *UnwrapPromiseType(checker::Type *type);
    bool IsPromiseType(Type *type);
    // Required
    Type *HandleRequiredType(Type *typeToBeRequired);
    void MakePropertiesNonNullish(ETSObjectType *classType);
    template <PropertyType PROP_TYPE>
    void MakePropertyNonNullish(ETSObjectType *classType, varbinder::LocalVariable *prop);
    void ValidateObjectLiteralForRequiredType(const ETSObjectType *requiredType,
                                              const ir::ObjectExpression *initObjExpr);
    // ReturnType
    Type *HandleReturnTypeUtilityType(Type *baseType);
    void ValidateReturnTypeUtilityType(const Type *typeToValidate, const ir::TSTypeParameterInstantiation *typeParams);

    bool IsStaticInvoke(ir::MemberExpression *const expr);
    void ValidateCallExpressionIdentifier(ir::Identifier *const ident, Type *const type);

    using NamedAccessMeta = std::tuple<ETSObjectType const *, checker::Type const *, const util::StringView>;
    static NamedAccessMeta FormNamedAccessMetadata(varbinder::Variable const *prop);

    // Smart cast support
    [[nodiscard]] checker::Type *ResolveSmartType(checker::Type *sourceType, checker::Type *targetType,
                                                  std::optional<double> value = std::nullopt);
    [[nodiscard]] std::pair<Type *, Type *> CheckTestNullishCondition(Type *testedType, Type *actualType, bool strict);
    [[nodiscard]] std::pair<Type *, Type *> CheckTestObjectCondition(ETSObjectType *testedType, Type *actualType);
    [[nodiscard]] std::pair<Type *, Type *> CheckTestObjectCondition(ETSArrayType *testedType, Type *actualType);

    void ApplySmartCast(varbinder::Variable const *variable, checker::Type *smartType) noexcept;

    bool IsInLocalClass(const ir::AstNode *node) const;
    // Exception
    ETSObjectType *CheckExceptionOrErrorType(checker::Type *type, lexer::SourcePosition pos);

    static Type *TryToInstantiate(Type *type, ArenaAllocator *allocator, TypeRelation *relation,
                                  GlobalTypesHolder *globalTypes);

    // Extension function
    void HandleUpdatedCallExpressionNode(ir::CallExpression *callExpr);
    Signature *FindExtensionSetterInMap(util::StringView name, ETSObjectType *type);
    Signature *FindExtensionGetterInMap(util::StringView name, ETSObjectType *type);
    void InsertExtensionSetterToMap(util::StringView name, ETSObjectType *type, Signature *sig);
    void InsertExtensionGetterToMap(util::StringView name, ETSObjectType *type, Signature *sig);

    // Static invoke
    void CheckInvokeMethodsLegitimacy(ETSObjectType *classType);
    bool IsClassStaticMethod(checker::ETSObjectType *objType, checker::Signature *signature);

    // Covariant and contravariant
    void CheckTypeParameterVariance(ir::ClassDefinition *classDef);
    void CheckTypeParameterVariance(ir::TSInterfaceDeclaration *ifaceDecl);

    checker::Type *CheckArrayElements(ir::ArrayExpression *init);
    void ResolveReturnStatement(ETSChecker *checker, checker::Type *funcReturnType, checker::Type *argumentType,
                                ir::ScriptFunction *containingFunc, ir::ReturnStatement *st);

    std::recursive_mutex *Mutex()
    {
        return &mtx_;
    }

    template <typename T, typename... Args>
    T *AllocNode(Args &&...args)
    {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return util::NodeAllocator::ForceSetParent<T>(Allocator(), std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    T *ProgramAllocNode(Args &&...args)
    {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return util::NodeAllocator::ForceSetParent<T>(ProgramAllocator(), std::forward<Args>(args)...);
    }

    std::vector<ConstraintCheckRecord> &PendingConstraintCheckRecords();
    size_t &ConstraintCheckScopesCount();

    ETSObjectType *GetCachedFunctionalInterface(ir::ETSFunctionType *type);
    void CacheFunctionalInterface(ir::ETSFunctionType *type, ETSObjectType *ifaceType);
    void CollectReturnStatements(ir::AstNode *parent);

    evaluate::ScopedDebugInfoPlugin *GetDebugInfoPlugin();
    const evaluate::ScopedDebugInfoPlugin *GetDebugInfoPlugin() const;

    void SetDebugInfoPlugin(evaluate::ScopedDebugInfoPlugin *debugInfo);

    using ClassBuilder = std::function<void(ArenaVector<ir::AstNode *> &)>;
    using ClassInitializerBuilder =
        std::function<void(ArenaVector<ir::Statement *> *, ArenaVector<ir::Expression *> *)>;

    const varbinder::Variable *GetTargetRef(const ir::MemberExpression *memberExpr);

    void LogUnresolvedReferenceError(ir::Identifier *ident);
    void WrongContextErrorClassifyByType(ir::Identifier *ident);
    Type *CreateSyntheticTypeFromOverload(varbinder::Variable *const var);

    void CreateOverloadSigContainer(Signature *overloadHelperSig)
    {
        if (!overloadSigContainer_.empty()) {
            overloadSigContainer_.pop_back();
        }
        ES2PANDA_ASSERT(overloadSigContainer_.empty());
        overloadSigContainer_.insert(overloadSigContainer_.end(), overloadHelperSig);
    }

    std::vector<Signature *> &GetOverloadSigContainer()
    {
        ES2PANDA_ASSERT(overloadSigContainer_.size() == 1);
        return overloadSigContainer_;
    }

    ObjectInstantiationMap &GetObjectInstantiationMap()
    {
        return objectInstantiationMap_;
    }

    FunctionSignatureMap &GetInvokeToArrowSignatures()
    {
        return invokeToArrowSignatures_;
    }

    FunctionInterfaceMap &GetArrowToFuncInterfaces()
    {
        return arrowToFuncInterfaces_;
    }

    void ClearApparentTypes() noexcept
    {
        apparentTypes_.clear();
    }

    void CleanUp() override
    {
        Checker::CleanUp();
        arrayTypes_.clear();
        pendingConstraintCheckRecords_.clear();
        constraintCheckScopesCount_ = 0;
        globalArraySignatures_.clear();
        unionAssemblerTypes_.clear();
        cachedComputedAbstracts_.clear();
        functionalInterfaceCache_.clear();
        constantBuiltinTypesCache_.clear();
        apparentTypes_.clear();
        elementStack_.clear();
        overloadSigContainer_.clear();
    }

    // This helper finds the intersection of two callSignatures sets
    // The result is stored in callSignatures of newly created
    // ETSFunctionType
    checker::ETSFunctionType *IntersectSignatureSets(const checker::ETSFunctionType *left,
                                                     const checker::ETSFunctionType *right);

    ComputedAbstracts &GetCachedComputedAbstracts()
    {
        return cachedComputedAbstracts_;
    }

private:
    std::pair<const ir::Identifier *, ir::TypeNode *> GetTargetIdentifierAndType(ir::Identifier *ident);
    void NotResolvedError(ir::Identifier *const ident, const varbinder::Variable *classVar,
                          const ETSObjectType *classType);
    void ValidateNewClassInstanceIdentifier(ir::Identifier *const ident);
    void ValidateMemberIdentifier(ir::Identifier *const ident);
    void ValidateAssignmentIdentifier(ir::Identifier *const ident, Type *const type);
    bool ValidateBinaryExpressionIdentifier(ir::Identifier *const ident, Type *const type);
    ETSFunctionType *ResolveAccessorTypeByFlag(ir::MemberExpression *const memberExpr, ETSFunctionType *propType,
                                               ETSFunctionType *funcType, PropertySearchFlags searchFlag);
    std::vector<ResolveResult *> ValidateAccessor(ir::MemberExpression *const memberExpr,
                                                  varbinder::LocalVariable *const oAcc, varbinder::Variable *const eAcc,
                                                  PropertySearchFlags searchFlag);
    void ValidateAccessorIdentifier(ir::Identifier *ident);
    ir::ClassProperty *FindClassProperty(const ETSObjectType *objectType, const ETSFunctionType *propType);
    bool IsInitializedProperty(const ir::ClassDefinition *classDefinition, const ir::ClassProperty *prop);
    bool FindPropertyInAssignment(const ir::AstNode *it, const std::string &targetName);
    void ValidateReadonlyProperty(const ir::MemberExpression *memberExpr, const ETSFunctionType *propType,
                                  lexer::SourcePosition sourcePos);
    std::tuple<bool, bool> IsResolvedAndValue(const ir::Expression *expr, Type *type) const;
    PropertySearchFlags GetSearchFlags(const ir::MemberExpression *memberExpr, const varbinder::Variable *targetRef);
    PropertySearchFlags GetInitialSearchFlags(const ir::Expression *memberExpr);
    Type *GetTypeOfSetterGetter([[maybe_unused]] varbinder::Variable *var);
    SavedCheckerContext CreateSavedCheckerContext(varbinder::Variable *const var);
    bool CheckInit(ir::Identifier *ident, ir::TypeNode *typeAnnotation, ir::Expression *init,
                   checker::Type *annotationType, varbinder::Variable *const bindingVar);
    void CheckItemCasesConstant(ArenaVector<ir::SwitchCaseStatement *> const &cases);

    void CheckAnnotations(const ArenaVector<ir::AnnotationUsage *> &annotations);
    void ReputCheckerDataProgram(ETSChecker *eChecker);

    template <typename EnumType>
    EnumType *CreateEnumTypeFromEnumDeclaration(ir::TSEnumDeclaration const *const enumDecl);

    using Type2TypeMap = std::unordered_map<varbinder::Variable *, varbinder::Variable *>;
    using TypeSet = std::unordered_set<varbinder::Variable *>;
    bool CheckTypeParameterConstraint(ir::TSTypeParameter *param, Type2TypeMap &extends);
    bool CheckDefaultTypeParameter(const ir::TSTypeParameter *param, TypeSet &typeParameterDecls);

    void SetUpTypeParameterConstraint(ir::TSTypeParameter *param);
    void CheckProgram(parser::Program *program, bool runAnalysis = false);
    void CheckWarnings(parser::Program *program, const util::Options &options);

    template <typename... Args>
    ETSObjectType *AsETSObjectType(Type *(GlobalTypesHolder::*typeFunctor)(Args...), Args... args) const;

    // Static invoke
    bool SetStaticInvokeValues(ir::Identifier *const ident, ir::Identifier *classId, ir::Identifier *methodId,
                               varbinder::LocalVariable *instantiateMethod);
    void CreateTransformedCallee(ir::Identifier *ident, ir::Identifier *classId, ir::Identifier *methodId,
                                 ir::CallExpression *callExpr);
    bool TryTransformingToStaticInvoke(ir::Identifier *ident, const Type *resolvedType);

    // Partial
    Type *HandleUnionForPartialType(ETSUnionType *typeToBePartial);
    Type *CreatePartialTypeParameter(ETSTypeParameter *typeToBePartial);
    Type *CreatePartialTypeClass(ETSObjectType *typeToBePartial, ir::AstNode *typeDeclNode);
    Type *CreatePartialTypeClassDef(ir::ClassDefinition *partialClassDef, ir::ClassDefinition *classDef,
                                    ETSObjectType *typeToBePartial, varbinder::RecordTable *recordTableToUse);
    void CreateConstructorForPartialType(ir::ClassDefinition *partialClassDef, checker::ETSObjectType *partialType,
                                         varbinder::RecordTable *recordTable);

    // Check type alias for recursive cases
    bool IsAllowedTypeAliasRecursion(const ir::TSTypeAliasDeclaration *typeAliasNode,
                                     std::unordered_set<const ir::TSTypeAliasDeclaration *> &typeAliases);

    bool IsExceptionOrErrorType(checker::Type *type);

    ArrayMap arrayTypes_;
    std::vector<ConstraintCheckRecord> pendingConstraintCheckRecords_ {};
    ObjectInstantiationMap objectInstantiationMap_;
    TypeInstantiationCacheMap<ETSFunctionType> functionTypeInstantiationMap_;
    TypeInstantiationCacheMap<ETSTupleType> tupleInstantiationCacheMap_;
    TypeInstantiationCacheMap<Type> unionInstantiationCacheMap_;
    FunctionSignatureMap invokeToArrowSignatures_;
    FunctionInterfaceMap arrowToFuncInterfaces_;
    std::unordered_map<Type *, Type *> awaitedTypeCache_;
    size_t constraintCheckScopesCount_ {0};
    GlobalArraySignatureMap globalArraySignatures_;
    std::unordered_set<util::StringView> unionAssemblerTypes_;
    ComputedAbstracts cachedComputedAbstracts_;
    std::unordered_map<Type *, Type *> constantBuiltinTypesCache_;
    FunctionalInterfaceMap functionalInterfaceCache_;
    TypeMapping apparentTypes_;
    std::recursive_mutex mtx_;
    evaluate::ScopedDebugInfoPlugin *debugInfoPlugin_ {nullptr};
    std::unordered_set<ir::TSTypeAliasDeclaration *> elementStack_;
    std::vector<Signature *> overloadSigContainer_;
    std::unordered_set<ETSChecker *> readdedChecker_;
    bool permitRelaxedAny_ {false};
    std::unordered_map<std::string, checker::ETSStringType *> stringLiteralTypes_;
};

}  // namespace ark::es2panda::checker

#endif /* CHECKER_H */
