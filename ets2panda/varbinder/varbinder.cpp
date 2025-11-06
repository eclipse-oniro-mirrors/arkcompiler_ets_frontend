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

#include "varbinder.h"
#include "public/public.h"
#include "ir/ets/etsDestructuring.h"

namespace ark::es2panda::varbinder {
void VarBinder::InitTopScope()
{
    if (program_->Kind() == parser::ScriptKind::MODULE) {
        topScope_ = Allocator()->New<ModuleScope>(Allocator());
    } else {
        topScope_ = Allocator()->New<GlobalScope>(Allocator());
    }
    scope_ = topScope_;
    varScope_ = topScope_;
}

Variable *VarBinder::AddParamDecl(ir::Expression *param)
{
    ES2PANDA_ASSERT(scope_->IsFunctionParamScope() || scope_->IsCatchParamScope());

    auto [var, node] = static_cast<ParamScope *>(scope_)->AddParamDecl(Allocator(), this, param);
    ES2PANDA_ASSERT(var != nullptr);

    if (node != nullptr) {
        ThrowRedeclaration(node->Start(), var->Name(), var->Declaration()->Type());
    }

    return var;
}

void VarBinder::ThrowRedeclaration(const lexer::SourcePosition &pos, const util::StringView &name,
                                   DeclType declType) const
{
    ThrowError(pos, diagnostic::VARIABLE_REDECLARED, {name});

    switch (declType) {
        case DeclType::CLASS:
        case DeclType::INTERFACE:
        case DeclType::ENUM:
            ThrowError(pos, diagnostic::MERGED_DECLS);
            break;
        default:
            break;
    }
}

void VarBinder::ThrowLocalRedeclaration(const lexer::SourcePosition &pos, const util::StringView &className) const
{
    ThrowError(pos, diagnostic::ID_REDECLARED, {className});
}

void VarBinder::ThrowUnresolvableType(const lexer::SourcePosition &pos, const util::StringView &name) const
{
    ThrowError(pos, diagnostic::TYPE_NOT_FOUND, {name});
}

void VarBinder::ThrowTDZ(const lexer::SourcePosition &pos, const util::StringView &name) const
{
    ThrowError(pos, diagnostic::TEMPORAL_DEADZONE, {name});
}

void VarBinder::ThrowInvalidCapture(const lexer::SourcePosition &pos, const util::StringView &name) const
{
    ThrowError(pos, diagnostic::INVALID_CAPTURE, {name});
}

void VarBinder::ThrowPrivateFieldMismatch(const lexer::SourcePosition &pos, const util::StringView &name) const
{
    ThrowError(pos, diagnostic::PRIVATE_FIELD_MISMATCH, {name});
}

void VarBinder::ThrowError(const lexer::SourcePosition &pos, const diagnostic::DiagnosticKind &kind,
                           const util::DiagnosticMessageParams &params) const
{
    context_->diagnosticEngine->ThrowSyntaxError(kind, params, pos);
}

bool VarBinder::IsGlobalIdentifier(const util::StringView &str) const
{
    return util::Helpers::IsGlobalIdentifier(str);
}

void VarBinder::IdentifierAnalysis()
{
    ES2PANDA_ASSERT(program_->Ast());
    ES2PANDA_ASSERT(scope_ == topScope_);
    ES2PANDA_ASSERT(varScope_ == topScope_);

    functionScopes_.push_back(topScope_);
    topScope_->BindName(MAIN);
    topScope_->BindInternalName(BuildFunctionName(MAIN, 0));

    topScope_->CheckDirectEval(context_);

    ResolveReferences(program_->Ast());
    AddMandatoryParams();
}

void VarBinder::LookupReference(const util::StringView &name)
{
    auto res = scope_->Find(name);
    if (res.level == 0) {
        return;
    }

    ES2PANDA_ASSERT(res.variable);
    res.variable->SetLexical(res.scope);
}

bool VarBinder::InstantiateArgumentsImpl(Scope **scope, Scope *iter, const ir::AstNode *node)
{
    if (node->AsScriptFunction()->IsArrow()) {
        return false;
    }

    auto [argumentsVariable, exists] =
        (*scope)->AddDecl<ConstDecl, LocalVariable>(Allocator(), FUNCTION_ARGUMENTS, VariableFlags::INITIALIZED);
    if (exists && Extension() != ScriptExtension::JS) {
        ThrowRedeclaration(node->Start(), FUNCTION_ARGUMENTS, argumentsVariable->Declaration()->Type());
    } else if (iter->IsFunctionParamScope()) {
        *scope = iter->AsFunctionParamScope()->GetFunctionScope();
        (*scope)->InsertBinding(argumentsVariable->Name(), argumentsVariable);
    }

    (*scope)->AddFlag(ScopeFlags::USE_ARGS);
    return true;
}

void VarBinder::InstantiateArguments()
{
    auto *iter = scope_;
    while (true) {
        Scope *scope = iter->IsFunctionParamScope() ? iter : iter->EnclosingVariableScope();
        ES2PANDA_ASSERT(scope != nullptr);
        const auto *node = scope->Node();

        if (scope->IsLoopScope()) {
            iter = scope->Parent();
            continue;
        }

        if (!node->IsScriptFunction()) {
            break;
        }

        if (InstantiateArgumentsImpl(&scope, iter, node)) {
            break;
        }

        iter = scope->Parent();
    }
}

void VarBinder::PropagateDirectEval() const
{
    auto *iter = scope_;

    do {
        VariableScope *scope = iter->IsFunctionParamScope() ? iter->AsFunctionParamScope()->GetFunctionScope()
                                                            : iter->EnclosingVariableScope();
        ES2PANDA_ASSERT(scope != nullptr);
        scope->AddFlag(ScopeFlags::NO_REG_STORE);
        iter = iter->Parent();
    } while (iter != nullptr);
}

void VarBinder::InstantiatePrivateContext(const ir::Identifier *ident) const
{
    auto *classDef = util::Helpers::GetContainingClassDefinition(ident);

    while (classDef != nullptr) {
        auto *scope = classDef->Scope();
        Variable *variable = scope->FindLocal(classDef->InternalName(), varbinder::ResolveBindingOptions::BINDINGS);
        ES2PANDA_ASSERT(variable != nullptr);
        if (!variable->HasFlag(VariableFlags::INITIALIZED)) {
            break;
        }

        if (classDef->HasMatchingPrivateKey(ident->Name())) {
            variable->SetLexical(scope);
            return;
        }

        classDef = util::Helpers::GetContainingClassDefinition(classDef->Parent());
    }

    ThrowPrivateFieldMismatch(ident->Start(), ident->Name());
}

void VarBinder::LookupIdentReference(ir::Identifier *ident)
{
    if (!ident->IsReference(Extension())) {
        return;
    }

    if (ident->Name().Is(FUNCTION_ARGUMENTS)) {
        InstantiateArguments();
    }

    if (ident->IsPrivateIdent()) {
        InstantiatePrivateContext(ident);
        return;
    }

    auto res = scope_->Find(ident->Name(), BindingOptions());
    if (res.level != 0) {
        ES2PANDA_ASSERT(res.variable);
        res.variable->SetLexical(res.scope);
    }

    if (res.variable == nullptr) {
        return;
    }

    if (res.variable->Declaration()->IsLetOrConstDecl() && !res.variable->HasFlag(VariableFlags::INITIALIZED)) {
        ident->SetTdz();
    }

    ident->SetVariable(res.variable);
}

util::StringView VarBinder::BuildFunctionName(util::StringView name, uint32_t idx)
{
    std::stringstream ss;
    ss << "func_" << name << "_" << std::to_string(idx);
    util::UString internalName(ss.str(), Allocator());

    return internalName.View();
}

bool VarBinder::BuildInternalName(ir::ScriptFunction *scriptFunc)
{
    auto *funcScope = scriptFunc->Scope();
    auto name = util::Helpers::FunctionName(Allocator(), scriptFunc);

    uint32_t idx = functionScopes_.size();
    funcScope->BindName(name);
    funcScope->BindInternalName(BuildFunctionName(name, idx));

    return !scriptFunc->IsOverload();
}

void VarBinder::BuildVarDeclaratorIdFromIdentifier(ir::Identifier *ident)
{
    const auto &name = ident->Name();

    if (IsGlobalIdentifier(name) || name.Is(ERROR_LITERAL)) {
        return;
    }

    auto *variable = scope_->FindLocal(name, varbinder::ResolveBindingOptions::BINDINGS);
    ES2PANDA_ASSERT(variable);
    ident->SetVariable(variable);
    BuildSignatureDeclarationBaseParams(ident->TypeAnnotation());
    variable->AddFlag(VariableFlags::INITIALIZED);
}

void VarBinder::BuildVarDeclaratorId(ir::AstNode *childNode)
{
    switch (childNode->Type()) {
        case ir::AstNodeType::IDENTIFIER: {
            BuildVarDeclaratorIdFromIdentifier(childNode->AsIdentifier());
            break;
        }
        case ir::AstNodeType::OBJECT_PATTERN: {
            auto *objPattern = childNode->AsObjectPattern();

            for (auto *prop : objPattern->Properties()) {
                BuildVarDeclaratorId(prop);
            }

            BuildSignatureDeclarationBaseParams(objPattern->TypeAnnotation());
            break;
        }
        case ir::AstNodeType::ARRAY_PATTERN: {
            auto *arrayPattern = childNode->AsArrayPattern();

            for (auto *element : childNode->AsArrayPattern()->Elements()) {
                BuildVarDeclaratorId(element);
            }

            BuildSignatureDeclarationBaseParams(arrayPattern->TypeAnnotation());
            break;
        }
        case ir::AstNodeType::ETS_DESTRUCTURING: {
            auto dstr = childNode->AsETSDestructuring();
            for (auto *element : dstr->Elements()) {
                BuildVarDeclaratorId(element);
            }
            break;
        }
        case ir::AstNodeType::ASSIGNMENT_PATTERN: {
            ResolveReference(childNode->AsAssignmentPattern()->Right());
            BuildVarDeclaratorId(childNode->AsAssignmentPattern()->Left());
            break;
        }
        case ir::AstNodeType::PROPERTY: {
            ResolveReference(childNode->AsProperty()->Key());
            BuildVarDeclaratorId(childNode->AsProperty()->Value());
            break;
        }
        case ir::AstNodeType::REST_ELEMENT: {
            BuildVarDeclaratorId(childNode->AsRestElement()->Argument());
            break;
        }
        default:
            break;
    }
}

void VarBinder::BuildVarDeclarator(ir::VariableDeclarator *varDecl)
{
    if (varDecl->Parent()->AsVariableDeclaration()->Kind() == ir::VariableDeclaration::VariableDeclarationKind::VAR) {
        ResolveReferences(varDecl);
        return;
    }

    if (varDecl->Init() != nullptr) {
        ResolveReference(varDecl->Init());
    }

    BuildVarDeclaratorId(varDecl->Id());
}

void VarBinder::BuildClassProperty(const ir::ClassProperty *prop)
{
    const ir::ScriptFunction *ctor = util::Helpers::GetContainingConstructor(prop);
    ES2PANDA_ASSERT(ctor != nullptr);
    auto scopeCtx = LexicalScope<FunctionScope>::Enter(this, ctor->Scope());

    ResolveReferences(prop);
}

void VarBinder::InitializeClassBinding(ir::ClassDefinition *classDef)
{
    auto res = scope_->Find(classDef->Ident()->Name());

    ES2PANDA_ASSERT(res.variable && res.variable->Declaration()->IsLetDecl());
    res.variable->AddFlag(VariableFlags::INITIALIZED);
}

void VarBinder::InitializeClassIdent(ir::ClassDefinition *classDef)
{
    auto res = scope_->Find(classDef->Ident()->Name());

    ES2PANDA_ASSERT(res.variable &&
                    (res.variable->Declaration()->IsConstDecl() || res.variable->Declaration()->IsReadonlyDecl()));
    res.variable->AddFlag(VariableFlags::INITIALIZED);
}

void VarBinder::BuildClassDefinition(ir::ClassDefinition *classDef)
{
    if (classDef->Parent()->IsClassDeclaration() || classDef->Parent()->IsETSStructDeclaration()) {
        InitializeClassBinding(classDef);
    }

    auto scopeCtx = LexicalScope<LocalScope>::Enter(this, classDef->Scope());

    if (classDef->Super() != nullptr) {
        ResolveReference(classDef->Super());
    }

    Variable *variable = scope_->FindLocal(classDef->InternalName(), varbinder::ResolveBindingOptions::BINDINGS);
    ES2PANDA_ASSERT(variable != nullptr);
    variable->AddFlag(VariableFlags::INITIALIZED);

    if (classDef->Ident() != nullptr) {
        InitializeClassIdent(classDef);
    }

    ResolveReference(classDef->Ctor());

    for (auto *stmt : classDef->Body()) {
        ResolveReference(stmt);
    }
}

void VarBinder::BuildForUpdateLoop(ir::ForUpdateStatement *forUpdateStmt)
{
    auto *loopScope = forUpdateStmt->Scope();

    auto declScopeCtx = LexicalScope<LoopDeclarationScope>::Enter(this, loopScope->DeclScope());

    if (forUpdateStmt->Init() != nullptr) {
        ResolveReference(forUpdateStmt->Init());
    }

    if (forUpdateStmt->Update() != nullptr) {
        ResolveReference(forUpdateStmt->Update());
    }

    auto loopCtx = LexicalScope<LoopScope>::Enter(this, loopScope);

    if (forUpdateStmt->Test() != nullptr) {
        ResolveReference(forUpdateStmt->Test());
    }

    ResolveReference(forUpdateStmt->Body());

    loopCtx.GetScope()->ConvertToVariableScope(Allocator());
}

void VarBinder::BuildForInOfLoop(varbinder::LoopScope *loopScope, ir::AstNode *left, ir::Expression *right,
                                 ir::Statement *body)
{
    auto declScopeCtx = LexicalScope<LoopDeclarationScope>::Enter(this, loopScope->DeclScope());

    ResolveReference(right);
    ResolveReference(left);

    auto loopCtx = LexicalScope<LoopScope>::Enter(this, loopScope);

    ResolveReference(body);
    loopCtx.GetScope()->ConvertToVariableScope(Allocator());
}

void VarBinder::BuildCatchClause(ir::CatchClause *catchClauseStmt)
{
    if (catchClauseStmt->Param() != nullptr) {
        auto paramScopeCtx = LexicalScope<CatchParamScope>::Enter(this, catchClauseStmt->Scope()->ParamScope());
        ResolveReference(catchClauseStmt->Param());
    }

    auto scopeCtx = LexicalScope<CatchScope>::Enter(this, catchClauseStmt->Scope());
    ResolveReference(catchClauseStmt->Body());
}

void VarBinder::BuildTypeAliasDeclaration(ir::TSTypeAliasDeclaration *const typeAliasDecl)
{
    if (typeAliasDecl->TypeParams() != nullptr) {
        const auto typeAliasScope = LexicalScope<LocalScope>::Enter(this, typeAliasDecl->TypeParams()->Scope());
        ResolveReferences(typeAliasDecl);
        return;
    }

    ResolveReferences(typeAliasDecl);
}

void VarBinder::AddCompilableFunction(ir::ScriptFunction *func)
{
    if (func->IsArrow()) {
        VariableScope *outerVarScope = scope_->EnclosingVariableScope();
        ES2PANDA_ASSERT(outerVarScope != nullptr);
        outerVarScope->AddFlag(ScopeFlags::INNER_ARROW);
    }

    AddCompilableFunctionScope(func->Scope());
}

void VarBinder::AddCompilableFunctionScope(varbinder::FunctionScope *funcScope)
{
    functionScopes_.push_back(funcScope);
}

void VarBinder::VisitScriptFunction(ir::ScriptFunction *func)
{
    auto *funcScope = func->Scope();
    {
        ES2PANDA_ASSERT(funcScope != nullptr);
        auto paramScopeCtx = LexicalScope<FunctionParamScope>::Enter(this, funcScope->ParamScope());

        for (auto *param : func->Params()) {
            ResolveReference(param);
        }
    }

    if (func->ReturnTypeAnnotation() != nullptr) {
        ResolveReference(func->ReturnTypeAnnotation());
    }

    if (!BuildInternalName(func)) {
        if (func->Body() == nullptr) {
            return;
        }
        auto stmt = func->Body()->AsBlockStatement()->Statements();
        auto scopeCtx = LexicalScope<FunctionScope>::Enter(this, funcScope);
        std::function<void(ir::AstNode *)> doNode = [&](ir::AstNode *node) {
            if (node->IsTSInterfaceDeclaration() || node->IsTSEnumDeclaration()) {
                ResolveReference(node);
            }
            node->Iterate([&](ir::AstNode *child) { doNode(child); });
        };
        doNode(func->Body());
        return;
    }

    if (!func->IsDeclare()) {
        AddCompilableFunction(func);
    }

    auto scopeCtx = LexicalScope<FunctionScope>::Enter(this, funcScope);

    if (func->Body() != nullptr) {
        ResolveReference(func->Body());
    }
}

void VarBinder::VisitScriptFunctionWithPotentialTypeParams(ir::ScriptFunction *func)
{
    if (func->TypeParams() != nullptr) {
        auto typeParamScopeCtx = LexicalScope<Scope>::Enter(this, func->TypeParams()->Scope());
        VisitScriptFunction(func);
        return;
    }

    VisitScriptFunction(func);
}

void VarBinder::ResolveReferenceDoWhileHelper(ir::AstNode *childNode)
{
    auto *doWhileStatement = childNode->AsDoWhileStatement();

    {
        auto loopScopeCtx = LexicalScope<LoopScope>::Enter(this, doWhileStatement->Scope());
        ResolveReference(doWhileStatement->Body());
    }
    return ResolveReference(doWhileStatement->Test());
}

void VarBinder::ResolveReferenceWhileHelper(ir::AstNode *childNode)
{
    auto *whileStatement = childNode->AsWhileStatement();
    ResolveReference(whileStatement->Test());

    auto loopScopeCtx = LexicalScope<LoopScope>::Enter(this, whileStatement->Scope());
    return ResolveReference(whileStatement->Body());
}

// CC-OFFNXT(huge_method,huge_cyclomatic_complexity,G.FUN.01-CPP) big switch-case, solid logic
void VarBinder::ResolveReference(ir::AstNode *childNode)
{
    switch (childNode->Type()) {
        case ir::AstNodeType::IDENTIFIER:
            LookupIdentReference(childNode->AsIdentifier());
            return ResolveReferences(childNode);
        case ir::AstNodeType::SUPER_EXPRESSION:
            ES2PANDA_ASSERT(scope_->EnclosingVariableScope() != nullptr);
            scope_->EnclosingVariableScope()->AddFlag(ScopeFlags::USE_SUPER);
            return ResolveReferences(childNode);
        case ir::AstNodeType::SCRIPT_FUNCTION: {
            return VisitScriptFunctionWithPotentialTypeParams(childNode->AsScriptFunction());
        }
        case ir::AstNodeType::VARIABLE_DECLARATOR:
            return BuildVarDeclarator(childNode->AsVariableDeclarator());
        case ir::AstNodeType::CLASS_DEFINITION:
            return BuildClassDefinition(childNode->AsClassDefinition());
        case ir::AstNodeType::CLASS_PROPERTY:
            return BuildClassProperty(childNode->AsClassProperty());
        case ir::AstNodeType::BLOCK_STATEMENT: {
            auto scopeCtx = LexicalScope<Scope>::Enter(this, childNode->AsBlockStatement()->Scope());
            return ResolveReferences(childNode);
        }
        case ir::AstNodeType::BLOCK_EXPRESSION: {
            auto scopeCtx = LexicalScope<Scope>::Enter(this, childNode->AsBlockExpression()->Scope());
            return ResolveReferences(childNode);
        }
        case ir::AstNodeType::SWITCH_STATEMENT: {
            auto scopeCtx = LexicalScope<LocalScope>::Enter(this, childNode->AsSwitchStatement()->Scope());
            return ResolveReferences(childNode);
        }
        case ir::AstNodeType::DO_WHILE_STATEMENT:
            return ResolveReferenceDoWhileHelper(childNode);
        case ir::AstNodeType::WHILE_STATEMENT:
            return ResolveReferenceWhileHelper(childNode);
        case ir::AstNodeType::FOR_UPDATE_STATEMENT:
            return BuildForUpdateLoop(childNode->AsForUpdateStatement());
        case ir::AstNodeType::FOR_IN_STATEMENT: {
            auto *forInStmt = childNode->AsForInStatement();
            return BuildForInOfLoop(forInStmt->Scope(), forInStmt->Left(), forInStmt->Right(), forInStmt->Body());
        }
        case ir::AstNodeType::FOR_OF_STATEMENT: {
            auto *forOfStmt = childNode->AsForOfStatement();
            return BuildForInOfLoop(forOfStmt->Scope(), forOfStmt->Left(), forOfStmt->Right(), forOfStmt->Body());
        }
        case ir::AstNodeType::CATCH_CLAUSE:
            return BuildCatchClause(childNode->AsCatchClause());
        case ir::AstNodeType::TS_TYPE_ALIAS_DECLARATION:
            return BuildTypeAliasDeclaration(childNode->AsTSTypeAliasDeclaration());
        default:
            return HandleCustomNodes(childNode);
    }
}

void VarBinder::ResolveReferences(const ir::AstNode *parent)
{
    parent->Iterate([this](auto *childNode) { ResolveReference(childNode); });
}

LocalVariable *VarBinder::AddMandatoryParam(const std::string_view &name)
{
    ES2PANDA_ASSERT(scope_->IsFunctionParamScope());

    auto *decl = Allocator()->New<ParameterDecl>(name);
    auto *param = Allocator()->New<LocalVariable>(decl, VariableFlags::VAR);

    auto &funcParams = scope_->AsFunctionParamScope()->Params();

    funcParams.insert(funcParams.begin(), param);
    scope_->AsFunctionParamScope()->GetFunctionScope()->InsertBinding(decl->Name(), param);
    scope_->InsertBinding(decl->Name(), param);

    return param;
}

void VarBinder::LookUpMandatoryReferences(const FunctionScope *funcScope, bool needLexicalFuncObj)
{
    LookupReference(MANDATORY_PARAM_NEW_TARGET);
    LookupReference(MANDATORY_PARAM_THIS);

    if (funcScope->HasFlag(ScopeFlags::USE_ARGS)) {
        LookupReference(FUNCTION_ARGUMENTS);
    }

    if (needLexicalFuncObj) {
        LookupReference(MANDATORY_PARAM_FUNC);
    }
}

void VarBinder::AddMandatoryParams()
{
    ES2PANDA_ASSERT(scope_ == topScope_);
    ES2PANDA_ASSERT(!functionScopes_.empty());
    auto iter = functionScopes_.begin();
    [[maybe_unused]] auto *funcScope = *iter++;

    ES2PANDA_ASSERT(funcScope->IsGlobalScope() || funcScope->IsModuleScope());

    const auto *options = context_->config->options;
    if (options->IsDirectEval()) {
        AddMandatoryParams(EVAL_SCRIPT_MANDATORY_PARAMS);
        topScope_->ParamScope()->Params().back()->SetLexical(topScope_);
    } else {
        AddMandatoryParams(FUNCTION_MANDATORY_PARAMS);
    }

    if (options->IsFunctionEval()) {
        ES2PANDA_ASSERT(iter != functionScopes_.end());
        funcScope = *iter++;
        auto scopeCtx = LexicalScope<FunctionScope>::Enter(this, funcScope);
        AddMandatoryParams(ARROW_MANDATORY_PARAMS);
        LookUpMandatoryReferences(funcScope, false);
    }

    for (; iter != functionScopes_.end(); iter++) {
        funcScope = *iter;
        const auto *scriptFunc = funcScope->Node()->AsScriptFunction();

        auto scopeCtx = LexicalScope<FunctionScope>::Enter(this, funcScope);

        if (!scriptFunc->IsArrow()) {
            AddMandatoryParams(FUNCTION_MANDATORY_PARAMS);
            continue;
        }

        const ir::ScriptFunction *ctor = util::Helpers::GetContainingConstructor(scriptFunc);
        bool lexicalFunctionObject {};

        if (ctor != nullptr && util::Helpers::GetClassDefinition(ctor)->Super() != nullptr &&
            funcScope->HasFlag(ScopeFlags::USE_SUPER)) {
            ES2PANDA_ASSERT(ctor->Scope()->HasFlag(ScopeFlags::INNER_ARROW));
            ctor->Scope()->AddFlag(ScopeFlags::SET_LEXICAL_FUNCTION);
            lexicalFunctionObject = true;
            AddMandatoryParams(CTOR_ARROW_MANDATORY_PARAMS);
        } else {
            AddMandatoryParams(ARROW_MANDATORY_PARAMS);
        }

        LookUpMandatoryReferences(funcScope, lexicalFunctionObject);
    }
}
}  // namespace ark::es2panda::varbinder
