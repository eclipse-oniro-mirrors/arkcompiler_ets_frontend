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
#include "mem/arena_allocator.h"

#include "compiler/core/compileQueue.h"
#include "parser/ETSparser.h"
#include "checker/checker.h"
#include "checker/IsolatedDeclgenChecker.h"
#include "compiler/core/emitter.h"

namespace ark::es2panda::util {
class Options;
}  // namespace ark::es2panda::util

namespace ark::es2panda::compiler {
class PhaseManager;
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
    explicit TransitionMemory(ArenaAllocator *allocator)
        : permanentAllocator_(allocator), compiledPrograms_(allocator->Adapter())
    {
        compiledPrograms_ = {};
    }

    NO_COPY_SEMANTIC(TransitionMemory);
    DEFAULT_MOVE_SEMANTIC(TransitionMemory);

    ~TransitionMemory() = default;

    ArenaAllocator *PermanentAllocator() const
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
    std::unique_ptr<ArenaAllocator> permanentAllocator_;
    ArenaVector<parser::Program *> compiledPrograms_;
    varbinder::VarBinder *varbinder_ {nullptr};
    checker::GlobalTypesHolder *globalTypes_ {nullptr};
    ComputedAbstracts *cachedComputedAbstracts_ {nullptr};
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

    void MarkGenAbcForExternal(std::unordered_set<std::string> &genAbcList, public_lib::ExternalSource &extSources)
    {
        size_t genCount = 0;
        std::unordered_set<std::string> genAbcListAbsolute;

        for (auto &path : genAbcList) {
            genAbcListAbsolute.insert(os::GetAbsolutePath(path));
        }
        for (auto &[_, extPrograms] : extSources) {
            (void)_;
            bool setFlag = false;
            for (auto *prog : extPrograms) {
                if (auto it = genAbcListAbsolute.find(prog->AbsoluteName().Mutf8()); it != genAbcListAbsolute.end()) {
                    ++genCount;
                    setFlag = true;
                }
            }
            if (!setFlag) {
                continue;
            }
            for (auto *prog : extPrograms) {
                prog->SetGenAbcForExternalSources();
            }
        }

        if (genCount != genAbcListAbsolute.size()) {
            diagnosticEngine->LogFatalError(diagnostic::SIMULTANEOUSLY_MARK_FAILED.Message());
        }
    }

    ConfigImpl *config = nullptr;
    std::string sourceFileName;
    std::string input;
    SourceFile const *sourceFile = nullptr;
    ArenaAllocator *allocator = nullptr;
    compiler::CompileQueue *queue = nullptr;
    std::vector<util::Plugin> const *plugins = nullptr;
    std::vector<compiler::LiteralBuffer> contextLiterals;
    CodeGenCb codeGenCb;
    compiler::PhaseManager *phaseManager = nullptr;

    parser::Program *parserProgram = nullptr;
    parser::ParserImpl *parser = nullptr;
    checker::Checker *checker = nullptr;
    checker::IsolatedDeclgenChecker *isolatedDeclgenChecker = nullptr;

    checker::SemanticAnalyzer *analyzer = nullptr;
    compiler::Emitter *emitter = nullptr;
    pandasm::Program *program = nullptr;
    util::DiagnosticEngine *diagnosticEngine = nullptr;

    es2panda_ContextState state = ES2PANDA_STATE_NEW;
    std::string errorMessage;
    lexer::SourcePosition errorPos;

    CompilingState compilingState {CompilingState::NONE_COMPILING};
    ExternalSources externalSources;
    TransitionMemory *transitionMemory {nullptr};
    std::vector<std::string> sourceFileNames;
    std::map<util::StringView, parser::Program *> dupPrograms {};
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};
}  // namespace ark::es2panda::public_lib

#endif
