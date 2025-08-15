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

#include "ETSAnalyzer.h"

#include "checker/ETSchecker.h"
#include "compiler/lowering/util.h"
#include "generated/diagnostic.h"
#include "checker/types/globalTypesHolder.h"
#include "checker/types/ets/etsTupleType.h"
#include "checker/types/gradualType.h"
#include "evaluate/scopedDebugInfoPlugin.h"
#include "types/signature.h"
#include "compiler/lowering/ets/setJumpTarget.h"
#include "checker/types/ets/etsAsyncFuncReturnType.h"
#include "types/type.h"
#include "checker/types/typeError.h"

#include <unordered_set>

namespace ark::es2panda::checker {

static Type *GetAppropriatePreferredType(Type *originalType, std::function<bool(Type *)> const &predicate);

ETSChecker *ETSAnalyzer::GetETSChecker() const
{
    return static_cast<ETSChecker *>(GetChecker());
}

// from base folder
checker::Type *ETSAnalyzer::Check(ir::CatchClause *st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::Type *exceptionType = checker->GlobalTypeError();

    if (st->Param() != nullptr) {
        ES2PANDA_ASSERT(st->Param()->IsIdentifier());

        ir::Identifier *paramIdent = st->Param()->AsIdentifier();
        if (!paramIdent->IsErrorPlaceHolder()) {
            if (paramIdent->TypeAnnotation() != nullptr) {
                checker::Type *catchParamAnnotationType = paramIdent->TypeAnnotation()->GetType(checker);
                exceptionType = checker->CheckExceptionOrErrorType(catchParamAnnotationType, st->Param()->Start());
            } else {
                exceptionType = checker->GlobalETSObjectType();
            }
            paramIdent->Variable()->SetTsType(exceptionType);
        }
        paramIdent->SetTsType(exceptionType);
    } else {
        ES2PANDA_ASSERT(checker->IsAnyError());
    }

    const varbinder::Variable *catchVar = nullptr;
    if (st->Param() != nullptr && st->Param()->IsIdentifier()) {
        catchVar = st->Param()->AsIdentifier()->Variable();
        ES2PANDA_ASSERT(catchVar != nullptr);
        catchParamStack_.push_back(catchVar);
    }

    st->Body()->Check(checker);

    if (catchVar != nullptr) {
        catchParamStack_.pop_back();
    }

    return st->SetTsType(exceptionType);
}

checker::Type *ETSAnalyzer::Check(ir::ClassDefinition *node) const
{
    ETSChecker *checker = GetETSChecker();

    if (node->TsType() == nullptr) {
        checker->BuildBasicClassProperties(node);
    }

    if (!node->IsClassDefinitionChecked()) {
        checker->CheckClassDefinition(node);
    }

    return node->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::ClassProperty *st) const
{
    if (st->TsType() != nullptr) {
        return st->TsType();
    }

    ES2PANDA_ASSERT(st->Id() != nullptr);

    ETSChecker *checker = GetETSChecker();

    if (st->Id()->Variable() == nullptr) {
        // Now invalid or dummy nodes obtaining after parsing don't have associated variables at all, that leads to
        // incorrect AST and multiple reported errors in AST verifier. Need to create and bind [special]? variables for
        // them with default TypeError set[?]. Why can't we directly check the 'Id'? During the process of
        // resolveIdentifier, we might obtain the wrong variable, which breaks the consistency between the variable and
        // its tsType. see wrong_variable_binding.ets for more details.
        auto ident = st->Id();
        auto [decl, var] = checker->VarBinder()->NewVarDecl<varbinder::LetDecl>(
            ident->Start(), compiler::GenName(checker->ProgramAllocator()).View());
        var->SetScope(checker->VarBinder()->GetScope());
        ident->SetVariable(var);
        decl->BindNode(ident);
        ident->SetTsType(var->SetTsType(checker->GlobalTypeError()));
    }

    ES2PANDA_ASSERT(st->Id()->Variable() != nullptr);

    checker->CheckAnnotations(st->Annotations());
    if (st->TypeAnnotation() != nullptr) {
        st->TypeAnnotation()->Check(checker);
    }

    checker::SavedCheckerContext savedContext(checker, checker->Context().Status(),
                                              checker->Context().ContainingClass(),
                                              checker->Context().ContainingSignature());

    if (st->IsStatic()) {
        checker->AddStatus(checker::CheckerStatus::IN_STATIC_CONTEXT);
    }

    checker::Type *propertyType =
        checker->CheckVariableDeclaration(st->Id(), st->TypeAnnotation(), st->Value(), st->Modifiers());

    propertyType = propertyType != nullptr ? propertyType : checker->GlobalTypeError();
    st->SetTsType(propertyType);
    if (st->IsDefinite() && st->TsType()->PossiblyETSNullish()) {
        checker->LogError(diagnostic::LATE_INITIALIZATION_FIELD_HAS_INVALID_TYPE, st->TypeAnnotation()->Start());
    }

    return propertyType;
}

checker::Type *ETSAnalyzer::Check(ir::ClassStaticBlock *st) const
{
    ETSChecker *checker = GetETSChecker();

    if (checker->HasStatus(checker::CheckerStatus::INNER_CLASS)) {
        checker->LogError(diagnostic::STATIC_INIT_IN_NESTED_CLASS, {}, st->Start());
        st->SetTsType(checker->GlobalTypeError());
        return st->TsType();
    }

    auto *func = st->Function();
    checker->BuildFunctionSignature(func);

    if (func->Signature() == nullptr) {
        st->SetTsType(checker->GlobalTypeError());
    } else {
        st->SetTsType(checker->BuildMethodType(func));
    }

    if (!func->HasBody() || (func->IsExternal() && !func->IsExternalOverload())) {
        return st->TsType();
    }

    checker::ScopeContext scopeCtx(checker, func->Scope());
    checker::SavedCheckerContext savedContext(checker, checker->Context().Status(),
                                              checker->Context().ContainingClass());
    checker->AddStatus(checker::CheckerStatus::IN_STATIC_BLOCK | checker::CheckerStatus::IN_STATIC_CONTEXT);
    func->Body()->Check(checker);
    return st->TsType();
}

// Satisfy the Chinese code checker
static void HandleNativeAndAsyncMethods(ETSChecker *checker, ir::MethodDefinition *node)
{
    auto *scriptFunc = node->Function();
    ES2PANDA_ASSERT(scriptFunc != nullptr);

    if (util::Helpers::IsAsyncMethod(node)) {
        if (scriptFunc->ReturnTypeAnnotation() != nullptr && scriptFunc->Signature() != nullptr) {
            auto *asyncFuncReturnType = scriptFunc->Signature()->ReturnType();

            if (!asyncFuncReturnType->IsETSObjectType() ||
                asyncFuncReturnType->AsETSObjectType()->GetOriginalBaseType() != checker->GlobalBuiltinPromiseType()) {
                checker->LogError(diagnostic::ASYNC_FUNCTION_RETURN_TYPE, {}, scriptFunc->Start());
                scriptFunc->Signature()->SetReturnType(checker->GlobalTypeError());
                return;
            }
        }
    }
}

//  Extacted from 'ETSAnalyzer::Check(ir::MethodDefinition *node)' to reduce its size
static checker::Type *CheckMethodDefinitionHelper(ETSChecker *checker, ir::MethodDefinition *node) noexcept
{
    // NOTE(gogabr): temporary, until we have proper bridges, see #16485
    // Don't check overriding for synthetic functional classes.
    if ((node->Parent()->Modifiers() & ir::ModifierFlags::FUNCTIONAL) == 0) {
        checker->CheckOverride(node->TsType()->AsETSFunctionType()->FindSignature(node->Function()));
    }

    for (auto *overload : node->Overloads()) {
        overload->Check(checker);
    }

    return node->TsType();
}

static bool IsInitializerBlockTransfer(std::string_view str)
{
    auto prefix = compiler::Signatures::INITIALIZER_BLOCK_INIT;
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

checker::Type *ETSAnalyzer::Check(ir::MethodDefinition *node) const
{
    ETSChecker *checker = GetETSChecker();
    auto *scriptFunc = node->Function();

    // CC-OFFNXT(G.FMT.14-CPP) project code style
    auto const returnErrorType = [checker, node]() -> checker::Type * {
        node->SetTsType(checker->GlobalTypeError());
        return node->TsType();
    };

    checker->CheckAnnotations(scriptFunc->Annotations());
    checker->CheckFunctionSignatureAnnotations(scriptFunc->Params(), scriptFunc->TypeParams(),
                                               scriptFunc->ReturnTypeAnnotation());

    if (scriptFunc->IsProxy()) {
        return ReturnTypeForStatement(node);
    }

    ES2PANDA_ASSERT(!(scriptFunc->IsGetter() && scriptFunc->IsSetter()));
    if (scriptFunc->IsGetter() || scriptFunc->IsSetter()) {
        auto status = scriptFunc->IsGetter() ? CheckerStatus::IN_GETTER : CheckerStatus::IN_SETTER;
        checker->AddStatus(status);
    }

    // NOTE: aszilagyi. make it correctly check for open function not have body
    if (!scriptFunc->HasBody() && !(node->IsAbstract() || node->IsNative() || node->IsDeclare() ||
                                    checker->HasStatus(checker::CheckerStatus::IN_INTERFACE))) {
        checker->LogError(diagnostic::FUNCTION_WITHOUT_BODY, {}, scriptFunc->Start());
        return returnErrorType();
    }

    if (CheckReturnTypeNecessity(node) && scriptFunc->ReturnTypeAnnotation() == nullptr) {
        checker->LogError(diagnostic::MISSING_RETURN_TYPE, {}, scriptFunc->Start());
        return returnErrorType();
    }

    if (node->TsType() == nullptr) {
        node->SetTsType(checker->BuildMethodSignature(node));
    }

    if (IsInitializerBlockTransfer(scriptFunc->Id()->Name().Utf8())) {
        checker->AddStatus(CheckerStatus::IN_STATIC_BLOCK);
    }

    if (node->TsType() != nullptr && node->TsType()->IsTypeError()) {
        return node->TsType();
    }

    this->CheckMethodModifiers(node);
    HandleNativeAndAsyncMethods(checker, node);
    DoBodyTypeChecking(checker, node, scriptFunc);
    CheckPredefinedMethodReturnType(checker, scriptFunc);
    if (node->TsType()->IsTypeError()) {
        return node->TsType();
    }

    return CheckMethodDefinitionHelper(checker, node);
}

void ETSAnalyzer::CheckMethodModifiers(ir::MethodDefinition *node) const
{
    ETSChecker *checker = GetETSChecker();
    auto const notValidInAbstract = ir::ModifierFlags::NATIVE | ir::ModifierFlags::PRIVATE |
                                    ir::ModifierFlags::OVERRIDE | ir::ModifierFlags::FINAL | ir::ModifierFlags::STATIC;

    if (node->IsAbstract() && (node->flags_ & notValidInAbstract) != 0U) {
        checker->LogError(diagnostic::ABSTRACT_METHOD_INVALID_MODIFIER, {}, node->Start());
        node->SetTsType(checker->GlobalTypeError());
        return;
    }

    if ((node->IsAbstract() || (!node->Function()->HasBody() && !node->IsNative() && !node->IsDeclare())) &&
        !(checker->HasStatus(checker::CheckerStatus::IN_ABSTRACT) ||
          checker->HasStatus(checker::CheckerStatus::IN_INTERFACE))) {
        checker->LogError(diagnostic::ABSTRACT_IN_CONCRETE, {}, node->Start());
        node->SetTsType(checker->GlobalTypeError());
    }

    auto const notValidInFinal = ir::ModifierFlags::ABSTRACT | ir::ModifierFlags::STATIC;

    if (node->IsFinal() && (node->flags_ & notValidInFinal) != 0U) {
        checker->LogError(diagnostic::FINAL_METHOD_INVALID_MODIFIER, {}, node->Start());
        node->SetTsType(checker->GlobalTypeError());
    }

    auto const notValidInStatic = ir::ModifierFlags::ABSTRACT | ir::ModifierFlags::FINAL | ir::ModifierFlags::OVERRIDE;

    if (node->IsStatic() && (node->flags_ & notValidInStatic) != 0U) {
        checker->LogError(diagnostic::STATIC_METHOD_INVALID_MODIFIER, {}, node->Start());
        node->SetTsType(checker->GlobalTypeError());
    }
}

static void CheckDuplicationInOverloadDeclaration(ETSChecker *const checker, ir::OverloadDeclaration *const node)
{
    auto overloadedNameSet = ArenaSet<std::string>(checker->ProgramAllocator()->Adapter());
    for (ir::Expression *const overloadedName : node->OverloadedList()) {
        bool isQualifiedName = true;
        std::function<std::string(ir::Expression *const)> getFullOverloadedName =
            [&isQualifiedName, &getFullOverloadedName](ir::Expression *const expr) -> std::string {
            if (!isQualifiedName) {
                return "";
            }
            if (expr->IsIdentifier()) {
                return expr->AsIdentifier()->Name().Mutf8();
            }
            if (expr->IsMemberExpression()) {
                return getFullOverloadedName(expr->AsMemberExpression()->Object()) + "." +
                       getFullOverloadedName(expr->AsMemberExpression()->Property());
            }
            isQualifiedName = false;
            return "";
        };
        std::string fullOverloadedName = getFullOverloadedName(overloadedName);
        if (!isQualifiedName) {
            continue;
        }
        if (overloadedNameSet.find(fullOverloadedName) != overloadedNameSet.end()) {
            checker->LogError(diagnostic::DUPLICATE_OVERLOADED_NAME, overloadedName->Start());
            continue;
        }
        overloadedNameSet.insert(fullOverloadedName);
    }
}

static void CheckOverloadSameNameMethod(ETSChecker *const checker, ir::OverloadDeclaration *const overloadDecl)
{
    Type *objectType = overloadDecl->Parent()->IsClassDefinition()
                           ? overloadDecl->Parent()->AsClassDefinition()->Check(checker)
                           : overloadDecl->Parent()->Parent()->AsTSInterfaceDeclaration()->Check(checker);
    ES2PANDA_ASSERT(objectType->IsETSObjectType());

    PropertySearchFlags searchFlags = PropertySearchFlags::DISALLOW_SYNTHETIC_METHOD_CREATION |
                                      (overloadDecl->IsStatic() ? PropertySearchFlags::SEARCH_STATIC_METHOD
                                                                : PropertySearchFlags::SEARCH_INSTANCE_METHOD);
    auto *sameNameMethod = objectType->AsETSObjectType()->GetProperty(overloadDecl->Id()->Name(), searchFlags);
    if (sameNameMethod == nullptr) {
        return;
    }

    auto serachName = overloadDecl->Id()->Name().Mutf8();
    auto hasSameNameMethod =
        std::find_if(overloadDecl->OverloadedList().begin(), overloadDecl->OverloadedList().end(),
                     [serachName](ir::Expression *overloadId) {
                         return overloadId->IsIdentifier() && overloadId->AsIdentifier()->Name().Is(serachName);
                     });
    if (hasSameNameMethod == overloadDecl->OverloadedList().end()) {
        checker->LogError(diagnostic::OVERLOAD_SAME_NAME_METHOD, {serachName}, overloadDecl->Start());
    }
}

checker::Type *ETSAnalyzer::Check(ir::OverloadDeclaration *node) const
{
    ETSChecker *checker = GetETSChecker();
    ES2PANDA_ASSERT(node != nullptr);
    ES2PANDA_ASSERT(node->Key());

    CheckDuplicationInOverloadDeclaration(checker, node);
    CheckOverloadSameNameMethod(checker, node);

    if (node->IsConstructorOverloadDeclaration()) {
        ES2PANDA_ASSERT(node->Parent()->IsClassDefinition());
        checker->CheckConstructorOverloadDeclaration(checker, node);
    } else if (node->IsFunctionOverloadDeclaration()) {
        ES2PANDA_ASSERT(
            node->Parent()->IsClassDefinition() &&
            (compiler::HasGlobalClassParent(node) || node->Parent()->AsClassDefinition()->IsNamespaceTransformed()));
        checker->CheckFunctionOverloadDeclaration(checker, node);
    } else if (node->IsClassMethodOverloadDeclaration()) {
        ES2PANDA_ASSERT(node->Parent()->IsClassDefinition());
        checker->CheckClassMethodOverloadDeclaration(checker, node);
    } else if (node->IsInterfaceMethodOverloadDeclaration()) {
        ES2PANDA_ASSERT(node->Parent()->Parent()->IsTSInterfaceDeclaration());
        checker->CheckInterfaceMethodOverloadDeclaration(checker, node);
    }

    return checker->CreateSyntheticTypeFromOverload(node->Id()->Variable());
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::Property *expr) const
{
    ETSChecker *checker = GetETSChecker();
    return checker->GlobalTypeError();
}

checker::Type *ETSAnalyzer::Check(ir::SpreadElement *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *checker = GetETSChecker();
    if (expr->PreferredType() != nullptr) {
        expr->Argument()->SetPreferredType(expr->PreferredType());
    }
    auto type = expr->Argument()->Check(checker);
    Type *exprType = type->MaybeBaseTypeOfGradualType();

    if (exprType->IsETSResizableArrayType()) {
        return expr->SetTsType(exprType->AsETSObjectType()->TypeArguments().front());
    }

    if (!exprType->IsETSArrayType() && !exprType->IsETSTupleType() && !exprType->IsETSReadonlyArrayType()) {
        if (!exprType->IsTypeError()) {
            // Don't duplicate error messages for the same error
            checker->LogError(diagnostic::SPREAD_OF_INVALID_TYPE, {exprType}, expr->Start());
        }
        return checker->InvalidateType(expr);
    }

    checker::Type *const elementType = exprType->IsETSTupleType() ? type : checker->GetElementTypeOfArray(exprType);
    return expr->SetTsType(elementType);
}

checker::Type *ETSAnalyzer::Check(ir::TemplateElement *expr) const
{
    ETSChecker *checker = GetETSChecker();
    expr->SetTsType(checker->CreateETSStringLiteralType(expr->Raw()));
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::ETSClassLiteral *expr) const
{
    ETSChecker *checker = GetETSChecker();
    auto *const literal = expr->Expr();

    checker->LogError(diagnostic::UNSUPPORTED_CLASS_LITERAL, {}, literal->Start());
    expr->SetTsType(checker->GlobalTypeError());
    return expr->TsType();

    auto exprType = literal->Check(checker);
    if (exprType->IsETSVoidType()) {
        checker->LogError(diagnostic::INVALID_DOT_CLASS, {}, literal->Start());
        expr->SetTsType(checker->GlobalTypeError());
        return expr->TsType();
    }

    ArenaVector<checker::Type *> typeArgTypes(checker->ProgramAllocator()->Adapter());
    typeArgTypes.push_back(exprType);  // NOTE: Box it if it's a primitive type

    checker::InstantiationContext ctx(checker, checker->GlobalBuiltinTypeType(), std::move(typeArgTypes),
                                      expr->Range().start);
    expr->SetTsType(ctx.Result());

    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::ETSFunctionType *node) const
{
    if (node->TsType() != nullptr) {
        return node->TsType();
    }
    ETSChecker *checker = GetETSChecker();
    checker->CheckAnnotations(node->Annotations());
    checker->CheckFunctionSignatureAnnotations(node->Params(), node->TypeParams(), node->ReturnType());

    auto *signatureInfo = checker->ComposeSignatureInfo(node->TypeParams(), node->Params());
    if (signatureInfo == nullptr) {
        ES2PANDA_ASSERT(GetChecker()->IsAnyError());
        return node->SetTsType(checker->GlobalTypeError());
    }
    auto *returnType = node->IsExtensionFunction() && node->ReturnType()->IsTSThisType()
                           ? signatureInfo->params.front()->TsType()
                           : checker->ComposeReturnType(node->ReturnType(), node->IsAsync());

    auto *const signature =
        checker->CreateSignature(signatureInfo, returnType, node->Flags(), node->IsExtensionFunction());
    if (signature == nullptr) {  // #23134
        ES2PANDA_ASSERT(GetChecker()->IsAnyError());
        return node->SetTsType(checker->GlobalTypeError());
    }

    signature->SetOwner(checker->Context().ContainingClass());

    return node->SetTsType(checker->CreateETSArrowType(signature));
}

template <typename T, typename = typename std::enable_if_t<std::is_base_of_v<ir::Expression, T>>>
static bool CheckArrayElementType(ETSChecker *checker, T *newArrayInstanceExpr)
{
    ES2PANDA_ASSERT(checker != nullptr);
    ES2PANDA_ASSERT(newArrayInstanceExpr != nullptr);

    checker::Type *elementType = newArrayInstanceExpr->TypeReference()->GetType(checker)->MaybeBaseTypeOfGradualType();
    ES2PANDA_ASSERT(elementType != nullptr);
    if (elementType->IsETSPrimitiveType()) {
        return true;
    }

    if (elementType->IsETSObjectType()) {
        auto *calleeObj = elementType->AsETSObjectType();
        const auto flags = checker::ETSObjectFlags::ABSTRACT | checker::ETSObjectFlags::INTERFACE;
        if (!calleeObj->HasObjectFlag(flags)) {
            // A workaround check for new Interface[...] in test cases
            newArrayInstanceExpr->SetSignature(checker->CollectParameterlessConstructor(
                calleeObj->ConstructSignatures(), newArrayInstanceExpr->Start()));
            checker->ValidateSignatureAccessibility(calleeObj, newArrayInstanceExpr->Signature(),
                                                    newArrayInstanceExpr->Start());
        } else {
            checker->LogError(diagnostic::ABSTRACT_CLASS_AS_ARRAY_ELEMENT_TYPE, {}, newArrayInstanceExpr->Start());
            return false;
        }
    } else {
        if (!checker->Relation()->IsSupertypeOf(elementType, checker->GlobalETSUndefinedType()) &&
            !checker->Relation()->IsIdenticalTo(checker->GetApparentType(elementType), elementType)) {
            checker->LogError(diagnostic::TYPE_PARAMETER_AS_ARRAY_ELEMENT_TYPE, {}, newArrayInstanceExpr->Start());
            return false;
        }
        if (!checker->Relation()->IsSupertypeOf(elementType, checker->GlobalETSUndefinedType())) {
            checker->LogError(diagnostic::NON_SUPERTYPE_OF_UNDEFINED_AS_ARRAY_ELEMENT_TYPE, {},
                              newArrayInstanceExpr->Start());
            return false;
        }
    }
    return true;
}

static bool NeedCreateETSResizableArrayType(ETSChecker *checker, Type *type)
{
    return type == nullptr ||
           checker->Relation()->IsSupertypeOf(type, checker->GetGlobalTypesHolder()->GlobalArrayBuiltinType());
}

checker::Type *ETSAnalyzer::Check(ir::ETSNewArrayInstanceExpression *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *checker = GetETSChecker();

    auto *elementType = expr->TypeReference()->GetType(checker);
    checker->ValidateArrayIndex(expr->Dimension(), true);

    CheckArrayElementType(checker, expr);
    auto *preferredType = GetAppropriatePreferredType(
        expr->PreferredType(), [](Type *tp) -> bool { return tp->IsETSArrayType() || tp->IsETSResizableArrayType(); });

    if (NeedCreateETSResizableArrayType(checker, expr->PreferredType()) || preferredType == nullptr ||
        preferredType->IsETSResizableArrayType()) {
        expr->SetTsType(checker->CreateETSResizableArrayType(elementType));
    } else {
        expr->SetTsType(checker->CreateETSArrayType(elementType));
    }
    if (expr->TsType()->IsETSArrayType()) {
        checker->CreateBuiltinArraySignature(expr->TsType()->AsETSArrayType(), 1);
    }

    return expr->TsType();
}

static checker::Type *CheckInstantiatedNewType(ETSChecker *checker, ir::ETSNewClassInstanceExpression *expr)
{
    checker::Type *res = expr->GetTypeRef()->Check(checker);
    auto calleeType = res->MaybeBaseTypeOfGradualType();
    FORWARD_TYPE_ERROR(checker, calleeType, expr->GetTypeRef());

    if (calleeType->IsETSUnionType()) {
        return checker->TypeError(expr->GetTypeRef(), diagnostic::UNION_NONCONSTRUCTIBLE, expr->Start());
    }
    if (!ir::ETSNewClassInstanceExpression::TypeIsAllowedForInstantiation(calleeType)) {
        return checker->TypeError(expr->GetTypeRef(), diagnostic::CALLEE_NONCONSTRUCTIBLE, {calleeType}, expr->Start());
    }
    if (!calleeType->IsETSObjectType()) {
        return checker->TypeError(expr->GetTypeRef(), diagnostic::EXPR_NONCONSTRUCTIBLE, {}, expr->Start());
    }

    auto calleeObj = calleeType->AsETSObjectType();
    if (calleeObj->HasObjectFlag(checker::ETSObjectFlags::ABSTRACT)) {
        checker->LogError(diagnostic::ABSTRACT_INSTANTIATION, {calleeObj->Name()}, expr->Start());
        return checker->GlobalTypeError();
    }

    if (calleeObj->HasObjectFlag(checker::ETSObjectFlags::INTERFACE)) {
        checker->LogError(diagnostic::INTERFACE_INSTANTIATION, {calleeObj->Name()}, expr->Start());
        return checker->GlobalTypeError();
    }

    if (calleeObj->HasObjectFlag(ETSObjectFlags::REQUIRED) &&
        !expr->HasAstNodeFlags(ir::AstNodeFlags::ALLOW_REQUIRED_INSTANTIATION)) {
        checker->LogError(diagnostic::NONLITERAL_INSTANTIATION, {}, expr->GetTypeRef()->Start());
        return checker->GlobalTypeError();
    }

    return res;
}

checker::Type *ETSAnalyzer::Check(ir::ETSNewClassInstanceExpression *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *checker = GetETSChecker();
    auto *calleeType = CheckInstantiatedNewType(checker, expr);
    FORWARD_TYPE_ERROR(checker, calleeType, expr);

    auto *calleeObj = calleeType->MaybeBaseTypeOfGradualType()->AsETSObjectType();
    expr->SetTsType(calleeType);

    auto *signature = checker->ResolveConstructExpression(calleeObj, expr->GetArguments(), expr->Start());

    if (signature == nullptr) {
        return checker->InvalidateType(expr);
    }

    checker->CheckObjectLiteralArguments(signature, expr->GetArguments());

    checker->ValidateSignatureAccessibility(calleeObj, signature, expr->Start());

    expr->SetSignature(signature);

    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::ETSNewMultiDimArrayInstanceExpression *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }
    ETSChecker *checker = GetETSChecker();

    CheckArrayElementType(checker, expr);
    auto *elementType = expr->TypeReference()->GetType(checker);

    auto *fixedArrayType = elementType;
    for (auto *dim : expr->Dimensions()) {
        checker->ValidateArrayIndex(dim, true);
        fixedArrayType = checker->CreateETSArrayType(fixedArrayType);
    }
    auto *preferredType = GetAppropriatePreferredType(
        expr->PreferredType(), [](Type *tp) -> bool { return tp->IsETSArrayType() || tp->IsETSResizableArrayType(); });

    if (NeedCreateETSResizableArrayType(checker, preferredType) || preferredType->IsETSResizableArrayType()) {
        expr->SetTsType(checker->CreateETSMultiDimResizableArrayType(elementType, expr->Dimensions().size()));
    } else {
        expr->SetTsType(fixedArrayType);
    }

    if (expr->TsType()->IsETSArrayType()) {
        expr->SetSignature(
            checker->CreateBuiltinArraySignature(expr->TsType()->AsETSArrayType(), expr->Dimensions().size()));
    }

    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSPackageDeclaration *st) const
{
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::ETSParameterExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }
    ASSERT_PRINT(expr->Initializer() == nullptr, "default parameter was not lowered");

    if (expr->Ident()->TsType() != nullptr) {
        expr->SetTsType(expr->Ident()->TsType());
    } else if (expr->IsRestParameter()) {
        expr->SetTsType(expr->RestParameter()->Check(checker));
    } else {
        expr->SetTsType(expr->Ident()->Check(checker));
    }
    ES2PANDA_ASSERT(!expr->IsOptional() ||
                    checker->Relation()->IsSupertypeOf(expr->TsType(), checker->GlobalETSUndefinedType()));
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSPrimitiveType *node) const
{
    ETSChecker *checker = GetETSChecker();
    return node->GetType(checker);
}

checker::Type *ETSAnalyzer::Check(ir::ETSStructDeclaration *node) const
{
    ETSChecker *checker = GetETSChecker();
    node->Definition()->Check(checker);
    return ReturnTypeForStatement(node);
}

checker::Type *ETSAnalyzer::Check(ir::ETSTypeReference *node) const
{
    ETSChecker *checker = GetETSChecker();
    checker->CheckAnnotations(node->Annotations());
    return node->GetType(checker);
}

checker::Type *ETSAnalyzer::Check(ir::ETSTypeReferencePart *node) const
{
    ETSChecker *checker = GetETSChecker();
    return node->GetType(checker);
}

checker::Type *ETSAnalyzer::Check(ir::ETSNonNullishTypeNode *node) const
{
    if (node->TsType() != nullptr) {
        return node->TsType();
    }
    ETSChecker *checker = GetETSChecker();
    return node->SetTsType(checker->GetNonNullishType(node->GetTypeNode()->Check(checker)));
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSNullType *node) const
{
    ETSChecker *checker = GetETSChecker();
    checker->CheckAnnotations(node->Annotations());
    return node->SetTsType(checker->GlobalETSNullType());
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSUndefinedType *node) const
{
    ETSChecker *checker = GetETSChecker();
    checker->CheckAnnotations(node->Annotations());
    return node->SetTsType(checker->GlobalETSUndefinedType());
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::ETSNeverType *node) const
{
    ETSChecker *checker = GetETSChecker();
    return checker->GlobalETSNeverType();
}

checker::Type *ETSAnalyzer::Check(ir::ETSStringLiteralType *node) const
{
    ETSChecker *checker = GetETSChecker();
    checker->CheckAnnotations(node->Annotations());
    return node->GetType(checker);
}

checker::Type *ETSAnalyzer::Check(ir::ETSKeyofType *node) const
{
    ETSChecker *checker = GetETSChecker();
    return node->GetType(checker);
}

// compile methods for EXPRESSIONS in alphabetical order

static void AddSpreadElementTypes(ETSChecker *checker, ir::SpreadElement *const element,
                                  ArenaVector<std::pair<Type *, ir::Expression *>> &elementTypes)
{
    Type *const spreadType = element->Check(checker);

    if (spreadType->IsTypeError()) {
        // error recovery
        return;
    }

    Type *const spreadArgumentType = element->Argument()->TsType()->MaybeBaseTypeOfGradualType();

    if (spreadArgumentType->IsETSTupleType()) {
        for (Type *type : spreadArgumentType->AsETSTupleType()->GetTupleTypesList()) {
            elementTypes.emplace_back(type, element);
        }
    } else if (spreadArgumentType->IsETSArrayType()) {
        elementTypes.emplace_back(spreadArgumentType->AsETSArrayType()->ElementType(), element);
    } else {
        ES2PANDA_ASSERT(spreadArgumentType->IsETSResizableArrayType());
        elementTypes.emplace_back(spreadArgumentType->AsETSObjectType()->TypeArguments().front(), element);
    }
}

static bool ValidArrayExprSizeForTupleSize(ETSChecker *checker, Type *possibleTupleType,
                                           ir::Expression *possibleArrayExpr)
{
    if (!possibleArrayExpr->IsArrayExpression() || !possibleTupleType->IsETSTupleType()) {
        return true;
    }

    return checker->IsArrayExprSizeValidForTuple(possibleArrayExpr->AsArrayExpression(),
                                                 possibleTupleType->AsETSTupleType());
}

static ArenaVector<std::pair<Type *, ir::Expression *>> GetElementTypes(ETSChecker *checker, ir::ArrayExpression *expr)
{
    ArenaVector<std::pair<Type *, ir::Expression *>> elementTypes(checker->ProgramAllocator()->Adapter());

    auto *const exprPreferredType = expr->PreferredType();
    auto *const exprTupleType = exprPreferredType->IsETSTupleType() ? exprPreferredType->AsETSTupleType() : nullptr;
    checker::Type *elemPreferredType =
        exprPreferredType->IsETSTupleType() ? nullptr : checker->GetElementTypeOfArray(exprPreferredType);

    for (std::size_t idx = 0U; idx < expr->Elements().size(); ++idx) {
        ir::Expression *const element = expr->Elements()[idx];

        if (element->IsSpreadElement()) {
            element->SetPreferredType(exprPreferredType);
            AddSpreadElementTypes(checker, element->AsSpreadElement(), elementTypes);
            continue;
        }

        if (exprTupleType != nullptr && exprPreferredType->IsETSTupleType()) {
            if (idx >= exprTupleType->GetTupleSize() ||
                !ValidArrayExprSizeForTupleSize(checker, exprTupleType->GetTypeAtIndex(idx), element)) {
                elementTypes.emplace_back(element->SetTsType(checker->GlobalTypeError()), element);
                continue;
            }
            elemPreferredType = exprTupleType->GetTypeAtIndex(idx);
        }

        element->SetPreferredType(elemPreferredType);
        elementTypes.emplace_back(element->Check(checker), element);
    }

    return elementTypes;
}

static Type *GetArrayElementType(ETSChecker *checker, Type *preferredType)
{
    if (preferredType->IsETSArrayType()) {
        return checker->GetNonConstantType(checker->GetElementTypeOfArray(preferredType));
    }
    ES2PANDA_ASSERT(preferredType->IsETSResizableArrayType());
    return preferredType->AsETSResizableArrayType()->ElementType();
}

static bool CheckElement(ETSChecker *checker, Type *const preferredType,
                         ArenaVector<std::pair<Type *, ir::Expression *>> arrayExprElementTypes, std::size_t idx)
{
    auto [elementType, currentElement] = arrayExprElementTypes[idx];

    if (elementType->IsTypeError()) {
        return true;
    }

    Type *targetType = nullptr;

    if (preferredType->IsETSTupleType()) {
        const auto *const tupleType = preferredType->AsETSTupleType();
        if (tupleType->GetTupleSize() != arrayExprElementTypes.size()) {
            return false;
        }

        auto *const compareType = tupleType->GetTypeAtIndex(idx);
        if (compareType == nullptr) {
            checker->LogError(diagnostic::TUPLE_SIZE_MISMATCH, {tupleType->GetTupleSize()}, currentElement->Start());
            return false;
        }

        auto ctx = AssignmentContext(checker->Relation(), currentElement, elementType, compareType,
                                     currentElement->Start(), std::nullopt, TypeRelationFlag::NO_THROW);
        if (!ctx.IsAssignable()) {
            checker->LogError(diagnostic::TUPLE_UNASSIGNABLE_ARRAY, {idx}, currentElement->Start());
            return false;
        }

        const CastingContext castCtx(
            checker->Relation(), diagnostic::CAST_FAIL_UNREACHABLE, {},
            CastingContext::ConstructorData {currentElement, compareType, checker->MaybeBoxType(compareType),
                                             currentElement->Start(), TypeRelationFlag::NO_THROW});

        targetType = compareType;
    } else {
        targetType = GetArrayElementType(checker, preferredType);
    }

    auto ctx = AssignmentContext(checker->Relation(), currentElement, elementType, targetType, currentElement->Start(),
                                 {}, TypeRelationFlag::NO_THROW);
    if (!ctx.IsAssignable()) {
        checker->LogError(diagnostic::ARRAY_ELEMENT_INIT_TYPE_INCOMPAT, {idx, elementType, targetType},
                          currentElement->Start());
        return false;
    }

    return true;
}

static Type *InferPreferredTypeFromElements(ETSChecker *checker, ir::ArrayExpression *arrayExpr)
{
    ArenaVector<Type *> arrayExpressionElementTypes(checker->ProgramAllocator()->Adapter());
    for (auto *const element : arrayExpr->Elements()) {
        auto *elementType = *element->Check(checker);
        if (element->IsSpreadElement() && elementType->IsETSTupleType()) {
            for (auto *typeFromTuple : elementType->AsETSTupleType()->GetTupleTypesList()) {
                arrayExpressionElementTypes.emplace_back(typeFromTuple);
            }

            continue;
        }

        if (element->IsSpreadElement() && elementType->IsETSArrayType()) {
            elementType = elementType->AsETSArrayType()->ElementType();
        }

        arrayExpressionElementTypes.emplace_back(elementType);
    }

    // NOTE (smartin): fix union type normalization. Currently for primitive types like a 'char | char' type, it will be
    // normalized to 'Char'. However it shouldn't be boxed, and be kept as 'char'. For a quick fix, if all types are
    // primitive, then after making the union type, explicitly unbox it.
    if (std::all_of(arrayExpressionElementTypes.begin(), arrayExpressionElementTypes.end(),
                    [](Type *const typeOfElement) { return typeOfElement->IsETSPrimitiveType(); })) {
        return checker->CreateETSResizableArrayType(checker->GetNonConstantType(
            checker->MaybeUnboxType(checker->CreateETSUnionType(std::move(arrayExpressionElementTypes)))));
    }

    // NOTE (smartin): optimize element access on constant array expressions (note is here, because the constant value
    // will be present on the type)
    return checker->CreateETSResizableArrayType(
        checker->GetNonConstantType(checker->CreateETSUnionType(std::move(arrayExpressionElementTypes))));
}

static bool CheckArrayExpressionElements(ETSChecker *checker, ir::ArrayExpression *arrayExpr)
{
    const ArenaVector<std::pair<Type *, ir::Expression *>> arrayExprElementTypes = GetElementTypes(checker, arrayExpr);

    bool allElementsAssignable = !std::any_of(arrayExprElementTypes.begin(), arrayExprElementTypes.end(),
                                              [](auto &pair) { return pair.first->IsTypeError(); });

    for (std::size_t idx = 0; idx < arrayExprElementTypes.size(); ++idx) {
        allElementsAssignable &= CheckElement(checker, arrayExpr->PreferredType(), arrayExprElementTypes, idx);
    }

    return allElementsAssignable;
}

static Type *GetAppropriatePreferredType(Type *originalType, std::function<bool(Type *)> const &predicate)
{
    if (originalType == nullptr) {
        return nullptr;
    }

    while (originalType->IsETSTypeAliasType()) {
        if (predicate(originalType)) {
            return originalType;
        }
        originalType = originalType->AsETSTypeAliasType()->GetTargetType();
    }

    if (predicate(originalType)) {
        return originalType;
    }

    if (!originalType->IsETSUnionType()) {
        return nullptr;
    }

    Type *preferredType = nullptr;
    for (auto &type : originalType->AsETSUnionType()->ConstituentTypes()) {
        if (predicate(type)) {
            if (preferredType != nullptr) {
                return nullptr;  // ambiguity
            }
            preferredType = type;
        }
    }
    return preferredType;
}

checker::Type *ETSAnalyzer::Check(ir::ArrayExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    auto *preferredType = GetAppropriatePreferredType(expr->PreferredType(), &Type::IsAnyETSArrayOrTupleType);

    if (preferredType != nullptr && preferredType->IsETSReadonlyArrayType()) {
        const auto elementType = preferredType->AsETSObjectType()->TypeArguments().front();
        preferredType = checker->CreateETSResizableArrayType(elementType);
    }

    if (!IsArrayExpressionValidInitializerForType(checker, preferredType)) {
        checker->LogError(diagnostic::UNEXPECTED_ARRAY, {expr->PreferredType()}, expr->Start());
        return checker->InvalidateType(expr);
    }

    if (!expr->Elements().empty()) {
        if (preferredType == nullptr || preferredType == checker->GlobalETSObjectType()) {
            preferredType = InferPreferredTypeFromElements(checker, expr);
        }

        expr->SetPreferredType(preferredType);
    }

    if (preferredType == nullptr) {
        return checker->TypeError(expr, diagnostic::UNRESOLVABLE_ARRAY, expr->Start());
    }

    if (!ValidArrayExprSizeForTupleSize(checker, preferredType, expr) ||
        (!expr->Elements().empty() && !CheckArrayExpressionElements(checker, expr))) {
        return checker->InvalidateType(expr);
    }

    expr->SetTsType(preferredType);
    if (!preferredType->IsETSResizableArrayType() && !preferredType->IsETSTupleType()) {
        ES2PANDA_ASSERT(preferredType->IsETSArrayType());
        const auto *const arrayType = preferredType->AsETSArrayType();
        checker->CreateBuiltinArraySignature(arrayType, arrayType->Rank());
    }
    return expr->TsType();
}

void TryInferPreferredType(ir::ArrowFunctionExpression *expr, checker::Type *preferredType, ETSChecker *checker)
{
    if (!preferredType->IsETSUnionType()) {
        if (preferredType->IsETSArrowType() &&
            !preferredType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow().empty()) {
            checker->TryInferTypeForLambdaTypeAlias(expr, preferredType->AsETSFunctionType());
            checker->BuildFunctionSignature(expr->Function(), false);
        }
        return;
    }

    for (auto &ct : preferredType->AsETSUnionType()->ConstituentTypes()) {
        if (!ct->IsETSArrowType() || ct->AsETSFunctionType()->CallSignaturesOfMethodOrArrow().empty()) {
            continue;
        }
        checker->TryInferTypeForLambdaTypeAlias(expr, ct->AsETSFunctionType());
        checker->BuildFunctionSignature(expr->Function(), false);
        if (expr->Function()->Signature() != nullptr) {
            return;
        }
    }
}

checker::Type *ETSAnalyzer::Check(ir::ArrowFunctionExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    checker->CheckAnnotations(expr->Annotations());
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }
    checker::ScopeContext scopeCtx(checker, expr->Function()->Scope());

    if (checker->HasStatus(checker::CheckerStatus::IN_EXTENSION_METHOD) && !expr->Function()->HasReceiver()) {
        /*
        example code:
        ```
            class A {
                prop:number
            }
            function method(this: A) {
                let a = () => {
                    console.log(this.prop)
                }
            }
        ```
        here the enclosing class of arrow function should be Class A
        */
        checker->Context().SetContainingClass(
            checker->Scope()->Find(varbinder::VarBinder::MANDATORY_PARAM_THIS).variable->TsType()->AsETSObjectType());
    }

    auto lambdaSavedSmartCasts = checker->Context().CloneSmartCasts();
    checker::SavedCheckerContext savedContext(checker, checker->Context().Status(),
                                              checker->Context().ContainingClass());

    if (expr->Parent()->IsCallExpression() && !expr->Function()->IsAsyncFunc()) {
        checker->Context().RestoreSmartCasts(lambdaSavedSmartCasts);
    }

    checker->AddStatus(checker::CheckerStatus::IN_LAMBDA);
    checker->Context().SetContainingLambda(expr);

    auto preferredType = expr->GetPreferredType();
    if (preferredType != nullptr) {
        TryInferPreferredType(expr, preferredType, checker);
    } else {
        checker->BuildFunctionSignature(expr->Function(), false);
    }

    if (expr->Function()->Signature() == nullptr) {
        return checker->InvalidateType(expr);
    }

    if (expr->Function()->HasReceiver()) {
        checker->AddStatus(checker::CheckerStatus::IN_EXTENSION_METHOD);
        CheckExtensionMethod(checker, expr->Function(), expr);
    }
    auto *signature = expr->Function()->Signature();

    checker->Context().SetContainingSignature(signature);
    expr->Function()->Body()->Check(checker);

    auto *funcType = checker->CreateETSArrowType(signature);
    checker->Context().SetContainingSignature(nullptr);

    if (expr->Function()->IsAsyncFunc()) {
        auto *retType = signature->ReturnType();
        if (!retType->IsETSObjectType() ||
            retType->AsETSObjectType()->GetOriginalBaseType() != checker->GlobalBuiltinPromiseType()) {
            checker->LogError(diagnostic::ASYNC_DOESNT_PROMISE, {}, expr->Function()->Start());
            expr->SetTsType(checker->GlobalTypeError());
            return expr->TsType();
        }
    }
    expr->SetTsType(funcType);
    return expr->TsType();
}

static bool IsInvalidArrayMemberAssignment(const ir::AssignmentExpression *const expr, ETSChecker *checker)
{
    if (!expr->Left()->IsMemberExpression()) {
        return false;
    }

    const auto *const leftExpr = expr->Left()->AsMemberExpression();
    if (leftExpr->Object()->TsType()->IsETSArrayType() || leftExpr->Object()->TsType()->IsETSTupleType() ||
        leftExpr->Object()->TsType()->IsETSResizableArrayType()) {
        if (leftExpr->Object()->TsType()->IsETSArrayType() && leftExpr->Property()->IsIdentifier() &&
            leftExpr->Property()->AsIdentifier()->Name().Is("length")) {
            checker->LogError(diagnostic::ARRAY_LENGTH_MODIFICATION, {}, expr->Left()->Start());
            return true;
        }

        if (leftExpr->Object()->TsType()->HasTypeFlag(TypeFlag::READONLY)) {
            checker->LogError(diagnostic::READONLY_ARRAYLIKE_MODIFICATION, {}, expr->Left()->Start());
            return true;
        }
    }

    return false;
}

checker::Type *ETSAnalyzer::GetSmartType(ir::AssignmentExpression *expr, checker::Type *leftType,
                                         checker::Type *rightType) const
{
    ETSChecker *checker = GetETSChecker();
    checker::Type *smartType = leftType;

    if (expr->Left()->IsIdentifier() && expr->Target() != nullptr) {
        //  Now try to define the actual type of Identifier so that smart cast can be used in further checker
        //  processing
        auto const value = expr->Right()->IsNumberLiteral()
                               ? std::make_optional(expr->Right()->AsNumberLiteral()->Number().GetDouble())
                               : std::nullopt;
        smartType = checker->ResolveSmartType(rightType, leftType, value);
        auto const *const variable = expr->Target();

        //  Add/Remove/Modify smart cast for identifier
        //  (excluding the variables defined at top-level scope or captured in lambda-functions!)
        auto const *const variableScope = variable->GetScope();
        auto const topLevelVariable =
            variableScope != nullptr && (variableScope->IsGlobalScope() || (variableScope->Parent() != nullptr &&
                                                                            variableScope->Parent()->IsGlobalScope()));
        if (!topLevelVariable) {
            if (checker->Relation()->IsIdenticalTo(leftType, smartType)) {
                checker->Context().RemoveSmartCast(variable);
            } else {
                expr->Left()->SetTsType(smartType);
                checker->Context().SetSmartCast(variable, smartType);
            }
        }
    }
    return smartType;
}

static ir::MethodDefinition const *ResolveMethodDefinition(const ir::Expression *const expression, ETSChecker *checker)
{
    if (!expression->IsMemberExpression()) {
        return nullptr;
    }

    auto const *memberExpression = expression->AsMemberExpression();
    if (memberExpression->Kind() != ir::MemberExpressionKind::PROPERTY_ACCESS ||
        memberExpression->Property() == nullptr || !memberExpression->Property()->IsIdentifier()) {
        return nullptr;
    }

    auto const *variable = memberExpression->Property()->Variable();
    if (variable == nullptr) {
        if (auto const *objectType = memberExpression->Object()->TsType();
            objectType != nullptr && objectType->IsETSObjectType()) {
            // Process possible case of the same name method with receiver defined
            auto resolved = checker->ResolveMemberReference(memberExpression, objectType->AsETSObjectType());
            if (resolved.size() == 2U && resolved[1]->Kind() == checker::ResolvedKind::PROPERTY) {
                variable = resolved[1U]->Variable()->AsLocalVariable();
            }
        }
    }

    if (variable != nullptr) {
        if (variable->Declaration() != nullptr && variable->Declaration()->Node()->IsMethodDefinition()) {
            return variable->Declaration()->Node()->AsMethodDefinition();
        }
    }

    return nullptr;
}

static bool IsInvalidMethodAssignment(const ir::AssignmentExpression *const expr, ETSChecker *checker)
{
    auto left = expr->Left();
    if (auto const *methodDefinition = ResolveMethodDefinition(left, checker); methodDefinition != nullptr) {
        if (!methodDefinition->IsSetter() &&
            std::none_of(methodDefinition->Overloads().cbegin(), methodDefinition->Overloads().cend(),
                         [](const auto *overload) { return overload->IsSetter(); })) {
            checker->LogError(diagnostic::METHOD_ASSIGNMENT, left->Start());
            return true;
        }
    }
    return false;
}

// In assignment expression or object literal, we need the type of the setter instead of the type of the getter
static checker::Type *GetSetterType(varbinder::Variable *const var, ETSChecker *checker)
{
    if (var == nullptr || !checker->IsVariableGetterSetter(var)) {
        return nullptr;
    }

    if (var->TsType()->IsETSFunctionType()) {
        auto *funcType = var->TsType()->AsETSFunctionType();
        if (funcType->HasTypeFlag(checker::TypeFlag::SETTER)) {
            auto *setter = funcType->FindSetter();
            ES2PANDA_ASSERT(setter != nullptr && setter->Params().size() == 1);
            return setter->Params()[0]->TsType();
        }
    }

    return nullptr;
}

// Helper to set the target of assignment expression
bool ETSAnalyzer::SetAssignmentExpressionTarget(ir::AssignmentExpression *const expr, ETSChecker *checker) const
{
    if (expr->Left()->IsIdentifier()) {
        expr->target_ = expr->Left()->AsIdentifier()->Variable();
    } else if (expr->Left()->IsMemberExpression()) {
        if (!expr->IsIgnoreConstAssign() &&
            expr->Left()->AsMemberExpression()->Object()->TsType()->HasTypeFlag(TypeFlag::READONLY)) {
            checker->LogError(diagnostic::READONLY_PROPERTY_REASSIGN, {}, expr->Left()->Start());
        }
        expr->target_ = expr->Left()->AsMemberExpression()->PropVar();
    } else {
        return false;
    }
    return true;
}

checker::Type *ETSAnalyzer::Check(ir::AssignmentExpression *const expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *checker = GetETSChecker();

    if (checker->HasStatus(CheckerStatus::IN_SETTER) && expr->Left()->IsMemberExpression()) {
        checker->WarnForEndlessLoopInGetterSetter(expr->Left()->AsMemberExpression());
    }

    checker::Type *leftType = expr->Left()->Check(checker);

    if (IsInvalidArrayMemberAssignment(expr, checker) || IsInvalidMethodAssignment(expr, checker)) {
        expr->SetTsType(checker->GlobalTypeError());
        return expr->TsType();
    }

    if (!SetAssignmentExpressionTarget(expr, checker)) {
        checker->LogError(diagnostic::ASSIGNMENT_INVALID_LHS, {}, expr->Left()->Start());
        expr->SetTsType(checker->GlobalTypeError());
        return expr->TsType();
    }

    if (expr->target_ != nullptr && !expr->IsIgnoreConstAssign()) {
        checker->ValidateUnaryOperatorOperand(expr->target_, expr);
    }

    checker->InferLambdaInAssignmentExpression(expr);

    if (auto setterType = GetSetterType(expr->target_, checker); setterType != nullptr) {
        leftType = setterType;
        expr->Left()->SetTsType(leftType);
    }

    auto [rightType, relationNode] = CheckAssignmentExprOperatorType(expr, leftType);
    if (rightType->IsTypeError()) {
        return expr->SetTsType(leftType);
    }

    CastPossibleTupleOnRHS(checker, expr);

    checker::Type *smartType = rightType;
    auto isLazyImportObject =
        leftType->MaybeBaseTypeOfGradualType()->IsETSObjectType() &&
        leftType->MaybeBaseTypeOfGradualType()->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::LAZY_IMPORT_OBJECT);
    if (!leftType->IsTypeError() && !isLazyImportObject) {
        if (const auto ctx = checker::AssignmentContext(checker->Relation(), relationNode, rightType, leftType,
                                                        expr->Right()->Start(),
                                                        {{diagnostic::INVALID_ASSIGNMNENT, {rightType, leftType}}});
            ctx.IsAssignable()) {
            smartType = GetSmartType(expr, leftType, rightType);
        }
    }

    return expr->SetTsType(smartType);
}

static checker::Type *HandleSubstitution(ETSChecker *checker, ir::AssignmentExpression *expr, Type *const leftType)
{
    bool possibleInferredTypeOfArray = leftType->IsETSArrayType() || leftType->IsETSResizableArrayType() ||
                                       leftType->IsETSTupleType() || leftType->IsETSUnionType();
    if (expr->Right()->IsArrayExpression() && possibleInferredTypeOfArray) {
        checker->ModifyPreferredType(expr->Right()->AsArrayExpression(), leftType);
    } else if (expr->Right()->IsArrowFunctionExpression() &&
               (leftType->IsETSArrowType() || leftType->IsETSUnionType())) {
        if (auto *preferredType = GetAppropriatePreferredType(leftType, [](Type *tp) { return tp->IsETSArrowType(); });
            preferredType != nullptr) {
            checker->TryInferTypeForLambdaTypeAlias(expr->Right()->AsArrowFunctionExpression(),
                                                    preferredType->AsETSFunctionType());
        }
    } else if (expr->Right()->IsObjectExpression()) {
        if (leftType->IsETSObjectType() && leftType->IsGradualType() &&
            (leftType->HasTypeFlag(TypeFlag::READONLY) ||
             leftType->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::REQUIRED))) {
            checker->LogError(diagnostic::DYMANIC_INIT_WITH_OBJEXPR, {leftType}, expr->Right()->Start());
        }
        expr->Right()->AsObjectExpression()->SetPreferredType(leftType);
    } else {
        expr->Right()->SetPreferredType(leftType);
    }

    return expr->Right()->Check(checker);
}

std::tuple<Type *, ir::Expression *> ETSAnalyzer::CheckAssignmentExprOperatorType(ir::AssignmentExpression *expr,
                                                                                  Type *const leftType) const
{
    ETSChecker *checker = GetETSChecker();
    checker::Type *sourceType {};
    ir::Expression *relationNode = expr->Right();
    switch (expr->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_MULTIPLY_EQUAL:
        case lexer::TokenType::PUNCTUATOR_EXPONENTIATION_EQUAL:
        case lexer::TokenType::PUNCTUATOR_DIVIDE_EQUAL:
        case lexer::TokenType::PUNCTUATOR_MOD_EQUAL:
        case lexer::TokenType::PUNCTUATOR_MINUS_EQUAL:
        case lexer::TokenType::PUNCTUATOR_LEFT_SHIFT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_RIGHT_SHIFT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_UNSIGNED_RIGHT_SHIFT_EQUAL:
        case lexer::TokenType::PUNCTUATOR_BITWISE_AND_EQUAL:
        case lexer::TokenType::PUNCTUATOR_BITWISE_XOR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_BITWISE_OR_EQUAL:
        case lexer::TokenType::PUNCTUATOR_PLUS_EQUAL: {
            std::tie(std::ignore, expr->operationType_) = checker->CheckBinaryOperator(
                expr->Left(), expr->Right(), expr, expr->OperatorType(), expr->Start(), true);
            sourceType = leftType;
            relationNode = expr;
            break;
        }
        case lexer::TokenType::PUNCTUATOR_SUBSTITUTION: {
            sourceType = HandleSubstitution(checker, expr, leftType);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
            break;
        }
    }

    return {sourceType, relationNode};
}

checker::Type *ETSAnalyzer::Check(ir::AwaitExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    expr->SetTsType(checker->HandleAwaitExpression(expr->argument_->Check(checker), expr));
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::BinaryExpression *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *checker = GetETSChecker();

    bool inSmartExpr = false;
    if (!checker->Context().IsInTestExpression()) {
        switch (expr->OperatorType()) {
            case lexer::TokenType::KEYW_INSTANCEOF:
            case lexer::TokenType::PUNCTUATOR_EQUAL:
            case lexer::TokenType::PUNCTUATOR_NOT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL:
            case lexer::TokenType::PUNCTUATOR_LOGICAL_AND:
            case lexer::TokenType::PUNCTUATOR_LOGICAL_OR: {
                inSmartExpr = true;
                SmartCastArray smartCasts = checker->Context().EnterTestExpression();
                break;
            }
            default:
                break;
        }
    }

    auto [newTsType, operationType] =
        checker->CheckBinaryOperator(expr->Left(), expr->Right(), expr, expr->OperatorType(), expr->Start());
    expr->SetTsType(checker->MaybeBoxType(newTsType));
    expr->SetOperationType(checker->MaybeBoxType(operationType));

    checker->Context().CheckBinarySmartCastCondition(expr);

    if (inSmartExpr) {
        checker->Context().ExitTestExpression();
    }

    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::BlockExpression *st) const
{
    if (st->TsType() != nullptr) {
        return st->TsType();
    }

    ETSChecker *checker = GetETSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    // NOLINTNEXTLINE(modernize-loop-convert)
    for (std::size_t idx = 0; idx < st->Statements().size(); idx++) {
        st->Statements()[idx]->Check(checker);
    }

    auto lastStmt = st->Statements().back();
    ES2PANDA_ASSERT(lastStmt->IsExpressionStatement());
    st->SetTsType(lastStmt->AsExpressionStatement()->GetExpression()->TsType());
    return st->TsType();
}

static bool LambdaIsField(ir::CallExpression *expr)
{
    if (!expr->Callee()->IsMemberExpression()) {
        return false;
    }
    auto *me = expr->Callee()->AsMemberExpression();
    return me->PropVar() != nullptr;
}

static bool OverloadDeclaration(ir::Expression *expr)
{
    while (expr->IsMemberExpression()) {
        expr = expr->AsMemberExpression()->Property();
    }

    if (expr->IsIdentifier() && expr->AsIdentifier()->Variable() != nullptr) {
        return expr->AsIdentifier()->Variable()->HasFlag(varbinder::VariableFlags::OVERLOAD);
    }
    return false;
}

checker::Signature *ETSAnalyzer::ResolveSignature(ETSChecker *checker, ir::CallExpression *expr,
                                                  checker::Type *calleeType) const
{
    if (calleeType->IsETSFunctionType() && calleeType->AsETSFunctionType()->HasHelperSignature() &&
        expr->Signature() != nullptr) {
        // Note: Only works when rechecking in DeclareOveloadLowering phase
        auto *helperSignature = calleeType->AsETSFunctionType()->GetHelperSignature();
        checker->LogDiagnostic(diagnostic::DUPLICATE_SIGS, {helperSignature->Function()->Id()->Name(), helperSignature},
                               expr->Start());
        checker->CreateOverloadSigContainer(helperSignature);
        return checker->ResolveCallExpressionAndTrailingLambda(checker->GetOverloadSigContainer(), expr, expr->Start());
    }

    if (calleeType->IsETSFunctionType() && OverloadDeclaration(expr->Callee())) {
        return checker->FirstMatchSignatures(expr, calleeType);
    }

    if (calleeType->IsETSExtensionFuncHelperType()) {
        auto *signature =
            ResolveCallForETSExtensionFuncHelperType(calleeType->AsETSExtensionFuncHelperType(), checker, expr);
        GetChecker()->AsETSChecker()->UpdateDeclarationFromSignature(expr, signature);
        return signature;
    }

    // when a lambda with receiver is a class field or interface property,
    // then it can only be called like a lambda without receiver.
    if (checker->IsExtensionETSFunctionType(calleeType) && !LambdaIsField(expr)) {
        auto *signature = ResolveCallExtensionFunction(calleeType, checker, expr);
        if (signature != nullptr && signature->IsExtensionAccessor() &&
            !checker->HasStatus(CheckerStatus::IN_EXTENSION_ACCESSOR_CHECK)) {
            checker->LogError(diagnostic::EXTENSION_ACCESSOR_INVALID_CALL, {}, expr->Start());
            return nullptr;
        }
        return signature;
    }
    auto &signatures = expr->IsETSConstructorCall() ? calleeType->AsETSObjectType()->ConstructSignatures()
                                                    : calleeType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow();

    return checker->ResolveCallExpressionAndTrailingLambda(signatures, expr, expr->Start());
}

static ETSObjectType *GetCallExpressionCalleeObject(ETSChecker *checker, ir::CallExpression *expr, Type *calleeType)
{
    if (expr->IsETSConstructorCall()) {
        return calleeType->MaybeBaseTypeOfGradualType()->AsETSObjectType();
    }
    auto callee = expr->Callee();
    if (callee->IsMemberExpression()) {
        return callee->AsMemberExpression()->ObjType();
    }
    ES2PANDA_ASSERT(callee->IsIdentifier());
    return checker->Context().ContainingClass();
}

Type *ETSAnalyzer::GetReturnType(ir::CallExpression *expr, Type *calleeType) const
{
    ETSChecker *checker = GetETSChecker();

    if (calleeType->IsTypeError()) {
        return checker->GlobalTypeError();
    }

    if (!calleeType->IsETSFunctionType() && !expr->IsETSConstructorCall() &&
        !calleeType->IsETSExtensionFuncHelperType()) {
        checker->LogError(diagnostic::NO_CALL_SIGNATURE, {calleeType}, expr->Start());
        return checker->GlobalTypeError();
    }

    Signature *const signature = ResolveSignature(checker, expr, calleeType);

    if (signature == nullptr) {
        return checker->GlobalTypeError();
    }

    checker->CheckObjectLiteralArguments(signature, expr->Arguments());

    if (calleeType->IsETSMethodType()) {
        ETSObjectType *calleeObj = GetCallExpressionCalleeObject(checker, expr, calleeType);
        checker->ValidateSignatureAccessibility(calleeObj, signature, expr->Start());
    }

    expr->SetSignature(signature);

    // #22951: this type should not be encoded as a signature flag
    if (signature->HasSignatureFlag(SignatureFlags::THIS_RETURN_TYPE)) {
        return signature->HasSignatureFlag(SignatureFlags::EXTENSION_FUNCTION)
                   ? expr->Arguments()[0]->TsType()
                   : GetCallExpressionCalleeObject(checker, expr, calleeType);
    }
    return signature->ReturnType();
}

static void CheckAbstractCall(ETSChecker *checker, ir::CallExpression *expr)
{
    if (expr->Callee()->IsMemberExpression()) {
        auto obj = expr->Callee()->AsMemberExpression()->Object();
        if (obj != nullptr && obj->IsSuperExpression()) {
            if ((expr->Signature() != nullptr) && (expr->Signature()->HasSignatureFlag(SignatureFlags::ABSTRACT))) {
                checker->LogError(diagnostic::ABSTRACT_CALL, {}, expr->Start());
                expr->SetTsType(checker->GlobalTypeError());
            }
        }
    }
}

static void CheckCallee(ETSChecker *checker, ir::CallExpression *expr)
{
    checker->CheckNonNullish(expr->Callee());
    if (!expr->Callee()->IsMemberExpression()) {
        return;
    }
    auto memberExpr = expr->Callee()->AsMemberExpression();
    if (memberExpr->Object() == nullptr) {
        return;
    }
    auto baseType = memberExpr->Object()->TsType()->MaybeBaseTypeOfGradualType();
    if (baseType->IsETSObjectType() && baseType->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::READONLY)) {
        checker->LogError(diagnostic::READONLY_CALL, {}, expr->Start());
        expr->SetTsType(checker->GlobalTypeError());
    }
}

// Restore CheckerContext of the owner class if we want to perform checking
static checker::SavedCheckerContext ReconstructOwnerClassContext(ETSChecker *checker, ETSObjectType *owner)
{
    if (owner == nullptr) {
        return SavedCheckerContext(checker, CheckerStatus::NO_OPTS, nullptr);
    }
    ES2PANDA_ASSERT(!owner->HasObjectFlag(ETSObjectFlags::ENUM));
    CheckerStatus const status =
        (owner->HasObjectFlag(ETSObjectFlags::CLASS) ? CheckerStatus::IN_CLASS : CheckerStatus::IN_INTERFACE) |
        (owner->HasObjectFlag(ETSObjectFlags::ABSTRACT) ? CheckerStatus::IN_ABSTRACT : CheckerStatus::NO_OPTS) |
        (owner->HasObjectFlag(ETSObjectFlags::INNER) ? CheckerStatus::INNER_CLASS : CheckerStatus::NO_OPTS) |
        (owner->GetDeclNode()->IsClassDefinition() && owner->GetDeclNode()->AsClassDefinition()->IsLocal()
             ? CheckerStatus::IN_LOCAL_CLASS
             : CheckerStatus::NO_OPTS);

    return SavedCheckerContext(checker, status, owner);
}

checker::Type *ETSAnalyzer::GetCallExpressionReturnType(ir::CallExpression *expr, checker::Type *calleeType) const
{
    ETSChecker *checker = GetETSChecker();
    checker::Type *returnType = GetReturnType(expr, calleeType);

    if (returnType->IsTypeError()) {
        return checker->GlobalTypeError();
    }

    auto *const signature = expr->Signature();
    if (signature->RestVar() != nullptr && signature->RestVar()->TsType()->IsETSArrayType()) {
        auto *elementType = signature->RestVar()->TsType()->AsETSArrayType()->ElementType();
        auto *const arrayType = checker->CreateETSArrayType(elementType)->AsETSArrayType();
        checker->CreateBuiltinArraySignature(arrayType, arrayType->Rank());
    }

    if (!signature->HasSignatureFlag(checker::SignatureFlags::NEED_RETURN_TYPE) ||
        (signature->HasSignatureFlag(checker::SignatureFlags::CONSTRUCTOR))) {
        return returnType;
    }

    if (!signature->HasFunction()) {
        return checker->GlobalTypeError();
    }

    auto owner = const_cast<ETSObjectType *>(util::Helpers::GetContainingObjectType(signature->Function()));
    SavedCheckerContext savedCtx(ReconstructOwnerClassContext(checker, owner));

    ir::AstNode *methodDef = signature->Function();
    while (!methodDef->IsMethodDefinition()) {
        methodDef = methodDef->Parent();
        ES2PANDA_ASSERT(methodDef != nullptr);
    }
    ES2PANDA_ASSERT(methodDef->IsMethodDefinition());
    methodDef->Check(checker);

    if (!signature->Function()->HasBody()) {
        return signature->ReturnType();
    }

    if (signature->Function()->IsExternal()) {
        checker->VarBinder()->AsETSBinder()->ResolveReferencesForScopeWithContext(signature->Function()->Body(),
                                                                                  signature->Function()->Scope());
    }
    checker::ScopeContext scopeCtx(checker, signature->Function()->Body()->Scope());
    checker->CollectReturnStatements(signature->Function());
    return signature->ReturnType();
    // NOTE(vpukhov): #14902 substituted signature is not updated
}

static void CheckOverloadCall(ETSChecker *checker, ir::CallExpression *expr)
{
    if (!expr->Callee()->IsMemberExpression() || !OverloadDeclaration(expr->Callee())) {
        return;
    }

    auto *sig = expr->Signature();
    auto *functionNode = sig->OwnerVar()->Declaration()->Node();
    ir::AstNode *parent = functionNode->Parent();

    bool isExported = functionNode->IsExported() || functionNode->IsDefaultExported();
    if (parent != nullptr && parent->IsClassDefinition() && parent->AsClassDefinition()->IsNamespaceTransformed() &&
        !parent->AsClassDefinition()->IsDeclare() && !isExported) {
        checker->LogError(diagnostic::NOT_EXPORTED,
                          {sig->OwnerVar()->Declaration()->Name(), parent->AsClassDefinition()->Ident()->Name()},
                          expr->Start());
    }
}

checker::Type *ETSAnalyzer::Check(ir::CallExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }
    ES2PANDA_ASSERT(!expr->IsOptional());

    auto *oldCallee = expr->Callee();
    checker::Type *calleeType = checker->GetApparentType(expr->Callee()->Check(checker));
    if (calleeType->IsTypeError()) {
        return checker->InvalidateType(expr);
    }

    if (expr->Callee() != oldCallee) {
        // If it is a static invoke, the callee will be transformed from an identifier to a member expression
        // Type check the callee again for member expression
        calleeType = checker->GetApparentType(expr->Callee()->Check(checker));
    }

    CheckCallee(checker, expr);

    checker::TypeStackElement tse(checker, expr, {{diagnostic::CYCLIC_CALLEE, {}}}, expr->Start());
    ERROR_SANITY_CHECK(checker, !tse.HasTypeError(), return expr->SetTsType(checker->GlobalTypeError()));

    checker::Type *const returnType = GetCallExpressionReturnType(expr, calleeType);
    expr->SetTsType(returnType);
    if (returnType->IsTypeError()) {
        return returnType;
    }

    if (calleeType->IsETSArrowType()) {
        expr->SetUncheckedType(checker->GuaranteedTypeForUncheckedCast(
            checker->GlobalETSAnyType(), checker->MaybeBoxType(expr->Signature()->ReturnType())));
    } else {
        expr->SetUncheckedType(checker->GuaranteedTypeForUncheckedCallReturn(expr->Signature()));
    }

    if (expr->UncheckedType() != nullptr) {
        ES2PANDA_ASSERT(expr->UncheckedType()->IsETSReferenceType());
        checker->ComputeApparentType(returnType);
    }

    CheckOverloadCall(checker, expr);
    CheckVoidTypeExpression(checker, expr);
    CheckAbstractCall(checker, expr);
    return expr->TsType();
}

static bool IsNumericType(ETSChecker *checker, Type *type)
{
    return checker->Relation()->IsSupertypeOf(checker->GetGlobalTypesHolder()->GlobalNumericBuiltinType(), type);
}

static Type *BiggerNumericType(ETSChecker *checker, Type *t1, Type *t2)
{
    ES2PANDA_ASSERT(IsNumericType(checker, t1));
    ES2PANDA_ASSERT(IsNumericType(checker, t2));

    auto *rel = checker->Relation();

    if (rel->IsSupertypeOf(checker->GlobalDoubleBuiltinType(), t1) ||
        rel->IsSupertypeOf(checker->GlobalDoubleBuiltinType(), t2)) {
        return checker->GlobalDoubleBuiltinType();
    }
    if (rel->IsSupertypeOf(checker->GlobalFloatBuiltinType(), t1) ||
        rel->IsSupertypeOf(checker->GlobalFloatBuiltinType(), t2)) {
        return checker->GlobalFloatBuiltinType();
    }
    if (rel->IsSupertypeOf(checker->GlobalLongBuiltinType(), t1) ||
        rel->IsSupertypeOf(checker->GlobalLongBuiltinType(), t2)) {
        return checker->GlobalLongBuiltinType();
    }
    if (rel->IsSupertypeOf(checker->GlobalIntBuiltinType(), t1) ||
        rel->IsSupertypeOf(checker->GlobalIntBuiltinType(), t2)) {
        return checker->GlobalIntBuiltinType();
    }
    if (rel->IsSupertypeOf(checker->GlobalShortBuiltinType(), t1) ||
        rel->IsSupertypeOf(checker->GlobalShortBuiltinType(), t2)) {
        return checker->GlobalShortBuiltinType();
    }
    if (rel->IsSupertypeOf(checker->GlobalByteBuiltinType(), t1) ||
        rel->IsSupertypeOf(checker->GlobalByteBuiltinType(), t2)) {
        return checker->GlobalByteBuiltinType();
    }
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check(ir::ConditionalExpression *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *const checker = GetETSChecker();

    SmartCastArray smartCasts = checker->Context().EnterTestExpression();
    checker->CheckTruthinessOfType(expr->Test());
    SmartCastTypes testedTypes = checker->Context().ExitTestExpression();
    if (testedTypes.has_value()) {
        for (auto [variable, consequentType, _] : *testedTypes) {
            checker->ApplySmartCast(variable, consequentType);
        }
    }

    auto *consequent = expr->Consequent();
    Type *consequentType = consequent->Check(checker);

    SmartCastArray consequentSmartCasts = checker->Context().CloneSmartCasts();
    checker->Context().RestoreSmartCasts(smartCasts);

    if (testedTypes.has_value()) {
        for (auto [variable, _, alternateType] : *testedTypes) {
            checker->ApplySmartCast(variable, alternateType);
        }
    }

    auto *alternate = expr->Alternate();
    Type *alternateType = alternate->Check(checker);

    // Here we need to combine types from consequent and alternate if blocks.
    checker->Context().CombineSmartCasts(consequentSmartCasts);

    if (checker->IsTypeIdenticalTo(consequentType, alternateType)) {
        expr->SetTsType(consequentType);
    } else if (IsNumericType(GetETSChecker(), consequentType) && IsNumericType(GetETSChecker(), alternateType)) {
        expr->SetTsType(BiggerNumericType(GetETSChecker(), consequentType, alternateType));
    } else {
        expr->SetTsType(checker->CreateETSUnionType({consequentType, alternateType}));
    }

    // Restore smart casts to initial state.
    checker->Context().RestoreSmartCasts(smartCasts);

    return expr->TsType();
}

// Convert method references to Arrow type if method is used as value
static Type *TransformTypeForMethodReference(ETSChecker *checker, ir::Expression *const use, Type *type)
{
    ES2PANDA_ASSERT(use->IsIdentifier() || use->IsMemberExpression());
    if (!type->IsETSMethodType()) {
        if (use->Parent()->IsCallExpression() && type->IsETSObjectType() && use->IsMemberExpression()) {
            checker->ValidateCallExpressionIdentifier(use->AsMemberExpression()->Property()->AsIdentifier(), type);
        }
        return type;
    }
    auto const getUseSite = [use]() {
        return use->IsIdentifier() ? use->Start() : use->AsMemberExpression()->Property()->Start();
    };

    ir::Expression *expr = use;
    while (expr->Parent()->IsMemberExpression() && expr->Parent()->AsMemberExpression()->Property() == expr) {
        expr = expr->Parent()->AsMemberExpression();
    }
    if (expr->Parent()->IsCallExpression() && expr->Parent()->AsCallExpression()->Callee() == expr) {
        return type;  // type is actually used as method
    }
    if (expr->Parent()->IsOverloadDeclaration()) {
        return type;  // Don't trans overloaded name to arrow type.
    }

    auto *const functionType = type->AsETSFunctionType();
    auto &signatures = functionType->CallSignatures();

    if (signatures.at(0)->HasSignatureFlag(SignatureFlags::PRIVATE)) {
        checker->LogError(diagnostic::PRIVATE_OR_PROTECTED_METHOD_AS_VALUE, {"Private"}, getUseSite());
        return checker->GlobalTypeError();
    }
    if (signatures.at(0)->HasSignatureFlag(SignatureFlags::PROTECTED)) {
        checker->LogError(diagnostic::PRIVATE_OR_PROTECTED_METHOD_AS_VALUE, {"Protected"}, getUseSite());
        return checker->GlobalTypeError();
    }

    auto it = signatures.begin();
    while (it != signatures.end()) {
        if ((*it)->HasSignatureFlag(SignatureFlags::ABSTRACT) &&
            !(*it)->Owner()->GetDeclNode()->IsTSInterfaceDeclaration()) {
            it = signatures.erase(it);
        } else {
            ++it;
        }
    }

    if (signatures.size() > 1U) {
        checker->LogError(diagnostic::OVERLOADED_METHOD_AS_VALUE, getUseSite());
        return checker->GlobalTypeError();
    }
    auto *otherFuncType = functionType->MethodToArrow(checker);
    return otherFuncType == nullptr ? checker->GlobalTypeError() : otherFuncType;
}

checker::Type *ETSAnalyzer::Check(ir::Identifier *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *checker = GetETSChecker();

    auto *identType = TransformTypeForMethodReference(checker, expr, checker->ResolveIdentifier(expr));

    if (expr->TsType() != nullptr && expr->TsType()->IsTypeError()) {
        return expr->TsType();
    }
    ES2PANDA_ASSERT(expr->Variable() != nullptr);
    if (expr->Parent() == nullptr || !expr->Parent()->IsAssignmentExpression() ||
        expr != expr->Parent()->AsAssignmentExpression()->Left()) {
        auto *const smartType = checker->Context().GetSmartCast(expr->Variable());
        if (smartType != nullptr) {
            identType = smartType;
        }
    }

    expr->SetTsType(identType);
    ES2PANDA_ASSERT(identType != nullptr);
    if (!identType->IsTypeError()) {
        checker->Context().CheckIdentifierSmartCastCondition(expr);
    }
    return expr->TsType();
}

std::pair<checker::Type *, util::StringView> SearchReExportsType(ETSObjectType *baseType, ir::MemberExpression *expr,
                                                                 util::StringView const &aliasName, ETSChecker *checker)
{
    std::pair<ETSObjectType *, util::StringView> ret {};

    for (auto *const item : baseType->ReExports()) {
        auto name = item->GetReExportAliasValue(aliasName);
        if (name == aliasName && item->IsReExportHaveAliasValue(name)) {
            continue;
        }

        if (item->GetProperty(name, PropertySearchFlags::SEARCH_ALL) != nullptr) {
            if (ret.first != nullptr) {
                checker->LogError(diagnostic::AMBIGUOUS_REFERENCE, {aliasName}, expr->Start());
                expr->SetTsType(checker->GlobalTypeError());
                return ret;
            }
            ret = {item, name};
        }

        if (auto reExportType = SearchReExportsType(item, expr, name, checker); reExportType.first != nullptr) {
            return reExportType;
        }
    }

    return ret;
}

static void TypeErrorOnMissingProperty(ir::MemberExpression *expr, checker::Type *baseType,
                                       checker::ETSChecker *checker)
{
    std::ignore = checker->TypeError(expr, diagnostic::PROPERTY_NONEXISTENT,
                                     {expr->Property()->AsIdentifier()->Name(), baseType}, expr->Object()->Start());
}

checker::Type *ETSAnalyzer::ResolveMemberExpressionByBaseType(ETSChecker *checker, checker::Type *baseType,
                                                              ir::MemberExpression *expr) const
{
    if (baseType->IsTypeError()) {
        return checker->InvalidateType(expr);
    }

    if (baseType->IsGradualType()) {
        return ResolveMemberExpressionByBaseType(checker, baseType->AsGradualType()->GetBaseType(), expr);
    }

    if (baseType->IsETSArrayType()) {
        if (expr->Property()->AsIdentifier()->Name().Is("length")) {
            return expr->AdjustType(checker, checker->GlobalIntBuiltinType());
        }

        return expr->SetAndAdjustType(checker, checker->GlobalETSObjectType());
    }

    if (baseType->IsETSTupleType()) {
        return expr->SetAndAdjustType(checker, checker->GlobalETSObjectType());
    }

    if (baseType->IsETSFunctionType()) {
        return expr->SetAndAdjustType(checker, checker->GlobalBuiltinFunctionType());
    }

    if (baseType->IsETSObjectType()) {
        checker->ETSObjectTypeDeclNode(checker, baseType->AsETSObjectType());
        return expr->SetTsType(TransformTypeForMethodReference(
            checker, expr, expr->SetAndAdjustType(checker, baseType->AsETSObjectType())));
    }

    if (baseType->IsETSUnionType()) {
        return expr->AdjustType(checker, expr->CheckUnionMember(checker, baseType));
    }

    // NOTE(mshimenkov): temporary workaround to deliver 'primitives refactoring' patch
    // To be removed after complete refactoring
    if (baseType->IsETSPrimitiveType()) {
        static std::array<std::string_view, 7U> castMethods {{
            "toChar",
            "toByte",
            "toShort",
            "toInt",
            "toLong",
            "toFloat",
            "toDouble",
        }};
        auto res = std::find(castMethods.begin(), castMethods.end(), expr->Property()->AsIdentifier()->Name().Utf8());
        if (res != castMethods.end()) {
            auto type = checker->MaybeBoxType(baseType);
            expr->SetAstNodeFlags(ir::AstNodeFlags::TMP_CONVERT_PRIMITIVE_CAST_METHOD_CALL);
            checker->ETSObjectTypeDeclNode(checker, type->AsETSObjectType());
            return expr->SetTsType(TransformTypeForMethodReference(
                checker, expr, expr->SetAndAdjustType(checker, type->AsETSObjectType())));
        }
    }

    TypeErrorOnMissingProperty(expr, baseType, checker);
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::MemberExpression *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }
    ES2PANDA_ASSERT(!expr->IsOptional());
    ETSChecker *checker = GetETSChecker();
    auto *baseType = checker->GetNonConstantType(checker->GetApparentType(expr->Object()->Check(checker)));
    //  Note: don't use possible smart cast to null-like types.
    //        Such situation should be correctly resolved in the subsequent lowering.
    ES2PANDA_ASSERT(baseType != nullptr);
    if (baseType->DefinitelyETSNullish() && expr->Object()->IsIdentifier()) {
        baseType = expr->Object()->AsIdentifier()->Variable()->TsType();
    }

    if (baseType->IsETSObjectType() && !baseType->AsETSObjectType()->ReExports().empty() &&
        baseType->AsETSObjectType()->GetProperty(expr->Property()->AsIdentifier()->Name(),
                                                 PropertySearchFlags::SEARCH_ALL) == nullptr) {
        if (auto reExportType = SearchReExportsType(baseType->AsETSObjectType(), expr,
                                                    expr->Property()->AsIdentifier()->Name(), checker);
            reExportType.first != nullptr) {
            baseType = reExportType.first;
            expr->object_->SetTsType(baseType);
            expr->property_->AsIdentifier()->SetName(reExportType.second);
        }
    }
    if (!checker->CheckNonNullish(expr->Object())) {
        auto *invalidType = checker->HasStatus(checker::CheckerStatus::IN_EXTENSION_ACCESSOR_CHECK)
                                ? checker->GlobalETSUnionUndefinedNull()
                                : checker->InvalidateType(expr);
        return invalidType;
    }

    if (expr->IsComputed()) {
        return expr->AdjustType(checker, expr->CheckComputed(checker, baseType));
    }

    return ResolveMemberExpressionByBaseType(checker, baseType, expr);
}

checker::Type *ETSAnalyzer::CheckDynamic(ir::ObjectExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    for (ir::Expression *propExpr : expr->Properties()) {
        ES2PANDA_ASSERT(propExpr->IsProperty());
        ir::Property *prop = propExpr->AsProperty();
        ir::Expression *value = prop->Value();
        value->Check(checker);
        ES2PANDA_ASSERT(value->TsType());
    }

    expr->SetTsType(expr->PreferredType());
    return expr->PreferredType();
}

static bool ValidatePreferredType(ETSChecker *checker, ir::ObjectExpression *expr)
{
    auto preferredType = expr->PreferredType()->MaybeBaseTypeOfGradualType();
    if (preferredType == nullptr) {
        checker->LogError(diagnostic::CLASS_COMPOSITE_UNKNOWN_TYPE, {}, expr->Start());
        return false;
    }

    if (preferredType->IsTypeError()) {
        //  Don't need to duplicate error message for a single error.
        return false;
    }

    if (!preferredType->IsETSObjectType()) {
        checker->LogError(diagnostic::CLASS_COMPOSITE_INVALID_TARGET, {preferredType}, expr->Start());
        return false;
    }

    return true;
}

static void SetTypeforRecordProperties(const ir::ObjectExpression *expr, checker::ETSObjectType *objType,
                                       ETSChecker *checker)
{
    const auto &recordProperties = expr->Properties();
    auto typeArguments = objType->TypeArguments();
    auto *const valueType = typeArguments[1];  //  Record<K, V>  type arguments

    for (auto *const recordProperty : recordProperties) {
        ir::Expression *recordPropertyExpr = nullptr;
        if (recordProperty->IsProperty()) {
            recordPropertyExpr = recordProperty->AsProperty()->Value();
        } else if (recordProperty->IsSpreadElement()) {
            recordPropertyExpr = recordProperty->AsSpreadElement()->Argument();
        } else if (recordProperty->IsIdentifier() && recordProperty->AsIdentifier()->IsErrorPlaceHolder()) {
            ES2PANDA_ASSERT(checker->IsAnyError());
            continue;
        } else {
            ES2PANDA_UNREACHABLE();
        }

        recordPropertyExpr->SetPreferredType(valueType);
        recordPropertyExpr->Check(checker);
    }
}

// Helper to check for parameterless constructor
static bool HasParameterlessConstructor(checker::ETSObjectType *objType, ETSChecker *checker,
                                        const lexer::SourcePosition &pos)
{
    for (checker::Signature *sig : objType->ConstructSignatures()) {
        if (sig->Params().empty()) {
            checker->ValidateSignatureAccessibility(objType, sig, pos);
            return true;
        }
    }
    return false;
}

// Helper to resolve property name from key expression
static std::optional<util::StringView> GetPropertyNameFromKey(ir::Expression *key)
{
    if (key->IsStringLiteral()) {
        return key->AsStringLiteral()->Str();
    }
    if (key->IsIdentifier()) {
        return key->AsIdentifier()->Name();
    }
    return std::nullopt;  // Indicates invalid key type
}

// Helper to determine property search flags based on object type
static checker::PropertySearchFlags DetermineSearchFlagsForLiteral(checker::ETSObjectType *potentialObjType)
{
    if (potentialObjType->HasObjectFlag(checker::ETSObjectFlags::INTERFACE)) {
        return checker::PropertySearchFlags::SEARCH_INSTANCE_FIELD |
               checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD |
               checker::PropertySearchFlags::SEARCH_INSTANCE_DECL | checker::PropertySearchFlags::SEARCH_IN_INTERFACES;
    }
    return checker::PropertySearchFlags::SEARCH_INSTANCE_FIELD | checker::PropertySearchFlags::SEARCH_IN_BASE |
           checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD;
}

static bool CheckSinglePropertyCompatibility(ir::Expression *propExpr, checker::ETSObjectType *potentialObjType)
{
    if (!propExpr->IsProperty()) {
        return false;  // Not a key-value property
    }
    ir::Expression *key = propExpr->AsProperty()->Key();

    std::optional<util::StringView> optPname = GetPropertyNameFromKey(key);
    if (!optPname.has_value()) {
        return false;  // Invalid key type in literal
    }
    util::StringView pname = optPname.value();

    checker::PropertySearchFlags searchFlags = DetermineSearchFlagsForLiteral(potentialObjType);

    return potentialObjType->GetProperty(pname, searchFlags) != nullptr;
}

static bool CheckObjectLiteralCompatibility(ir::ObjectExpression *expr, checker::ETSObjectType *potentialObjType)
{
    for (ir::Expression *propExpr : expr->Properties()) {
        if (!CheckSinglePropertyCompatibility(propExpr, potentialObjType)) {
            return false;
        }
    }
    return true;  // All properties found
}

// Helper to check if a property type indicates optionality (union with undefined)
static bool IsPropertyTypeOptional(checker::Type *propertyType)
{
    if (!propertyType->IsETSUnionType()) {
        return false;
    }

    auto *unionType = propertyType->AsETSUnionType();
    for (auto *constituentType : unionType->ConstituentTypes()) {
        if (constituentType->IsETSUndefinedType()) {
            return true;
        }
    }
    return false;
}

// Helper to check if a property has a default value in its declaration
static bool HasPropertyDefaultValue(varbinder::LocalVariable *property)
{
    auto *decl = property->Declaration();
    if (decl == nullptr || decl->Node() == nullptr || !decl->Node()->IsClassProperty()) {
        return false;
    }

    auto *classProp = decl->Node()->AsClassProperty();
    return classProp->Value() != nullptr;
}

// Helper to check if a property is optional (flag or declaration)
static bool IsPropertyOptional(varbinder::LocalVariable *property, checker::Type *propertyType)
{
    // Check if property is marked as optional
    if (property->HasFlag(varbinder::VariableFlags::OPTIONAL)) {
        return true;
    }

    // Check if property type includes undefined (indicating optional)
    if (IsPropertyTypeOptional(propertyType)) {
        return true;
    }

    // Check if declaration has optional modifier
    auto *decl = property->Declaration();
    if (decl != nullptr && decl->Node() != nullptr && decl->Node()->IsClassProperty()) {
        auto *classProp = decl->Node()->AsClassProperty();
        if (classProp->IsOptionalDeclaration()) {
            return true;
        }
    }

    return false;
}

// Helper to check if a method property is only getters/setters
static bool IsMethodOnlyAccessors(checker::Type *propertyType)
{
    if (!propertyType->IsETSMethodType()) {
        return false;
    }

    auto methodType = propertyType->AsETSFunctionType();
    for (auto *sig : methodType->CallSignatures()) {
        if (!sig->HasSignatureFlag(checker::SignatureFlags::GETTER) &&
            !sig->HasSignatureFlag(checker::SignatureFlags::SETTER)) {
            // Regular method found
            return false;
        }
    }
    return true;
}

// Helper to check if an interface property is compatible with object literal property
static bool IsInterfacePropertyCompatible(ir::Expression *propExpr, checker::ETSObjectType *interfaceType,
                                          ETSChecker *checker)
{
    if (!propExpr->IsProperty()) {
        return false;
    }

    ir::Expression *key = propExpr->AsProperty()->Key();
    std::optional<util::StringView> optPname = GetPropertyNameFromKey(key);
    if (!optPname.has_value()) {
        return false;
    }
    util::StringView pname = optPname.value();

    // Check if property exists in interface
    varbinder::LocalVariable *property =
        interfaceType->GetProperty(pname, checker::PropertySearchFlags::SEARCH_INSTANCE_FIELD |
                                              checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD |
                                              checker::PropertySearchFlags::SEARCH_INSTANCE_DECL |
                                              checker::PropertySearchFlags::SEARCH_IN_INTERFACES);

    if (property == nullptr) {
        return false;
    }

    auto *propertyType = checker->GetTypeOfVariable(property);

    // If it's a method type, it should only be getter/setter, not regular methods
    if (propertyType->IsETSMethodType()) {
        return IsMethodOnlyAccessors(propertyType);
    }

    return true;
}

// Helper to check if all required interface properties are satisfied
static bool AreAllRequiredInterfacePropertiesSatisfied(ir::ObjectExpression *expr,
                                                       checker::ETSObjectType *interfaceType, ETSChecker *checker)
{
    // Get all properties of the interface using GetAllProperties
    auto allProperties = interfaceType->GetAllProperties();

    // Create a set of property names provided in the object literal
    std::unordered_set<std::string_view> literalProperties;
    for (ir::Expression *propExpr : expr->Properties()) {
        if (propExpr->IsProperty()) {
            ir::Expression *key = propExpr->AsProperty()->Key();
            if (auto optPname = GetPropertyNameFromKey(key); optPname.has_value()) {
                literalProperties.insert(optPname.value().Utf8());
            }
        }
    }

    // Check if all literal properties exist in this interface
    for (const auto &litPropName : literalProperties) {
        bool found = false;
        for (auto *property : allProperties) {
            if (property->Name().Utf8() == litPropName) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    // Check that all required interface properties are satisfied
    for (auto *property : allProperties) {
        auto *propertyType = checker->GetTypeOfVariable(property);

        // Skip method types that aren't getters/setters (they make interface incompatible anyway)
        if (propertyType->IsETSMethodType()) {
            if (!IsMethodOnlyAccessors(propertyType)) {
                // Regular methods not allowed
                return false;
            }
        }
        // Check if this property is provided in the literal
        bool isInLiteral = literalProperties.find(property->Name().Utf8()) != literalProperties.end();
        if (!isInLiteral) {
            // Property not in literal - check if it's optional or has default value
            bool isOptional = IsPropertyOptional(property, propertyType);
            bool hasDefaultValue = HasPropertyDefaultValue(property);
            if (!isOptional && !hasDefaultValue) {
                return false;
            }
        }
    }

    return true;  // All required properties are satisfied
}

static bool IsObjectTypeCompatibleWithLiteral(ETSChecker *checker, ir::ObjectExpression *expr,
                                              checker::ETSObjectType *potentialObjType)
{
    // Split record/map types, class types and interfaces as requested by reviewer

    checker::ETSObjectType *originalBaseType = potentialObjType->GetOriginalBaseType();
    checker::GlobalTypesHolder *globalTypes = checker->GetGlobalTypesHolder();

    // Handle Record/Map types
    if (checker->IsTypeIdenticalTo(originalBaseType, globalTypes->GlobalMapBuiltinType()) ||
        checker->IsTypeIdenticalTo(originalBaseType, globalTypes->GlobalRecordBuiltinType())) {
        // Maps and Records are always compatible with object literals
        return true;
    }

    // Handle interface types
    if (potentialObjType->HasObjectFlag(checker::ETSObjectFlags::INTERFACE)) {
        // For interface types, check that all literal properties exist in the interface
        // and that interface has no regular methods (only getters/setters allowed)

        // For non-empty literals, check that all literal properties exist in interface
        // and all required interface properties are satisfied
        for (ir::Expression *propExpr : expr->Properties()) {
            if (!IsInterfacePropertyCompatible(propExpr, potentialObjType, checker)) {
                return false;
            }
        }

        // Check all required interface properties are satisfied
        return AreAllRequiredInterfacePropertiesSatisfied(expr, potentialObjType, checker);
    }

    // Handle class types
    // For class types, you need to check:
    // - that there is a parameterless constructor, and
    // - that all fields/properties set in the object literal are present in the class

    if (!HasParameterlessConstructor(potentialObjType, checker, expr->Start())) {
        return false;
    }

    // Check that all properties in literal exist in class
    return CheckObjectLiteralCompatibility(expr, potentialObjType);
}

checker::ETSObjectType *ResolveUnionObjectTypeForObjectLiteral(ETSChecker *checker, ir::ObjectExpression *expr,
                                                               checker::ETSUnionType *unionType)
{
    std::vector<checker::ETSObjectType *> candidateObjectTypes;
    // Phase 1: Gather all ETSObjectTypes from the union
    for (auto *constituentType : unionType->ConstituentTypes()) {
        auto type = constituentType->MaybeBaseTypeOfGradualType();
        if (type->IsETSObjectType()) {
            candidateObjectTypes.push_back(type->AsETSObjectType());
        }
    }

    std::vector<checker::ETSObjectType *> matchingObjectTypes;
    // Phase 2: Filter candidates using the helper function
    for (auto *potentialObjType : candidateObjectTypes) {
        if (IsObjectTypeCompatibleWithLiteral(checker, expr, potentialObjType)) {
            matchingObjectTypes.push_back(potentialObjType);
        }
    }

    // Phase 3: Decide based on number of matches
    if (matchingObjectTypes.size() == 1) {
        return matchingObjectTypes.front();
    }
    if (matchingObjectTypes.empty()) {
        // No candidate ETSObjectType from the union matched all properties
        checker->LogError(diagnostic::CLASS_COMPOSITE_INVALID_TARGET, {expr->PreferredType()}, expr->Start());
        return nullptr;
    }
    // Ambiguous
    checker->LogError(diagnostic::AMBIGUOUS_REFERENCE, {expr->PreferredType()->ToString()}, expr->Start());
    return nullptr;
}

static checker::ETSObjectType *ResolveObjectTypeFromPreferredType(ETSChecker *checker, ir::ObjectExpression *expr)
{
    // Assume not null, checked by caller in Check()
    checker::Type *preferredType = expr->PreferredType()->MaybeBaseTypeOfGradualType();

    if (preferredType->IsETSAsyncFuncReturnType()) {
        preferredType = preferredType->AsETSAsyncFuncReturnType()->GetPromiseTypeArg();
    }

    if (preferredType->IsETSUnionType()) {
        return ResolveUnionObjectTypeForObjectLiteral(checker, expr, preferredType->AsETSUnionType());
    }

    if (preferredType->IsETSObjectType()) {
        return preferredType->AsETSObjectType();
    }

    return nullptr;
}

// Helper to handle interface type objects
static checker::Type *HandleInterfaceType(ETSChecker *checker, ir::ObjectExpression *expr,
                                          checker::ETSObjectType *objType)
{
    auto *analyzer = static_cast<checker::ETSAnalyzer *>(checker->GetAnalyzer());
    analyzer->CheckObjectExprProps(
        expr, objType,
        checker::PropertySearchFlags::SEARCH_INSTANCE_FIELD | checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD |
            checker::PropertySearchFlags::SEARCH_INSTANCE_DECL | checker::PropertySearchFlags::SEARCH_IN_INTERFACES);
    expr->SetTsType(objType);
    return objType;
}

// Helper to handle Record/Map types
static checker::Type *HandleRecordOrMapType(ETSChecker *checker, ir::ObjectExpression *expr,
                                            checker::ETSObjectType *objType)
{
    expr->SetTsType(objType);
    SetTypeforRecordProperties(expr, objType, checker);
    return objType;
}

checker::Type *ETSAnalyzer::Check(ir::ObjectExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    if (expr->PreferredType() == nullptr) {
        checker->LogError(diagnostic::CLASS_COMPOSITE_UNKNOWN_TYPE, {}, expr->Start());
        expr->SetTsType(checker->GlobalTypeError());
        return expr->TsType();
    }

    if (!expr->PreferredType()->IsETSUnionType() && !ValidatePreferredType(checker, expr)) {
        expr->SetTsType(checker->GlobalTypeError());
        return expr->TsType();
    }

    checker::ETSObjectType *objType = ResolveObjectTypeFromPreferredType(checker, expr);

    if (objType == nullptr) {
        if (!expr->PreferredType()->IsETSUnionType()) {
            checker->LogError(diagnostic::CLASS_COMPOSITE_INVALID_TARGET, {expr->PreferredType()}, expr->Start());
        }
        expr->SetTsType(checker->GlobalTypeError());
        return expr->TsType();
    }

    if (objType->HasObjectFlag(checker::ETSObjectFlags::INTERFACE)) {
        return HandleInterfaceType(checker, expr, objType);
    }

    checker::ETSObjectType *originalBaseObjType = objType->GetOriginalBaseType();
    checker::GlobalTypesHolder *globalTypes = checker->GetGlobalTypesHolder();
    if (checker->IsTypeIdenticalTo(originalBaseObjType, globalTypes->GlobalMapBuiltinType()) ||
        checker->IsTypeIdenticalTo(originalBaseObjType, globalTypes->GlobalRecordBuiltinType())) {
        return HandleRecordOrMapType(checker, expr, objType);
    }

    // If we reach here, objType is a class. It must have a parameterless constructor
    if (!HasParameterlessConstructor(objType, checker, expr->Start())) {
        expr->SetTsType(checker->TypeError(expr, diagnostic::NO_PARAMLESS_CTOR, {objType->Name()}, expr->Start()));
        return expr->TsType();
    }

    CheckObjectExprProps(expr, objType,
                         checker::PropertySearchFlags::SEARCH_INSTANCE_FIELD |
                             checker::PropertySearchFlags::SEARCH_IN_BASE |
                             checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD);

    expr->SetTsType(objType);
    return objType;
}

void ETSAnalyzer::CollectNonOptionalProperty(const ETSObjectType *objType,
                                             std::unordered_map<util::StringView, ETSObjectType *> &props) const
{
    ETSChecker *checker = GetETSChecker();
    // Note: all the properties of an interface will be lowered as accessor before checker.
    auto const &methodMap = objType->InstanceMethods();
    for (const auto &[propName, var] : methodMap) {
        if (!checker->IsVariableGetterSetter(var)) {
            continue;
        }

        auto propertyType = checker->GetTypeOfVariable(var);
        if (propertyType->IsTypeError()) {
            // Note: error handle later.
            continue;
        }

        if (checker->Relation()->IsSupertypeOf(propertyType, checker->GlobalETSUndefinedType())) {
            // non-optional properties
            continue;
        }
        props.insert({propName, const_cast<ETSObjectType *>(objType)});
    }

    for (auto const *superInterface : objType->Interfaces()) {
        CollectNonOptionalProperty(superInterface, props);
    }
}

void ETSAnalyzer::CheckObjectExprPropsHelper(const ir::ObjectExpression *expr, checker::ETSObjectType *objType,
                                             checker::PropertySearchFlags searchFlags,
                                             std::unordered_map<util::StringView, ETSObjectType *> &properties) const
{
    ETSChecker *checker = GetETSChecker();
    for (ir::Expression *propExpr : expr->Properties()) {
        if (!propExpr->IsProperty()) {
            checker->LogError(diagnostic::OBJECT_LITERAL_NOT_KV, {}, expr->Start());
            return;
        }
        ir::Expression *key = propExpr->AsProperty()->Key();
        ir::Expression *value = propExpr->AsProperty()->Value();

        util::StringView pname;
        if (key->IsStringLiteral()) {
            pname = key->AsStringLiteral()->Str();
        } else if (key->IsIdentifier()) {
            pname = key->AsIdentifier()->Name();
        } else {
            checker->LogError(diagnostic::CLASS_COMPOSITE_INVALID_KEY, {}, expr->Start());
            return;
        }
        varbinder::LocalVariable *lv = objType->GetProperty(pname, searchFlags);
        if (lv == nullptr) {
            checker->LogError(diagnostic::UNDEFINED_PROPERTY, {objType->Name(), pname}, propExpr->Start());
            return;
        }
        checker->ValidatePropertyAccess(lv, objType, propExpr->Start());

        if (key->IsIdentifier()) {
            key->AsIdentifier()->SetVariable(lv);
        }

        auto *propType = checker->GetTypeOfVariable(lv);
        if (propType->IsETSMethodType()) {
            checker->LogError(diagnostic::OBJECT_LITERAL_METHOD_KEY, {}, propExpr->Start());
            return;
        }

        if (auto setterType = GetSetterType(lv, checker); setterType != nullptr) {
            propType = setterType;
        }

        value->SetPreferredType(propType);
        propExpr->SetTsType(propType);
        key->SetTsType(propType);
        value->SetTsType(value->Check(checker));

        checker::AssignmentContext(checker->Relation(), value, value->TsType(), propType, value->Start(),
                                   {{diagnostic::PROP_INCOMPAT, {value->TsType(), propType, pname}}});
        if (properties.find(pname) != properties.end()) {
            properties.erase(pname);
        }
    }
}

void ETSAnalyzer::CheckObjectExprProps(const ir::ObjectExpression *expr,
                                       checker::ETSObjectType *objectTypeForProperties,
                                       checker::PropertySearchFlags searchFlags) const
{
    ETSChecker *checker = GetETSChecker();
    checker::ETSObjectType *objType = objectTypeForProperties;
    if (objType->IsGlobalETSObjectType() && !expr->Properties().empty()) {
        checker->LogError(diagnostic::ERROR_ARKTS_NO_UNTYPED_OBJ_LITERALS, expr->Start());
    }

    std::unordered_map<util::StringView, ETSObjectType *> propertyWithNonOptionalType;
    if (objType->HasObjectFlag(ETSObjectFlags::INTERFACE)) {
        CollectNonOptionalProperty(objType, propertyWithNonOptionalType);
    }

    CheckObjectExprPropsHelper(expr, objType, searchFlags, propertyWithNonOptionalType);

    for (const auto &[propName, ownerType] : propertyWithNonOptionalType) {
        if (objType == ownerType) {
            checker->LogError(diagnostic::OBJECT_LITERAL_NON_OPTIONAL_PROP_LOST, {propName, objType}, expr->Start());
        } else {
            checker->LogError(diagnostic::OBJECT_LITERAL_NON_OPTIONAL_PROP_OF_SUPER_LOST,
                              {propName, ownerType, objType}, expr->Start());
        }
    }

    if (objType->HasObjectFlag(ETSObjectFlags::REQUIRED)) {
        checker->ValidateObjectLiteralForRequiredType(objType, expr);
    }
}

checker::Type *ETSAnalyzer::Check(ir::OpaqueTypeNode *expr) const
{
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::BrokenTypeNode *expr) const
{
    return GetETSChecker()->GlobalTypeError();
}

checker::Type *ETSAnalyzer::Check(ir::SequenceExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    for (auto *it : expr->Sequence()) {
        it->Check(checker);
    }
    ES2PANDA_ASSERT(!expr->Sequence().empty());
    expr->SetTsType(expr->Sequence().back()->TsType());
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::SuperExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    expr->SetTsType(checker->CheckThisOrSuperAccess(expr, checker->Context().ContainingClass()->SuperType(), "super"));
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::TemplateLiteral *expr) const
{
    ETSChecker *checker = GetETSChecker();

    for (auto *it : expr->Expressions()) {
        it->Check(checker);
    }

    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    if (expr->Quasis().size() != expr->Expressions().size() + 1U) {
        checker->LogError(diagnostic::TEMPLATE_COUNT_MISMATCH, {}, expr->Start());
        expr->SetTsType(checker->GlobalTypeError());
        return expr->TsType();
    }

    for (auto *it : expr->Quasis()) {
        it->Check(checker);
    }

    expr->SetTsType(checker->CreateETSStringLiteralType(expr->GetMultilineString()));
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::ThisExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    /*
    example code:
    ```
        class A {
            prop
        }
        function A.method() {
            let a = () => {
                console.println(this.prop)
            }
        }
        is identical to
        function method(this: A) {
            let a = () => {
                console.println(this.prop)
            }
        }
    ```
    here when "this" is used inside an extension function, we need to bind "this" to the first
    parameter(MANDATORY_PARAM_THIS), and capture the parameter's variable other than containing class's variable
    */
    auto *variable = checker->AsETSChecker()->Scope()->Find(varbinder::VarBinder::MANDATORY_PARAM_THIS).variable;
    if (checker->HasStatus(checker::CheckerStatus::IN_EXTENSION_METHOD)) {
        ES2PANDA_ASSERT(variable != nullptr);
        expr->SetTsType(variable->TsType());
    } else {
        expr->SetTsType(checker->CheckThisOrSuperAccess(expr, checker->Context().ContainingClass(), "this"));
    }

    return expr->TsType();
}

// Get string literal type as potential typeof result type with respect to spec p.7.17
static checker::Type *GetTypeOfStringType(checker::Type *argType, ETSChecker *checker)
{
    if (auto unboxed = checker->MaybeUnboxType(argType); unboxed->IsETSPrimitiveType()) {
        switch (checker->TypeKind(unboxed)) {
            case TypeFlag::ETS_BOOLEAN:
                return checker->CreateETSStringLiteralType("boolean");
            case TypeFlag::BYTE:
            case TypeFlag::CHAR:
            case TypeFlag::SHORT:
            case TypeFlag::INT:
            case TypeFlag::LONG:
            case TypeFlag::FLOAT:
            case TypeFlag::DOUBLE:
                return checker->CreateETSStringLiteralType("number");
            default:
                ES2PANDA_UNREACHABLE();
        }
    }
    if (argType->IsETSUndefinedType()) {
        return checker->CreateETSStringLiteralType("undefined");
    }
    if (argType->IsETSArrayType() || argType->IsETSNullType() || argType->IsETSResizableArrayType()) {
        return checker->CreateETSStringLiteralType("object");
    }
    if (argType->IsETSStringType()) {
        return checker->CreateETSStringLiteralType("string");
    }
    if (argType->IsETSBigIntType()) {
        return checker->CreateETSStringLiteralType("bigint");
    }
    if (argType->IsETSFunctionType()) {
        return checker->CreateETSStringLiteralType("function");
    }
    if (argType->IsETSIntEnumType()) {
        return checker->CreateETSStringLiteralType("number");
    }
    if (argType->IsETSStringEnumType()) {
        return checker->CreateETSStringLiteralType("string");
    }
    return checker->GlobalBuiltinETSStringType();
}

static checker::Type *ComputeTypeOfType(ETSChecker *checker, checker::Type *argType)
{
    checker::Type *ret = nullptr;
    ArenaVector<checker::Type *> types(checker->ProgramAllocator()->Adapter());
    ES2PANDA_ASSERT(argType != nullptr);
    if (argType->IsETSUnionType()) {
        for (auto *it : argType->AsETSUnionType()->ConstituentTypes()) {
            checker::Type *elType = ComputeTypeOfType(checker, it);
            types.push_back(elType);
        }
        ret = checker->CreateETSUnionType(std::move(types));
    } else {
        ret = GetTypeOfStringType(argType, checker);
    }
    return ret;
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::TypeofExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    expr->Argument()->Check(checker);
    expr->SetTsType(ComputeTypeOfType(checker, expr->Argument()->TsType()));
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::UnaryExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();

    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    auto argType = expr->argument_->Check(checker);
    const auto isCondExpr = expr->OperatorType() == lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK;
    checker::Type *operandType = checker->ApplyUnaryOperatorPromotion(expr->argument_, argType, isCondExpr);
    if (argType != nullptr && argType->IsETSBigIntType() && argType->HasTypeFlag(checker::TypeFlag::BIGINT_LITERAL)) {
        switch (expr->OperatorType()) {
            case lexer::TokenType::PUNCTUATOR_MINUS: {
                checker::Type *type = checker->CreateETSBigIntLiteralType(argType->AsETSBigIntType()->GetValue());

                // We do not need this const anymore as we are negating the bigint object in runtime
                ES2PANDA_ASSERT(type != nullptr);
                type->RemoveTypeFlag(checker::TypeFlag::CONSTANT);
                expr->argument_->SetTsType(type);
                expr->SetTsType(type);
                return expr->TsType();
            }
            default:
                // Handled below
                // NOTE(kkonsw): handle other unary operators for bigint literals
                break;
        }
    }

    if (argType != nullptr && argType->IsETSBigIntType()) {
        switch (expr->OperatorType()) {
            case lexer::TokenType::PUNCTUATOR_MINUS:
            case lexer::TokenType::PUNCTUATOR_PLUS:
            case lexer::TokenType::PUNCTUATOR_TILDE: {
                expr->SetTsType(argType);
                return expr->TsType();
            }
            default:
                break;
        }
    }

    SetTsTypeForUnaryExpression(checker, expr, operandType);

    checker->Context().CheckUnarySmartCastCondition(expr);

    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::UpdateExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    checker::Type *operandType = expr->argument_->Check(checker);
    FORWARD_TYPE_ERROR(checker, operandType, expr);

    if (expr->Argument()->IsIdentifier()) {
        checker->ValidateUnaryOperatorOperand(expr->Argument()->AsIdentifier()->Variable(), expr);
    } else if (expr->Argument()->IsTSAsExpression()) {
        if (auto *const asExprVar = expr->Argument()->AsTSAsExpression()->Variable(); asExprVar != nullptr) {
            checker->ValidateUnaryOperatorOperand(asExprVar, expr);
        }
    } else if (expr->Argument()->IsTSNonNullExpression()) {
        if (auto *const nonNullExprVar = expr->Argument()->AsTSNonNullExpression()->Variable();
            nonNullExprVar != nullptr) {
            checker->ValidateUnaryOperatorOperand(nonNullExprVar, expr);
        }
    } else if (expr->Argument()->IsMemberExpression()) {
        varbinder::LocalVariable *propVar = expr->argument_->AsMemberExpression()->PropVar();
        if (propVar != nullptr) {
            checker->ValidateUnaryOperatorOperand(propVar, expr);
        }
    } else {
        ES2PANDA_ASSERT(checker->IsAnyError());
        expr->Argument()->SetTsType(checker->GlobalTypeError());
        return expr->SetTsType(checker->GlobalTypeError());
    }

    if (operandType->IsETSBigIntType()) {
        return expr->SetTsType(operandType);
    }

    auto unboxedType = checker->MaybeUnboxInRelation(operandType);
    if (unboxedType == nullptr || !unboxedType->HasTypeFlag(checker::TypeFlag::ETS_CONVERTIBLE_TO_NUMERIC)) {
        checker->LogError(diagnostic::OPERAND_NOT_NUMERIC, {}, expr->Argument()->Start());
        return expr->SetTsType(checker->GlobalTypeError());
    }

    return expr->SetTsType(operandType);
}

// compile methods for LITERAL EXPRESSIONS in alphabetical order
checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::BigIntLiteral *expr) const
{
    ETSChecker *checker = GetETSChecker();
    expr->SetTsType(checker->CreateETSBigIntLiteralType(expr->Str()));
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::BooleanLiteral *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() == nullptr) {
        auto type = checker->GlobalETSBooleanBuiltinType()->Clone(GetChecker());
        type->AddTypeFlag(TypeFlag::CONSTANT);
        expr->SetTsType(type);
    }
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::CharLiteral *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() == nullptr) {
        auto type = checker->GlobalCharBuiltinType()->Clone(GetChecker());
        type->AddTypeFlag(TypeFlag::CONSTANT);
        expr->SetTsType(type);
    }
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::NullLiteral *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() == nullptr) {
        expr->SetTsType(checker->GlobalETSNullType());
    }
    return expr->TsType();
}

static bool CheckIfLiteralValueIsAppropriate(ETSChecker *checker, Type *type, ir::NumberLiteral *expr)
{
    auto number = expr->Number();
    auto relation = checker->Relation();
    if (relation->IsSupertypeOf(checker->GetGlobalTypesHolder()->GlobalIntegralBuiltinType(), type)) {
        if (number.IsReal()) {
            return false;
        }
        auto val = number.GetValueAndCastTo<int64_t>();
        if (relation->IsIdenticalTo(type, checker->GlobalByteBuiltinType())) {
            return val >= std::numeric_limits<int8_t>::min() && val <= std::numeric_limits<int8_t>::max();
        }
        if (relation->IsIdenticalTo(type, checker->GlobalShortBuiltinType())) {
            return val >= std::numeric_limits<int16_t>::min() && val <= std::numeric_limits<int16_t>::max();
        }
        if (relation->IsIdenticalTo(type, checker->GlobalIntBuiltinType())) {
            return val >= std::numeric_limits<int32_t>::min() && val <= std::numeric_limits<int32_t>::max();
        }
    } else if (relation->IsIdenticalTo(type, checker->GlobalCharBuiltinType())) {
        auto val = number.GetValueAndCastTo<int64_t>();
        return !number.IsReal() && val >= std::numeric_limits<uint16_t>::min() &&
               val <= std::numeric_limits<uint16_t>::max();
    } else if (number.IsDouble()) {
        if (relation->IsIdenticalTo(type, checker->GlobalFloatBuiltinType())) {
            auto doubleVal = number.GetDouble();
            if (doubleVal < std::numeric_limits<float>::min() || doubleVal > std::numeric_limits<float>::max()) {
                return false;
            }
            auto floatVal = static_cast<float>(doubleVal);
            return static_cast<double>(floatVal) == doubleVal;
        }
        return relation->IsIdenticalTo(checker->GlobalDoubleBuiltinType(), type);
    }
    return true;
}

checker::Type *ETSAnalyzer::Check(ir::NumberLiteral *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    ETSChecker *checker = GetETSChecker();
    Type *type;

    if (auto *preferredType =
            GetAppropriatePreferredType(expr->PreferredType(), [&](Type *tp) { return checker->CheckIfNumeric(tp); });
        preferredType != nullptr && !expr->IsFolded() &&
        CheckIfLiteralValueIsAppropriate(checker, preferredType, expr)) {
        type = preferredType->Clone(checker);
    } else if (expr->Number().IsInt()) {
        type = checker->GlobalIntBuiltinType()->Clone(checker);
    } else if (expr->Number().IsLong()) {
        type = checker->GlobalLongBuiltinType()->Clone(checker);
    } else if (expr->Number().IsFloat()) {
        type = checker->GlobalFloatBuiltinType()->Clone(checker);
    } else if (expr->Number().IsDouble()) {
        type = checker->GlobalDoubleBuiltinType()->Clone(checker);
    } else if (expr->Number().IsShort()) {
        type = checker->GlobalShortBuiltinType()->Clone(checker);
    } else if (expr->Number().IsByte()) {
        type = checker->GlobalByteBuiltinType()->Clone(checker);
    } else {
        return checker->GlobalTypeError();
    }

    type->AddTypeFlag(TypeFlag::CONSTANT);
    return expr->SetTsType(type);
}

checker::Type *ETSAnalyzer::Check(ir::StringLiteral *expr) const
{
    ETSChecker *checker = GetETSChecker();
    if (expr->TsType() == nullptr) {
        expr->SetTsType(checker->CreateETSStringLiteralType(expr->Str()));
    }
    return expr->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::ETSIntrinsicNode *node) const
{
    ETSChecker *checker = GetETSChecker();
    for (auto *arg : node->Arguments()) {
        arg->Check(checker);
    }
    // Note (daizihan): #27074, make it more scalable when IntrinsicNodeType is extended.
    if (node->Type() == ir::IntrinsicNodeType::TYPE_REFERENCE) {
        auto type = checker->GlobalBuiltinClassType()->Clone(checker);
        // Since std.core.Class initialize() is instance method, need to remove the variable flag.
        auto newVar = type->Variable()->AsLocalVariable()->Copy(checker->Allocator(), type->Variable()->Declaration());
        newVar->RemoveFlag(varbinder::VariableFlags::CLASS_OR_INTERFACE);
        type->SetVariable(newVar);
        return node->SetTsType(type);
    }
    ES2PANDA_UNREACHABLE();
    return checker->GlobalTypeError();
}

checker::Type *ETSAnalyzer::Check(ir::ImportDeclaration *st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::Type *type = nullptr;
    for (auto *spec : st->Specifiers()) {
        if (spec->IsImportNamespaceSpecifier()) {
            type = spec->AsImportNamespaceSpecifier()->Check(checker);
        }
    }

    return type;
}

checker::Type *ETSAnalyzer::Check(ir::ImportNamespaceSpecifier *st) const
{
    ETSChecker *checker = GetETSChecker();
    if (st->Local()->Name().Empty()) {
        return ReturnTypeForStatement(st);
    }

    if (st->Local()->AsIdentifier()->TsType() != nullptr) {
        return st->Local()->TsType();
    }

    ir::ETSImportDeclaration *importDecl = nullptr;
    if (st->Parent()->IsETSImportDeclaration()) {
        importDecl = st->Parent()->AsETSImportDeclaration();
    } else if (st->Parent()->IsETSReExportDeclaration()) {
        importDecl = st->Parent()->AsETSReExportDeclaration()->GetETSImportDeclarations();
    } else {
        ES2PANDA_UNREACHABLE();
    }

    if (importDecl->IsPureDynamic()) {
        auto *type = checker->GetImportSpecifierObjectType(importDecl, st->Local()->AsIdentifier())->AsETSObjectType();
        checker->SetrModuleObjectTsType(st->Local(), type);
        return type;
    }

    return checker->GetImportSpecifierObjectType(importDecl, st->Local()->AsIdentifier());
}

// compile methods for STATEMENTS in alphabetical order
checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::AssertStatement *st) const
{
    ES2PANDA_UNREACHABLE();
}

checker::Type *ETSAnalyzer::Check(ir::BlockStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    // Iterator type checking of statements is modified to index type, to allow modifying the statement list during
    // checking without invalidating the iterator
    //---- Don't modify this to iterator, as it may break things during checking
    for (std::size_t idx = 0; idx < st->Statements().size(); ++idx) {
        auto *stmt = st->Statements()[idx];
        stmt->Check(checker);

        //  NOTE! Processing of trailing blocks was moved here so that smart casts could be applied correctly
        if (auto *const trailingBlock = st->SearchStatementInTrailingBlock(stmt); trailingBlock != nullptr) {
            trailingBlock->Check(checker);
            st->AddStatement(idx, trailingBlock);
            ++idx;
        }
    }
    if (UNLIKELY(checker->GetDebugInfoPlugin() != nullptr)) {
        // Compilation in eval-mode might require to create additional statements.
        // In this case, they must be created after iteration through statements ends.
        checker->GetDebugInfoPlugin()->AddPrologueEpilogue(st);
    }

    auto const *const scope = st->Scope();
    if (scope == nullptr) {
        return ReturnTypeForStatement(st);
    }

    //  Remove possible smart casts for variables declared in inner scope:
    if (scope->IsFunctionScope() && st->Parent()->Parent()->Parent()->IsMethodDefinition()) {
        // When exiting method definition, just clear all smart casts
        checker->Context().ClearSmartCasts();
    } else if (!scope->IsGlobalScope()) {
        // otherwise only check inner declarations
        for (auto const *const decl : scope->Decls()) {
            if (decl->IsLetOrConstDecl() && decl->Node() != nullptr && decl->Node()->IsIdentifier()) {
                checker->Context().RemoveSmartCast(decl->Node()->AsIdentifier()->Variable());
            }
        }
    }

    // Note: Guarantee all the const property need to be initialized in initializer block is initialized.
    if (st->IsETSModule() && st->AsETSModule()->Program()->IsPackage() &&
        (checker->Context().Status() & checker::CheckerStatus::IN_EXTERNAL) == 0) {
        CheckAllConstPropertyInitialized(checker, st->AsETSModule());
    }
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::BreakStatement *st) const
{
    ETSChecker *checker = GetETSChecker();

    if (!st->HasTarget()) {
        compiler::SetJumpTargetPhase setJumpTarget;
        setJumpTarget.FindJumpTarget(checker->VarBinder()->GetContext(), st);
    }

    if (st->Target() == nullptr) {
        return checker->GlobalTypeError();
    }

    checker->Context().OnBreakStatement(st);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::ClassDeclaration *st) const
{
    ETSChecker *checker = GetETSChecker();
    st->Definition()->Check(checker);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::AnnotationDeclaration *st) const
{
    if (st->Expr()->TsType() != nullptr) {
        return ReturnTypeForStatement(st);
    }
    ETSChecker *checker = GetETSChecker();
    st->Expr()->Check(checker);

    for (auto *anno : st->Annotations()) {
        checker->CheckStandardAnnotation(anno);
        anno->Check(checker);
    }

    ScopeContext scopeCtx(checker, st->Scope());
    for (auto *it : st->Properties()) {
        auto *property = it->AsClassProperty();
        if (checker::Type *propertyType = property->Check(checker); !propertyType->IsTypeError()) {
            checker->CheckAnnotationPropertyType(property);
        }
    }

    auto baseName = st->GetBaseName();
    if (!baseName->IsErrorPlaceHolder() && baseName->Variable()->Declaration()->Node()->IsAnnotationDeclaration()) {
        auto *annoDecl = baseName->Variable()->Declaration()->Node()->AsAnnotationDeclaration();
        if (annoDecl != st && annoDecl->IsDeclare()) {
            checker->CheckAmbientAnnotation(st, annoDecl);
        }
    }

    return ReturnTypeForStatement(st);
}

static void ProcessRequiredFields(ArenaUnorderedMap<util::StringView, ir::ClassProperty *> &fieldMap,
                                  ir::AnnotationUsage *st, ETSChecker *checker)
{
    for (const auto &entry : fieldMap) {
        if (entry.second->Value() == nullptr) {
            checker->LogError(diagnostic::ANNOT_FIELD_NO_VAL, {entry.first}, st->Start());
            continue;
        }
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        auto *clone = entry.second->Clone(checker->Allocator(), st);
        st->AddProperty(clone);
        clone->Check(checker);
    }
}

checker::Type *ETSAnalyzer::Check(ir::AnnotationUsage *st) const
{
    if (st->Expr()->TsType() != nullptr) {
        return ReturnTypeForStatement(st);
    }
    ETSChecker *checker = GetETSChecker();
    st->Expr()->Check(checker);

    auto *baseName = st->GetBaseName();
    if (baseName->Variable() == nullptr || !baseName->Variable()->Declaration()->Node()->IsAnnotationDeclaration()) {
        if (!baseName->IsErrorPlaceHolder()) {
            checker->LogError(diagnostic::NOT_AN_ANNOTATION, {baseName->Name()}, baseName->Start());
        }

        ES2PANDA_ASSERT(checker->IsAnyError());
        return ReturnTypeForStatement(st);
    }

    auto *annoDecl = baseName->Variable()->Declaration()->Node()->AsAnnotationDeclaration();
    annoDecl->Check(checker);

    ArenaUnorderedMap<util::StringView, ir::ClassProperty *> fieldMap {checker->ProgramAllocator()->Adapter()};
    for (auto *it : annoDecl->Properties()) {
        auto *field = it->AsClassProperty();
        ES2PANDA_ASSERT(field->Id() != nullptr);
        fieldMap.insert(std::make_pair(field->Id()->Name(), field));
    }

    if (annoDecl->Properties().size() < st->Properties().size()) {
        checker->LogError(diagnostic::ANNOTATION_ARG_COUNT_MISMATCH, {}, st->Start());
        return ReturnTypeForStatement(st);
    }

    if (st->Properties().size() == 1 && st->Properties().at(0)->AsClassProperty()->Id() != nullptr &&
        st->Properties().at(0)->AsClassProperty()->Id()->Name() == compiler::Signatures::ANNOTATION_KEY_VALUE) {
        checker->CheckSinglePropertyAnnotation(st, annoDecl);
        fieldMap.clear();
    } else {
        checker->CheckMultiplePropertiesAnnotation(st, baseName->Name(), fieldMap);
    }

    ProcessRequiredFields(fieldMap, st, checker);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::ContinueStatement *st) const
{
    ETSChecker *checker = GetETSChecker();

    if (!st->HasTarget()) {
        compiler::SetJumpTargetPhase setJumpTarget;
        setJumpTarget.FindJumpTarget(checker->VarBinder()->GetContext(), st);
    }

    if (st->Target() == nullptr) {
        return checker->GlobalTypeError();
    }

    // CTE if target is outside the function
    auto getEnclosingMethod = [](const ir::AstNode *node) {
        const ir::AstNode *enclosingMethod = node->Parent();
        while (enclosingMethod != nullptr && !enclosingMethod->IsMethodDefinition() &&
               !enclosingMethod->IsArrowFunctionExpression()) {
            enclosingMethod = enclosingMethod->Parent();
        }
        return enclosingMethod;
    };
    if (getEnclosingMethod(st) != getEnclosingMethod(st->Target())) {
        checker->LogError(diagnostic::CONTINUE_TARGET_OUTSIDE_FUNCTION, {}, st->Start());
    }

    checker->AddStatus(CheckerStatus::MEET_CONTINUE);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::DoWhileStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    //  NOTE: Smart casts are not processed correctly within the loops now, thus clear them at this point.
    auto [smartCasts, clearFlag] = checker->Context().EnterLoop(*st, std::nullopt);

    checker->CheckTruthinessOfType(st->Test());
    st->Body()->Check(checker);

    checker->Context().ExitLoop(smartCasts, clearFlag, st);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check([[maybe_unused]] ir::EmptyStatement *st) const
{
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::ExpressionStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    return st->GetExpression()->Check(checker);
}

static bool ValidateAndProcessIteratorType(ETSChecker *checker, Type *elemType, ir::ForOfStatement *const st)
{
    checker::Type *iterType = GetIteratorType(checker, elemType, st->Left());
    if (iterType->IsTypeError()) {
        return false;
    }

    const auto ident = st->Left()->IsVariableDeclaration()
                           ? st->Left()->AsVariableDeclaration()->Declarators().front()->Id()->AsIdentifier()
                           : st->Left()->AsIdentifier();
    auto *const relation = checker->Relation();
    relation->SetFlags(checker::TypeRelationFlag::ASSIGNMENT_CONTEXT);
    relation->SetNode(ident);
    if (auto ctx = checker::AssignmentContext(checker->Relation(), ident, elemType, iterType, ident->Start(),
                                              std::nullopt, TypeRelationFlag::NO_THROW);
        !ctx.IsAssignable() && !relation->IsLegalBoxedPrimitiveConversion(iterType, elemType)) {
        checker->LogError(diagnostic::ITERATOR_ELEMENT_TYPE_MISMATCH, {elemType, iterType}, st->Start());
        return false;
    }

    relation->SetNode(nullptr);
    relation->SetFlags(checker::TypeRelationFlag::NONE);

    const auto variable = ident->Variable();
    if (variable != nullptr) {
        // Set smart type for variable of 'for-of' statement
        const auto smartType = checker->ResolveSmartType(elemType, variable->TsType());
        checker->Context().SetSmartCast(variable, smartType);
    }

    return true;
}

checker::Type *ETSAnalyzer::Check(ir::ForOfStatement *const st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    //  NOTE: Smart casts are not processed correctly within the loops now, thus clear them at this point.
    auto [smartCasts, clearFlag] = checker->Context().EnterLoop(*st, std::nullopt);

    checker::Type *const exprType = st->Right()->Check(checker)->MaybeBaseTypeOfGradualType();
    checker::Type *elemType = checker->GlobalTypeError();

    if (exprType->IsETSStringType()) {
        elemType = checker->GlobalBuiltinETSStringType();
    } else if (exprType->IsETSArrayType() || exprType->IsETSResizableArrayType()) {
        elemType = checker->GetElementTypeOfArray(exprType);
    } else if (exprType->IsETSObjectType() || exprType->IsETSUnionType() || exprType->IsETSTypeParameter()) {
        elemType = st->CheckIteratorMethod(checker);
    }

    if (elemType == checker->GlobalTypeError()) {
        checker->LogError(diagnostic::FOROF_SOURCE_NONITERABLE, {}, st->Right()->Start());
        return checker->GlobalTypeError();
    }

    st->Left()->Check(checker);

    if (!ValidateAndProcessIteratorType(checker, elemType, st)) {
        return checker->GlobalTypeError();
    };

    st->Body()->Check(checker);

    checker->Context().ExitLoop(smartCasts, clearFlag, st);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::ForUpdateStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    //  NOTE: Smart casts are not processed correctly within the loops now, thus clear them at this point.
    auto [smartCasts, clearFlag] = checker->Context().EnterLoop(*st, std::nullopt);

    if (st->Init() != nullptr) {
        st->Init()->Check(checker);
    }

    if (st->Test() != nullptr) {
        checker->CheckTruthinessOfType(st->Test());
    }

    if (st->Update() != nullptr) {
        st->Update()->Check(checker);
    }

    st->Body()->Check(checker);

    checker->Context().ExitLoop(smartCasts, clearFlag, st);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::IfStatement *st) const
{
    ETSChecker *const checker = GetETSChecker();

    SmartCastArray smartCasts = checker->Context().EnterTestExpression();
    checker->CheckTruthinessOfType(st->Test());
    SmartCastTypes testedTypes = checker->Context().ExitTestExpression();
    if (testedTypes.has_value()) {
        for (auto [variable, consequentType, _] : *testedTypes) {
            checker->ApplySmartCast(variable, consequentType);
        }
    }

    checker->Context().EnterPath();
    st->Consequent()->Check(checker);
    bool const consequentTerminated = checker->Context().ExitPath();
    SmartCastArray consequentSmartCasts = checker->Context().CloneSmartCasts();

    // Restore smart casts to initial state.
    checker->Context().RestoreSmartCasts(smartCasts);
    //  Apply the alternate smart casts
    if (testedTypes.has_value()) {
        for (auto [variable, _, alternateType] : *testedTypes) {
            checker->ApplySmartCast(variable, alternateType);
        }
    }

    if (st->Alternate() != nullptr) {
        checker->Context().EnterPath();
        st->Alternate()->Check(checker);
        bool const alternateTerminated = checker->Context().ExitPath();
        if (alternateTerminated) {
            if (!consequentTerminated) {
                // Here we need to restore types from consequent if block.
                checker->Context().RestoreSmartCasts(consequentSmartCasts);
            } else {
                // Here we need to restore initial smart types.
                checker->Context().RestoreSmartCasts(smartCasts);
            }
        } else if (!consequentTerminated) {
            // Here we need to combine types from consequent and alternate if blocks.
            checker->Context().CombineSmartCasts(consequentSmartCasts);
        }
    } else {
        if (!consequentTerminated) {
            // Here we need to combine types from consequent if block and initial.
            checker->Context().CombineSmartCasts(consequentSmartCasts);
        }
    }

    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::LabelledStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    st->body_->Check(checker);
    return ReturnTypeForStatement(st);
}

static bool CheckIsValidReturnTypeAnnotation(ir::ReturnStatement *st, ir::ScriptFunction *containingFunc,
                                             ir::TypeNode *returnTypeAnnotation, ETSChecker *checker)
{
    // check valid `this` type as return type
    if (containingFunc->GetPreferredReturnType() != nullptr ||
        (returnTypeAnnotation != nullptr && !returnTypeAnnotation->IsTSThisType())) {
        return true;
    }

    // only extension function and class method could return `this`;
    bool inValidNormalFuncReturnThisType = st->Argument() == nullptr || !st->Argument()->IsThisExpression();
    bool inValidExtensionFuncReturnThisType =
        !containingFunc->HasReceiver() ||
        (containingFunc->HasReceiver() && (st->Argument() == nullptr || !st->Argument()->IsIdentifier() ||
                                           !st->Argument()->AsIdentifier()->IsReceiver()));
    if (inValidNormalFuncReturnThisType && inValidExtensionFuncReturnThisType) {
        checker->LogError(diagnostic::RETURN_THIS_OUTSIDE_METHOD, {}, st->Start());
        return false;
    }

    return true;
}

bool ETSAnalyzer::CheckInferredFunctionReturnType(ir::ReturnStatement *st, ir::ScriptFunction *containingFunc,
                                                  checker::Type *&funcReturnType, ir::TypeNode *returnTypeAnnotation,
                                                  ETSChecker *checker) const
{
    if (!CheckIsValidReturnTypeAnnotation(st, containingFunc, returnTypeAnnotation, checker)) {
        return false;
    }

    if (containingFunc->ReturnTypeAnnotation() != nullptr) {
        if (containingFunc->IsAsyncFunc()) {
            auto *promiseType = containingFunc->ReturnTypeAnnotation()->GetType(checker);
            if (!promiseType->IsETSObjectType() || promiseType->AsETSObjectType()->TypeArguments().size() != 1) {
                return false;
            }
            funcReturnType = checker->CreateETSAsyncFuncReturnTypeFromPromiseType(promiseType->AsETSObjectType());
        } else {
            funcReturnType = containingFunc->ReturnTypeAnnotation()->GetType(checker);
        }
    } else {
        funcReturnType = containingFunc->GetPreferredReturnType();
    }

    // Case when function's return type is defined explicitly:
    if (st->argument_ == nullptr) {
        ES2PANDA_ASSERT(funcReturnType != nullptr);
        if (!funcReturnType->MaybeBaseTypeOfGradualType()->IsETSVoidType() &&
            funcReturnType != checker->GlobalVoidType() &&
            !funcReturnType->MaybeBaseTypeOfGradualType()->IsETSAsyncFuncReturnType()) {
            checker->LogError(diagnostic::RETURN_WITHOUT_VALUE, {}, st->Start());
            return false;
        }
        funcReturnType = checker->GlobalVoidType();
    } else {
        const auto name = containingFunc->Scope()->InternalName().Mutf8();
        if (!CheckArgumentVoidType(funcReturnType->MaybeBaseTypeOfGradualType(), checker, name, st)) {
            return false;
        }

        if (st->argument_->IsMemberExpression()) {
            checker->SetArrayPreferredTypeForNestedMemberExpressions(st->argument_->AsMemberExpression(),
                                                                     funcReturnType);
        } else {
            st->argument_->SetPreferredType(funcReturnType);
        }

        checker::Type *argumentType = st->argument_->Check(checker);
        return CheckReturnType(checker, funcReturnType->MaybeBaseTypeOfGradualType(), argumentType, st->argument_,
                               containingFunc);
    }
    return true;
}

checker::Type *ETSAnalyzer::GetFunctionReturnType(ir::ReturnStatement *st, ir::ScriptFunction *containingFunc) const
{
    ES2PANDA_ASSERT(containingFunc->ReturnTypeAnnotation() != nullptr ||
                    containingFunc->Signature()->ReturnType() != nullptr ||
                    containingFunc->GetPreferredReturnType() != nullptr);

    ETSChecker *checker = GetETSChecker();
    checker::Type *funcReturnType = nullptr;

    if (auto *const returnTypeAnnotation = containingFunc->ReturnTypeAnnotation();
        returnTypeAnnotation != nullptr || containingFunc->GetPreferredReturnType() != nullptr) {
        if (!CheckInferredFunctionReturnType(st, containingFunc, funcReturnType, returnTypeAnnotation, checker)) {
            return checker->GlobalTypeError();
        }
    } else {
        //  Case when function's return type should be inferred from return statement(s):
        if (containingFunc->Signature()->HasSignatureFlag(checker::SignatureFlags::NEED_RETURN_TYPE)) {
            funcReturnType = InferReturnType(checker, containingFunc,
                                             st->argument_);  // This removes the NEED_RETURN_TYPE flag, so only the
                                                              // first return statement going to land here...
        } else {
            //  All subsequent return statements:
            funcReturnType =
                ProcessReturnStatements(checker, containingFunc, st,
                                        st->argument_);  // and the remaining return statements will get processed here.
        }
    }

    if ((st->argument_ != nullptr) && st->argument_->IsArrayExpression() && funcReturnType->IsArrayType()) {
        checker->ModifyPreferredType(st->argument_->AsArrayExpression(), funcReturnType);
        st->argument_->Check(checker);
    }

    return funcReturnType;
}

checker::Type *ETSAnalyzer::Check(ir::ReturnStatement *st) const
{
    ETSChecker *checker = GetETSChecker();

    ir::AstNode *ancestor = util::Helpers::FindAncestorGivenByType(st, ir::AstNodeType::SCRIPT_FUNCTION);
    ES2PANDA_ASSERT(ancestor && ancestor->IsScriptFunction());

    auto *containingFunc = ancestor->AsScriptFunction();
    containingFunc->AddFlag(ir::ScriptFunctionFlags::HAS_RETURN);

    if (containingFunc->Signature() == nullptr) {
        ES2PANDA_ASSERT(checker->IsAnyError());
        return ReturnTypeForStatement(st);
    }

    checker->AddStatus(CheckerStatus::MEET_RETURN);

    if (containingFunc->IsConstructor()) {
        if (st->argument_ != nullptr) {
            checker->LogError(diagnostic::NON_VOID_RETURN_IN_CONSTRUCTOR, {}, st->Start());
            return checker->GlobalTypeError();
        }
        return ReturnTypeForStatement(st);
    }

    st->returnType_ = GetFunctionReturnType(st, containingFunc);

    if (containingFunc->ReturnTypeAnnotation() == nullptr) {
        containingFunc->AddReturnStatement(st);
    }

    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::SwitchStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());
    checker::SavedTypeRelationFlagsContext savedTypeRelationFlagCtx(checker->Relation(),
                                                                    checker::TypeRelationFlag::NONE);

    auto *comparedExprType = checker->CheckSwitchDiscriminant(st->Discriminant());
    // may have no meaning to unbox comparedExprType
    auto unboxedDiscType = checker->MaybeUnboxType(comparedExprType);

    SmartCastArray smartCasts = checker->Context().CloneSmartCasts();
    bool hasDefaultCase = false;

    for (auto &it : st->Cases()) {
        checker->Context().EnterPath();
        it->CheckAndTestCase(checker, comparedExprType, unboxedDiscType, st->Discriminant(), hasDefaultCase);
        bool const caseTerminated = checker->Context().ExitPath();

        if (it != st->Cases().back()) {
            if (!caseTerminated) {
                checker->Context().CombineSmartCasts(smartCasts);
            } else {
                checker->Context().RestoreSmartCasts(smartCasts);
            }
        } else {
            if (!caseTerminated) {
                //  if the recent switch case isn't terminated in any way, copy actual smart casts to the array of
                //  smart casts for the other case blocks so that it can be processed in unified way
                checker->Context().AddBreakSmartCasts(st, checker->Context().CloneSmartCasts());
            }
            checker->Context().ClearSmartCasts();
        }
    }

    // If default case is absent initial smart casts should be also applied here
    if (!hasDefaultCase) {
        checker->Context().AddBreakSmartCasts(st, std::move(smartCasts));
    }

    // Combine smart casts from all [non-terminated] case blocks with 'break'
    checker->Context().CombineBreakSmartCasts(st);

    checker->CheckForSameSwitchCases(st->Cases());
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::ThrowStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    const auto *arg = st->argument_;
    checker::Type *argType = st->argument_->Check(checker);

    bool isRethrow = false;
    if (arg->IsIdentifier() && !catchParamStack_.empty()) {
        const varbinder::Variable *sym = arg->AsIdentifier()->Variable();
        ES2PANDA_ASSERT(sym != nullptr);
        if (!catchParamStack_.empty() && sym == catchParamStack_.back()) {
            isRethrow = true;
        }
    }
    if (!isRethrow && !argType->IsTypeError()) {
        checker->CheckExceptionOrErrorType(argType, st->Start());
    }

    checker->AddStatus(CheckerStatus::MEET_THROW);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::TryStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    std::vector<checker::ETSObjectType *> exceptions {};

    std::vector<SmartCastArray> casts {};
    auto smartCasts = checker->Context().CheckTryBlock(*st->Block());
    st->Block()->Check(checker);

    bool defaultCatchFound = false;
    for (auto *catchClause : st->CatchClauses()) {
        if (defaultCatchFound) {
            checker->LogError(diagnostic::CATCH_DEFAULT_NOT_LAST, {}, catchClause->Start());
            return checker->GlobalTypeError();
        }

        checker->Context().RestoreSmartCasts(smartCasts);

        if (auto const exceptionType = catchClause->Check(checker); !exceptionType->IsTypeError()) {
            auto *clauseType = exceptionType->AsETSObjectType();
            checker->CheckExceptionClauseType(exceptions, catchClause, clauseType);
            exceptions.emplace_back(clauseType);
        }

        defaultCatchFound = catchClause->IsDefaultCatchClause();

        casts.emplace_back(checker->Context().CloneSmartCasts());
    }

    checker->Context().RestoreSmartCasts(smartCasts);
    if (!casts.empty()) {
        for (auto const &cast : casts) {
            checker->Context().CombineSmartCasts(cast);
        }
    }

    if (st->HasFinalizer()) {
        st->FinallyBlock()->Check(checker);
    }

    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::VariableDeclarator *st) const
{
    bool initChecked = st->Init() != nullptr ? st->Init()->TsType() != nullptr : true;
    if (st->TsType() != nullptr && initChecked) {
        return st->TsType();
    }

    ETSChecker *checker = GetETSChecker();
    ES2PANDA_ASSERT(st->Id()->IsIdentifier());
    auto *const ident = st->Id()->AsIdentifier();
    ir::ModifierFlags flags = ir::ModifierFlags::NONE;

    if (ident->Parent()->Parent()->AsVariableDeclaration()->Kind() ==
        ir::VariableDeclaration::VariableDeclarationKind::CONST) {
        flags |= ir::ModifierFlags::CONST;
    }

    if (ident->IsOptionalDeclaration()) {
        flags |= ir::ModifierFlags::OPTIONAL;
    }

    // Processing possible parser errors
    if (ident->Variable() == nullptr) {
        ident->Check(checker);
    }
    auto *const variableType = checker->CheckVariableDeclaration(ident, ident->TypeAnnotation(), st->Init(), flags);

    //  Now try to define the actual type of Identifier so that smart cast can be used in further checker processing
    //  NOTE: T_S and K_o_t_l_i_n don't act in such way, but we can try - why not? :)
    auto *smartType = variableType;
    if (auto *const initType = st->Init() != nullptr ? st->Init()->TsType() : nullptr; initType != nullptr) {
        auto const value = st->Init()->IsNumberLiteral()
                               ? std::make_optional(st->Init()->AsNumberLiteral()->Number().GetDouble())
                               : std::nullopt;

        smartType = checker->ResolveSmartType(initType, variableType, value);
        //  Set smart type for identifier if it differs from annotated type
        //  Top-level and captured variables are not processed here!
        if (!checker->Relation()->IsIdenticalTo(variableType, smartType)) {
            ident->SetTsType(smartType);
            checker->Context().SetSmartCast(ident->Variable(), smartType);
        }
    }

    return st->SetTsType(smartType);
}

checker::Type *ETSAnalyzer::Check(ir::VariableDeclaration *st) const
{
    ETSChecker *checker = GetETSChecker();

    checker->CheckAnnotations(st->Annotations());

    for (auto *it : st->Declarators()) {
        it->Check(checker);
    }

    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::WhileStatement *st) const
{
    ETSChecker *checker = GetETSChecker();
    checker::ScopeContext scopeCtx(checker, st->Scope());

    // Invalidate smart cast for variables in the test condition, that will be reassigned in the loop body
    const auto reassignedVars = checker->Context().GetReassignedVariablesInNode(st->Body());
    for (const auto &[var, _] : reassignedVars) {
        checker->Context().RemoveSmartCast(var);
    }

    SmartCastArray savedSmartCasts = checker->Context().EnterTestExpression();
    checker->CheckTruthinessOfType(st->Test());
    SmartCastTypes testedTypes = checker->Context().ExitTestExpression();
    if (testedTypes.has_value()) {
        for (auto [variable, consequentType, _] : *testedTypes) {
            checker->ApplySmartCast(variable, consequentType);
        }
    }

    auto [smartCasts, clearFlag] = checker->Context().EnterLoop(*st, testedTypes);
    st->Body()->Check(checker);
    checker->Context().ExitLoop(savedSmartCasts, clearFlag, st);
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::TSArrayType *node) const
{
    ETSChecker *checker = GetETSChecker();
    checker->CheckAnnotations(node->Annotations());
    node->elementType_->Check(checker);
    node->SetTsType(node->GetType(checker));

    const auto *arrayType = node->TsType()->AsETSArrayType();
    checker->CreateBuiltinArraySignature(arrayType, arrayType->Rank());
    return node->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::TSAsExpression *expr) const
{
    ETSChecker *checker = GetETSChecker();

    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }

    checker->CheckAnnotations(expr->TypeAnnotation()->Annotations());
    auto *const targetType = expr->TypeAnnotation()->AsTypeNode()->GetType(checker);
    FORWARD_TYPE_ERROR(checker, targetType, expr);

    expr->Expr()->SetPreferredType(targetType);

    auto const sourceType = expr->Expr()->Check(checker);
    FORWARD_TYPE_ERROR(checker, sourceType, expr);

    if (sourceType->DefinitelyETSNullish() && !targetType->PossiblyETSNullish()) {
        return expr->SetTsType(checker->TypeError(expr, diagnostic::NULLISH_CAST_TO_NONNULLISH, expr->Start()));
    }

    if (expr->Expr()->IsLiteral() && sourceType->IsBuiltinNumeric() && targetType->IsETSTypeParameter()) {
        checker->LogError(diagnostic::INVALID_CAST, {sourceType->ToString(), targetType->ToString()},
                          expr->Expr()->Start());
        return checker->InvalidateType(expr);
    }

    if (expr->Expr()->IsLiteral() && sourceType->IsBuiltinNumeric() && targetType->IsETSUnionType()) {
        bool allAreTypeParams = true;
        for (auto *sub : targetType->AsETSUnionType()->ConstituentTypes()) {
            if (!sub->IsETSTypeParameter()) {
                allAreTypeParams = false;
            }
        }
        if (allAreTypeParams) {
            checker->LogError(diagnostic::INVALID_CAST, {sourceType->ToString(), targetType->ToString()},
                              expr->Expr()->Start());
            return checker->InvalidateType(expr);
        }
    }

    const checker::CastingContext ctx(
        checker->Relation(),
        sourceType->IsBuiltinNumeric() && targetType->IsBuiltinNumeric() ? diagnostic::IMPROPER_NUMERIC_CAST
                                                                         : diagnostic::INVALID_CAST,
        // CC-OFFNXT(G.FMT.03-CPP) project code style
        {sourceType, targetType},
        checker::CastingContext::ConstructorData {expr->Expr(), sourceType, targetType, expr->Expr()->Start()});

    expr->isUncheckedCast_ = ctx.UncheckedCast();

    // Make sure the array type symbol gets created for the assembler to be able to emit checkcast.
    // Because it might not exist, if this particular array type was never created explicitly.
    if (!expr->isUncheckedCast_ && targetType->IsETSArrayType()) {
        const auto *const targetArrayType = targetType->AsETSArrayType();
        checker->CreateBuiltinArraySignature(targetArrayType, targetArrayType->Rank());
    }

    if (targetType == checker->GetGlobalTypesHolder()->GlobalETSNeverType()) {
        return expr->SetTsType(checker->TypeError(expr, diagnostic::CAST_TO_NEVER, expr->Start()));
    }

    return expr->SetTsType(targetType);
}

checker::Type *ETSAnalyzer::Check(ir::TSEnumDeclaration *st) const
{
    // Some invalid TSEnumDeclaration will not be transformed to class.
    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::Check(ir::TSInterfaceDeclaration *st) const
{
    if (st->TsType() != nullptr) {
        return st->TsType();
    }

    ETSChecker *checker = GetETSChecker();
    auto *stmtType = checker->BuildBasicInterfaceProperties(st);
    ES2PANDA_ASSERT(stmtType != nullptr);

    FORWARD_TYPE_ERROR(checker, stmtType, st);

    auto *interfaceType = stmtType->IsGradualType() ? stmtType->AsGradualType()->GetBaseType()->AsETSObjectType()
                                                    : stmtType->AsETSObjectType();
    checker->CheckInterfaceAnnotations(st);

    interfaceType->SetSuperType(checker->GlobalETSObjectType());
    checker->CheckInvokeMethodsLegitimacy(interfaceType);

    st->SetTsType(stmtType);
    checker->CheckDynamicInheritanceAndImplement(interfaceType->AsETSObjectType());
    checker::ScopeContext scopeCtx(checker, st->Scope());
    auto savedContext = checker::SavedCheckerContext(checker, checker::CheckerStatus::IN_INTERFACE, interfaceType);

    for (auto *it : st->Body()->Body()) {
        it->Check(checker);
    }
    return st->TsType();
}

checker::Type *ETSAnalyzer::Check(ir::TSNonNullExpression *expr) const
{
    if (expr->TsType() != nullptr) {
        return expr->TsType();
    }
    ETSChecker *checker = GetETSChecker();
    auto exprType = expr->expr_->Check(checker);
    //  If the actual [smart] type is definitely 'null' or 'undefined' then probably CTE should be thrown.
    //  Anyway we'll definitely obtain NullPointerError at runtime.
    if (exprType->DefinitelyETSNullish()) {
        checker->LogDiagnostic(diagnostic::NULLISH_OPERAND, expr->Expr()->Start());

        if (expr->expr_->IsIdentifier()) {
            ES2PANDA_ASSERT(expr->expr_->AsIdentifier()->Variable() != nullptr);
            auto originalType = expr->expr_->AsIdentifier()->Variable()->TsType();
            if (originalType != nullptr) {
                expr->SetTsType(checker->GetNonNullishType(originalType));
            }
        }
    }

    if (expr->TsType() == nullptr) {
        expr->SetTsType(checker->GetNonNullishType(exprType));
    }
    expr->SetOriginalType(expr->TsType());
    return expr->TsType();
}

static varbinder::Variable *FindNameForImportNamespace(ETSChecker *checker, util::StringView &searchName,
                                                       ETSObjectType *baseType)
{
    /* This function try to find name1.name2, name1.A in file file1.ets,
     * ./file1.ets:
     * import * as name1 from "./file2"
     *
     * ./file2.ets:
     * import * as name2 from "./file3"
     * import {A} from "./file3"
     * export {name2}
     * export {A}
     *
     * ./file3.ets
     * export class A{}
     *
     * 1. Find in file2->program->ast->scope first
     * 2. Find in varbinder->selectiveExportAliasMultimap second
     * if both found, return variable
     */
    auto declNode = baseType->GetDeclNode();
    if (!declNode->IsIdentifier()) {
        return nullptr;
    }
    if (declNode->Parent() == nullptr || declNode->Parent()->Parent() == nullptr) {
        return nullptr;
    }
    auto importDeclNode = declNode->Parent()->Parent();
    if (!importDeclNode->IsETSImportDeclaration()) {
        return nullptr;
    }

    auto importDecl = importDeclNode->AsETSImportDeclaration();

    parser::Program *program = checker->SelectEntryOrExternalProgram(
        static_cast<varbinder::ETSBinder *>(checker->VarBinder()), importDecl->ImportMetadata().resolvedSource);

    auto &bindings = program->Ast()->Scope()->Bindings();

    if (auto result = bindings.find(searchName); result != bindings.end()) {
        auto &sMap = checker->VarBinder()
                         ->AsETSBinder()
                         ->GetSelectiveExportAliasMultimap()
                         .find(importDecl->ImportMetadata().resolvedSource)
                         ->second;
        if (auto it = sMap.find(searchName); it != sMap.end()) {
            return result->second;
        }
    }
    return nullptr;
}

checker::Type *ETSAnalyzer::Check(ir::TSQualifiedName *expr) const
{
    ETSChecker *checker = GetETSChecker();
    checker::Type *baseType = expr->Left()->Check(checker)->MaybeBaseTypeOfGradualType();
    if (baseType->IsETSObjectType()) {
        // clang-format off
        auto searchName = expr->Right()->Name();
        // clang-format on
        // NOTE (oeotvos) This should be done differently in the follow-up patch.
        if (searchName.Empty()) {
            searchName = expr->Right()->Name();
        }
        varbinder::Variable *prop =
            baseType->AsETSObjectType()->GetProperty(searchName, PropertySearchFlags::SEARCH_DECL);

        if (prop == nullptr) {
            prop = FindNameForImportNamespace(GetETSChecker(), searchName, baseType->AsETSObjectType());
        }
        // NOTE(dslynko): in debugger evaluation mode must lazily generate module's properties here.
        if (prop == nullptr) {
            checker->LogError(diagnostic::NONEXISTENT_TYPE, {expr->Right()->Name()}, expr->Right()->Start());
            return checker->GlobalTypeError();
        }

        checker->ValidateNamespaceProperty(prop, baseType->AsETSObjectType(), expr->Right());
        expr->Right()->SetVariable(prop);
        return checker->GetTypeOfVariable(prop);
    }

    checker->LogError(diagnostic::NONEXISTENT_TYPE, {expr->Right()->Name()}, expr->Right()->Start());
    return checker->GlobalTypeError();
}

checker::Type *ETSAnalyzer::Check(ir::TSTypeAliasDeclaration *st) const
{
    ETSChecker *checker = GetETSChecker();
    auto checkerContext = SavedCheckerContext(checker, CheckerStatus::NO_OPTS, checker->Context().ContainingClass());

    checker->CheckAnnotations(st->Annotations());

    if (st->TypeParams() == nullptr) {
        const checker::SavedTypeRelationFlagsContext savedFlagsCtx(
            checker->Relation(), checker::TypeRelationFlag::NO_THROW_GENERIC_TYPEALIAS);

        if (st->TypeAnnotation()->TsType() == nullptr) {
            st->TypeAnnotation()->Check(checker);
        }

        return ReturnTypeForStatement(st);
    }

    if (st->TypeParameterTypes().empty()) {
        auto [typeParamTypes, ok] = checker->CreateUnconstrainedTypeParameters(st->TypeParams());
        st->SetTypeParameterTypes(std::move(typeParamTypes));
        if (ok) {
            checker->AssignTypeParameterConstraints(st->TypeParams());
        }
    }

    const checker::SavedTypeRelationFlagsContext savedFlagsCtx(checker->Relation(),
                                                               checker::TypeRelationFlag::NO_THROW_GENERIC_TYPEALIAS);

    if (st->TypeAnnotation()->TsType() == nullptr) {
        st->TypeAnnotation()->Check(checker);
    }

    return ReturnTypeForStatement(st);
}

checker::Type *ETSAnalyzer::ReturnTypeForStatement([[maybe_unused]] const ir::Statement *const st) const
{
    ES2PANDA_ASSERT(st->IsStatement());
    return nullptr;
}

}  // namespace ark::es2panda::checker
