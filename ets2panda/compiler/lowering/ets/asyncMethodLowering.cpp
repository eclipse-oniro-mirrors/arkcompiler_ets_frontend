/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "asyncMethodLowering.h"

#include "checker/ETSchecker.h"
#include "checker/types/ets/etsAsyncFuncReturnType.h"
#include "compiler/lowering/util.h"

namespace ark::es2panda::compiler {

std::string_view AsyncMethodLowering::Name() const
{
    return "AsyncMethodLowering";
}

static void CreateFuncDecl(checker::ETSChecker *checker, ir::MethodDefinition *func, varbinder::LocalScope *scope)
{
    auto *allocator = checker->Allocator();
    auto *varBinder = checker->VarBinder();
    // Add the function declarations to the lambda class scope
    auto ctx = varbinder::LexicalScope<varbinder::LocalScope>::Enter(varBinder, scope);
    ES2PANDA_ASSERT(func->Id() != nullptr);
    varbinder::Variable *var = scope->FindLocal(func->Id()->Name(), varbinder::ResolveBindingOptions::ALL_DECLARATION);
    if (var == nullptr) {
        var = std::get<1>(
            varBinder->NewVarDecl<varbinder::FunctionDecl>(func->Id()->Start(), allocator, func->Id()->Name(), func));
    }
    var->AddFlag(varbinder::VariableFlags::METHOD);
    var->SetScope(ctx.GetScope());
    func->Function()->Id()->SetVariable(var);
}

ir::ETSTypeReference *CreateAsyncImplMethodReturnTypeAnnotation(checker::ETSChecker *checker,
                                                                ir::ScriptFunction *asyncFunc)
{
    // Set impl method return type "Object" because it may return Promise as well as Promise parameter's type
    auto *objectId =
        checker->AllocNode<ir::Identifier>(compiler::Signatures::BUILTIN_OBJECT_CLASS, checker->Allocator());
    checker->VarBinder()->AsETSBinder()->LookupTypeReference(objectId);

    auto *returnTypeAnn = checker->AllocNode<ir::ETSTypeReference>(
        checker->AllocNode<ir::ETSTypeReferencePart>(objectId, nullptr, nullptr, checker->Allocator()),
        checker->Allocator());
    ES2PANDA_ASSERT(returnTypeAnn != nullptr);
    objectId->SetParent(returnTypeAnn->Part());
    returnTypeAnn->Part()->SetParent(returnTypeAnn);

    auto *asyncFuncRetTypeAnn = asyncFunc->ReturnTypeAnnotation();
    auto *promiseType = [checker](ir::TypeNode *type) {
        if (type != nullptr) {
            return type->GetType(checker)->AsETSObjectType();
        }
        return checker->GlobalBuiltinPromiseType()->AsETSObjectType();
    }(asyncFuncRetTypeAnn);
    auto *retType = checker->CreateETSAsyncFuncReturnTypeFromPromiseType(promiseType);
    returnTypeAnn->SetTsType(retType);
    return returnTypeAnn;
}

ir::MethodDefinition *CreateAsyncImplMethod(checker::ETSChecker *checker, ir::MethodDefinition *asyncMethod,
                                            ir::ClassDefinition *classDef)
{
    util::UString implName(checker->GetAsyncImplName(asyncMethod), checker->Allocator());
    ir::ModifierFlags modifiers = asyncMethod->Modifiers();
    // clear ASYNC flag for implementation
    modifiers &= ~ir::ModifierFlags::ASYNC;
    ir::ScriptFunction *asyncFunc = asyncMethod->Function();
    ES2PANDA_ASSERT(asyncFunc != nullptr);
    ir::ScriptFunctionFlags flags = ir::ScriptFunctionFlags::METHOD;

    if (asyncFunc->IsProxy()) {
        flags |= ir::ScriptFunctionFlags::PROXY;
    }

    if (asyncFunc->HasReturnStatement()) {
        flags |= ir::ScriptFunctionFlags::HAS_RETURN;
    }

    asyncMethod->AddModifier(ir::ModifierFlags::NATIVE);
    asyncFunc->AddModifier(ir::ModifierFlags::NATIVE);
    // Create async_impl method copied from CreateInvokeFunction
    auto scopeCtx =
        varbinder::LexicalScope<varbinder::ClassScope>::Enter(checker->VarBinder(), classDef->Scope()->AsClassScope());
    auto *body = asyncFunc->Body();
    ArenaVector<ir::Expression *> params(checker->Allocator()->Adapter());
    ArenaUnorderedMap<varbinder::Variable *, varbinder::Variable *> oldParam2NewParamMap(
        checker->Allocator()->Adapter());
    varbinder::FunctionParamScope *paramScope = checker->CopyParams(asyncFunc->Params(), params, &oldParam2NewParamMap);
    body->IterateRecursively([&oldParam2NewParamMap](ir::AstNode *astNode) -> void {
        if (oldParam2NewParamMap.find(astNode->Variable()) != oldParam2NewParamMap.end()) {
            astNode->SetVariable(oldParam2NewParamMap.at(astNode->Variable()));
        }
    });

    ir::ETSTypeReference *returnTypeAnn = nullptr;

    if (!asyncFunc->Signature()->HasSignatureFlag(checker::SignatureFlags::INFERRED_RETURN_TYPE)) {
        returnTypeAnn = CreateAsyncImplMethodReturnTypeAnnotation(checker, asyncFunc);
    }  // NOTE(vpukhov): #19874 - returnTypeAnn is not set

    ir::MethodDefinition *implMethod =
        checker->CreateMethod(implName.View(), modifiers, flags, std::move(params), paramScope, returnTypeAnn, body);
    asyncFunc->SetBody(nullptr);

    if (returnTypeAnn != nullptr) {
        returnTypeAnn->SetParent(implMethod->Function());
    }

    implMethod->Function()->AddFlag(ir::ScriptFunctionFlags::ASYNC_IMPL);
    implMethod->SetParent(asyncMethod->Parent());
    return implMethod;
}

ir::MethodDefinition *CreateAsyncProxy(checker::ETSChecker *checker, ir::MethodDefinition *asyncMethod,
                                       ir::ClassDefinition *classDef)
{
    ir::ScriptFunction *asyncFunc = asyncMethod->Function();
    ES2PANDA_ASSERT(asyncFunc != nullptr);

    ir::MethodDefinition *implMethod = CreateAsyncImplMethod(checker, asyncMethod, classDef);
    ES2PANDA_ASSERT(implMethod != nullptr && implMethod->Function() != nullptr && implMethod->Id() != nullptr);
    varbinder::FunctionScope *implFuncScope = implMethod->Function()->Scope();
    for (auto *decl : asyncFunc->Scope()->Decls()) {
        auto res = asyncFunc->Scope()->Bindings().find(decl->Name());
        ES2PANDA_ASSERT(res != asyncFunc->Scope()->Bindings().end());
        auto *const var = std::get<1>(*res);
        var->SetScope(implFuncScope);
        implFuncScope->Decls().push_back(decl);
        implFuncScope->InsertBinding(decl->Name(), var);
    }

    checker->ReplaceScope(implMethod->Function()->Body(), asyncFunc, implFuncScope);

    bool isStatic = asyncMethod->IsStatic();
    if (isStatic) {
        CreateFuncDecl(checker, implMethod, classDef->Scope()->AsClassScope()->StaticMethodScope());
    } else {
        CreateFuncDecl(checker, implMethod, classDef->Scope()->AsClassScope()->InstanceMethodScope());
    }
    implMethod->Id()->SetVariable(implMethod->Function()->Id()->Variable());

    checker->VarBinder()->AsETSBinder()->BuildProxyMethod(implMethod->Function(), classDef->InternalName(),
                                                          asyncFunc->IsExternal());
    implMethod->SetParent(asyncMethod->Parent());

    return implMethod;
}

void ComposeAsyncImplMethod(checker::ETSChecker *checker, ir::MethodDefinition *node)
{
    ES2PANDA_ASSERT(checker->FindAncestorGivenByType(node, ir::AstNodeType::CLASS_DEFINITION));
    auto *classDef = checker->FindAncestorGivenByType(node, ir::AstNodeType::CLASS_DEFINITION)->AsClassDefinition();
    ir::MethodDefinition *implMethod = CreateAsyncProxy(checker, node, classDef);

    implMethod->Check(checker);
    node->SetAsyncPairMethod(implMethod);
    node->Function()->SetAsyncPairMethod(implMethod->Function());

    ES2PANDA_ASSERT(node->Function() != nullptr);
    if (node->Function()->IsOverload() && node->BaseOverloadMethod()->AsyncPairMethod() != nullptr) {
        auto *baseOverloadImplMethod = node->BaseOverloadMethod()->AsyncPairMethod();
        ES2PANDA_ASSERT(implMethod->Function() != nullptr && baseOverloadImplMethod->Function() != nullptr);
        implMethod->Function()->Id()->SetVariable(baseOverloadImplMethod->Function()->Id()->Variable());
        baseOverloadImplMethod->AddOverload(implMethod);
        implMethod->SetParent(baseOverloadImplMethod);
    } else if (node->Function()->IsOverload() && node->BaseOverloadMethod()->AsyncPairMethod() == nullptr) {
        // If it's base overload function doesnot marked as async,
        // then current AsyncImpl should be treated as AsyncPairMethod in base overload.
        node->BaseOverloadMethod()->SetAsyncPairMethod(implMethod);
        classDef->EmplaceBody(implMethod);
    } else {
        classDef->EmplaceBody(implMethod);
    }
}

void HandleMethod(checker::ETSChecker *checker, ir::MethodDefinition *node)
{
    ES2PANDA_ASSERT(!node->TsType()->IsTypeError());
    if (util::Helpers::IsAsyncMethod(node) && node->Function() != nullptr && !node->Function()->IsExternal()) {
        ComposeAsyncImplMethod(checker, node);
    }

    for (auto overload : node->Overloads()) {
        HandleMethod(checker, overload);
    }
}

void UpdateClassDefintion(checker::ETSChecker *checker, ir::ClassDefinition *classDef)
{
    checker::SavedCheckerContext savedContext(checker, checker->Context().Status(),
                                              classDef->TsType()->AsETSObjectType());
    for (auto *it : classDef->Body()) {
        if (it->IsMethodDefinition()) {
            HandleMethod(checker, it->AsMethodDefinition());
        }
    }
}

bool AsyncMethodLowering::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    checker::ETSChecker *const checker = ctx->GetChecker()->AsETSChecker();

    ir::NodeTransformer handleClassAsyncMethod = [checker](ir::AstNode *const ast) {
        if (ast->IsClassDefinition()) {
            UpdateClassDefintion(checker, ast->AsClassDefinition());
        }
        return ast;
    };

    program->Ast()->TransformChildrenRecursively(handleClassAsyncMethod, Name());

    return true;
}
}  // namespace ark::es2panda::compiler
