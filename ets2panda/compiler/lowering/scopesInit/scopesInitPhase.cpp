/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#include "scopesInitPhase.h"
#include "compiler/lowering/scopesInit/savedBindingsCtx.h"
#include "compiler/lowering/util.h"
#include "varbinder/tsBinding.h"

namespace ark::es2panda::compiler {

template <typename T>
varbinder::LexicalScope<T> LexicalScopeCreateOrEnter(varbinder::VarBinder *varBinder, ir::AstNode *ast)
{
    if (ast != nullptr && ast->Scope() != nullptr) {
        return varbinder::LexicalScope<T>::Enter(varBinder, reinterpret_cast<T *>(ast->Scope()));
    }
    return varbinder::LexicalScope<T>(varBinder);
}

template <typename T, typename... Args>
T *AddOrGetDecl(varbinder::VarBinder *varBinder, util::StringView name, ir::AstNode *ast,
                const lexer::SourcePosition &pos, Args &&...args)
{
    if (name.Is(ERROR_LITERAL)) {
        return nullptr;
    }

    if (auto *var = varBinder->GetScope()->FindLocal(name, varbinder::ResolveBindingOptions::BINDINGS);
        var != nullptr && var->Declaration() != nullptr && var->Declaration()->Node() == ast) {
        return reinterpret_cast<T *>(var->Declaration());
    }

    return varBinder->AddDecl<T>(pos, args...);
}

bool ScopesInitPhase::Perform(PhaseContext *ctx, parser::Program *program)
{
    Prepare(ctx, program);
    program->VarBinder()->InitTopScope();
    HandleBlockStmt(program->Ast(), GetScope());
    Finalize();
    return true;
}

void ScopesInitPhase::VisitScriptFunction(ir::ScriptFunction *scriptFunction)
{
    if (auto *const id = scriptFunction->Id(); id != nullptr && id->Variable() == nullptr) {
        auto const *const curScope = VarBinder()->GetScope();
        auto const &functionName = id->Name();
        auto const res =
            curScope->Find(functionName, scriptFunction->IsStatic() ? varbinder::ResolveBindingOptions::ALL_STATIC
                                                                    : varbinder::ResolveBindingOptions::ALL_NON_STATIC);
        if (res.variable != nullptr && res.variable->Declaration()->IsFunctionDecl()) {
            id->SetVariable(res.variable);
        }
    }

    HandleFunction(scriptFunction);
}

void ScopesInitPhase::VisitBlockStatement(ir::BlockStatement *blockStmt)
{
    auto localCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), blockStmt);
    HandleBlockStmt(blockStmt, GetScope());
}

void ScopesInitPhase::VisitImportDeclaration(ir::ImportDeclaration *importDeclaration)
{
    ImportDeclarationContext importCtx(VarBinder());
    Iterate(importDeclaration);
    importCtx.BindImportDecl(importDeclaration);
}

void ScopesInitPhase::VisitClassStaticBlock(ir::ClassStaticBlock *staticBlock)
{
    Iterate(staticBlock);
}

void ScopesInitPhase::VisitMethodDefinition(ir::MethodDefinition *methodDefinition)
{
    Iterate(methodDefinition);
}

varbinder::FunctionParamScope *ScopesInitPhase::HandleFunctionSig(ir::TSTypeParameterDeclaration *typeParams,
                                                                  const ArenaVector<ir::Expression *> &params,
                                                                  ir::TypeNode *returnType)
{
    auto typeParamsCtx = varbinder::LexicalScope<varbinder::LocalScope>(VarBinder());
    CallNode(typeParams);

    auto lexicalScope = varbinder::LexicalScope<varbinder::FunctionParamScope>(VarBinder());
    CallFuncParams(params);
    CallNode(returnType);

    return lexicalScope.GetScope();
}

void ScopesInitPhase::HandleFunction(ir::ScriptFunction *function)
{
    CallNode(function->Id());

    // NOTE(gogabr): this will skip type/value parameters when they are added to an existing function sig
    auto funcParamScope = (function->Scope() == nullptr) ? HandleFunctionSig(function->TypeParams(), function->Params(),
                                                                             function->ReturnTypeAnnotation())
                                                         : function->Scope()->ParamScope();
    auto paramCtx = varbinder::LexicalScope<varbinder::FunctionParamScope>::Enter(VarBinder(), funcParamScope, false);

    auto functionCtx = LexicalScopeCreateOrEnter<varbinder::FunctionScope>(VarBinder(), function);
    auto *functionScope = functionCtx.GetScope();
    BindFunctionScopes(functionScope, funcParamScope);

    if (function->Body() != nullptr && function->Body()->IsBlockStatement()) {
        HandleBlockStmt(function->Body()->AsBlockStatement(), functionScope);
    } else {
        Iterate(function->Body());
    }
    BindScopeNode(functionScope, function);
    funcParamScope->BindNode(function);

    CallNodeAnnotations(function);
}

void ScopesInitPhase::HandleBlockStmt(ir::BlockStatement *block, varbinder::Scope *scope)
{
    if (block->Scope() == nullptr) {
        BindScopeNode(scope, block);
    }
    Iterate(block);
}

void ScopesInitPhase::VisitClassDefinition(ir::ClassDefinition *classDef)
{
    auto classCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), classDef);
    AddOrGetDecl<varbinder::ConstDecl>(VarBinder(), classDef->InternalName(), classDef, classDef->Start(),
                                       classDef->InternalName());
    BindClassName(classDef);

    auto *classScope = classCtx.GetScope();
    BindScopeNode(classScope, classDef);
    Iterate(classDef);
}

void ScopesInitPhase::VisitForUpdateStatement(ir::ForUpdateStatement *forUpdateStmt)
{
    auto declCtx = (forUpdateStmt->Scope() == nullptr)
                       ? varbinder::LexicalScope<varbinder::LoopDeclarationScope>(VarBinder())
                       : varbinder::LexicalScope<varbinder::LoopDeclarationScope>::Enter(
                             // CC-OFFNXT(G.FMT.06-CPP) project code style
                             VarBinder(), forUpdateStmt->Scope()->DeclScope());
    CallNode(forUpdateStmt->Init());

    auto lexicalScope = LexicalScopeCreateOrEnter<varbinder::LoopScope>(VarBinder(), forUpdateStmt);
    AttachLabelToScope(forUpdateStmt);
    CallNode(forUpdateStmt->Test());
    CallNode(forUpdateStmt->Update());
    CallNode(forUpdateStmt->Body());
    lexicalScope.GetScope()->BindDecls(declCtx.GetScope());
    HandleFor(declCtx.GetScope(), lexicalScope.GetScope(), forUpdateStmt);
}

void ScopesInitPhase::VisitForInStatement(ir::ForInStatement *forInStmt)
{
    auto declCtx = (forInStmt->Scope() == nullptr)
                       ? varbinder::LexicalScope<varbinder::LoopDeclarationScope>(VarBinder())
                       : varbinder::LexicalScope<varbinder::LoopDeclarationScope>::Enter(
                             // CC-OFFNXT(G.FMT.06-CPP) project code style
                             VarBinder(), forInStmt->Scope()->DeclScope());
    CallNode(forInStmt->Left());

    auto lexicalScope = LexicalScopeCreateOrEnter<varbinder::LoopScope>(VarBinder(), forInStmt);
    CallNode(forInStmt->Right());
    CallNode(forInStmt->Body());
    HandleFor(declCtx.GetScope(), lexicalScope.GetScope(), forInStmt);
}

void ScopesInitPhase::VisitForOfStatement(ir::ForOfStatement *forOfStmt)
{
    auto declCtx = (forOfStmt->Scope() == nullptr)
                       ? varbinder::LexicalScope<varbinder::LoopDeclarationScope>(VarBinder())
                       : varbinder::LexicalScope<varbinder::LoopDeclarationScope>::Enter(
                             // CC-OFFNXT(G.FMT.06-CPP) project code style
                             VarBinder(), forOfStmt->Scope()->DeclScope());
    CallNode(forOfStmt->Left());

    auto lexicalScope = LexicalScopeCreateOrEnter<varbinder::LoopScope>(VarBinder(), forOfStmt);
    AttachLabelToScope(forOfStmt);
    CallNode(forOfStmt->Right());
    CallNode(forOfStmt->Body());
    HandleFor(declCtx.GetScope(), lexicalScope.GetScope(), forOfStmt);
}

void ScopesInitPhase::VisitCatchClause(ir::CatchClause *catchClause)
{
    auto catchParamCtx = (catchClause->Scope() == nullptr)
                             ? varbinder::LexicalScope<varbinder::CatchParamScope>(VarBinder())
                             : varbinder::LexicalScope<varbinder::CatchParamScope>::Enter(
                                   // CC-OFFNXT(G.FMT.06-CPP) project code style
                                   VarBinder(), catchClause->Scope()->ParamScope());
    auto *catchParamScope = catchParamCtx.GetScope();
    auto *param = catchClause->Param();

    CallNode(param);

    if (param != nullptr) {
        auto *const var = VarBinder()->AddParamDecl(param);
        if (param->IsIdentifier()) {
            var->SetScope(catchParamScope);
            param->AsIdentifier()->SetVariable(var);
        }
    }

    // Catch Clause is scope bearer
    catchParamScope->BindNode(catchClause);

    auto catchCtx = LexicalScopeCreateOrEnter<varbinder::CatchScope>(VarBinder(), catchClause);
    auto *catchScope = catchCtx.GetScope();

    catchScope->AssignParamScope(catchParamScope);
    auto body = catchClause->Body();
    HandleBlockStmt(body, catchScope);

    BindScopeNode(catchScope, catchClause);
}

void ScopesInitPhase::VisitVariableDeclarator(ir::VariableDeclarator *varDecl)
{
    auto init = varDecl->Id();
    std::vector<ir::Identifier *> bindings = util::Helpers::CollectBindingNames(VarBinder(), init);
    for (auto *binding : bindings) {
        auto name = binding->Name();
        if (binding->Variable() == nullptr && !name.Is(ERROR_LITERAL) && VarBinder()->IsETSBinder()) {
            auto var = VarBinder()->GetScope()->FindLocal(name, varbinder::ResolveBindingOptions::ALL_VARIABLES);
            if (var != nullptr) {
                VarBinder()->ThrowRedeclaration(binding->Start(), name, var->Declaration()->Type());
                continue;
            }
        }

        auto [decl, var] = AddOrGetVarDecl(varDecl->Flag(), binding);
        BindVarDecl(binding, init, decl, var);
    }
    Iterate(varDecl);
}

void ScopesInitPhase::VisitSwitchStatement(ir::SwitchStatement *switchStmt)
{
    CallNode(switchStmt->Discriminant());
    auto localCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), switchStmt);
    AttachLabelToScope(switchStmt);
    BindScopeNode(localCtx.GetScope(), switchStmt);
    CallNode(switchStmt->Cases());
}

void ScopesInitPhase::VisitWhileStatement(ir::WhileStatement *whileStmt)
{
    CallNode(whileStmt->Test());
    auto lexicalScope = LexicalScopeCreateOrEnter<varbinder::LoopScope>(VarBinder(), whileStmt);
    AttachLabelToScope(whileStmt);
    BindScopeNode(lexicalScope.GetScope(), whileStmt);
    CallNode(whileStmt->Body());
}

void ScopesInitPhase::VisitETSStructDeclaration(ir::ETSStructDeclaration *structDecl)
{
    LogDiagnostic(diagnostic::UNEXPECTED_STRUCT, structDecl->Start());
    // For multiple error report
    Iterate(structDecl);
    BindClassDefinition(structDecl->Definition());
}

void ScopesInitPhase::VisitClassDeclaration(ir::ClassDeclaration *classDecl)
{
    Iterate(classDecl);
    BindClassDefinition(classDecl->Definition());
}

void ScopesInitPhase::VisitAnnotationDeclaration(ir::AnnotationDeclaration *annoDecl)
{
    // First check if there is any redefinition in the current scope
    const auto locStart = annoDecl->GetBaseName()->Start();
    const auto &annoName = annoDecl->GetBaseName()->Name();
    AddOrGetDecl<varbinder::AnnotationDecl>(VarBinder(), annoName, annoDecl, locStart, annoName, annoDecl);
    // Create and enter annotation scope
    auto annoCtx = LexicalScopeCreateOrEnter<varbinder::AnnotationScope>(VarBinder(), annoDecl);
    auto *classScope = annoCtx.GetScope();
    BindScopeNode(classScope, annoDecl);
    Iterate(annoDecl);
}

void ScopesInitPhase::VisitAnnotationUsage(ir::AnnotationUsage *annoUsage)
{
    auto annoCtx = LexicalScopeCreateOrEnter<varbinder::AnnotationParamScope>(VarBinder(), annoUsage);
    auto *curScope = annoCtx.GetScope();
    BindScopeNode(curScope, annoUsage);
    Iterate(annoUsage);
}

void ScopesInitPhase::VisitDoWhileStatement(ir::DoWhileStatement *doWhileStmt)
{
    auto lexicalScope = LexicalScopeCreateOrEnter<varbinder::LoopScope>(VarBinder(), doWhileStmt);
    AttachLabelToScope(doWhileStmt);
    BindScopeNode(lexicalScope.GetScope(), doWhileStmt);
    Iterate(doWhileStmt);
}

void ScopesInitPhase::VisitFunctionDeclaration(ir::FunctionDeclaration *funcDecl)
{
    const auto func = funcDecl->Function();
    if (!funcDecl->IsAnonymous()) {
        CreateFuncDecl(func);
    }
    Iterate(funcDecl);
}

void ScopesInitPhase::VisitExportAllDeclaration(ir::ExportAllDeclaration *exportAllDecl)
{
    Iterate(exportAllDecl);
    const auto name = exportAllDecl->Exported() != nullptr ? exportAllDecl->Exported()->Name() : "*";
    auto *decl =
        AddOrGetDecl<varbinder::ExportDecl>(VarBinder(), name, exportAllDecl, exportAllDecl->Start(), name, "*");
    VarBinder()->GetScope()->AsModuleScope()->AddExportDecl(exportAllDecl, decl);
}

void ScopesInitPhase::VisitImportNamespaceSpecifier(ir::ImportNamespaceSpecifier *importSpec)
{
    Iterate(importSpec);
    AddOrGetDecl<varbinder::ImportDecl>(VarBinder(), importSpec->Local()->Name(), importSpec, importSpec->Start(), "*",
                                        importSpec->Local()->Name(), importSpec);
}

void ScopesInitPhase::VisitImportSpecifier(ir::ImportSpecifier *importSpec)
{
    Iterate(importSpec);
    const auto *imported = importSpec->Imported();
    AddOrGetDecl<varbinder::ImportDecl>(VarBinder(), importSpec->Local()->Name(), importSpec, importSpec->Start(),
                                        imported->Name(), importSpec->Local()->Name(), importSpec);
}

void ScopesInitPhase::VisitImportDefaultSpecifier(ir::ImportDefaultSpecifier *importSpec)
{
    Iterate(importSpec);
    const auto *local = importSpec->Local();
    AddOrGetDecl<varbinder::ImportDecl>(VarBinder(), local->Name(), importSpec, local->Start(), "default",
                                        local->Name(), importSpec);
}

void ScopesInitPhase::VisitExportDefaultDeclaration(ir::ExportDefaultDeclaration *exportDecl)
{
    ExportDeclarationContext exportDeclCtx(VarBinder());
    Iterate(exportDecl);
    exportDeclCtx.BindExportDecl(exportDecl);
}

void ScopesInitPhase::VisitArrowFunctionExpression(ir::ArrowFunctionExpression *arrowExpr)
{
    Iterate(arrowExpr);
}

void ScopesInitPhase::VisitDirectEvalExpression(ir::DirectEvalExpression *directCallExpr)
{
    VarBinder()->PropagateDirectEval();
    Iterate(directCallExpr);
}

void ScopesInitPhase::VisitExportNamedDeclaration(ir::ExportNamedDeclaration *exportDecl)
{
    if (exportDecl->Decl() != nullptr) {
        ExportDeclarationContext exportDeclCtx(VarBinder());
        Iterate(exportDecl);
        exportDeclCtx.BindExportDecl(exportDecl);
    } else {
        varbinder::ModuleScope::ExportDeclList exportDecls(program_->Allocator()->Adapter());

        for (auto *spec : exportDecl->Specifiers()) {
            auto *decl =
                AddOrGetDecl<varbinder::ExportDecl>(VarBinder(), spec->Local()->Name(), spec, exportDecl->Start(),
                                                    spec->Exported()->Name(), spec->Local()->Name(), spec);
            exportDecls.push_back(decl);
        }
        VarBinder()->GetScope()->AsModuleScope()->AddExportDecl(exportDecl, std::move(exportDecls));
    }
}

void ScopesInitPhase::VisitTSFunctionType(ir::TSFunctionType *funcType)
{
    auto lexicalScope = LexicalScopeCreateOrEnter<varbinder::FunctionParamScope>(VarBinder(), funcType);
    auto *funcParamScope = lexicalScope.GetScope();
    BindScopeNode(funcParamScope, funcType);
    Iterate(funcType);
}

void ScopesInitPhase::SetProgram(parser::Program *program) noexcept
{
    program_ = program;
}

void ScopesInitPhase::CallFuncParams(const ArenaVector<ir::Expression *> &params)
{
    // NOTE: extract params to separate class
    for (auto *param : params) {
        if (!param->IsETSParameterExpression()) {
            VarBinder()->AddParamDecl(param);
        }
    }
    CallNode(params);
}

void ScopesInitPhase::IterateNoTParams(ir::ClassDefinition *classDef)
{
    CallNode(classDef->Super());
    CallNode(classDef->SuperTypeParams());
    CallNodeAnnotations(classDef);
    CallNode(classDef->Implements());
    CallNode(classDef->Ctor());

    for (auto property : classDef->Body()) {
        if (property->IsOverloadDeclaration()) {
            continue;
        }
        CallNode(property);
    }

    for (auto property : classDef->Body()) {
        if (property->IsOverloadDeclaration()) {
            CallNode(property);
        }
    }
}

void ScopesInitPhase::LogDiagnostic(const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &params,
                                    const lexer::SourcePosition &pos) const
{
    ctx_->diagnosticEngine->LogDiagnostic(kind, params, pos);
}

void ScopesInitPhase::CreateFuncDecl(ir::ScriptFunction *func)
{
    AddOrGetDecl<varbinder::FunctionDecl>(VarBinder(), func->Id()->Name(), func, func->Id()->Start(), Allocator(),
                                          func->Id()->Name(), func);
}

util::StringView ScopesInitPhase::FormInterfaceOrEnumDeclarationIdBinding(ir::Identifier *id)
{
    return id->Name();
}

varbinder::Decl *ScopesInitPhase::BindClassName(ir::ClassDefinition *classDef)
{
    const auto identNode = classDef->Ident();
    if (identNode == nullptr) {
        return nullptr;
    }

    auto identDecl = AddOrGetDecl<varbinder::ConstDecl>(VarBinder(), identNode->Name(), classDef, identNode->Start(),
                                                        identNode->Name());
    if (identDecl != nullptr) {
        identDecl->BindNode(classDef);
    }
    return identDecl;
}

void ScopesInitPhase::BindFunctionScopes(varbinder::FunctionScope *scope, varbinder::FunctionParamScope *paramScope)
{
    scope->BindParamScope(paramScope);
    paramScope->BindFunctionScope(scope);
}

void ScopesInitPhase::BindClassDefinition(ir::ClassDefinition *classDef)
{
    if (classDef->IsGlobal()) {
        return;  // We handle it in ClassDeclaration
    }
    const auto locStart = classDef->Ident()->Start();
    const auto &className = classDef->Ident()->Name();
    if ((classDef->Modifiers() & ir::ClassDefinitionModifiers::CLASS_DECL) != 0U) {
        AddOrGetDecl<varbinder::ClassDecl>(VarBinder(), className, classDef, locStart, className, classDef);
    } else {
        AddOrGetDecl<varbinder::LetDecl>(VarBinder(), className, classDef, locStart, className, classDef);
    }
}

std::tuple<varbinder::Decl *, varbinder::Variable *> ScopesInitPhase::AddOrGetVarDecl(ir::VariableDeclaratorFlag flag,
                                                                                      const ir::Identifier *id)
{
    auto name = id->Name();
    auto scope = VarBinder()->GetScope();
    if (auto var = id->Variable(); var != nullptr) {
        if (!name.Is(ERROR_LITERAL) &&
            (scope->FindLocal(name, varbinder::ResolveBindingOptions::ALL_VARIABLES) == nullptr)) {
            var = scope->AddDecl(Allocator(), var->Declaration(), VarBinder()->Extension());
        }
        return {var->Declaration(), var};
    }

    if (name.Is(ERROR_LITERAL)) {
        name = compiler::GenName(Allocator()).View();
    } else if (VarBinder()->IsETSBinder()) {
        if (auto var = scope->FindLocal(name, varbinder::ResolveBindingOptions::ALL_VARIABLES); var != nullptr) {
            ES2PANDA_ASSERT(ctx_->diagnosticEngine->IsAnyError());
            return {var->Declaration(), var};
        }
    }

    switch (flag) {
        case ir::VariableDeclaratorFlag::LET:
            return VarBinder()->NewVarDecl<varbinder::LetDecl>(id->Start(), name);
        case ir::VariableDeclaratorFlag::VAR:
            if (VarBinder()->IsETSBinder()) {
                return VarBinder()->NewVarDecl<varbinder::LetDecl>(id->Start(), name);
            }
            return VarBinder()->NewVarDecl<varbinder::VarDecl>(id->Start(), name);
        case ir::VariableDeclaratorFlag::CONST:
            return VarBinder()->NewVarDecl<varbinder::ConstDecl>(id->Start(), name);
        default:
            ES2PANDA_UNREACHABLE();
    }
}

void ScopesInitPhase::BindVarDecl([[maybe_unused]] ir::Identifier *binding, ir::Expression *init, varbinder::Decl *decl,
                                  [[maybe_unused]] varbinder::Variable *var)
{
    decl->BindNode(init);
}

void ScopesInitPhase::AttachLabelToScope([[maybe_unused]] ir::AstNode *node) {}

void ScopesInitPhase::VisitFunctionExpression(ir::FunctionExpression *funcExpr)
{
    Iterate(funcExpr);
    if (!funcExpr->IsAnonymous()) {
        auto func = funcExpr->Function();
        auto id = funcExpr->Id();
        auto *funcParamScope = func->Scope()->ParamScope();
        funcParamScope->BindName(Allocator(), id->Name());
        func->SetIdent(id->Clone(Allocator(), nullptr));
    }
}

void ScopesInitPhase::Prepare(ScopesInitPhase::PhaseContext *ctx, parser::Program *program)
{
    ctx_ = ctx;
    program_ = program;
}

void ScopesInitPhase::Finalize()
{
    ES2PANDA_ASSERT(program_->Ast() != nullptr);

    auto const removeDuplicates = [](ir::AstNode *ast) -> void {
        if (!ast->IsTSInterfaceBody()) {
            return;
        }

        auto &body = ast->AsTSInterfaceBody()->Body();
        auto it = body.begin();
        while (it != body.end()) {
            if ((*it)->IsMethodDefinition() &&
                ((*it)->AsMethodDefinition()->Function()->Flags() &
                 (ir::ScriptFunctionFlags::OVERLOAD | ir::ScriptFunctionFlags::EXTERNAL_OVERLOAD)) != 0U) {
                it = body.erase(it);
            } else {
                ++it;
            }
        }
    };

    //  Remove duplicates for overloaded methods.
    program_->Ast()->IterateRecursively(removeDuplicates);

    AnalyzeExports();
}

void ScopesInitPhase::AnalyzeExports()
{
    if (Program()->Kind() == parser::ScriptKind::MODULE && VarBinder()->TopScope()->IsModuleScope() &&
        !VarBinder()->TopScope()->AsModuleScope()->ExportAnalysis()) {
        LogDiagnostic(diagnostic::INVALID_EXPORT, Program()->Ast()->End());
    }
}

void ScopeInitTyped::VisitTSModuleDeclaration(ir::TSModuleDeclaration *moduleDecl)
{
    if (!moduleDecl->IsExternalOrAmbient()) {
        auto name = moduleDecl->Name()->AsIdentifier()->Name();
        auto *decl = AddOrGetDecl<varbinder::VarDecl>(VarBinder(), name, moduleDecl, moduleDecl->Name()->Start(), name);
        ES2PANDA_ASSERT(decl != nullptr);
        decl->BindNode(moduleDecl);
    }
    auto localCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), moduleDecl);
    BindScopeNode(localCtx.GetScope(), moduleDecl);
    Iterate(moduleDecl);
}

void ScopeInitTyped::VisitTSModuleBlock(ir::TSModuleBlock *block)
{
    auto localCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), block);
    Iterate(block);
    BindScopeNode(localCtx.GetScope(), block);
}

void ScopeInitTyped::VisitTSTypeAliasDeclaration(ir::TSTypeAliasDeclaration *typeAliasDecl)
{
    const auto id = typeAliasDecl->Id();
    varbinder::TSBinding tsBinding(Allocator(), id->Name());
    auto *decl = VarBinder()->AddTsDecl<varbinder::TypeAliasDecl>(id->Start(), tsBinding.View());
    auto typeParamsCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), typeAliasDecl->TypeParams());
    decl->BindNode(typeAliasDecl);
    Iterate(typeAliasDecl);
}

util::StringView ScopeInitTyped::FormInterfaceOrEnumDeclarationIdBinding(ir::Identifier *id)
{
    varbinder::TSBinding tsBinding(Allocator(), id->Name());
    return tsBinding.View();
}

void ScopeInitTyped::VisitTSInterfaceDeclaration(ir::TSInterfaceDeclaration *interfDecl)
{
    const auto &bindings = VarBinder()->GetScope()->Bindings();
    const auto ident = interfDecl->Id();
    const auto name = FormInterfaceOrEnumDeclarationIdBinding(ident);
    auto res = bindings.find(name);

    varbinder::InterfaceDecl *decl {};

    bool alreadyExists = false;
    if (res == bindings.end()) {
        decl = VarBinder()->AddTsDecl<varbinder::InterfaceDecl>(ident->Start(), Allocator(), name);
    } else if (!AllowInterfaceRedeclaration()) {
        LogDiagnostic(diagnostic::INTERFACE_REDECLARED, interfDecl->Start());
        return;
    } else if (!res->second->Declaration()->IsInterfaceDecl()) {
        VarBinder()->ThrowRedeclaration(ident->Start(), ident->Name(), res->second->Declaration()->Type());
        return;
    } else {
        decl = res->second->Declaration()->AsInterfaceDecl();
        alreadyExists = true;
    }

    CallNode(ident);
    auto typeParamsCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), interfDecl->TypeParams());
    CallNode(interfDecl->TypeParams());
    CallNode(interfDecl->Extends());

    auto localScope = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), interfDecl);
    auto *identDecl =
        AddOrGetDecl<varbinder::ConstDecl>(VarBinder(), ident->Name(), interfDecl, ident->Start(), ident->Name());
    ES2PANDA_ASSERT(identDecl != nullptr);
    identDecl->BindNode(interfDecl);
    BindScopeNode(localScope.GetScope(), interfDecl);

    CallNode(interfDecl->Body());
    if (decl == nullptr) {  // Error processing.
        return;
    }

    if (!alreadyExists) {
        decl->BindNode(interfDecl);
    }
    decl->Add(interfDecl);
}

void ScopeInitTyped::VisitTSEnumMember(ir::TSEnumMember *enumMember)
{
    const auto key = enumMember->Key();
    util::StringView name;
    if (key->IsIdentifier()) {
        name = key->AsIdentifier()->Name();
    } else if (key->IsStringLiteral()) {
        name = key->AsStringLiteral()->Str();
    } else {
        ES2PANDA_UNREACHABLE();
    }
    if (auto *decl = AddOrGetDecl<varbinder::EnumDecl>(VarBinder(), name, enumMember, key->Start(), name);
        decl != nullptr) {
        decl->BindNode(enumMember);
    }
}

void ScopeInitTyped::VisitTSEnumDeclaration(ir::TSEnumDeclaration *enumDecl)
{
    util::StringView ident = FormInterfaceOrEnumDeclarationIdBinding(enumDecl->Key());
    const auto &bindings = VarBinder()->GetScope()->Bindings();
    varbinder::EnumLiteralDecl *decl {};

    if (auto res = bindings.find(ident); res == bindings.end()) {
        decl = VarBinder()->AddTsDecl<varbinder::EnumLiteralDecl>(enumDecl->Start(), ident, enumDecl->IsConst());
        varbinder::LexicalScope enumCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), enumDecl);
        decl->BindScope(enumCtx.GetScope());
        BindScopeNode(VarBinder()->GetScope()->AsLocalScope(), enumDecl);
    } else {
        if (!res->second->Declaration()->IsEnumLiteralDecl() ||
            (enumDecl->IsConst() ^ res->second->Declaration()->AsEnumLiteralDecl()->IsConst()) != 0) {
            VarBinder()->ThrowRedeclaration(enumDecl->Key()->Start(), enumDecl->Key()->Name(),
                                            res->second->Declaration()->Type());
            return;
        }
        decl = res->second->Declaration()->AsEnumLiteralDecl();
        auto scopeCtx = varbinder::LexicalScope<varbinder::LocalScope>::Enter(VarBinder(), decl->Scope());
    }
    decl->BindNode(enumDecl);
    Iterate(enumDecl);
}

void ScopeInitTyped::VisitTSTypeParameter(ir::TSTypeParameter *typeParam)
{
    auto name = typeParam->Name()->Name();
    if (auto *decl = AddOrGetDecl<varbinder::TypeParameterDecl>(VarBinder(), name, typeParam, typeParam->Start(), name);
        decl != nullptr) {
        decl->BindNode(typeParam);
    }
    Iterate(typeParam);
}

void ScopeInitTyped::VisitTSTypeParameterDeclaration(ir::TSTypeParameterDeclaration *paramDecl)
{
    BindScopeNode(VarBinder()->GetScope()->AsLocalScope(), paramDecl);
    Iterate(paramDecl);
}

void ScopeInitTyped::VisitClassDefinition(ir::ClassDefinition *classDef)
{
    auto typeParamsCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), classDef->TypeParams());
    CallNode(classDef->TypeParams());

    auto classCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), classDef);
    BindClassName(classDef);
    AddOrGetDecl<varbinder::ConstDecl>(VarBinder(), classDef->InternalName(), classDef, classDef->Start(),
                                       classDef->InternalName());
    BindScopeNode(classCtx.GetScope(), classDef);
    IterateNoTParams(classDef);
}

void InitScopesPhaseTs::VisitExportDefaultDeclaration(ir::ExportDefaultDeclaration *exportDecl)
{
    ExportDeclarationContext exportDeclCtx(VarBinder());
    Iterate(exportDecl);
}

void InitScopesPhaseTs::VisitExportNamedDeclaration(ir::ExportNamedDeclaration *exportDecl)
{
    ExportDeclarationContext exportDeclCtx(VarBinder());
    Iterate(exportDecl);
}

void InitScopesPhaseTs::VisitImportDeclaration(ir::ImportDeclaration *importDeclaration)
{
    ImportDeclarationContext importCtx(VarBinder());
    Iterate(importDeclaration);
}

void InitScopesPhaseTs::VisitTSFunctionType(ir::TSFunctionType *constrType)
{
    auto lexicalScope =
        (constrType->Scope() == nullptr)
            ? HandleFunctionSig(constrType->TypeParams(), constrType->Params(), constrType->ReturnType())
            : constrType->Scope();
    BindScopeNode(lexicalScope, constrType);
}

void InitScopesPhaseTs::CreateFuncDecl(ir::ScriptFunction *func)
{
    const auto identNode = func->Id();
    const auto startLoc = identNode->Start();
    const auto &bindings = VarBinder()->GetScope()->Bindings();
    auto res = bindings.find(identNode->Name());
    varbinder::FunctionDecl *decl {};

    if (res == bindings.end()) {
        decl = VarBinder()->AddDecl<varbinder::FunctionDecl>(startLoc, Allocator(), identNode->Name(), func);
    } else {
        varbinder::Decl *currentDecl = res->second->Declaration();

        auto &existing = currentDecl->AsFunctionDecl()->Decls();
        if (std::find(existing.begin(), existing.end(), func) != existing.end()) {
            return;
        }

        if (!currentDecl->IsFunctionDecl() ||
            !currentDecl->AsFunctionDecl()->Node()->AsScriptFunction()->IsOverload()) {
            VarBinder()->ThrowRedeclaration(startLoc, currentDecl->Name(), currentDecl->Type());
            return;
        }
        decl = currentDecl->AsFunctionDecl();
    }

    ES2PANDA_ASSERT(decl != nullptr);
    decl->Add(func);
}

void InitScopesPhaseTs::VisitTSConstructorType(ir::TSConstructorType *constrT)
{
    auto funcParamScope = (constrT->Scope() == nullptr)
                              ? HandleFunctionSig(constrT->TypeParams(), constrT->Params(), constrT->ReturnType())
                              : constrT->Scope();
    BindScopeNode(funcParamScope, constrT);
}

void InitScopesPhaseTs::VisitArrowFunctionExpression(ir::ArrowFunctionExpression *arrowFExpr)
{
    auto typeParamsCtx =
        LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), arrowFExpr->Function()->TypeParams());
    Iterate(arrowFExpr);
}

void InitScopesPhaseTs::VisitTSSignatureDeclaration(ir::TSSignatureDeclaration *signDecl)
{
    auto funcParamScope = (signDecl->Scope() == nullptr) ? HandleFunctionSig(signDecl->TypeParams(), signDecl->Params(),
                                                                             signDecl->ReturnTypeAnnotation())
                                                         : signDecl->Scope();
    BindScopeNode(funcParamScope, signDecl);
}

void InitScopesPhaseTs::VisitTSMethodSignature(ir::TSMethodSignature *methodSign)
{
    auto funcParamScope =
        (methodSign->Scope() == nullptr)
            ? HandleFunctionSig(methodSign->TypeParams(), methodSign->Params(), methodSign->ReturnTypeAnnotation())
            : methodSign->Scope();
    BindScopeNode(funcParamScope, methodSign);
}

void InitScopesPhaseETS::RunExternalNode(ir::AstNode *node, varbinder::VarBinder *varbinder)
{
    auto program = parser::Program(varbinder->Allocator(), varbinder);
    RunExternalNode(node, &program);
}

void InitScopesPhaseETS::RunExternalNode(ir::AstNode *node, parser::Program *ctx)
{
    auto scopesPhase = InitScopesPhaseETS();
    scopesPhase.SetProgram(ctx);
    scopesPhase.CallNode(node);
}

bool InitScopesPhaseETS::Perform(PhaseContext *ctx, parser::Program *program)
{
    Prepare(ctx, program);

    if (program->VarBinder()->TopScope() == nullptr) {
        program->VarBinder()->InitTopScope();
        BindScopeNode(GetScope(), program->Ast());
        AddGlobalToBinder(program);
    } else if ((ctx->compilingState == public_lib::CompilingState::MULTI_COMPILING_FOLLOW) &&
               program->GlobalClass()->Scope() == nullptr) {
        BindScopeNode(GetScope(), program->Ast());
        AddGlobalToBinder(program);
    }
    HandleProgram(program);
    Finalize();
    return true;
}

void InitScopesPhaseETS::HandleProgram(parser::Program *program)
{
    for (auto &[_, progList] : program->ExternalSources()) {
        (void)_;
        auto savedTopScope(program->VarBinder()->TopScope());
        auto mainProg = progList.front();
        if (mainProg->IsASTLowered()) {
            continue;
        }
        mainProg->VarBinder()->InitTopScope();
        AddGlobalToBinder(mainProg);
        BindScopeNode(mainProg->VarBinder()->GetScope(), mainProg->Ast());
        auto globalClass = mainProg->GlobalClass();
        auto globalScope = mainProg->GlobalScope();
        for (auto &prog : progList) {
            if (prog->GetFlag(parser::ProgramFlags::AST_HAS_SCOPES_INITIALIZED)) {
                continue;
            }
            prog->SetGlobalClass(globalClass);
            BindScopeNode(prog->VarBinder()->GetScope(), prog->Ast());
            prog->VarBinder()->ResetTopScope(globalScope);
            if (mainProg->Ast() != nullptr) {
                InitScopesPhaseETS().Perform(Context(), prog);
                prog->SetFlag(parser::ProgramFlags::AST_HAS_SCOPES_INITIALIZED);
            }
        }
        program->VarBinder()->ResetTopScope(savedTopScope);
    }
    ES2PANDA_ASSERT(program->Ast() != nullptr);

    auto *ast = program->Ast();
    HandleETSModule(ast);
}

void InitScopesPhaseETS::BindVarDecl(ir::Identifier *binding, ir::Expression *init, varbinder::Decl *decl,
                                     varbinder::Variable *var)
{
    binding->SetVariable(var);
    var->SetScope(VarBinder()->GetScope());
    var->AddFlag(varbinder::VariableFlags::LOCAL);
    decl->BindNode(init);
}

void InitScopesPhaseETS::VisitBlockExpression(ir::BlockExpression *blockExpr)
{
    auto localCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), blockExpr);
    if (blockExpr->Scope() == nullptr) {
        BindScopeNode(GetScope(), blockExpr);
    }
    Iterate(blockExpr);
}

void InitScopesPhaseETS::VisitClassStaticBlock(ir::ClassStaticBlock *staticBlock)
{
    const auto func = staticBlock->Function();

    {
        auto funcParamCtx = (func->Scope() == nullptr)
                                ? varbinder::LexicalScope<varbinder::FunctionParamScope>(VarBinder())
                                : varbinder::LexicalScope<varbinder::FunctionParamScope>::Enter(
                                      // CC-OFFNXT(G.FMT.06-CPP) project code style
                                      VarBinder(), func->Scope()->ParamScope());
        auto *funcParamScope = funcParamCtx.GetScope();
        auto funcCtx = LexicalScopeCreateOrEnter<varbinder::FunctionScope>(VarBinder(), func);
        auto *funcScope = funcCtx.GetScope();

        func->Body()->AsBlockStatement()->SetScope(funcScope);
        BindScopeNode(funcScope, func);
        funcParamScope->BindNode(func);
        BindFunctionScopes(funcScope, funcParamScope);
        Iterate(func->Body()->AsBlockStatement());
    }

    auto classCtx = varbinder::LexicalScope<varbinder::LocalScope>::Enter(
        VarBinder(), VarBinder()->GetScope()->AsClassScope()->StaticMethodScope());

    if (func->Id()->Variable() != nullptr) {
        return;
    }

    varbinder::Variable *var;
    if (var = VarBinder()->GetScope()->FindLocal(func->Id()->Name(), varbinder::ResolveBindingOptions::STATIC_METHODS);
        var == nullptr) {
        var = std::get<1>(VarBinder()->NewVarDecl<varbinder::FunctionDecl>(func->Id()->Start(), Allocator(),
                                                                           func->Id()->Name(), staticBlock));
        var->AddFlag(varbinder::VariableFlags::METHOD);
    }

    func->Id()->SetVariable(var);
}

void InitScopesPhaseETS::VisitImportNamespaceSpecifier(ir::ImportNamespaceSpecifier *importSpec)
{
    if (importSpec->Local()->Name().Empty()) {
        return;
    }
    AddOrGetDecl<varbinder::ImportDecl>(VarBinder(), importSpec->Local()->Name(), importSpec, importSpec->Start(),
                                        importSpec->Local()->Name(), importSpec->Local()->Name(), importSpec);
    auto var =
        VarBinder()->GetScope()->FindLocal(importSpec->Local()->Name(), varbinder::ResolveBindingOptions::BINDINGS);
    importSpec->Local()->SetVariable(var);
    Iterate(importSpec);
}

void InitScopesPhaseETS::VisitImportSpecifier(ir::ImportSpecifier *importSpec)
{
    Iterate(importSpec);
}

//  Auxiliary method to avoid extra nested levels and too large function size
void AddOverload(ir::MethodDefinition *overload, varbinder::Variable *variable) noexcept
{
    auto *currentNode = variable->Declaration()->Node();
    currentNode->AsMethodDefinition()->AddOverload(overload);
    ES2PANDA_ASSERT(overload->Id() != nullptr);
    overload->Id()->SetVariable(variable);
    overload->SetParent(currentNode);
}

void InitScopesPhaseETS::DeclareClassMethod(ir::MethodDefinition *method)
{
    ES2PANDA_ASSERT(VarBinder()->GetScope()->IsClassScope() && method->AsMethodDefinition()->Function() != nullptr);

    if ((method->AsMethodDefinition()->Function()->Flags() & ir::ScriptFunctionFlags::OVERLOAD) != 0) {
        return;
    }

    const auto methodName = method->Id();
    ES2PANDA_ASSERT(methodName != nullptr);
    auto *const clsScope = VarBinder()->GetScope()->AsClassScope();
    auto options =
        method->IsStatic()
            ? varbinder::ResolveBindingOptions::STATIC_VARIABLES | varbinder::ResolveBindingOptions::STATIC_DECLARATION
            : varbinder::ResolveBindingOptions::VARIABLES | varbinder::ResolveBindingOptions::DECLARATION;
    auto variable = clsScope->FindLocal(methodName->Name(), options);
    if (variable != nullptr) {
        VarBinder()->ThrowRedeclaration(methodName->Start(), methodName->Name(), variable->Declaration()->Type());
    }

    varbinder::LocalScope *targetScope {};
    if (method->IsStatic() || method->IsConstructor()) {
        targetScope = clsScope->StaticMethodScope();
    } else {
        targetScope = clsScope->InstanceMethodScope();
    }
    auto *found = targetScope->FindLocal(methodName->Name(), varbinder::ResolveBindingOptions::BINDINGS);

    MaybeAddOverload(method, methodName, found, clsScope, targetScope);
}

void InitScopesPhaseETS::MaybeAddOverload(ir::MethodDefinition *method, ir::Identifier *methodName,
                                          varbinder::Variable *found, varbinder::ClassScope *clsScope,
                                          varbinder::LocalScope *targetScope)
{
    if (found == nullptr) {
        auto classCtx = varbinder::LexicalScope<varbinder::LocalScope>::Enter(VarBinder(), targetScope);

        auto *var = methodName->Variable();
        if (var == nullptr) {
            var = std::get<1>(VarBinder()->NewVarDecl<varbinder::FunctionDecl>(methodName->Start(), Allocator(),
                                                                               methodName->Name(), method));
            var->SetScope(clsScope);
            if (targetScope->HasFlag(varbinder::ScopeFlags::STATIC)) {
                var->AddFlag(varbinder::VariableFlags::STATIC);
            }
            var->AddFlag(varbinder::VariableFlags::METHOD);
            methodName->SetVariable(var);
        }
        for (auto *overload : method->Overloads()) {
            ES2PANDA_ASSERT(overload->Id() != nullptr &&
                            ((overload->Function()->Flags() & ir::ScriptFunctionFlags::OVERLOAD) ||
                             (overload->Function()->Flags() & ir::ScriptFunctionFlags::EXTERNAL_OVERLOAD)));
            overload->Id()->SetVariable(var);
            overload->SetParent(var->Declaration()->Node());
        }
    } else {
        if (methodName->Name().Is(compiler::Signatures::MAIN) && clsScope->Parent()->IsGlobalScope()) {
            LogDiagnostic(diagnostic::OVERLOADED_MAIN, methodName->Start());
        }
        ES2PANDA_ASSERT(method->Function() != nullptr &&
                        (method->Function()->Flags() & ir::ScriptFunctionFlags::OVERLOAD) == 0U);

        AddOverload(method, found);
        method->Function()->AddFlag(ir::ScriptFunctionFlags::OVERLOAD);

        // default params overloads
        for (auto *overload : method->Overloads()) {
            ES2PANDA_ASSERT((overload->Function()->Flags() & ir::ScriptFunctionFlags::OVERLOAD) != 0U);
            AddOverload(overload, found);
        }
        method->ClearOverloads();
    }
}

void InitScopesPhaseETS::VisitOverloadDeclaration(ir::OverloadDeclaration *overload)
{
    auto *const clsScope = VarBinder()->GetScope()->AsClassScope();
    const auto overloadName = overload->Id();
    auto options =
        (overload->IsStatic() || overload->IsConstructorOverloadDeclaration())
            ? varbinder::ResolveBindingOptions::STATIC_VARIABLES | varbinder::ResolveBindingOptions::STATIC_DECLARATION
            : varbinder::ResolveBindingOptions::VARIABLES | varbinder::ResolveBindingOptions::DECLARATION;

    auto variable = clsScope->FindLocal(overloadName->Name(), options);
    if (variable != nullptr) {
        VarBinder()->ThrowRedeclaration(overloadName->Start(), overloadName->Name(), variable->Declaration()->Type());
    }

    Iterate(overload);
    DeclareClassOverload(overload);
}

varbinder::LocalScope *InitScopesPhaseETS::OverloadTargetScope(ir::OverloadDeclaration *overloaddecl,
                                                               varbinder::ClassScope *clsScope)
{
    varbinder::LocalScope *targetScope {};
    if (overloaddecl->IsConstructorOverloadDeclaration()) {
        targetScope = clsScope->StaticDeclScope();

        auto *found = clsScope->StaticMethodScope()->FindLocal(Signatures::CONSTRUCTOR_NAME,
                                                               varbinder::ResolveBindingOptions::BINDINGS);
        if (found == nullptr ||
            (!overloaddecl->OverloadedList().empty() && overloaddecl->OverloadedList().front()->IsIdentifier() &&
             overloaddecl->OverloadedList().front()->AsIdentifier()->Name().Is(Signatures::CONSTRUCTOR_NAME))) {
            return targetScope;
        }
        ir::Identifier *anonyConstructor = Allocator()->New<ir::Identifier>(Signatures::CONSTRUCTOR_NAME, Allocator());
        overloaddecl->PushFront(anonyConstructor);
        anonyConstructor->SetParent(overloaddecl);
    } else {
        targetScope = overloaddecl->IsStatic() ? clsScope->StaticDeclScope() : clsScope->InstanceDeclScope();
    }

    return targetScope;
}

void InitScopesPhaseETS::DeclareClassOverload(ir::OverloadDeclaration *overloaddecl)
{
    ES2PANDA_ASSERT(VarBinder()->GetScope()->IsClassScope());

    const auto overloadName = overloaddecl->Id();
    auto *const clsScope = VarBinder()->GetScope()->AsClassScope();

    varbinder::LocalScope *targetScope = OverloadTargetScope(overloaddecl, clsScope);
    varbinder::LocalScope *serachMethodScope =
        (overloaddecl->IsStatic() || overloaddecl->IsConstructorOverloadDeclaration())
            ? clsScope->StaticMethodScope()
            : clsScope->InstanceMethodScope();

    for (auto *methodName : overloaddecl->OverloadedList()) {
        if (!methodName->IsIdentifier()) {
            continue;
        }

        auto *found = serachMethodScope->FindLocal(methodName->AsIdentifier()->Name(),
                                                   varbinder::ResolveBindingOptions::BINDINGS);
        if (found == nullptr) {
            continue;
        }
        if (!found->Declaration()->Node()->IsMethodDefinition()) {
            VarBinder()->ThrowError(methodName->Start(), diagnostic::OVERLOADED_NAME_MUST_FUNCTION);
            continue;
        }

        methodName->SetVariable(found);
    }

    auto classCtx = varbinder::LexicalScope<varbinder::LocalScope>::Enter(VarBinder(), targetScope);
    auto var = std::get<1>(VarBinder()->NewVarDecl<varbinder::FunctionDecl>(overloadName->Start(), Allocator(),
                                                                            overloadName->Name(), overloaddecl));
    var->SetScope(clsScope);
    if (targetScope->HasFlag(varbinder::ScopeFlags::STATIC)) {
        var->AddFlag(varbinder::VariableFlags::STATIC);
    }
    var->AddFlag(varbinder::VariableFlags::OVERLOAD);
    overloadName->SetVariable(var);
}

void InitScopesPhaseETS::VisitETSReExportDeclaration(ir::ETSReExportDeclaration *reExport)
{
    if (!reExport->GetETSImportDeclarations()->IsValid()) {
        return;
    }
    if (reExport->GetETSImportDeclarations()->Language().IsDynamic()) {
        VarBinder()->AsETSBinder()->AddDynamicImport(reExport->GetETSImportDeclarations());
    }
    VarBinder()->AsETSBinder()->AddReExportImport(reExport);
    Iterate(reExport);
}

void InitScopesPhaseETS::VisitETSParameterExpression(ir::ETSParameterExpression *paramExpr)
{
    auto *const var = VarBinder()->AddParamDecl(paramExpr);
    paramExpr->SetVariable(var);
    var->SetScope(VarBinder()->GetScope());
    var->AddFlag(varbinder::VariableFlags::INITIALIZED);
    Iterate(paramExpr);
}

void InitScopesPhaseETS::VisitETSImportDeclaration(ir::ETSImportDeclaration *importDecl)
{
    if (!importDecl->IsValid()) {
        return;
    }
    ImportDeclarationContext importCtx(VarBinder());
    if (importDecl->Language().IsDynamic()) {
        VarBinder()->AsETSBinder()->AddDynamicImport(importDecl);
    }
    Iterate(importDecl);
}

void InitScopesPhaseETS::VisitTSEnumMember(ir::TSEnumMember *enumMember)
{
    auto ident = enumMember->Key()->AsIdentifier();
    if (ident->Variable() != nullptr) {
        return;
    }

    auto const &name = ident->Name();
    varbinder::Variable *var = nullptr;

    if (var = VarBinder()->GetScope()->FindLocal(name, varbinder::ResolveBindingOptions::STATIC_VARIABLES);
        var == nullptr) {
        varbinder::Decl *decl = nullptr;
        std::tie(decl, var) = VarBinder()->NewVarDecl<varbinder::ReadonlyDecl>(ident->Start(), name);
        var->SetScope(VarBinder()->GetScope());
        var->AddFlag(varbinder::VariableFlags::STATIC);
        decl->BindNode(enumMember);
    } else {
        VarBinder()->ThrowRedeclaration(ident->Start(), name, var->Declaration()->Type());
    }

    ident->SetVariable(var);
    Iterate(enumMember);
}

void InitScopesPhaseETS::VisitMethodDefinition(ir::MethodDefinition *method)
{
    ES2PANDA_ASSERT(method->Function() != nullptr);
    if (method->Function()->Scope() != nullptr) {
        return;
    }

    auto *curScope = VarBinder()->GetScope();
    const auto methodName = method->Id();
    ES2PANDA_ASSERT(methodName != nullptr);
    auto res =
        curScope->Find(methodName->Name(), method->IsStatic() ? varbinder::ResolveBindingOptions::ALL_STATIC
                                                              : varbinder::ResolveBindingOptions::ALL_NON_STATIC);
    if (res.variable != nullptr && !res.variable->Declaration()->IsFunctionDecl() && res.scope == curScope) {
        VarBinder()->ThrowRedeclaration(methodName->Start(), res.name, varbinder::DeclType::METHOD);
    }

    auto result = curScope->FindLocal(methodName->Name(), varbinder::ResolveBindingOptions::ALL_DECLARATION);
    if (result != nullptr) {
        VarBinder()->ThrowLocalRedeclaration(methodName->Start(), result->Name());
    }

    Iterate(method);
    DeclareClassMethod(method);
}

void InitScopesPhaseETS::VisitETSFunctionType(ir::ETSFunctionType *funcType)
{
    auto typeParamsCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), funcType->TypeParams());

    // Check for existing scope
    // In some cases we can visit function again with scope that already exists
    // Example: async lambda, we "move" original function to another place and visit it again
    if (funcType->Scope() == nullptr) {
        varbinder::LexicalScope<varbinder::FunctionParamScope> lexicalScope(VarBinder());
        auto *funcParamScope = lexicalScope.GetScope();
        BindScopeNode(funcParamScope, funcType);
        Iterate(funcType);
    }
}

void InitScopesPhaseETS::VisitETSNewClassInstanceExpression(ir::ETSNewClassInstanceExpression *newClassExpr)
{
    CallNode(newClassExpr->GetArguments());
    CallNode(newClassExpr->GetTypeRef());
}

void InitScopesPhaseETS::VisitTSTypeParameter(ir::TSTypeParameter *typeParam)
{
    if (typeParam->Name()->Variable() != nullptr) {
        return;
    }
    auto [decl, var] =
        VarBinder()->NewVarDecl<varbinder::TypeParameterDecl>(typeParam->Name()->Start(), typeParam->Name()->Name());
    typeParam->Name()->SetVariable(var);
    var->SetScope(VarBinder()->GetScope());
    var->AddFlag(varbinder::VariableFlags::TYPE_PARAMETER);
    decl->BindNode(typeParam);
    Iterate(typeParam);
}

void InitScopesPhaseETS::VisitTSInterfaceDeclaration(ir::TSInterfaceDeclaration *interfaceDecl)
{
    {
        auto typeParamsCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), interfaceDecl->TypeParams());
        CallNode(interfaceDecl->TypeParams());
        CallNode(interfaceDecl->Extends());
        auto localScope = LexicalScopeCreateOrEnter<varbinder::ClassScope>(VarBinder(), interfaceDecl);
        CallNode(interfaceDecl->Body());
        CallNodeAnnotations(interfaceDecl);
        BindScopeNode(localScope.GetScope(), interfaceDecl);
    }
    auto name = FormInterfaceOrEnumDeclarationIdBinding(interfaceDecl->Id());
    if (auto *decl = AddOrGetDecl<varbinder::InterfaceDecl>(VarBinder(), name, interfaceDecl, interfaceDecl->Start(),
                                                            Allocator(), name, interfaceDecl);
        decl != nullptr) {
        decl->AsInterfaceDecl()->Add(interfaceDecl);
    }
}

void InitScopesPhaseETS::VisitTSEnumDeclaration(ir::TSEnumDeclaration *enumDecl)
{
    {
        const auto enumCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), enumDecl);
        BindScopeNode(enumCtx.GetScope(), enumDecl);
        Iterate(enumDecl);
    }
    auto name = FormInterfaceOrEnumDeclarationIdBinding(enumDecl->Key());
    if (auto *decl = AddOrGetDecl<varbinder::EnumLiteralDecl>(VarBinder(), name, enumDecl, enumDecl->Key()->Start(),
                                                              name, enumDecl, enumDecl->IsConst());
        decl != nullptr) {
        decl->BindScope(enumDecl->Scope());
    }
}

void InitScopesPhaseETS::VisitTSTypeAliasDeclaration(ir::TSTypeAliasDeclaration *typeAlias)
{
    AddOrGetDecl<varbinder::TypeAliasDecl>(VarBinder(), typeAlias->Id()->Name(), typeAlias, typeAlias->Id()->Start(),
                                           typeAlias->Id()->Name(), typeAlias);
    auto typeParamsCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), typeAlias->TypeParams());
    Iterate(typeAlias);
}

void InitScopesPhaseETS::AddGlobalToBinder(parser::Program *program)
{
    auto globalId = program->GlobalClass()->Ident();
    if (globalId->Variable() != nullptr) {
        return;
    }

    auto start = globalId->Start();
    auto name = globalId->Name();
    auto *varBinder = program->VarBinder();
    auto *ctx = varBinder->GetContext();
    if (ctx->compilingState == public_lib::CompilingState::MULTI_COMPILING_FOLLOW) {
        auto *scope = varBinder->GetScope();
        auto bindings = scope->Bindings();
        auto res = bindings.find(name);
        if (res != bindings.end()) {
            auto classCtx =
                LexicalScopeCreateOrEnter<varbinder::ClassScope>(program->VarBinder(), program->GlobalClass());
            classCtx.GetScope()->BindNode(program->GlobalClass());
            program->GlobalClass()->SetScope(classCtx.GetScope());
            globalId->SetVariable(res->second);
            return;
        }
    }
    auto [decl2, var] = varBinder->NewVarDecl<varbinder::ClassDecl>(start, name);

    auto classCtx = LexicalScopeCreateOrEnter<varbinder::ClassScope>(program->VarBinder(), program->GlobalClass());
    classCtx.GetScope()->BindNode(program->GlobalClass());
    program->GlobalClass()->SetScope(classCtx.GetScope());

    auto *classDecl = program->GlobalClass()->Parent();
    decl2->BindNode(classDecl);
    globalId->SetVariable(var);
}

void InitScopesPhaseETS::HandleETSModule(ir::BlockStatement *script)
{
    for (auto decl : script->Statements()) {
        if (decl->IsETSImportDeclaration() || decl->IsETSReExportDeclaration()) {
            CallNode(decl);
        } else {
            auto classCtx =
                varbinder::LexicalScope<varbinder::ClassScope>::Enter(VarBinder(), Program()->GlobalClassScope());
            CallNode(decl);
        }
    }
    auto classCtx = varbinder::LexicalScope<varbinder::ClassScope>::Enter(VarBinder(), Program()->GlobalClassScope());

    for (auto decl : script->Statements()) {
        AddGlobalDeclaration(decl);
    }
}

void InitScopesPhaseETS::VisitClassDefinition(ir::ClassDefinition *classDef)
{
    if (classDef->IsGlobal()) {
        ParseGlobalClass(classDef);
        return;
    }
    auto typeParamsCtx = LexicalScopeCreateOrEnter<varbinder::LocalScope>(VarBinder(), classDef->TypeParams());
    CallNode(classDef->TypeParams());
    auto classCtx = LexicalScopeCreateOrEnter<varbinder::ClassScope>(VarBinder(), classDef);

    IterateNoTParams(classDef);

    // Will generate new node when compiling decl, so when compiling main file, will not generate new history in decl.
    FilterOverloads(classDef->BodyForUpdate());
    auto *classScope = classCtx.GetScope();
    BindScopeNode(classScope, classDef);
}

void InitScopesPhaseETS::VisitTSInterfaceBody(ir::TSInterfaceBody *interfBody)
{
    for (auto property : interfBody->Body()) {
        if (property->IsOverloadDeclaration()) {
            continue;
        }
        CallNode(property);
    }

    for (auto property : interfBody->Body()) {
        if (property->IsOverloadDeclaration()) {
            CallNode(property);
        }
    }
    FilterInterfaceOverloads(interfBody->Body());
}

void InitScopesPhaseETS::FilterInterfaceOverloads(ArenaVector<ir::AstNode *, false> &props)
{
    auto condition = [](ir::AstNode *prop) {
        if (prop->IsMethodDefinition()) {
            const auto func = prop->AsMethodDefinition()->Function();
            return func->IsOverload() && func->Body() != nullptr;
        }
        return false;
    };
    props.erase(std::remove_if(props.begin(), props.end(), condition), props.end());
}

void InitScopesPhaseETS::FilterOverloads(ArenaVector<ir::AstNode *, false> &props)
{
    auto condition = [](ir::AstNode *prop) {
        if (prop->IsMethodDefinition()) {
            const auto func = prop->AsMethodDefinition()->Function();
            return func->IsOverload();
        }
        return false;
    };
    props.erase(std::remove_if(props.begin(), props.end(), condition), props.end());
}

void InitScopesPhaseETS::VisitClassProperty(ir::ClassProperty *classProp)
{
    auto curScope = VarBinder()->GetScope();
    const auto name = classProp->Key()->AsIdentifier()->Name();

    ES2PANDA_ASSERT(!classProp->IsClassStaticBlock());

    if (classProp->IsConst()) {
        ES2PANDA_ASSERT(curScope->Parent() != nullptr);
        const auto initializer = classProp->Value();
        if (initializer == nullptr && curScope->Parent()->IsGlobalScope() && !classProp->IsDeclare() &&
            !classProp->NeedInitInStaticBlock()) {
            auto pos = classProp->End();
            // NOTE: Just use property Name?
            if (classProp->TypeAnnotation() != nullptr && !classProp->TypeAnnotation()->IsETSPrimitiveType()) {
                --(pos.index);
            }
            LogDiagnostic(diagnostic::CONST_WITHOUT_INIT, pos);
        }
        AddOrGetDecl<varbinder::ConstDecl>(VarBinder(), name, classProp, classProp->Key()->Start(), name, classProp);
    } else if (classProp->IsReadonly()) {
        AddOrGetDecl<varbinder::ReadonlyDecl>(VarBinder(), name, classProp, classProp->Key()->Start(), name, classProp);
    } else {
        AddOrGetDecl<varbinder::LetDecl>(VarBinder(), name, classProp, classProp->Key()->Start(), name, classProp);
    }
    Iterate(classProp);
}

void InitScopesPhaseETS::VisitBreakStatement(ir::BreakStatement *stmt)
{
    auto label = stmt->Ident();
    if (label != nullptr) {
        auto scope = VarBinder()->GetScope();
        auto var = scope->FindInFunctionScope(label->Name(), varbinder::ResolveBindingOptions::ALL).variable;
        label->SetVariable(var);
    }
}

void InitScopesPhaseETS::VisitContinueStatement(ir::ContinueStatement *stmt)
{
    auto label = stmt->Ident();
    if (label != nullptr) {
        auto scope = VarBinder()->GetScope();
        auto var = scope->FindInFunctionScope(label->Name(), varbinder::ResolveBindingOptions::ALL).variable;
        label->SetVariable(var);
    }
}

void InitScopesPhaseETS::AttachLabelToScope(ir::AstNode *node)
{
    if (node->Parent() == nullptr) {
        return;
    }

    if (!node->Parent()->IsLabelledStatement()) {
        return;
    }

    auto stmt = node->Parent()->AsLabelledStatement();
    auto label = stmt->Ident();
    if (label == nullptr) {
        return;
    }

    auto decl = AddOrGetDecl<varbinder::LabelDecl>(VarBinder(), label->Name(), stmt, label->Start(), label->Name());
    if (decl == nullptr) {
        return;
    }

    decl->BindNode(stmt);

    auto var = VarBinder()->GetScope()->FindLocal(label->Name(), varbinder::ResolveBindingOptions::BINDINGS);
    if (var != nullptr) {
        label->SetVariable(var);
        var->SetScope(VarBinder()->GetScope());
        var->AddFlag(varbinder::VariableFlags::LOCAL);
    }
}

void InitScopesPhaseETS::ParseGlobalClass(ir::ClassDefinition *global)
{
    bool defaultExported = false;
    for (auto decl : global->Body()) {
        if (decl->IsDefaultExported()) {
            if (defaultExported) {
                LogDiagnostic(diagnostic::MULTIPLE_DEFAULT_EXPORTS, decl->Start());
            }
            defaultExported = true;
        }
        CallNode(decl);
    }
    CallNodeAnnotations(global);
    FilterOverloads(global->BodyForUpdate());
}

void InitScopesPhaseETS::AddGlobalDeclaration(ir::AstNode *node)
{
    ir::Identifier *ident = nullptr;
    bool isBuiltin = false;
    switch (node->Type()) {
        case ir::AstNodeType::CLASS_DECLARATION: {
            auto def = node->AsClassDeclaration()->Definition();
            if (def->IsGlobal()) {
                return;
            }
            ident = def->Ident();
            isBuiltin = def->IsFromExternal();
            break;
        }
        case ir::AstNodeType::ANNOTATION_DECLARATION: {
            ident = node->AsAnnotationDeclaration()->GetBaseName();
            isBuiltin = false;
            break;
        }
        case ir::AstNodeType::STRUCT_DECLARATION: {
            ident = node->AsETSStructDeclaration()->Definition()->Ident();
            isBuiltin = node->AsETSStructDeclaration()->Definition()->IsFromExternal();
            break;
        }
        case ir::AstNodeType::TS_INTERFACE_DECLARATION: {
            ident = node->AsTSInterfaceDeclaration()->Id();
            isBuiltin = node->AsTSInterfaceDeclaration()->IsFromExternal();
            break;
        }
        case ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION: {
            ident = node->AsTSTypeAliasDeclaration()->Id();
            break;
        }
        default: {
            break;
        }
    }
    if (ident != nullptr && ident->Variable() != nullptr) {
        VarBinder()->TopScope()->InsertBinding(ident->Name(), ident->Variable());
        if (isBuiltin) {
            ident->Variable()->AddFlag(varbinder::VariableFlags::BUILTIN_TYPE);
        }
    }
}

void InitScopesPhaseAS::VisitArrowFunctionExpression(ir::ArrowFunctionExpression *arrowExpr)
{
    Iterate(arrowExpr);
}

void InitScopesPhaseAS::VisitExportNamedDeclaration(ir::ExportNamedDeclaration *exportDecl)
{
    ExportDeclarationContext exportDeclCtx(VarBinder());
    Iterate(exportDecl);
}

}  // namespace ark::es2panda::compiler
