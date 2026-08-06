/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef APPS_GLUEGEN_NATIVE_INCLUDE_CONTEXT_H
#define APPS_GLUEGEN_NATIVE_INCLUDE_CONTEXT_H

#include <string>
#include "libarkbase/macros.h"
#include "./logger/log.h"
#include "./diagnostic.h"
#include "./symbol.h"
#include "logger/logger.h"

namespace ark::es2panda::gluegen {

// Bundles the state a single Gluegen run shares across its phases (Gluec analysis, Gluel
// linking, and cache (de)serialization), following the same "explicit session object" shape used
// by mature compiler front ends (Clang's CompilerInstance, TypeScript's Program/CompilerHost,
// rustc's TyCtxt) instead of scattering that state across ambient globals/singletons. One
// Context is created per Gluegen run and passed by reference into Gluec/Gluel, so each run owns
// its own isolated symbol table and diagnostics rather than sharing process-wide state.
class Context {
public:
    Context() = default;
    ~Context() = default;

    NO_COPY_SEMANTIC(Context);
    NO_MOVE_SEMANTIC(Context);

    // Owns every SymbolNode allocated during this run -- by Gluec, Gluel, and IntermediateCache/
    // GlueConfig deserialization alike -- in place of the SymbolNodeManager singleton this class
    // replaces.
    SymbolNodeManager symbolNodeManager;

    // Collects every diagnostic (NOTE/WARNING/ERROR) reported while producing this run's output.
    DiagnosticEngine diagnosticEngine;

    // Logger
    log::Logger logger;

    // Absolute path to the working directory this run was invoked from (see
    // fs::current_path()), used to resolve default option paths (output/cache/
    // arktsconfig) and passed through to es2panda for `--ets-warnings:base-path`.
    std::string workingDirectory;
};

}  // namespace ark::es2panda::gluegen

#endif  // APPS_GLUEGEN_NATIVE_INCLUDE_CONTEXT_H
