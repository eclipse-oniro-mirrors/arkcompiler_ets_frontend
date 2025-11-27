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

#ifndef ES2PANDA_PUBLIC_H
#define ES2PANDA_PUBLIC_H

#include "util/es2pandaMacros.h"
#include "util/plugin.h"
#include "util/diagnostic.h"
#include "generated/options.h"
#include "util/language.h"

namespace ark::es2panda::ir {
class AstNode;
class Identifier;
class BlockStatement;
}  // namespace ark::es2panda::ir

namespace ark::pandasm {
struct Program;
}  // namespace ark::pandasm

namespace ark::es2panda {

using ETSWarnings = util::gen::ets_warnings::Enum;
using EvalMode = util::gen::eval_mode::Enum;
using ScriptExtension = util::gen::extension::Enum;

constexpr std::string_view ES2PANDA_VERSION = "0.1";

namespace util {
class Options;
class DiagnosticEngine;
}  // namespace util
namespace parser {
class ParserImpl;
}  // namespace parser

namespace compiler {
class CompilerImpl;
}  // namespace compiler

namespace varbinder {
class VarBinder;
}  // namespace varbinder

enum class CompilationMode {
    GEN_STD_LIB,
    PROJECT,
    SINGLE_FILE,
    GEN_ABC_FOR_EXTERNAL_SOURCE,
};
// CC-OFFNXT(G.FUD.06) switch-case, ODR
inline Language ToLanguage(ScriptExtension ext)
{
    switch (ext) {
        case ScriptExtension::JS:
            return Language(Language::Id::JS);
        case ScriptExtension::TS:
            return Language(Language::Id::TS);
        case ScriptExtension::AS:
            return Language(Language::Id::AS);
        case ScriptExtension::ETS:
            return Language(Language::Id::ETS);
        default:
            ES2PANDA_UNREACHABLE();
    }
}

struct SourceFile {
    SourceFile(std::string_view fn, std::string_view s);
    SourceFile(std::string_view fn, std::string_view s, bool m);
    SourceFile(std::string_view fn, std::string_view s, bool m, std::string_view d);
    SourceFile(std::string_view fn, std::string_view s, std::string_view rp, bool m, bool d);

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    std::string_view filePath {};
    std::string_view fileFolder {};
    std::string_view source {};
    std::string_view resolvedPath {};
    bool isModule {};
    // NOTE(dkofanov): Should be aligned with 'Program::moduleInfo_'.
    bool isDeclForDynamicStaticInterop {};
    bool isExternalSourceImport {};
    std::string_view dest {};
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

// NOLINTBEGIN(modernize-avoid-c-arrays)
inline constexpr char const ERROR_LITERAL[] = "*ERROR_LITERAL*";
inline constexpr char const ERROR_TYPE[] = "*ERROR_TYPE*";
inline constexpr char const INVALID_EXPRESSION[] = "...";
// NOLINTEND(modernize-avoid-c-arrays)

class Compiler {
public:
    explicit Compiler(ScriptExtension ext);
    explicit Compiler(ScriptExtension ext, size_t threadCount);
    explicit Compiler(ScriptExtension ext, size_t threadCount, std::vector<util::Plugin> &&plugins);
    ~Compiler();
    NO_COPY_SEMANTIC(Compiler);
    NO_MOVE_SEMANTIC(Compiler);

    pandasm::Program *Compile(const SourceFile &input, const util::Options &options,
                              util::DiagnosticEngine &diagnosticEngine, uint32_t parseStatus = 0);

    static void DumpAsm(const pandasm::Program *prog);

    const util::ThrowableDiagnostic &GetError() const noexcept
    {
        return error_;
    }

    std::string GetPhasesList() const;

    std::vector<util::Plugin> const &Plugins()
    {
        return plugins_;
    }

private:
    std::vector<util::Plugin> const plugins_ {};
    compiler::CompilerImpl *compiler_ {};
    util::ThrowableDiagnostic error_ {};
    ScriptExtension ext_ {};
};

// g_diagnosticEngine used only for flush diagnostic before unexpected process termination:
// - inside SIGSEGV handler
extern util::DiagnosticEngine *g_diagnosticEngine;
}  // namespace ark::es2panda

#endif
