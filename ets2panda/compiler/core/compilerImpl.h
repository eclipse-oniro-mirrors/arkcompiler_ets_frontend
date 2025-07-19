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

#ifndef ES2PANDA_COMPILER_INCLUDE_COMPILER_IMPL_H
#define ES2PANDA_COMPILER_INCLUDE_COMPILER_IMPL_H

#include "compiler/core/compileQueue.h"

namespace ark::pandasm {
struct Program;
}  // namespace ark::pandasm

namespace ark::es2panda::compiler {
class CompileQueue;

void HandleGenerateDecl(const parser::Program &program, util::DiagnosticEngine &diagnosticEngine,
                        const std::string &outputPath);

class CompilationUnit {
public:
    explicit CompilationUnit(const SourceFile &i, const util::Options &o, uint32_t s, ScriptExtension e,
                             util::DiagnosticEngine &de)
        : input(i), options(o), rawParserStatus(s), ext(e), diagnosticEngine(de)
    {
    }

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    const SourceFile &input;
    const util::Options &options;
    uint32_t rawParserStatus;
    ScriptExtension ext;
    util::DiagnosticEngine &diagnosticEngine;
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

class CompilerImpl {
public:
    explicit CompilerImpl(size_t threadCount, std::vector<util::Plugin> const *plugins)
        : queue_(threadCount), plugins_(plugins)
    {
    }
    NO_COPY_SEMANTIC(CompilerImpl);
    NO_MOVE_SEMANTIC(CompilerImpl);
    ~CompilerImpl() = default;

    pandasm::Program *Compile(const CompilationUnit &unit, public_lib::Context *context);

    std::vector<util::Plugin> const &Plugins()
    {
        return *plugins_;
    }

    static void DumpAsm(const ark::pandasm::Program *prog);
    static std::string GetPhasesList(ScriptExtension ext);

    void Emit(public_lib::Context *context);

    CompileQueue *Queue()
    {
        return &queue_;
    }

private:
    static void HandleContextLiterals(public_lib::Context *context);

    CompileQueue queue_;
    std::vector<util::Plugin> const *plugins_;
};
}  // namespace ark::es2panda::compiler

#endif
