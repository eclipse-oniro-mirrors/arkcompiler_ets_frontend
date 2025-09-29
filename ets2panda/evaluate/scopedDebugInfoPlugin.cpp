/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "evaluate/helpers.h"
#include "evaluate/entityDeclarator-inl.h"
#include "evaluate/scopedDebugInfoPlugin-inl.h"
#include "evaluate/debugInfoDeserialization/debugInfoDeserializer.h"

#include "checker/ETSchecker.h"
#include "parser/program/program.h"
#include "compiler/lowering/phase.h"
#include "compiler/lowering/util.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "ir/statements/blockStatement.h"

namespace ark::es2panda::evaluate {

namespace {

ir::VariableDeclaration *CreateVariableDeclaration(checker::ETSChecker *checker, ir::Identifier *ident,
                                                   ir::Expression *init)
{
    auto *declarator = checker->AllocNode<ir::VariableDeclarator>(ir::VariableDeclaratorFlag::CONST, ident, init);
    ES2PANDA_ASSERT(declarator != nullptr);

    ArenaVector<ir::VariableDeclarator *> declarators(1, declarator, checker->Allocator()->Adapter());
    auto *declaration = checker->AllocNode<ir::VariableDeclaration>(
        ir::VariableDeclaration::VariableDeclarationKind::CONST, checker->Allocator(), std::move(declarators));

    declarator->SetParent(declaration);
    return declaration;
}

/**
 * @brief Break function's last statement into variable declaration and return statement.
 * Hence we ensure that expression will return result, and local variables
 * could be updated by inserting `DebuggerAPI.setLocal<>` calls between result and return.
 * @param checker used for allocation purposes only.
 * @param methodName used for returned variable name generation.
 * @param lastStatement function's last statement to break.
 * @returns pair of created AST nodes for variable declaration and return statement.
 */
std::pair<ir::VariableDeclaration *, ir::ReturnStatement *> BreakLastStatement(checker::ETSChecker *checker,
                                                                               util::StringView methodName,
                                                                               ir::ExpressionStatement *lastStatement)
{
    static constexpr std::string_view GENERATED_VAR_SUFFIX = "_generated_var";

    ES2PANDA_ASSERT(checker);
    ES2PANDA_ASSERT(lastStatement);
    auto *allocator = checker->Allocator();

    auto returnVariableNameView = [methodName, allocator]() {
        std::stringstream ss;
        ss << methodName << GENERATED_VAR_SUFFIX;
        util::UString variableName(ss.str(), allocator);
        return variableName.View();
    }();
    auto *variableIdent = checker->AllocNode<ir::Identifier>(returnVariableNameView, allocator);
    auto *exprInit = lastStatement->AsExpressionStatement()->GetExpression();
    auto *variableDeclaration = CreateVariableDeclaration(checker, variableIdent, exprInit);

    auto *returnStatement = checker->AllocNode<ir::ReturnStatement>(variableIdent->Clone(allocator, nullptr));

    // Unattach previous statement.
    lastStatement->SetParent(nullptr);

    return std::make_pair(variableDeclaration, returnStatement);
}

}  // namespace

ScopedDebugInfoPlugin::ScopedDebugInfoPlugin(public_lib::Context *ctx)
    : e2pctx_(ctx),
      checker_(e2pctx_->GetChecker()->AsETSChecker()),
      context_(*e2pctx_->config->options),
      irCheckHelper_(checker_, e2pctx_->parserProgram->VarBinder()->AsETSBinder()),
      debugInfoStorage_(*e2pctx_->config->options, checker_->Allocator()),
      debugInfoDeserializer_(*this),
      pathResolver_(debugInfoStorage_),
      prologueEpilogueMap_(checker_->Allocator()->Adapter()),
      proxyProgramsCache_(checker_->Allocator()),
      entityDeclarator_(*this)
{
    ES2PANDA_ASSERT(e2pctx_->parserProgram);

    auto isContextValid = debugInfoStorage_.FillEvaluateContext(context_);
    if (!isContextValid) {
        LOG(FATAL, ES2PANDA) << "Can't create evaluate context" << std::endl;
    }

    CreateContextPrograms();
}

void ScopedDebugInfoPlugin::PreCheck()
{
    irCheckHelper_.PreCheck();

    // Find evaluation method after parse and before any checks.
    context_.FindEvaluationMethod(GetEvaluatedExpressionProgram());
}

void ScopedDebugInfoPlugin::PostCheck()
{
    ES2PANDA_ASSERT(prologueEpilogueMap_.empty());

    [[maybe_unused]] auto inserted = InsertReturnStatement();
    LOG(DEBUG, ES2PANDA) << "Evaluation method will return: " << std::boolalpha << inserted << std::noboolalpha;
}

bool ScopedDebugInfoPlugin::InsertReturnStatement()
{
    auto *lastStatement = context_.lastStatement;
    if (lastStatement == nullptr) {
        // Last evaluation statement cannot return a value.
        return false;
    }
    auto *returnType = lastStatement->GetExpression()->TsType();
    if (returnType == nullptr || !returnType->IsETSPrimitiveType()) {
        // NOTE(dslynko): currently expression evaluation supports only primitives.
        // In future this condition might be replaced with `returnType is not void`.
        return false;
    }

    // As an example, the code below will transform
    // ```
    // localVar += 1                        // This expression returns new value of `localVar`.
    // DebuggerAPI.setLocalInt(0, localVar) // Already generated by plugin.
    // ```
    // into
    // ```
    // const eval_file_generated_var = (localVar += 1)
    // DebuggerAPI.setLocalInt(0, localVar)
    // return eval_file_generated_var
    // ```
    // which will also modify method signature's return type.

    auto *evalMethodStatements = context_.methodStatements;
    auto &statementsList = evalMethodStatements->StatementsForUpdates();
    // Omit the emplaced `DebuggerAPI.setLocal<>` calls and find the original last statement.
    auto lastStatementIter = std::find(statementsList.rbegin(), statementsList.rend(), lastStatement);
    ES2PANDA_ASSERT(lastStatementIter != statementsList.rend());

    // Break the last user's statement into variable declaration and return statement.
    auto *scope = compiler::NearestScope(lastStatement);
    auto *scriptFunction = evalMethodStatements->Parent()->AsScriptFunction();
    auto [variableDeclaration, returnStatement] =
        BreakLastStatement(checker_, scriptFunction->Id()->Name(), lastStatement);

    // Attach new nodes to statements block.
    variableDeclaration->SetParent(evalMethodStatements);
    *lastStatementIter = variableDeclaration;
    returnStatement->SetParent(evalMethodStatements);
    statementsList.emplace_back(returnStatement);

    scriptFunction->AddFlag(ir::ScriptFunctionFlags::HAS_RETURN);
    auto *signature = scriptFunction->Signature();
    signature->AddSignatureFlag(checker::SignatureFlags::NEED_RETURN_TYPE);
    signature->SetReturnType(returnType);

    auto newNodeInitializer = [this, scope](ir::AstNode *node) {
        auto *varBinder = GetETSBinder();
        helpers::DoScopedAction(checker_, varBinder, GetEvaluatedExpressionProgram(), scope, nullptr,
                                [this, varBinder, scope, node]() {
                                    compiler::InitScopesPhaseETS::RunExternalNode(node, varBinder);
                                    varBinder->HandleCustomNodes(node);
                                    varBinder->ResolveReferencesForScope(node, scope);
                                    node->Check(checker_);
                                });
    };
    newNodeInitializer(variableDeclaration);
    newNodeInitializer(returnStatement);

    return true;
}

void ScopedDebugInfoPlugin::AddPrologueEpilogue(ir::BlockStatement *block)
{
    auto iter = prologueEpilogueMap_.find(block);
    if (iter == prologueEpilogueMap_.end()) {
        return;
    }

    // Prepend prologue.
    auto &statements = block->StatementsForUpdates();
    for (auto *stmt : iter->second.first) {
        statements.insert(statements.begin(), stmt);
    }

    // Append epilogue.
    for (auto *stmt : iter->second.second) {
        statements.emplace_back(stmt);
    }

    prologueEpilogueMap_.erase(iter);
}

varbinder::Variable *ScopedDebugInfoPlugin::FindIdentifier(ir::Identifier *ident)
{
    ES2PANDA_ASSERT(ident);

    helpers::SafeStateScope s(checker_, GetETSBinder());

    auto *var = FindLocalVariable(ident);
    if (var != nullptr) {
        return var;
    }
    var = FindGlobalVariable(ident);
    if (var != nullptr) {
        return var;
    }
    var = FindClass(ident);
    if (var != nullptr) {
        return var;
    }
    return FindGlobalFunction(ident);
}

varbinder::Variable *ScopedDebugInfoPlugin::FindClass(ir::Identifier *ident)
{
    // The following algorithm is used:
    // - Search for `import * as B from "C"` statements.
    //   - If found, [Not implemented yet]
    //   - Else, proceed.
    // - Search classes which defined in the context file:
    //   - If found, recreate its structure and return.
    //   - Else, proceed.
    // - Search through the imported entities extracted from imports/exports table:
    //   - If the class was found, create parser::Program corresponding for the import source,
    //     where the class could be recreated.
    //   - Else, return nullptr.

    // NOTE: support "import * as X".

    ES2PANDA_ASSERT(ident);
    LOG(DEBUG, ES2PANDA) << "ScopedDebugInfoPlugin: FindClass " << ident->Name();

    auto *importerProgram = GetETSBinder()->Program();
    const auto &identName = ident->Name();

    // Search "import * as B" statements.
    // NOTE: separate this into a method.
    auto importPath = pathResolver_.FindNamedImportAll(context_.sourceFilePath.Utf8(), identName.Utf8());
    if (!importPath.empty()) {
        ES2PANDA_UNREACHABLE();
        return nullptr;
    }

    // Search in the context file.
    auto classId = debugInfoStorage_.FindClass(context_.sourceFilePath.Utf8(), identName.Utf8());
    if (classId.IsValid()) {
        return entityDeclarator_.ImportGlobalEntity(
            context_.sourceFilePath, identName, importerProgram, identName,
            [classId](auto *deserializer, auto *program, auto declSourcePath, auto declName) {
                return deserializer->CreateIrClass(classId, program, declSourcePath, declName);
            });
    }

    // Search in imported entities.
    auto optFoundEntity = pathResolver_.FindImportedEntity(context_.sourceFilePath.Utf8(), identName.Utf8());
    if (!optFoundEntity) {
        return nullptr;
    }

    const auto &[entitySourceFile, entitySourceName] = optFoundEntity.value();

    classId = debugInfoStorage_.FindClass(entitySourceFile, entitySourceName);
    if (!classId.IsValid()) {
        // The entity is not a class.
        return nullptr;
    }

    // Must pass the name of class as declared in the found file.
    return entityDeclarator_.ImportGlobalEntity(
        entitySourceFile, entitySourceName, importerProgram, identName,
        [classId](auto *deserializer, auto *program, auto declSourcePath, auto declName) {
            return deserializer->CreateIrClass(classId, program, declSourcePath, declName);
        });
}

varbinder::Variable *ScopedDebugInfoPlugin::FindGlobalFunction(ir::Identifier *ident)
{
    // Correct overload resolution requires us to create all reachable functions with the given name,
    // so that Checker later could choose the correct one.
    ES2PANDA_ASSERT(ident);
    LOG(DEBUG, ES2PANDA) << "ScopedDebugInfoPlugin: FindGlobalFunction " << ident->Name();

    auto *allocator = Allocator();

    auto *importerProgram = GetETSBinder()->Program();
    auto identName = ident->Name();

    ArenaVector<std::pair<parser::Program *, ArenaVector<ir::AstNode *>>> createdMethods(allocator->Adapter());

    // Build every global function from the context file.
    createdMethods.emplace_back(GetProgram(context_.sourceFilePath), ArenaVector<ir::AstNode *>(allocator->Adapter()));
    auto &fromContextFile = createdMethods.back().second;

    auto *var = entityDeclarator_.ImportGlobalEntity(
        context_.sourceFilePath, identName, importerProgram, identName,
        [&fromContextFile](auto *deserializer, auto *program, auto declSourcePath, auto declName) {
            return deserializer->CreateIrGlobalMethods(fromContextFile, program, declSourcePath, declName);
        });

    // Then search in imports.
    ArenaVector<EntityInfo> importedFunctions(allocator->Adapter());
    pathResolver_.FindImportedFunctions(importedFunctions, context_.sourceFilePath.Utf8(), identName.Utf8());

    // Build all the found functions.
    for (const auto &[funcSourceFile, funcSourceName] : importedFunctions) {
        createdMethods.emplace_back(GetProgram(funcSourceFile), ArenaVector<ir::AstNode *>(allocator->Adapter()));
        auto &fromImported = createdMethods.back().second;

        auto *importedVar = entityDeclarator_.ImportGlobalEntity(
            funcSourceFile, funcSourceName, importerProgram, identName,
            [&fromImported](auto *deserializer, auto *program, auto declSourcePath, auto declName) {
                return deserializer->CreateIrGlobalMethods(fromImported, program, declSourcePath, declName);
            });
        if (importedVar != nullptr) {
            ES2PANDA_ASSERT(var == nullptr || var == importedVar);
            var = importedVar;
        }
    }

    // Run Checker only after all functions are created, so that overloading could work correctly.
    for (auto &[program, methods] : createdMethods) {
        auto *globalClass = program->GlobalClass();
        auto *globalClassScope = program->GlobalClassScope();
        for (auto *method : methods) {
            irCheckHelper_.CheckNewNode(method, globalClassScope, globalClass, program);
        }
    }

    return var;
}

varbinder::Variable *ScopedDebugInfoPlugin::FindGlobalVariable(ir::Identifier *ident)
{
    ES2PANDA_ASSERT(ident);
    LOG(DEBUG, ES2PANDA) << "ScopedDebugInfoPlugin: FindGlobalVariable " << ident->Name();

    auto *importerProgram = GetETSBinder()->Program();
    auto identName = ident->Name();

    // Search in the context file.
    auto *var = entityDeclarator_.ImportGlobalEntity(context_.sourceFilePath, identName, importerProgram, identName,
                                                     &DebugInfoDeserializer::CreateIrGlobalVariable);
    if (var != nullptr) {
        return var;
    }

    // Search within the imports.
    auto optFoundEntity = pathResolver_.FindImportedEntity(context_.sourceFilePath.Utf8(), identName.Utf8());
    if (!optFoundEntity) {
        return nullptr;
    }

    const auto &[entitySourceFile, entitySourceName] = optFoundEntity.value();

    // Search once again, but in the exported source. Must pass the name of entity as declared in the found file.
    return entityDeclarator_.ImportGlobalEntity(entitySourceFile, entitySourceName, importerProgram, identName,
                                                &DebugInfoDeserializer::CreateIrGlobalVariable);
}

varbinder::Variable *ScopedDebugInfoPlugin::FindLocalVariable(ir::Identifier *ident)
{
    ES2PANDA_ASSERT(ident);
    // Search local variables only in evaluation method.
    if (helpers::GetEnclosingBlock(ident) != context_.methodStatements) {
        return nullptr;
    }

    LOG(DEBUG, ES2PANDA) << "ScopedDebugInfoPlugin: FindLocalVariable " << ident->Name();

    // NOTE: verify that function arguments are included.
    const auto &localVariableTable = context_.extractor->GetLocalVariableTable(context_.methodId);
    return debugInfoDeserializer_.CreateIrLocalVariable(ident, localVariableTable, context_.bytecodeOffset);
}

void ScopedDebugInfoPlugin::CreateContextPrograms()
{
    debugInfoStorage_.EnumerateContextFiles([this](auto sourceFilePath, auto, auto, auto moduleName) {
        CreateEmptyProgram(sourceFilePath, moduleName);
        return true;
    });
}

parser::Program *ScopedDebugInfoPlugin::CreateEmptyProgram(std::string_view sourceFilePath, std::string_view moduleName)
{
    parser::Program *program =
        e2pctx_->parser->GetImportPathManager()->SetupProgramForDebugInfoPlugin(sourceFilePath, moduleName);

    proxyProgramsCache_.AddProgram(program);

    return program;
}

parser::Program *ScopedDebugInfoPlugin::GetProgram(util::StringView fileName)
{
    auto *program = proxyProgramsCache_.GetProgram(fileName);
    ES2PANDA_ASSERT(program);
    return program;
}

parser::Program *ScopedDebugInfoPlugin::GetEvaluatedExpressionProgram()
{
    auto *program = GetETSBinder()->GetContext()->parserProgram;
    ES2PANDA_ASSERT(program);
    return program;
}

varbinder::ETSBinder *ScopedDebugInfoPlugin::GetETSBinder()
{
    return e2pctx_->parserProgram->VarBinder()->AsETSBinder();
}

ArenaAllocator *ScopedDebugInfoPlugin::Allocator()
{
    return checker_->Allocator();
}

}  // namespace ark::es2panda::evaluate
