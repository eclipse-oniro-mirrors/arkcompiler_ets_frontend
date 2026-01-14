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
        if (program->IsPackage()) {
            return nullptr;
        }

        for (auto [_, program_list] : program->ExternalSources()) {
            for (auto prog : program_list) {
                ClearHelper(prog);
            }
        }

        ClearHelper(program);

        varBinder->CleanUp();
        for (auto *phase : phaseManager->RebindPhases()) {
            phase->Apply(varBinder->GetContext(), program);
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
                                    std::map<std::string, parser::Program *> &progsFromPath)
{
    auto &directSources = program->DirectExternalSources();
    for (const auto &fileDepends : globalProg->GetFileDependencies()[program->SourceFilePath()]) {
        if (progsFromPath.find(std::string {fileDepends}) == progsFromPath.end()) {
            continue;
        }

        ArenaVector<parser::Program *> extSources;

        if (const auto &it = directSources.find(fileDepends); it != directSources.end()) {
            extSources = it->second;
        } else {
            extSources = ArenaVector<parser::Program *>(program->Allocator()->Adapter());
        }
        extSources.emplace_back(progsFromPath[std::string {fileDepends}]);
        directSources.try_emplace(fileDepends, extSources);
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
                                                     std::map<std::string, parser::Program *> &progsFromPath,
                                                     RecheckGraph *graph)
{
    if (graph->Programs().find(program) != graph->Programs().end()) {
        return &graph->Programs().at(program);
    }
    graph->Programs().emplace(program, RecheckGraph::Node(program));
    auto node = &graph->Programs().at(program);
    if (program->DirectExternalSources().empty()) {
        CollectDirectExtSources(globalProg, program, progsFromPath);
    }

    auto runOnSources = [&](auto &sources) {
        for (auto [_, program_list] : sources) {
            for (auto prog : program_list) {
                RecheckGraph::Node *importedNode = nullptr;
                importedNode = RecheckGraphCreatorHelper(globalProg, prog, progsFromPath, graph);
                node->ImportedNodes().emplace(importedNode);
                importedNode->NodesImportedBy().emplace(node);
            }
        }
    };
    runOnSources(program->DirectExternalSources());
    runOnSources(program->ExternalSources());

    if (program->IsProgramModified()) {
        graph->FoundModifiedProgs().emplace(node);
    }
    program->DirectExternalSources().clear();
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

static bool ExtendModifiedFlagOnDependentPrograms(parser::Program *globalProg, parser::Program *program)
{
    std::map<std::string, parser::Program *> progsFromPath;
    progsFromPath[program->SourceFilePath().Mutf8()] = program;

    for (auto [_, program_list] : program->ExternalSources()) {
        for (auto prog : program_list) {
            progsFromPath.emplace(prog->SourceFilePath().Mutf8(), prog);
        }
    }
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

using SavedVarbindersAndCheckers = std::map<parser::Program *, std::pair<varbinder::VarBinder *, checker::Checker *>>;
static SavedVarbindersAndCheckers SaveExternalVarbindersAndCheckers(parser::Program *program,
                                                                    std::set<varbinder::VarBinder *> &savedVarBinders)
{
    std::map<parser::Program *, std::pair<varbinder::VarBinder *, checker::Checker *>> varbindersCheckers;
    for (auto [_, program_list] : program->ExternalSources()) {
        for (auto prog : program_list) {
            if (!prog->IsASTLowered() && prog->IsProgramModified()) {
                continue;
            }
            savedVarBinders.insert(prog->VarBinder());
            varbindersCheckers[prog].first = prog->VarBinder();
            varbindersCheckers[prog].second = prog->Checker();
        }
    }
    return varbindersCheckers;
}

static void HandleExternalProgram(
    varbinder::ETSBinder *newVarbinder, parser::Program *program,
    std::map<parser::Program *, std::pair<varbinder::VarBinder *, checker::Checker *>> varbinders)
{
    for (auto [_, program_list] : program->ExternalSources()) {
        for (auto prog : program_list) {
            if (!prog->IsASTLowered() && prog->IsProgramModified()) {
                ClearHelper(prog);
                prog->PushVarBinder(newVarbinder);
                continue;
            }
            prog->PushVarBinder(varbinders.at(prog).first);
            prog->PushChecker(varbinders.at(prog).second);
        }
    }
}

static void RecheckProgram(PhaseManager *phaseManager, varbinder::ETSBinder *varBinder, parser::Program *program)
{
    auto ctx = varBinder->GetContext();
    if (!ExtendModifiedFlagOnDependentPrograms(ctx->parserProgram, program)) {
        return;
    }
    checker::GlobalTypesHolder *globalTypesHolder = nullptr;
    for (auto [_, program_list] : program->ExternalSources()) {
        if (auto prog = program_list.front(); !prog->IsProgramModified() || prog->IsASTLowered()) {
            globalTypesHolder = prog->Checker()->GetGlobalTypesHolder();
            break;
        }
    }
    std::set<varbinder::VarBinder *> savedVarBinders {};
    auto varbindersCheckers = SaveExternalVarbindersAndCheckers(program, savedVarBinders);
    phaseManager->SetCurrentPhaseId(0);

    auto newVarbinder = new varbinder::ETSBinder(ctx->allocator);
    newVarbinder->SetProgram(program);
    newVarbinder->SetContext(ctx);
    program->PushVarBinder(newVarbinder);
    varBinder->CopyTo(newVarbinder);
    HandleExternalProgram(newVarbinder, program, varbindersCheckers);

    ClearHelper(program);

    auto newChecker = ctx->allocator->New<checker::ETSChecker>(ctx->allocator, *ctx->diagnosticEngine);
    auto analyzer = ctx->allocator->New<checker::ETSAnalyzer>(newChecker);

    ctx->PushAnalyzer(analyzer);
    newChecker->SetAnalyzer(analyzer);
    newChecker->Initialize(newVarbinder);
    ctx->PushChecker(newChecker);
    if (globalTypesHolder != nullptr) {
        newChecker->SetGlobalTypesHolder(globalTypesHolder);
    }

    for (auto *savedVarBinder : savedVarBinders) {
        for (auto func : savedVarBinder->Functions()) {
            if (func->Node()->Program() != nullptr && !func->Node()->Program()->IsProgramModified()) {
                newVarbinder->Functions().push_back(func);
            }
        }
    }

    for (auto *phase : phaseManager->RecheckPhases()) {
        phase->Apply(ctx, program);
    }
    phaseManager->SetCurrentPhaseIdToAfterCheck();
    for (auto [_, program_list] : program->ExternalSources()) {
        for (auto prog : program_list) {
            prog->PushVarBinder(newVarbinder);
            prog->PushChecker(newChecker);
        }
    }
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

parser::Program *SearchExternalProgramInImport(const parser::Program::DirectExternalSource &extSource,
                                               const util::ImportPathManager::ImportMetadata &importMetadata)
{
    parser::Program *extProg = nullptr;
    const auto importPath = importMetadata.resolvedSource;
    const auto declPath = importMetadata.declPath;
    // Search Correct external program by comparing importPath and absolutePath
    for (auto &[_, progs] : extSource) {
        auto it = std::find_if(progs.begin(), progs.end(), [&](const auto *prog) {
            return prog->AbsoluteName() == importPath || prog->AbsoluteName() == declPath;
        });
        if (it != progs.end()) {
            extProg = *it;
            break;
        }
    }
    return extProg;
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
