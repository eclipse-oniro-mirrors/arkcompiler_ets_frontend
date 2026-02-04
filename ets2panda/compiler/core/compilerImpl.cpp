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

#include "compilerImpl.h"

#include "es2panda.h"
#include "ast_verifier/ASTVerifier.h"
#include "checker/ETSAnalyzer.h"
#include "checker/TSAnalyzer.h"
#include "checker/TSchecker.h"
#include "checker/ETSchecker.h"
#include "checker/ASchecker.h"
#include "checker/JSchecker.h"
#include "compiler/core/compileQueue.h"
#include "compiler/core/compilerImpl.h"
#include "compiler/core/pandagen.h"
#include "compiler/core/ETSCompiler.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/JSCompiler.h"
#include "compiler/core/JSemitter.h"
#include "compiler/core/ETSemitter.h"
#include "compiler/lowering/phase.h"
#include "compiler/lowering/scopesInit/scopesInitPhase.h"
#include "compiler/lowering/checkerPhase.h"
#include "evaluate/scopedDebugInfoPlugin.h"
#include "parser/program/DeclarationCache.h"
#include "parser/parserImpl.h"
#include "parser/JSparser.h"
#include "parser/ASparser.h"
#include "parser/TSparser.h"
#include "parser/ETSparser.h"
#include "parser/program/program.h"
#include "public/public.h"
#include "util/eheap.h"
#include "util/ustring.h"
#include "util/perfMetrics.h"
#include "varbinder/JSBinder.h"
#include "varbinder/ASBinder.h"
#include "varbinder/TSBinder.h"
#include "varbinder/ETSBinder.h"

namespace ark::es2panda::compiler {

void CompilerImpl::HandleContextLiterals(public_lib::Context *context)
{
    auto *emitter = context->emitter;

    uint32_t index = 0;
    for (const auto &buff : context->contextLiterals) {
        emitter->AddLiteralBuffer(buff, index++);
    }

    emitter->LiteralBufferIndex() += context->contextLiterals.size();
}

void CompilerImpl::Emit(public_lib::Context *context)
{
    HandleContextLiterals(context);

    queue_.Schedule(context);

    /* Main thread can also be used instead of idling */
    queue_.Consume();
    auto *emitter = context->emitter;
    queue_.Wait([emitter](CompileJob *job) { emitter->AddProgramElement(job->GetProgramElement()); });
}

template <typename CodeGen, typename RegSpiller, typename FunctionEmitter, typename Emitter, typename AstCompiler>
static public_lib::Context::CodeGenCb MakeCompileJob()
{
    return [](public_lib::Context *context, varbinder::FunctionScope *scope,
              compiler::ProgramElement *programElement) -> void {
        if constexpr (std::is_same_v<FunctionEmitter, compiler::ETSFunctionEmitter>) {
            if (!compiler::ETSFunctionEmitter::IsEmissionRequired(scope->Node()->AsScriptFunction(),
                                                                  context->parserProgram)) {
                return;
            }
        }
        RegSpiller regSpiller;
        auto allocator = ScopedAllocatorsManager::CreateAllocator();
        AstCompiler astcompiler;
        compiler::SetPhaseManager(context->phaseManager);
        CodeGen cg(&allocator, &regSpiller, context, std::make_tuple(scope, programElement, &astcompiler));
        FunctionEmitter funcEmitter(&cg, programElement);
        funcEmitter.Generate();
    };
}

static bool CheckOptionsBeforePhase(const public_lib::Context &context, const std::string &name)
{
    const auto &options = *context.config->options;
    if (options.GetDumpBeforePhases().count(name) > 0U) {
        std::cout << "Before phase " << name << ":\n";
        std::cout << context.parserProgram->Dump() << std::endl;
    }

    if (options.GetDumpEtsSrcBeforePhases().count(name) > 0U) {
        std::cout << "Before phase " << name << " ets source:\n";
        std::cout << context.parserProgram->Ast()->DumpEtsSrc() << std::endl;
    }

    return options.GetExitBeforePhase() == name;
}

static void WriteStringToFile(public_lib::Context *context, const std::string &outputPath, std::string_view contents)
{
    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        context->diagnosticEngine->LogFatalError(diagnostic::OPEN_FAILED, util::DiagnosticMessageParams {outputPath},
                                                 lexer::SourcePosition());
        return;
    }

    outFile << contents;
    outFile.close();
}

void HandleGenerateDecl(public_lib::Context *context, const parser::Program *program, const std::string &outputPath)
{
    ir::Declgen dg {context};
    ir::SrcDumper dumper {&dg};
    program->Ast()->Dump(&dumper);
    dumper.GetDeclgen()->Run();
    dumper.DumpExports();

    std::string res = "'use static'\n";
    dg.DumpImports(res);
    res += dumper.Str();

    std::string_view textToWrite;
    if (context->config->options->IsStoreDeclarationCacheDirectlyInMemory()) {
        using DC = parser::DeclarationCache;
        textToWrite = DC::PromoteExistingEntryToLowdeclaration(program->GetImportMetadata(), std::move(res));
    } else {
        textToWrite = res;
    }

    WriteStringToFile(context, outputPath, textToWrite);
}

static bool CheckOptionsAfterPhase(const public_lib::Context &context, const std::string &name)
{
    const auto &options = *context.config->options;
    if (options.GetDumpAfterPhases().count(name) > 0U) {
        std::cout << "After phase " << name << ":\n";
        std::cout << context.parserProgram->Dump() << std::endl;
    }

    if (options.GetDumpEtsSrcAfterPhases().count(name) > 0U) {
        std::cout << "After phase " << name << " ets source:\n";
        std::cout << context.parserProgram->Ast()->DumpEtsSrc() << std::endl;
    }

    return options.GetExitAfterPhase() == name;
}

static void GenDeclsForStdlib(public_lib::Context *context)
{
    ES2PANDA_ASSERT(context->config->options->IsGenStdlib());
    context->parserProgram->GetExternalSources()->Visit([context](parser::PackageProgram *extProgram) {
        ir::Declgen dg {context};
        ir::SrcDumper dumper {&dg};
        ES2PANDA_ASSERT(!extProgram->GetUnmergedPackagePrograms().empty());
        // NOTE(dkofanov): stdlib is not merged yet, but should be in 'PackageImplicitImport' lowering.
        for (const auto *fraction : extProgram->GetUnmergedPackagePrograms()) {
            fraction->Ast()->Dump(&dumper);
        }
        dumper.GetDeclgen()->Run();
        dumper.DumpExports();

        // NOTE(dkofanov): #32416 'ImportPathManager::FormEtscacheFilePath' should be used instead.
        std::string path = std::string(extProgram->ModuleName()) + std::string(util::ImportPathManager::CACHE_SUFFIX);
        if (context->config->options->WasSetGenerateDeclPath()) {
            // NOTE: "/" at the end needed because of bug in GetParentDir
            auto parentDir = ark::os::GetParentDir(context->config->options->GetGenerateDeclPath() + "/");
            ark::os::CreateDirectories(parentDir);
            path = parentDir.append("/").append(path);
        }

        std::string res = "'use static'\n";
        dg.DumpImports(res);
        res += dumper.Str();

        WriteStringToFile(context, path, std::move(res));
    });
}

// NOTE(dkofanov): #32416 'ImportPathManager::FormEtscacheFilePath' should be used instead.
static std::string ResolveDeclsOutputPath(const public_lib::Context &context)
{
    if (!context.config->options->WasSetGenerateDeclPath()) {
        return ark::os::RemoveExtension(util::BaseName(context.config->options->SourceFileName()))
            .append(util::ImportPathManager::CACHE_SUFFIX);
    }
    return context.config->options->GetGenerateDeclPath();
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP, G.FUD.05) solid logic
static bool RunVerifierAndPhases(public_lib::Context *context)
{
    const auto &options = *context->config->options;
    const auto verifierEachPhase = options.IsAstVerifierEachPhase();

    ast_verifier::ASTVerifier verifier(*context);
    verifier.Before();

    bool afterCheckerPhase = false;
    while (auto phase = context->phaseManager->NextPhase()) {
        const auto name = std::string {phase->Name()};
        if (name == "plugins-after-check") {
            afterCheckerPhase = true;
        }
        ES2PANDA_PERF_EVENT_SCOPE("@phases/" + name);

        if (options.GetSkipPhases().count(name) > 0) {
            continue;
        }

        if (CheckOptionsBeforePhase(*context, name) || !phase->Apply(context) ||
            CheckOptionsAfterPhase(*context, name)) {
            return false;
        }

        if (verifier.IntroduceNewInvariants(phase->Name());
            verifierEachPhase || options.HasVerifierPhase(phase->Name())) {
            verifier.Verify(phase->Name());
        }

        // Stop lowerings processing after Checker phase if any error happened.
        if (afterCheckerPhase && context->diagnosticEngine->IsAnyError()) {
            return false;
        }

        if (options.IsGenerateDeclEnabled() && name == compiler::CheckerPhase::NAME &&
            !context->diagnosticEngine->IsAnyError()) {
            if (options.IsGenStdlib()) {
                GenDeclsForStdlib(context);
            } else {
                HandleGenerateDecl(context, context->parserProgram, ResolveDeclsOutputPath(*context));
            }
        }
    }

    verifier.After();
    return true;
}

static bool RunPhases(public_lib::Context *context)
{
    const auto &options = *context->config->options;

    while (auto phase = context->phaseManager->NextPhase()) {
        const auto name = std::string {phase->Name()};
        if (options.GetSkipPhases().count(name) > 0) {
            continue;
        }

        if (CheckOptionsBeforePhase(*context, name)) {
            return false;
        }

        if (!phase->Apply(context)) {
            return false;
        }

        if (CheckOptionsAfterPhase(*context, name)) {
            return false;
        }
    }

    return true;
}

using EmitCb = std::function<pandasm::Program *(public_lib::Context *)>;
using PhaseListGetter = std::function<std::vector<compiler::Phase *>(ScriptExtension)>;

static void MarkAsLowered(public_lib::Context *ctx)
{
    ctx->parserProgram->GetExternalSources()->Visit([](auto *extProg) {
        if (!extProg->IsASTLowered()) {
            extProg->MarkASTAsLowered();
        }
    });
}

static pandasm::Program *EmitProgram(CompilerImpl *compilerImpl, public_lib::Context *context)
{
    ES2PANDA_PERF_SCOPE("@EmitProgram");
    compilerImpl->Emit(context);
    context->emitter->GenAnnotation();
    return context->emitter->Finalize(context->config->options->IsDumpDebugInfo(), Signatures::ETS_GLOBAL);
}

static bool ParseAndRunPhases(const CompilationUnit &unit, public_lib::Context *context)
{
    ES2PANDA_ASSERT(context->parserProgram == nullptr);
    ES2PANDA_PERF_SCOPE("@phases");

    if (context->config->options->GetCompilationMode() == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE &&
        context->config->options->GetExtension() == ScriptExtension::ETS) {
        if (!context->sourceFile->filePath.empty()) {
            context->sourceFileNames.emplace_back(os::GetAbsolutePath(context->sourceFile->filePath));
        } else if (auto compilationList = FindProjectSources(context->config->options->ArkTSConfig());
                   !compilationList.empty()) {
            for (auto &[src, _] : compilationList) {
                context->sourceFileNames.push_back(os::GetAbsolutePath(src));
            }
        }

        if (context->sourceFileNames.empty()) {
            context->diagnosticEngine->LogDiagnostic(diagnostic::NO_INPUT, util::DiagnosticMessageParams {});
            return false;
        }
        context->parser->AsETSParser()->ParseInSimultMode();
    } else {
        context->parser->ParseGlobal();
    }
    ES2PANDA_ASSERT(context->parserProgram != nullptr);

    //  We have to check the return status of 'RunVerifierAndPhase` and 'RunPhases` separately because there can be
    //  some internal errors (say, in Post-Conditional check) or terminate options (say in 'CheckOptionsAfterPhase')
    //  that were not reported to the log.
    if (unit.ext == ScriptExtension::ETS) {
        if (!RunVerifierAndPhases(context)) {
            return false;
        }
    } else if (context->diagnosticEngine->IsAnyError()) {
        if (context->config->options->IsDumpAst()) {
            std::cout << context->parserProgram->Dump() << std::endl;
        }
    } else if (!RunPhases(context)) {
        return false;
    }

    return !context->diagnosticEngine->IsAnyError();
}

static pandasm::Program *ClearContextAndReturnProgam(public_lib::Context *context, pandasm::Program *program)
{
    context->config = nullptr;
    context->parser = nullptr;
    context->ClearCheckers();
    context->ClearAnalyzers();
    context->phaseManager = nullptr;
    context->parserProgram = nullptr;
    context->emitter = nullptr;
    return program;
}

// NOTE(dkofanov): should be alligned with the public-lib context initialization and processing.
template <typename Parser, typename Checker, typename Analyzer, typename AstCompiler, typename CodeGen,
          typename RegSpiller, typename FunctionEmitter, typename Emitter>
static pandasm::Program *Compile(const CompilationUnit &unit, CompilerImpl *compilerImpl, public_lib::Context *context)
{
    ir::DisableContextHistory();
    parser::DeclarationCache::ActivateCache();

    auto config = public_lib::ConfigImpl {};
    auto phaseManager = compiler::PhaseManager(context, unit.ext, context->allocator);
    context->config = &config;
    context->config->options = &unit.options;
    context->queue = compilerImpl->Queue();
    context->plugins = &compilerImpl->Plugins();

    // NOTE(dkofanov): #32416 eliminate 'SourceFile' and 'CompilationUnit'
    context->input = unit.input.source;
    context->sourceFileName = unit.input.filePath;
    context->sourceFile = &unit.input;
    auto parser = Parser(context, static_cast<parser::ParserStatus>(unit.rawParserStatus));
    context->parser = &parser;

    auto checker = Checker(context->allocator, unit.diagnosticEngine);
    context->PushChecker(&checker);
    auto analyzer = Analyzer(&checker);
    checker.SetAnalyzer(&analyzer);
    context->PushAnalyzer(checker.GetAnalyzer());
    context->codeGenCb = MakeCompileJob<CodeGen, RegSpiller, FunctionEmitter, Emitter, AstCompiler>();
    context->diagnosticEngine = &unit.diagnosticEngine;
    context->phaseManager = &phaseManager;

    auto emitter = Emitter(context);
    context->emitter = &emitter;

    try {
        if (!ParseAndRunPhases(unit, context)) {
            context->diagnosticEngine->EnsureLocations();
            return ClearContextAndReturnProgam(context, nullptr);
        }

        MarkAsLowered(context);
        context->diagnosticEngine->EnsureLocations();
        return ClearContextAndReturnProgam(context, EmitProgram(compilerImpl, context));
    } catch (util::ThrowableDiagnostic &e) {
        context->diagnosticEngine->EnsureLocations();
        e.EnsureLocation();
        throw e;
    }
}

pandasm::Program *CompilerImpl::Compile(const CompilationUnit &unit, public_lib::Context *context)
{
    switch (unit.ext) {
        case ScriptExtension::TS: {
            return compiler::Compile<parser::TSParser, checker::TSChecker, checker::TSAnalyzer, compiler::JSCompiler,
                                     compiler::PandaGen, compiler::DynamicRegSpiller, compiler::JSFunctionEmitter,
                                     compiler::JSEmitter>(unit, this, context);
        }
        case ScriptExtension::AS: {
            return compiler::Compile<parser::ASParser, checker::ASChecker, checker::TSAnalyzer, compiler::JSCompiler,
                                     compiler::PandaGen, compiler::DynamicRegSpiller, compiler::JSFunctionEmitter,
                                     compiler::JSEmitter>(unit, this, context);
        }
        case ScriptExtension::ETS: {
            return compiler::Compile<parser::ETSParser, checker::ETSChecker, checker::ETSAnalyzer,
                                     compiler::ETSCompiler, compiler::ETSGen, compiler::StaticRegSpiller,
                                     compiler::ETSFunctionEmitter, compiler::ETSEmitter>(unit, this, context);
        }
        case ScriptExtension::JS: {
            return compiler::Compile<parser::JSParser, checker::JSChecker, checker::TSAnalyzer, compiler::JSCompiler,
                                     compiler::PandaGen, compiler::DynamicRegSpiller, compiler::JSFunctionEmitter,
                                     compiler::JSEmitter>(unit, this, context);
        }
        default: {
            ES2PANDA_UNREACHABLE();
            return nullptr;
        }
    }
}

void CompilerImpl::DumpAsm(const ark::pandasm::Program *prog)
{
    Emitter::DumpAsm(prog);
}

std::string CompilerImpl::GetPhasesList(const ScriptExtension ext)
{
    std::stringstream ss;
    auto phaseManager = compiler::PhaseManager(ext, nullptr);
    while (auto phase = phaseManager.NextPhase()) {
        ss << " " << phase->Name() << std::endl;
    }
    return ss.str();
}

}  // namespace ark::es2panda::compiler
