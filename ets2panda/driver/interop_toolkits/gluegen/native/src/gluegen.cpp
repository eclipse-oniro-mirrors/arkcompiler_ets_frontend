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

#include "gluegen.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include "logger/log.h"
#include "public/es2panda_lib.h"
#include "public/public.h"
#include "libarkbase/utils/expected.h"
#include "parser/program/program.h"
#include "es2panda.h"
#include "nlohmann/json.hpp"
#include "./gluec.h"
#include "./gluel.h"
#include "util/es2pandaMacros.h"
#include "utils.h"

namespace ark::es2panda::gluegen {

namespace {

// Writes `content` to `outputPath`, creating its parent directory first if needed. Shared by
// Gluegen::Link() (the normal GlueConfig JSON output) and Gluegen::WriteStatusOutput() (the
// early-exit "syntax-error" status file), so the create-directories+open+write+check sequence
// only needs to be written (and checked for errors) once.
ark::Expected<std::monostate, std::string> WriteFile(const fs::path &outputPath, const std::string &content)
{
    std::error_code ec;
    if (outputPath.has_parent_path()) {
        fs::create_directories(ToLongPathIfNeeded(outputPath.parent_path()), ec);
        if (ec) {
            return ark::Unexpected<std::string>(
                "Failed to create output directory: " + outputPath.parent_path().string() + ": " + ec.message());
        }
    }

    std::ofstream out(ToLongPathIfNeeded(outputPath), std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return ark::Unexpected<std::string>("Failed to open output file for writing: " + outputPath.string());
    }
    out << content;
    if (!out) {
        return ark::Unexpected<std::string>("Failed to write output file: " + outputPath.string());
    }
    return std::monostate {};
}

// Releases everything inside an es2panda Context that must happen *before* DestroyContext frees the
// arena: clears diagnostics (so EnsureLocations is a no-op), then releases every Program's heap-owned
// lineIndex_ (arena-allocated Programs don't run their destructor). Extracted from Reset to keep that
// function flat; `ctx` must be non-null.
void PrepareContextForDestruction(ark::es2panda::public_lib::Context *ctx)
{
    // DestroyContext() runs EnsureLocations() on the config's diagnostic engine, which resolves
    // diagnostic positions through Program::GetLineIndex() and thereby lazily allocates
    // Program::lineIndex_ (a heap-owned std::unique_ptr). Clear the diagnostics first so that
    // EnsureLocations() becomes a no-op and cannot re-materialize lineIndex_ after it has been
    // released below.
    if (ctx->config != nullptr && ctx->config->diagnosticEngine != nullptr) {
        ctx->config->diagnosticEngine->ClearDiagnostics();
    }
    ctx->diagnosticEngine->EnsureLocations();
    // Program is arena-allocated: tearing the arena down does not run Program's destructor, so
    // lineIndex_ would leak unless it is released explicitly before DestroyContext() frees the
    // arena. Mirrors compilerImpl's ResetLineIndexCaches (main program + external decls).
    if (ctx->parserProgram != nullptr) {
        ctx->parserProgram->ResetLineIndexCache();
        ctx->parserProgram->GetExternalDecls()->Visit([](auto *extProgram) { extProgram->ResetLineIndexCache(); });
    }
    // The stdlib imports program ("<default_import>.ets") is introduced during binding and is
    // registered only in the ImportPathManager's parse queue, not in parserProgram's external
    // decls, so the Visit above misses it. Walk the parse queue too so every parsed program's
    // heap-owned lineIndex_ is released before DestroyContext() frees the arena. The parse queue
    // is not cleared in the simultaneous-mode flow, so it still holds these programs at teardown.
    if (ctx->parser == nullptr) {
        return;
    }
    auto *ipm = ctx->parser->GetImportPathManager();
    if (ipm == nullptr) {
        return;
    }
    for (const auto &parseInfo : ipm->GetParseQueue()) {
        if (parseInfo.program != nullptr) {
            parseInfo.program->ResetLineIndexCache();
        }
    }
}

// Strips known source-file extensions (.d.ets, .d.ts, .ets, .ts) from `filename`, returning the
// stem. Longer patterns are checked first so ".d.ets" isn't mis-recognized as ".ets".
// On case-insensitive filesystems (Windows, macOS) the comparison is case-insensitive so that
// "foo.ETS" / "foo.Ets" are recognised -- the OS itself treats those as the same file. On
// case-sensitive filesystems (Linux) the comparison stays case-sensitive, because "foo.ETS" and
// "foo.ets" really are two distinct files there. Every known extension is pure ASCII, so
// per-character std::tolower is sufficient for the case-insensitive path.
std::string StripSourceExtension(const std::string &filename)
{
    static constexpr std::pair<const char *, std::size_t> EXTENSIONS[] = {
        {".d.ets", 6},
        {".d.ts", 5},
        {".ets", 4},
        {".ts", 3},
    };
    for (const auto &[ext, len] : EXTENSIONS) {
        if (filename.size() < len) {
            continue;
        }
#if defined(_WIN32) || defined(__APPLE__)
        // Case-insensitive filesystem: compare without regard to case.
        bool matches = true;
        for (std::size_t i = 0; i < len; ++i) {
            if (std::tolower(static_cast<unsigned char>(filename[filename.size() - len + i])) !=
                std::tolower(static_cast<unsigned char>(ext[i]))) {
                matches = false;
                break;
            }
        }
        if (matches) {
            return filename.substr(0, filename.size() - len);
        }
#else
        // Case-sensitive filesystem (Linux): compare exactly.
        if (filename.compare(filename.size() - len, len, ext) == 0) {
            return filename.substr(0, filename.size() - len);
        }
#endif
    }
    return filename;
}

// `reportPath` is normally a *file* path (where the diagnostics report is written), but for
// convenience the user may also point it at a directory -- in that case, the report is written to
// "<reportPath>/report.json" instead. A path is treated as a directory when it already exists as
// one on disk, or when it's spelled with a trailing path separator (so a not-yet-created directory
// can still be requested explicitly, e.g. "--report-path ./out/").
fs::path ResolveDiagnosticsReportFilePath(const std::string &reportPath)
{
    static constexpr const char *defaultReportFileName = "report.json";
    fs::path path(reportPath);
    const bool endsWithSeparator = !reportPath.empty() && (reportPath.back() == '/' || reportPath.back() == '\\');
    std::error_code ec;
    const bool isExistingDirectory = fs::is_directory(ToLongPathIfNeeded(path), ec);
    if (endsWithSeparator || isExistingDirectory) {
        return path / defaultReportFileName;
    }
    return path;
}

// Prefix/suffix every per-run log file is named with, so PruneOldLogFiles() below can recognize
// which files in the directory are ones it's allowed to manage (and not, say, a manifest.json or
// some unrelated file a caller happens to keep in the same cache directory).
constexpr const char *LOG_FILE_PREFIX = "gluegen-";
constexpr const char *LOG_FILE_SUFFIX = ".log";

// Builds a log file name unique to this run (down to the microsecond), e.g.
// "gluegen-20260728-153045-123456.log", so every Gluegen::Initialize() call writes its own,
// independent log file instead of appending to (or clobbering) a previous run's.
std::string GenerateLogFileName()
{
    const auto now = std::chrono::system_clock::now();
    const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000;
    const auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf {};
#if defined(_WIN32)
    if (localtime_s(&tmBuf, &timeT) != 0) {
#else
    if (localtime_r(&timeT, &tmBuf) == nullptr) {
#endif
        // timeT cannot be broken down (out of representable range): reset to a zeroed tm (the
        // epoch) so put_time still yields a valid, unique file name rather than garbage contents.
        tmBuf = {};
    }
    std::ostringstream oss;
    constexpr int setwWidth = 6;
    oss << LOG_FILE_PREFIX << std::put_time(&tmBuf, "%Y%m%d-%H%M%S") << '-' << std::setfill('0') << std::setw(setwWidth)
        << micros << LOG_FILE_SUFFIX;
    return oss.str();
}

// Deletes every "gluegen-*.log" file in `dir` except the `maxCount` most recently modified ones,
// so a long-lived cache directory doesn't accumulate an unbounded number of per-run log files.
// Called once per run, after this run's own log file has already been created (see Initialize()),
// so it is naturally counted among the files being kept.
void PruneOldLogFiles(const fs::path &dir, std::size_t maxCount, log::Logger &logger)
{
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) {
        if (ec) {
            GLUEGEN_LOG_WARN(logger) << "Failed to check log directory for pruning: " << dir.string() << " ("
                                     << ec.message() << ")";
        }
        return;
    }

    std::vector<fs::directory_entry> logFiles;
    for (const auto &entry : fs::directory_iterator(dir, ec)) {
        if (ec) {
            GLUEGEN_LOG_WARN(logger) << "Failed to iterate log directory for pruning: " << dir.string() << " ("
                                     << ec.message() << ")";
            return;
        }
        const auto fileName = entry.path().filename().string();
        const bool hasPrefix = fileName.compare(0, std::strlen(LOG_FILE_PREFIX), LOG_FILE_PREFIX) == 0;
        const auto suffixLen = std::strlen(LOG_FILE_SUFFIX);
        const bool hasSuffix = fileName.size() >= suffixLen &&
                               fileName.compare(fileName.size() - suffixLen, suffixLen, LOG_FILE_SUFFIX) == 0;
        if (fs::is_regular_file(entry.path(), ec) && hasPrefix && hasSuffix) {
            logFiles.push_back(entry);
        }
    }
    if (logFiles.size() <= maxCount) {
        return;
    }

    // Newest (most recently written) first, so the files kept are the maxCount most recent ones.
    std::sort(logFiles.begin(), logFiles.end(), [](const auto &lhs, const auto &rhs) {
        std::error_code lhsEc;
        std::error_code rhsEc;
        return fs::last_write_time(lhs, lhsEc) > fs::last_write_time(rhs, rhsEc);
    });
    for (std::size_t i = maxCount; i < logFiles.size(); ++i) {
        fs::remove(logFiles[i], ec);
        if (ec) {
            GLUEGEN_LOG_WARN(logger) << "Failed to prune old log file: " << logFiles[i].path().string() << " ("
                                     << ec.message() << ")";
            ec.clear();
        }
    }
}
}  // namespace

Es2pandaSession::Es2pandaSession(const es2panda_Impl *impl, es2panda_Config *config, es2panda_Context *context)
    : impl_(impl), config_(config), context_(context)
{
}

Es2pandaSession::Es2pandaSession(Es2pandaSession &&other) noexcept
    : impl_(other.impl_), config_(other.config_), context_(other.context_)
{
    other.impl_ = nullptr;
    other.config_ = nullptr;
    other.context_ = nullptr;
}

Es2pandaSession &Es2pandaSession::operator=(Es2pandaSession &&other) noexcept
{
    if (this != &other) {
        Reset();
        impl_ = other.impl_;
        config_ = other.config_;
        context_ = other.context_;
        other.impl_ = nullptr;
        other.config_ = nullptr;
        other.context_ = nullptr;
    }
    return *this;
}

Es2pandaSession::~Es2pandaSession()
{
    Reset();
}

void Es2pandaSession::Reset()
{
    // Context must be destroyed before Config: a live Context holds a pointer back into Config
    // (options/diagnosticEngine) that must stay valid for the duration of DestroyContext.
    if (context_ != nullptr) {
        auto *ctx = reinterpret_cast<ark::es2panda::public_lib::Context *>(context_);
        PrepareContextForDestruction(ctx);
        impl_->DestroyContext(context_);
        context_ = nullptr;
    }
    if (config_ != nullptr) {
        impl_->DestroyConfigWithoutLog(config_);
        config_ = nullptr;
    }
}

ark::Expected<fs::path, std::string> ValidateArktsconfigPath(const std::string &arktsconfigPath)
{
    if (arktsconfigPath.empty()) {
        return ark::Unexpected<std::string>("Arktsconfig path was not specified");
    }
    auto path = fs::path(arktsconfigPath);
    std::error_code ec;
    if (!fs::exists(ToLongPathIfNeeded(path), ec)) {
        return ark::Unexpected<std::string>("Arktsconfig file does not exist: " + path.string());
    }
    return path;
}

std::vector<const char *> BuildCreateConfigArgv(const std::string &arktsconfigPath,
                                                const std::string &etsWarningsBasePathArg)
{
    static constexpr const char *simultaneous = "--simultaneous";
    static constexpr const char *arktsconfig = "--arktsconfig";
    static constexpr const char *extension = "--extension";
    static constexpr const char *extensionValue = "ets";
    // Reaching ES2PANDA_STATE_BOUND runs the `plugins-after-parse` phase, after which the AST
    // verifier's CheckStructDeclaration invariant fires (see options.yaml's ast-verifier:errors
    // default list). It expects `struct` (UI/ArkUI) syntax to have already been lowered to plain
    // classes by that plugin phase; since gluegen doesn't register the ArkUI plugin, any `struct`
    // declaration would still be present and this check would LOG(FATAL, ...) -- an actual process
    // abort, not a recoverable diagnostic. We keep every other default "error" invariant active and
    // only demote CheckStructDeclaration to a warning, since gluegen intentionally doesn't lower UI
    // syntax. NOTE: mirrors options.yaml's ast-verifier:errors default minus CheckStructDeclaration
    // -- keep in sync if that default list changes upstream.
    static constexpr const char *astVerifierErrors =
        "--ast-verifier:errors="
        "ArithmeticOperationValid:CheckAbstractMethod:EveryChildHasValidParent:ForLoopCorrectlyInitialized:"
        "GetterSetterValidation:IdentifierHasVariable:ModifierAccessValid:NodeHasParent:NodeHasType:"
        "NoPrimitiveTypes:EnumHasCorrectType:ReferenceTypeAnnotationIsNull:SequenceExpressionHasLastType:"
        "VariableHasEnclosingScope:VariableHasScope:VariableNameIdentifierNameSame";
    static constexpr const char *astVerifierWarnings = "--ast-verifier:warnings=CheckStructDeclaration";

    std::vector<const char *> argv;
    constexpr size_t argvReserve = 9;  // number of entries pushed below
    argv.reserve(argvReserve);
    // `--simultaneous` puts Options into CompilationMode::SIMULTANEOUS (not PROJECT), so
    // Options::DetermineExtension() can't fall back to its "always ETS" shortcut. Since no
    // positional source file is passed to CreateConfig() either (files are only given later, to
    // CreateContextSimultaneousMode), it has nothing to infer the extension from and fails with
    // "Unknown extension of sourcefile" unless we pass --extension explicitly here. This mirrors
    // the ordering used by the build system driver's formCompileCliCmd()/formDeclgenCliCmd() in
    // driver/build_system/src/util/ets2panda.ts.
    argv.push_back("_");
    argv.push_back(extension);
    argv.push_back(extensionValue);
    argv.push_back(arktsconfig);
    argv.push_back(arktsconfigPath.c_str());
    argv.push_back(simultaneous);
    argv.push_back(astVerifierErrors);
    argv.push_back(astVerifierWarnings);
    // Note: intentionally no --parse-only here. GlobalClass()/GlobalClassHandler and import/scope
    // resolution only run as part of the Bind() phase list (reached via ES2PANDA_STATE_BOUND), and
    // --parse-only would make ResolveIdentifiers::Perform() bail out early during that phase.
    // Sets the DiagnosticEngine's basePath_ so diagnostic messages report file paths relative to
    // the working directory instead of absolute paths (same option the build system driver passes
    // as `--ets-warnings:base-path=` + this.projectRootPath).
    argv.push_back(etsWarningsBasePathArg.c_str());
    return argv;
}

std::vector<fs::path> NormalizeSourceFiles(const std::vector<std::string> &sourceFiles)
{
    std::vector<fs::path> result;
    result.reserve(sourceFiles.size());
    for (const auto &sourceFile : sourceFiles) {
        result.emplace_back(NormalizePath(fs::path(sourceFile)));
    }
    return result;
}

// GetAllErrorMessages() returns every collected diagnostic (one per line, formatted as
// "[file:line:col] <Type>: <message>") regardless of DiagnosticType -- Fatal, Syntax, Semantic,
// ArkTS config errors, etc. all get mixed together. This keeps only the genuine "Syntax error"
// lines, dropping everything else (notably Fatal errors like F0016 "Can't find prefix for '...'
// in arktsconfig.json", which commonly fires because gluegen's arktsconfig doesn't map every
// import gluegen happens to see).
// NOTE: this only trims the *reported message text*. It does not make parsing succeed: a Fatal
// diagnostic still marks the context as errored (DiagnosticEngine::IsError() unconditionally
// treats FATAL as an error, it's not configurable like the AST-verifier invariants), so
// Initialize() still fails overall whenever one occurs -- callers just won't see it in the text
// unless no "Syntax error" lines were present either, in which case a generic message is used.
std::string KeepOnlySyntaxErrors(const std::string &allMessages)
{
    std::string result;
    std::size_t pos = 0;
    while (pos < allMessages.size()) {
        auto newlinePos = allMessages.find('\n', pos);
        auto line =
            (newlinePos == std::string::npos) ? allMessages.substr(pos) : allMessages.substr(pos, newlinePos - pos);
        if (line.find("Syntax error") != std::string::npos) {
            result += line;
            result += '\n';
        }
        if (newlinePos == std::string::npos) {
            break;
        }
        pos = newlinePos + 1;
    }
    return result;
}

// Joins argv's entries with a single space, for embedding into a diagnostic message (e.g. so a
// CreateConfig() failure's message shows exactly what arguments were passed to it).
std::string JoinArgv(const std::vector<const char *> &argv)
{
    std::string result;
    for (const auto *arg : argv) {
        if (arg == nullptr) {
            continue;
        }
        if (!result.empty()) {
            result += ' ';
        }
        result += arg;
    }
    return result;
}

// Creates an es2panda Config and simultaneous-mode Context from `arktsconfigPath` for the given
// `files`, returning a move-only Es2pandaSession that owns both.  The returned session is in the
// NEW state -- call ParseFilesSimultaneously() next to advance it to BOUND.
// On any failure (bad arktsconfig, failed CreateConfig, failed CreateContext, ...) returns an
// error string directly -- no ParseFailure indirection needed because callers only care about
// "did init succeed" at this point.
ark::Expected<Es2pandaSession, std::string> InitEs2panda(const es2panda_Impl *impl, const fs::path &arktsconfigPath,
                                                         const std::string &workingDirectory,
                                                         const std::vector<fs::path> &files)
{
    const std::string etsWarningsBasePathArg = "--ets-warnings:base-path=" + workingDirectory;
    const std::string arktsconfigPathStr = arktsconfigPath.string();
    auto createConfigArgv = BuildCreateConfigArgv(arktsconfigPathStr, etsWarningsBasePathArg);
    auto *config = impl->CreateConfig(static_cast<int>(createConfigArgv.size()), createConfigArgv.data());
    if (config == nullptr) {
        // On failure, CreateConfig() internally runs Options::Parse(), which logs the actual
        // reason into a DiagnosticEngine it allocates on the heap -- then returns nullptr without
        // exposing that engine or freeing it (it's leaked). DiagnosticEngine's constructor stashes
        // `this` into the global `g_diagnosticEngine`, so we can pull the formatted text back out
        // of it best-effort.
        std::string diagnosticText;
        if (ark::es2panda::g_diagnosticEngine != nullptr) {
            diagnosticText = ark::es2panda::g_diagnosticEngine->PrintAndFlushErrorDiagnostic();
        }
        const std::string argvText = JoinArgv(createConfigArgv);
        if (!diagnosticText.empty()) {
            return ark::Unexpected<std::string>("Failed to create es2panda configuration: " + diagnosticText +
                                                " (createConfigArgv: " + argvText + ")");
        }
        return ark::Unexpected<std::string>("Failed to create es2panda configuration (createConfigArgv: " + argvText +
                                            ")");
    }

    // unique_ptr with custom deleter so `config` is cleaned up if CreateContext fails below.
    auto configDeleter = [impl](es2panda_Config *cfg) {
        if (cfg != nullptr) {
            impl->DestroyConfigWithoutLog(cfg);
        }
    };
    std::unique_ptr<es2panda_Config, decltype(configDeleter)> configGuard(config, configDeleter);

    std::vector<std::string> sourceFileStrings;
    sourceFileStrings.reserve(files.size());
    for (const auto &sourceFile : files) {
        sourceFileStrings.push_back(sourceFile.string());
    }
    std::vector<const char *> sourceFilePtrs;
    sourceFilePtrs.reserve(sourceFileStrings.size());
    for (const auto &sourceFileString : sourceFileStrings) {
        sourceFilePtrs.push_back(sourceFileString.c_str());
    }

    auto *context = impl->CreateContextSimultaneousMode(configGuard.get(), static_cast<int>(sourceFilePtrs.size()),
                                                        sourceFilePtrs.data());
    if (context == nullptr) {
        return ark::Unexpected<std::string>("Failed to create es2panda simultaneous parsing context");
    }

    // Both config and context were created successfully -- release guards and package them into
    // an Es2pandaSession (which takes ownership and manages destruction order).
    return Es2pandaSession(impl, configGuard.release(), context);
}

// Advances `session` (already initialised by InitEs2panda) to ES2PANDA_STATE_BOUND and checks
// for errors.  Returns monostate on success, or the syntax-error message text via Unexpected
// when the source contains syntax errors.  The caller translates that into a ParseOutcome.
// The caller retains ownership of `session` regardless of outcome -- even after a syntax error
// the session is still valid, so GetRootDirFromArktsConfig() and similar APIs remain usable.
ark::Expected<std::monostate, std::string> ParseFilesSimultaneously(Es2pandaSession &session)
{
    // WARNING: The following code needs to be careful reviewed.
    auto *impl = session.Impl();
    auto *context = session.Context();

    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);

    if (impl->ContextState(context) == ES2PANDA_STATE_ERROR || impl->IsAnyError(context)) {
        auto *errors = impl->GetAllErrorMessages(context);
        if (errors != nullptr && std::strlen(errors) > 0) {
            return ark::Unexpected<std::string>(std::string(errors));
        }
    }
    // ES2PANDA_STATE_BOUND (not just PARSED) is required: Program::GlobalClass() is only
    // populated by the TopLevelStatements/GlobalClassHandler compiler phase, which runs as part of
    // Bind() -- stopping at PARSED leaves GlobalClass() null for every program.
    impl->ProceedToState(context, ES2PANDA_STATE_BOUND);

    return std::monostate {};
}

Gluegen::Gluegen(const GluegenOptions &options, DiagnosticConsumer *consumer) : options_(options)
{
    context_.diagnosticEngine.SetConsumer(consumer);
}

ark::Expected<std::monostate, std::string> Gluegen::Initialize()
{
    // Both of the following must happen before InitializeOptions() runs, not after:
    //  - InitializeOptions() itself reads context_.workingDirectory to fill in default
    //    output/cache/arktsconfig paths, so leaving it unset until afterward means those
    //    defaults would be resolved against an empty path instead of the real cwd.
    //  - The Logger's sinks must be attached and its backend thread started before any log call
    //    is made -- including calls made from within InitializeOptions() itself -- otherwise
    //    those records have nowhere to go.
    context_.workingDirectory = fs::current_path().string();

    // This is must be called after context_.workingDirectory is set!
    InitializeOptions();

    // Start the logger thread and attach a file sink if caching is enabled. The logger must be
    // started before any log call is made, including calls made from within InitializeOptions()
    if (cacheEnabled_) {
        // Only the 5 most recent runs' log files are kept -- see PruneOldLogFiles() -- so the
        // cache directory doesn't accumulate an unbounded number of them over time.
        static constexpr std::size_t MAX_LOG_FILES_KEPT = 5;
        const auto logDir = fs::path(options_.cacheDir);
        std::error_code ec;
        fs::create_directories(logDir, ec);
        const auto logPath = logDir / GenerateLogFileName();
        context_.logger.AddSink(std::make_shared<log::FileSink>(logPath.string()));
        PruneOldLogFiles(logDir, MAX_LOG_FILES_KEPT, context_.logger);
    }
#ifndef NDEBUG
    context_.logger.SetLevel(log::LogLevel::DEBUG);
#endif
    context_.logger.Start();

    GLUEGEN_LOG_INFO(context_.logger) << "Gluegen initialized with working directory: " << context_.workingDirectory;
    GLUEGEN_LOG_DEBUG(context_.logger) << "Validating Options..." << context_.workingDirectory;
    if (auto result = ValidateOptions(); !result) {
        GLUEGEN_LOG_ERROR(context_.logger) << "Failed to validate options: " << result.Error();
        return ark::Unexpected<std::string>(std::move(result.Error()));
    }

    return std::monostate {};
}

ark::Expected<ParseOutcome, std::string> Gluegen::Parse()
{
    GLUEGEN_LOG_INFO(context_.logger) << "Parsing with es2panda...";
    GLUEGEN_LOG_DEBUG(context_.logger) << "Getting es2panda public implementation...";
    auto *impl = es2panda_GetImpl(ES2PANDA_LIB_VERSION);
    if (impl == nullptr) {
        const std::string message = "Failed to load es2panda public implementation.";
        GLUEGEN_LOG_ERROR(context_.logger) << message;
        context_.diagnosticEngine.Error(DiagnosticCode::INVALID_CONFIG, message);
        return ark::Unexpected<std::string>(message);
    }

    GLUEGEN_LOG_DEBUG(context_.logger) << "Validating arktsconfig path: " << options_.arktsconfigPath;
    auto arktsconfigPath = ValidateArktsconfigPath(options_.arktsconfigPath);
    if (!arktsconfigPath) {
        const std::string message = "Invalid arktsconfig path: " + arktsconfigPath.Error();
        GLUEGEN_LOG_ERROR(context_.logger) << message;
        context_.diagnosticEngine.Error(DiagnosticCode::INVALID_CONFIG, arktsconfigPath.Error());
        return ark::Unexpected<std::string>(std::move(arktsconfigPath.Error()));
    }

    auto normalizedSourceFiles = NormalizeSourceFiles(options_.sourceFiles);

    GLUEGEN_LOG_INFO(context_.logger) << "Parsing files simultaneously, source files are: "
                                      << Joined(normalizedSourceFiles);

    // Phase 1: initialise es2panda (creates Config + simultaneous Context).
    auto session = InitEs2panda(impl, arktsconfigPath.Value(), context_.workingDirectory, normalizedSourceFiles);
    if (!session) {
        GLUEGEN_LOG_ERROR(context_.logger) << "InitEs2panda failed: " << session.Error();
        context_.diagnosticEngine.Error(DiagnosticCode::INVALID_CONFIG, session.Error());
        return ark::Unexpected<std::string>(std::move(session.Error()));
    }
    session_ = std::move(session.Value());
    // session_ is valid from this point on -- even a subsequent syntax error won't invalidate it,
    // so GetRootDirFromArktsConfig() (and any other session-based API) works in error paths.

    // Phase 2: advance to BOUND and check for syntax errors.
    auto parseResult = ParseFilesSimultaneously(session_);
    if (!parseResult) {
        // A syntax error in the source is a normal, terminal outcome -- not a tool failure.
        GLUEGEN_LOG_WARN(context_.logger) << "Syntax error, gluegen will continue. Details: " << parseResult.Error();
        context_.diagnosticEngine.Warning(DiagnosticCode::SOURCE_PARSE_FAILED, parseResult.Error());
        return ParseOutcome::SYNTAX_ERROR;
    }

    return ParseOutcome::READY;
}

ark::Expected<std::monostate, std::string> Gluegen::WriteDiagnosticsReport()
{
    const auto reportPath = ResolveDiagnosticsReportFilePath(options_.reportPath);
    auto jsonReport = nlohmann::json::object();
    jsonReport["diagnostics"] = nlohmann::json::object();
    jsonReport["diagnostics"]["warnings"] = nlohmann::json::array();
    jsonReport["diagnostics"]["errors"] = nlohmann::json::array();
    for (const auto &diagnostic : context_.diagnosticEngine.Records()) {
        if (diagnostic.severity == DiagnosticSeverity::WARNING) {
            jsonReport["diagnostics"]["warnings"].push_back(diagnostic.diagnostic);
        } else if (diagnostic.severity == DiagnosticSeverity::ERROR) {
            jsonReport["diagnostics"]["errors"].push_back(diagnostic.diagnostic);
        }
    }
    GLUEGEN_LOG_INFO(context_.logger) << "Writing report into " << reportPath.string() << "...";
    constexpr int jsonIndent = 2;  // matches nlohmann::json::dump() default
    return WriteFile(reportPath, jsonReport.dump(jsonIndent));
}

ark::Expected<std::monostate, std::string> Gluegen::Run()
{
    auto result = RunImpl();
    auto reportResult = WriteDiagnosticsReport();
    if (!result) {
        // The run's own failure is the more actionable one to surface -- don't mask it with a
        // failure to write the (best-effort) diagnostics report.
        GLUEGEN_LOG_ERROR(context_.logger) << "Gluegen run failed.";
        return result;
    }
    if (!reportResult) {
        GLUEGEN_LOG_ERROR(context_.logger) << "Failed to write report: " << reportResult.Error();
        return ark::Unexpected<std::string>(std::move(reportResult.Error()));
    }
    return result;
}

ark::Expected<std::monostate, std::string> Gluegen::RunImpl()
{
    if (auto initResult = Initialize(); !initResult) {
        GLUEGEN_LOG_ERROR(context_.logger) << "Gluegen initialization failed: " << initResult.Error();
        return ark::Unexpected<std::string>(std::move(initResult.Error()));
    }
    auto parseResult = Parse();
    if (!parseResult) {
        GLUEGEN_LOG_ERROR(context_.logger) << "Gluegen failed in parse stage!";
        return ark::Unexpected<std::string>(std::move(parseResult.Error()));
    }
    if (parseResult.Value() == ParseOutcome::SYNTAX_ERROR) {
        // Terminate the generation flow here (skip Parse()/Link() entirely) but still record the
        // outcome on disk, so a caller that only inspects options_.outputPath (rather than Run()'s
        // return value) can tell the source had a syntax error. A syntax error is a normal,
        // successful outcome for Run() -- gluegen did its job by determining the source doesn't
        // compile -- so this still returns std::monostate rather than ark::Unexpected.
        if (auto writeResult = WriteStatusOutput("syntax-error"); !writeResult) {
            GLUEGEN_LOG_WARN(context_.logger)
                << "Syntax error! Gluegen will continue and generate the final result with status 'syntax-error'.";
            return ark::Unexpected<std::string>(std::move(writeResult.Error()));
        }
        GLUEGEN_LOG_ERROR(context_.logger) << "Gluegen failed in parse stage!";
        return std::monostate {};
    }
    auto collectResult = Collect();
    if (!collectResult) {
        GLUEGEN_LOG_ERROR(context_.logger) << "Gluegen failed in collect stage!";
        return ark::Unexpected<std::string>(std::move(collectResult.Error()));
    }
    auto linkResult = Link();
    if (!linkResult) {
        GLUEGEN_LOG_ERROR(context_.logger) << "Gluegen failed in link stage!";
        return ark::Unexpected<std::string>(std::move(linkResult.Error()));
    }
    return std::monostate {};
}

namespace {
// Abstracts over "is incremental disk caching active" so Gluegen::Parse()'s main loop doesn't
// need to branch on cacheEnabled_ at all -- Null Object pattern: NullCacheStore below makes "no
// cache directory configured" just another ICacheStore (one that always misses and never writes),
// rather than a separately-coded path duplicating the cache-enabled loop.
class ICacheStore {
public:
    ICacheStore() = default;
    virtual ~ICacheStore() = default;

    NO_COPY_SEMANTIC(ICacheStore);
    NO_MOVE_SEMANTIC(ICacheStore);

    // The manifest-recorded mtime for `sourceFile`, or std::nullopt if there is none (no cache
    // configured, cold cache, or this file was never cached before).
    virtual std::optional<std::string> CachedMTime(const std::string &sourceFile) const = 0;
    // Asynchronously loads `sourceFile`'s cache, invoking `onLoaded` with it (nullptr on any
    // failure) once ready.
    virtual void EnqueueRead(const std::string &sourceFile,
                             std::function<void(std::shared_ptr<IntermediateCache>)> onLoaded) = 0;
    // Asynchronously persists `cache` to disk, if this store persists anything at all.
    virtual void EnqueueWrite(std::shared_ptr<IntermediateCache> cache) = 0;
    // Blocks until every previously-EnqueueRead'd load has finished.
    virtual void WaitReads() = 0;
    // Blocks until every previously-EnqueueWrite'd write has finished, returning any errors.
    virtual std::vector<std::string> WaitWrites() = 0;
};

// The "no cache directory configured" case: every lookup misses (forcing a fresh Gluec parse for
// every file) and nothing is ever written to disk.
class NullCacheStore final : public ICacheStore {
public:
    std::optional<std::string> CachedMTime(const std::string & /*sourceFile*/) const override
    {
        return std::nullopt;
    }
    void EnqueueRead(const std::string & /*sourceFile*/,
                     std::function<void(std::shared_ptr<IntermediateCache>)> onLoaded) override
    {
        // Never actually invoked by Parse() (CachedMTime always misses here), but implemented
        // correctly regardless so this class stays a safe, honest ICacheStore on its own.
        onLoaded(nullptr);
    }
    void EnqueueWrite(std::shared_ptr<IntermediateCache> /*cache*/) override {}
    void WaitReads() override {}
    std::vector<std::string> WaitWrites() override
    {
        return {};
    }
};

// The real, disk-backed cache: thin adapter composing IntermediateCacheReader/Writer behind the
// ICacheStore interface.
class DiskCacheStore final : public ICacheStore {
public:
    DiskCacheStore(const std::string &cacheDir, Context &context)
        : reader_(cacheDir, context), writer_(cacheDir, context)
    {
    }

    std::optional<std::string> CachedMTime(const std::string &sourceFile) const override
    {
        return reader_.CachedMTime(sourceFile);
    }
    void EnqueueRead(const std::string &sourceFile,
                     std::function<void(std::shared_ptr<IntermediateCache>)> onLoaded) override
    {
        reader_.Enqueue(sourceFile, std::move(onLoaded));
    }
    void EnqueueWrite(std::shared_ptr<IntermediateCache> cache) override
    {
        writer_.Enqueue(std::move(cache));
    }
    void WaitReads() override
    {
        reader_.Wait();
    }
    std::vector<std::string> WaitWrites() override
    {
        return writer_.Wait();
    }

private:
    IntermediateCacheReader reader_;
    IntermediateCacheWriter writer_;
};
}  // namespace

// Collects every static (non-dynamic-interop) Program from the es2panda parsing context that
// needs a Gluec analysis, extracted from Collect() to keep it within the 50-line guideline.
static std::vector<parser::Program *> GatherPrograms(es2panda_Context *ctx, log::Logger &logger)
{
    std::vector<parser::Program *> programs;
    auto *pubCtx = reinterpret_cast<ark::es2panda::public_lib::Context *>(ctx);
    pubCtx->parserProgram->GetExternalDecls()->Visit([&logger, &programs](parser::Program *prog) {
        if (!prog->IsDeclForDynamicStaticInterop()) {
            GLUEGEN_LOG_DEBUG(logger) << "Collecting program for source file: " << prog->SourceFilePath().Mutf8();
            programs.push_back(prog);
        } else {
            GLUEGEN_LOG_DEBUG(logger) << "Skipping dynamic interop program for source file: "
                                      << prog->SourceFilePath().Mutf8();
        }
    });
    return programs;
}

// Runs Gluec on a single program and enqueues the resulting IntermediateCache for asynchronous
// write. Replaces the `runGluec` lambda originally defined inside Collect().
static void RunGluecOnProgram(parser::Program *prog, Context &context, ICacheStore &cacheStore,
                              std::vector<std::string> &gluecErrors,
                              std::vector<std::shared_ptr<IntermediateCache>> &intermediateCaches)
{
    auto gluec = std::make_unique<Gluec>(prog, context);
    auto gluecResult = gluec->Run();
    if (!gluecResult) {
        gluecErrors.push_back(gluecResult.Error());
        return;
    }
    std::shared_ptr<IntermediateCache> cache(std::move(gluecResult.Value()));
    intermediateCaches.push_back(cache);
    cacheStore.EnqueueWrite(cache);
}

// The cache-hit/miss dispatch loop and the reconciliation pass, extracted from Collect().
// Phase 1: for each program, check mtime against the cache — hit → async read, miss → Gluec.
// Phase 2: after all reads complete, merge successfully-read caches; re-parse any cache hits
//          whose on-disk file turned out to be corrupt or missing.
// NOLINTNEXTLINE
static void ProcessProgramsWithCache(const std::vector<parser::Program *> &programs, ICacheStore &cacheStore,
                                     Context &context, std::vector<std::string> &gluecErrors,
                                     std::vector<std::shared_ptr<IntermediateCache>> &intermediateCaches,
                                     bool cacheEnabled)
{
    auto &logger = context.logger;
    GLUEGEN_LOG_INFO(logger) << "Collecting " << programs.size() << " program(s), "
                             << (cacheEnabled ? "cache enabled" : "cache disabled");

    std::vector<std::shared_ptr<IntermediateCache>> cacheHits(programs.size());
    std::vector<uint8_t> isCacheHit(programs.size(), false);

    // Phase 1: dispatch — cache hit or fresh Gluec parse.
    for (std::size_t i = 0; i < programs.size(); ++i) {
        auto *prog = programs[i];
        auto sourceFilePath = NormalizePath(prog->SourceFilePath().Mutf8());
        auto mtime = GetLastModifiedTime(sourceFilePath);
        auto cachedMTime = cacheStore.CachedMTime(sourceFilePath);
        if (mtime && cachedMTime && FileTimeToString(*mtime) == *cachedMTime) {
            GLUEGEN_LOG_DEBUG(logger) << "Cache hit for " << sourceFilePath;
            isCacheHit[i] = true;
            cacheStore.EnqueueRead(sourceFilePath, [&cacheHits, i](std::shared_ptr<IntermediateCache> cache) {
                cacheHits[i] = std::move(cache);
            });
            continue;
        }
        GLUEGEN_LOG_DEBUG(logger) << "Cache miss for " << sourceFilePath
                                  << (mtime ? " (mtime=" + FileTimeToString(*mtime) + ")" : " (no mtime)")
                                  << (cachedMTime ? " cache[" + *cachedMTime + "]" : " cache[none]");
        RunGluecOnProgram(prog, context, cacheStore, gluecErrors, intermediateCaches);
    }

    // Phase 2: wait for async reads, then reconcile — merge successful reads, re-parse failures.
    cacheStore.WaitReads();
    for (std::size_t i = 0; i < programs.size(); ++i) {
        if (!isCacheHit[i]) {
            continue;
        }
        if (cacheHits[i]) {
            GLUEGEN_LOG_DEBUG(logger) << "Cache read succeeded for "
                                      << NormalizePath(programs[i]->SourceFilePath().Mutf8());
            intermediateCaches.push_back(std::move(cacheHits[i]));
            continue;
        }
        GLUEGEN_LOG_WARN(logger) << "Cache read failed for " << NormalizePath(programs[i]->SourceFilePath().Mutf8())
                                 << " (corrupt or missing), falling back to fresh Gluec parse";
        RunGluecOnProgram(programs[i], context, cacheStore, gluecErrors, intermediateCaches);
    }
}

ark::Expected<std::monostate, std::string> Gluegen::Collect()
{
    auto programs = GatherPrograms(session_.Context(), context_.logger);

    std::unique_ptr<ICacheStore> cacheStore =
        cacheEnabled_ ? std::unique_ptr<ICacheStore>(std::make_unique<DiskCacheStore>(options_.cacheDir, context_))
                      : std::unique_ptr<ICacheStore>(std::make_unique<NullCacheStore>());

    std::vector<std::string> gluecErrors;
    ProcessProgramsWithCache(programs, *cacheStore, context_, gluecErrors, intermediateCaches_, cacheEnabled_);

    auto writeErrors = cacheStore->WaitWrites();
    if (!writeErrors.empty()) {
        GLUEGEN_LOG_ERROR(context_.logger)
            << writeErrors.size() << " cache write error(s): " << Joined(writeErrors, "; ");
    }
    gluecErrors.insert(gluecErrors.end(), writeErrors.begin(), writeErrors.end());
    if (!gluecErrors.empty()) {
        GLUEGEN_LOG_ERROR(context_.logger)
            << "Failed to execute gluec (" << gluecErrors.size() << " error(s), see above)";
        return ark::Unexpected<std::string>("Failed to execute gluec (" + std::to_string(gluecErrors.size()) +
                                            " error(s), see above)");
    }
    return std::monostate {};
}

// Runs the link phase (gluel): merges every IntermediateCache built by Parse() into a single
// GlueConfig (one entry per requested target file, with its re-exports fully resolved) and writes
// it to options_.outputPath. Only one output file is produced, so it is written synchronously.
ark::Expected<std::monostate, std::string> Gluegen::Link()
{
    GLUEGEN_LOG_INFO(context_.logger) << "Linking " << intermediateCaches_.size() << " intermediate cache(s) into "
                                      << options_.outputPath;
    Gluel gluel(intermediateCaches_, context_);
    // The entry source files are the link targets; they were already normalized in
    // InitializeOptions(), matching the (normalized) source paths cached in intermediateCaches_.
    auto linkResult = gluel.Link(options_.sourceFiles);
    if (!linkResult) {
        GLUEGEN_LOG_ERROR(context_.logger) << "Gluel link failed: " << linkResult.Error();
        return ark::Unexpected<std::string>(std::move(linkResult.Error()));
    }

    const auto outputPath = fs::path(options_.outputPath);
    if (!options_.singleFileEmit) {
        return WriteDefaultMode(*linkResult.Value(), outputPath);
    }
    return WriteSingleFileEmitMode(*linkResult.Value(), outputPath);
}

ark::Expected<std::monostate, std::string> Gluegen::WriteDefaultMode(const GlueConfig &config,
                                                                     const fs::path &outputPath)
{
    auto writeResult = WriteFile(outputPath, GlueConfig::serialize(config));
    if (!writeResult) {
        GLUEGEN_LOG_ERROR(context_.logger)
            << "Failed to write link output to " << outputPath << ": " << writeResult.Error();
        context_.diagnosticEngine.Error(DiagnosticCode::OUTPUT_WRITE_FAILED, writeResult.Error());
        return writeResult;
    }
    GLUEGEN_LOG_INFO(context_.logger) << "Link complete, output written to " << outputPath;
    return std::monostate {};
}

ark::Expected<std::monostate, std::string> Gluegen::WriteSingleFileEmitMode(const GlueConfig &config,
                                                                            const fs::path &outputDir)
{
    auto rootDirOpt = GetRootDirFromArktsConfig();
    if (!rootDirOpt) {
        const std::string message = "singleFileEmit mode requires rootDir from arktsconfig, but it was not available";
        GLUEGEN_LOG_ERROR(context_.logger) << message;
        context_.diagnosticEngine.Error(DiagnosticCode::INVALID_CONFIG, message);
        return ark::Unexpected<std::string>(message);
    }
    const auto rootDir = fs::path(*rootDirOpt);

    for (const auto &[sourceFile, fileConfig] : config.files) {
        // sourceFile is in forward-slash form (GlueConfig key convention); convert to native for
        // filesystem relative-path computation.
        auto sourcePath = fs::path(sourceFile);

        fs::path relativePath;
        try {
            relativePath = RelativePath(sourcePath, rootDir);
        } catch (const fs::filesystem_error &error) {
            const std::string message =
                "Failed to compute relative path for " + sourceFile + ": " + error.code().message();
            GLUEGEN_LOG_ERROR(context_.logger) << "Failed to compute relative path for " << sourceFile << " from "
                                               << rootDir << ": " << error.code().message();
            context_.diagnosticEngine.Error(DiagnosticCode::INVALID_CONFIG, message);
            return ark::Unexpected<std::string>(message);
        }

        auto stem = StripSourceExtension(sourcePath.filename().string());
        auto outputFilePath = outputDir / relativePath.parent_path() / (stem + ".json");

        GlueConfig singleFileConfig;
        singleFileConfig.files[sourceFile] = fileConfig;
        singleFileConfig.status = config.status;

        auto writeResult = WriteFile(outputFilePath, GlueConfig::serialize(singleFileConfig));
        if (!writeResult) {
            GLUEGEN_LOG_ERROR(context_.logger)
                << "Failed to write single-file output to " << outputFilePath << ": " << writeResult.Error();
            context_.diagnosticEngine.Error(DiagnosticCode::OUTPUT_WRITE_FAILED, writeResult.Error());
            return writeResult;
        }
        GLUEGEN_LOG_INFO(context_.logger) << "Single-file output written to " << outputFilePath;
    }

    GLUEGEN_LOG_INFO(context_.logger) << "Link complete, " << config.files.size()
                                      << " single-file output(s) written to " << outputDir;
    return std::monostate {};
}

// Writes a standalone `{"status": status}` JSON object. Used by Run() to record a terminal status
// (currently only "syntax-error") for the case where Initialize() fails before Parse()/Link()
// ever run and so no GlueConfig exists yet to carry a status field -- compare GlueConfig::status,
// which covers the normal "success" case as part of Link()'s output.
// In default (single-file) mode this writes directly to options_.outputPath (the output *file*).
// In singleFileEmit mode options_.outputPath is a directory; one `{"status": status}` file is
// written per entry source file, mirroring the source-tree layout under rootDir, so every
// consumer expecting a per-file output still receives an unambiguous terminal signal.
ark::Expected<std::monostate, std::string> Gluegen::WriteStatusOutput(const std::string &status)
{
    auto outputPath = fs::path(options_.outputPath);

    if (!options_.singleFileEmit) {
        nlohmann::json j;
        j["status"] = status;
        auto writeResult = WriteFile(outputPath, j.dump(2));
        if (!writeResult) {
            context_.diagnosticEngine.Error(DiagnosticCode::OUTPUT_WRITE_FAILED, writeResult.Error());
        }
        return writeResult;
    }

    // singleFileEmit mode: write one status file per input source file, each at the same
    // relative path under outputPath that WriteSingleFileEmitMode would use for its JSON.
    // session_ is valid at this point (Parse() sets it before detecting syntax errors), so
    // GetRootDirFromArktsConfig() works directly -- no need for a separate JSON-file reader.
    auto rootDirOpt = GetRootDirFromArktsConfig();
    if (!rootDirOpt) {
        // Can't determine the source-tree layout -- fall back to a single status.json at the
        // output root so the run isn't silently lost.
        GLUEGEN_LOG_WARN(context_.logger) << "singleFileEmit: cannot read rootDir from " << options_.arktsconfigPath
                                          << ", writing status.json at output root instead";
        nlohmann::json j;
        j["status"] = status;
        auto writeResult = WriteFile(outputPath / "status.json", j.dump(2));
        if (!writeResult) {
            context_.diagnosticEngine.Error(DiagnosticCode::OUTPUT_WRITE_FAILED, writeResult.Error());
        }
        return writeResult;
    }
    const auto rootDir = fs::path(*rootDirOpt);

    for (const auto &sourceFile : options_.sourceFiles) {
        auto sourcePath = fs::path(sourceFile);

        fs::path relativePath;
        try {
            relativePath = RelativePath(sourcePath, rootDir);
        } catch (const fs::filesystem_error &error) {
            GLUEGEN_LOG_WARN(context_.logger) << "singleFileEmit: cannot compute relative path for " << sourceFile
                                              << " from " << rootDir << ": " << error.code().message() << ", skipping";
            continue;
        }

        auto stem = StripSourceExtension(sourcePath.filename().string());
        auto fileOutputPath = outputPath / relativePath.parent_path() / (stem + ".json");

        nlohmann::json j;
        j["status"] = status;
        auto writeResult = WriteFile(fileOutputPath, j.dump(2));
        if (!writeResult) {
            GLUEGEN_LOG_ERROR(context_.logger)
                << "Failed to write status output to " << fileOutputPath << ": " << writeResult.Error();
            context_.diagnosticEngine.Error(DiagnosticCode::OUTPUT_WRITE_FAILED, writeResult.Error());
            return writeResult;
        }
        GLUEGEN_LOG_INFO(context_.logger) << "Status output written to " << fileOutputPath;
    }

    return std::monostate {};
}

void Gluegen::InitializeOptions()
{
    // verify output path
    if (options_.outputPath.empty()) {
        options_.outputPath = NormalizePath(fs::path(context_.workingDirectory) / "gluegen.json");
    }
    // verify report path: defaults to "report.json" next to the final output, so the diagnostics
    // report ends up co-located with the generated artifact unless the caller asks for somewhere
    // else via --report-path. Filled in here as a concrete *file* path (not just the directory) so
    // it isn't mistaken for a directory-that-doesn't-exist-yet by
    // ResolveDiagnosticsReportFilePath's is_directory check.
    if (options_.reportPath.empty()) {
        options_.reportPath = (fs::path(options_.outputPath).parent_path() / "report.json").string();
    }
    // verify cache dir
    // cacheEnabled_ must be captured *before* defaulting an unspecified cacheDir below: an
    // explicit, non-empty cacheDir is what "the user asked for incremental caching" means here.
    // When it's empty (the user didn't pass one), Parse() skips both writing and reading any
    // on-disk cache entirely, regardless of the default path filled in just below.
    cacheEnabled_ = !options_.cacheDir.empty();
    // verify arktsconfig path
    if (options_.arktsconfigPath.empty()) {
        options_.arktsconfigPath = NormalizePath(fs::path(context_.workingDirectory) / "arktsconfig.json");
    }
}

std::optional<std::string> Gluegen::GetRootDirFromArktsConfig()
{
    const auto *opts = session_.Impl()->ConfigGetOptions(session_.Config());
    if (opts == nullptr) {
        return std::nullopt;
    }
    const auto *arkTsConfig =
        session_.Impl()->OptionsUtilArkTSConfigConst(session_.Context(), const_cast<es2panda_Options *>(opts));
    if (arkTsConfig == nullptr) {
        return std::nullopt;
    }
    const char *rootDir =
        session_.Impl()->ArkTsConfigRootDirConst(session_.Context(), const_cast<es2panda_ArkTsConfig *>(arkTsConfig));
    if (rootDir == nullptr) {
        return std::nullopt;
    }
    return std::make_optional<std::string>(NormalizePath(rootDir));
}

ark::Expected<std::monostate, std::string> Gluegen::ValidateOptions()
{
    // verify input files
    if (options_.sourceFiles.empty()) {
        const std::string message = "Gluegen requires at least one source file";
        context_.diagnosticEngine.Error(DiagnosticCode::INVALID_CONFIG, message);
        return ark::Unexpected<std::string>(message);
    }
    for (auto &filePath : options_.sourceFiles) {
        if (!fs::exists(filePath)) {
            const std::string message = "Source file does not exist: " + filePath;
            context_.diagnosticEngine.Error(DiagnosticCode::INVALID_CONFIG, message);
            return ark::Unexpected<std::string>(message);
        }
        filePath = NormalizePath(filePath);
    }
    if (!fs::exists(options_.arktsconfigPath)) {
        const std::string message = "Arktsconfig file does not exist: " + options_.arktsconfigPath;
        context_.diagnosticEngine.Error(DiagnosticCode::INVALID_CONFIG, message);
        return ark::Unexpected<std::string>(message);
    }

    return std::monostate {};
}

}  // namespace ark::es2panda::gluegen
