/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "lambdaLowering.h"
#include <sstream>

#include "checker/types/ets/etsTupleType.h"
#include "checker/ets/typeRelationContext.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "compiler/lowering/util.h"
#include "util/options.h"
#include "util/nameMangler.h"

namespace ark::es2panda::compiler {

struct LambdaInfo {
    ir::ClassDeclaration *calleeClass = nullptr;
    ir::TSInterfaceDeclaration *calleeInterface = nullptr;
    ir::ScriptFunction *enclosingFunction = nullptr;
    util::StringView name = "";
    util::StringView originalFuncName = "";
    ArenaSet<varbinder::Variable *> *capturedVars = nullptr;
    ir::Expression *callReceiver = nullptr;
    bool isFunctionReference = false;
    checker::ETSObjectType *objType = nullptr;
};

struct CalleeMethodInfo {
    util::StringView calleeName;
    ir::AstNode *body = nullptr;
    checker::Type *forcedReturnType = nullptr;
    ir::ModifierFlags auxModifierFlags = ir::ModifierFlags::NONE;
    ir::ScriptFunctionFlags auxFunctionFlags = ir::ScriptFunctionFlags::NONE;
};

struct LambdaClassInvokeInfo {
    checker::Signature *lambdaSignature = nullptr;
    ir::MethodDefinition *callee = nullptr;
    ir::ClassDefinition *classDefinition = nullptr;
    checker::Substitution *substitution = nullptr;
    size_t arity = 0;
    util::StringView restParameterIdentifier = "";
    util::StringView restArgumentIdentifier = "";
    ArenaVector<util::UString> *argNames = nullptr;
};

static std::pair<ir::ClassDeclaration *, ir::ScriptFunction *> FindEnclosingClassAndFunction(ir::AstNode *ast)
{
    ir::ScriptFunction *function = nullptr;
    for (ir::AstNode *curr = ast->Parent(); curr != nullptr; curr = curr->Parent()) {
        if (curr->IsClassDeclaration()) {
            return {curr->AsClassDeclaration(), function};
        }
        if (curr->IsScriptFunction()) {
            function = curr->AsScriptFunction();
        }
    }
    ES2PANDA_UNREACHABLE();
}

static bool CheckIfNeedThis(ir::ArrowFunctionExpression *lambda, checker::ETSChecker *checker)
{
    auto *lambdaClass = ContainingClass(lambda);
    return lambda->IsAnyChild([&checker, &lambdaClass](ir::AstNode *ast) {
        return (ast->IsThisExpression() || ast->IsSuperExpression()) &&
               checker->Relation()->IsIdenticalTo(lambdaClass, ContainingClass(ast));
    });
}

static size_t g_calleeCount = 0;
static std::mutex g_calleeCountMutex {};

// Make calleeCount behaviour predictable
static void ResetCalleeCount()
{
    std::lock_guard lock(g_calleeCountMutex);
    g_calleeCount = 0;
}

static util::StringView CreateCalleeName(ArenaAllocator *allocator)
{
    std::lock_guard lock(g_calleeCountMutex);
    auto name = util::UString(
        util::StringView(util::NameMangler::GetInstance()->CreateMangledNameForLambdaInvoke(g_calleeCount++)),
        allocator);
    return name.View();
}

static void ProcessTypeParameterProperties(checker::ETSTypeParameter *oldTypeParam,
                                           checker::ETSTypeParameter *newTypeParam,
                                           ir::TSTypeParameter *newTypeParamNode, checker::Substitution *substitution,
                                           public_lib::Context *ctx)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *allocator = ctx->allocator;
    if (auto *oldConstraint = oldTypeParam->GetConstraintType(); oldConstraint != nullptr) {
        auto *newConstraint = oldConstraint->Substitute(checker->Relation(), substitution);
        newTypeParam->SetConstraintType(newConstraint);
        auto *newConstraintNode = allocator->New<ir::OpaqueTypeNode>(newConstraint, allocator);
        ES2PANDA_ASSERT(newConstraintNode != nullptr);
        newTypeParamNode->SetConstraint(newConstraintNode);
        newConstraintNode->SetParent(newTypeParamNode);
    }

    if (auto *oldDefault = oldTypeParam->GetDefaultType(); oldDefault != nullptr) {
        auto *newDefault = oldDefault->Substitute(checker->Relation(), substitution);
        newTypeParam->SetDefaultType(newDefault);
        auto *newDefaultNode = allocator->New<ir::OpaqueTypeNode>(newDefault, allocator);
        ES2PANDA_ASSERT(newDefaultNode != nullptr);
        newTypeParamNode->SetDefaultType(newDefaultNode);
        newDefaultNode->SetParent(newTypeParamNode);
    }
}

// NOTE (smartin): The two methods 'CreateNewTypeParamVectors' and 'SetConstraintTypeAndDefaultTypeForTypeParams'
// contain very similar logic that can be found in 'CloneTypeParamsForClass'. Merge these later if possible
static void FillNewTypeParamVectors(ThreadSafeArenaAllocator *allocator,
                                    ArenaVector<checker::ETSTypeParameter *> &newTypeParams,
                                    ArenaVector<ir::TSTypeParameter *> &newTypeParamNodes,
                                    const checker::Signature *const lambdaSig,
                                    checker::Substitution *const substitution)
{
    for (auto *ix : lambdaSig->TypeParams()) {
        auto *oldTypeParam = ix->AsETSTypeParameter();
        auto *newTypeParamId = allocator->New<ir::Identifier>(oldTypeParam->Name(), allocator);
        auto *newTypeParamNode = util::NodeAllocator::ForceSetParent<ir::TSTypeParameter>(allocator, newTypeParamId,
                                                                                          nullptr, nullptr, allocator);
        auto *newTypeParam = allocator->New<checker::ETSTypeParameter>();
        newTypeParam->SetDeclNode(newTypeParamNode);

        auto *newTypeParamDecl = allocator->New<varbinder::TypeParameterDecl>(newTypeParamId->Name());
        newTypeParamDecl->BindNode(newTypeParamNode);
        auto *newTypeParamVar =
            allocator->New<varbinder::LocalVariable>(newTypeParamDecl, varbinder::VariableFlags::TYPE_PARAMETER);

        newTypeParamVar->SetTsType(newTypeParam);
        newTypeParamId->SetVariable(newTypeParamVar);

        newTypeParams.push_back(newTypeParam);
        newTypeParamNodes.push_back(newTypeParamNode);
        substitution->emplace(oldTypeParam, newTypeParam);
    }
}

static void SetConstraintTypeAndDefaultTypeForTypeParams(public_lib::Context *ctx,
                                                         const ArenaVector<checker::ETSTypeParameter *> &newTypeParams,
                                                         const ArenaVector<ir::TSTypeParameter *> &newTypeParamNodes,
                                                         const checker::Signature *const lambdaSig,
                                                         const checker::Substitution *const substitution)
{
    auto *allocator = ctx->allocator;
    auto *checker = ctx->GetChecker()->AsETSChecker();

    for (size_t ix = 0; ix < lambdaSig->TypeParams().size(); ix++) {
        auto *oldTypeParam = lambdaSig->TypeParams()[ix]->AsETSTypeParameter();

        if (auto *oldConstraint = oldTypeParam->GetConstraintType(); oldConstraint != nullptr) {
            auto *newConstraint = oldConstraint->Substitute(checker->Relation(), substitution);
            newTypeParams[ix]->SetConstraintType(newConstraint);
            newTypeParamNodes[ix]->SetConstraint(allocator->New<ir::OpaqueTypeNode>(newConstraint, allocator));
            newTypeParamNodes[ix]->Constraint()->SetParent(newTypeParamNodes[ix]);
        }
        if (auto *oldDefault = oldTypeParam->GetDefaultType(); oldDefault != nullptr) {
            auto *newDefault = oldDefault->Substitute(checker->Relation(), substitution);
            newTypeParams[ix]->SetDefaultType(newDefault);
            newTypeParamNodes[ix]->SetDefaultType(allocator->New<ir::OpaqueTypeNode>(newDefault, allocator));
            newTypeParamNodes[ix]->DefaultType()->SetParent(newTypeParamNodes[ix]);
        }
    }
}

static ir::TSTypeParameterDeclaration *CloneTypeParamsForSignature(public_lib::Context *ctx,
                                                                   LambdaClassInvokeInfo *lciInfo)
{
    ir::TSTypeParameterDeclaration *oldIrTypeParams = lciInfo->callee->Function()->TypeParams();
    checker::Signature *lambdaSig = lciInfo->lambdaSignature;
    // NOTE (smartin): the first condition can be deleted, once the generic lambdas generate correct invoke function
    // into the global scope (currently we don't generate type params for them)
    if (oldIrTypeParams == nullptr || lambdaSig->TypeParams().empty()) {
        return nullptr;
    }

    auto *allocator = ctx->allocator;
    auto newTypeParams = ArenaVector<checker::ETSTypeParameter *>(allocator->Adapter());
    auto newTypeParamNodes = ArenaVector<ir::TSTypeParameter *>(allocator->Adapter());

    FillNewTypeParamVectors(allocator, newTypeParams, newTypeParamNodes, lambdaSig, lciInfo->substitution);
    SetConstraintTypeAndDefaultTypeForTypeParams(ctx, newTypeParams, newTypeParamNodes, lambdaSig,
                                                 lciInfo->substitution);

    auto *newIrTypeParams = util::NodeAllocator::ForceSetParent<ir::TSTypeParameterDeclaration>(
        allocator, std::move(newTypeParamNodes), oldIrTypeParams->RequiredParams());

    return newIrTypeParams;
}

static std::pair<ir::TSTypeParameterDeclaration *, checker::Substitution> CloneTypeParamsForClass(
    public_lib::Context *ctx, ir::TSTypeParameterDeclaration *oldIrTypeParams, ir::ScriptFunction *enclosingFunction,
    varbinder::Scope *enclosingScope)
{
    if (oldIrTypeParams == nullptr) {
        return {nullptr, {}};
    }

    auto *allocator = ctx->allocator;

    auto *newScope = allocator->New<varbinder::LocalScope>(allocator, enclosingScope);
    auto newTypeParams = ArenaVector<checker::ETSTypeParameter *>(allocator->Adapter());
    auto newTypeParamNodes = ArenaVector<ir::TSTypeParameter *>(allocator->Adapter());
    auto substitution = checker::Substitution {};

    for (size_t ix = 0; ix < oldIrTypeParams->Params().size(); ix++) {
        auto *oldTypeParamNode = oldIrTypeParams->Params()[ix];
        auto *oldTypeParam = enclosingFunction->Signature()->TypeParams()[ix]->AsETSTypeParameter();
        auto *newTypeParamId = allocator->New<ir::Identifier>(oldTypeParamNode->Name()->Name(), allocator);
        auto *newTypeParamNode = util::NodeAllocator::ForceSetParent<ir::TSTypeParameter>(allocator, newTypeParamId,
                                                                                          nullptr, nullptr, allocator);
        auto *newTypeParam = allocator->New<checker::ETSTypeParameter>();
        auto *newTypeParamDecl = allocator->New<varbinder::TypeParameterDecl>(newTypeParamId->Name());
        auto *newTypeParamVar =
            allocator->New<varbinder::LocalVariable>(newTypeParamDecl, varbinder::VariableFlags::TYPE_PARAMETER);
        ES2PANDA_ASSERT(newTypeParam != nullptr && newScope != nullptr && newTypeParamDecl != nullptr &&
                        newTypeParamVar != nullptr);
        newTypeParam->SetDeclNode(newTypeParamNode);

        newTypeParamDecl->BindNode(newTypeParamNode);
        newTypeParamVar->SetTsType(newTypeParam);
        newScope->InsertBinding(newTypeParamId->Name(), newTypeParamVar);
        newTypeParamId->SetVariable(newTypeParamVar);

        newTypeParams.push_back(newTypeParam);
        newTypeParamNodes.push_back(newTypeParamNode);
        substitution.emplace(oldTypeParam, newTypeParam);
    }

    for (size_t ix = 0; ix < oldIrTypeParams->Params().size(); ix++) {
        auto *oldTypeParam = enclosingFunction->Signature()->TypeParams()[ix]->AsETSTypeParameter();
        ProcessTypeParameterProperties(oldTypeParam, newTypeParams[ix], newTypeParamNodes[ix], &substitution, ctx);
    }

    auto *newIrTypeParams = util::NodeAllocator::ForceSetParent<ir::TSTypeParameterDeclaration>(
        allocator, std::move(newTypeParamNodes), oldIrTypeParams->RequiredParams());
    ES2PANDA_ASSERT(newIrTypeParams != nullptr);
    newIrTypeParams->SetScope(newScope);

    return {newIrTypeParams, std::move(substitution)};
}

using ParamsAndVarMap =
    std::pair<ArenaVector<ir::Expression *>, ArenaMap<varbinder::Variable *, varbinder::Variable *>>;

inline static varbinder::Variable *InitNewParameterVariable(varbinder::VarBinder *varBinder,
                                                            ir::ETSParameterExpression *param,
                                                            checker::Type *newParamType,
                                                            varbinder::ParamScope *paramScope)
{
    ES2PANDA_ASSERT(param != nullptr);
    auto *var = varBinder->AddParamDecl(param);
    var->SetTsType(newParamType);
    var->SetScope(paramScope);
    param->SetVariable(var);
    param->SetTsType(newParamType);
    return var;
}

ParamsAndVarMap CreateLambdaCalleeParameters(public_lib::Context *ctx, ir::ArrowFunctionExpression *lambda,
                                             ArenaSet<varbinder::Variable *> const &captured,
                                             varbinder::ParamScope *paramScope, checker::Substitution *substitution)
{
    auto allocator = ctx->allocator;
    auto checker = ctx->GetChecker()->AsETSChecker();
    auto varBinder = ctx->GetChecker()->VarBinder();
    auto resParams = ArenaVector<ir::Expression *>(allocator->Adapter());
    auto varMap = ArenaMap<varbinder::Variable *, varbinder::Variable *>(allocator->Adapter());

    auto paramLexScope = varbinder::LexicalScope<varbinder::ParamScope>::Enter(varBinder, paramScope);

    for (auto capturedVar : captured) {
        auto *newType = capturedVar->TsType()->Substitute(checker->Relation(), substitution);
        auto newId = util::NodeAllocator::ForceSetParent<ir::Identifier>(
            allocator, capturedVar->Name(), allocator->New<ir::OpaqueTypeNode>(newType, allocator), allocator);
        auto param =
            util::NodeAllocator::ForceSetParent<ir::ETSParameterExpression>(allocator, newId, false, allocator);
        auto *var = InitNewParameterVariable(varBinder, param, newType, paramScope);
        resParams.push_back(param);
        varMap[capturedVar] = var;
    }

    for (auto *oldParam : lambda->Function()->Params()) {
        auto *oldParamType = oldParam->AsETSParameterExpression()->Ident()->TsType();
        auto *newParamType = oldParamType->Substitute(checker->Relation(), substitution);
        auto *newParam = oldParam->AsETSParameterExpression()->Clone(allocator, nullptr);
        ES2PANDA_ASSERT(newParam != nullptr);

        if (newParam->IsOptional()) {
            newParam->SetOptional(false);
            newParamType = checker->CreateETSUnionType({newParamType, checker->GlobalETSUndefinedType()});
        }

        newParam->SetTypeAnnotation(allocator->New<ir::OpaqueTypeNode>(newParamType, allocator));
        auto *var = InitNewParameterVariable(varBinder, newParam, newParamType, paramScope);
        newParam->Ident()->SetTsType(newParamType);
        if (newParam->IsRestParameter()) {
            newParam->TypeAnnotation()->SetParent(newParam->Spread());
            newParam->Spread()->SetTsType(newParamType);
        } else {
            newParam->TypeAnnotation()->SetParent(newParam->Ident());
        }
        resParams.push_back(newParam);
        varMap[oldParam->AsETSParameterExpression()->Variable()] = var;

        if (newParam->TypeAnnotation()->IsETSFunctionType()) {
            // Parameter can be a function with other parameters inside
            // Restart varbinder to set correct scopes for inner parameters
            InitScopesPhaseETS::RunExternalNode(newParam->TypeAnnotation(), varBinder);
        }
    }

    return {resParams, varMap};
}

static void ProcessCalleeMethodBody(ir::AstNode *body, checker::ETSChecker *checker, varbinder::Scope *paramScope,
                                    checker::Substitution *substitution,
                                    ArenaMap<varbinder::Variable *, varbinder::Variable *> const &varMap)
{
    if (body == nullptr) {
        return;
    }
    body->Scope()->SetParent(paramScope);
    body->IterateRecursively([&](ir::AstNode *node) {
        if (node->IsIdentifier()) {
            auto *id = node->AsIdentifier();
            if (auto ref = varMap.find(id->Variable()); ref != varMap.end()) {
                id->SetVariable(ref->second);
                id->Check(checker);
            }
        }
        if (substitution == nullptr) {
            return;
        }
        if (node->IsTyped() && node->AsTyped()->TsType() != nullptr) {
            node->AsTyped()->SetTsType(node->AsTyped()->TsType()->Substitute(checker->Relation(), substitution));
            if (node->IsTSNonNullExpression()) {
                auto expr = node->AsTSNonNullExpression();
                expr->SetOriginalType(expr->OriginalType()->Substitute(checker->Relation(), substitution));
            }
        }
        if (node->IsCallExpression()) {
            node->AsCallExpression()->SetSignature(
                node->AsCallExpression()->Signature()->Substitute(checker->Relation(), substitution));
        }
        if (node->IsETSNewClassInstanceExpression()) {
            node->AsETSNewClassInstanceExpression()->SetSignature(
                node->AsETSNewClassInstanceExpression()->GetSignature()->Substitute(checker->Relation(), substitution));
        }
        if (node->IsScriptFunction()) {
            node->AsScriptFunction()->SetSignature(
                node->AsScriptFunction()->Signature()->Substitute(checker->Relation(), substitution));
        }
        if (node->IsVariableDeclarator()) {
            auto *id = node->AsVariableDeclarator()->Id();
            id->Variable()->SetTsType(id->Variable()->TsType()->Substitute(checker->Relation(), substitution));
        }
    });
}

static ir::MethodDefinition *SetUpCalleeMethod(public_lib::Context *ctx, LambdaInfo const *info,
                                               CalleeMethodInfo const *cmInfo, ir::ScriptFunction *func,
                                               varbinder::Scope *scopeForMethod)
{
    auto *allocator = ctx->allocator;
    auto *varBinder = ctx->GetChecker()->VarBinder()->AsETSBinder();

    auto *calleeClass = info->calleeClass;
    auto *funcScope = func->Scope();
    auto *paramScope = funcScope->ParamScope();
    auto modifierFlags = ir::ModifierFlags::PUBLIC |
                         (info->callReceiver != nullptr ? ir::ModifierFlags::NONE : ir::ModifierFlags::STATIC) |
                         cmInfo->auxModifierFlags;

    auto *calleeNameId = allocator->New<ir::Identifier>(cmInfo->calleeName, allocator);
    func->SetIdent(calleeNameId);
    calleeNameId->SetParent(func);

    auto *calleeNameClone = calleeNameId->Clone(allocator, nullptr);
    auto *funcExpr = util::NodeAllocator::ForceSetParent<ir::FunctionExpression>(allocator, func);
    auto *method = util::NodeAllocator::ForceSetParent<ir::MethodDefinition>(
        allocator, ir::MethodDefinitionKind::METHOD, calleeNameClone, funcExpr, modifierFlags, allocator, false);
    calleeClass->Definition()->EmplaceBody(method);
    method->SetParent(calleeClass->Definition());

    auto *var =
        std::get<1>(varBinder->NewVarDecl<varbinder::FunctionDecl>(func->Start(), allocator, cmInfo->calleeName, func));
    var->AddFlag(varbinder::VariableFlags::METHOD);
    var->SetScope(scopeForMethod);
    func->Id()->SetVariable(var);
    ES2PANDA_ASSERT(method->Id());
    method->Id()->SetVariable(var);
    if (info->callReceiver != nullptr) {
        auto paramScopeCtx = varbinder::LexicalScope<varbinder::FunctionParamScope>::Enter(varBinder, paramScope);
        varBinder->AddMandatoryParam(varbinder::TypedBinder::MANDATORY_PARAM_THIS);
        calleeClass->Definition()->TsType()->AsETSObjectType()->AddProperty<checker::PropertyType::INSTANCE_METHOD>(
            var->AsLocalVariable());
    } else {
        calleeClass->Definition()->TsType()->AsETSObjectType()->AddProperty<checker::PropertyType::STATIC_METHOD>(
            var->AsLocalVariable());
    }

    varbinder::BoundContext bctx {varBinder->GetRecordTable(), calleeClass->Definition(), true};
    varBinder->ResolveReferencesForScopeWithContext(func, funcScope);

    auto checkerCtx = checker::SavedCheckerContext(ctx->GetChecker(), checker::CheckerStatus::IN_CLASS,
                                                   calleeClass->Definition()->TsType()->AsETSObjectType());
    method->Check(ctx->GetChecker()->AsETSChecker());

    return method;
}

using ISS = ir::ScriptFunction::ScriptFunctionData;
static ir::MethodDefinition *CreateCalleeMethod(public_lib::Context *ctx, ir::ArrowFunctionExpression *lambda,
                                                LambdaInfo const *info, CalleeMethodInfo const *cmInfo)
{
    auto *allocator = ctx->allocator;
    auto *varBinder = ctx->GetChecker()->VarBinder()->AsETSBinder();
    auto *checker = ctx->GetChecker()->AsETSChecker();

    auto *classScope = info->calleeClass->Definition()->Scope()->AsClassScope();

    auto *oldTypeParams = (info->enclosingFunction != nullptr) ? info->enclosingFunction->TypeParams() : nullptr;
    auto enclosingScope =
        info->callReceiver != nullptr ? classScope->InstanceMethodScope() : classScope->StaticMethodScope();

    auto [newTypeParams, subst0] = CloneTypeParamsForClass(ctx, oldTypeParams, info->enclosingFunction, enclosingScope);
    auto &substitution = subst0;  // NOTE(gogabr): needed to capture in a lambda later.
    auto *scopeForMethod = newTypeParams != nullptr ? newTypeParams->Scope() : enclosingScope;

    auto lexScope = varbinder::LexicalScope<varbinder::LocalScope>::Enter(varBinder, enclosingScope);
    auto paramScope = allocator->New<varbinder::FunctionParamScope>(allocator, scopeForMethod);

    auto [params, vMap] = CreateLambdaCalleeParameters(ctx, lambda, *info->capturedVars, paramScope, &substitution);
    auto varMap = std::move(vMap);

    auto arrowReturnType = lambda->TsType()->AsETSFunctionType()->ArrowSignature()->ReturnType();
    auto *alternative = arrowReturnType->Substitute(checker->Relation(), &substitution);
    auto *returnType = cmInfo->forcedReturnType != nullptr ? cmInfo->forcedReturnType : alternative;
    auto returnTypeAnnotation = allocator->New<ir::OpaqueTypeNode>(returnType, allocator);

    auto funcFlags = ir::ScriptFunctionFlags::METHOD | cmInfo->auxFunctionFlags;
    auto modifierFlags = ir::ModifierFlags::PUBLIC |
                         (info->callReceiver != nullptr ? ir::ModifierFlags::NONE : ir::ModifierFlags::STATIC) |
                         cmInfo->auxModifierFlags;

    auto func = util::NodeAllocator::ForceSetParent<ir::ScriptFunction>(
        allocator, allocator,
        ISS {cmInfo->body,
             ir::FunctionSignature(newTypeParams, std::move(params), returnTypeAnnotation,
                                   lambda->Function()->HasReceiver()),
             funcFlags, modifierFlags});
    auto *funcScope = cmInfo->body == nullptr ? allocator->New<varbinder::FunctionScope>(allocator, paramScope)
                                              : cmInfo->body->Scope()->AsFunctionScope();
    ES2PANDA_ASSERT(funcScope);
    funcScope->BindName(info->calleeClass->Definition()->TsType()->AsETSObjectType()->AssemblerName());
    func->SetScope(funcScope);
    ProcessCalleeMethodBody(cmInfo->body, checker, paramScope, &substitution, varMap);

    for (auto *param : func->Params()) {
        param->SetParent(func);
    }

    // Bind the scopes
    funcScope->BindNode(func);
    paramScope->BindNode(func);
    funcScope->AssignParamScope(paramScope);
    paramScope->BindFunctionScope(funcScope);

    /* NOTE(gogabr): Why does function scope need to replicate bindings from param scope?.
       Keeping it for now.
    */
    for (auto [ov, nv] : varMap) {
        ES2PANDA_ASSERT(ov->Name() == nv->Name());
        funcScope->EraseBinding(ov->Name());
        funcScope->InsertBinding(ov->Name(), nv);
    }

    return SetUpCalleeMethod(ctx, info, cmInfo, func, scopeForMethod);
}

static ir::MethodDefinition *CreateCallee(public_lib::Context *ctx, ir::ArrowFunctionExpression *lambda,
                                          LambdaInfo const *info)
{
    auto *allocator = ctx->allocator;
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *body = lambda->Function()->Body()->AsBlockStatement();
    auto calleeName = lambda->Function()->IsAsyncFunc()
                          ? (util::UString {checker::ETSChecker::GetAsyncImplName(info->name), allocator}).View()
                          : info->name;
    auto *forcedReturnType = lambda->Function()->IsAsyncFunc() ? checker->GlobalETSAnyType() : nullptr;

    CalleeMethodInfo cmInfo;
    cmInfo.calleeName = calleeName;
    cmInfo.body = body;
    cmInfo.forcedReturnType = forcedReturnType;
    if (lambda->Function()->IsAsyncFunc()) {
        cmInfo.auxFunctionFlags = ir::ScriptFunctionFlags::ASYNC_IMPL;
    }
    auto *method = CreateCalleeMethod(ctx, lambda, info, &cmInfo);

    if (lambda->Function()->IsAsyncFunc()) {
        CalleeMethodInfo cmInfoAsync;
        cmInfoAsync.calleeName = info->name;
        cmInfoAsync.body = nullptr;
        cmInfoAsync.forcedReturnType = nullptr;
        cmInfoAsync.auxModifierFlags = ir::ModifierFlags::NATIVE;
        cmInfoAsync.auxFunctionFlags = ir::ScriptFunctionFlags::ASYNC;
        auto *asyncMethod = CreateCalleeMethod(ctx, lambda, info, &cmInfoAsync);
        asyncMethod->Function()->SetAsyncPairMethod(method->Function());
        return asyncMethod;
    }

    return method;
}

// The name "=t" used in extension methods has special meaning for the code generator;
// avoid it as parameter and field name in our generated code.
static util::StringView AvoidMandatoryThis(util::StringView name)
{
    return (name == varbinder::TypedBinder::MANDATORY_PARAM_THIS) ? "$extensionThis" : name;
}

static void CreateLambdaClassFields(public_lib::Context *ctx, ir::ClassDefinition *classDefinition,
                                    LambdaInfo const *info, checker::Substitution *substitution)
{
    auto *allocator = ctx->allocator;
    auto *parser = ctx->parser->AsETSParser();
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto props = ArenaVector<ir::AstNode *>(allocator->Adapter());

    checker::Type *objectType = info->objType != nullptr
                                    ? info->objType
                                    : (info->calleeClass != nullptr ? info->calleeClass->Definition()->TsType()
                                                                    : info->calleeInterface->TsType());

    if (info->callReceiver != nullptr) {
        auto *outerThisDeclaration = parser->CreateFormattedClassFieldDefinition(
            "@@I1: @@T2", "$this", objectType->Substitute(checker->Relation(), substitution));
        props.push_back(outerThisDeclaration);
    }

    for (auto *captured : *info->capturedVars) {
        auto *varDeclaration = parser->CreateFormattedClassFieldDefinition(
            "@@I1: @@T2", AvoidMandatoryThis(captured->Name()),
            captured->TsType()->Substitute(checker->Relation(), substitution));
        props.push_back(varDeclaration);
    }

    classDefinition->AddProperties(std::move(props));
}

static void CreateLambdaClassConstructor(public_lib::Context *ctx, ir::ClassDefinition *classDefinition,
                                         LambdaInfo const *info, checker::Substitution *substitution)
{
    auto *allocator = ctx->allocator;
    auto *parser = ctx->parser->AsETSParser();
    auto *checker = ctx->GetChecker()->AsETSChecker();

    auto params = ArenaVector<ir::Expression *>(allocator->Adapter());
    auto makeParam = [checker, allocator, substitution, &params](util::StringView name, checker::Type *type) {
        auto *substitutedType = type->Substitute(checker->Relation(), substitution);
        auto *id = util::NodeAllocator::ForceSetParent<ir::Identifier>(
            allocator, name, allocator->New<ir::OpaqueTypeNode>(substitutedType, allocator), allocator);
        auto *param = util::NodeAllocator::ForceSetParent<ir::ETSParameterExpression>(allocator, id, false, allocator);
        params.push_back(param);
    };

    checker::Type *objectType = info->objType != nullptr
                                    ? info->objType
                                    : (info->calleeClass != nullptr ? info->calleeClass->Definition()->TsType()
                                                                    : info->calleeInterface->TsType());

    if (info->callReceiver != nullptr) {
        makeParam("$this", objectType);
    }
    for (auto *var : *info->capturedVars) {
        makeParam(AvoidMandatoryThis(var->Name()), var->TsType());
    }

    auto bodyStmts = ArenaVector<ir::Statement *>(allocator->Adapter());
    auto makeStatement = [&parser, &bodyStmts](util::StringView name) {
        auto adjustedName = AvoidMandatoryThis(name);
        bodyStmts.push_back(parser->CreateFormattedStatement("this.@@I1 = @@I2", adjustedName, adjustedName));
    };
    if (info->callReceiver != nullptr) {
        makeStatement("$this");
    }
    for (auto *var : *info->capturedVars) {
        makeStatement(var->Name());
    }

    auto *body = util::NodeAllocator::ForceSetParent<ir::BlockStatement>(allocator, allocator, std::move(bodyStmts));

    auto *constructorId = allocator->New<ir::Identifier>("constructor", allocator);

    auto *func = util::NodeAllocator::ForceSetParent<ir::ScriptFunction>(
        allocator, allocator,
        ir::ScriptFunction::ScriptFunctionData {body, ir::FunctionSignature(nullptr, std::move(params), nullptr),
                                                ir::ScriptFunctionFlags::CONSTRUCTOR |
                                                    ir::ScriptFunctionFlags::IMPLICIT_SUPER_CALL_NEEDED});
    ES2PANDA_ASSERT(func);
    func->SetIdent(constructorId);
    auto *funcExpr = util::NodeAllocator::ForceSetParent<ir::FunctionExpression>(allocator, func);

    auto *ctor = util::NodeAllocator::ForceSetParent<ir::MethodDefinition>(
        allocator, ir::MethodDefinitionKind::CONSTRUCTOR, constructorId->Clone(allocator, nullptr), funcExpr,
        ir::ModifierFlags::NONE, allocator, false);

    classDefinition->EmplaceBody(ctor);
    ctor->SetParent(classDefinition);
}

// NOTE(vpukhov): requires the optimization based on the array type
// CC-OFFNXT(G.FUN.01, huge_method) solid logic
static ArenaVector<ark::es2panda::ir::Statement *> CreateRestArgumentsArrayReallocation(
    public_lib::Context *ctx, LambdaClassInvokeInfo const *lciInfo, size_t startIdx)
{
    if (!lciInfo->lambdaSignature->HasRestParameter() ||
        lciInfo->lambdaSignature->RestVar()->TsType()->IsETSTupleType()) {
        return ArenaVector<ir::Statement *>(ctx->allocator->Adapter());
    }

    auto *allocator = ctx->allocator;
    auto *parser = ctx->parser->AsETSParser();
    auto *checker = ctx->GetChecker()->AsETSChecker();

    auto *restParameterType = lciInfo->lambdaSignature->RestVar()->TsType();

    auto *restParameterSubstituteType = restParameterType->Substitute(checker->Relation(), lciInfo->substitution);
    auto *elementType = checker->GetElementTypeOfArray(restParameterSubstituteType);
    auto *elementTypeWithDefault = checker->TypeHasDefaultValue(elementType)
                                       ? elementType
                                       : checker->CreateETSUnionType({elementType, checker->GlobalETSUndefinedType()});
    std::stringstream statements;
    auto restParameterLen = GenName(allocator).View();
    ir::Statement *args = nullptr;
    if (restParameterSubstituteType->IsETSArrayType()) {
        auto tmpArray = GenName(allocator).View();
        // NOTE(vpukhov): this is a clear null-safety violation (when elementType->IsETSReferenceType()) that should be
        // rewitten with a runtime intrinsic
        // CC-OFFNXT(G.FMT.06) false positive
        statements << "let @@I1: int = @@I2.length - " << startIdx
                   << ";"
                   // CC-OFFNXT(G.FMT.06) false positive
                   << "let @@I3: FixedArray<@@T4> = new (@@T5)[@@I6];"
                   // CC-OFFNXT(G.FMT.06) false positive
                   << "let @@I7 = @@I8 as FixedArray<@@T9>;"
                   // CC-OFFNXT(G.FMT.06) false positive
                   << "for (let i: int = 0; i < @@I10; i = i + 1) {"
                   // CC-OFFNXT(G.FMT.06) false positive
                   << "    @@I11[i] = @@I12[i + " << startIdx << "] as @@T13;"
                   << "}";
        args = parser->CreateFormattedStatement(
            statements.str(), restParameterLen, lciInfo->restParameterIdentifier, tmpArray, elementTypeWithDefault,
            elementTypeWithDefault, restParameterLen, lciInfo->restArgumentIdentifier, tmpArray, elementType,
            restParameterLen, lciInfo->restArgumentIdentifier, lciInfo->restParameterIdentifier, elementType);
    } else {
        ES2PANDA_ASSERT(restParameterSubstituteType->IsETSResizableArrayType());
        auto *typeNode = allocator->New<ir::OpaqueTypeNode>(
            checker->GetElementTypeOfArray(lciInfo->lambdaSignature->RestVar()->TsType()), allocator);
        // CC-OFFNXT(G.FMT.06) false positive
        statements << "let @@I1: int = @@I2.length - " << startIdx
                   << ";"
                   // CC-OFFNXT(G.FMT.06) false positive
                   << "let @@I3 = new Array<@@T4>(@@I5);"
                   // CC-OFFNXT(G.FMT.06) false positive
                   << "for (let i: int = 0; i < @@I6; i = i + 1) {"
                   // CC-OFFNXT(G.FMT.06) false positive
                   << "    @@I7.$_set(i, @@I8[i + " << startIdx << "] as @@T9);"
                   << "}";
        args = parser->CreateFormattedStatement(statements.str(), restParameterLen, lciInfo->restParameterIdentifier,
                                                lciInfo->restArgumentIdentifier, typeNode, restParameterLen,
                                                restParameterLen, lciInfo->restArgumentIdentifier,
                                                lciInfo->restParameterIdentifier, typeNode);
    }

    ES2PANDA_ASSERT(args != nullptr);
    return ArenaVector<ir::Statement *>(args->AsBlockStatement()->Statements());
}

static void CreateInvokeMethodRestParameter(public_lib::Context *ctx, LambdaClassInvokeInfo *lciInfo,
                                            ArenaVector<ir::Expression *> *params)
{
    auto *allocator = ctx->allocator;
    auto *checker = ctx->GetChecker()->AsETSChecker();

    auto *restIdent = Gensym(allocator);
    ES2PANDA_ASSERT(restIdent != nullptr);
    lciInfo->restParameterIdentifier = restIdent->Name();
    auto *spread = allocator->New<ir::SpreadElement>(ir::AstNodeType::REST_ELEMENT, allocator, restIdent);
    ES2PANDA_ASSERT(spread != nullptr);
    auto *arr = checker->CreateETSArrayType(checker->GlobalETSAnyType());

    auto *typeAnnotation = allocator->New<ir::OpaqueTypeNode>(arr, allocator);

    spread->SetTsTypeAnnotation(typeAnnotation);
    spread->SetTsType(arr);
    restIdent->SetTsType(arr);
    auto *param = allocator->New<ir::ETSParameterExpression>(spread, nullptr, allocator);

    restIdent->SetParent(spread);
    typeAnnotation->SetParent(spread);
    spread->SetParent(param);
    params->push_back(param);
}

static ir::Expression *SetRestIdentOfCallArguments(public_lib::Context *ctx, LambdaClassInvokeInfo const *lciInfo)
{
    auto *allocator = ctx->allocator;

    auto restType = lciInfo->lambdaSignature->RestVar()->TsType();
    if (restType->IsETSTupleType()) {
        ArenaVector<ir::Expression *> tupleElements(allocator->Adapter());
        for (std::uint16_t i = 0; i < restType->AsETSTupleType()->GetTupleSize(); ++i) {
            auto ident = allocator->New<ir::Identifier>(lciInfo->restParameterIdentifier, allocator);
            auto number = allocator->New<ir::NumberLiteral>(lexer::Number(i));
            auto indexed = util::NodeAllocator::ForceSetParent<ir::MemberExpression>(
                allocator, ident, number, ir::MemberExpressionKind::ELEMENT_ACCESS, true, false);

            auto typeNode =
                allocator->New<ir::OpaqueTypeNode>(restType->AsETSTupleType()->GetTupleTypesList()[i], allocator);
            auto cast = util::NodeAllocator::ForceSetParent<ir::TSAsExpression>(allocator, indexed, typeNode, false);
            tupleElements.push_back(cast);
        }
        auto arrayExpr =
            util::NodeAllocator::ForceSetParent<ir::ArrayExpression>(allocator, std::move(tupleElements), allocator);
        auto *spread = util::NodeAllocator::ForceSetParent<ir::SpreadElement>(
            allocator, ir::AstNodeType::SPREAD_ELEMENT, allocator, arrayExpr);
        return spread;
    }
    auto *restIdent =
        util::NodeAllocator::ForceSetParent<ir::Identifier>(allocator, lciInfo->restArgumentIdentifier, allocator);
    if (restType->IsETSArrayType()) {
        auto *spread = allocator->New<ir::SpreadElement>(ir::AstNodeType::SPREAD_ELEMENT, allocator, restIdent);
        restIdent->SetParent(spread);
        return spread;
    }

    ES2PANDA_ASSERT(restType->IsETSResizableArrayType());
    restIdent->AddAstNodeFlags(ir::AstNodeFlags::RESIZABLE_REST);
    return restIdent;
}

static ArenaVector<ir::Expression *> CreateCallArgumentsForLambdaClassInvoke(public_lib::Context *ctx,
                                                                             LambdaInfo const *info,
                                                                             LambdaClassInvokeInfo const *lciInfo,
                                                                             bool wrapToObject)
{
    auto *allocator = ctx->allocator;
    auto *parser = ctx->parser->AsETSParser();
    auto *checker = ctx->GetChecker()->AsETSChecker();

    auto callArguments = ArenaVector<ir::Expression *>(allocator->Adapter());
    for (auto *captured : *info->capturedVars) {
        auto *arg = parser->CreateFormattedExpression("this.@@I1", AvoidMandatoryThis(captured->Name()));
        callArguments.push_back(arg);
    }
    for (size_t idx = 0; idx < lciInfo->lambdaSignature->ArgCount(); ++idx) {
        auto lambdaParam = lciInfo->lambdaSignature->Params().at(idx);
        if (idx >= lciInfo->arity) {
            callArguments.push_back(allocator->New<ir::UndefinedLiteral>());
            continue;
        }
        auto argName = lambdaParam->Name();
        auto *type = lambdaParam->TsType()->Substitute(checker->Relation(), lciInfo->substitution);
        auto *arg = wrapToObject ? parser->CreateFormattedExpression("@@I1 as @@T2", argName, type)
                                 : allocator->New<ir::Identifier>(argName, allocator);
        callArguments.push_back(arg);
    }

    if (!lciInfo->lambdaSignature->HasRestParameter()) {
        return callArguments;
    }
    callArguments.push_back(SetRestIdentOfCallArguments(ctx, lciInfo));
    return callArguments;
}

static ArenaVector<ir::Expression *> CreateCallArgumentsForLambdaClassInvokeN(public_lib::Context *ctx,
                                                                              LambdaInfo const *info,
                                                                              LambdaClassInvokeInfo const *lciInfo)
{
    auto *allocator = ctx->allocator;
    auto *parser = ctx->parser->AsETSParser();
    auto *checker = ctx->GetChecker()->AsETSChecker();

    auto callArguments = ArenaVector<ir::Expression *>(allocator->Adapter());
    for (auto *captured : *info->capturedVars) {
        auto *arg = parser->CreateFormattedExpression("this.@@I1", AvoidMandatoryThis(captured->Name()));
        callArguments.push_back(arg);
    }

    ES2PANDA_ASSERT(lciInfo->argNames->size() ==
                    lciInfo->lambdaSignature->ArgCount() - lciInfo->lambdaSignature->MinArgCount());

    for (size_t idx = 0; idx < lciInfo->lambdaSignature->MinArgCount(); ++idx) {
        auto lambdaParam = lciInfo->lambdaSignature->Params().at(idx);
        auto argName = lciInfo->restParameterIdentifier;
        auto *type = lambdaParam->TsType()->Substitute(checker->Relation(), lciInfo->substitution);
        auto arg = parser->CreateFormattedExpression("@@I1[" + std::to_string(idx) + "] as @@T2 as @@T3", argName,
                                                     checker->MaybeBoxType(type), type);
        callArguments.push_back(arg);
    }
    for (size_t nameIdx = 0, idx = lciInfo->lambdaSignature->MinArgCount(); idx < lciInfo->lambdaSignature->ArgCount();
         ++idx, ++nameIdx) {
        auto arg = parser->CreateFormattedExpression("@@I1", (*lciInfo->argNames)[nameIdx]);
        callArguments.push_back(arg);
    }

    if (!lciInfo->lambdaSignature->HasRestParameter()) {
        return callArguments;
    }

    callArguments.push_back(SetRestIdentOfCallArguments(ctx, lciInfo));
    return callArguments;
}

static ir::CallExpression *CreateCallForLambdaClassInvoke(public_lib::Context *ctx, LambdaInfo const *info,
                                                          LambdaClassInvokeInfo const *lciInfo, bool wrapToObject,
                                                          bool isInvokeN)
{
    auto *allocator = ctx->allocator;
    auto *parser = ctx->parser->AsETSParser();

    auto callArguments = isInvokeN ? CreateCallArgumentsForLambdaClassInvokeN(ctx, info, lciInfo)
                                   : CreateCallArgumentsForLambdaClassInvoke(ctx, info, lciInfo, wrapToObject);
    ir::Expression *calleeReceiver;
    if (info->callReceiver != nullptr) {
        calleeReceiver = parser->CreateFormattedExpression("this.@@I1", "$this");
    } else {
        calleeReceiver = lciInfo->callee->Parent()->AsClassDefinition()->Ident()->Clone(allocator, nullptr);
    }

    auto *calleeMemberExpr = util::NodeAllocator::ForceSetParent<ir::MemberExpression>(
        allocator, calleeReceiver, lciInfo->callee->Key()->Clone(allocator, nullptr)->AsExpression(),
        ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);

    auto *call = util::NodeAllocator::ForceSetParent<ir::CallExpression>(allocator, calleeMemberExpr,
                                                                         std::move(callArguments), nullptr, false);

    // NOTE (smartin): the condition would be better to check the size of the signature's type parameters. But currently
    // generic lambdas don't allocate type parameters for they global invoke function, so fix this when the generation
    // will be corrected
    if (lciInfo->callee->Function()->TypeParams() != nullptr) {
        auto origCallTypeParams = lciInfo->lambdaSignature->TypeParams();
        auto typeArgs = ArenaVector<ir::TypeNode *>(allocator->Adapter());
        for (auto *tp : origCallTypeParams) {
            typeArgs.push_back(allocator->New<ir::OpaqueTypeNode>(
                tp->Substitute(ctx->GetChecker()->Relation(), lciInfo->substitution), allocator));
        }
        auto *typeArg =
            util::NodeAllocator::ForceSetParent<ir::TSTypeParameterInstantiation>(allocator, std::move(typeArgs));
        call->SetTypeParams(typeArg);
        typeArg->SetParent(call);
    }

    if (lciInfo->classDefinition->TypeParams() != nullptr) {
        auto typeArgs = ArenaVector<ir::TypeNode *>(allocator->Adapter());
        for (auto *tp : lciInfo->classDefinition->TypeParams()->Params()) {
            typeArgs.push_back(
                allocator->New<ir::OpaqueTypeNode>(tp->Name()->AsIdentifier()->Variable()->TsType(), allocator));
        }
        auto *typeArg =
            util::NodeAllocator::ForceSetParent<ir::TSTypeParameterInstantiation>(allocator, std::move(typeArgs));
        call->SetTypeParams(typeArg);
        typeArg->SetParent(call);
    }

    return call;
}

static ir::BlockStatement *CreateLambdaClassInvokeBody(public_lib::Context *ctx, LambdaInfo const *info,
                                                       LambdaClassInvokeInfo const *lciInfo, bool wrapToObject)
{
    auto *allocator = ctx->allocator;
    auto *parser = ctx->parser->AsETSParser();
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *anyType = checker->GlobalETSAnyType();

    auto *call = CreateCallForLambdaClassInvoke(ctx, info, lciInfo, wrapToObject, false);
    auto bodyStmts = CreateRestArgumentsArrayReallocation(ctx, lciInfo, 0);

    if (lciInfo->lambdaSignature->ReturnType() == checker->GlobalVoidType()) {
        auto *callStmt = util::NodeAllocator::ForceSetParent<ir::ExpressionStatement>(allocator, call);
        bodyStmts.push_back(callStmt);
        if (wrapToObject) {
            auto *returnStmt = util::NodeAllocator::ForceSetParent<ir::ReturnStatement>(
                allocator, allocator->New<ir::UndefinedLiteral>());
            bodyStmts.push_back(returnStmt);
        }
    } else {
        auto *returnExpr = wrapToObject ? parser->CreateFormattedExpression("@@E1 as @@T2", call, anyType) : call;
        auto *returnStmt = util::NodeAllocator::ForceSetParent<ir::ReturnStatement>(allocator, returnExpr);
        bodyStmts.push_back(returnStmt);
    }

    return util::NodeAllocator::ForceSetParent<ir::BlockStatement>(allocator, allocator, std::move(bodyStmts));
}

static void CreateLambdaClassInvokeMethod(public_lib::Context *ctx, LambdaInfo const *info,
                                          LambdaClassInvokeInfo *lciInfo, util::StringView methodName,
                                          bool wrapToObject)
{
    auto *allocator = ctx->allocator;
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *anyType = checker->GlobalETSAnyType();

    auto *invokeSigTypeParams = CloneTypeParamsForSignature(ctx, lciInfo);

    auto params = ArenaVector<ir::Expression *>(allocator->Adapter());
    for (size_t idx = 0; idx < lciInfo->arity; ++idx) {
        auto *lparam = lciInfo->lambdaSignature->Params().at(idx);
        auto *type = wrapToObject ? anyType : lparam->TsType()->Substitute(checker->Relation(), lciInfo->substitution);
        auto *id = util::NodeAllocator::ForceSetParent<ir::Identifier>(
            allocator, lparam->Name(), allocator->New<ir::OpaqueTypeNode>(type, allocator), allocator);
        auto *param = util::NodeAllocator::ForceSetParent<ir::ETSParameterExpression>(allocator, id, false, allocator);
        params.push_back(param);
    }

    if (lciInfo->lambdaSignature->HasRestParameter()) {
        CreateInvokeMethodRestParameter(ctx, lciInfo, &params);
        lciInfo->restArgumentIdentifier = GenName(allocator).View();
    }

    auto *returnType2 = allocator->New<ir::OpaqueTypeNode>(
        wrapToObject ? anyType
                     : lciInfo->lambdaSignature->ReturnType()->Substitute(checker->Relation(), lciInfo->substitution),
        allocator);
    bool hasReceiver = lciInfo->lambdaSignature->HasSignatureFlag(checker::SignatureFlags::EXTENSION_FUNCTION);
    ir::ScriptFunctionFlags functionFlag = ir::ScriptFunctionFlags::METHOD;
    auto *func = util::NodeAllocator::ForceSetParent<ir::ScriptFunction>(
        allocator, allocator,
        ir::ScriptFunction::ScriptFunctionData {
            CreateLambdaClassInvokeBody(ctx, info, lciInfo, wrapToObject),
            ir::FunctionSignature(invokeSigTypeParams, std::move(params), returnType2, hasReceiver), functionFlag});

    auto *invokeId = allocator->New<ir::Identifier>(methodName, allocator);
    func->SetIdent(invokeId);

    auto *funcExpr = util::NodeAllocator::ForceSetParent<ir::FunctionExpression>(allocator, func);

    auto *invokeIdClone = invokeId->Clone(allocator, nullptr);
    auto *invokeMethod = util::NodeAllocator::ForceSetParent<ir::MethodDefinition>(
        allocator, ir::MethodDefinitionKind::METHOD, invokeIdClone, funcExpr, ir::ModifierFlags::NONE, allocator,
        false);
    ES2PANDA_ASSERT(!invokeMethod->IsStatic());

    lciInfo->classDefinition->EmplaceBody(invokeMethod);
    invokeMethod->SetParent(lciInfo->classDefinition);
}

static ir::BlockStatement *CreateLambdaClassInvokeNBody(public_lib::Context *ctx, LambdaInfo const *info,
                                                        LambdaClassInvokeInfo *lciInfo)
{
    auto *allocator = ctx->allocator;
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *parser = ctx->parser->AsETSParser();

    auto bodyStmts = CreateRestArgumentsArrayReallocation(ctx, lciInfo, lciInfo->lambdaSignature->MinArgCount());
    auto tempVarNames = allocator->New<ArenaVector<util::UString>>(allocator->Adapter());

    for (size_t idx = lciInfo->lambdaSignature->MinArgCount(); idx < lciInfo->arity; ++idx) {
        auto lambdaParam = lciInfo->lambdaSignature->Params().at(idx);
        auto argName = lciInfo->restParameterIdentifier;
        auto *type = lambdaParam->TsType()->Substitute(checker->Relation(), lciInfo->substitution);
        std::stringstream stream;
        stream << "let @@I1 = @@I2.length > " << idx << " ? @@I3[" << idx << "] as @@T4 as @@T5 : undefined;";
        tempVarNames->push_back(GenName(allocator));
        auto stmt = parser->CreateFormattedStatement(stream.str(), tempVarNames->back(), argName, argName,
                                                     checker->MaybeBoxType(type), type);
        bodyStmts.push_back(stmt);
    }

    lciInfo->argNames = tempVarNames;
    auto *call = CreateCallForLambdaClassInvoke(ctx, info, lciInfo, false, true);

    if (lciInfo->lambdaSignature->ReturnType() == checker->GlobalVoidType()) {
        auto *callStmt = util::NodeAllocator::ForceSetParent<ir::ExpressionStatement>(allocator, call);
        bodyStmts.push_back(callStmt);
        auto *returnStmt =
            util::NodeAllocator::ForceSetParent<ir::ReturnStatement>(allocator, allocator->New<ir::UndefinedLiteral>());
        bodyStmts.push_back(returnStmt);
    } else {
        auto *returnStmt = util::NodeAllocator::ForceSetParent<ir::ReturnStatement>(allocator, call);
        bodyStmts.push_back(returnStmt);
    }

    return util::NodeAllocator::ForceSetParent<ir::BlockStatement>(allocator, allocator, std::move(bodyStmts));
}

static void CreateLambdaClassInvokeN(public_lib::Context *ctx, LambdaInfo const *info, LambdaClassInvokeInfo *lciInfo)
{
    auto *allocator = ctx->allocator;
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *anyType = checker->GlobalETSAnyType();

    auto params = ArenaVector<ir::Expression *>(allocator->Adapter());
    CreateInvokeMethodRestParameter(ctx, lciInfo, &params);

    if (lciInfo->lambdaSignature->HasRestParameter()) {
        lciInfo->restArgumentIdentifier = GenName(allocator).View();
    }

    auto *returnType = allocator->New<ir::OpaqueTypeNode>(anyType, allocator);
    bool hasReceiver = lciInfo->lambdaSignature->HasSignatureFlag(checker::SignatureFlags::EXTENSION_FUNCTION);
    ir::ScriptFunctionFlags functionFlag = ir::ScriptFunctionFlags::METHOD;
    auto *func = util::NodeAllocator::ForceSetParent<ir::ScriptFunction>(
        allocator, allocator,
        ir::ScriptFunction::ScriptFunctionData {
            CreateLambdaClassInvokeNBody(ctx, info, lciInfo),
            ir::FunctionSignature(nullptr, std::move(params), returnType, hasReceiver), functionFlag});

    auto *invokeId = allocator->New<ir::Identifier>("unsafeCall", allocator);
    func->SetIdent(invokeId);

    auto *funcExpr = util::NodeAllocator::ForceSetParent<ir::FunctionExpression>(allocator, func);

    auto *invokeIdClone = invokeId->Clone(allocator, nullptr);
    auto *invokeMethod = util::NodeAllocator::ForceSetParent<ir::MethodDefinition>(
        allocator, ir::MethodDefinitionKind::METHOD, invokeIdClone, funcExpr, ir::ModifierFlags::NONE, allocator,
        false);
    ES2PANDA_ASSERT(!invokeMethod->IsStatic());

    lciInfo->classDefinition->EmplaceBody(invokeMethod);
    invokeMethod->SetParent(lciInfo->classDefinition);
}

static checker::ETSObjectType *FunctionTypeToLambdaProviderType(checker::ETSChecker *checker,
                                                                checker::Signature *signature)
{
    if (signature->RestVar() != nullptr) {
        ES2PANDA_ASSERT(checker->GlobalBuiltinLambdaType(signature->ArgCount(), true));
        return checker->GlobalBuiltinLambdaType(signature->ArgCount(), true)->AsETSObjectType();
    }
    return checker->GlobalBuiltinLambdaType(signature->ArgCount(), false)->AsETSObjectType();
}

// The `invoke` and `invoke0` of extension lambda class has two `this` identifier in parameter scope,
// first one is the lambdaClass itself and second one is the receiver class,
// the true `this` of the `invoke` and `invoke0` functionScope is the lambdaClass.
static void CorrectTheTrueThisForExtensionLambda(public_lib::Context *ctx, ir::ClassDeclaration *lambdaClass,
                                                 size_t arity, bool hasRestParam)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *classScope = lambdaClass->Definition()->Scope();
    ArenaVector<varbinder::Variable *> invokeFuncsOfLambda(ctx->Allocator()->Adapter());
    auto invokeName = checker->FunctionalInterfaceInvokeName(arity, hasRestParam);
    invokeFuncsOfLambda.emplace_back(
        classScope->FindLocal(compiler::Signatures::LAMBDA_OBJECT_INVOKE, varbinder::ResolveBindingOptions::METHODS));
    invokeFuncsOfLambda.emplace_back(
        classScope->FindLocal(util::StringView(invokeName), varbinder::ResolveBindingOptions::METHODS));
    for (auto *invokeFuncOfLambda : invokeFuncsOfLambda) {
        if (invokeFuncOfLambda == nullptr) {
            continue;
        }
        auto *scriptFunc = invokeFuncOfLambda->Declaration()
                               ->AsFunctionDecl()
                               ->Node()
                               ->AsMethodDefinition()
                               ->Value()
                               ->AsFunctionExpression()
                               ->Function();
        if (!scriptFunc->Signature()->HasSignatureFlag(checker::SignatureFlags::EXTENSION_FUNCTION)) {
            ES2PANDA_ASSERT(!scriptFunc->IsExtensionMethod());
            continue;
        }
        ES2PANDA_ASSERT(scriptFunc->IsExtensionMethod());
        auto *functionScope = scriptFunc->Scope();
        auto *functionParamScope = scriptFunc->Scope()->ParamScope();
        auto *theTrueThisVar = functionParamScope->Params()[0];
        auto &bindings = const_cast<varbinder::Scope::VariableMap &>(functionScope->Bindings());
        bindings.erase(varbinder::ETSBinder::MANDATORY_PARAM_THIS);
        bindings.insert({varbinder::ETSBinder::MANDATORY_PARAM_THIS, theTrueThisVar});
    }
}

static ir::ClassDeclaration *CreateEmptyLambdaClassDeclaration(public_lib::Context *ctx, LambdaInfo const *info,
                                                               ir::TSTypeParameterDeclaration *newTypeParams,
                                                               checker::ETSObjectType *fnInterface,
                                                               checker::ETSObjectType *lambdaProviderClass)
{
    auto *allocator = ctx->allocator;
    auto *parser = ctx->parser->AsETSParser();
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *varBinder = ctx->GetChecker()->VarBinder()->AsETSBinder();

    auto lambdaClassName = util::UString {
        std::string_view {util::NameMangler::GetInstance()->CreateMangledNameForLambdaObject(info->name)}, allocator};

    ES2PANDA_ASSERT(lambdaProviderClass);
    auto *providerTypeReference = checker->AllocNode<ir::ETSTypeReference>(
        checker->AllocNode<ir::ETSTypeReferencePart>(
            checker->AllocNode<ir::Identifier>(lambdaProviderClass->AsETSObjectType()->Name(), checker->Allocator()),
            nullptr, nullptr, allocator),
        allocator);

    std::stringstream stream;
    stream << "@" << Signatures::NAMED_FUNCTION_OBJECT << "({name: \"" << info->originalFuncName
           << "\"}) final class @@I1 extends @@T2 implements @@T3 {}";

    auto *classDeclaration =
        parser->CreateFormattedTopLevelStatement(stream.str(), lambdaClassName, providerTypeReference, fnInterface)
            ->AsClassDeclaration();
    auto *classDefinition = classDeclaration->Definition();

    // Adjust the class definition compared to what the parser gives.
    classDefinition->ClearBody();  // remove the default empty constructor
    classDefinition->AddModifier(ir::ModifierFlags::PUBLIC | ir::ModifierFlags::FUNCTIONAL);
    if (newTypeParams != nullptr) {
        classDefinition->SetTypeParams(newTypeParams);
        newTypeParams->SetParent(classDefinition);
    }

    auto *program = varBinder->GetRecordTable()->Program();
    program->Ast()->AddStatement(classDeclaration);
    classDeclaration->SetParent(program->Ast());

    return classDeclaration;
}

static void SetModifiersForFunctionReference(ir::ClassDefinition *classDefinition, ir::MethodDefinition *callee,
                                             LambdaInfo const *info)
{
    if (info->isFunctionReference) {
        ES2PANDA_ASSERT(callee->Function());
        classDefinition->SetFunctionalReferenceReferencedMethod(callee);
        classDefinition->SetModifiers(classDefinition->Modifiers() |
                                      ir::ClassDefinitionModifiers::FUNCTIONAL_REFERENCE);
    }
}

static ir::ClassDeclaration *CreateLambdaClass(public_lib::Context *ctx, checker::ETSFunctionType *fntype,
                                               ir::MethodDefinition *callee, LambdaInfo const *info)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *varBinder = ctx->GetChecker()->VarBinder()->AsETSBinder();

    auto *oldTypeParams = (info->enclosingFunction != nullptr) ? info->enclosingFunction->TypeParams() : nullptr;
    auto [newTypeParams, subst0] =
        CloneTypeParamsForClass(ctx, oldTypeParams, info->enclosingFunction, ctx->parserProgram->GlobalClassScope());
    auto &substitution = subst0;  // NOTE(gogabr): needed to capture in a lambda later.

    auto fnInterface = fntype->Substitute(checker->Relation(), &substitution)->ArrowToFunctionalInterface(checker);
    auto lambdaProviderClass = FunctionTypeToLambdaProviderType(checker, fntype->ArrowSignature());

    auto lexScope = varbinder::LexicalScope<varbinder::Scope>::Enter(varBinder, ctx->parserProgram->GlobalClassScope());

    auto classDeclaration =
        CreateEmptyLambdaClassDeclaration(ctx, info, newTypeParams, fnInterface, lambdaProviderClass);
    auto classDefinition = classDeclaration->Definition();
    SetModifiersForFunctionReference(classDefinition, callee, info);

    CreateLambdaClassFields(ctx, classDefinition, info, &substitution);
    CreateLambdaClassConstructor(ctx, classDefinition, info, &substitution);

    auto signature = fntype->ArrowSignature();

    LambdaClassInvokeInfo lciInfo;
    lciInfo.callee = callee;
    lciInfo.classDefinition = classDefinition;
    lciInfo.substitution = &substitution;
    lciInfo.lambdaSignature = signature;

    if (signature->ArgCount() < checker->GlobalBuiltinFunctionTypeVariadicThreshold()) {
        for (size_t arity = signature->MinArgCount(); arity <= signature->ArgCount(); ++arity) {
            lciInfo.arity = arity;
            auto invokeMethodName =
                util::UString {checker->FunctionalInterfaceInvokeName(arity, signature->HasRestParameter()),
                               ctx->allocator}
                    .View();
            CreateLambdaClassInvokeMethod(ctx, info, &lciInfo, invokeMethodName, true);
            // NOTE(vpukhov): for optional methods, the required invokeRk k={min, max-1} is not emitted
        }
    } else {
        lciInfo.arity = signature->ArgCount();
        CreateLambdaClassInvokeN(ctx, info, &lciInfo);
    }
    CreateLambdaClassInvokeMethod(ctx, info, &lciInfo, compiler::Signatures::LAMBDA_OBJECT_INVOKE, false);

    InitScopesPhaseETS::RunExternalNode(classDeclaration, varBinder);
    varBinder->ResolveReferencesForScopeWithContext(classDeclaration, varBinder->TopScope());
    classDeclaration->Check(checker);
    CorrectTheTrueThisForExtensionLambda(ctx, classDeclaration, signature->MinArgCount(),
                                         signature->HasRestParameter());
    return classDeclaration;
}

static ir::ETSNewClassInstanceExpression *CreateConstructorCall(public_lib::Context *ctx, ir::AstNode *lambdaOrFuncRef,
                                                                ir::ClassDeclaration *lambdaClass,
                                                                LambdaInfo const *info)
{
    auto *allocator = ctx->allocator;
    auto *varBinder = ctx->GetChecker()->VarBinder()->AsETSBinder();
    auto *checker = ctx->GetChecker()->AsETSChecker();

    auto args = ArenaVector<ir::Expression *>(allocator->Adapter());
    if (info->callReceiver != nullptr) {
        args.push_back(info->callReceiver);
    }
    for (auto captured : *info->capturedVars) {
        auto *id = allocator->New<ir::Identifier>(captured->Name(), allocator);
        args.push_back(id);
    }

    checker::ETSObjectType *constructedType = lambdaClass->Definition()->TsType()->AsETSObjectType();
    if (info->enclosingFunction != nullptr) {
        constructedType = constructedType->SubstituteArguments(checker->Relation(),
                                                               info->enclosingFunction->Signature()->TypeParams());
    }
    auto *newExpr = util::NodeAllocator::ForceSetParent<ir::ETSNewClassInstanceExpression>(
        allocator, allocator->New<ir::OpaqueTypeNode>(constructedType, allocator), std::move(args));
    auto *lambdaOrFuncRefParent = lambdaOrFuncRef->Parent();
    ES2PANDA_ASSERT(newExpr);
    newExpr->SetParent(lambdaOrFuncRefParent);
    // NOTE(dslynko, #19869): Required for correct debug-info generation
    ES2PANDA_ASSERT(newExpr);
    newExpr->SetRange(lambdaOrFuncRefParent != nullptr ? lambdaOrFuncRefParent->Range() : lambdaOrFuncRef->Range());

    auto *nearestScope = NearestScope(lambdaOrFuncRef);
    auto lexScope = varbinder::LexicalScope<varbinder::Scope>::Enter(varBinder, nearestScope);
    varBinder->ResolveReferencesForScopeWithContext(newExpr, nearestScope);

    checker::Type *objectType =
        info->calleeClass != nullptr ? info->calleeClass->Definition()->TsType() : info->calleeInterface->TsType();
    auto checkerCtx = checker::SavedCheckerContext(ctx->GetChecker(), checker::CheckerStatus::IN_CLASS,
                                                   objectType->AsETSObjectType());
    auto scopeCtx = checker::ScopeContext(ctx->GetChecker(), nearestScope);
    newExpr->Check(checker);

    return newExpr;
}

static ir::AstNode *ConvertLambda(public_lib::Context *ctx, ir::ArrowFunctionExpression *lambda)
{
    auto *allocator = ctx->allocator;
    auto *checker = ctx->GetChecker()->AsETSChecker();

    lambda->Check(checker);
    ES2PANDA_ASSERT(lambda->TsType()->IsETSFunctionType());

    LambdaInfo info;
    std::tie(info.calleeClass, info.enclosingFunction) = FindEnclosingClassAndFunction(lambda);
    info.name = CreateCalleeName(allocator);

    if ((lambda->Parent() != nullptr) && lambda->Parent()->IsVariableDeclarator()) {
        info.originalFuncName = lambda->Parent()->AsVariableDeclarator()->Id()->AsIdentifier()->Name();
    } else if ((lambda->Parent() != nullptr) && lambda->Parent()->IsClassProperty()) {
        info.originalFuncName = lambda->Parent()->AsClassProperty()->Id()->Name();
    }

    auto capturedVars = FindCaptured(allocator, lambda);
    info.capturedVars = &capturedVars;
    info.callReceiver = CheckIfNeedThis(lambda, checker) ? allocator->New<ir::ThisExpression>() : nullptr;
    info.isFunctionReference = false;

    auto *callee = CreateCallee(ctx, lambda, &info);
    auto *lambdaType = lambda->TsType()->AsETSFunctionType();
    auto *lambdaClass = CreateLambdaClass(ctx, lambdaType, callee, &info);
    return CreateConstructorCall(ctx, lambda, lambdaClass, &info);
}

static ir::ScriptFunction *GetWrappingLambdaParentFunction(public_lib::Context *ctx, ir::Expression *funcRef,
                                                           checker::Signature *signature)
{
    auto *allocator = ctx->allocator;
    ArenaVector<ir::Expression *> params {allocator->Adapter()};
    for (auto *p : signature->Params()) {
        params.push_back(util::NodeAllocator::ForceSetParent<ir::ETSParameterExpression>(
            allocator,
            allocator->New<ir::Identifier>(p->Name(), allocator->New<ir::OpaqueTypeNode>(p->TsType(), allocator),
                                           allocator),
            false, allocator));
    }
    auto *func = util::NodeAllocator::ForceSetParent<ir::ScriptFunction>(
        allocator, allocator,
        ir::ScriptFunction::ScriptFunctionData {
            nullptr,
            ir::FunctionSignature {nullptr, std::move(params),
                                   allocator->New<ir::OpaqueTypeNode>(signature->ReturnType(), allocator)},
            ir::ScriptFunctionFlags::ARROW});
    ES2PANDA_ASSERT(func != nullptr);
    ArenaVector<ir::Statement *> bodyStmts {allocator->Adapter()};
    ArenaVector<ir::Expression *> callArgs {allocator->Adapter()};

    for (auto *p : func->Params()) {
        ir::Identifier *clone = p->AsETSParameterExpression()->Ident()->Clone(allocator, nullptr);
        ES2PANDA_ASSERT(clone != nullptr);
        if (clone->IsIdentifier() && (clone->IsReference(ScriptExtension::ETS)) &&
            (clone->TypeAnnotation() != nullptr)) {
            clone->SetTsTypeAnnotation(nullptr);
        }
        callArgs.push_back(clone);
    }
    auto *callExpr = util::NodeAllocator::ForceSetParent<ir::CallExpression>(allocator, funcRef, std::move(callArgs),
                                                                             nullptr, false);
    ir::Statement *stmt;
    if (signature->ReturnType() == ctx->GetChecker()->AsETSChecker()->GlobalVoidType()) {
        stmt = util::NodeAllocator::ForceSetParent<ir::ExpressionStatement>(allocator, callExpr);
    } else {
        stmt = util::NodeAllocator::ForceSetParent<ir::ReturnStatement>(allocator, callExpr);
    }
    bodyStmts.push_back(stmt);
    func->SetBody(util::NodeAllocator::ForceSetParent<ir::BlockStatement>(allocator, allocator, std::move(bodyStmts)));
    ES2PANDA_ASSERT(func->Body());
    func->Body()->SetParent(func);
    return func;
}

static ir::ArrowFunctionExpression *CreateWrappingLambda(public_lib::Context *ctx, ir::Expression *funcRef)
{
    auto *allocator = ctx->allocator;
    auto *varBinder = ctx->GetChecker()->VarBinder()->AsETSBinder();
    ES2PANDA_ASSERT(funcRef->TsType()->IsETSArrowType());
    auto signature = funcRef->TsType()->AsETSFunctionType()->ArrowSignature();

    auto *parent = funcRef->Parent();
    auto *func = GetWrappingLambdaParentFunction(ctx, funcRef, signature);

    auto *lambda = util::NodeAllocator::ForceSetParent<ir::ArrowFunctionExpression>(allocator, func, allocator);
    ES2PANDA_ASSERT(lambda);
    lambda->SetParent(parent);

    auto *nearestScope = NearestScope(lambda);
    auto lexScope = varbinder::LexicalScope<varbinder::Scope>::Enter(varBinder, nearestScope);
    InitScopesPhaseETS::RunExternalNode(lambda, varBinder);
    varBinder->ResolveReferencesForScopeWithContext(lambda, nearestScope);

    auto [enclosingClass, _] = FindEnclosingClassAndFunction(parent);

    auto checkerCtx = checker::SavedCheckerContext(ctx->GetChecker(), checker::CheckerStatus::IN_CLASS,
                                                   enclosingClass->Definition()->TsType()->AsETSObjectType());
    auto scopeCtx = checker::ScopeContext(ctx->GetChecker(), nearestScope);
    lambda->Check(ctx->GetChecker()->AsETSChecker());

    return lambda;
}

static LambdaInfo GenerateLambdaInfoForFunctionReference(public_lib::Context *ctx, ir::Expression *funcRef,
                                                         ir::MethodDefinition *method)
{
    auto *allocator = ctx->allocator;
    LambdaInfo info;
    if (method->Parent()->Parent()->IsClassDeclaration()) {
        info.calleeClass = method->Parent()->Parent()->AsClassDeclaration();
    } else if (method->Parent()->Parent()->IsTSInterfaceDeclaration()) {
        info.calleeInterface = method->Parent()->Parent()->AsTSInterfaceDeclaration();
    } else {
        ES2PANDA_UNREACHABLE();
    }
    info.enclosingFunction = nullptr;
    info.name = CreateCalleeName(allocator);
    info.originalFuncName = method->Id()->Name();
    info.capturedVars = allocator->New<ArenaSet<varbinder::Variable *>>(allocator->Adapter());
    info.isFunctionReference = true;
    if (method->IsStatic()) {
        info.callReceiver = nullptr;
    } else {
        ES2PANDA_ASSERT(funcRef->IsMemberExpression());
        info.callReceiver = funcRef->AsMemberExpression()->Object();
    }
    if (funcRef->IsMemberExpression()) {
        info.objType = funcRef->AsMemberExpression()->ObjType();
    }
    return info;
}

static ir::AstNode *ConvertFunctionReference(public_lib::Context *ctx, ir::Expression *funcRef)
{
    ES2PANDA_ASSERT(funcRef->IsIdentifier() ||
                    (funcRef->IsMemberExpression() &&
                     funcRef->AsMemberExpression()->Kind() == ir::MemberExpressionKind::PROPERTY_ACCESS &&
                     funcRef->AsMemberExpression()->Property()->IsIdentifier()));
    varbinder::Variable *var;
    if (funcRef->IsIdentifier()) {
        var = funcRef->AsIdentifier()->Variable();
    } else {
        auto *mexpr = funcRef->AsMemberExpression();
        // NOTE(gogabr): mexpr->PropVar() is a synthetic variable wwith no reference to the method definition. Why?
        auto refVar = ctx->GetChecker()->AsETSChecker()->GetTargetRef(mexpr);
        auto flags = checker::PropertySearchFlags::SEARCH_IN_BASE |
                     checker::PropertySearchFlags::DISALLOW_SYNTHETIC_METHOD_CREATION;
        if (refVar != nullptr && refVar->HasFlag(varbinder::VariableFlags::CLASS)) {
            flags |= checker::PropertySearchFlags::SEARCH_STATIC_METHOD;
        } else {
            flags |= (checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD |
                      checker::PropertySearchFlags::SEARCH_STATIC_METHOD);
        }
        var =
            mexpr->Object()->TsType()->AsETSObjectType()->GetProperty(mexpr->Property()->AsIdentifier()->Name(), flags);
        ES2PANDA_ASSERT(var != nullptr);
    }

    ES2PANDA_ASSERT(var->Declaration()->Node()->IsMethodDefinition());
    auto *method = var->Declaration()->Node()->AsMethodDefinition();

    if (method->IsPrivate() || method->IsProtected()) {
        // Direct reference to method will be impossible from the lambda class, so replace func ref with a lambda
        // that will translate to a proxy method
        auto *lam = CreateWrappingLambda(ctx, funcRef);
        return lam == nullptr ? funcRef : ConvertLambda(ctx, lam);
    }

    LambdaInfo info = GenerateLambdaInfoForFunctionReference(ctx, funcRef, method);

    ES2PANDA_ASSERT(funcRef->TsType()->IsETSArrowType());
    auto *lambdaClass = CreateLambdaClass(ctx, funcRef->TsType()->AsETSFunctionType(), method, &info);
    auto *constructorCall = CreateConstructorCall(ctx, funcRef, lambdaClass, &info);
    ES2PANDA_ASSERT(constructorCall);
    if (constructorCall->TsType()->IsETSObjectType()) {
        constructorCall->TsType()->AsETSObjectType()->AddObjectFlag(checker::ETSObjectFlags::FUNCTIONAL_REFERENCE);
    }

    return constructorCall;
}

static bool IsVariableOriginalAccessor(const varbinder::Variable *var)
{
    return checker::ETSChecker::IsVariableGetterSetter(var) && !(checker::ETSChecker::IsVariableExtensionAccessor(var));
}

static bool IsFunctionOrMethodCall(checker::ETSChecker *checker, ir::CallExpression const *node)
{
    auto const *callee = node->Callee();
    if (callee->TsType() != nullptr && callee->TsType()->IsETSExtensionFuncHelperType()) {
        return true;
    }

    // NOTE: Skip if invoke pattern Union.<method>()
    // Not skip if invoke pattern Union.<field>() where field is of ETSArrowType
    if (callee->IsMemberExpression()) {
        auto me = callee->AsMemberExpression();
        ES2PANDA_ASSERT(me->TsType() != nullptr);
        if (me->Object()->TsType() != nullptr && checker->GetApparentType(me->Object()->TsType())->IsETSUnionType() &&
            me->TsType()->IsETSMethodType()) {
            return true;
        }
    }

    varbinder::Variable *var = nullptr;
    if (callee->IsMemberExpression() &&
        (callee->AsMemberExpression()->Kind() & ir::MemberExpressionKind::PROPERTY_ACCESS) != 0) {
        var = callee->AsMemberExpression()->Property()->Variable();
    } else if (callee->IsIdentifier()) {
        var = callee->AsIdentifier()->Variable();
    }

    return var != nullptr && !IsVariableOriginalAccessor(var) && (var->Flags() & varbinder::VariableFlags::METHOD) != 0;
}

static bool IsTypeErrorCall(ir::CallExpression const *node)
{
    auto const *callee = node->Callee();
    ES2PANDA_ASSERT(callee->TsType() != nullptr);
    return callee->TsType()->IsTypeError();
}

static ir::AstNode *TransformTupleSpread(public_lib::Context *ctx, ir::CallExpression *call)
{
    auto *allocator = ctx->allocator;
    auto *checker = ctx->GetChecker()->AsETSChecker();
    ArenaVector<ir::Expression *> newArgs(allocator->Adapter());
    bool modified = false;

    for (auto *arg : call->Arguments()) {
        if (!arg->IsSpreadElement() || !arg->TsType()->IsETSTupleType()) {
            newArgs.push_back(arg);
            continue;
        }
        modified = true;

        std::stringstream ss;
        auto *genSymIdent = Gensym(allocator);
        ss << "let @@I1: @@T2 = @@E3;";
        ss << "@@E4 as FixedArray<Any>";

        ArenaVector<ir::Expression *> tupleElements(allocator->Adapter());
        for (std::size_t idx = 0U; idx < arg->TsType()->AsETSTupleType()->GetTupleSize(); ++idx) {
            auto *ident = genSymIdent->Clone(allocator, nullptr);
            auto *number = allocator->New<ir::NumberLiteral>(lexer::Number(idx));
            auto *indexed = util::NodeAllocator::ForceSetParent<ir::MemberExpression>(
                allocator, ident, number, ir::MemberExpressionKind::ELEMENT_ACCESS, true, false);
            tupleElements.push_back(indexed);
        }
        auto arrayExpr =
            util::NodeAllocator::ForceSetParent<ir::ArrayExpression>(allocator, std::move(tupleElements), allocator);

        auto typeNode = util::NodeAllocator::ForceSetParent<ir::OpaqueTypeNode>(allocator, arg->TsType(), allocator);

        auto *blockExpression = ctx->parser->AsETSParser()->CreateFormattedExpression(
            ss.str(), genSymIdent, typeNode, arg->AsSpreadElement()->Argument()->Clone(allocator, nullptr), arrayExpr);

        auto *spreadElement = util::NodeAllocator::ForceSetParent<ir::SpreadElement>(
            allocator, ir::AstNodeType::SPREAD_ELEMENT, allocator, blockExpression);
        newArgs.push_back(spreadElement);
        spreadElement->SetParent(call);

        CheckLoweredNode(checker->VarBinder()->AsETSBinder(), checker, spreadElement);
    }
    if (modified) {
        call->Arguments() = std::move(newArgs);
    }
    return call;
}

// CC-OFFNXT(G.FUN.01, huge_method, huge_method[C++]) solid logic
static ir::AstNode *InsertInvokeCall(public_lib::Context *ctx, ir::CallExpression *call)
{
    auto *allocator = ctx->allocator;
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *varBinder = checker->VarBinder()->AsETSBinder();

    auto *oldCallee = call->Callee();
    auto *oldType = checker->GetApparentType(oldCallee->TsType());
    ES2PANDA_ASSERT(oldType != nullptr);
    size_t arity = call->Arguments().size();
    auto *ifaceType = oldType->IsETSObjectType()
                          ? oldType->AsETSObjectType()
                          : oldType->AsETSFunctionType()->ArrowToFunctionalInterfaceDesiredArity(checker, arity);
    ES2PANDA_ASSERT(ifaceType != nullptr);
    bool hasRestParam =
        (oldType->IsETSFunctionType() && oldType->AsETSFunctionType()->ArrowSignature()->HasRestParameter()) ||
        call->Signature()->HasRestParameter();
    util::StringView invokeMethodName =
        util::UString {checker->FunctionalInterfaceInvokeName(arity, hasRestParam), allocator}.View();

    auto *prop = ifaceType->GetProperty(invokeMethodName, checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD |
                                                              checker::PropertySearchFlags::SEARCH_IN_INTERFACES);
    ES2PANDA_ASSERT(prop != nullptr);
    auto *invoke0Id = allocator->New<ir::Identifier>(invokeMethodName, allocator);
    ES2PANDA_ASSERT(invoke0Id != nullptr);
    invoke0Id->SetTsType(prop->TsType());
    invoke0Id->SetVariable(prop);

    auto *newCallee = util::NodeAllocator::ForceSetParent<ir::MemberExpression>(
        allocator, oldCallee, invoke0Id, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
    ES2PANDA_ASSERT(newCallee != nullptr);
    newCallee->SetTsType(prop->TsType());
    newCallee->SetObjectType(ifaceType);

    /* Pull out substituted call signature */
    auto *funcIface =
        ifaceType->HasObjectFlag(checker::ETSObjectFlags::INTERFACE) ? ifaceType : ifaceType->Interfaces()[0];
    checker::Signature *callSig = funcIface->GetFunctionalInterfaceInvokeType()->CallSignatures()[0];
    ES2PANDA_ASSERT(callSig != nullptr);

    call->SetCallee(newCallee);
    call->SetSignature(callSig);

    /* NOTE(gogabr): argument types may have been spoiled by widening/narrowing conversions.
       Repair them here.
       In the future, make sure those conversions behave appropriately.
    */
    for (auto *arg : call->Arguments()) {
        if (arg->IsSpreadElement()) {
            continue;
        }
        CheckLoweredNode(varBinder, checker, arg);
    }

    return TransformTupleSpread(ctx, call);
}

static bool IsRedirectingConstructorCall(ir::CallExpression *expr)
{
    return expr->Callee()->IsThisExpression() || expr->Callee()->IsSuperExpression();
}

static bool IsInCalleePosition(ir::Expression *expr)
{
    return expr->Parent()->IsCallExpression() && expr->Parent()->AsCallExpression()->Callee() == expr;
}

static bool IsEnumFunctionCall(const ir::Identifier *const id)
{
    if (id->Parent() != nullptr && id->Parent()->IsMemberExpression()) {
        const auto *const expr = id->Parent()->AsMemberExpression();
        if (expr->Object()->TsType()->IsETSEnumType()) {
            return true;
        }
    }

    return false;
}

static bool IsValidFunctionDeclVar(const varbinder::Variable *const var)
{
    // Note: If a function is accessor, then no need to build lambda class.
    return var != nullptr && var->Declaration() != nullptr && var->Declaration()->IsFunctionDecl() &&
           !var->TsType()->HasTypeFlag(checker::TypeFlag::GETTER_SETTER);
}

static bool IsOverloadedName(ir::Expression *const expr)
{
    if ((!expr->IsIdentifier() && !expr->IsMemberExpression()) || !expr->Parent()->IsOverloadDeclaration()) {
        return false;
    }

    auto overloadedList = expr->Parent()->AsOverloadDeclaration()->OverloadedList();
    return std::any_of(overloadedList.begin(), overloadedList.end(),
                       [&expr](const ir::Expression *overloadedName) { return overloadedName == expr; });
}

static ir::AstNode *BuildLambdaClassWhenNeeded(public_lib::Context *ctx, ir::AstNode *node)
{
    if (node->IsArrowFunctionExpression()) {
        return ConvertLambda(ctx, node->AsArrowFunctionExpression());
    }

    if (node->IsIdentifier()) {
        auto *id = node->AsIdentifier();
        auto *var = id->Variable();
        // We are running this lowering only for ETS files
        // so it is correct to pass ETS extension here to isReference()
        if (id->IsReference(ScriptExtension::ETS) && id->TsType() != nullptr && id->TsType()->IsETSFunctionType() &&
            !IsInCalleePosition(id) && !IsEnumFunctionCall(id) && IsValidFunctionDeclVar(var) &&
            !IsOverloadedName(id)) {
            return ConvertFunctionReference(ctx, id);
        }
    }
    if (node->IsMemberExpression()) {
        auto *mexpr = node->AsMemberExpression();
        if (mexpr->Kind() == ir::MemberExpressionKind::PROPERTY_ACCESS && mexpr->TsType() != nullptr &&
            mexpr->TsType()->IsETSFunctionType() && mexpr->Object()->TsType()->IsETSObjectType() &&
            mexpr->PropVar() != nullptr && !mexpr->PropVar()->HasFlag(varbinder::VariableFlags::DYNAMIC)) {
            ES2PANDA_ASSERT(mexpr->Property()->IsIdentifier());
            auto *var = mexpr->Object()->TsType()->AsETSObjectType()->GetProperty(
                mexpr->Property()->AsIdentifier()->Name(),
                checker::PropertySearchFlags::SEARCH_INSTANCE_METHOD |
                    checker::PropertySearchFlags::SEARCH_STATIC_METHOD | checker::PropertySearchFlags::SEARCH_IN_BASE |
                    checker::PropertySearchFlags::DISALLOW_SYNTHETIC_METHOD_CREATION);
            if (IsValidFunctionDeclVar(var) && !IsInCalleePosition(mexpr) && !IsOverloadedName(mexpr)) {
                return ConvertFunctionReference(ctx, mexpr);
            }
        }
    }
    return node;
}

static ir::AstNode *LowerTypeNodeIfNeeded(public_lib::Context *ctx, ir::AstNode *node)
{
    if (!node->IsExpression() || !node->AsExpression()->IsTypeNode()) {
        return node;
    }

    auto type = node->AsExpression()->AsTypeNode()->TsType();
    if (type == nullptr || !type->IsETSArrowType()) {
        return node;
    }

    auto allocator = ctx->allocator;
    auto checker = ctx->GetChecker()->AsETSChecker();

    auto newTypeNode =
        allocator->New<ir::OpaqueTypeNode>(type->AsETSFunctionType()->ArrowToFunctionalInterface(checker), allocator);
    newTypeNode->SetParent(node->Parent());
    return newTypeNode;
}

bool LambdaConversionPhase::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    auto *varBinder = ctx->GetChecker()->VarBinder()->AsETSBinder();
    varbinder::RecordTableContext bctx {varBinder, program == ctx->parserProgram ? nullptr : program};
    parser::SavedFormattingFileName savedFormattingName(ctx->parser->AsETSParser(), "lambda-conversion");

    // For reproducibility of results when several compilation sessions are executed during
    // the same process's lifetime.
    if (program == ctx->parserProgram &&
        ctx->config->options->GetCompilationMode() != CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE) {
        ResetCalleeCount();
    }

    program->Ast()->TransformChildrenRecursivelyPostorder(
        [ctx](ir::AstNode *node) { return BuildLambdaClassWhenNeeded(ctx, node); }, Name());

    program->Ast()->TransformChildrenRecursivelyPreorder(
        [ctx](ir::AstNode *node) { return LowerTypeNodeIfNeeded(ctx, node); }, Name());

    auto insertInvokeIfNeeded = [ctx](ir::AstNode *node) {
        if (node->IsCallExpression() &&
            !IsFunctionOrMethodCall(ctx->GetChecker()->AsETSChecker(), node->AsCallExpression()) &&
            !IsRedirectingConstructorCall(node->AsCallExpression()) && !IsTypeErrorCall(node->AsCallExpression())) {
            return InsertInvokeCall(ctx, node->AsCallExpression());
        }
        return node;
    };

    // at this moment, the AST in subexpressions is not consistent, so the preorder is chosen
    program->Ast()->TransformChildrenRecursivelyPreorder(insertInvokeIfNeeded, Name());

    return true;
}

bool LambdaConversionPhase::PostconditionForModule([[maybe_unused]] public_lib::Context *ctx,
                                                   parser::Program const *program)
{
    return !program->Ast()->IsAnyChild([](ir::AstNode const *node) { return node->IsArrowFunctionExpression(); });
}

}  // namespace ark::es2panda::compiler
