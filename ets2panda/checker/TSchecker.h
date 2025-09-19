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

#ifndef ES2PANDA_CHECKER_TS_CHECKER_H
#define ES2PANDA_CHECKER_TS_CHECKER_H

#include "checker/checker.h"
#include "varbinder/enumMemberResult.h"
#include "checker/types/globalTypesHolder.h"
#include "checker/types/ts/types.h"
#include "util/enumbitops.h"
#include "util/ustring.h"
#include "util/es2pandaMacros.h"

#include <cstdint>
#include <initializer_list>
#include <unordered_map>
#include <unordered_set>

namespace ark::es2panda::varbinder {
class VarBinder;
class Decl;
class EnumVariable;
class FunctionDecl;
class LocalVariable;
class Scope;
class Variable;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::ir {
class AstNode;
class SpreadElement;
class AssignmentExpression;
class Property;
class Expression;
class ScriptFunction;
class UnaryExpression;
class BinaryExpression;
class Identifier;
class MemberExpression;
class TSEnumDeclaration;
class TSInterfaceDeclaration;
class ObjectExpression;
class TSArrayType;
class TSUnionType;
class TSFunctionType;
class TSConstructorType;
class TSTypeLiteral;
class TSTypeReference;
class TSQualifiedName;
class TSIndexedAccessType;
class TSInterfaceHeritage;
class TSTypeQuery;
class TSTupleType;
class ArrayExpression;
class Statement;
class TSTypeParameterDeclaration;
class TSTypeParameterInstantiation;
class BlockStatement;
class VariableDeclaration;
class IfStatement;
class DoWhileStatement;
class WhileStatement;
class ForUpdateStatement;
class ForInStatement;
class ForOfStatement;
class ReturnStatement;
class SwitchStatement;
class LabelledStatement;
class ThrowStatement;
class TryStatement;
class TSTypeAliasDeclaration;
class TSAsExpression;
class ThisExpression;
class TypeofExpression;
class NewExpression;
class FunctionExpression;
class AwaitExpression;
class UpdateExpression;
class ConditionalExpression;
class YieldExpression;
class ArrowFunctionExpression;
class TemplateLiteral;
class TaggedTemplateExpression;
class TSIndexSignature;
class TSSignatureDeclaration;
class TSPropertySignature;
class TSMethodSignature;
class ChainExpression;
class VariableDeclarator;

enum class AstNodeType : uint8_t;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::checker {

struct ExpressionTypeInfo {
    Type *leftType;
    Type *rightType;
};

struct TupleTypeInfo {
    ElementFlags combinedFlags;
    uint32_t minLength;
    uint32_t fixedLength;
    bool readonly;
};

class TSChecker : public Checker {
public:
    // NOLINTNEXTLINE(readability-redundant-member-init)
    explicit TSChecker([[maybe_unused]] ThreadSafeArenaAllocator *allocator, util::DiagnosticEngine &diagnosticEngine,
                       [[maybe_unused]] ThreadSafeArenaAllocator *programAllocator)
        : Checker(allocator, diagnosticEngine, programAllocator)
    {
    }

    Type *GlobalNumberType()
    {
        return GetGlobalTypesHolder()->GlobalNumberType();
    }

    Type *GlobalAnyType()
    {
        return GetGlobalTypesHolder()->GlobalAnyType();
    }

    Type *GlobalStringType()
    {
        return GetGlobalTypesHolder()->GlobalStringType();
    }

    Type *GlobalBooleanType()
    {
        return GetGlobalTypesHolder()->GlobalBooleanType();
    }

    Type *GlobalVoidType()
    {
        return GetGlobalTypesHolder()->GlobalVoidType();
    }

    Type *GlobalNullType()
    {
        return GetGlobalTypesHolder()->GlobalNullType();
    }

    Type *GlobalUndefinedType()
    {
        return GetGlobalTypesHolder()->GlobalUndefinedType();
    }

    Type *GlobalUnknownType()
    {
        return GetGlobalTypesHolder()->GlobalUnknownType();
    }

    Type *GlobalNeverType()
    {
        return GetGlobalTypesHolder()->GlobalNeverType();
    }

    Type *GlobalNonPrimitiveType()
    {
        return GetGlobalTypesHolder()->GlobalNonPrimitiveType();
    }

    Type *GlobalBigintType()
    {
        return GetGlobalTypesHolder()->GlobalBigintType();
    }

    Type *GlobalFalseType()
    {
        return GetGlobalTypesHolder()->GlobalFalseType();
    }

    Type *GlobalTrueType()
    {
        return GetGlobalTypesHolder()->GlobalTrueType();
    }

    Type *GlobalNumberOrBigintType()
    {
        return GetGlobalTypesHolder()->GlobalNumberOrBigintType();
    }

    Type *GlobalStringOrNumberType()
    {
        return GetGlobalTypesHolder()->GlobalStringOrNumberType();
    }

    Type *GlobalZeroType()
    {
        return GetGlobalTypesHolder()->GlobalZeroType();
    }

    Type *GlobalEmptyStringType()
    {
        return GetGlobalTypesHolder()->GlobalEmptyStringType();
    }

    Type *GlobalZeroBigintType()
    {
        return GetGlobalTypesHolder()->GlobalZeroBigintType();
    }

    Type *GlobalPrimitiveType()
    {
        return GetGlobalTypesHolder()->GlobalPrimitiveType();
    }

    Type *GlobalEmptyTupleType()
    {
        return GetGlobalTypesHolder()->GlobalEmptyTupleType();
    }

    Type *GlobalEmptyObjectType()
    {
        return GetGlobalTypesHolder()->GlobalEmptyObjectType();
    }

    Type *GlobalResolvingReturnType()
    {
        return GetGlobalTypesHolder()->GlobalResolvingReturnType();
    }

    Type *GlobalErrorType()
    {
        return GetGlobalTypesHolder()->GlobalErrorType();
    }

    NumberLiteralPool &NumberLiteralMap()
    {
        return numberLiteralMap_;
    }

    StringLiteralPool &StringLiteralMap()
    {
        return stringLiteralMap_;
    }

    StringLiteralPool &BigintLiteralMap()
    {
        return bigintLiteralMap_;
    }

    bool StartChecker([[maybe_unused]] varbinder::VarBinder *varbinder, const util::Options &options) override;
    Type *CheckTypeCached(ir::Expression *expr) override;

    // Util
    static bool InAssignment(ir::AstNode *node);
    static bool IsAssignmentOperator(lexer::TokenType op);
    static bool IsLiteralType(const Type *type);
    static ir::AstNode *FindAncestorUntilGivenType(ir::AstNode *node, ir::AstNodeType stop);
    static bool MaybeTypeOfKind(const Type *type, TypeFlag flags);
    static bool MaybeTypeOfKind(const Type *type, ObjectType::ObjectTypeKind kind);
    static bool IsConstantMemberAccess(ir::Expression *expr);
    static bool IsStringLike(ir::Expression *expr);
    static ir::MemberExpression *ResolveLeftMostMemberExpression(ir::MemberExpression *expr);

    // Helpers
    void CheckTruthinessOfType(Type *type, lexer::SourcePosition lineInfo);
    Type *CheckNonNullType(Type *type, lexer::SourcePosition lineInfo);
    Type *GetBaseTypeOfLiteralType(Type *type);
    void CheckReferenceExpression(ir::Expression *expr, const char *invalidReferenceMsg,
                                  const char *invalidOptionalChainMsg);
    void CheckTestingKnownTruthyCallableOrAwaitableType(ir::Expression *condExpr, Type *type, ir::AstNode *body);
    Type *ExtractDefinitelyFalsyTypes(Type *type);
    Type *RemoveDefinitelyFalsyTypes(Type *type);
    TypeFlag GetFalsyFlags(Type *type);
    bool IsVariableUsedInConditionBody(ir::AstNode *parent, varbinder::Variable *searchVar);
    bool FindVariableInBinaryExpressionChain(ir::AstNode *parent, varbinder::Variable *searchVar);
    bool IsVariableUsedInBinaryExpressionChain(ir::AstNode *parent, varbinder::Variable *searchVar);
    [[noreturn]] void ThrowTypeError(std::string_view message, const lexer::SourcePosition &pos);
    [[noreturn]] void ThrowTypeError(const util::DiagnosticMessageParams &list, const lexer::SourcePosition &pos);
    [[noreturn]] void ThrowBinaryLikeError(lexer::TokenType op, Type *leftType, Type *rightType,
                                           lexer::SourcePosition lineInfo);
    [[noreturn]] void ThrowAssignmentError(Type *source, Type *target, lexer::SourcePosition lineInfo,
                                           bool isAsSrcLeftType = false);
    void ElaborateElementwise(Type *targetType, ir::Expression *sourceNode, const lexer::SourcePosition &pos);
    void InferSimpleVariableDeclaratorType(ir::VariableDeclarator *declarator);
    void GetTypeVar(varbinder::Decl *decl);
    void GetTypeParam(varbinder::Variable *var, varbinder::Decl *decl);
    void GetTypeEnum(varbinder::Variable *var, varbinder::Decl *decl);
    Type *GetDeclTsType(varbinder::Variable *var, varbinder::Decl *decl);
    Type *GetTypeOfVariable(varbinder::Variable *var) override;
    Type *GetUnaryResultType(Type *operandType);
    Type *GetTypeFromClassOrInterfaceReference(ir::TSTypeReference *node, varbinder::Variable *var);
    Type *GetTypeFromTypeAliasReference(ir::TSTypeReference *node, varbinder::Variable *var);
    Type *GetTypeReferenceType(ir::TSTypeReference *node, varbinder::Variable *var);

    // Type creation
    Type *CreateNumberLiteralType(double value);
    Type *CreateBigintLiteralType(const util::StringView &str, bool negative);
    Type *CreateStringLiteralType(const util::StringView &str);
    Type *CreateFunctionTypeWithSignature(Signature *callSignature);
    Type *CreateConstructorTypeWithSignature(Signature *constructSignature);
    Type *CreateTupleType(ObjectDescriptor *desc, ArenaVector<ElementFlags> &&elementFlags,
                          const TupleTypeInfo &tupleTypeInfo);
    Type *CreateTupleType(ObjectDescriptor *desc, ArenaVector<ElementFlags> &&elementFlags,
                          const TupleTypeInfo &tupleTypeInfo, NamedTupleMemberPool &&namedMembers);
    Type *CreateUnionType(std::initializer_list<Type *> constituentTypes);
    Type *CreateUnionType(ArenaVector<Type *> &&constituentTypes);
    Type *CreateUnionType(ArenaVector<Type *> &constituentTypes);
    Type *CreateObjectTypeWithCallSignature(Signature *callSignature);
    Type *CreateObjectTypeWithConstructSignature(Signature *constructSignature);

    // Object
    void ResolvePropertiesOfObjectType(ObjectType *type, ir::AstNode *member,
                                       std::vector<ir::TSSignatureDeclaration *> &signatureDeclarations,
                                       std::vector<ir::TSIndexSignature *> &indexDeclarations, bool isInterface);
    void ResolveSignaturesOfObjectType(ObjectType *type,
                                       std::vector<ir::TSSignatureDeclaration *> &signatureDeclarations);
    void ResolveIndexInfosOfObjectType(ObjectType *type, std::vector<ir::TSIndexSignature *> &indexDeclarations);
    void ResolveDeclaredMembers(InterfaceType *type);
    bool ValidateInterfaceMemberRedeclaration(ObjectType *type, varbinder::Variable *prop,
                                              const lexer::SourcePosition &locInfo);
    varbinder::Variable *GetPropertyOfType(Type *type, const util::StringView &name, bool getPartial = false,
                                           varbinder::VariableFlags propagateFlags = varbinder::VariableFlags::NONE);
    varbinder::Variable *GetPropertyOfUnionType(UnionType *type, const util::StringView &name, bool getPartial,
                                                varbinder::VariableFlags propagateFlags);
    void CheckIndexConstraints(Type *type);
    void ResolveUnionTypeMembers(UnionType *type);
    void ResolveObjectTypeMembers(ObjectType *type);
    void ResolveInterfaceOrClassTypeMembers(InterfaceType *type);
    Type *CheckComputedPropertyName(ir::Expression *key);
    Type *GetPropertyTypeForIndexType(Type *type, Type *indexType);
    IndexInfo *GetApplicableIndexInfo(Type *type, Type *indexType);
    ArenaVector<ObjectType *> GetBaseTypes(InterfaceType *type);
    void ResolveStructuredTypeMembers(Type *type) override;

    // Function
    Type *HandleFunctionReturn(ir::ScriptFunction *func);
    void CheckFunctionParameterDeclarations(const ArenaVector<ir::Expression *> &params, SignatureInfo *signatureInfo);
    std::tuple<varbinder::LocalVariable *, varbinder::LocalVariable *, bool> CheckFunctionParameter(
        ir::Expression *param, SignatureInfo *signatureInfo);
    std::tuple<varbinder::LocalVariable *, varbinder::LocalVariable *, bool> CheckFunctionIdentifierParameter(
        ir::Identifier *param);
    std::tuple<varbinder::LocalVariable *, varbinder::LocalVariable *, bool> CheckFunctionAssignmentPatternParameter(
        ir::AssignmentExpression *param);
    std::tuple<varbinder::LocalVariable *, varbinder::LocalVariable *, bool> CheckFunctionRestParameter(
        ir::SpreadElement *param, SignatureInfo *signatureInfo);
    std::tuple<varbinder::LocalVariable *, varbinder::LocalVariable *, bool> CheckFunctionArrayPatternParameter(
        ir::ArrayExpression *param);
    std::tuple<varbinder::LocalVariable *, varbinder::LocalVariable *, bool> CheckFunctionObjectPatternParameter(
        ir::ObjectExpression *param);
    void ValidateSubsequentNode(const ir::Statement *const subsequentNode, const ir::ScriptFunction *const func);
    void CheckOverloadSignatureCompatibility(Signature *bodyCallSignature, Signature *signature);
    void InferFunctionDeclarationType(const varbinder::FunctionDecl *decl, varbinder::Variable *funcVar);
    void CollectTypesFromReturnStatements(ir::AstNode *parent, ArenaVector<Type *> *returnTypes);
    void CheckAllCodePathsInNonVoidFunctionReturnOrThrow(ir::ScriptFunction *func, lexer::SourcePosition lineInfo,
                                                         const char *errMsg);
    void CreatePatternParameterName(ir::AstNode *node, std::stringstream &ss);
    void HandlePropertyPatternParameterName(ir::Property *prop, std::stringstream &ss);
    void ThrowReturnTypeCircularityError(ir::ScriptFunction *func);
    ArgRange GetArgRange(const ArenaVector<Signature *> &signatures, std::vector<Signature *> *potentialSignatures,
                         uint32_t callArgsSize, bool *haveSignatureWithRest);
    bool CallMatchesSignature(const ArenaVector<ir::Expression *> &args, Signature *signature, bool throwError);
    Type *ResolveCallOrNewExpression(const ArenaVector<Signature *> &signatures,
                                     ArenaVector<ir::Expression *> arguments, const lexer::SourcePosition &errPos);
    Type *CreateParameterTypeForArrayAssignmentPattern(ir::ArrayExpression *arrayPattern, Type *inferredType);
    Type *CreateParameterTypeForObjectAssignmentPattern(ir::ObjectExpression *objectPattern, Type *inferredType);

    // Binary like expression
    Type *CheckBinaryOperator(ExpressionTypeInfo *leftRightType, ir::Expression *leftExpr, ir::Expression *rightExpr,
                              ir::AstNode *expr, lexer::TokenType op);
    Type *CheckPlusOperator(ExpressionTypeInfo *leftRightType, ir::Expression *leftExpr, ir::Expression *rightExpr,
                            ir::AstNode *expr, lexer::TokenType op);
    Type *CheckCompareOperator(ExpressionTypeInfo *leftRightType, ir::Expression *leftExpr, ir::Expression *rightExpr,
                               ir::AstNode *expr, lexer::TokenType op);
    Type *CheckAndOperator(Type *leftType, Type *rightType, ir::Expression *leftExpr);
    Type *CheckOrOperator(Type *leftType, Type *rightType, ir::Expression *leftExpr);
    Type *CheckInstanceofExpression(Type *leftType, Type *rightType, ir::Expression *rightExpr, ir::AstNode *expr);
    Type *CheckInExpression(Type *leftType, Type *rightType, ir::Expression *leftExpr, ir::Expression *rightExpr,
                            ir::AstNode *expr);
    void CheckAssignmentOperator(lexer::TokenType op, ir::Expression *leftExpr, Type *leftType, Type *valueType);

private:
    NumberLiteralPool numberLiteralMap_;
    StringLiteralPool stringLiteralMap_;
    StringLiteralPool bigintLiteralMap_;

    // Binary like expression
    void CheckBooleanLikeType(Type *leftType, Type *rightType, ir::AstNode *expr, lexer::TokenType op);

    void CheckExtendsBases(ObjectType *&baseObj, InterfaceType *&type, varbinder::InterfaceDecl *&decl);
};

}  // namespace ark::es2panda::checker

#endif /* CHECKER_H */
