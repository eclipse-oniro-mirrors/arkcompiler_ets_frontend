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

#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>
#include <string_view>
#include "public/es2panda_lib.h"

#include "assembler/assembly-program.h"
#include "libarkbase/mem/arena_allocator.h"

#include "parser/ETSparser.h"
#include "parser/program/program.h"
#include "checker/ETSchecker.h"
#include "compiler/core/emitter.h"
#include "compiler/core/compileQueue.h"
#include "util/patchFix.h"

namespace ark::es2panda::util {
class Options;
class PatchFix;
}  // namespace ark::es2panda::util

namespace ark::es2panda::compiler {
class PhaseManager;
void SetPhaseManager(PhaseManager *phaseManager);
PhaseManager *GetPhaseManager();
}  // namespace ark::es2panda::compiler

class DepAnalyzer;

namespace ark::es2panda::public_lib {

// Stable content-addressed copies of borrowed CAPI getter results; append-only, so earlier
// snapshots stay readable after edits. Implementation lives in public.cpp (es2panda-lib).
class ApiResultStorage {
public:
    using StringHashFn = size_t (*)(std::string_view);
    using PointerHashFn = size_t (*)(size_t hash);

    template <typename T>
    using PointerReader = T (*)(const void *, size_t);

    // Hash overrides exist for tests that need deterministic collisions.
    explicit ApiResultStorage(StringHashFn stringHash = nullptr, PointerHashFn pointerHash = nullptr);
    ~ApiResultStorage();
    ApiResultStorage(const ApiResultStorage &) = delete;
    ApiResultStorage &operator=(const ApiResultStorage &) = delete;

    char *String(std::string_view value);
    // Stable typed buffer with the reader's values; an empty result still
    // provides a valid address for [result, result + 0) readers.
    char **Pointers(size_t count, const void *source, PointerReader<char *> read);
    es2panda_AstNode **Pointers(size_t count, const void *source, PointerReader<es2panda_AstNode *> read);
    es2panda_Type **Pointers(size_t count, const void *source, PointerReader<es2panda_Type *> read);
    es2panda_Signature **Pointers(size_t count, const void *source, PointerReader<es2panda_Signature *> read);
    es2panda_Variable **Pointers(size_t count, const void *source, PointerReader<es2panda_Variable *> read);
    es2panda_Declaration **Pointers(size_t count, const void *source, PointerReader<es2panda_Declaration *> read);
    es2panda_Scope **Pointers(size_t count, const void *source, PointerReader<es2panda_Scope *> read);
    void Clear();

private:
    struct Impl;
    Impl &GetImpl();

    StringHashFn stringHash_;
    PointerHashFn pointerHash_;
    std::unique_ptr<Impl> impl_;
};

// Shared by generated getter copies and tests. The stack-local reader is
// consumed synchronously, without materializing a temporary input vector.
template <typename T, typename Read>
T *CopyApiPointerResult(ApiResultStorage &storage, size_t count, Read read)
{
    return storage.Pointers(count, &read, [](const void *source, size_t index) -> T {
        return (*static_cast<const Read *>(source))(index);
    });
}

struct ConfigImpl {
    const util::Options *options = nullptr;
    util::DiagnosticEngine *diagnosticEngine = nullptr;
    std::list<diagnostic::DiagnosticKind> diagnosticKindStorage;
};

using ArenaExternalSources = parser::Program::ExternalPrograms;
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
    // NOLINTBEGIN(misc-non-private-member-variables-in-class)
    Context();
    ~Context();

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
    ApiResultStorage apiResults;
    compiler::CompileQueue *queue = nullptr;
    std::vector<util::Plugin> const *plugins = nullptr;
    std::vector<compiler::LiteralBuffer> contextLiterals;
    CodeGenCb codeGenCb;
    compiler::PhaseManager *phaseManager = nullptr;

    // NOTE(dkofanov)): The "main" program.
    // Keeps so called external sources.
    // Should be renamed or revised completely (taking into account "simultaneous" mode):
    ark::es2panda::parser::Program *parserProgram = nullptr;

    parser::ParserImpl *parser = nullptr;
    compiler::Emitter *emitter = nullptr;
    std::unique_ptr<util::PatchFix> patchFixHelper;

    // NOTE(mshimenkov): In case of using ETSEmitter and building in simultaneous incremental mode
    // This map will contain more than one entry
    std::unordered_map<std::string, std::unique_ptr<pandasm::Program>> output = {};

    DepAnalyzer *depAnalyzer = nullptr;
    util::DiagnosticEngine *diagnosticEngine = nullptr;

    es2panda_ContextState state = ES2PANDA_STATE_NEW;
    std::string errorMessage;
    lexer::SourcePosition errorPos;

    bool isExternal = false;
    bool isLspUsage = false;
    bool lazyCheck = true;
    std::vector<std::string> sourceFileNames;
    panda_file::MetadataByPackages metadata;
    std::function<void(ir::AstNode *)> materializeMembers;
    // NOLINTEND(misc-non-private-member-variables-in-classes)

private:
    std::vector<checker::Checker *> checkers_;
    std::vector<checker::SemanticAnalyzer *> analyzers_;
};

}  // namespace ark::es2panda::public_lib

#endif
