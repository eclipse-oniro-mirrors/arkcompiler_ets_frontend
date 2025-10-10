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

#ifndef PANDA_GLOBALCLASSHANDLER_H
#define PANDA_GLOBALCLASSHANDLER_H

#include "compiler/lowering/ets/topLevelStmts/globalDeclTransformer.h"
#include "ir/ets/etsReExportDeclaration.h"
#include "parser/program/program.h"
#include "public/public.h"
#include "ir/astNode.h"

namespace ark::es2panda::compiler {

class GlobalClassHandler {
public:
    using ModuleDependencies = ArenaUnorderedSet<parser::Program *>;

    struct GlobalStmts {
        parser::Program *program;
        ArenaVector<ir::Statement *> statements;
    };
    explicit GlobalClassHandler(parser::ETSParser *parser, ArenaAllocator *allocator, parser::Program *program)
        : parser_(parser),
          allocator_(allocator),
          globalProgram_(program),
          packageInitializerBlockCount_(allocator->Adapter()) {};

    static void MergeNamespace(ArenaVector<ir::ETSModule *> &namespaces, parser::Program *program);

    /**
     * Each "Module" has it's own global class, which contains all top level statements across "module"
     * Result - creation of global class and _$init$_ method
     * @param programs - vector of files in module
     */
    void SetupGlobalClass(const ArenaVector<parser::Program *> &programs, const ModuleDependencies *moduleDependencies);

    void CheckPackageMultiInitializerBlock(util::StringView packageName,
                                           const ArenaVector<ArenaVector<ir::Statement *>> &initializerBlocks);
    void SetGlobalProgram(parser::Program *program)
    {
        globalProgram_ = program;
    }

private:
    /**
     * Move top level statements to _$init$_ and
     * @param program program of module
     * @param init_statements statements which should be executed
     */
    void SetupGlobalMethods(ArenaVector<ir::Statement *> &&statements);
    void AddStaticBlockToClass(ir::AstNode *node);
    void CollectProgramGlobalClasses(ArenaVector<ir::ETSModule *> namespaces);
    ir::ClassDeclaration *TransformNamespace(ir::ETSModule *ns);
    ir::ClassDeclaration *CreateTransformedClass(ir::ETSModule *ns);
    template <class Node>
    void CollectExportedClasses(parser::Program *program, ir::ClassDefinition *classDef,
                                const ArenaVector<Node *> &statements);
    void CollectReExportedClasses(parser::Program *program, ir::ClassDefinition *classDef,
                                  const ir::ETSReExportDeclaration *reExport);
    void CollectNamespaceExportedClasses(parser::Program *program, ir::ClassDefinition *classDef);
    void SetupGlobalMethods(ArenaVector<ir::Statement *> &&initStatements, ir::ClassDefinition *globalClass,
                            bool isDeclare);
    void SetupInitializerBlock(ArenaVector<ArenaVector<ir::Statement *>> &&initializerBlock,
                               ir::ClassDefinition *globalClass);
    void SetupInitializationMethodIfNeeded(ir::ClassDefinition *classDef);
    ArenaVector<ir::Statement *> TransformNamespaces(ArenaVector<ir::ETSModule *> &namespaces);

    ir::ClassDeclaration *CreateGlobalClass(const parser::Program *globalProgram);
    ir::ClassStaticBlock *CreateStaticBlock(ir::ClassDefinition *classDef);
    ir::MethodDefinition *CreateGlobalMethod(std::string_view name, ArenaVector<ir::Statement *> &&statements);
    void AddInitStatementsToStaticBlock(ir::ClassDefinition *globalClass,
                                        ArenaVector<ir::Statement *> &&initStatements);
    void AddInitCallToStaticBlock(ir::ClassDefinition *globalClass, ir::MethodDefinition *initMethod);
    void AddInitializerBlockToStaticBlock(ir::ClassDefinition *globalClass,
                                          ArenaVector<ir::Statement *> &&initializerBlocks);

    ArenaVector<ArenaVector<ir::Statement *>> FormInitStaticBlockMethodStatements(
        ArenaVector<GlobalStmts> &&initStatements);
    void TransformBrokenNamespace(ir::AstNode *node);

    ArenaVector<ir::Statement *> FormInitMethodStatements(ArenaVector<GlobalStmts> &&initStatements);

    GlobalDeclTransformer::ResultT CollectProgramGlobalStatements(ArenaVector<ir::Statement *> &stmts,
                                                                  ir::ClassDefinition *classDef,
                                                                  ir::Statement const *stmt);

    ir::Identifier *RefIdent(const util::StringView &name);

    parser::ETSParser *const parser_;
    ArenaAllocator *const allocator_;
    parser::Program *globalProgram_;
    ArenaUnorderedSet<util::StringView> packageInitializerBlockCount_;
};
}  // namespace ark::es2panda::compiler

#endif  // PANDA_GLOBALCLASSHANDLER_H
