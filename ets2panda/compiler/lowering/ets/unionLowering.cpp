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

#include <algorithm>
#include "unionLowering.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "compiler/lowering/util.h"
#include "varbinder/ETSBinder.h"
#include "checker/ETSchecker.h"
#include "util/nameMangler.h"

namespace ark::es2panda::compiler {

static void ReplaceAll(std::string &str, std::string_view substr, std::string_view replacement)
{
    for (size_t pos = str.find(substr, 0); pos != std::string::npos; pos = str.find(substr, pos)) {
        str.replace(pos, substr.size(), replacement);
        pos += replacement.size();
    }
}

std::string GetAccessClassName(const checker::ETSUnionType *unionType)
{
    std::stringstream ss;
    unionType->ToString(ss, false);
    std::string newName = util::NameMangler::GetInstance()->CreateMangledNameForUnionProperty(ss.str());
    ReplaceAll(newName, "[]", "[$]$");
    return newName;
}

static ir::ClassDefinition *GetUnionAccessClass(public_lib::Context *ctx, varbinder::VarBinder *varbinder,
                                                std::string const &name)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *allocator = ctx->Allocator();
    // Create the name for the synthetic class node
    if (auto foundVar = checker->Scope()->FindLocal(util::StringView(name), varbinder::ResolveBindingOptions::BINDINGS);
        foundVar != nullptr) {
        return foundVar->Declaration()->Node()->AsClassDefinition();
    }
    util::UString unionFieldClassName(util::StringView(name), allocator);
    auto *ident = ctx->AllocNode<ir::Identifier>(unionFieldClassName.View(), allocator);
    auto [decl, var] = varbinder->NewVarDecl<varbinder::ClassDecl>(ident->Start(), ident->Name());
    ES2PANDA_ASSERT(ident != nullptr);
    ident->SetVariable(var);

    auto classCtx = varbinder::LexicalScope<varbinder::ClassScope>(varbinder);
    auto *classDef = ctx->AllocNode<ir::ClassDefinition>(ctx->Allocator(), ident, ir::ClassDefinitionModifiers::GLOBAL,
                                                         ir::ModifierFlags::ABSTRACT, Language(Language::Id::ETS));
    ES2PANDA_ASSERT(classDef != nullptr);
    classDef->SetScope(classCtx.GetScope());
    auto *classDecl = ctx->AllocNode<ir::ClassDeclaration>(classDef, allocator);
    ES2PANDA_ASSERT(classDecl != nullptr);
    classDef->Scope()->BindNode(classDecl->Definition());
    decl->BindNode(classDef);
    var->SetScope(classDef->Scope());

    varbinder->AsETSBinder()->BuildClassDefinition(classDef);

    auto globalBlock = varbinder->Program()->Ast();
    classDecl->SetParent(globalBlock);
    globalBlock->AddStatement(classDecl);
    classDecl->Check(checker);
    return classDef;
}

static std::tuple<varbinder::LocalVariable *, checker::Signature *> CreateNamedAccessMethod(
    public_lib::Context *ctx, varbinder::VarBinder *varbinder, ir::MemberExpression *expr,
    checker::Signature *signature)
{
    auto *allocator = ctx->Allocator();
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto apparentType = checker->GetApparentType(checker->GetNonNullishType(expr->Object()->TsType()));
    ES2PANDA_ASSERT(apparentType != nullptr);
    auto unionType = apparentType->AsETSUnionType();
    auto *const accessClass = GetUnionAccessClass(ctx, varbinder, GetAccessClassName(unionType));
    auto methodName = expr->TsType()->AsETSFunctionType()->Name();

    // Create method name for synthetic class
    auto *methodIdent = ctx->AllocNode<ir::Identifier>(methodName, allocator);

    ArenaVector<ir::Expression *> params {allocator->Adapter()};
    for (auto param : signature->Function()->Params()) {
        params.emplace_back(param->Clone(allocator, nullptr)->AsETSParameterExpression());
    }
    auto returnTypeAnno = ctx->AllocNode<ir::OpaqueTypeNode>(signature->ReturnType(), allocator);

    auto *func = ctx->AllocNode<ir::ScriptFunction>(
        allocator, ir::ScriptFunction::ScriptFunctionData {
                       // CC-OFFNXT(G.FMT.02-CPP) project code style
                       nullptr, ir::FunctionSignature(nullptr, std::move(params), returnTypeAnno),
                       // CC-OFFNXT(G.FMT.02-CPP) project code style
                       ir::ScriptFunctionFlags::METHOD, ir::ModifierFlags::PUBLIC});
    ES2PANDA_ASSERT(func != nullptr && methodIdent != nullptr);
    func->SetIdent(methodIdent->Clone(allocator, nullptr));

    // Create the synthetic function node
    auto *funcExpr = ctx->AllocNode<ir::FunctionExpression>(func);

    // Create the synthetic method definition node
    auto *method =
        ctx->AllocNode<ir::MethodDefinition>(ir::MethodDefinitionKind::METHOD, methodIdent, funcExpr,
                                             ir::ModifierFlags::PUBLIC | ir::ModifierFlags::ABSTRACT, allocator, false);
    ArenaVector<ir::AstNode *> methodDecl {allocator->Adapter()};
    methodDecl.push_back(method);
    accessClass->AddProperties(std::move(methodDecl));

    {
        auto clsCtx =
            varbinder::LexicalScope<varbinder::ClassScope>::Enter(varbinder, accessClass->Scope()->AsClassScope());
        auto boundCtx = varbinder::BoundContext(varbinder->AsETSBinder()->GetRecordTable(), accessClass, true);
        CheckLoweredNode(varbinder->AsETSBinder(), checker, method);
    }
    ES2PANDA_ASSERT(method->Id() != nullptr && method->TsType() != nullptr);
    return {method->Id()->Variable()->AsLocalVariable(),
            method->TsType()->AsETSFunctionType()->CallSignatures().front()};
}

static varbinder::LocalVariable *CreateNamedAccessProperty(public_lib::Context *ctx, varbinder::VarBinder *varbinder,
                                                           ir::MemberExpression *expr)
{
    auto *const allocator = ctx->Allocator();
    auto *checker = ctx->GetChecker()->AsETSChecker();

    auto apparentType = checker->GetApparentType(checker->GetNonNullishType(expr->Object()->TsType()));
    ES2PANDA_ASSERT(apparentType != nullptr);
    auto unionType = apparentType->AsETSUnionType();
    auto *const accessClass = GetUnionAccessClass(ctx, varbinder, GetAccessClassName(unionType));
    auto propName = expr->Property()->AsIdentifier()->Name();
    auto fieldType = expr->TsType();
    auto uncheckedType = expr->UncheckedType();
    auto *typeToSet = uncheckedType == nullptr ? fieldType : uncheckedType;

    // Create field name for synthetic class
    auto *fieldIdent = ctx->AllocNode<ir::Identifier>(propName, allocator);

    // Create the synthetic class property node
    auto *field =
        ctx->AllocNode<ir::ClassProperty>(fieldIdent, nullptr, nullptr, ir::ModifierFlags::NONE, allocator, false);
    ES2PANDA_ASSERT(field != nullptr);
    // Add the declaration to the scope
    auto [decl, var] = varbinder->NewVarDecl<varbinder::LetDecl>(fieldIdent->Start(), fieldIdent->Name());
    var->AddFlag(varbinder::VariableFlags::PROPERTY);
    var->SetTsType(typeToSet);
    fieldIdent->SetVariable(var);
    field->SetTsType(typeToSet);
    decl->BindNode(field);

    ArenaVector<ir::AstNode *> fieldDecl {allocator->Adapter()};
    fieldDecl.push_back(field);
    accessClass->AddProperties(std::move(fieldDecl));
    return var->AsLocalVariable();
}

static varbinder::LocalVariable *CreateNamedAccess(public_lib::Context *ctx, varbinder::VarBinder *varbinder,
                                                   ir::MemberExpression *expr)
{
    auto type = expr->TsType();
    auto name = expr->Property()->AsIdentifier()->Name();
    auto *checker = ctx->GetChecker()->AsETSChecker();
    auto *apparentType = checker->GetApparentType(checker->GetNonNullishType(expr->Object()->TsType()));
    ES2PANDA_ASSERT(apparentType != nullptr);
    auto unionType = apparentType->AsETSUnionType();
    auto *const accessClass = GetUnionAccessClass(ctx, varbinder, GetAccessClassName(unionType));
    auto *classScope = accessClass->Scope()->AsClassScope();

    if (auto *var = classScope->FindLocal(name, varbinder::ResolveBindingOptions::ALL_NON_STATIC); var != nullptr) {
        return var->AsLocalVariable();
    }

    // arrow type fields should be processed as property access not method invocation
    if (type->IsETSMethodType() && !type->IsETSArrowType()) {
        auto parent = expr->Parent()->AsCallExpression();
        ES2PANDA_ASSERT(parent->Callee() == expr && parent->Signature()->HasFunction());

        auto [var, sig] = CreateNamedAccessMethod(ctx, varbinder, expr, parent->Signature());
        parent->AsCallExpression()->SetSignature(sig);
        return var;
    }

    // Enter the union filed class instance field scope
    auto fieldCtx = varbinder::LexicalScope<varbinder::LocalScope>::Enter(varbinder, classScope->InstanceFieldScope());
    return CreateNamedAccessProperty(ctx, varbinder, expr);
}

static void HandleUnionPropertyAccess(public_lib::Context *ctx, varbinder::VarBinder *vbind, ir::MemberExpression *expr)
{
    if (expr->PropVar() != nullptr) {
        return;
    }

    [[maybe_unused]] auto const *const parent = expr->Parent();
    expr->SetPropVar(CreateNamedAccess(ctx, vbind, expr));
    ES2PANDA_ASSERT(expr->PropVar() != nullptr);
}

bool UnionLowering::PerformForModule(public_lib::Context *ctx, parser::Program *program)
{
    checker::ETSChecker *checker = ctx->GetChecker()->AsETSChecker();

    program->Ast()->TransformChildrenRecursively(
        [ctx, checker](checker::AstNodePtr ast) -> checker::AstNodePtr {
            if (ast->IsMemberExpression() && ast->AsMemberExpression()->Object()->TsType() != nullptr) {
                auto *objType =
                    checker->GetApparentType(checker->GetNonNullishType(ast->AsMemberExpression()->Object()->TsType()));
                if (objType->IsETSUnionType()) {
                    HandleUnionPropertyAccess(ctx, checker->VarBinder(), ast->AsMemberExpression());
                    return ast;
                }
            }

            return ast;
        },
        Name());

    return true;
}

bool UnionLowering::PostconditionForModule(public_lib::Context *ctx, const parser::Program *program)
{
    auto *checker = ctx->GetChecker()->AsETSChecker();
    bool current = !program->Ast()->IsAnyChild([checker](ir::AstNode *ast) {
        if (!ast->IsMemberExpression() || ast->AsMemberExpression()->Object()->TsType() == nullptr) {
            return false;
        }
        auto *objType =
            checker->GetApparentType(checker->GetNonNullishType(ast->AsMemberExpression()->Object()->TsType()));
        auto *parent = ast->Parent();
        if (!parent->IsCallExpression() || parent->AsCallExpression()->Signature() == nullptr ||
            parent->AsCallExpression()->Signature()->HasFunction()) {
            return false;
        }
        return objType->IsETSUnionType() && ast->AsMemberExpression()->PropVar() == nullptr;
    });
    if (!current || ctx->config->options->GetCompilationMode() != CompilationMode::GEN_STD_LIB) {
        return current;
    }

    return true;
}

}  // namespace ark::es2panda::compiler
