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
#include "parser/parserImpl.h"
#include "parser/JSparser.h"
#include "parser/ASparser.h"
#include "parser/TSparser.h"
#include "parser/ETSparser.h"
#include "parser/program/program.h"
#include "public/public.h"
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

ark::pandasm::Program *CompilerImpl::Emit(public_lib::Context *context)
{
    HandleContextLiterals(context);

    queue_.Schedule(context);

    /* Main thread can also be used instead of idling */
    queue_.Consume();
    auto *emitter = context->emitter;
    queue_.Wait([emitter](CompileJob *job) { emitter->AddProgramElement(job->GetProgramElement()); });

    return emitter->Finalize(context->config->options->IsDumpDebugInfo(), Signatures::ETS_GLOBAL);
}

template <typename CodeGen, typename RegSpiller, typename FunctionEmitter, typename Emitter, typename AstCompiler>
static public_lib::Context::CodeGenCb MakeCompileJob()
{
    return [](public_lib::Context *context, varbinder::FunctionScope *scope,
              compiler::ProgramElement *programElement) -> void {
        RegSpiller regSpiller;
        ArenaAllocator allocator(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);
        AstCompiler astcompiler;
        compiler::SetPhaseManager(context->phaseManager);
        CodeGen cg(&allocator, &regSpiller, context, std::make_tuple(scope, programElement, &astcompiler));
        FunctionEmitter funcEmitter(&cg, programElement);
        funcEmitter.Generate();
    };
}

static bool CheckOptionsBeforePhase(const util::Options &options, const parser::Program &program,
                                    const std::string &name)
{
    if (options.GetDumpBeforePhases().count(name) > 0U) {
        std::cout << "Before phase " << name << ":\n";
        std::cout << program.Dump() << std::endl;
    }

    if (options.GetDumpEtsSrcBeforePhases().count(name) > 0U) {
        std::cout << "Before phase " << name << " ets source:\n";
        std::cout << program.Ast()->DumpEtsSrc() << std::endl;
    }

    return options.GetExitBeforePhase() == name;
}

void HandleGenerateDecl(const parser::Program &program, util::DiagnosticEngine &diagnosticEngine,
                        const std::string &outputPath)
{
    std::ofstream outFile(outputPath);
    if (!outFile.is_open()) {
        diagnosticEngine.LogFatalError(diagnostic::OPEN_FAILED, util::DiagnosticMessageParams {outputPath},
                                       lexer::SourcePosition());
        return;
    }
    std::string result = program.Ast()->DumpDecl();
    result.erase(0, result.find_first_not_of('\n'));
    result = "'use static'\n" + result;

    outFile << result;
    outFile.close();
}

static bool CheckOptionsAfterPhase(const util::Options &options, const parser::Program &program,
                                   const std::string &name)
{
    if (options.GetDumpAfterPhases().count(name) > 0U) {
        std::cout << "After phase " << name << ":\n";
        std::cout << program.Dump() << std::endl;
    }

    if (options.GetDumpEtsSrcAfterPhases().count(name) > 0U) {
        std::cout << "After phase " << name << " ets source:\n";
        std::cout << program.Ast()->DumpEtsSrc() << std::endl;
    }

    return options.GetExitAfterPhase() == name;
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP, G.FUD.05) solid logic
static bool RunVerifierAndPhases(public_lib::Context &context, parser::Program &program)
{
    const auto &options = *context.config->options;
    const auto verifierEachPhase = options.IsAstVerifierEachPhase();

    ast_verifier::ASTVerifier verifier(context, program);
    verifier.Before();

    bool afterCheckerPhase = false;
    while (auto phase = context.phaseManager->NextPhase()) {
        const auto name = std::string {phase->Name()};
        if (name == "plugins-after-check") {
            afterCheckerPhase = true;
        }
        ES2PANDA_PERF_EVENT_SCOPE("@phases/" + name);

        if (options.GetSkipPhases().count(name) > 0) {
            continue;
        }

        if (CheckOptionsBeforePhase(options, program, name) || !phase->Apply(&context, &program) ||
            CheckOptionsAfterPhase(options, program, name)) {
            return false;
        }

        if (verifier.IntroduceNewInvariants(phase->Name());
            verifierEachPhase || options.HasVerifierPhase(phase->Name())) {
            verifier.Verify(phase->Name());
        }

        // Stop lowerings processing after Checker phase if any error happened.
        if (afterCheckerPhase && context.diagnosticEngine->IsAnyError()) {
            return false;
        }

        if (options.IsGenerateDeclEnabled() && name == compiler::CheckerPhase::NAME) {
            std::string path;
            if (!options.WasSetGenerateDeclPath()) {
                path = ark::os::RemoveExtension(util::BaseName(options.SourceFileName())).append(".d.ets");
            } else {
                path = options.GetGenerateDeclPath();
            }
            HandleGenerateDecl(program, *context.diagnosticEngine, path);
        }
    }

    verifier.After();
    return true;
}

static bool RunPhases(public_lib::Context &context, parser::Program &program)
{
    const auto &options = *context.config->options;

    while (auto phase = context.phaseManager->NextPhase()) {
        const auto name = std::string {phase->Name()};
        if (options.GetSkipPhases().count(name) > 0) {
            continue;
        }

        if (CheckOptionsBeforePhase(options, program, name)) {
            return false;
        }

        if (!phase->Apply(&context, &program)) {
            return false;
        }

        if (CheckOptionsAfterPhase(options, program, name)) {
            return false;
        }
    }

    return true;
}

static void CreateDebuggerEvaluationPlugin(checker::ETSChecker &checker, ArenaAllocator &allocator,
                                           parser::Program *program, const util::Options &options)
{
    // Sometimes evaluation mode might work without project context.
    // In this case, users might omit context files.
    if (options.IsDebuggerEval() && !options.GetDebuggerEvalPandaFiles().empty()) {
        auto *plugin = allocator.New<evaluate::ScopedDebugInfoPlugin>(program, &checker, options);
        checker.SetDebugInfoPlugin(plugin);
    }
}

using EmitCb = std::function<pandasm::Program *(public_lib::Context *)>;
using PhaseListGetter = std::function<std::vector<compiler::Phase *>(ScriptExtension)>;

static void AddExternalPrograms(public_lib::Context *ctx, const CompilationUnit &unit, parser::Program *program)
{
    auto &extSources = program->ExternalSources();
    for (const auto &[moduleName, extPrograms] : ctx->externalSources) {
        for (auto *const extProg : extPrograms) {
            if (extProg->SourceFilePath() == unit.input.filePath) {
                continue;
            }
            if (extSources.count(moduleName) == 0) {
                extSources.emplace(moduleName, program->Allocator()->Adapter());
            }
            extSources.at(moduleName).emplace_back(extProg);
        }
    }
    auto varbinder = static_cast<varbinder::ETSBinder *>(ctx->transitionMemory->VarBinder());
    auto externalRecordTable = varbinder->GetExternalRecordTable();
    for (auto [extProg, recordTable] : externalRecordTable) {
        if (program->SourceFilePath() == extProg->SourceFilePath()) {
            externalRecordTable.erase(externalRecordTable.find(extProg));
            break;
        }
    }
    auto &prevGlobalTypes = ctx->transitionMemory->GlobalTypes()->GlobalTypes();
    for (auto *gt : prevGlobalTypes) {
        if (gt != nullptr && gt->IsETSObjectType()) {
            auto *relation = gt->AsETSObjectType()->GetRelation();
            if (relation != nullptr) {
                relation->SetChecker(ctx->GetChecker());
            }
        }
    }
}

[[maybe_unused]] static void MarkAsLowered(parser::Program &program)
{
    for (auto &[name, extPrograms] : program.ExternalSources()) {
        for (auto &extProgram : extPrograms) {
            if (!extProgram->IsASTLowered()) {
                extProgram->MarkASTAsLowered();
            }
        }
    }
}

static void EmplaceProgram(public_lib::Context *ctx, util::StringView moduleName, parser::Program *extProg)
{
    auto &extSources = ctx->externalSources;
    if (extSources.count(moduleName) == 0) {
        extSources.emplace(moduleName, ctx->transitionMemory->PermanentAllocator()->Adapter());
    }
    bool found = false;
    for (auto extSrc : extSources.at(moduleName)) {
        if (extSrc->SourceFilePath() == extProg->SourceFilePath()) {
            found = true;
            break;
        }
    }
    if (!found) {
        extSources.at(moduleName).emplace_back(extProg);
    }
}

static void SavePermanents(public_lib::Context *ctx, parser::Program *program)
{
    for (const auto &[moduleName, extPrograms] : program->ExternalSources()) {
        for (auto *const extProg : extPrograms) {
            EmplaceProgram(ctx, moduleName, extProg);
        }
    }
    ctx->transitionMemory->SetVarBinder(program->VarBinder());
    auto *topScope = ctx->transitionMemory->VarBinder()->TopScope();
    ES2PANDA_ASSERT(topScope->IsGlobalScope());
    auto decls = topScope->Decls();
    auto path = program->SourceFilePath();
    // clang-format off
    decls.erase(std::remove_if(decls.begin(), decls.end(),
        [path](varbinder::Decl *d) -> bool {
            auto *n = d->Node();
            if (n == nullptr || n->Start().Program() == nullptr) {
                return true;
            }
            auto sourceFile = n->Start().Program()->SourceFilePath();
            return sourceFile == "" || sourceFile == path;
        }),
        decls.end());
    // clang-format on

    auto res = topScope->Find(util::StringView("ETSGLOBAL"));
    auto *var = res.variable;
    var->SetTsType(nullptr);
    auto moduleName = std::string(program->ModuleName());
    auto globalClassName = moduleName.substr(moduleName.find_last_of('.') + 1);
    topScope->EraseBinding(util::StringView(globalClassName));

    std::vector<util::StringView> declaredKeys;
    for (auto &&it : topScope->Bindings()) {
        var = it.second;
        if (var->Declaration() != nullptr && var->Declaration()->Node() != nullptr) {
            auto *node = var->Declaration()->Node();
            if (node->Range().start.Program() == program) {
                declaredKeys.emplace_back(it.first);
            }
        }
    }
    for (auto &&key : declaredKeys) {
        topScope->EraseBinding(key);
    }

    auto *varbinder = static_cast<varbinder::ETSBinder *>(program->VarBinder());
    varbinder->GetGlobalRecordTable()->CleanUp();
    varbinder->Functions().clear();

    ctx->transitionMemory->SetGlobalTypes(ctx->GetChecker()->GetGlobalTypesHolder());
    ctx->transitionMemory->SetCachechedComputedAbstracts(
        ctx->GetChecker()->AsETSChecker()->GetCachedComputedAbstracts());
    ctx->transitionMemory->AddCompiledProgram(ctx->parserProgram);
}

static pandasm::Program *EmitProgram(CompilerImpl *compilerImpl, public_lib::Context *context,
                                     const CompilationUnit &unit)
{
    ES2PANDA_PERF_SCOPE("@EmitProgram");
    context->emitter->GenAnnotation();
    auto result = compilerImpl->Emit(context);
    if (unit.ext == ScriptExtension::ETS && context->compilingState != public_lib::CompilingState::SINGLE_COMPILING) {
        SavePermanents(context, context->parserProgram);
    }
    return result;
}

static bool ExecuteParsingAndCompiling(const CompilationUnit &unit, public_lib::Context *context)
{
    ES2PANDA_PERF_SCOPE("@phases");
    parser::Program *program = context->parserProgram;
    if (unit.ext == ScriptExtension::ETS &&
        context->compilingState == public_lib::CompilingState::MULTI_COMPILING_FOLLOW) {
        AddExternalPrograms(context, unit, program);
    }

    if (context->config->options->GetCompilationMode() == CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE &&
        context->config->options->GetExtension() == ScriptExtension::ETS) {
        std::unordered_set<std::string> sourceFileNamesSet;
        util::UString absolutePath(os::GetAbsolutePath(context->sourceFile->filePath), context->allocator);
        sourceFileNamesSet.insert(absolutePath.View().Mutf8());
        context->sourceFileNames.emplace_back(absolutePath.View().Utf8());
        parser::ETSParser::AddGenExtenralSourceToParseList(context);
        context->MarkGenAbcForExternal(sourceFileNamesSet, context->parserProgram->ExternalSources());
    } else {
        context->parser->ParseScript(unit.input, unit.options.GetCompilationMode() == CompilationMode::GEN_STD_LIB);
    }

    //  We have to check the return status of 'RunVerifierAndPhase` and 'RunPhases` separately because there can be
    //  some internal errors (say, in Post-Conditional check) or terminate options (say in 'CheckOptionsAfterPhase')
    //  that were not reported to the log.
    if (unit.ext == ScriptExtension::ETS) {
        if (!RunVerifierAndPhases(*context, *program)) {
            return false;
        }
    } else if (context->diagnosticEngine->IsAnyError()) {
        if (unit.options.IsDumpAst()) {
            std::cout << program->Dump() << std::endl;
        }
    } else if (!RunPhases(*context, *program)) {
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

template <typename Parser, typename VarBinder, typename Checker, typename Analyzer, typename AstCompiler,
          typename CodeGen, typename RegSpiller, typename FunctionEmitter, typename Emitter>
static pandasm::Program *Compile(const CompilationUnit &unit, CompilerImpl *compilerImpl, public_lib::Context *context)
{
    ir::DisableContextHistory();
    auto config = public_lib::ConfigImpl {};
    auto phaseManager = compiler::PhaseManager(context, unit.ext, context->allocator);
    context->config = &config;
    context->config->options = &unit.options;
    context->sourceFile = &unit.input;
    context->queue = compilerImpl->Queue();
    context->plugins = &compilerImpl->Plugins();
    auto program = parser::Program::NewProgram<VarBinder>(
        context->allocator, context->compilingState == public_lib::CompilingState::MULTI_COMPILING_FOLLOW
                                ? context->transitionMemory->VarBinder()
                                : nullptr);
    auto parser =
        Parser(&program, unit.options, unit.diagnosticEngine, static_cast<parser::ParserStatus>(unit.rawParserStatus));
    context->parser = &parser;
    parser.SetContext(context);
    auto checker = Checker(context->allocator, unit.diagnosticEngine, context->allocator);
    context->parserProgram = &program;
    context->PushChecker(&checker);
    auto analyzer = Analyzer(&checker);
    checker.SetAnalyzer(&analyzer);
    context->PushAnalyzer(checker.GetAnalyzer());
    context->codeGenCb = MakeCompileJob<CodeGen, RegSpiller, FunctionEmitter, Emitter, AstCompiler>();
    context->diagnosticEngine = &unit.diagnosticEngine;
    context->phaseManager = &phaseManager;

    if constexpr (std::is_same_v<Checker, checker::ETSChecker>) {
        CreateDebuggerEvaluationPlugin(checker, *context->allocator, &program, unit.options);
        if (context->compilingState == public_lib::CompilingState::MULTI_COMPILING_FOLLOW) {
            checker.SetCachedComputedAbstracts(context->transitionMemory->CachedComputedAbstracts());
            checker.SetGlobalTypesHolder(context->transitionMemory->GlobalTypes());
            checker.AddStatus(ark::es2panda::checker::CheckerStatus::BUILTINS_INITIALIZED);
        } else {
            checker.InitCachedComputedAbstracts();
        }
    }
    auto emitter = Emitter(context);
    context->emitter = &emitter;
    auto *varbinder = program.VarBinder();
    varbinder->SetProgram(&program);
    varbinder->SetContext(context);
    context->GetChecker()->Initialize(varbinder);

    if (!ExecuteParsingAndCompiling(unit, context)) {
        return ClearContextAndReturnProgam(context, nullptr);
    }

    MarkAsLowered(program);
    return ClearContextAndReturnProgam(context, EmitProgram(compilerImpl, context, unit));
}

pandasm::Program *CompilerImpl::Compile(const CompilationUnit &unit, public_lib::Context *context)
{
    switch (unit.ext) {
        case ScriptExtension::TS: {
            return compiler::Compile<parser::TSParser, varbinder::TSBinder, checker::TSChecker, checker::TSAnalyzer,
                                     compiler::JSCompiler, compiler::PandaGen, compiler::DynamicRegSpiller,
                                     compiler::JSFunctionEmitter, compiler::JSEmitter>(unit, this, context);
        }
        case ScriptExtension::AS: {
            return compiler::Compile<parser::ASParser, varbinder::ASBinder, checker::ASChecker, checker::TSAnalyzer,
                                     compiler::JSCompiler, compiler::PandaGen, compiler::DynamicRegSpiller,
                                     compiler::JSFunctionEmitter, compiler::JSEmitter>(unit, this, context);
        }
        case ScriptExtension::ETS: {
            return compiler::Compile<parser::ETSParser, varbinder::ETSBinder, checker::ETSChecker, checker::ETSAnalyzer,
                                     compiler::ETSCompiler, compiler::ETSGen, compiler::StaticRegSpiller,
                                     compiler::ETSFunctionEmitter, compiler::ETSEmitter>(unit, this, context);
        }
        case ScriptExtension::JS: {
            return compiler::Compile<parser::JSParser, varbinder::JSBinder, checker::JSChecker, checker::TSAnalyzer,
                                     compiler::JSCompiler, compiler::PandaGen, compiler::DynamicRegSpiller,
                                     compiler::JSFunctionEmitter, compiler::JSEmitter>(unit, this, context);
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
