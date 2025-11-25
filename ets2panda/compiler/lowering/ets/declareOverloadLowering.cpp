/*
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

#include "declareOverloadLowering.h"

#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "compiler/lowering/util.h"
#include "util/options.h"

namespace ark::es2panda::compiler {

void GenerateOverloadHelperParams(public_lib::Context *ctx, uint32_t minArg, size_t maxArg, bool hasRestVar,
                                  ArenaVector<ir::Expression *> &params)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *allocator = ctx->allocator;

    if (!hasRestVar) {
        for (size_t idx = 0; idx < maxArg; ++idx) {
            auto *id = Gensym(allocator);
            auto *typeAnnotation = ctx->AllocNode<ir::OpaqueTypeNode>(checker->GlobalETSAnyType(), allocator);
            id->SetTsTypeAnnotation(typeAnnotation);
            typeAnnotation->SetParent(id);
            auto *param = ctx->AllocNode<ir::ETSParameterExpression>(id, false, allocator);
            ES2PANDA_ASSERT(param);
            param->SetOptional(idx >= minArg);
            params.push_back(param);
        }
        return;
    }

    auto *restIdent = Gensym(allocator);
    auto *spread = ctx->AllocNode<ir::SpreadElement>(ir::AstNodeType::REST_ELEMENT, allocator, restIdent);
    auto *arr = checker->CreateETSArrayType(checker->GlobalETSAnyType(), false);
    auto *typeAnnotation = ctx->AllocNode<ir::OpaqueTypeNode>(arr, allocator);

    spread->SetTsTypeAnnotation(typeAnnotation);
    spread->SetTsType(arr);
    restIdent->SetTsType(arr);
    auto *param = ctx->AllocNode<ir::ETSParameterExpression>(spread, nullptr, allocator);
    ES2PANDA_ASSERT(param);

    restIdent->SetParent(spread);
    typeAnnotation->SetParent(spread);
    spread->SetParent(param);
    params.push_back(param);
}

void BuildOverloadHelperFunction(public_lib::Context *ctx, ir::MethodDefinition *method)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *allocator = ctx->allocator;
    auto *varBinder = ctx->GetChecker()->VarBinder()->AsETSBinder();

    auto const &[minArg, maxArg, needHelperOverload, isDeclare, hasRestVar, returnVoid] = method->GetOverloadInfo();
    ES2PANDA_ASSERT(needHelperOverload && method->Function() != nullptr);

    auto params = ArenaVector<ir::Expression *>(allocator->Adapter());
    GenerateOverloadHelperParams(ctx, minArg, maxArg, hasRestVar, params);

    auto *returnType = returnVoid ? checker->GlobalVoidType() : checker->GlobalETSAnyType();
    auto *returnAnno = ctx->AllocNode<ir::OpaqueTypeNode>(returnType, allocator);

    ir::ScriptFunctionFlags functionFlag = method->Function()->Flags();
    auto *func = ctx->AllocNode<ir::ScriptFunction>(
        allocator,
        ir::ScriptFunction::ScriptFunctionData {nullptr, ir::FunctionSignature(nullptr, std::move(params), returnAnno),
                                                functionFlag, method->Function()->Modifiers()});
    ES2PANDA_ASSERT(func != nullptr && method->Id() != nullptr);
    auto *methodId = ctx->AllocNode<ir::Identifier>(method->Id()->Name(), allocator);
    func->SetIdent(methodId);
    auto *funcExpr = ctx->AllocNode<ir::FunctionExpression>(func);
    auto *methodIdClone = methodId->Clone(allocator, nullptr);
    auto *helperOverload = ctx->AllocNode<ir::MethodDefinition>(method->Kind(), methodIdClone, funcExpr,
                                                                method->Modifiers(), allocator, false);

    method->AddOverload(helperOverload);
    ES2PANDA_ASSERT(helperOverload->Function() != nullptr);
    ES2PANDA_ASSERT(helperOverload->Id() != nullptr);
    helperOverload->Function()->ClearFlag((ir::ScriptFunctionFlags::OVERLOAD));
    helperOverload->SetParent(method);

    checker::SavedCheckerContext scc(checker, checker->Context().Status(), method->Function()->Signature()->Owner());
    Rebind(ctx->phaseManager, varBinder, helperOverload);
    helperOverload->Function()->AddFlag((ir::ScriptFunctionFlags::OVERLOAD));
    auto funcScope = helperOverload->Function()->Scope();
    funcScope->BindName(method->Function()->Scope()->Name());

    helperOverload->Function()->Id()->SetVariable(helperOverload->Id()->Variable());
    checker->BuildFunctionSignature(helperOverload->Function(), method->IsConstructor());

    auto *const overloadType = checker->BuildMethodType(helperOverload->Function());
    helperOverload->SetTsType(overloadType);

    ES2PANDA_ASSERT(method->TsType()->IsETSFunctionType());
    method->TsType()->AsETSFunctionType()->SetHelperSignature(helperOverload->Function()->Signature());
}

static checker::ETSObjectType *FindContainingClass(ir::CallExpression *expr)
{
    varbinder::Scope *scope = NearestScope(expr);
    while (scope != nullptr && !scope->IsClassScope()) {
        scope = scope->Parent();
    }

    if (scope == nullptr || scope->AsClassScope()->Node() == nullptr) {
        return nullptr;
    }

    return scope->AsClassScope()->Node()->AsClassDefinition()->TsType()->AsETSObjectType();
}

static void UpdateCallSignature(public_lib::Context *ctx, ir::CallExpression *expr)
{
    ES2PANDA_ASSERT(expr->Signature()->HasSignatureFlag(checker::SignatureFlags::DUPLICATE_ASM));
    auto *checker = ctx->GetChecker()->AsETSChecker();
    expr->SetTsType(nullptr);
    checker->Context().SetContainingClass(FindContainingClass(expr));
    expr->Check(checker);
}

bool DeclareOverloadLowering::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    // Note: Generate helper overload method
    auto const transformMethodDef = [ctx](ir::AstNode *ast) {
        if (ast->IsMethodDefinition() && ast->AsMethodDefinition()->GetOverloadInfo().needHelperOverload) {
            BuildOverloadHelperFunction(ctx, ast->AsMethodDefinition());
        }
        return ast;
    };

    // Note: Update signature for call expression
    auto const transformCallExpr = [ctx](ir::AstNode *ast) {
        if (!ast->IsCallExpression() || ast->AsCallExpression()->Signature() == nullptr) {
            return ast;
        }
        if (ast->AsCallExpression()->Signature()->HasSignatureFlag(checker::SignatureFlags::DUPLICATE_ASM)) {
            UpdateCallSignature(ctx, ast->AsCallExpression());
        }
        return ast;
    };

    program->Ast()->TransformChildrenRecursively(
        [transformMethodDef, transformCallExpr](ir::AstNode *ast) {
            return transformMethodDef(transformCallExpr(ast));
        },
        Name());
    return true;
}

}  // namespace ark::es2panda::compiler
