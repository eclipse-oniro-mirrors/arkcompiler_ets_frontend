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

#ifndef ES2PANDA_EVALUATE_SCOPED_DEBUG_INFO_PLUGIN_H
#define ES2PANDA_EVALUATE_SCOPED_DEBUG_INFO_PLUGIN_H

#include "evaluate/debugInfoStorage.h"
#include "evaluate/evaluateContext.h"
#include "evaluate/helpers.h"
#include "evaluate/irCheckHelper.h"
#include "evaluate/proxyProgramsCache.h"
#include "evaluate/pathResolver.h"
#include "evaluate/entityDeclarator.h"
#include "evaluate/debugInfoDeserialization/debugInfoDeserializer.h"

namespace ark::es2panda::ir {
class BlockStatement;
class Statement;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::varbinder {
class ETSBinder;
class Variable;
}  // namespace ark::es2panda::varbinder

namespace ark::es2panda::evaluate {

/// @brief Context-dependent debug info plugin. Must be created before parsing phase.
class ScopedDebugInfoPlugin final {
public:
    explicit ScopedDebugInfoPlugin(public_lib::Context *ctx);

    NO_COPY_SEMANTIC(ScopedDebugInfoPlugin);
    NO_MOVE_SEMANTIC(ScopedDebugInfoPlugin);

    ~ScopedDebugInfoPlugin() = default;

    /**
     * @brief Searches debug-info for the given identifier
     * @param ident node with name of either a local or global variable or a class
     */
    varbinder::Variable *FindIdentifier(ir::Identifier *ident);

    /**
     * @brief Searches debug-info for the given class
     * On success creates IR for this and all dependencies.
     */
    varbinder::Variable *FindClass(ir::Identifier *ident);

    /**
     * @brief Adds collected prologue and epilogue statements in the block
     * In effect, previously collected prologue-epilogue statements are dropped.
     */
    void AddPrologueEpilogue(ir::BlockStatement *block);

    /**
     * @brief Save \p stmt node to be pushed later at the beginning or end of the \p block depending on the IS_PROLOGUE
     * parameter
     */
    template <bool IS_PROLOGUE>
    void RegisterPrologueEpilogue(ir::BlockStatement *block, ir::Statement *stmt);

    /**
     * @brief Initialization before ETSChecker starts checking AST
     * Since we can resolve references like `new A()`, that resolved before ETSChecker was started,
     * we need such a precheck call.
     */
    void PreCheck();

    /**
     * @brief Finalization after ETSChecker checks the main program
     */
    void PostCheck();

    PathResolver *GetPathResolver()
    {
        return &pathResolver_;
    }

    const PathResolver *GetPathResolver() const
    {
        return &pathResolver_;
    }

    IrCheckHelper *GetIrCheckHelper()
    {
        return &irCheckHelper_;
    }

    const IrCheckHelper *GetIrCheckHelper() const
    {
        return &irCheckHelper_;
    }

    DebugInfoStorage *GetDebugInfoStorage()
    {
        return &debugInfoStorage_;
    }

    const DebugInfoStorage *GetDebugInfoStorage() const
    {
        return &debugInfoStorage_;
    }

    ProxyProgramsCache *GetProxyProgramsCache()
    {
        return &proxyProgramsCache_;
    }

    const ProxyProgramsCache *GetProxyProgramsCache() const
    {
        return &proxyProgramsCache_;
    }

    EntityDeclarator *GetEntityDeclarator()
    {
        return &entityDeclarator_;
    }

    const EntityDeclarator *GetEntityDeclarator() const
    {
        return &entityDeclarator_;
    }

    DebugInfoDeserializer *GetDebugInfoDeserializer()
    {
        return &debugInfoDeserializer_;
    }

    const DebugInfoDeserializer *GetDebugInfoDeserializer() const
    {
        return &debugInfoDeserializer_;
    }

    varbinder::ETSBinder *GetETSBinder();

    auto *E2PContext()
    {
        return e2pctx_;
    }

private:
    using PrologueEpiloguePair = std::pair<ArenaVector<ir::Statement *>, ArenaVector<ir::Statement *>>;
    using PrologueEpilogueMap = ArenaUnorderedMap<ir::BlockStatement *, PrologueEpiloguePair>;

private:
    /**
     * @brief Creates `Program` instances for each input context .abc file.
     */
    void CreateContextPrograms();

    /**
     * @brief Creates a program with the given package name and adds it as external for the given global program.
     * This method must be called once and before running any compiler phases.
     * @param sourceFilePath corresponding to source code of one of the provided .abc files.
     * @param moduleName of source file, extracted from .abc file encoding.
     * @returns pointer to initialized program.
     */
    parser::Program *CreateEmptyProgram(std::string_view sourceFilePath, std::string_view moduleName);

    /**
     * @brief Returns non-null program for the given source file path.
     */
    parser::Program *GetProgram(util::StringView fileName);

    parser::Program *GetEvaluatedExpressionProgram();

    /// Search methods.
    varbinder::Variable *FindGlobalVariable(ir::Identifier *ident);
    varbinder::Variable *FindGlobalFunction(ir::Identifier *ident);
    varbinder::Variable *FindLocalVariable(ir::Identifier *ident);

    /**
     * @brief Pushes back return statement into the evaluation method
     * if its last statement potentially returns a primitive value.
     * @returns true if insertion took place, false otherwise.
     */
    bool InsertReturnStatement();

    ArenaAllocator *Allocator();

private:
    public_lib::Context *e2pctx_ {};
    checker::ETSChecker *checker_ {nullptr};

    EvaluateContext context_;

    IrCheckHelper irCheckHelper_;
    DebugInfoStorage debugInfoStorage_;
    DebugInfoDeserializer debugInfoDeserializer_;
    PathResolver pathResolver_;
    PrologueEpilogueMap prologueEpilogueMap_;
    ProxyProgramsCache proxyProgramsCache_;
    EntityDeclarator entityDeclarator_;
};

}  // namespace ark::es2panda::evaluate

#endif  // ES2PANDA_EVALUATE_SCOPED_DEBUG_INFO_PLUGIN_H
