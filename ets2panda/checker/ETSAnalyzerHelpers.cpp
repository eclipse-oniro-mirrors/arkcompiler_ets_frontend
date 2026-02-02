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

#include "ETSAnalyzerHelpers.h"
#include "checker/types/ets/etsAsyncFuncReturnType.h"
#include "checker/types/typeError.h"
#include "util/diagnostic.h"

namespace ark::es2panda::checker {

static bool IsValidReceiverParameter(Type *const thisType)
{
    if (thisType == nullptr) {
        return false;
    }

    if (thisType->IsETSArrayType() || thisType->IsETSTypeParameter()) {
        return true;
    }

    if (!thisType->IsETSObjectType() || thisType->IsETSEnumType()) {
        return false;
    }

    auto *const thisObjectType = thisType->AsETSObjectType();
    return thisObjectType->HasObjectFlag(ETSObjectFlags::CLASS | ETSObjectFlags::INTERFACE);
}

void CheckExtensionIsShadowedInCurrentClassOrInterface(checker::ETSChecker *checker, checker::ETSObjectType *objType,
                                                       ir::ScriptFunction *extensionFunc, checker::Signature *signature)
{
    const auto methodName = extensionFunc->Id()->Name();

    // check if there are class and interfaces' instance fields with the same name as extensions.
    auto *const fieldVariable = objType->GetOwnProperty<checker::PropertyType::INSTANCE_FIELD>(methodName);
    if (fieldVariable != nullptr) {
        checker->LogError(diagnostic::EXTENSION_NAME_CONFLICT_WITH_FIELD, {methodName, objType->Name()},
                          extensionFunc->Body()->Start());
        return;
    }

    // check if there are class and interfaces' instance methods with the same name as extensions.
    auto *const methodVariable = objType->GetOwnProperty<checker::PropertyType::INSTANCE_METHOD>(methodName);
    if (methodVariable == nullptr || methodVariable->TsType()->IsTypeError()) {
        return;
    }

    if (methodVariable->TsType()->IsTypeError()) {
        return;
    }
    const auto *const funcType = methodVariable->TsType()->AsETSFunctionType();
    for (auto *funcSignature : funcType->CallSignatures()) {
        ES2PANDA_ASSERT(signature != nullptr);
        signature->SetReturnType(funcSignature->ReturnType());
        if (!checker->Relation()->SignatureIsSupertypeOf(signature, funcSignature) &&
            !checker->HasSameAssemblySignature(signature, funcSignature)) {
            continue;
        }

        if (signature->Function()->IsGetter() || signature->Function()->IsSetter()) {
            checker->LogError(diagnostic::EXTENSION_NAME_CONFLICT_WITH_METHOD, {funcType->Name(), objType->Name()},
                              extensionFunc->Body()->Start());
        } else if (funcSignature->HasSignatureFlag(SignatureFlags::PUBLIC)) {
            checker->LogError(diagnostic::EXTENSION_FUNC_NAME_CONFLICT_WITH_METH, {funcType->Name(), objType->Name()},
                              extensionFunc->Body()->Start());
        } else {
            checker->LogDiagnostic(diagnostic::EXTENSION_NONPUBLIC_COLLISION, {funcType->Name(), objType->Name()},
                                   extensionFunc->Body()->Start());
        }
        return;
    }
}

void CheckExtensionIsShadowedByMethod(checker::ETSChecker *checker, checker::ETSObjectType *objType,
                                      ir::ScriptFunction *extensionFunc, checker::Signature *signature)
{
    if (objType == nullptr) {
        return;
    }

    CheckExtensionIsShadowedInCurrentClassOrInterface(checker, objType, extensionFunc, signature);

    for (auto *interface : objType->Interfaces()) {
        CheckExtensionIsShadowedByMethod(checker, interface, extensionFunc, signature);
    }

    CheckExtensionIsShadowedByMethod(checker, objType->SuperType(), extensionFunc, signature);
}

static void ReplaceThisInExtensionMethod(checker::ETSChecker *checker, ir::ScriptFunction *extensionFunc)
{
    //  Skip processing of possibly invalid extension method
    if (extensionFunc->Params().empty() ||
        !extensionFunc->Params()[0]->AsETSParameterExpression()->Ident()->IsReceiver()) {
        ES2PANDA_ASSERT(checker->IsAnyError());
        return;
    }

    auto thisVariable = extensionFunc->Params()[0]->Variable();
    extensionFunc->Body()->TransformChildrenRecursively(
        [=](ir::AstNode *ast) {
            if (ast->IsThisExpression()) {
                auto *thisParam = checker->ProgramAllocator()->New<ir::Identifier>(
                    varbinder::TypedBinder::MANDATORY_PARAM_THIS, checker->ProgramAllocator());
                thisParam->SetRange(ast->Range());
                thisParam->SetParent(ast->Parent());
                thisParam->SetVariable(thisVariable);
                return static_cast<ir::AstNode *>(thisParam);
            }
            return ast;
        },
        "replace-this-in-extension-method");
}

void CheckExtensionMethod(checker::ETSChecker *checker, ir::ScriptFunction *extensionFunc, ir::AstNode *node)
{
    auto *const thisType =
        !extensionFunc->Signature()->Params().empty() ? extensionFunc->Signature()->Params()[0]->TsType() : nullptr;

    // "Extension Functions" are only allowed for classes, interfaces, arrays and type parameters extends from object.
    if (IsValidReceiverParameter(thisType)) {
        // Skip for arrays (array does not contain a class definition) and checked class definition.
        if (!thisType->IsETSArrayType() && !thisType->IsETSTypeParameter() &&
            thisType->Variable()->Declaration()->Node()->IsClassDefinition() &&
            !thisType->Variable()->Declaration()->Node()->AsClassDefinition()->IsClassDefinitionChecked()) {
            thisType->Variable()->Declaration()->Node()->Check(checker);
        }
        // NOTE(gogabr): should be done in a lowering
        ReplaceThisInExtensionMethod(checker, extensionFunc);
        if (extensionFunc->IsArrow()) {
            return;
        }

        checker::SignatureInfo *originalExtensionSigInfo = checker->ProgramAllocator()->New<checker::SignatureInfo>(
            extensionFunc->Signature()->GetSignatureInfo(), checker->ProgramAllocator());
        ES2PANDA_ASSERT(originalExtensionSigInfo != nullptr);
        originalExtensionSigInfo->minArgCount -= 1U;
        originalExtensionSigInfo->params.erase(originalExtensionSigInfo->params.begin());
        checker::Signature *originalExtensionSignature =
            checker->CreateSignature(originalExtensionSigInfo, extensionFunc->Signature()->ReturnType(), extensionFunc);

        // The shadowing check is only relevant for classes and interfaces,
        // since for arrays there are no other ways to declare a method other than "Extension Functions".
        if (thisType->IsETSObjectType()) {
            CheckExtensionIsShadowedByMethod(checker, thisType->AsETSObjectType(), extensionFunc,
                                             originalExtensionSignature);
        }

        // etsArrayType may extends method from GlobalETSObjectType
        if (thisType->IsETSArrayType()) {
            CheckExtensionIsShadowedByMethod(checker, checker->GlobalETSObjectType(), extensionFunc,
                                             originalExtensionSignature);
        }
    } else {
        checker->LogError(diagnostic::EXTENSION_FUNC_ON_INEXETENSIBLE, {}, node->Start());
    }
}

static void CheckMethodBodyForNativeAbstractDeclare(ETSChecker *checker, ir::MethodDefinition *node,
                                                    ir::ScriptFunction *scriptFunc)
{
    if ((node->IsNative() && !node->IsConstructor()) || node->IsAbstract() || node->IsDeclare()) {
        checker->LogError(diagnostic::UNEXPECTED_FUNC_BODY, {}, scriptFunc->Body()->Start());
    }
}

static void CheckNativeConstructorBody(ETSChecker *checker, ir::MethodDefinition *node, ir::ScriptFunction *scriptFunc)
{
    if (node->IsNative() && node->IsConstructor()) {
        checker->LogError(diagnostic::NATIVE_WITH_BODY, {}, scriptFunc->Body()->Start());
    }
}

void DoBodyTypeChecking(ETSChecker *checker, ir::MethodDefinition *node, ir::ScriptFunction *scriptFunc)
{
    if (scriptFunc->HasBody()) {
        CheckMethodBodyForNativeAbstractDeclare(checker, node, scriptFunc);
        CheckNativeConstructorBody(checker, node, scriptFunc);
    }

    if (scriptFunc->WasChecked() || !scriptFunc->HasBody() ||
        (scriptFunc->IsExternal() && !scriptFunc->IsExternalOverload())) {
        return;
    }

    checker::ScopeContext scopeCtx(checker, scriptFunc->Scope());
    checker::SavedCheckerContext savedContext(checker, checker->Context().Status(),
                                              checker->Context().ContainingClass());
    checker->Context().SetContainingSignature(checker->GetSignatureFromMethodDefinition(node));

    if (node->IsStatic() && !node->IsConstructor() &&
        !checker->Context().ContainingClass()->HasObjectFlag(checker::ETSObjectFlags::GLOBAL)) {
        checker->AddStatus(checker::CheckerStatus::IN_STATIC_CONTEXT);
    }

    if (node->IsConstructor()) {
        checker->AddStatus(checker::CheckerStatus::IN_CONSTRUCTOR);
    }

    if (node->IsExtensionMethod() && scriptFunc->Signature() != nullptr) {
        CheckExtensionMethod(checker, scriptFunc, node);
    }

    scriptFunc->Body()->Check(checker);
    scriptFunc->SetWasChecked();

    if (scriptFunc->ReturnTypeAnnotation() == nullptr) {
        /*
         * NOTE(knazarov): To not break compatibility with existing behaviour,
         * we keep return type of the AsyncImpl methods as Object.
         */
        if (scriptFunc->IsAsyncImplFunc()) {
            const auto oldReturnType = scriptFunc->Signature()->ReturnType();
            const auto newReturnType = checker->CreateETSAsyncFuncReturnTypeFromBaseType(oldReturnType);
            scriptFunc->Signature()->SetReturnType(newReturnType);
        }

        for (auto &returnStatement : scriptFunc->ReturnStatements()) {
            returnStatement->SetReturnType(checker, scriptFunc->Signature()->ReturnType());
        }
    }

    checker->Context().SetContainingSignature(nullptr);
}

void CheckPredefinedMethodReturnType(ETSChecker *checker, ir::ScriptFunction *scriptFunc)
{
    if (scriptFunc->Signature() == nullptr) {
        ES2PANDA_ASSERT(checker->IsAnyError());
        return;
    }

    auto const &position = scriptFunc->Start();

    if (scriptFunc->IsGetter() && (scriptFunc->Signature()->ReturnType() == checker->GlobalVoidType())) {
        checker->LogError(diagnostic::GETTER_VOID, {}, position);
    }

    auto const &name = scriptFunc->Id()->Name();
    auto const methodName = std::string {ir::PREDEFINED_METHOD} + std::string {name.Utf8()};

    if (name.Is(compiler::Signatures::GET_INDEX_METHOD)) {
        if (scriptFunc->Signature()->ReturnType() == checker->GlobalVoidType()) {
            checker->LogError(diagnostic::UNEXPECTED_VOID, {util::StringView(methodName)}, position);
        }
    } else if (name.Is(compiler::Signatures::SET_INDEX_METHOD)) {
        if (scriptFunc->Signature()->ReturnType() != checker->GlobalVoidType()) {
            checker->LogError(diagnostic::UNEXPECTED_NONVOID, {util::StringView(methodName)}, position);
        }
    } else if (name.Is(compiler::Signatures::ITERATOR_METHOD)) {
        CheckIteratorMethodReturnType(checker, scriptFunc, position, methodName);
    }
}

static bool HasIteratorInterface(ETSObjectType const *const objectType)
{
    auto const hasIteratorInterfaceImpl = [](ETSObjectType const *const checkType,
                                             auto &&iteratorInterfaceImpl) -> bool {
        if (checkType->Name().Is(ir::ITERATOR_INTERFACE_NAME)) {
            return true;
        }
        for (const auto *const interface : checkType->Interfaces()) {
            if (iteratorInterfaceImpl(interface, iteratorInterfaceImpl)) {
                return true;
            }
        }
        return false;
    };

    return hasIteratorInterfaceImpl(objectType, hasIteratorInterfaceImpl);
}

void CheckIteratorMethodReturnType(ETSChecker *checker, ir::ScriptFunction *scriptFunc,
                                   const lexer::SourcePosition &position,
                                   [[maybe_unused]] const std::string &methodName)
{
    const auto *returnType = scriptFunc->Signature()->ReturnType();

    if (returnType->IsETSTypeParameter()) {
        returnType = checker->GetApparentType(returnType->AsETSTypeParameter()->GetConstraintType());
    }

    ES2PANDA_ASSERT(returnType != nullptr);
    if (returnType->IsETSObjectType() && HasIteratorInterface(returnType->AsETSObjectType())) {
        return;
    }

    while (returnType->IsETSObjectType() && returnType->AsETSObjectType()->SuperType() != nullptr) {
        returnType = returnType->AsETSObjectType()->SuperType();
        if (returnType->IsETSObjectType() && HasIteratorInterface(returnType->AsETSObjectType())) {
            return;
        }
    }

    checker->LogError(diagnostic::RETURN_ISNT_ITERATOR, {scriptFunc->Id()->Name()}, position);
}

static void SwitchMethodCallToFunctionCall(checker::ETSChecker *checker, ir::CallExpression *expr, Signature *signature)
{
    ES2PANDA_ASSERT(expr->Callee()->IsMemberExpression());
    auto *memberExpr = expr->Callee()->AsMemberExpression();
    if (expr->Arguments().empty() || expr->Arguments()[0] != memberExpr->Object()) {
        expr->Arguments().insert(expr->Arguments().begin(), memberExpr->Object());
    }
    expr->SetSignature(signature);
    expr->SetCallee(memberExpr->Property());
    memberExpr->Property()->AsIdentifier()->SetParent(expr);
    expr->Arguments()[0]->SetParent(expr);
    auto scopeCtx = varbinder::LexicalScope<varbinder::Scope>::Enter(checker->VarBinder(), expr->EnclosingScope(expr));
    checker->HandleUpdatedCallExpressionNode(expr);
    // Set TsType for new Callee(original member expression's Object)
    expr->Callee()->Check(checker);
}

checker::Signature *ResolveCallExtensionFunction(checker::Type *functionType, checker::ETSChecker *checker,
                                                 ir::CallExpression *expr,
                                                 [[maybe_unused]] const TypeRelationFlag reportFlag)
{
    // We have to ways to call ExtensionFunction `function foo(this: A, ...)`:
    // 1. Make ExtensionFunction as FunctionCall: `foo(a,...);`
    // 2. Make ExtensionFunction as MethodCall: `a.foo(...)`, here `a` is the receiver;
    auto &signatures = expr->IsETSConstructorCall()
                           ? functionType->AsETSObjectType()->ConstructSignatures()
                           : functionType->AsETSFunctionType()->CallSignaturesOfMethodOrArrow();
    if (expr->Callee()->IsMemberExpression()) {
        // when handle extension function as MethodCall, we temporarily transfer the expr node from `a.foo(...)` to
        // `foo(a, ...)` and resolve the call. If we find the suitable signature, then switch the expr node to
        // function call.
        auto *memberExpr = expr->Callee()->AsMemberExpression();
        expr->Arguments().insert(expr->Arguments().begin(), memberExpr->Object());
        auto *signature = checker->FirstMatchSignatures(signatures, expr);
        if (signature == nullptr) {
            expr->Arguments().erase(expr->Arguments().begin());
            return nullptr;
        }

        SwitchMethodCallToFunctionCall(checker, expr, signature);
        return signature;
    }

    return checker->FirstMatchSignatures(signatures, expr);
}

checker::Signature *ResolveCallForClassMethod(checker::ETSExtensionFuncHelperType *type, checker::ETSChecker *checker,
                                              ir::CallExpression *expr)
{
    ES2PANDA_ASSERT(expr->Callee()->IsMemberExpression());
    auto signature = checker->FirstMatchSignatures(type->ClassMethodType()->CallSignatures(), expr);
    if (signature != nullptr) {
        auto *memberExpr = expr->Callee()->AsMemberExpression();
        auto *var = type->ClassMethodType()->Variable();
        memberExpr->Property()->AsIdentifier()->SetVariable(var);
    }
    return signature;
}

checker::Signature *GetMostSpecificSigFromExtensionFuncAndClassMethod(checker::ETSExtensionFuncHelperType *type,
                                                                      checker::ETSChecker *checker,
                                                                      ir::CallExpression *expr)
{
    // We try to find the most suitable signature for a.foo(...) both in ExtensionFunctionType and ClassMethodType.
    // So we temporarily transfer expr node from `a.foo(...)` to `a.foo(a, ...)`.
    // For allCallSignatures in ClassMethodType, temporarily insert the dummyReceiver into their signatureInfo,
    // otherwise we can't get the most suitable classMethod signature if all the extensionFunction signature mismatched.
    ArenaVector<Signature *> signatures(checker->ProgramAllocator()->Adapter());
    auto const &classMethodSignatures = type->ClassMethodType()->CallSignatures();
    auto const &extensionMethodSignatures = type->ExtensionMethodType()->CallSignaturesOfMethodOrArrow();

    signatures.insert(signatures.end(), classMethodSignatures.cbegin(), classMethodSignatures.cend());
    signatures.insert(signatures.end(), extensionMethodSignatures.cbegin(), extensionMethodSignatures.cend());

    auto *memberExpr = expr->Callee()->AsMemberExpression();
    auto *dummyReceiver = memberExpr->Object();
    auto *dummyReceiverVar = extensionMethodSignatures[0]->Params()[0];
    expr->Arguments().insert(expr->Arguments().begin(), dummyReceiver);
    const bool typeParamsNeeded = dummyReceiverVar->TsType()->IsETSObjectType();

    for (auto *methodCallSig : classMethodSignatures) {
        methodCallSig->GetSignatureInfo()->minArgCount++;
        auto &paramsVar = methodCallSig->Params();
        paramsVar.insert(paramsVar.begin(), dummyReceiverVar);
        auto &params = methodCallSig->Function()->ParamsForUpdate();
        params.insert(params.begin(), dummyReceiver);
        if (typeParamsNeeded) {
            auto &typeParams = methodCallSig->TypeParams();
            typeParams.insert(typeParams.end(), dummyReceiverVar->TsType()->AsETSObjectType()->TypeArguments().begin(),
                              dummyReceiverVar->TsType()->AsETSObjectType()->TypeArguments().end());
        }
    }

    auto *signature = checker->FirstMatchSignatures(signatures, expr);

    for (auto *methodCallSig : classMethodSignatures) {
        methodCallSig->GetSignatureInfo()->minArgCount--;
        auto &paramsVar = methodCallSig->Params();
        paramsVar.erase(paramsVar.begin());
        auto &params = methodCallSig->Function()->ParamsForUpdate();
        params.erase(params.begin());
        if (typeParamsNeeded) {
            auto &typeParams = methodCallSig->TypeParams();
            typeParams.resize(typeParams.size() -
                              dummyReceiverVar->TsType()->AsETSObjectType()->TypeArguments().size());
        }
    }
    expr->Arguments().erase(expr->Arguments().begin());

    if (signature != nullptr) {
        if (signature->Owner()->GetDeclNode()->IsClassDefinition() &&
            signature->Owner()->GetDeclNode()->AsClassDefinition()->IsGlobal()) {
            SwitchMethodCallToFunctionCall(checker, expr, signature);
        } else {
            auto *var = type->ClassMethodType()->Variable();
            memberExpr->Property()->AsIdentifier()->SetVariable(var);
        }
    }
    return signature;
}

checker::Signature *ResolveCallForETSExtensionFuncHelperType(checker::ETSExtensionFuncHelperType *type,
                                                             checker::ETSChecker *checker, ir::CallExpression *expr)
{
    ES2PANDA_ASSERT(expr->Callee()->IsMemberExpression());
    auto *calleeObj = expr->Callee()->AsMemberExpression()->Object();
    ERROR_SANITY_CHECK(checker, calleeObj->TsType()->IsETSObjectType(), return nullptr);
    bool isCalleeObjETSGlobal = calleeObj->TsType()->AsETSObjectType()->GetDeclNode()->IsClassDefinition() &&
                                calleeObj->TsType()->AsETSObjectType()->GetDeclNode()->AsClassDefinition()->IsGlobal();
    // for callExpr `a.foo`, there are 3 situations:
    // 1.`a.foo` is private method call of class A;
    // 2.`a.foo` is extension function of `A`(function with receiver `A`)
    // 3.`a.foo` is public method call of class A;
    Signature *signature = nullptr;
    if (checker->IsTypeIdenticalTo(checker->Context().ContainingClass(), calleeObj->TsType()) || isCalleeObjETSGlobal) {
        // When called `a.foo` in `a.anotherFunc`, we should find signature through private or protected method firstly.
        signature = ResolveCallForClassMethod(type, checker, expr);
        if (signature != nullptr) {
            UpdateDeclarationFromSignature(checker, expr, signature);
            return signature;
        }
    }

    signature = GetMostSpecificSigFromExtensionFuncAndClassMethod(type, checker, expr);
    if (signature == nullptr) {
        checker->LogSignatureMismatch(type->ExtensionMethodType()->CallSignaturesOfMethodOrArrow(), expr->Arguments(),
                                      expr->Start(), "call");
    }

    UpdateDeclarationFromSignature(checker, expr, signature);
    return signature;
}

ArenaVector<checker::Signature *> GetUnionTypeSignatures(ETSChecker *checker, checker::ETSUnionType *etsUnionType)
{
    ArenaVector<checker::Signature *> callSignatures(checker->ProgramAllocator()->Adapter());

    for (auto *constituentType : etsUnionType->ConstituentTypes()) {
        if (constituentType->IsETSFunctionType()) {
            ArenaVector<checker::Signature *> tmpCallSignatures(checker->ProgramAllocator()->Adapter());
            tmpCallSignatures = constituentType->AsETSFunctionType()->CallSignatures();
            callSignatures.insert(callSignatures.end(), tmpCallSignatures.begin(), tmpCallSignatures.end());
        }
        if (constituentType->IsETSUnionType()) {
            ArenaVector<checker::Signature *> tmpCallSignatures(checker->ProgramAllocator()->Adapter());
            tmpCallSignatures = GetUnionTypeSignatures(checker, constituentType->AsETSUnionType());
            callSignatures.insert(callSignatures.end(), tmpCallSignatures.begin(), tmpCallSignatures.end());
        }
    }

    return callSignatures;
}

void ProcessExclamationMark(ETSChecker *checker, ir::UnaryExpression *expr, checker::Type *operandType)
{
    FORWARD_VALUE_ON_TYPE_ERROR(checker, operandType, expr, EMPTY_VALUE);

    expr->SetTsType(checker->GlobalETSBooleanBuiltinType());
}

void SetTsTypeForUnaryExpression(ETSChecker *checker, ir::UnaryExpression *expr, checker::Type *operandType)
{
    switch (expr->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_MINUS:
        case lexer::TokenType::PUNCTUATOR_PLUS: {
            if (operandType == nullptr || !operandType->IsETSObjectType() ||
                !operandType->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::CONVERTIBLE_TO_NUMERIC)) {
                checker->LogError(diagnostic::OPERAND_NOT_NUMERIC, {}, expr->Argument()->Start());
                expr->SetTsType(checker->GlobalTypeError());
                break;
            }

            expr->SetTsType(operandType);
            break;
        }
        case lexer::TokenType::PUNCTUATOR_TILDE: {
            if (operandType == nullptr || !operandType->IsETSObjectType() ||
                !operandType->AsETSObjectType()->HasObjectFlag(checker::ETSObjectFlags::CONVERTIBLE_TO_NUMERIC)) {
                checker->LogError(diagnostic::OPERAND_NOT_NUMERIC, {}, expr->Argument()->Start());
                expr->SetTsType(checker->GlobalTypeError());
                break;
            }
            auto exprType = expr->SetTsType(checker->SelectGlobalIntegerTypeForNumeric(operandType));
            if (!expr->Argument()->TsType()->IsETSNumericEnumType()) {
                expr->Argument()->SetTsType(exprType);
            }
            break;
        }
        case lexer::TokenType::PUNCTUATOR_EXCLAMATION_MARK: {
            ProcessExclamationMark(checker, expr, operandType);
            break;
        }
        default: {
            ES2PANDA_UNREACHABLE();
        }
    }
}

checker::Type *GetIteratorType(ETSChecker *checker, checker::Type *elemType, ir::AstNode *left)
{
    // Just to avoid extra nested level(s)
    // CC-OFFNXT(G.FMT.14-CPP) project code style
    auto const getIterType = [checker, elemType](ir::VariableDeclarator *const declarator) -> checker::Type * {
        if (declarator->TsType() == nullptr) {
            if (auto *resolved = checker->FindVariableInFunctionScope(declarator->Id()->AsIdentifier()->Name());
                resolved != nullptr) {
                resolved->SetTsType(elemType);
                return elemType;
            }
        } else {
            return declarator->TsType();
        }
        return checker->GlobalTypeError();
    };

    checker::Type *iterType = nullptr;
    if (left->IsIdentifier()) {
        auto *const variable = left->Variable();
        ES2PANDA_ASSERT(variable != nullptr && variable->Declaration() != nullptr);

        auto *decl = variable->Declaration();
        if (decl->IsConstDecl() || decl->IsReadonlyDecl()) {
            const auto &errorMsg =
                decl->IsConstDecl() ? diagnostic::INVALID_CONST_ASSIGNMENT : diagnostic::INVALID_READONLY_ASSIGNMENT;
            checker->LogError(errorMsg, {variable->Name()}, decl->Node()->Start());
        }
        iterType = left->AsIdentifier()->TsType();
    } else if (left->IsVariableDeclaration()) {
        if (auto const &declarators = left->AsVariableDeclaration()->Declarators(); !declarators.empty()) {
            iterType = getIterType(declarators.front());
        }
    }

    if (iterType == nullptr) {
        checker->LogError(diagnostic::ITERATOR_TYPE_ABSENT, {}, left->Start());
        return checker->GlobalTypeError();
    }
    return checker->GetNonConstantType(iterType);
}

bool CheckArgumentVoidType(checker::Type *funcReturnType, ETSChecker *checker, const std::string &name,
                           ir::ReturnStatement *st)
{
    if (name.find(compiler::Signatures::ETS_MAIN_WITH_MANGLE_BEGIN) != std::string::npos) {
        if (!funcReturnType->IsETSVoidType() &&
            !checker->Relation()->IsSupertypeOf(checker->GlobalIntBuiltinType(), funcReturnType)) {
            checker->LogError(diagnostic::MAIN_BAD_RETURN, {}, st->Start());
        }
    }
    return true;
}

static bool CheckAsyncReturnType(ETSChecker *checker, checker::Type *funcReturnType, checker::Type *argumentType,
                                 ir::Expression *stArgument)
{
    auto const checkPromiseType = [checker, argumentType, stArgument](checker::Type *type) -> bool {
        if (type->IsETSObjectType() && checker->IsPromiseType(type->AsETSObjectType())) {
            auto promiseArg = type->AsETSObjectType()->TypeArguments()[0];
            checker::AssignmentContext(checker->Relation(), stArgument, argumentType, promiseArg, stArgument->Start(),
                                       std::nullopt, checker::TypeRelationFlag::DIRECT_RETURN);
            if (checker->Relation()->IsTrue()) {
                return true;
            }
        }
        return false;
    };

    if (funcReturnType->IsETSObjectType()) {
        if (checkPromiseType(funcReturnType)) {
            return true;
        }
    } else if (funcReturnType->IsETSUnionType()) {
        for (auto *type : funcReturnType->AsETSUnionType()->ConstituentTypes()) {
            if (checkPromiseType(type)) {
                return true;
            }
        }
    }
    return false;
}

bool CheckReturnType(ETSChecker *checker, checker::Type *funcReturnType, checker::Type *argumentType,
                     ir::Expression *stArgument, ir::ScriptFunction *containingFunc)
{
    if (containingFunc->IsDeclaredAsync() && CheckAsyncReturnType(checker, funcReturnType, argumentType, stArgument)) {
        return true;
    }

    if (!checker::AssignmentContext(checker->Relation(), stArgument, argumentType, funcReturnType, stArgument->Start(),
                                    std::nullopt, checker::TypeRelationFlag::DIRECT_RETURN)
             // CC-OFFNXT(G.FMT.02) project code style
             .IsAssignable()) {
        checker->LogError(diagnostic::ARROW_TYPE_MISMATCH, {argumentType, funcReturnType}, stArgument->Start());
        return false;
    }
    return true;
}

bool HasSingleReturnStatement(const ir::AstNode *node)
{
    auto count = 0;
    node->Iterate([&count](const ir::AstNode *child) {
        if (child->IsReturnStatement()) {
            count++;
        }
        count += HasSingleReturnStatement(child) ? 1 : 0;
    });
    return count == 1;
}

checker::Type *InferReturnType(ETSChecker *checker, ir::ScriptFunction *containingFunc, ir::Expression *stArgument)
{
    ES2PANDA_ASSERT(containingFunc->Signature()->HasSignatureFlag(checker::SignatureFlags::NEED_RETURN_TYPE));
    ES2PANDA_ASSERT(containingFunc->ReturnTypeAnnotation() == nullptr);

    //  First (or single) return statement in the function:
    const auto baseFuncReturnType =
        stArgument ? checker->GetNormalizedType(checker->GetNonConstantType(stArgument->Check(checker)))
                   : checker->GlobalVoidType();
    /**
     * Spec 15.7.2 If a function, a method, or a lambda is async (see Asynchronous execution), a return
     * type is inferred by applying the above rules, and the return type T is not Promise, then the
     * return type is assumed to be Promise<T>.
     */
    const auto funcReturnType = containingFunc->IsAsyncFunc() && !checker->IsPromiseType(baseFuncReturnType)
                                    ? checker->CreatePromiseOf(baseFuncReturnType)
                                    : baseFuncReturnType;

    containingFunc->Signature()->RemoveSignatureFlag(checker::SignatureFlags::NEED_RETURN_TYPE);

    ES2PANDA_ASSERT(funcReturnType != nullptr);
    if (funcReturnType->IsTypeError()) {
        return funcReturnType;
    }

    /*
    when st_argment is ArrowFunctionExpression, need infer type for st_argment
    example code:
    ```
    return () => {}
    ```
    */

    containingFunc->Signature()->SetReturnType(funcReturnType);
    if (stArgument != nullptr && stArgument->IsIdentifier() && stArgument->OriginalNode() != nullptr &&
        stArgument->OriginalNode()->IsThisExpression() && containingFunc->IsMethod()) {
        containingFunc->Signature()->AddSignatureFlag(SignatureFlags::THIS_RETURN_TYPE);
    } else {
        containingFunc->Signature()->AddSignatureFlag(checker::SignatureFlags::INFERRED_RETURN_TYPE);
    }
    checker->VarBinder()->AsETSBinder()->BuildFunctionName(containingFunc);

    if (stArgument != nullptr) {
        stArgument->SetPreferredType(funcReturnType);
    }

    return funcReturnType;
}

bool IsArrayExpressionValidInitializerForType(ETSChecker *checker, const Type *const arrayExprPreferredType)
{
    const auto validForTarget = arrayExprPreferredType == nullptr  // preferred type will be inferred from elements
                                || arrayExprPreferredType->IsAnyETSArrayOrTupleType()  // valid for array or tuple types
                                || checker->Relation()->IsSupertypeOf(arrayExprPreferredType,  // valid for 'Object'
                                                                      checker->GlobalETSObjectType());

    return validForTarget;
}

checker::Type *ProcessReturnStatements(ETSChecker *checker, ir::ScriptFunction *containingFunc, ir::ReturnStatement *st,
                                       ir::Expression *stArgument)
{
    auto *const relation = checker->Relation();
    const auto funcReturnType = containingFunc->Signature()->ReturnType();
    const auto voidType = containingFunc->IsDeclaredAsync() ? checker->CreatePromiseOf(checker->GlobalVoidType())
                                                            : checker->GlobalVoidType();
    const auto isReturnVoid = relation->IsIdenticalTo(funcReturnType, voidType);

    if (stArgument == nullptr) {
        // previous return statement(s) have value
        if (!isReturnVoid) {
            checker->LogError(diagnostic::MIXED_VOID_NONVOID, {}, st->Start());
            return funcReturnType;
        }
    } else {
        if (stArgument->IsObjectExpression()) {
            stArgument->AsObjectExpression()->SetPreferredType(funcReturnType);
        }

        if (stArgument->IsMemberExpression()) {
            checker->SetArrayPreferredTypeForNestedMemberExpressions(stArgument->AsMemberExpression(), funcReturnType);
        }

        checker::Type *argumentType = checker->GetNonConstantType(stArgument->Check(checker));

        // previous return statement(s) don't have any value
        ES2PANDA_ASSERT(argumentType != nullptr);
        /*
         * NOTE(knazarov): Workaround for Promise<void> being initialized with
         * undefined. Should become IsIdenticalTo after #32490 is done.
         */
        if (isReturnVoid && !relation->IsSupertypeOf(voidType, argumentType)) {
            checker->LogError(diagnostic::MIXED_VOID_NONVOID, {}, stArgument->Start());
            return funcReturnType;
        }

        const auto name = containingFunc->Scope()->InternalName().Mutf8();
        if (!CheckArgumentVoidType(funcReturnType, checker, name, st)) {
            return funcReturnType;
        }

        relation->SetNode(stArgument);

        if (!relation->IsIdenticalTo(funcReturnType, argumentType)) {
            checker->ResolveReturnStatement(containingFunc, argumentType, st);
        }

        relation->SetNode(nullptr);
        relation->SetFlags(checker::TypeRelationFlag::NONE);
    }

    return containingFunc->Signature()->ReturnType();
}

bool CheckReturnTypeNecessity(ir::MethodDefinition *node)
{
    bool needReturnType = true;
    auto *scriptFunc = node->Function();
    needReturnType &= (node->IsNative() || node->IsDeclare());
    needReturnType &= !node->IsConstructor();
    ES2PANDA_ASSERT(scriptFunc != nullptr);
    needReturnType &= !scriptFunc->IsSetter();
    return needReturnType;
}

void CheckAllConstPropertyInitialized(checker::ETSChecker *checker, ir::ETSModule *pkg)
{
    auto globalDecl = std::find_if(pkg->Statements().begin(), pkg->Statements().end(), [](ir::AstNode *node) {
        return node->IsClassDeclaration() && node->AsClassDeclaration()->Definition()->IsGlobal();
    });
    if (globalDecl == pkg->Statements().end()) {
        return;
    }

    auto const &globalClassBody = (*globalDecl)->AsClassDeclaration()->Definition()->AsClassDefinition()->Body();
    for (auto const *prop : globalClassBody) {
        if (!prop->IsClassProperty()) {
            continue;
        }

        auto *classProp = prop->AsClassProperty();
        if (classProp->Key()->Variable() == nullptr) {
            continue;
        }

        if (classProp->Key()->Variable()->HasFlag(varbinder::VariableFlags::INIT_IN_STATIC_BLOCK) &&
            !classProp->Key()->Variable()->HasFlag(varbinder::VariableFlags::INITIALIZED)) {
            checker->LogError(diagnostic::MISSING_INIT_FOR_CONST_PACKAGE_PROP, {}, prop->Start());
        }
    }
}

// NOLINTBEGIN(readability-else-after-return)
std::tuple<bool, bool> IsConstantTestValue(ir::Expression const *expr)
{
    if (expr->IsNullLiteral() || expr->IsUndefinedLiteral()) {
        return {true, false};
    } else if (expr->IsBooleanLiteral()) {
        return {true, expr->AsBooleanLiteral()->Value()};
    } else if (expr->IsStringLiteral()) {
        return {true, expr->AsStringLiteral()->Str().Length() != 0};
    } else if (expr->IsCharLiteral()) {
        return {true, expr->AsCharLiteral()->Char() != 0};
    } else if (expr->IsBigIntLiteral()) {
        return {true, expr->AsBigIntLiteral()->Str() != "0"};
    } else if (expr->IsNumberLiteral()) {
        auto num = expr->AsNumberLiteral()->Number();
        return {true, !num.IsZero()};
    } else if (expr->TsType()->IsETSEnumType() && expr->TsType()->IsConstantType()) {
        // NOTE(gogabr): Should handle enum constants
        return {false, false};
    }
    return {false, false};
}
// NOLINTEND(readability-else-after-return)

static bool IsNullishComparisonOperand(const ir::Expression *const expr, bool &compareAgainstNull)
{
    if (expr->IsNullLiteral()) {
        compareAgainstNull = true;
        return true;
    }
    if (expr->IsUndefinedLiteral()) {
        compareAgainstNull = false;
        return true;
    }

    auto *const exprType = expr->TsType();
    if (exprType == nullptr) {
        return false;
    }
    if (exprType->IsETSNullType()) {
        compareAgainstNull = true;
        return true;
    }
    if (exprType->IsETSUndefinedType()) {
        compareAgainstNull = false;
        return true;
    }

    return false;
}

static bool ExtractNullishComparisonOperands(const ir::BinaryExpression *binary, const ir::Expression *&checkedExpr,
                                             bool &compareAgainstNull)
{
    if (IsNullishComparisonOperand(binary->Left(), compareAgainstNull)) {
        checkedExpr = binary->Right();
        return true;
    }

    if (IsNullishComparisonOperand(binary->Right(), compareAgainstNull)) {
        checkedExpr = binary->Left();
        return true;
    }

    return false;
}

static std::optional<bool> EvaluateNullishEqualityTypeResult(const Type *checkedType, bool compareAgainstNull,
                                                             bool strict)
{
    if (!strict) {
        if (checkedType->DefinitelyNotETSNullish()) {
            return false;
        }
        if (checkedType->DefinitelyETSNullish()) {
            return true;
        }
        return std::nullopt;
    }

    if (compareAgainstNull) {
        if (checkedType->IsETSNullType()) {
            return true;
        }
        if (checkedType->DefinitelyNotETSNullish() || checkedType->IsETSUndefinedType()) {
            return false;
        }
        return std::nullopt;
    }

    if (checkedType->IsETSUndefinedType()) {
        return true;
    }
    if (checkedType->DefinitelyNotETSNullish() || checkedType->IsETSNullType()) {
        return false;
    }

    return std::nullopt;
}

static std::optional<bool> TryResolveNullishEqualityTestValue(const ir::Expression *const test)
{
    ES2PANDA_ASSERT(test != nullptr);

    if (!test->IsBinaryExpression()) {
        return std::nullopt;
    }

    auto *const binary = test->AsBinaryExpression();
    if (!binary->IsEquality()) {
        return std::nullopt;
    }

    bool strict = false;
    bool negate = false;
    switch (binary->OperatorType()) {
        case lexer::TokenType::PUNCTUATOR_EQUAL:
            break;
        case lexer::TokenType::PUNCTUATOR_NOT_EQUAL:
            negate = true;
            break;
        case lexer::TokenType::PUNCTUATOR_STRICT_EQUAL:
            strict = true;
            break;
        case lexer::TokenType::PUNCTUATOR_NOT_STRICT_EQUAL:
            strict = true;
            negate = true;
            break;
        default:
            ES2PANDA_UNREACHABLE();
    }

    const ir::Expression *checkedExpr = nullptr;
    bool compareAgainstNull = false;
    if (!ExtractNullishComparisonOperands(binary, checkedExpr, compareAgainstNull)) {
        return std::nullopt;
    }

    auto *const checkedType = checkedExpr->TsType();
    if (checkedType == nullptr) {
        return std::nullopt;
    }
    auto const result = EvaluateNullishEqualityTypeResult(checkedType, compareAgainstNull, strict);
    if (!result.has_value()) {
        return std::nullopt;
    }

    return negate ? !result.value() : result.value();
}

static bool IsValueBasedTruthinessType(const Type *const conditionType)
{
    ES2PANDA_ASSERT(conditionType != nullptr);

    if (conditionType->IsETSPrimitiveOrEnumType()) {
        return true;
    }

    return conditionType->IsETSObjectType() &&
           conditionType->AsETSObjectType()->HasObjectFlag(ETSObjectFlags::VALUE_TYPED);
}

static std::optional<bool> TryResolveTypeKnownTruthiness(ETSChecker *const checker, const ir::Expression *const test)
{
    ES2PANDA_ASSERT(checker != nullptr);
    ES2PANDA_ASSERT(test != nullptr);

    auto *const testType = const_cast<Type *>(test->TsType());
    if (testType == nullptr) {
        return std::nullopt;
    }
    auto *const conditionType = checker->MaybeUnboxConditionalInRelation(testType);
    ES2PANDA_ASSERT(conditionType != nullptr);

    if (checker->IsNullLikeOrVoidExpression(test)) {
        return false;
    }

    auto [resolved, value] = IsConstantTestValue(test);
    if (resolved) {
        return value;
    }

    if (test->IsBinaryExpression()) {
        return std::nullopt;
    }

    if (conditionType->IsETSBooleanType() || conditionType->IsBooleanLiteralType()) {
        return std::nullopt;
    }

    if (testType->DefinitelyNotETSNullish() && !IsValueBasedTruthinessType(conditionType)) {
        return true;
    }

    return std::nullopt;
}

std::optional<bool> TryResolveConditionalTestValue(ETSChecker *const checker, const ir::Expression *const test)
{
    if (auto const value = TryResolveTypeKnownTruthiness(checker, test); value.has_value()) {
        return value;
    }

    return TryResolveNullishEqualityTestValue(test);
}

void UpdateDeclarationFromSignature(ETSChecker *checker, ir::CallExpression *expr, checker::Signature *signature)
{
    if (signature == nullptr) {
        return;
    }

    ir::AstNode *callIdentifier = expr->Callee();
    while (callIdentifier != nullptr && callIdentifier->IsMemberExpression()) {
        callIdentifier = callIdentifier->AsMemberExpression()->Property();
    }
    if (callIdentifier == nullptr || !callIdentifier->IsIdentifier()) {
        return;
    }

    auto signatureVar = callIdentifier->Variable();
    if (signatureVar == nullptr || !signatureVar->HasFlag(varbinder::VariableFlags::METHOD) ||
        !signature->HasFunction() || signature->Function()->IsDynamic()) {
        return;
    }

    auto sigName = signature->Function()->Id()->Name();
    if (callIdentifier->AsIdentifier()->Name() != sigName) {
        return;
    }

    ir::AstNode *declNode = signature->Function();
    while (!declNode->IsMethodDefinition()) {
        declNode = declNode->Parent();
    }
    auto allocator = checker->ProgramAllocator();
    auto newDecl = allocator->New<varbinder::FunctionDecl>(allocator, sigName, declNode);
    auto newVar = allocator->New<varbinder::LocalVariable>(newDecl, varbinder::VariableFlags::METHOD |
                                                                        varbinder::VariableFlags::SYNTHETIC);
    newVar->SetTsType(declNode->AsMethodDefinition()->TsType()->Clone(checker));
    callIdentifier->SetVariable(newVar);
}

}  // namespace ark::es2panda::checker
