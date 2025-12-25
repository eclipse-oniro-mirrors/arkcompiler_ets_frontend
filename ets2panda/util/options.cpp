/**
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "options.h"
#include "util/diagnostic.h"
#include "util/diagnosticEngine.h"
#include "util/ustring.h"
#include "libarkbase/os/filesystem.h"
#include "libarkbase/utils/pandargs.h"
#include "arktsconfig.h"
#include "generated/diagnostic.h"

#include <random>
#include <unordered_set>
#include <utility>

#ifdef PANDA_WITH_BYTECODE_OPTIMIZER
#include "bytecode_optimizer/bytecodeopt_options.h"
#include "compiler/compiler_options.h"
#endif

#ifdef ES2PANDA_COMPILE_BY_GN
#include "generated/es2panda_build_info.h"
#endif

namespace ark::es2panda::util {

static std::string Usage(const ark::PandArgParser &argparser)
{
    std::stringstream ss;

    ss << argparser.GetErrorString() << std::endl;
    ss << "Usage: es2panda [OPTIONS] [input file]" << std::endl;
    ss << std::endl;
    ss << "optional arguments:" << std::endl;
    ss << argparser.GetHelpString() << std::endl;
    ss << std::endl;

    return ss.str();
}

static std::string GetVersion()
{
    std::stringstream ss;

    ss << std::endl;
    ss << "  Es2panda Version " << ES2PANDA_VERSION << std::endl;

// add check for PANDA_PRODUCT_BUILD after normal version tracking will be implemented
#ifdef ES2PANDA_DATE
    ss << std::endl;
    ss << "  Build date: ";
    ss << ES2PANDA_DATE;
#endif  // ES2PANDA_DATE
#ifdef ES2PANDA_HASH
    ss << std::endl;
    ss << "  Last commit hash: ";
    ss << ES2PANDA_HASH;
    ss << std::endl;
#endif  // ES2PANDA_HASH

    return ss.str();
}

template <typename T>
bool Options::CallPandArgParser(const std::vector<std::string> &args, T &options,
                                util::DiagnosticEngine &diagnosticEngine)
{
    ark::PandArgParser parser;
    options.AddOptions(&parser);

    if (!parser.Parse(args)) {
        diagnosticEngine.LogFatalError(parser.GetErrorString());
        std::cerr << parser.GetHelpString();
        return false;
    }

    if (auto optionsErr = options.Validate(); optionsErr) {
        diagnosticEngine.LogFatalError(optionsErr.value().GetMessage());
        return false;
    }

    return true;
}

bool Options::CallPandArgParser(const std::vector<std::string> &args)
{
    ark::PandArgParser parser;
    AddOptions(&parser);
    parser.PushBackTail(&inputFile_);
    parser.EnableTail();
    parser.EnableRemainder();
    if (!parser.Parse(args) || IsHelp()) {
        std::cerr << Usage(parser);
        return false;
    }

    if (auto optionsErr = Validate(); optionsErr) {
        diagnosticEngine_.LogFatalError(optionsErr.value().GetMessage());
        return false;
    }

    return true;
}

static std::tuple<std::string_view, std::string_view, std::string_view> SplitPath(std::string_view path)
{
    std::string_view fileDirectory;
    std::string_view fileBaseName = path;
    auto lastDelimPos = fileBaseName.find_last_of(ark::os::file::File::GetPathDelim());
    if (lastDelimPos != std::string_view::npos) {
        ++lastDelimPos;
        fileDirectory = fileBaseName.substr(0, lastDelimPos);
        fileBaseName = fileBaseName.substr(lastDelimPos);
    }

    // Save all extensions.
    std::string_view fileExtensions;
    auto fileBaseNamePos = fileBaseName.find_first_of('.');
    if (fileBaseNamePos > 0 && fileBaseNamePos != std::string_view::npos) {
        fileExtensions = fileBaseName.substr(fileBaseNamePos);
        fileBaseName = fileBaseName.substr(0, fileBaseNamePos);
    }

    return {fileDirectory, fileBaseName, fileExtensions};
}

/**
 * @brief Generate evaluated expression wrapping code.
 * @param sourceFilePath used for generating a unique package name.
 * @param input expression source code file stream.
 * @param output stream for generating expression wrapper.
 */
static void GenerateEvaluationWrapper(std::string_view sourceFilePath, std::ifstream &input, std::stringstream &output)
{
    static constexpr std::string_view EVAL_PREFIX = "eval_";
    static constexpr std::string_view EVAL_SUFFIX = "_eval";

    auto splittedPath = SplitPath(sourceFilePath);
    auto fileBaseName = std::get<1>(splittedPath);

    std::random_device rd;
    std::stringstream ss;
    ss << EVAL_PREFIX << fileBaseName << '_' << rd() << EVAL_SUFFIX;
    auto methodName = ss.str();

    output << "package " << methodName << "; class " << methodName << " { private static " << methodName << "() { "
           << input.rdbuf() << " } }";
}

bool Options::ParseInputOutput()
{
    auto isDebuggerEvalMode = IsDebuggerEval();
    if (isDebuggerEvalMode && compilationMode_ != CompilationMode::SINGLE_FILE) {
        diagnosticEngine_.LogDiagnostic(diagnostic::EVAL_MODE_NOT_SINGLE_INPUT, DiagnosticMessageParams {});
        return false;
    }

    if (compilationMode_ == CompilationMode::SINGLE_FILE || GetExtension() != ScriptExtension::ETS) {
        std::ifstream inputStream(SourceFileName());
        if (inputStream.fail()) {
            diagnosticEngine_.LogDiagnostic(diagnostic::OPEN_FAILED,
                                            util::DiagnosticMessageParams {util::StringView(SourceFileName())});
            return false;
        }

        std::stringstream ss;
        if (isDebuggerEvalMode) {
            GenerateEvaluationWrapper(SourceFileName(), inputStream, ss);
        } else {
            ss << inputStream.rdbuf();
        }
        parserInputContents_ = ss.str();
    }

    if (WasSetOutput()) {
        if (compilationMode_ == CompilationMode::PROJECT) {
            diagnosticEngine_.LogDiagnostic(diagnostic::PROJ_COMP_WITH_OUTPUT, DiagnosticMessageParams {});
            return false;
        }
    } else {
        SetOutput(ark::os::RemoveExtension(BaseName(SourceFileName())).append(".abc"));
    }

    return true;
}

bool Options::Parse(Span<const char *const> args)
{
    std::vector<std::string> es2pandaArgs;
    auto argc = args.size();
    for (size_t i = 1; i < argc; i++) {
        es2pandaArgs.emplace_back(args[i]);
    }

    if (!CallPandArgParser(es2pandaArgs)) {
        return false;
    }

    if (IsVersion()) {
        std::cerr << GetVersion();
        return false;
    }
#ifdef PANDA_WITH_BYTECODE_OPTIMIZER
    if ((WasSetBcoCompiler() && !CallPandArgParser(GetBcoCompiler(), ark::compiler::g_options, diagnosticEngine_)) ||
        (WasSetBcoOptimizer() &&
         !CallPandArgParser(GetBcoOptimizer(), ark::bytecodeopt::g_options, diagnosticEngine_))) {
        return false;
    }
#endif

    DetermineCompilationMode();
    if (!DetermineExtension()) {
        return false;
    }
    if (!ParseInputOutput()) {
        return false;
    }
    if (extension_ != ScriptExtension::JS && IsModule()) {
        diagnosticEngine_.LogDiagnostic(diagnostic::MODULE_INVALID_EXT, DiagnosticMessageParams {});
        return false;
    }

    if ((WasSetDumpEtsSrcBeforePhases() || WasSetDumpEtsSrcAfterPhases()) && extension_ != ScriptExtension::ETS) {
        diagnosticEngine_.LogDiagnostic(diagnostic::DUMP_ETS_INVALID_EXT, DiagnosticMessageParams {});
        return false;
    }

    if (WasSetLogLevel()) {
        logLevel_ = Logger::LevelFromString(GetLogLevel());
    }

    InitCompilerOptions();

    return ProcessEtsSpecificOptions();
}

auto VecToSet(const std::vector<std::string> &v)
{
    return std::set<std::string>(v.begin(), v.end());
}

void Options::InitAstVerifierOptions()
{
    auto initSeverity = [](std::array<bool, gen::ast_verifier::COUNT> *a, const std::vector<std::string> &v) {
        for (const auto &str : v) {
            (*a)[gen::ast_verifier::FromString(str)] = true;
        }
    };
    initSeverity(&verifierWarnings_, gen::Options::GetAstVerifierWarnings());
    initSeverity(&verifierErrors_, gen::Options::GetAstVerifierErrors());

    astVerifierPhases_ = VecToSet(gen::Options::GetAstVerifierPhases());

    if (HasVerifierPhase("before")) {
        astVerifierBeforePhases_ = true;
    }
    if (HasVerifierPhase("each")) {
        astVerifierEachPhase_ = true;
    }
    if (HasVerifierPhase("after")) {
        astVerifierAfterPhases_ = true;
    }
}

void Options::InitCompilerOptions()
{
    skipPhases_ = VecToSet(gen::Options::GetSkipPhases());

    dumpBeforePhases_ = VecToSet(gen::Options::GetDumpBeforePhases());
    dumpEtsSrcBeforePhases_ = VecToSet(gen::Options::GetDumpEtsSrcBeforePhases());
    dumpAfterPhases_ = VecToSet(gen::Options::GetDumpAfterPhases());
    dumpEtsSrcAfterPhases_ = VecToSet(gen::Options::GetDumpEtsSrcAfterPhases());

    InitAstVerifierOptions();

    if (IsEtsWarnings()) {
        InitializeWarnings();
    }
}

void Options::InitializeWarnings()
{
    if (WasSetEtsWarningsBasePath()) {
        diagnosticEngine_.SetBasePath(GetEtsWarningsBasePath());
    }
    std::array<bool, ETSWarnings::COUNT> warningSet {};
    ES2PANDA_ASSERT(ETSWarnings::LAST < ETSWarnings::COUNT);

    const auto processWarningList = [&warningSet](const auto &list, bool v) {
        static const std::map<std::string_view, std::pair<size_t, size_t>> WARNING_GROUPS {
            {"subset_aware", {ETSWarnings::SUBSET_AWARE_FIRST, ETSWarnings::SUBSET_AWARE_LAST}},
            {"subset_unaware", {ETSWarnings::SUBSET_UNAWARE_FIRST, ETSWarnings::SUBSET_UNAWARE_LAST}}};
        const auto setWarningRange = [&warningSet, v](size_t first, size_t last) {
            ES2PANDA_ASSERT(last < ETSWarnings::COUNT);
            for (size_t i = first; i <= last; i++) {
                warningSet[i] = v;
            }
        };
        for (const auto &warningOrGroup : list) {
            if (WARNING_GROUPS.find(warningOrGroup) != WARNING_GROUPS.end()) {
                auto [first, last] = WARNING_GROUPS.at(warningOrGroup);
                setWarningRange(first, last);
                continue;
            }
            ES2PANDA_ASSERT(ets_warnings::FromString(warningOrGroup) != ETSWarnings::INVALID);
            warningSet[ets_warnings::FromString(warningOrGroup)] = v;
        }
    };
    processWarningList(GetEtsWarningsEnable(), true);
    processWarningList(GetEtsWarningsDisable(), false);
    for (size_t i = ETSWarnings::FIRST; i <= ETSWarnings::LAST; i++) {
        if (warningSet[i]) {
            etsWarningCollection_.emplace_back(static_cast<ETSWarnings>(i));
        }
    }
}

bool Options::DetermineExtension()
{
    if (compilationMode_ == CompilationMode::PROJECT) {
        if (WasSetExtension() && gen::Options::GetExtension() != "ets") {
            diagnosticEngine_.LogDiagnostic(diagnostic::PROJECT_EXT_NOT_ETS, DiagnosticMessageParams {});
            return false;
        }
        extension_ = ScriptExtension::ETS;
        return true;
    }
    std::string sourceFileExtension = SourceFileName().substr(SourceFileName().find_last_of('.') + 1);
    if (!SourceFileName().empty() && WasSetExtension() && gen::Options::GetExtension() != sourceFileExtension) {
        diagnosticEngine_.LogDiagnostic(diagnostic::EXTENSION_MISMATCH,
                                        DiagnosticMessageParams {std::string_view(sourceFileExtension),
                                                                 std::string_view(gen::Options::GetExtension())});
    }
    // Note: the file suffix `.ets` is a valid suffix for compiler, which is equivalent to `.ets`
    sourceFileExtension = sourceFileExtension == "ets" ? "ets" : sourceFileExtension;
    auto tempExtension = WasSetExtension() ? gen::Options::GetExtension() : sourceFileExtension;
    if (gen::extension::FromString(tempExtension) == ScriptExtension::INVALID) {
        diagnosticEngine_.LogDiagnostic(diagnostic::UNKNOWN_EXT, DiagnosticMessageParams {});
        return false;
    }

    extension_ = gen::extension::FromString(tempExtension);
    switch (extension_) {
#ifndef PANDA_WITH_ECMASCRIPT
        case ScriptExtension::JS: {
            diagnosticEngine_.LogDiagnostic(diagnostic::JS_UNSUPPORTED, util::DiagnosticMessageParams {});
            return false;
        }
#endif
        case ScriptExtension::ETS: {
            std::ifstream inputStream(GetArktsconfig());
            if (inputStream.fail()) {
                diagnosticEngine_.LogDiagnostic(diagnostic::OPEN_FAILED_ARKTSCONF,
                                                util::DiagnosticMessageParams {GetArktsconfig()});
                return false;
            }
            return true;
        }
        default:
            return true;
    }
}

bool Options::ProcessEtsSpecificOptions()
{
    if (GetExtension() != ScriptExtension::ETS) {
        return true;
    }

    if (auto config = ParseArktsConfig(); config != std::nullopt) {
        arktsConfig_ = std::make_shared<ArkTsConfig>(*config);
        return true;
    }

    return false;
}

std::optional<ArkTsConfig> Options::ParseArktsConfig()
{
    auto config = ArkTsConfig {GetArktsconfig(), diagnosticEngine_};
    if (!config.Parse()) {
        diagnosticEngine_.LogDiagnostic(diagnostic::INVALID_ARKTSCONFIG,
                                        util::DiagnosticMessageParams {util::StringView(GetArktsconfig())});
        return std::nullopt;
    }
    config.ResolveAllDependenciesInArkTsConfig();
    return std::make_optional(config);
}

}  // namespace ark::es2panda::util
