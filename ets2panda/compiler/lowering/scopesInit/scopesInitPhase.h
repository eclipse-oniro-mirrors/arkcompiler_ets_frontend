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

#ifndef ES2PANDA_COMPILER_CORE_SCOPES_INIT_PHASE_H
#define ES2PANDA_COMPILER_CORE_SCOPES_INIT_PHASE_H

#include "compiler/lowering/phase.h"
#include "util/helpers.h"
#include "varbinder/ETSBinder.h"
#include "checker/checker.h"
#include "ir/visitor/IterateAstVisitor.h"
#include "ir/expressions/blockExpression.h"

namespace ark::es2panda::compiler {

/**
 * Responsible for initialization of scopes. Should be called right after Parser stage.
 */
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ScopesInitPhase : public Phase, public ir::visitor::IterateAstVisitor {
public:
    using PhaseContext = public_lib::Context;

    static constexpr std::string_view NAME = "ScopesInitPhase";

    std::string_view Name() const override
    {
        return NAME;
    }

    bool Perform(PhaseContext *ctx, parser::Program *program) override;

protected:
    void SetProgram(parser::Program *program) noexcept;

    void Prepare(PhaseContext *ctx, parser::Program *program);

    /**
     * Should be called at the end of each program perform
     */
    void Finalize();

    /**
     * Check if there's only one default export and no named export redeclaration,
     * throw error if so.
     * Side effect: fill local_exports_
     */
    void AnalyzeExports();

protected:
    template <typename T>
    void CallNode(T *node)
    {
        if (node) {
            node->Accept(this);
        }
    }

    template <typename T>
    void CallNode(const ArenaVector<T *> &nodes)
    {
        for (auto *node : nodes) {
            CallNode(node);
        }
    }

    template <typename T>
    void CallNodeAnnotations(const ir::AnnotationAllowed<T> *node)
    {
        if (node->HasAnnotations()) {
            CallNode(node->Annotations());
        }
    }

    void CallFuncParams(const ArenaVector<ir::Expression *> &params);
    void IterateNoTParams(ir::ClassDefinition *classDef);

protected:
    void LogDiagnostic(const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &params,
                       const lexer::SourcePosition &pos) const;

    void LogDiagnostic(const diagnostic::DiagnosticKind &kind, const lexer::SourcePosition &pos) const
    {
        LogDiagnostic(kind, util::DiagnosticMessageParams {}, pos);
    }

    void VisitFunctionExpression(ir::FunctionExpression *funcExpr) override;
    void VisitScriptFunction(ir::ScriptFunction *scriptFunction) override;
    void VisitBlockStatement(ir::BlockStatement *blockStmt) override;
    void VisitImportDeclaration(ir::ImportDeclaration *importDeclaration) override;
    void VisitClassStaticBlock(ir::ClassStaticBlock *staticBlock) override;
    void VisitClassDefinition(ir::ClassDefinition *classDef) override;
    void VisitMethodDefinition(ir::MethodDefinition *methodDefinition) override;
    void VisitForUpdateStatement(ir::ForUpdateStatement *forUpdateStmt) override;
    void VisitForInStatement(ir::ForInStatement *forInStmt) override;
    void VisitForOfStatement(ir::ForOfStatement *forOfStmt) override;
    void VisitCatchClause(ir::CatchClause *catchClause) override;
    void VisitVariableDeclarator(ir::VariableDeclarator *varDecl) override;
    void VisitSwitchStatement(ir::SwitchStatement *switchStmt) override;
    void VisitWhileStatement(ir::WhileStatement *whileStmt) override;
    void VisitETSStructDeclaration(ir::ETSStructDeclaration *structDecl) override;
    void VisitClassDeclaration(ir::ClassDeclaration *classDecl) override;
    void VisitDoWhileStatement(ir::DoWhileStatement *doWhileStmt) override;
    void VisitFunctionDeclaration(ir::FunctionDeclaration *funcDecl) override;
    void VisitExportAllDeclaration(ir::ExportAllDeclaration *exportAllDecl) override;
    void VisitImportNamespaceSpecifier(ir::ImportNamespaceSpecifier *importSpec) override;
    void VisitImportSpecifier(ir::ImportSpecifier *importSpec) override;
    void VisitImportDefaultSpecifier(ir::ImportDefaultSpecifier *importSpec) override;
    void VisitExportDefaultDeclaration(ir::ExportDefaultDeclaration *exportDecl) override;
    void VisitExportNamedDeclaration(ir::ExportNamedDeclaration *exportDecl) override;
    void VisitArrowFunctionExpression(ir::ArrowFunctionExpression *arrowExpr) override;
    void VisitDirectEvalExpression(ir::DirectEvalExpression *directCallExpr) override;
    void VisitTSFunctionType(ir::TSFunctionType *funcType) override;
    void VisitAnnotationDeclaration(ir::AnnotationDeclaration *annoDecl) override;
    void VisitAnnotationUsage(ir::AnnotationUsage *annoUsage) override;

protected:
    varbinder::Scope *GetScope()
    {
        return VarBinder()->GetScope();
    }

    ArenaAllocator *Allocator()
    {
        return program_->Allocator();
    }

    parser::Program *Program()
    {
        return program_;
    }

    const parser::Program *Program() const
    {
        return program_;
    }

    PhaseContext *Context()
    {
        return ctx_;
    }

    [[nodiscard]] varbinder::VarBinder *VarBinder() const
    {
        return program_->VarBinder();
    }

protected:
    virtual void CreateFuncDecl(ir::ScriptFunction *func);
    virtual util::StringView FormInterfaceOrEnumDeclarationIdBinding(ir::Identifier *id);
    void HandleFunction(ir::ScriptFunction *function);
    varbinder::FunctionParamScope *HandleFunctionSig(ir::TSTypeParameterDeclaration *typeParams,
                                                     const ArenaVector<ir::Expression *> &params,
                                                     ir::TypeNode *returnType);

    /**
     * Handle block from existing scope
     */
    void HandleBlockStmt(ir::BlockStatement *block, varbinder::Scope *scope);

    template <typename ForT>
    void HandleFor(varbinder::LoopDeclarationScope *declScope, varbinder::LoopScope *loopScope, ForT *forStmt)
    {
        loopScope->BindDecls(declScope);
        BindScopeNode(loopScope, forStmt);
        loopScope->DeclScope()->BindNode(forStmt);
    }

protected:
    virtual varbinder::Decl *BindClassName(ir::ClassDefinition *classDef);

    template <class Scope, class Node>
    static void BindScopeNode(Scope *scope, Node *node)
    {
        if (node->Scope() == nullptr || node->IsBlockStatement()) {
            scope->BindNode(node);
            node->SetScope(scope);
        }
    }

    static void BindFunctionScopes(varbinder::FunctionScope *scope, varbinder::FunctionParamScope *paramScope);

    void BindClassDefinition(ir::ClassDefinition *classDef);

    std::tuple<varbinder::Decl *, varbinder::Variable *> AddOrGetVarDecl(ir::VariableDeclaratorFlag flag,
                                                                         const ir::Identifier *id);

    virtual void BindVarDecl([[maybe_unused]] ir::Identifier *binding, ir::Expression *init, varbinder::Decl *decl,
                             [[maybe_unused]] varbinder::Variable *var);
    virtual void AttachLabelToScope(ir::AstNode *node);

private:
    PhaseContext *ctx_ {};
    parser::Program *program_ {};
};

/**
 * Specialization for typed script languages (typescript, ets)
 */
class ScopeInitTyped : public ScopesInitPhase {
protected:
public:
    void VisitTSModuleDeclaration(ir::TSModuleDeclaration *moduleDecl) override;

    void VisitTSModuleBlock(ir::TSModuleBlock *block) override;

    void VisitTSTypeAliasDeclaration(ir::TSTypeAliasDeclaration *typeAliasDecl) override;

    util::StringView FormInterfaceOrEnumDeclarationIdBinding(ir::Identifier *id) override;

    virtual bool AllowInterfaceRedeclaration()
    {
        return false;
    }

    void VisitTSInterfaceDeclaration(ir::TSInterfaceDeclaration *interfDecl) override;

    void VisitTSEnumMember(ir::TSEnumMember *enumMember) override;

    void VisitTSEnumDeclaration(ir::TSEnumDeclaration *enumDecl) override;

    void VisitTSTypeParameter(ir::TSTypeParameter *typeParam) override;

    void VisitTSTypeParameterDeclaration(ir::TSTypeParameterDeclaration *paramDecl) override;

    void VisitClassDefinition(ir::ClassDefinition *classDef) override;
};

class InitScopesPhaseJs : public ScopesInitPhase {
public:
    InitScopesPhaseJs() = default;
    NO_COPY_SEMANTIC(InitScopesPhaseJs);
    NO_MOVE_SEMANTIC(InitScopesPhaseJs);

    ~InitScopesPhaseJs() override = default;
};

class InitScopesPhaseTs : public ScopeInitTyped {
protected:
    bool AllowInterfaceRedeclaration() override
    {
        return true;
    }

    void VisitTSMappedType([[maybe_unused]] ir::TSMappedType *mapped) override {}
    void VisitTSInferType([[maybe_unused]] ir::TSInferType *infer) override {}
    void VisitExportDefaultDeclaration(ir::ExportDefaultDeclaration *exportDecl) override;
    void VisitExportNamedDeclaration(ir::ExportNamedDeclaration *exportDecl) override;
    void VisitImportDeclaration(ir::ImportDeclaration *importDeclaration) override;
    void VisitTSFunctionType(ir::TSFunctionType *constrType) override;
    void VisitTSConstructorType(ir::TSConstructorType *constrT) override;
    void VisitArrowFunctionExpression(ir::ArrowFunctionExpression *arrowFExpr) override;
    void VisitTSSignatureDeclaration(ir::TSSignatureDeclaration *signDecl) override;
    void VisitTSMethodSignature(ir::TSMethodSignature *methodSign) override;

    void CreateFuncDecl(ir::ScriptFunction *func) override;
};

class InitScopesPhaseETS : public ScopeInitTyped {
public:
    InitScopesPhaseETS() = default;
    NO_COPY_SEMANTIC(InitScopesPhaseETS);
    NO_MOVE_SEMANTIC(InitScopesPhaseETS);

    /**
     * Set scopes for ast-subtree
     * @param node ast-subtree, for this node and all children scopes will be initialized.
     * @param varbinder ref to VarBinder. All varbinder scopes should be set to current context.
     * Note: It's programmer responsibility to prepare VarBinder (remove previous names, set current scope, etc...)
     *
     * Example: // CC-OFF(G.CMT.04) false positive
     * f<T>(x: Int) :  {
     *     let y = 0;
     * }
     * After ScopesInitPhase scope structure will look something like this:
     * global_scope:
     *     [f],
     *     local_scope:
     *        [T],
     *        function_param_scope:
     *            [x],
     *            function_scope:
     *                [y]
     * Suppose you want to rewrite function body in some lowering later to
     * {
     *     let z = 123;
     * }
     *
     * Then you should pass your new created node = ir::BlockStatement() to RunExternalNode,
     * set varbinder to previous `function_scope` and call RunExternalNode(node, varbinder).
     * It will update scopes to: // CC-OFF(G.CMT.04) false positive
     * global_scope:
     *     [f],
     *     local_scope:
     *        [T],
     *        function_param_scope:
     *            [x],
     *            function_scope:
     *                [z]
     */
    static void RunExternalNode(ir::AstNode *node, varbinder::VarBinder *varbinder);
    /**
     * Same as previous, just uses varbinder from ctx->VarBinder()
     */
    static void RunExternalNode(ir::AstNode *node, parser::Program *ctx);

    /**
     * Run scope initialization on program.
     * It's not same as RunExternalNode(program->Ast()), because there's some specific handling for top scope.
     * @param ctx
     * @param program - program you want to set scopes on.
     * @return true if successful.
     */
    bool Perform(PhaseContext *ctx, parser::Program *program) override;

    ~InitScopesPhaseETS() override = default;

private:
    void HandleProgram(parser::Program *program);

    void HandleETSModule(ir::BlockStatement *script);

    void ParseGlobalClass(ir::ClassDefinition *global);

    void AddGlobalDeclaration(ir::AstNode *node);

    varbinder::Decl *BindClassName([[maybe_unused]] ir::ClassDefinition *identNode) override
    {
        return nullptr;
    }

    void BindVarDecl(ir::Identifier *binding, ir::Expression *init, varbinder::Decl *decl,
                     varbinder::Variable *var) override;
    void DeclareClassMethod(ir::MethodDefinition *method);
    void MaybeAddOverload(ir::MethodDefinition *method, ir::Identifier *methodName, varbinder::Variable *found,
                          varbinder::ClassScope *clsScope, varbinder::LocalScope *targetScope);
    void DeclareClassOverload(ir::OverloadDeclaration *overloaddecl);
    varbinder::LocalScope *OverloadTargetScope(ir::OverloadDeclaration *overloaddecl, varbinder::ClassScope *clsScope);

    void VisitClassStaticBlock(ir::ClassStaticBlock *staticBlock) override;
    void VisitBlockExpression(ir::BlockExpression *blockExpr) override;
    void VisitImportNamespaceSpecifier(ir::ImportNamespaceSpecifier *importSpec) override;
    void VisitImportSpecifier([[maybe_unused]] ir::ImportSpecifier *importSpec) override;
    void VisitImportDefaultSpecifier([[maybe_unused]] ir::ImportDefaultSpecifier *importSpec) override {};
    void VisitETSReExportDeclaration(ir::ETSReExportDeclaration *reExport) override;
    void VisitETSParameterExpression(ir::ETSParameterExpression *paramExpr) override;
    void VisitETSImportDeclaration(ir::ETSImportDeclaration *importDecl) override;
    void VisitTSEnumMember(ir::TSEnumMember *enumMember) override;
    void VisitOverloadDeclaration(ir::OverloadDeclaration *overload) override;
    void VisitMethodDefinition(ir::MethodDefinition *method) override;
    void VisitETSFunctionType(ir::ETSFunctionType *funcType) override;
    void VisitETSNewClassInstanceExpression(ir::ETSNewClassInstanceExpression *newClassExpr) override;
    void VisitTSTypeParameter(ir::TSTypeParameter *typeParam) override;
    void VisitTSInterfaceDeclaration(ir::TSInterfaceDeclaration *interfaceDecl) override;
    void VisitTSEnumDeclaration(ir::TSEnumDeclaration *enumDecl) override;
    void VisitTSTypeAliasDeclaration(ir::TSTypeAliasDeclaration *typeAlias) override;
    void VisitClassDefinition(ir::ClassDefinition *classDef) override;
    void VisitTSInterfaceBody(ir::TSInterfaceBody *interfBody) override;
    void VisitClassProperty(ir::ClassProperty *classProp) override;
    void VisitBreakStatement(ir::BreakStatement *stmt) override;
    void VisitContinueStatement(ir::ContinueStatement *stmt) override;
    void AttachLabelToScope(ir::AstNode *node) override;
    void VisitArrowFunctionExpression(ir::ArrowFunctionExpression *arrowExpr) override
    {
        Iterate(arrowExpr);
    }

    util::StringView FormInterfaceOrEnumDeclarationIdBinding(ir::Identifier *id) override
    {
        return id->Name();
    }

    static void AddGlobalToBinder(parser::Program *program);

    void FilterInterfaceOverloads(ArenaVector<ir::AstNode *> &props);

    void FilterOverloads(ArenaVector<ir::AstNode *> &props);
};

class InitScopesPhaseAS : public ScopesInitPhase {
public:
    NO_COPY_SEMANTIC(InitScopesPhaseAS);
    NO_MOVE_SEMANTIC(InitScopesPhaseAS);
    InitScopesPhaseAS() = default;
    ~InitScopesPhaseAS() override = default;

private:
    void VisitArrowFunctionExpression(ir::ArrowFunctionExpression *arrowExpr) override;
    void VisitExportNamedDeclaration(ir::ExportNamedDeclaration *exportDecl) override;
};
}  // namespace ark::es2panda::compiler

#endif
