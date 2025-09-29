/**
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#include "util.h"

#include "checker/checkerContext.h"
#include "checker/types/globalTypesHolder.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "ir/expressions/identifier.h"
#include "checker/checker.h"
#include "checker/ETSAnalyzer.h"
#include "parser/JsdocHelper.h"
#include "parser/program/program.h"
#include "util/ustring.h"
#include "varbinder/varbinder.h"

namespace ark::es2panda::compiler {

bool HasGlobalClassParent(const ir::AstNode *node)
{
    auto parentClass = util::Helpers::FindAncestorGivenByType(node, ir::AstNodeType::CLASS_DEFINITION);
    return parentClass != nullptr && parentClass->AsClassDefinition()->IsGlobal();
}

varbinder::Scope *NearestScope(const ir::AstNode *ast)
{
    while (ast != nullptr && !ast->IsScopeBearer()) {
        ast = ast->Parent();
    }

    return ast == nullptr ? nullptr : ast->Scope();
}

// Returns ArenaVector of ClassScopes from `findFrom` scope to common one with `base` scope, except ETSGLOBAL
std::vector<varbinder::ClassScope *> DiffClassScopes(varbinder::Scope *base, varbinder::Scope *findFrom)
{
    ES2PANDA_ASSERT(base != nullptr && findFrom != nullptr);

    auto result = std::vector<varbinder::ClassScope *> {};
    auto baseScopes = std::set<varbinder::Scope *> {};

    for (varbinder::Scope *currentScope = base; currentScope != nullptr && !currentScope->IsGlobalScope();
         currentScope = currentScope->Parent()) {
        baseScopes.insert(currentScope);
    }

    for (varbinder::Scope *currentScope = findFrom;
         baseScopes.find(currentScope) == baseScopes.end() && !currentScope->IsGlobalScope();
         currentScope = currentScope->Parent()) {
        if (currentScope->IsClassScope() && currentScope->AsClassScope()->Node() != nullptr &&
            currentScope->AsClassScope()->Node()->IsClassDefinition() &&
            !currentScope->AsClassScope()->Node()->AsClassDefinition()->IsGlobal()) {
            result.push_back(currentScope->AsClassScope());
        }
    }

    return result;
}

checker::ETSObjectType const *ContainingClass(const ir::AstNode *ast)
{
    ast = util::Helpers::FindAncestorGivenByType(ast, ir::AstNodeType::CLASS_DEFINITION);
    return ast == nullptr ? nullptr : ast->AsClassDefinition()->TsType()->AsETSObjectType();
}

ir::Identifier *Gensym(ArenaAllocator *const allocator)
{
    util::UString const s = GenName(allocator);
    return allocator->New<ir::Identifier>(s.View(), allocator);
}

std::string GenName()
{
    static std::size_t gensymCounter = 0U;
    static std::mutex gensymCounterMutex {};
    std::size_t individualGensym = 0;
    {
        std::lock_guard lock(gensymCounterMutex);
        individualGensym = ++gensymCounter;
    }
    return std::string(GENSYM_CORE) + std::to_string(individualGensym);
}

util::UString GenName(ArenaAllocator *const allocator)
{
    return util::UString {GenName(), allocator};
}

void SetSourceRangesRecursively(ir::AstNode *node, const lexer::SourceRange &range)
{
    ES2PANDA_ASSERT(node != nullptr);
    node->SetRange(range);
    node->IterateRecursively([](ir::AstNode *n) { n->SetRange(n->Parent()->Range()); });
}

ir::AstNode *RefineSourceRanges(ir::AstNode *node)
{
    auto const isInvalidRange = [](lexer::SourceRange const &range) {
        return (range.start.index == 0 && range.start.line == 0 && range.end.index == 0 && range.end.line == 0) ||
               (range.end.index < range.start.index);
    };

    auto const isDummyLoc = [isInvalidRange](lexer::SourceRange const &range, ir::AstNode *ast) {
        return isInvalidRange(range) || (range.start.index < ast->Parent()->Start().index) ||
               (range.end.index > ast->Parent()->End().index) ||
               (ast->IsMethodDefinition() && !ast->AsMethodDefinition()->Overloads().empty());
    };

    auto const refine = [isDummyLoc, isInvalidRange](ir::AstNode *ast) {
        if (ast->Parent() != nullptr && isDummyLoc(ast->Range(), ast) && !isInvalidRange(ast->Parent()->Range())) {
            ast->SetRange(ast->Parent()->Range());
        }
    };

    refine(node);
    node->IterateRecursively(refine);
    return node;
}

// Function to clear expression node types and identifier node variables (for correct re-binding and re-checking)
void ClearTypesVariablesAndScopes(ir::AstNode *node) noexcept
{
    std::function<void(ir::AstNode *)> doNode = [&](ir::AstNode *nn) {
        if (nn->IsOpaqueTypeNode()) {
            return;
        }
        if (nn->IsScopeBearer()) {
            nn->ClearScope();
        }
        if (nn->IsTyped() && !(nn->IsExpression() && nn->AsExpression()->IsTypeNode())) {
            nn->AsTyped()->SetTsType(nullptr);
        }
        if (nn->IsIdentifier()) {
            nn->AsIdentifier()->SetVariable(nullptr);
        }
        if (!nn->IsETSTypeReference() && !nn->IsLabelledStatement()) {
            nn->Iterate([&](ir::AstNode *child) { doNode(child); });
        }
    };

    doNode(node);
}

ArenaSet<varbinder::Variable *> FindCaptured(ArenaAllocator *allocator, ir::AstNode *scopeBearer) noexcept
{
    auto result = ArenaSet<varbinder::Variable *> {allocator->Adapter()};
    auto scopes = ArenaSet<varbinder::Scope *> {allocator->Adapter()};
    scopeBearer->IterateRecursivelyPreorder([&result, &scopes](ir::AstNode *ast) {
        if (ast->IsScopeBearer() && ast->Scope() != nullptr) {
            scopes.insert(ast->Scope());
            if (ast->Scope()->IsFunctionScope()) {
                scopes.insert(ast->Scope()->AsFunctionScope()->ParamScope());
            } else if (ast->IsForUpdateStatement() || ast->IsForInStatement() || ast->IsForOfStatement() ||
                       ast->IsCatchClause()) {
                // NOTE(gogabr) LoopScope _does not_ currently respond to IsLoopScope().
                // For now, this is the way to reach LoopDeclarationScope.
                scopes.insert(ast->Scope()->Parent());
            }
        }
        if (ast->IsIdentifier() && !ast->Parent()->IsLabelledStatement()) {
            auto *var = ast->AsIdentifier()->Variable();
            if (var == nullptr || !var->HasFlag(varbinder::VariableFlags::LOCAL)) {
                return;
            }
            auto *sc = var->GetScope();
            if (sc != nullptr && !sc->IsClassScope() && !sc->IsGlobalScope() && scopes.count(var->GetScope()) == 0) {
                result.insert(var);
            }
        }
    });
    return result;
}

static void ResetGlobalClass(parser::Program *prog)
{
    for (auto *statement : prog->Ast()->Statements()) {
        if (statement->IsClassDeclaration() && statement->AsClassDeclaration()->Definition()->IsGlobal()) {
            prog->SetGlobalClass(statement->AsClassDeclaration()->Definition());
            break;
        }
    }
}

static bool IsGeneratedForUtilityType(ir::AstNode const *ast)
{
    if (ast->IsClassDeclaration()) {
        auto &name = ast->AsClassDeclaration()->Definition()->Ident()->Name();
        return name.StartsWith(checker::PARTIAL_CLASS_PREFIX);
    }
    if (ast->IsTSInterfaceDeclaration()) {
        auto &name = ast->AsTSInterfaceDeclaration()->Id()->Name();
        return name.StartsWith(checker::PARTIAL_CLASS_PREFIX);
    }
    return false;
}

static void ClearHelper(parser::Program *prog)
{
    prog->RemoveAstChecked();
    ResetGlobalClass(prog);
    // #24256 Should be removed when code refactoring on checker is done and no ast node allocated in checker.
    auto &stmts = prog->Ast()->StatementsForUpdates();
    // clang-format off
    stmts.erase(std::remove_if(stmts.begin(), stmts.end(),
        [](ir::AstNode *ast) -> bool {
            return !ast->HasAstNodeFlags(ir::AstNodeFlags::NOCLEANUP) ||
                IsGeneratedForUtilityType(ast);
        }),
        stmts.end());
    // clang-format on

    prog->Ast()->IterateRecursively([](ir::AstNode *ast) -> void { ast->CleanUp(); });
    prog->Ast()->ClearScope();
}

// Rerun varbinder on the node. (First clear typesVariables and scopes)
varbinder::Scope *Rebind(PhaseManager *phaseManager, varbinder::ETSBinder *varBinder, ir::AstNode *node)
{
    if (node->IsProgram()) {
        auto program = node->AsETSModule()->Program();
        ES2PANDA_ASSERT(program == phaseManager->Context()->parserProgram);

        if (program->Is<util::ModuleKind::PACKAGE>()) {
            return nullptr;
        }

        program->GetExternalSources()->Visit([](auto *extProg) { ClearHelper(extProg); });

        ClearHelper(program);

        varBinder->CleanUp();
        for (auto *phase : phaseManager->RebindPhases()) {
            phase->Apply(phaseManager->Context());
        }

        return varBinder->TopScope();
    }

    auto *scope = NearestScope(node->Parent());
    auto bscope = varbinder::LexicalScope<varbinder::Scope>::Enter(varBinder, scope);

    ClearTypesVariablesAndScopes(node);
    InitScopesPhaseETS::RunExternalNode(node, varBinder);
    varBinder->ResolveReferencesForScopeWithContext(node, scope);

    return scope;
}

static void CollectDirectExtSources(parser::Program *globalProg, parser::Program *program,
                                    std::map<ArenaString, parser::Program *> &progsFromPath)
{
    auto &directSources = program->GetExternalSources()->Direct();
    for (const auto &fileDepends : globalProg->GetFileDependencies()[ArenaString {program->SourceFilePath().Utf8()}]) {
        if (progsFromPath.find(fileDepends) == progsFromPath.end()) {
            continue;
        }
        ES2PANDA_ASSERT(directSources.find(fileDepends) == directSources.end());
        directSources[fileDepends] = progsFromPath[fileDepends];
    }
}

class RecheckGraph {
public:
    class Node {
    public:
        explicit Node(parser::Program *prog) : prog_(prog) {}

        std::set<Node *> &ImportedNodes()
        {
            return importedNodes_;
        }
        std::set<Node *> &NodesImportedBy()
        {
            return nodesImportedBy_;
        }
        parser::Program *Prog() const
        {
            return prog_;
        }

    private:
        std::set<Node *> importedNodes_;
        std::set<Node *> nodesImportedBy_;
        parser::Program *prog_;
    };

    std::map<parser::Program *, Node> &Programs()
    {
        return programs_;
    }
    std::set<Node *> &FoundModifiedProgs()
    {
        return foundModifiedProgs_;
    }

private:
    std::map<parser::Program *, Node> programs_ {};
    std::set<Node *> foundModifiedProgs_ {};
};

static RecheckGraph::Node *RecheckGraphCreatorHelper(parser::Program *globalProg, parser::Program *program,
                                                     std::map<ArenaString, parser::Program *> &progsFromPath,
                                                     RecheckGraph *graph)
{
    if (graph->Programs().find(program) != graph->Programs().end()) {
        return &graph->Programs().at(program);
    }
    graph->Programs().emplace(program, RecheckGraph::Node(program));
    auto *node = &graph->Programs().at(program);
    if (program->GetExternalSources()->Empty()) {
        CollectDirectExtSources(globalProg, program, progsFromPath);
    }

    auto runOnSource = [&progsFromPath, globalProg, graph, node](auto *prog) {
        RecheckGraph::Node *importedNode = nullptr;
        importedNode = RecheckGraphCreatorHelper(globalProg, prog, progsFromPath, graph);
        node->ImportedNodes().emplace(importedNode);
        importedNode->NodesImportedBy().emplace(node);
    };
    for (auto [_, source] : program->GetExternalSources()->Direct()) {
        (void)_;
        runOnSource(source);
    }
    program->GetExternalSources()->Visit(runOnSource);

    if (program->IsProgramModified()) {
        graph->FoundModifiedProgs().emplace(node);
    }
    program->GetExternalSources()->Direct().clear();
    return node;
}

static void MarkModifiedRecursively(RecheckGraph::Node *node)
{
    if (node->Prog()->IsProgramModified()) {
        return;
    }
    node->Prog()->SetProgramModified(true);
    for (auto importedBy : node->NodesImportedBy()) {
        MarkModifiedRecursively(importedBy);
    }
}

// If any of package fractions is modified, whole package modified
static void ExtendModifiedFlagOnPackagePrograms(parser::Program *globalProg)
{
    std::unordered_set<parser::ProgramAdapter<util::ModuleKind::PACKAGE> *> modifiedPackagePrograms {};

    for (auto *packageProg : globalProg->GetExternalSources()->Get<util::ModuleKind::PACKAGE>()) {
        if (packageProg->GetUnmergedPackagePrograms().empty()) {
            continue;
        }
        packageProg->SetProgramModified(false);
        for (auto fraction : packageProg->GetUnmergedPackagePrograms()) {
            if (fraction->IsProgramModified()) {
                modifiedPackagePrograms.insert(packageProg);
                break;
            }
        }
    }

    for (auto *packageProg : modifiedPackagePrograms) {
        packageProg->SetProgramModified(true);
        for (auto fraction : packageProg->GetUnmergedPackagePrograms()) {
            fraction->SetProgramModified(true);
        }
    }
}

static bool ExtendModifiedFlagOnDependentPrograms(parser::Program *globalProg, parser::Program *program)
{
    ExtendModifiedFlagOnPackagePrograms(globalProg);

    std::map<ArenaString, parser::Program *> progsFromPath;
    progsFromPath[ArenaString {program->SourceFilePath().Utf8()}] = program;

    program->GetExternalSources()->Visit(
        [&progsFromPath](auto *prog) { progsFromPath.emplace(ArenaString {prog->SourceFilePath().Utf8()}, prog); });

    RecheckGraph graph;
    RecheckGraphCreatorHelper(globalProg, program, progsFromPath, &graph);

    for (auto node : graph.FoundModifiedProgs()) {
        if (node->Prog()->IsProgramModified()) {
            node->Prog()->SetProgramModified(false);
            MarkModifiedRecursively(node);
        }
    }

    return program->IsProgramModified();
}

template <typename CB>
static void IterateExternalProgramsForBinderAndCheckerPushing(parser::Program *program, const CB &cb)
{
    program->GetExternalSources()->Visit(cb);
    // push binders to package-top-programs since in the call above only package fractions were iterated:
    for (auto *packageProg : program->GetExternalSources()->Get<util::ModuleKind::PACKAGE>()) {
        cb(packageProg);
    }
}

static void RestoreGlobalTypesHolder(checker::ETSChecker *newChecker, parser::Program *program)
{
    checker::GlobalTypesHolder *globalTypesHolder = nullptr;
    program->GetExternalSources()->Visit([&globalTypesHolder](auto *extProg) {
        if ((globalTypesHolder == nullptr) && (!extProg->IsProgramModified() || extProg->IsASTLowered())) {
            globalTypesHolder = extProg->Checker()->GetGlobalTypesHolder();
        }
    });
    if (globalTypesHolder != nullptr) {
        newChecker->SetGlobalTypesHolder(globalTypesHolder);
    }
}

using SavedVarbindersAndCheckers = std::map<parser::Program *, std::pair<varbinder::VarBinder *, checker::Checker *>>;
static varbinder::ETSBinder *SetupNewVarBinderHierarchy(public_lib::Context *ctx, parser::Program *program,
                                                        SavedVarbindersAndCheckers varbindersCheckers,
                                                        varbinder::ETSBinder *varBinder)
{
    auto newVarbinder = new varbinder::ETSBinder(ctx);
    newVarbinder->SetProgram(program);
    program->PushVarBinder(newVarbinder);
    varBinder->CopyTo(newVarbinder);

    auto visitor = [newVarbinder, &varbindersCheckers](parser::Program *prog) {
        if (!prog->IsASTLowered() && prog->IsProgramModified()) {
            ClearHelper(prog);
            prog->PushVarBinder(newVarbinder);
            return;
        }
        prog->PushVarBinder(varbindersCheckers.at(prog).first);
        if (prog->Is<util::ModuleKind::PACKAGE>()) {
            return;
        }
        prog->PushChecker(varbindersCheckers.at(prog).second);
    };
    IterateExternalProgramsForBinderAndCheckerPushing(program, visitor);
    return newVarbinder;
}

static void RecheckProgram(PhaseManager *phaseManager, varbinder::ETSBinder *varBinder, parser::Program *program)
{
    auto ctx = phaseManager->Context();
    if (!ExtendModifiedFlagOnDependentPrograms(ctx->parserProgram, program)) {
        return;
    }

    auto newChecker = ctx->allocator->New<checker::ETSChecker>(ctx->allocator, *ctx->diagnosticEngine);
    auto analyzer = ctx->allocator->New<checker::ETSAnalyzer>(newChecker);

    RestoreGlobalTypesHolder(newChecker, program);

    std::set<varbinder::VarBinder *> savedVarBinders {};
    SavedVarbindersAndCheckers varbindersCheckers {};
    IterateExternalProgramsForBinderAndCheckerPushing(
        program, [&savedVarBinders, &varbindersCheckers](parser::Program *prog) {
            if (!prog->IsASTLowered() && prog->IsProgramModified()) {
                return;
            }
            savedVarBinders.insert(prog->VarBinder());
            varbindersCheckers[prog].first = prog->VarBinder();
            varbindersCheckers[prog].second = prog->Is<util::ModuleKind::PACKAGE>() ? nullptr : prog->Checker();
        });

    phaseManager->SetCurrentPhaseId(0);

    auto newVarbinder = SetupNewVarBinderHierarchy(ctx, program, varbindersCheckers, varBinder);

    ClearHelper(program);

    ctx->PushAnalyzer(analyzer);
    newChecker->SetAnalyzer(analyzer);
    newChecker->Initialize(newVarbinder);
    ctx->PushChecker(newChecker);

    for (auto *savedVarBinder : savedVarBinders) {
        for (auto func : savedVarBinder->Functions()) {
            if (func->Node()->Program() != nullptr && !func->Node()->Program()->IsProgramModified()) {
                newVarbinder->Functions().push_back(func);
            }
        }
    }

    for (auto *phase : phaseManager->RecheckPhases()) {
        phase->Apply(ctx);
    }
    phaseManager->SetCurrentPhaseIdToAfterCheck();
    IterateExternalProgramsForBinderAndCheckerPushing(program, [newVarbinder, newChecker](parser::Program *prog) {
        prog->PushVarBinder(newVarbinder);
        if (prog->Is<util::ModuleKind::PACKAGE>()) {
            return;
        }
        prog->PushChecker(newChecker);
    });
}

// Rerun varbinder and checker on the node.
void Recheck(PhaseManager *phaseManager, varbinder::ETSBinder *varBinder, checker::ETSChecker *checker,
             ir::AstNode *node)
{
    RefineSourceRanges(node);
    if (node->IsProgram()) {
        return RecheckProgram(phaseManager, varBinder, node->AsETSModule()->Program());
    }

    auto *scope = Rebind(phaseManager, varBinder, node);

    // NOTE(gogabr: should determine checker status more finely.
    auto *containingClass = ContainingClass(node);
    checker::CheckerStatus newStatus =
        (containingClass == nullptr) ? checker::CheckerStatus::NO_OPTS : checker::CheckerStatus::IN_CLASS;
    if ((checker->Context().Status() & checker::CheckerStatus::IN_EXTENSION_ACCESSOR_CHECK) != 0) {
        newStatus |= checker::CheckerStatus::IN_EXTENSION_ACCESSOR_CHECK;
    }
    auto checkerCtx = checker::SavedCheckerContext(checker, newStatus, containingClass);
    auto scopeCtx = checker::ScopeContext(checker, scope);

    node->Check(checker);
}

// NOTE: used to get the declaration name in Plugin API and LSP
std::optional<std::string> GetNameOfDeclaration(const ir::AstNode *node)
{
    if (node == nullptr) {
        return std::nullopt;
    }
    switch (node->Type()) {
        case ir::AstNodeType::IDENTIFIER:
            return std::string(node->AsIdentifier()->Name().Utf8());
        case ir::AstNodeType::METHOD_DEFINITION:
            return std::string(node->AsMethodDefinition()->Id()->Name().Utf8());
        case ir::AstNodeType::FUNCTION_DECLARATION:
            return std::string(node->AsFunctionDeclaration()->Function()->Id()->Name().Utf8());
        case ir::AstNodeType::FUNCTION_EXPRESSION:
            return std::string(node->AsFunctionExpression()->Function()->Id()->Name().Utf8());
        case ir::AstNodeType::CLASS_DEFINITION:
            return std::string(node->AsClassDefinition()->Ident()->Name().Utf8());
        case ir::AstNodeType::CLASS_PROPERTY:
            return std::string(node->AsClassProperty()->Id()->Name().Utf8());
        case ir::AstNodeType::TS_INTERFACE_DECLARATION:
            return std::string(node->AsTSInterfaceDeclaration()->Id()->Name().Utf8());
        case ir::AstNodeType::VARIABLE_DECLARATION:
            if (node->AsVariableDeclaration()->Declarators()[0]->Id()->IsIdentifier()) {
                ir::Identifier *ident = node->AsVariableDeclaration()->Declarators()[0]->Id()->AsIdentifier();
                return std::string(ident->Name().Utf8());
            } else {
                return std::nullopt;
            }
        default:
            return std::nullopt;
    }
}

// NOTE: used to get the declaration from identifier in Plugin API and LSP
ir::AstNode *DeclarationFromIdentifier(const ir::Identifier *node)
{
    if (node == nullptr) {
        return nullptr;
    }

    auto idVar = node->Variable();
    if (idVar == nullptr) {
        return nullptr;
    }
    auto decl = idVar->Declaration();
    if (decl == nullptr) {
        return nullptr;
    }
    return decl->Node();
}

// NOTE: used to get the license string from the input root node.
util::StringView GetLicenseFromRootNode(const ir::AstNode *node)
{
    std::unique_ptr<parser::JsdocHelper> jsdocGetter = std::make_unique<parser::JsdocHelper>(node);
    return jsdocGetter->GetLicenseStringFromStart();
}

// NOTE: used to get the jsdoc string from the input node.
util::StringView JsdocStringFromDeclaration(const ir::AstNode *node)
{
    std::unique_ptr<parser::JsdocHelper> jsdocGetter = std::make_unique<parser::JsdocHelper>(node);
    return jsdocGetter->GetJsdocBackward();
}

// Note: run varbinder on the new node generated in lowering phases (without ClearTypesVariablesAndScopes)
void BindLoweredNode(varbinder::ETSBinder *varBinder, ir::AstNode *node)
{
    RefineSourceRanges(node);
    InitScopesPhaseETS::RunExternalNode(node, varBinder);
    auto *scope = NearestScope(node);
    varBinder->ResolveReferencesForScopeWithContext(node, scope);
}

// Note: run varbinder and checker on the new node generated in lowering phases (without ClearTypesVariablesAndScopes)
void CheckLoweredNode(varbinder::ETSBinder *varBinder, checker::ETSChecker *checker, ir::AstNode *node)
{
    RefineSourceRanges(node);
    InitScopesPhaseETS::RunExternalNode(node, varBinder);
    auto *scope = NearestScope(node);
    varBinder->ResolveReferencesForScopeWithContext(node, scope);

    checker::CheckerStatus newStatus = checker::CheckerStatus::NO_OPTS;
    auto *containingClass = util::Helpers::GetContainingClassDefinition(node);

    if (containingClass != nullptr) {
        if (containingClass->IsAbstract()) {
            newStatus = checker::CheckerStatus::IN_ABSTRACT;
        } else {
            newStatus = checker::CheckerStatus::IN_CLASS;
        }
    }

    if ((checker->Context().Status() & checker::CheckerStatus::IN_EXTENSION_ACCESSOR_CHECK) != 0) {
        newStatus |= checker::CheckerStatus::IN_EXTENSION_ACCESSOR_CHECK;
    }

    auto checkerCtx = checker::SavedCheckerContext(
        checker, newStatus, containingClass == nullptr ? nullptr : containingClass->TsType()->AsETSObjectType());
    auto scopeCtx = checker::ScopeContext(checker, scope);

    node->Check(checker);
}

bool IsAnonymousClassType(const checker::Type *type)
{
    if (type == nullptr || !type->IsETSObjectType()) {
        return false;
    }

    auto declNode = type->AsETSObjectType()->GetDeclNode();
    return declNode != nullptr && declNode->IsClassDefinition() && declNode->AsClassDefinition()->IsAnonymous();
}

bool ClassDefinitionIsEnumTransformed(const ir::AstNode *node)
{
    return node != nullptr && node->IsClassDefinition() && node->AsClassDefinition()->IsEnumTransformed();
}
}  // namespace ark::es2panda::compiler
