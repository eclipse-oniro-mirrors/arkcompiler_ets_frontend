/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_EVALUATE_ENTITY_DECLARATOR_H
#define ES2PANDA_EVALUATE_ENTITY_DECLARATOR_H

#include "checker/ETSchecker.h"
#include "evaluate/proxyProgramsCache.h"
#include "evaluate/helpers.h"
#include "utils/arena_containers.h"

namespace ark::es2panda::parser {
class Program;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::varbinder {
class Variable;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::ir {
class Statement;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::evaluate {

using UMapStringViewVariable = ArenaUnorderedMap<util::StringView, varbinder::Variable *>;

/// @brief Helps to declare entities created by debug-info plugin in AST.
class EntityDeclarator final {
public:
    NO_COPY_SEMANTIC(EntityDeclarator);
    NO_MOVE_SEMANTIC(EntityDeclarator);

    explicit EntityDeclarator(ScopedDebugInfoPlugin &debugInfoPlugin);
    ~EntityDeclarator() = default;

    /**
     * @brief Invoke \p irCreator if entity was not already created and then create import declaration if it is needed
     *
     * If a variable of an entity that comes as a result of invoking irCreator is used in a program that does not match
     * the program where the entity was declared, then ImportGlobalEntity inserts an import declaration into the AST.
     */
    template <typename F>
    varbinder::Variable *ImportGlobalEntity(util::StringView pathToDeclSource, util::StringView declName,
                                            parser::Program *importerProgram, util::StringView importedName,
                                            F &&irCreator);

    /**
     * @brief Returns true if the entity has already been declared, or false otherwise
     */
    bool IsEntityDeclared(parser::Program *program, util::StringView name);

private:
    static varbinder::Variable *FindEntityVariable(UMapStringViewVariable &entitiesMap, util::StringView entityName);

    UMapStringViewVariable &GetOrCreateEntitiesMap(parser::Program *program);

    void CreateAndInsertImportStatement(util::StringView pathToDeclSource, util::StringView declName,
                                        parser::Program *importerProgram, util::StringView importedName,
                                        varbinder::Variable *var);

    ir::ETSImportDeclaration *CreateIrImport(util::StringView pathToDeclSourceFile, util::StringView classDeclName,
                                             util::StringView classImportedName);

    void InsertImportStatement(ir::Statement *importStatement, parser::Program *importerProgram);

private:
    ScopedDebugInfoPlugin &debugInfoPlugin_;

    ArenaUnorderedMap<parser::Program *, UMapStringViewVariable> createdEntities_;
};

}  // namespace ark::es2panda::evaluate

#endif  // ES2PANDA_EVALUATE_ENTITY_DECLARATOR_H
