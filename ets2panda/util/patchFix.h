/**
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_UTIL_PATCHFIX_H
#define ES2PANDA_UTIL_PATCHFIX_H

#include "assembly-function.h"
#include "assembly-program.h"
#include "util/eheap.h"
#include "util/symbolTable.h"

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace ark::es2panda::compiler {
class CodeGen;
}  // namespace ark::es2panda::compiler

namespace ark::es2panda::parser {
class Program;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::util {
class Options;
}  // namespace ark::es2panda::util

namespace ark::es2panda::util {

enum class PatchFixKind { DUMPSYMBOLTABLE, HOTFIX, COLDFIX, HOTRELOAD, COLDRELOAD };

class PatchFix {
public:
    PatchFix(PatchFixKind kind, const std::string &recordName, std::unique_ptr<SymbolTable> symbolTable);

    // Called from FunctionEmitter::Generate() after function instructions are fully emitted.
    // Dump mode:   computes function hash and writes to symbol table.
    // ColdReload:  validates function hash matches origin symbol table (error on mismatch).
    // HotReload:   same lookup, but allows hash mismatch (runtime handles hot-patching).
    void ProcessFunction(compiler::CodeGen *cg, pandasm::Function *func);

    // Called from Emitter after module records are emitted.
    // Dump mode: serializes module import/export info from parser::Program into symbol table.
    // ColdReload mode: validates module import/export info against origin symbol table.
    void ProcessModule(const parser::Program *program);

    // Called from Emitter with the set of exported class/interface names.
    // Detects changes in the export set (e.g. "export function" → "function").
    // The program parameter is used to generate a per-module key (e.g. "dep.ets:exports")
    // so that exports from different programs in simultaneous incremental mode don't collide.
    void ProcessExports(const parser::Program *program, const std::vector<std::string> &exportedNames);

    // Called from Emitter with class inheritance/interface info.
    // Each entry: (className, parentName, semicolon-joined interface names).
    // Detects changes in class inheritance or implemented interfaces.
    // The program parameter is used to generate a per-module key (e.g. "dep.ets:classinfo")
    // so that class info from different programs in simultaneous incremental mode don't collide.
    void ProcessClassInfo(const parser::Program *program,
                          const std::vector<std::tuple<std::string, std::string, std::string>> &classInfos);

    // Called at the end of compilation.
    // COLDRELOAD and DUMPSYMBOLTABLE: returns immediately (no patch bytecode generation).
    void Finalize(pandasm::Program **prog);

    bool IsDumpSymbolTable() const;
    bool IsHotFix() const;
    bool IsColdFix() const;
    bool IsHotReload() const;
    bool IsColdReload() const;

    bool HasError() const
    {
        return patchError_;
    }

    const std::string GetErrorMessage() const
    {
        return errMsg_.str();
    }

    SymbolTable *GetSymbolTable() const
    {
        return symbolTable_.get();
    }

private:
    // --- Dump mode (Phase 1) ---

    void DumpFunctionInfo(compiler::CodeGen *cg, pandasm::Function *func);
    void DumpModuleInfo(const parser::Program *program);

    // --- ColdReload / HotReload mode (Phase 2) ---
    // HandleFunction serves both ColdReload and HotReload.
    // ColdReload: funcHash mismatch → error.
    // HotReload:  funcHash mismatch → allowed (IsHotReload() early-return).

    void HandleFunction(compiler::CodeGen *cg, pandasm::Function *func);
    void ValidateModuleInfo(const parser::Program *program);

    // ColdReload only: cross-reference originFunctionInfo_ against matchedFunctions_
    // and newFunctionBaseNames_ to detect signature changes (vs. pure deletions).
    // Called from Finalize() once all functions have been processed.
    void DetectSignatureChanges();

    // --- Shared helpers ---

    // Generates hash from the compiled function's instruction sequence.
    std::string GenerateFunctionHash(pandasm::Function *func);

    // Derives a stable module key from the parser::Program, consistent across
    // dump and cold-reload phases (file stem, not full path).
    static std::string GetModuleKey(const parser::Program *program);

    // Serializes import/export info from parser::Program into a hash string.
    static std::string ComputeModuleHash(const parser::Program *program);

    // --- Members ---

    PatchFixKind patchFixKind_;
    std::unique_ptr<SymbolTable> symbolTable_;
    std::string recordName_;
    bool patchError_ {false};
    std::stringstream errMsg_;

    // Cached pointers to origin symbol table data (obtained in constructor).
    SArenaUnorderedMap<std::string, SymbolTable::OriginFunctionInfo> *originFunctionInfo_ {nullptr};
    SArenaUnorderedMap<std::string, std::string> *originModuleInfo_ {nullptr};

    // Tracks which origin functions were matched during HandleFunction.
    std::unordered_set<std::string> matchedFunctions_;
    // Base names (before ':') of new functions not in the origin table.
    std::unordered_set<std::string> newFunctionBaseNames_;
};

// Validates CLI option combinations for reload modes. Used by both CLI and C API paths.
bool ValidateReloadOptions(const Options &options);

// Creates PatchFix and SymbolTable from reload options and source path.
// Returns {true, PatchFix} on success (PatchFix is null if no reload flags set).
// Returns {false, nullptr} with stderr message on initialization failure.
// Used by both CLI (compilerImpl.cpp) and C API (es2panda_lib.cpp) paths.
std::pair<bool, std::unique_ptr<PatchFix>> InitPatchFix(const Options &options, const std::string &sourcePath);

// Runs validation and writes the symbol table. Does not own PatchFix or SymbolTable.
// If outErrorMsg is non-null, receives the error message (if any).
bool FinalizePatchFix(PatchFix &patchFix, std::string *outErrorMsg = nullptr);

}  // namespace ark::es2panda::util

#endif  // ES2PANDA_UTIL_PATCHFIX_H
