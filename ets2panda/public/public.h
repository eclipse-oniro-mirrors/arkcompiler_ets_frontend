/**
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include "libpandabase/mem/arena_allocator.h"

#include "compiler/core/compileQueue.h"
#include "parser/ETSparser.h"
#include "checker/ETSchecker.h"
#include "compiler/core/emitter.h"

namespace ark::es2panda::util {
class Options;
}  // namespace ark::es2panda::util

namespace ark::es2panda::compiler {
class PhaseManager;
void SetPhaseManager(PhaseManager *phaseManager);
PhaseManager *GetPhaseManager();
}  // namespace ark::es2panda::compiler

namespace ark::es2panda::public_lib {

enum class CompilingState : unsigned int {
    NONE_COMPILING = 0,
    SINGLE_COMPILING = 1,
    MULTI_COMPILING_INIT = 2,
    MULTI_COMPILING_FOLLOW = 3,
};

struct ConfigImpl {
    const util::Options *options = nullptr;
    util::DiagnosticEngine *diagnosticEngine = nullptr;
    std::list<diagnostic::DiagnosticKind> diagnosticKindStorage;
};

using ExternalSources = std::unordered_map<util::StringView, ArenaVector<parser::Program *>>;
using ExternalSource = ArenaUnorderedMap<util::StringView, ArenaVector<parser::Program *>>;
using ComputedAbstracts =
    ArenaUnorderedMap<checker::ETSObjectType *,
                      std::pair<ArenaVector<checker::ETSFunctionType *>, ArenaUnorderedSet<checker::ETSObjectType *>>>;

class TransitionMemory {
public:
    explicit TransitionMemory(ThreadSafeArenaAllocator *allocator)
        : permanentAllocator_(allocator), compiledPrograms_(allocator->Adapter())
    {
        compiledPrograms_ = {};
    }

    NO_COPY_SEMANTIC(TransitionMemory);
    DEFAULT_MOVE_SEMANTIC(TransitionMemory);

    ~TransitionMemory() = default;

    ThreadSafeArenaAllocator *PermanentAllocator() const
    {
        return permanentAllocator_.get();
    }

    const varbinder::VarBinder *VarBinder() const
    {
        return varbinder_;
    }

    varbinder::VarBinder *VarBinder()
    {
        return varbinder_;
    }

    void SetVarBinder(varbinder::VarBinder *varbinder)
    {
        varbinder_ = varbinder;
    }

    const checker::GlobalTypesHolder *GlobalTypes() const
    {
        return globalTypes_;
    }

    checker::GlobalTypesHolder *GlobalTypes()
    {
        return globalTypes_;
    }

    void SetGlobalTypes(checker::GlobalTypesHolder *globalTypes)
    {
        globalTypes_ = globalTypes;
    }

    void AddCompiledProgram(parser::Program *program)
    {
        compiledPrograms_.push_back(program);
    }

    ArenaVector<parser::Program *> &CompiledSources()
    {
        return compiledPrograms_;
    }

    const ArenaVector<parser::Program *> &CompiledPrograms() const
    {
        return compiledPrograms_;
    }

    const ComputedAbstracts *CachedComputedAbstracts() const
    {
        return cachedComputedAbstracts_;
    }

    ComputedAbstracts *CachedComputedAbstracts()
    {
        return cachedComputedAbstracts_;
    }

    void SetCachechedComputedAbstracts(ComputedAbstracts *cachedComputedAbstracts)
    {
        cachedComputedAbstracts_ = cachedComputedAbstracts;
    }

private:
    std::unique_ptr<ThreadSafeArenaAllocator> permanentAllocator_;
    ArenaVector<parser::Program *> compiledPrograms_;
    varbinder::VarBinder *varbinder_ {nullptr};
    checker::GlobalTypesHolder *globalTypes_ {nullptr};
    ComputedAbstracts *cachedComputedAbstracts_ {nullptr};
};

struct GlobalContext {
    std::unordered_map<std::string, ArenaAllocator *> externalProgramAllocators;
    std::unordered_map<std::string, ExternalSource *> cachedExternalPrograms;
    ThreadSafeArenaAllocator *stdLibAllocator = nullptr;
    ExternalSource *stdLibAstCache = nullptr;
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
        parserProgram->PushChecker(checker);
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

    void MarkGenAbcForExternal(std::unordered_set<std::string> &genAbcList, public_lib::ExternalSource &extSources);

    ConfigImpl *config = nullptr;
    GlobalContext *globalContext = nullptr;
    std::string sourceFileName;
    std::string input;
    SourceFile const *sourceFile = nullptr;
    ThreadSafeArenaAllocator *allocator = nullptr;
    compiler::CompileQueue *queue = nullptr;
    std::vector<util::Plugin> const *plugins = nullptr;
    std::vector<compiler::LiteralBuffer> contextLiterals;
    CodeGenCb codeGenCb;
    compiler::PhaseManager *phaseManager = nullptr;

    parser::Program *parserProgram = nullptr;
    parser::ParserImpl *parser = nullptr;
    compiler::Emitter *emitter = nullptr;
    pandasm::Program *program = nullptr;
    util::DiagnosticEngine *diagnosticEngine = nullptr;

    es2panda_ContextState state = ES2PANDA_STATE_NEW;
    std::string errorMessage;
    lexer::SourcePosition errorPos;

    CompilingState compilingState {CompilingState::NONE_COMPILING};
    ExternalSources externalSources;
    TransitionMemory *transitionMemory {nullptr};
    bool isExternal = false;
    bool compiledByCapi = false;
    std::vector<std::string> sourceFileNames;
    std::map<util::StringView, parser::Program *> dupPrograms {};
    // NOLINTEND(misc-non-private-member-variables-in-classes)

private:
    std::vector<checker::Checker *> checkers_;
    std::vector<checker::SemanticAnalyzer *> analyzers_;
};

}  // namespace ark::es2panda::public_lib

#endif
