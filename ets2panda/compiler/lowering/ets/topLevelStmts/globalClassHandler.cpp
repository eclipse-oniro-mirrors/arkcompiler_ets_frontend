/*
 * Copyright (c) 2023 - 2025 Huawei Device Co., Ltd.
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

#include "compiler/lowering/ets/topLevelStmts/globalClassHandler.h"
#include <algorithm>
#include "compiler/lowering/util.h"

#include "ir/base/classDefinition.h"
#include "ir/base/classProperty.h"
#include "ir/base/classStaticBlock.h"
#include "ir/base/scriptFunction.h"
#include "ir/base/methodDefinition.h"
#include "ir/ets/etsIntrinsicNode.h"
#include "ir/expressions/memberExpression.h"
#include "ir/expressions/identifier.h"
#include "ir/expressions/classExpression.h"
#include "ir/expressions/functionExpression.h"
#include "ir/expressions/callExpression.h"
#include "ir/statements/blockStatement.h"
#include "ir/statements/classDeclaration.h"
#include "ir/statements/expressionStatement.h"
#include "util/helpers.h"
#include "util/ustring.h"
#include "utils/arena_containers.h"
#include "generated/diagnostic.h"

namespace ark::es2panda::compiler {

using util::NodeAllocator;

void GlobalClassHandler::AddStaticBlockToClass(ir::AstNode *node)
{
    if (node->IsClassDefinition() && !node->AsClassDefinition()->IsDeclare()) {
        auto classDef = node->AsClassDefinition();
        if (auto staticBlock = CreateStaticBlock(classDef); staticBlock != nullptr) {
            classDef->EmplaceBody(staticBlock);  // NOTE(vpukhov): inserted to end for some reason
            staticBlock->SetParent(classDef);
        }
    }
}

std::string AddToNamespaceChain(std::string chain, std::string name)
{
    if (chain.empty()) {
        return name;
    }
    if (name.empty()) {
        return chain;
    }
    return chain + "." + name;
}

void GlobalClassHandler::CollectNamespaceExportedClasses(parser::Program *program, ir::ClassDefinition *classDef)
{
    CollectExportedClasses(program, classDef, classDef->Body());
}

void GlobalClassHandler::CollectReExportedClasses(parser::Program *program, ir::ClassDefinition *classDef,
                                                  const ir::ETSReExportDeclaration *reExport)
{
    auto importDecl = reExport->GetETSImportDeclarations();
    const auto importPath = reExport->GetETSImportDeclarations()->ImportMetadata().resolvedSource;
    parser::Program *extProg = nullptr;
    // Search Correct external program by comparing importPath and absolutePath
    for (auto &[_, progs] : program->DirectExternalSources()) {
        auto it = std::find_if(progs.begin(), progs.end(),
                               [&](const auto *prog) { return prog->AbsoluteName() == importPath; });
        if (it != progs.end()) {
            extProg = *it;
            break;
        }
    }
    if (extProg == nullptr) {
        return;
    }
    auto &externalExportedClasses = extProg->GlobalClass()->ExportedClasses();
    const auto &specifiers = importDecl->Specifiers();
    bool needAddETSGlobal = false;
    for (const auto *specifier : specifiers) {
        if (specifier->IsImportNamespaceSpecifier()) {
            classDef->BatchAddToExportedClasses(externalExportedClasses);
            break;
        }
        auto found = std::find_if(externalExportedClasses.begin(), externalExportedClasses.end(),
                                  [&specifier](const ir::ClassDeclaration *classDecl) {
                                      return specifier->IsImportSpecifier() &&
                                             specifier->AsImportSpecifier()->Imported()->Name() ==
                                                 // CC-OFFNXT(G.FMT.02-CPP) solid logic
                                                 classDecl->Definition()->Ident()->Name();
                                      // CC-OFFNXT(G.FMT.02-CPP) solid logic
                                  });
        if (found == externalExportedClasses.end()) {
            needAddETSGlobal = true;
            continue;
        }
        classDef->AddToExportedClasses(*found);
    }

    /*
     *        a.ets:                                 b.ets:
     * export let ident = 10             export {ident, A, B} from './a'
     * export class A {}
     * export class B {}
     *              Note: (`a.ets` exported classes: A, B and ETSGLOBAL)
     *
     * In this re-export declaration, we need manually add ETSGLOBAL to exportedClasses.
     */
    if (needAddETSGlobal) {
        classDef->AddToExportedClasses(extProg->GlobalClass()->Parent()->AsClassDeclaration());
    }
}

template <class Node>
void GlobalClassHandler::CollectExportedClasses(parser::Program *program, ir::ClassDefinition *classDef,
                                                const ArenaVector<Node *> &statements)
{
    for (const auto *statement : statements) {
        if (!statement->IsExported()) {
            continue;
        }
        if (statement->IsClassDeclaration()) {
            classDef->AddToExportedClasses(statement->AsClassDeclaration());
            continue;
        }
        if (statement->IsETSReExportDeclaration()) {
            CollectReExportedClasses(program, classDef, statement->AsETSReExportDeclaration());
        }
    }
    auto globalClass = program->GlobalClass();
    bool foundExport = false;
    // Add ETSGLOBAL to Module in case of export let a = 10
    std::function<void(ir::AstNode *)> findExportInGlobal = [&findExportInGlobal, &foundExport](ir::AstNode *node) {
        if (node->IsExported()) {
            foundExport = true;
            return;
        }
        node->Iterate(findExportInGlobal);
    };
    globalClass->Iterate(findExportInGlobal);
    if (foundExport) {
        auto globalClassDecl = globalClass->Parent()->AsClassDeclaration();
        classDef->AddToExportedClasses(globalClassDecl);
    }
}

ir::ClassDeclaration *GlobalClassHandler::CreateTransformedClass(ir::ETSModule *ns)
{
    auto className = ns->Ident()->Name();
    auto *ident = NodeAllocator::Alloc<ir::Identifier>(allocator_, className, allocator_);
    ES2PANDA_ASSERT(ident != nullptr);
    ident->SetRange(ns->Ident()->Range());

    auto *classDef = NodeAllocator::Alloc<ir::ClassDefinition>(allocator_, allocator_, ident,
                                                               ir::ClassDefinitionModifiers::CLASS_DECL,
                                                               ir::ModifierFlags::ABSTRACT, ns->Language());
    ES2PANDA_ASSERT(classDef != nullptr);
    classDef->SetRange(ns->Range());
    classDef->AddModifier(ns->Modifiers());
    auto *classDecl = NodeAllocator::Alloc<ir::ClassDeclaration>(allocator_, classDef, allocator_);
    ES2PANDA_ASSERT(classDecl != nullptr);
    classDecl->SetRange(ns->Range());
    classDecl->AddModifier(ns->Modifiers());
    classDef->SetNamespaceTransformed();
    ArenaVector<ir::AnnotationUsage *> annotations {allocator_->Adapter()};
    for (auto *anno : ns->Annotations()) {
        auto clone = anno->Clone(allocator_, classDef);
        annotations.push_back(clone);
    }

    classDef->SetAnnotations(std::move(annotations));
    classDecl->SetRange(ns->Range());
    return classDecl;
}

static void InsertInGlobal(ir::ClassDefinition *globalClass, ir::AstNode *node)
{
    ES2PANDA_ASSERT(node != nullptr);
    globalClass->BodyForUpdate().insert(globalClass->Body().begin(), node);
    node->SetParent(globalClass);
}

void GlobalClassHandler::SetupInitializerBlock(ArenaVector<ArenaVector<ir::Statement *>> &&initializerBlock,
                                               ir::ClassDefinition *globalClass)
{
    if (globalProgram_->IsDeclarationModule() || initializerBlock.empty()) {
        return;
    }

    ArenaVector<ir::Statement *> blockStmts(allocator_->Adapter());
    for (auto iBlock : initializerBlock) {
        if (iBlock.empty()) {
            continue;
        }
        blockStmts.emplace_back(
            NodeAllocator::ForceSetParent<ir::BlockStatement>(allocator_, allocator_, std::move(iBlock)));
    }

    // Note: cannot use the all same name for every stdlib package.
    std::string moduleName = std::string(globalProgram_->ModuleName());
    std::replace(moduleName.begin(), moduleName.end(), '.', '_');
    util::UString initializerBlockName =
        util::UString {std::string(compiler::Signatures::INITIALIZER_BLOCK_INIT) + moduleName, allocator_};
    ir::MethodDefinition *initializerBlockInit =
        CreateGlobalMethod(initializerBlockName.View().Utf8(), std::move(blockStmts));
    InsertInGlobal(globalClass, initializerBlockInit);
    AddInitCallToStaticBlock(globalClass, initializerBlockInit);
}

void GlobalClassHandler::SetupGlobalMethods(ArenaVector<ir::Statement *> &&initStatements,
                                            ir::ClassDefinition *globalClass, bool isDeclare)
{
    if (isDeclare) {
        return;
    }

    AddInitStatementsToStaticBlock(globalClass, std::move(initStatements));
}

void GlobalClassHandler::MergeNamespace(ArenaVector<ir::ETSModule *> &namespaces, parser::Program *program)
{
    auto *parser = program->VarBinder()->GetContext()->parser->AsETSParser();
    ArenaUnorderedMap<util::StringView, ir::ETSModule *> nsMap {program->Allocator()->Adapter()};
    for (auto it = namespaces.begin(); it != namespaces.end();) {
        auto *ns = *it;
        auto res = nsMap.find(ns->Ident()->Name());
        if (res != nsMap.end()) {
            if (res->second->Modifiers() != ns->Modifiers()) {
                parser->LogError(diagnostic::NAMESPACE_MERGE_ERROR, {ns->Ident()->Name().Mutf8()}, ns->Start());
            }
            if (!res->second->Annotations().empty() && !ns->Annotations().empty()) {
                parser->LogError(diagnostic::NAMESPACE_ANNOTATION_CONFLICT, {ns->Ident()->Name().Mutf8()}, ns->Start());
            } else if (!ns->Annotations().empty()) {
                ES2PANDA_ASSERT(res->second->Annotations().empty());
                res->second->SetAnnotations(std::move(ns->AnnotationsForUpdate()));
            }
            res->second->AddStatements(ns->Statements());
            namespaces.erase(it);
        } else {
            nsMap.insert({ns->Ident()->Name(), ns});
            ++it;
        }
    }
}

ArenaVector<ir::Statement *> GlobalClassHandler::TransformNamespaces(ArenaVector<ir::ETSModule *> &namespaces)
{
    ArenaVector<ir::Statement *> classDecls {allocator_->Adapter()};
    MergeNamespace(namespaces, globalProgram_);
    for (auto ns : namespaces) {
        classDecls.emplace_back(TransformNamespace(ns));
    }
    return classDecls;
}

void GlobalClassHandler::TransformBrokenNamespace(ir::AstNode *node)
{
    node->TransformChildrenRecursively(
        // clang-format off
        // CC-OFFNXT(G.FMT.14-CPP) project code style
        [this](ir::AstNode *child) -> ir::AstNode* {
            if (child->IsETSModule() && child->AsETSModule()->IsNamespace()) {
                auto res = TransformNamespace(child->AsETSModule());
                res->SetParent(child->Parent());
                return res;
            }
            return child;
        },
        // clang-format on
        "TransformBrokenNamespace");
}

ir::ClassDeclaration *GlobalClassHandler::TransformNamespace(ir::ETSModule *ns)
{
    ir::ClassDeclaration *const globalDecl = CreateTransformedClass(ns);
    ES2PANDA_ASSERT(globalDecl != nullptr);
    ir::ClassDefinition *const globalClass = globalDecl->Definition();

    ArenaVector<GlobalStmts> immediateInitializers(allocator_->Adapter());
    ArenaVector<GlobalStmts> initializerBlock(allocator_->Adapter());
    ArenaVector<ir::ETSModule *> namespaces(allocator_->Adapter());
    auto &body = ns->StatementsForUpdates();
    for (auto *statement : body) {
        statement->Iterate([this](ir::AstNode *node) { AddStaticBlockToClass(node); });
    }
    auto stmts = CollectProgramGlobalStatements(body, globalClass, ns);
    immediateInitializers.emplace_back(GlobalStmts {globalProgram_, std::move(stmts.immediateInit)});
    for (auto &initBlock : stmts.initializerBlocks) {
        initializerBlock.emplace_back(GlobalStmts {globalProgram_, std::move(initBlock)});
    }
    AddStaticBlockToClass(globalClass);
    const ModuleDependencies md(ArenaVector<parser::Program *>(allocator_->Adapter()),
                                ArenaUnorderedSet<parser::Program *>(allocator_->Adapter()));
    auto immediateInitStatements = FormInitMethodStatements(&md, std::move(immediateInitializers));
    auto initializerBlockStatements = FormInitStaticBlockMethodStatements(&md, std::move(initializerBlock));
    SetupGlobalMethods(std::move(immediateInitStatements), globalClass, ns->IsDeclare());
    SetupInitializerBlock(std::move(initializerBlockStatements), globalClass);

    // remove namespaceDecl from orginal node
    auto end = std::remove_if(body.begin(), body.end(), [&namespaces](ir::AstNode *node) {
        if (node->IsETSModule()) {
            namespaces.emplace_back(node->AsETSModule());
            return true;
        }
        return false;
    });
    body.erase(end, body.end());
    auto globalClasses = TransformNamespaces(namespaces);
    for (auto *cls : globalClasses) {
        globalClass->EmplaceBody(cls);
        cls->SetParent(globalClass);
        CollectNamespaceExportedClasses(globalProgram_, cls->AsClassDeclaration()->Definition());
    }

    // Add rest statement, such as type declaration
    for (auto *statement : body) {
        globalClass->EmplaceBody(statement);
        statement->SetParent(globalClass);
    }
    body.clear();
    return globalDecl;
}

void GlobalClassHandler::CollectProgramGlobalClasses(ArenaVector<ir::ETSModule *> namespaces)
{
    auto classDecls = TransformNamespaces(namespaces);
    globalProgram_->Ast()->AddStatements(classDecls);
    for (auto cls : classDecls) {
        cls->SetParent(globalProgram_->Ast());
        CollectNamespaceExportedClasses(globalProgram_, cls->AsClassDeclaration()->Definition());
    }
}

void GlobalClassHandler::CheckPackageMultiInitializerBlock(
    util::StringView packageName, const ArenaVector<ArenaVector<ir::Statement *>> &initializerBlocks)
{
    if (initializerBlocks.empty()) {
        return;
    }

    if (packageInitializerBlockCount_.count(packageName) != 0) {
        parser_->LogError(diagnostic::PACKAGE_MULTIPLE_STATIC_BLOCK, {}, initializerBlocks[0][0]->Start());
    } else {
        packageInitializerBlockCount_.insert(packageName);
    }
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP) solid logic
void GlobalClassHandler::SetupGlobalClass(const ArenaVector<parser::Program *> &programs,
                                          const ModuleDependencies *moduleDependencies)
{
    if (programs.empty()) {
        return;
    }
    if (globalProgram_->GlobalClass() != nullptr) {
        return;
    }

    ArenaUnorderedSet<util::StringView> packageInitializerBlockCount(allocator_->Adapter());
    ir::ClassDeclaration *const globalDecl = CreateGlobalClass(globalProgram_);
    ES2PANDA_ASSERT(globalDecl != nullptr);
    ir::ClassDefinition *const globalClass = globalDecl->Definition();

    // NOTE(vpukhov): a clash inside program list is possible
    ES2PANDA_ASSERT(globalProgram_->IsPackage() || programs.size() == 1);

    ArenaVector<GlobalStmts> immediateInitializers(allocator_->Adapter());
    ArenaVector<GlobalStmts> initializerBlock(allocator_->Adapter());
    ArenaVector<ir::ETSModule *> namespaces(allocator_->Adapter());

    for (auto const program : programs) {
        program->Ast()->IterateRecursively([this](ir::AstNode *node) { AddStaticBlockToClass(node); });
        auto &body = program->Ast()->StatementsForUpdates();
        auto stmts = CollectProgramGlobalStatements(body, globalClass, program->Ast());
        auto end = std::remove_if(body.begin(), body.end(), [&namespaces](ir::AstNode *node) {
            if (node->IsETSModule() && node->AsETSModule()->IsNamespace()) {
                namespaces.emplace_back(node->AsETSModule());
                return true;
            }
            return false;
        });
        body.erase(end, body.end());
        CheckPackageMultiInitializerBlock(program->ModuleName(), stmts.initializerBlocks);
        immediateInitializers.emplace_back(GlobalStmts {program, std::move(stmts.immediateInit)});
        for (auto &initBlock : stmts.initializerBlocks) {
            initializerBlock.emplace_back(GlobalStmts {program, std::move(initBlock)});
        }
        program->SetGlobalClass(globalClass);
    }

    globalProgram_->Ast()->AddStatement(globalDecl);
    globalDecl->SetParent(globalProgram_->Ast());
    globalClass->SetGlobalInitialized();

    CollectProgramGlobalClasses(namespaces);
    TransformBrokenNamespace(globalProgram_->Ast());
    auto initializerBlockStmts = FormInitStaticBlockMethodStatements(moduleDependencies, std::move(initializerBlock));

    CollectExportedClasses(globalProgram_, globalClass, globalProgram_->Ast()->Statements());

    // NOTE(vpukhov): stdlib checks are to be removed - do not extend the existing logic
    if (globalProgram_->Kind() != parser::ScriptKind::STDLIB) {
        AddStaticBlockToClass(globalClass);
        if (!util::Helpers::IsStdLib(globalProgram_)) {
            auto initStatements = FormInitMethodStatements(moduleDependencies, std::move(immediateInitializers));
            SetupGlobalMethods(std::move(initStatements));
        }
    }
    SetupInitializerBlock(std::move(initializerBlockStmts), globalClass);
}

static std::pair<lexer::SourcePosition, lexer::SourcePosition> GetBoundInBody(parser::Program *program,
                                                                              ir::BlockStatement *body)
{
    auto minBound = lexer::SourcePosition(program);
    auto maxBound = lexer::SourcePosition(program);
    if (!body->Statements().empty()) {
        minBound = body->Statements().front()->Start();
        maxBound = body->Statements().front()->End();
        for (const auto &stmt : body->Statements()) {
            if (stmt->Start().index < minBound.index) {
                minBound = stmt->Start();
            }
            if (stmt->End().index > maxBound.index) {
                maxBound = stmt->End();
            }
        }
    }
    return std::make_pair(minBound, maxBound);
}

ir::MethodDefinition *GlobalClassHandler::CreateGlobalMethod(const std::string_view name,
                                                             ArenaVector<ir::Statement *> &&statements)
{
    const auto functionFlags = ir::ScriptFunctionFlags::NONE;
    auto functionModifiers = ir::ModifierFlags::STATIC | ir::ModifierFlags::PUBLIC;
    auto ident = NodeAllocator::Alloc<ir::Identifier>(allocator_, name, allocator_);
    ES2PANDA_ASSERT(ident != nullptr);
    auto body = NodeAllocator::ForceSetParent<ir::BlockStatement>(allocator_, allocator_, std::move(statements));
    auto funcSignature = ir::FunctionSignature(nullptr, ArenaVector<ir::Expression *>(allocator_->Adapter()), nullptr);

    auto *func = NodeAllocator::Alloc<ir::ScriptFunction>(
        allocator_, allocator_,
        ir::ScriptFunction::ScriptFunctionData {
            body, std::move(funcSignature), functionFlags, {}, Language(Language::Id::ETS)});
    ES2PANDA_ASSERT(func != nullptr);
    func->SetIdent(ident);
    func->AddModifier(functionModifiers);

    auto *funcExpr = NodeAllocator::Alloc<ir::FunctionExpression>(allocator_, func);
    auto *identClone = ident->Clone(allocator_, nullptr);
    ES2PANDA_ASSERT(identClone != nullptr);
    auto *methodDef = NodeAllocator::Alloc<ir::MethodDefinition>(allocator_, ir::MethodDefinitionKind::METHOD,
                                                                 identClone->AsExpression(), funcExpr,
                                                                 functionModifiers, allocator_, false);
    ES2PANDA_ASSERT(methodDef != nullptr);

    auto [minBound, maxBound] = GetBoundInBody(globalProgram_, body);
    body->SetRange({minBound, maxBound});
    func->SetRange({minBound, maxBound});
    funcExpr->SetRange({minBound, maxBound});
    methodDef->SetRange({minBound, maxBound});

    return methodDef;
}

void GlobalClassHandler::AddInitializerBlockToStaticBlock(ir::ClassDefinition *globalClass,
                                                          ArenaVector<ir::Statement *> &&initializerBlocks)
{
    auto &globalBody = globalClass->Body();
    auto maybeStaticBlock = std::find_if(globalBody.begin(), globalBody.end(),
                                         [](ir::AstNode *node) { return node->IsClassStaticBlock(); });
    ES2PANDA_ASSERT(maybeStaticBlock != globalBody.end());

    auto *staticBlock = (*maybeStaticBlock)->AsClassStaticBlock();
    auto *initializerStmts =
        NodeAllocator::ForceSetParent<ir::BlockStatement>(allocator_, allocator_, std::move(initializerBlocks));
    ES2PANDA_ASSERT(initializerStmts != nullptr);
    auto *blockBody = staticBlock->Function()->Body()->AsBlockStatement();
    initializerStmts->SetParent(blockBody);
    blockBody->AddStatement(initializerStmts);
}

void GlobalClassHandler::AddInitCallToStaticBlock(ir::ClassDefinition *globalClass, ir::MethodDefinition *initMethod)
{
    ES2PANDA_ASSERT(initMethod != nullptr);

    auto &globalBody = globalClass->Body();
    auto maybeStaticBlock = std::find_if(globalBody.begin(), globalBody.end(),
                                         [](ir::AstNode *node) { return node->IsClassStaticBlock(); });
    ES2PANDA_ASSERT(maybeStaticBlock != globalBody.end());

    auto *staticBlock = (*maybeStaticBlock)->AsClassStaticBlock();
    ES2PANDA_ASSERT(initMethod->Id() != nullptr);
    auto *callee = RefIdent(initMethod->Id()->Name());

    auto *const callExpr = NodeAllocator::Alloc<ir::CallExpression>(
        allocator_, callee, ArenaVector<ir::Expression *>(allocator_->Adapter()), nullptr, false, false);

    auto *blockBody = staticBlock->Function()->Body()->AsBlockStatement();
    auto exprStmt = NodeAllocator::Alloc<ir::ExpressionStatement>(allocator_, callExpr);
    ES2PANDA_ASSERT(exprStmt != nullptr);
    exprStmt->SetParent(blockBody);
    blockBody->AddStatement(exprStmt);
}

void GlobalClassHandler::AddInitStatementsToStaticBlock(ir::ClassDefinition *globalClass,
                                                        ArenaVector<ir::Statement *> &&initStatements)
{
    auto &globalBody = globalClass->Body();
    auto maybeStaticBlock = std::find_if(globalBody.begin(), globalBody.end(),
                                         [](ir::AstNode *node) { return node->IsClassStaticBlock(); });
    ES2PANDA_ASSERT(maybeStaticBlock != globalBody.end());

    auto *staticBlock = (*maybeStaticBlock)->AsClassStaticBlock();

    auto *blockBody = staticBlock->Function()->Body()->AsBlockStatement();
    for (auto &stmt : initStatements) {
        blockBody->AddStatement(stmt);
    }

    auto [minBound, maxBound] = GetBoundInBody(globalProgram_, blockBody);
    blockBody->SetRange({minBound, maxBound});
    staticBlock->Function()->SetRange({minBound, maxBound});
    staticBlock->Value()->SetRange({minBound, maxBound});
    staticBlock->SetRange({minBound, maxBound});

    globalClass->SetInitInCctor();
}

ir::Identifier *GlobalClassHandler::RefIdent(const util::StringView &name)
{
    auto *const callee = NodeAllocator::Alloc<ir::Identifier>(allocator_, name, allocator_);
    return callee;
}

ArenaVector<ArenaVector<ir::Statement *>> GlobalClassHandler::FormInitStaticBlockMethodStatements(
    const ModuleDependencies *moduleDependencies, ArenaVector<GlobalStmts> &&initStatements)
{
    // Note: will create method body for initializer block one by one, don't merge them.
    ArenaVector<ArenaVector<ir::Statement *>> staticBlocks(allocator_->Adapter());
    for (const auto &[p, ps] : initStatements) {
        ArenaVector<ir::Statement *> statements(allocator_->Adapter());
        if (!util::Helpers::IsStdLib(globalProgram_) && moduleDependencies != nullptr) {
            FormDependentInitTriggers(statements, moduleDependencies);
        }
        statements.insert(statements.end(), ps.begin(), ps.end());
        std::for_each(statements.begin(), statements.end(), [](auto stmt) { stmt->SetParent(nullptr); });
        staticBlocks.emplace_back(std::move(statements));
    }
    return staticBlocks;
}

ArenaVector<ir::Statement *> GlobalClassHandler::FormInitMethodStatements(const ModuleDependencies *moduleDependencies,
                                                                          ArenaVector<GlobalStmts> &&initStatements)
{
    ArenaVector<ir::Statement *> statements(allocator_->Adapter());
    if (!util::Helpers::IsStdLib(globalProgram_) && moduleDependencies != nullptr) {
        FormDependentInitTriggers(statements, moduleDependencies);
    }
    for (const auto &[p, ps] : initStatements) {
        statements.insert(statements.end(), ps.begin(), ps.end());
    }
    for (auto st : statements) {
        TransformBrokenNamespace(st);
        st->SetParent(nullptr);
    }
    return statements;
}

void GlobalClassHandler::FormDependentInitTriggers(ArenaVector<ir::Statement *> &statements,
                                                   const ModuleDependencies *moduleDependencies)
{
    for (const auto module : moduleDependencies->first) {
        ArenaVector<ir::Expression *> params(allocator_->Adapter());
        auto moduleStr = util::UString {
            module->ModuleInfo().modulePrefix.Mutf8().append(compiler::Signatures::ETS_GLOBAL), allocator_};
        auto moduleName = NodeAllocator::Alloc<ir::StringLiteral>(allocator_, moduleStr.View());
        params.emplace_back(moduleName);
        // Note (daizihan): #27086, we should not use stringLiteral as argument in ETSIntrinsicNode, should be TypeNode.
        auto moduleNode = NodeAllocator::Alloc<ir::ETSIntrinsicNode>(allocator_, ir::IntrinsicNodeType::TYPE_REFERENCE,
                                                                     std::move(params));
        auto initIdent =
            NodeAllocator::Alloc<ir::Identifier>(allocator_, compiler::Signatures::CLASS_INITIALIZE_METHOD, allocator_);
        auto *callee = NodeAllocator::Alloc<ir::MemberExpression>(
            allocator_, moduleNode, initIdent, ir::MemberExpressionKind::PROPERTY_ACCESS, false, false);
        auto *const callExpr = NodeAllocator::Alloc<ir::CallExpression>(
            allocator_, callee, ArenaVector<ir::Expression *>(allocator_->Adapter()), nullptr, false, false);
        auto stmt = NodeAllocator::Alloc<ir::ExpressionStatement>(allocator_, callExpr);
        statements.emplace_back(stmt);
    }
}

ir::ClassStaticBlock *GlobalClassHandler::CreateStaticBlock(ir::ClassDefinition *classDef)
{
    bool hasStaticField = false;
    for (const auto *prop : classDef->Body()) {
        if (prop->IsClassStaticBlock()) {
            return nullptr;
        }
        if (prop->IsClassProperty() && prop->AsClassProperty()->IsStatic()) {
            hasStaticField = true;
        }
    }

    if (!hasStaticField && !classDef->IsModule()) {
        return nullptr;
    }

    ArenaVector<ir::Expression *> params(allocator_->Adapter());

    auto *id = NodeAllocator::Alloc<ir::Identifier>(allocator_, compiler::Signatures::CCTOR, allocator_);

    ArenaVector<ir::Statement *> statements(allocator_->Adapter());

    auto *body = NodeAllocator::Alloc<ir::BlockStatement>(allocator_, allocator_, std::move(statements));
    auto *func = NodeAllocator::Alloc<ir::ScriptFunction>(
        allocator_, allocator_,
        ir::ScriptFunction::ScriptFunctionData {body, ir::FunctionSignature(nullptr, std::move(params), nullptr),
                                                ir::ScriptFunctionFlags::STATIC_BLOCK | ir::ScriptFunctionFlags::HIDDEN,
                                                ir::ModifierFlags::STATIC, Language(Language::Id::ETS)});
    ES2PANDA_ASSERT(func != nullptr);
    func->SetIdent(id);

    auto *funcExpr = NodeAllocator::Alloc<ir::FunctionExpression>(allocator_, func);
    auto *staticBlock = NodeAllocator::Alloc<ir::ClassStaticBlock>(allocator_, funcExpr, allocator_);
    ES2PANDA_ASSERT(staticBlock != nullptr);
    staticBlock->AddModifier(ir::ModifierFlags::STATIC);
    staticBlock->SetRange({classDef->Start(), classDef->Start()});
    return staticBlock;
}

GlobalDeclTransformer::ResultT GlobalClassHandler::CollectProgramGlobalStatements(ArenaVector<ir::Statement *> &stmts,
                                                                                  ir::ClassDefinition *classDef,
                                                                                  ir::Statement const *stmt)
{
    auto globalDecl = GlobalDeclTransformer(allocator_, stmt, parser_);
    auto statements = globalDecl.TransformStatements(stmts);
    if (globalDecl.IsMultiInitializer() && stmt->IsETSModule() && stmt->AsETSModule()->IsNamespace()) {
        auto fristStaticBlock =
            std::find_if(stmts.cbegin(), stmts.cend(), [](auto *prop) { return prop->IsClassStaticBlock(); });
        ES2PANDA_ASSERT(fristStaticBlock != stmts.cend());
        parser_->LogError(diagnostic::MULTIPLE_STATIC_BLOCK, {}, (*fristStaticBlock)->Start());
    }

    if (stmt->IsETSModule() && !stmt->AsETSModule()->IsNamespace() && stmt->AsETSModule()->Program()->IsPackage()) {
        const auto &immInitsOfPackage = statements.immediateInit;
        std::for_each(immInitsOfPackage.begin(), immInitsOfPackage.end(), [this](auto immInit) {
            if (immInit->IsExpressionStatement() &&
                !immInit->AsExpressionStatement()->GetExpression()->IsAssignmentExpression()) {
                this->parser_->LogError(diagnostic::INVALID_PACKAGE_TOP_LEVEL_STMT, {}, immInit->Start());
            }
        });
    }
    classDef->AddProperties(util::Helpers::ConvertVector<ir::AstNode>(statements.classProperties));
    /*
    initializers consists of two parts:
    immediate initializers and initializer blocks, the former should be executed firstly.

    Example code:
        namespace NS {
            let a: number;
            let b: number = 2;
            static {
                a = 1;
                b = 0;
            }
        }

    In the example code, execute order will be: b = 2, a = 1, b = 0;
    */
    globalDecl.FilterDeclarations(stmts);
    return statements;
}

ir::ClassDeclaration *GlobalClassHandler::CreateGlobalClass(const parser::Program *const globalProgram)
{
    const auto rangeToStartOfFile =
        lexer::SourceRange(lexer::SourcePosition(globalProgram), lexer::SourcePosition(globalProgram));
    auto *ident = NodeAllocator::Alloc<ir::Identifier>(allocator_, compiler::Signatures::ETS_GLOBAL, allocator_);
    ES2PANDA_ASSERT(ident != nullptr);
    ident->SetRange(rangeToStartOfFile);
    auto lang =
        globalProgram->IsDeclForDynamicStaticInterop() ? Language(Language::Id::JS) : Language(Language::Id::ETS);
    auto *classDef = NodeAllocator::Alloc<ir::ClassDefinition>(
        allocator_, allocator_, ident, ir::ClassDefinitionModifiers::GLOBAL, ir::ModifierFlags::ABSTRACT, lang);
    ES2PANDA_ASSERT(classDef != nullptr);
    classDef->SetRange(rangeToStartOfFile);
    auto *classDecl = NodeAllocator::Alloc<ir::ClassDeclaration>(allocator_, classDef, allocator_);
    ES2PANDA_ASSERT(classDecl != nullptr);
    classDecl->SetRange(rangeToStartOfFile);

    return classDecl;
}

static bool HasMethod(ir::ClassDefinition const *cls, const std::string_view name)
{
    return std::any_of(cls->Body().begin(), cls->Body().end(), [name](ir::AstNode const *node) {
        return node->IsMethodDefinition() && node->AsMethodDefinition()->Key()->AsIdentifier()->Name().Is(name);
    });
}

void GlobalClassHandler::SetupGlobalMethods(ArenaVector<ir::Statement *> &&initStatements)
{
    ir::ClassDefinition *const globalClass = globalProgram_->GlobalClass();
    SetupGlobalMethods(std::move(initStatements), globalClass, globalProgram_->IsDeclarationModule());

    if (globalProgram_->IsSeparateModule() && !HasMethod(globalClass, compiler::Signatures::MAIN)) {
        ir::MethodDefinition *mainMethod =
            CreateGlobalMethod(compiler::Signatures::MAIN, ArenaVector<ir::Statement *>(allocator_->Adapter()));
        InsertInGlobal(globalClass, mainMethod);
    }
}

}  // namespace ark::es2panda::compiler
