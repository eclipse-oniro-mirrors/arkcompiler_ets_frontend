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

#ifndef ES2PANDA_PUBLIC_PUBLIC_H
#define ES2PANDA_PUBLIC_PUBLIC_H

#include <unordered_map>
#include "public/es2panda_lib.h"

#include "assembler/assembly-program.h"
#include "libarkbase/mem/arena_allocator.h"

#include "parser/ETSparser.h"
#include "parser/program/program.h"
#include "checker/ETSchecker.h"
#include "compiler/core/emitter.h"
#include "compiler/core/compileQueue.h"

namespace ark::es2panda::util {
class Options;
}  // namespace ark::es2panda::util

namespace ark::es2panda::compiler {
class PhaseManager;
void SetPhaseManager(PhaseManager *phaseManager);
PhaseManager *GetPhaseManager();
}  // namespace ark::es2panda::compiler

class DepAnalyzer;

namespace ark::es2panda::public_lib {

struct ConfigImpl {
    const util::Options *options = nullptr;
    util::DiagnosticEngine *diagnosticEngine = nullptr;
    std::list<diagnostic::DiagnosticKind> diagnosticKindStorage;
};

using ArenaExternalSources = parser::Program::ExternalSources;
using ComputedAbstracts =
    ArenaUnorderedMap<checker::ETSObjectType *,
                      std::pair<ArenaVector<checker::ETSFunctionType *>, ArenaUnorderedSet<checker::ETSObjectType *>>>;

struct GlobalContext {
    std::unordered_map<std::string, ArenaAllocator *> externalProgramAllocators;
    std::unordered_map<std::string, parser::Program *> cachedExternalPrograms;
    ArenaAllocator *stdLibAllocator = nullptr;
    ArenaExternalSources *stdLibAstCache = nullptr;
    std::unordered_set<varbinder::ETSBinder *> allocatedVarbinders;
};

struct Context {
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    using CodeGenCb =
        std::function<void(public_lib::Context *context, varbinder::FunctionScope *, compiler::ProgramElement *)>;

    ArenaAllocator *Allocator() const
    {
        return allocator;
    }

    template <typename T, typename... Args>
    T *AllocNode(Args &&...args)
    {
        // SUPPRESS_CSA_NEXTLINE(alpha.core.AllocatorETSCheckerHint)
        return util::NodeAllocator::ForceSetParent<T>(Allocator(), std::forward<Args>(args)...);
    }

    checker::Checker *GetChecker() const;

    void PushChecker(checker::Checker *checker)
    {
        checkers_.push_back(checker);
    }

    // NOTE(zhelyapov): It's calling side responsibility to release resources
    void ClearCheckers()
    {
        checkers_.clear();
    }

    checker::SemanticAnalyzer *GetAnalyzer() const;

    void PushAnalyzer(checker::SemanticAnalyzer *analyzer)
    {
        return analyzers_.push_back(analyzer);
    }

    // NOTE(zhelyapov): It's calling side responsibility to release resources
    void ClearAnalyzers()
    {
        analyzers_.clear();
    }

    ConfigImpl *config = nullptr;
    EHeap::Scope *eheapScope = nullptr;

    GlobalContext *globalContext = nullptr;
    std::string sourceFileName;
    std::string input;
    SourceFile const *sourceFile = nullptr;
    ArenaAllocator *allocator = nullptr;
    compiler::CompileQueue *queue = nullptr;
    std::vector<util::Plugin> const *plugins = nullptr;
    std::vector<compiler::LiteralBuffer> contextLiterals;
    CodeGenCb codeGenCb;
    compiler::PhaseManager *phaseManager = nullptr;

    // NOTE(dkofanov)): The "main" program.
    // Keeps so called external sources.
    // Should be renamed or revised completely (taking into account "simultaneous" mode):
    parser::Program *parserProgram = nullptr;

    parser::ParserImpl *parser = nullptr;
    compiler::Emitter *emitter = nullptr;
    pandasm::Program *program = nullptr;
    DepAnalyzer *depAnalyzer = nullptr;
    util::DiagnosticEngine *diagnosticEngine = nullptr;

    es2panda_ContextState state = ES2PANDA_STATE_NEW;
    std::string errorMessage;
    lexer::SourcePosition errorPos;

    bool isExternal = false;
    bool isLspUsage = false;
    bool compiledByCapi = false;
    bool lazyCheck = true;
    std::vector<std::string> sourceFileNames;
    // NOLINTEND(misc-non-private-member-variables-in-classes)

private:
    std::vector<checker::Checker *> checkers_;
    std::vector<checker::SemanticAnalyzer *> analyzers_;
};

}  // namespace ark::es2panda::public_lib

#endif
