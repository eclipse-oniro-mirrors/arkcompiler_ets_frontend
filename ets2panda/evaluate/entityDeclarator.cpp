/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "evaluate/scopedDebugInfoPlugin-inl.h"
#include "evaluate/entityDeclarator.h"
#include "evaluate/irCheckHelper.h"

#include "parser/program/program.h"
#include "ir/expressions/identifier.h"
#include "ir/ets/etsImportDeclaration.h"
#include "ir/expressions/literals/stringLiteral.h"

#include "public/public.h"
#include "util/importPathManager.h"

namespace ark::es2panda::evaluate {

EntityDeclarator::EntityDeclarator(ScopedDebugInfoPlugin &debugInfoPlugin)
    : debugInfoPlugin_(debugInfoPlugin),
      createdEntities_(debugInfoPlugin_.GetIrCheckHelper()->GetChecker()->Allocator()->Adapter())
{
}

UMapStringViewVariable &EntityDeclarator::GetOrCreateEntitiesMap(parser::Program *program)
{
    ES2PANDA_ASSERT(program);

    auto iter = createdEntities_.find(program);
    if (iter == createdEntities_.end()) {
        auto adapter = debugInfoPlugin_.GetIrCheckHelper()->GetChecker()->Allocator()->Adapter();
        return createdEntities_.emplace(program, UMapStringViewVariable(adapter)).first->second;
    }
    return iter->second;
}

void EntityDeclarator::CreateAndInsertImportStatement(util::StringView pathToDeclSource, util::StringView declName,
                                                      parser::Program *importerProgram, util::StringView importedName,
                                                      varbinder::Variable *var)
{
    ES2PANDA_ASSERT(importerProgram);
    ES2PANDA_ASSERT(var);

    auto &importEntitiesMap = GetOrCreateEntitiesMap(importerProgram);
    auto *findVar = FindEntityVariable(importEntitiesMap, importedName);
    if (findVar != nullptr) {
        // If var was found it means that import declaration has already been declared
        // and there is no need to create it once again.
        ES2PANDA_ASSERT(findVar == var);
        return;
    }

    auto *importStatement = CreateIrImport(pathToDeclSource, declName, importedName);
    InsertImportStatement(importStatement, importerProgram);

    if (!importEntitiesMap.emplace(importedName, var).second) {
        LOG(FATAL, ES2PANDA) << "Failed to emplace " << importedName << " in entity map.";
    }
}

ir::ETSImportDeclaration *EntityDeclarator::CreateIrImport(util::StringView pathToDeclSourceFile,
                                                           util::StringView classDeclName,
                                                           util::StringView classImportedName)
{
    auto *checker = debugInfoPlugin_.GetIrCheckHelper()->GetChecker();
    auto *allocator = checker->Allocator();

    auto moduleName = debugInfoPlugin_.GetDebugInfoStorage()->GetModuleName(pathToDeclSourceFile.Utf8());
    auto *source = checker->AllocNode<ir::StringLiteral>(moduleName);
    auto importLanguage = ToLanguage(debugInfoPlugin_.GetETSBinder()->Extension());
    util::ImportMetadata importMetadata {*debugInfoPlugin_.E2PContext()->parser->GetImportPathManager(),
                                         pathToDeclSourceFile.Mutf8(), importLanguage.GetId()};

    auto *local = checker->AllocNode<ir::Identifier>(classDeclName, allocator);
    auto *imported = checker->AllocNode<ir::Identifier>(classImportedName, allocator);
    auto *spec = checker->AllocNode<ir::ImportSpecifier>(imported, local);
    ArenaVector<ir::AstNode *> specifiers(1, spec, allocator->Adapter());
    // NOLINTNEXTLINE (performance-move-const-arg)
    return checker->AllocNode<ir::ETSImportDeclaration>(source, importMetadata, std::move(specifiers));
}

void EntityDeclarator::InsertImportStatement(ir::Statement *importStatement, parser::Program *importerProgram)
{
    ES2PANDA_ASSERT(importerProgram);
    ES2PANDA_ASSERT(importStatement);

    auto *topStatement = importerProgram->Ast();
    importStatement->SetParent(topStatement);
    // Can't insert right away until block's statements iteration ends.
    debugInfoPlugin_.RegisterPrologueEpilogue<true>(topStatement, importStatement);

    debugInfoPlugin_.GetIrCheckHelper()->CheckGlobalEntity(importerProgram, importStatement);
}

bool EntityDeclarator::IsEntityDeclared(parser::Program *program, util::StringView name)
{
    ES2PANDA_ASSERT(program);

    auto &entitiesMap = GetOrCreateEntitiesMap(program);
    return entitiesMap.find(name) != entitiesMap.end();
}

/* static */
varbinder::Variable *EntityDeclarator::FindEntityVariable(UMapStringViewVariable &entitiesMap,
                                                          util::StringView entityName)
{
    const auto &find = entitiesMap.find(entityName);
    if (find != entitiesMap.end()) {
        return find->second;
    }
    return nullptr;
}

}  // namespace ark::es2panda::evaluate
