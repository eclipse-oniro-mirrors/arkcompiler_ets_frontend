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

#ifndef ES2PANDA_DRIVER_GLUEGEN_NATIVE_GLUEGEN_H
#define ES2PANDA_DRIVER_GLUEGEN_NATIVE_GLUEGEN_H

#include <string>
#include <variant>
#include <vector>

#include "libarkbase/utils/expected.h"
#include "public/es2panda_lib.h"
#include "./gluec.h"
#include "./utils.h"

namespace ark::es2panda::gluegen {

enum class ParseOutcome {
    READY,
    SYNTAX_ERROR,
};

struct GlueConfig;

struct GluegenOptions {
    std::vector<std::string> sourceFiles;
    std::string cacheDir;
    std::string arktsconfigPath;
    std::string outputPath;
    // Directory the structured diagnostics report (gluegen.diagnostics.json) is written to.
    // Defaults to outputPath's directory when left empty -- see Gluegen::InitializeOptions().
    std::string reportPath;
    // When true, emit one output file per input source file instead of a single merged output.
    bool singleFileEmit = false;
};

// Move-only RAII holder for an es2panda Config/Context pair, so a parsed
// Context can be handed out of the function that created it (e.g. from
// ParseFilesSimultaneously to Gluegen) and kept alive across later phases
// (Bind/Check/Lower, IR emission, ...).
//
// Destruction order matters: a live Context holds a pointer back into its
// Config (options/diagnosticEngine), which DestroyContext/GetAllErrorMessages/
// IsAnyError rely on. So Context must always be destroyed before Config.
class Es2pandaSession {
public:
    Es2pandaSession() = default;
    Es2pandaSession(const es2panda_Impl *impl, es2panda_Config *config, es2panda_Context *context);

    NO_COPY_SEMANTIC(Es2pandaSession);

    Es2pandaSession(Es2pandaSession &&other) noexcept;
    Es2pandaSession &operator=(Es2pandaSession &&other) noexcept;

    ~Es2pandaSession();

    const es2panda_Impl *Impl() const
    {
        return impl_;
    }
    es2panda_Config *Config() const
    {
        return config_;
    }
    es2panda_Context *Context() const
    {
        return context_;
    }

    // Later phases (ProceedToState to BOUND/CHECKED/LOWERED/...) may return a
    // different Context pointer for the same logical session; update it here
    // so it keeps being tracked for destruction.
    void SetContext(es2panda_Context *context)
    {
        context_ = context;
    }

private:
    void Reset();

    const es2panda_Impl *impl_ = nullptr;
    es2panda_Config *config_ = nullptr;
    es2panda_Context *context_ = nullptr;
};

class Gluegen {
public:
    // `consumer`, if non-null, is installed on context_.diagnosticEngine at construction time so
    // every diagnostic reported during the entire lifetime of this Gluegen instance -- including
    // during Initialize() / Parse() / Collect() / Link() -- is forwarded to it immediately.
    // Caller owns `consumer` and must keep it alive for the duration of the Run() call.
    explicit Gluegen(const GluegenOptions &options, DiagnosticConsumer *consumer = nullptr);

    NO_COPY_SEMANTIC(Gluegen);
    NO_MOVE_SEMANTIC(Gluegen);

    ~Gluegen() = default;

    // Runs the full pipeline: Initialize() → Parse() → Collect() → Link() → WriteDiagnosticsReport().
    // The report is always written regardless of which phase (if any) fails, so the e2e test suite
    // can assert on DiagnosticEngine's structured records without scraping prose from stderr.
    ark::Expected<std::monostate, std::string> Run();

private:
    // One-time setup: resolves working directory, fills in default output/cache/arktsconfig paths,
    // starts the logger, validates options. Called by RunImpl() on every invocation of Run(), so
    // callers never need to invoke this directly.
    ark::Expected<std::monostate, std::string> Initialize();
    // Generate ast from compiler
    ark::Expected<ParseOutcome, std::string> Parse();
    // Run gluec
    ark::Expected<std::monostate, std::string> Collect();
    // Run gluel
    ark::Expected<std::monostate, std::string> Link();

    // Validate options
    ark::Expected<std::monostate, std::string> ValidateOptions();
    // Initialize options
    void InitializeOptions();

    // The actual body of Run(), i.e. Initialize()/Parse()/Link() in sequence. Split out of Run()
    // so Run() itself can unconditionally write the diagnostics report (see
    // WriteDiagnosticsReport()) after this returns, regardless of which phase (if any) failed.
    ark::Expected<std::monostate, std::string> RunImpl();

    // Serializes every diagnostic collected in context_.diagnosticEngine so far and writes it to
    // `<options_.reportPath>/gluegen.diagnostics.json`. Called once, unconditionally, by Run().
    ark::Expected<std::monostate, std::string> WriteDiagnosticsReport();

    // Writes a standalone `{"status": status}` JSON object to options_.outputPath. Used by Run()
    // to record a terminal status (currently only "syntax-error") when Initialize() fails before
    // any GlueConfig exists to attach a status field to -- see GlueConfig::status for the
    // "success" counterpart, written as part of the normal Link() output instead.
    ark::Expected<std::monostate, std::string> WriteStatusOutput(const std::string &status);

    // Extracts the rootDir field from arktsconfig.json via es2panda's public API
    std::optional<std::string> GetRootDirFromArktsConfig();

    // Writes the full merged GlueConfig as a single JSON file to outputPath (default mode).
    ark::Expected<std::monostate, std::string> WriteDefaultMode(const GlueConfig &config, const fs::path &outputPath);

    // Writes one JSON file per entry in GlueConfig::files to outputDir, each containing only that
    // file's entry in its `files` map. Output paths mirror the source tree relative to rootDir.
    ark::Expected<std::monostate, std::string> WriteSingleFileEmitMode(const GlueConfig &config,
                                                                       const fs::path &outputDir);

    GluegenOptions options_;
    // Owns the Config/Context produced by Parse() so Link() (and any future
    // phases) can keep operating on the same Context.
    Es2pandaSession session_;

    // Owns the SymbolNodeManager/DiagnosticEngine/workingDirectory shared by every phase
    // (Parse()/Gluec, Link()/Gluel, and cache (de)serialization) of this run -- see context.h.
    Context context_;

    // Every IntermediateCache built by Parse() (one per source/dependency file), kept in memory
    // (in addition to being written to disk asynchronously via IntermediateCacheWriter) so Link()
    // can consume them directly without re-reading them back from disk.
    std::vector<std::shared_ptr<IntermediateCache>> intermediateCaches_;

    // Whether incremental disk caching is active at all, set once by InitializeOptions() from
    // whether the user actually supplied a cache directory (*before* it fills options_.cacheDir
    // in with a default). When false, Parse() neither writes nor reads any IntermediateCache to
    // disk -- every file is re-parsed via Gluec every run, same as if no cache existed at all.
    bool cacheEnabled_ = false;
};
}  // namespace ark::es2panda::gluegen

#endif  // ES2PANDA_DRIVER_GLUEGEN_NATIVE_GLUEGEN_H
