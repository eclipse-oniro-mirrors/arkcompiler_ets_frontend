/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "es2panda_lib.h"
#include <cstddef>
#include <cstring>
#include <cstdint>

#include "util/diagnostic.h"
#include "varbinder/varbinder.h"
#include "varbinder/scope.h"
#include "public/public.h"
#include "generated/signatures.h"
#include "es2panda.h"
#include "varbinder/ETSBinder.h"
#include "checker/ETSAnalyzer.h"
#include "checker/ETSchecker.h"
#include "compiler/core/compileQueue.h"
#include "compiler/core/compilerImpl.h"
#include "compiler/core/ETSCompiler.h"
#include "compiler/core/ETSemitter.h"
#include "compiler/core/ETSGen.h"
#include "compiler/core/regSpiller.h"
#include "compiler/lowering/phase.h"
#include "ir/astNode.h"
#include "ir/expressions/arrowFunctionExpression.h"
#include "ir/ts/tsAsExpression.h"
#include "ir/expressions/assignmentExpression.h"
#include "ir/expressions/binaryExpression.h"
#include "ir/statements/blockStatement.h"
#include "ir/expressions/callExpression.h"
#include "ir/base/classProperty.h"
#include "ir/ets/etsFunctionType.h"
#include "ir/statements/ifStatement.h"
#include "ir/base/methodDefinition.h"
#include "ir/ets/etsNewClassInstanceExpression.h"
#include "ir/ets/etsNewArrayInstanceExpression.h"
#include "ir/ets/etsNewMultiDimArrayInstanceExpression.h"
#include "parser/ETSparser.h"
#include "parser/context/parserContext.h"
#include "parser/program/program.h"
#include "util/generateBin.h"
#include "util/options.h"
#include "compiler/lowering/util.h"
#include "generated/es2panda_lib/es2panda_lib_include.inc"
#include "declgen_ets2ts/declgenEts2Ts.h"

// NOLINTBEGIN

namespace ark::es2panda::public_lib {

struct TokenTypeToStr {
    lexer::TokenType token;
    char const *str;
};

__attribute__((unused)) lexer::TokenType StrToToken(TokenTypeToStr const *table, char const *str)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (auto *tp = table; tp->str != nullptr; tp++) {
        if (strcmp(str, tp->str) == 0) {
            return tp->token;
        }
    }
    ES2PANDA_UNREACHABLE();
}

__attribute__((unused)) char const *TokenToStr(TokenTypeToStr const *table, lexer::TokenType token)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (auto *tp = table; tp->str != nullptr; tp++) {
        if (tp->token == token) {
            return tp->str;
        }
    }
    ES2PANDA_UNREACHABLE();
}

char *StringViewToCString(ArenaAllocator *allocator, std::string_view const utf8)
{
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic, readability-simplify-subscript-expr)
    if (!utf8.empty() && (utf8.back() == '\0')) {
        // Avoid superfluous allocation.
        return const_cast<char *>(utf8.data());
    }
    char *res = reinterpret_cast<char *>(allocator->Alloc(utf8.size() + 1));
    if (!utf8.empty()) {
        [[maybe_unused]] auto err = memmove_s(res, utf8.size() + 1, utf8.cbegin(), utf8.size());
        ES2PANDA_ASSERT(err == EOK);
    }
    res[utf8.size()] = '\0';
    return res;
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic, readability-simplify-subscript-expr)
}

__attribute__((unused)) char *StringViewToCString(ArenaAllocator *allocator, util::StringView const sv)
{
    return StringViewToCString(allocator, sv.Utf8());
}

__attribute__((unused)) char *StdStringToCString(ArenaAllocator *allocator, std::string str)
{
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic, readability-simplify-subscript-expr)
    char *res = reinterpret_cast<char *>(allocator->Alloc(str.length() + 1));
    [[maybe_unused]] auto err = memcpy_s(res, str.length() + 1, str.c_str(), str.length() + 1);
    ES2PANDA_ASSERT(err == EOK);
    return res;
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic, readability-simplify-subscript-expr)
}

__attribute__((unused)) char *UStringToCString(ArenaAllocator *allocator, util::UString const sv)
{
    return StringViewToCString(allocator, sv.View());
}

__attribute__((unused)) es2panda_variantDoubleCharArrayBool EnumMemberResultToEs2pandaVariant(
    ArenaAllocator *allocator, varbinder::EnumMemberResult variant)
{
    // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic, readability-simplify-subscript-expr)
    es2panda_variantDoubleCharArrayBool es2panda_variant;
    es2panda_variant.index = static_cast<int>(variant.index());
    switch (es2panda_variant.index) {
        case es2panda_variantIndex::CAPI_DOUBLE:
            es2panda_variant.variant.d = std::get<double>(variant);
            break;
        case es2panda_variantIndex::CAPI_CHAR:
            es2panda_variant.variant.c = StringViewToCString(allocator, std::get<util::StringView>(variant));
            break;
        case es2panda_variantIndex::CAPI_BOOL:
            es2panda_variant.variant.b = std::get<bool>(variant);
            break;
        default:
            break;
    }
    return es2panda_variant;
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic, readability-simplify-subscript-expr)
    // NOLINTEND(cppcoreguidelines-pro-type-union-access)
}

__attribute__((unused)) es2panda_DynamicImportData *DynamicImportDataToE2pPtr(
    ArenaAllocator *allocator, const varbinder::DynamicImportData *dynamicImportData)
{
    auto import = reinterpret_cast<const es2panda_AstNode *>(dynamicImportData->import);
    auto specifier = reinterpret_cast<const es2panda_AstNode *>(dynamicImportData->specifier);
    auto variable = reinterpret_cast<es2panda_Variable *>(dynamicImportData->variable);
    auto es2pandaDynamicImportData = allocator->New<es2panda_DynamicImportData>();
    es2pandaDynamicImportData->import = import;
    es2pandaDynamicImportData->specifier = specifier;
    es2pandaDynamicImportData->variable = variable;
    return es2pandaDynamicImportData;
}

__attribute__((unused)) es2panda_DynamicImportData DynamicImportDataToE2p(
    const varbinder::DynamicImportData dynamicImportData)
{
    auto import = reinterpret_cast<const es2panda_AstNode *>(dynamicImportData.import);
    auto specifier = reinterpret_cast<const es2panda_AstNode *>(dynamicImportData.specifier);
    auto variable = reinterpret_cast<es2panda_Variable *>(dynamicImportData.variable);
    es2panda_DynamicImportData es2pandaDynamicImportData;
    es2pandaDynamicImportData.import = import;
    es2pandaDynamicImportData.specifier = specifier;
    es2pandaDynamicImportData.variable = variable;
    return es2pandaDynamicImportData;
}

__attribute__((unused)) es2panda_OverloadInfo *OverloadInfoToE2pPtr(ArenaAllocator *allocator,
                                                                    const ir::OverloadInfo *overloadInfo)
{
    auto es2pandaOverloadInfo = allocator->New<es2panda_OverloadInfo>();
    es2pandaOverloadInfo->minArg = overloadInfo->minArg;
    es2pandaOverloadInfo->maxArg = overloadInfo->maxArg;
    es2pandaOverloadInfo->needHelperOverload = overloadInfo->needHelperOverload;
    es2pandaOverloadInfo->isDeclare = overloadInfo->isDeclare;
    es2pandaOverloadInfo->hasRestVar = overloadInfo->hasRestVar;
    es2pandaOverloadInfo->returnVoid = overloadInfo->returnVoid;
    return es2pandaOverloadInfo;
}

__attribute__((unused)) es2panda_OverloadInfo OverloadInfoToE2p(const ir::OverloadInfo overloadInfo)
{
    es2panda_OverloadInfo es2pandaOverloadInfo;
    es2pandaOverloadInfo.minArg = overloadInfo.minArg;
    es2pandaOverloadInfo.maxArg = overloadInfo.maxArg;
    es2pandaOverloadInfo.needHelperOverload = overloadInfo.needHelperOverload;
    es2pandaOverloadInfo.isDeclare = overloadInfo.isDeclare;
    es2pandaOverloadInfo.hasRestVar = overloadInfo.hasRestVar;
    es2pandaOverloadInfo.returnVoid = overloadInfo.returnVoid;
    return es2pandaOverloadInfo;
}

__attribute__((unused)) char const *ArenaStrdup(ArenaAllocator *allocator, char const *src)
{
    size_t len = strlen(src);
    char *res = reinterpret_cast<char *>(allocator->Alloc(len + 1));
    [[maybe_unused]] auto err = memmove_s(res, len + 1, src, len);
    ES2PANDA_ASSERT(err == EOK);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    res[len] = '\0';
    return res;
}

extern "C" void MemInitialize()
{
    if (mem::MemConfig::IsInitialized()) {
        return;
    }
    mem::MemConfig::Initialize(0, 0, COMPILER_SIZE, 0, 0, 0);
    PoolManager::Initialize(PoolType::MMAP);
}

extern "C" void MemFinalize()
{
    PoolManager::Finalize();
    mem::MemConfig::Finalize();
}

extern "C" es2panda_Config *CreateConfig(int args, char const *const *argv)
{
    MemInitialize();
    auto diagnosticEngine = new util::DiagnosticEngine();
    auto *options = new util::Options(argv[0], *diagnosticEngine);
    if (!options->Parse(Span(argv, args))) {
        return nullptr;
    }
    ark::Logger::ComponentMask mask {};
    mask.set(ark::Logger::Component::ES2PANDA);
    ark::Logger::InitializeStdLogging(options->LogLevel(), mask);

    auto *res = new ConfigImpl;
    res->options = options;
    res->diagnosticEngine = diagnosticEngine;
    return reinterpret_cast<es2panda_Config *>(res);
}

extern "C" void DestroyConfig(es2panda_Config *config)
{
    auto *cfg = reinterpret_cast<ConfigImpl *>(config);
    if (cfg == nullptr) {
        return;
    }

    delete cfg->options;
    cfg->diagnosticEngine->FlushDiagnostic();
    delete cfg->diagnosticEngine;
    delete cfg;
}

extern "C" __attribute__((unused)) char const *GetAllErrorMessages(es2panda_Context *context)
{
    auto *ctx = reinterpret_cast<Context *>(context);
    ES2PANDA_ASSERT(ctx != nullptr);
    ES2PANDA_ASSERT(ctx->config != nullptr);
    ES2PANDA_ASSERT(ctx->allocator != nullptr);
    auto *cfg = reinterpret_cast<ConfigImpl *>(ctx->config);
    ES2PANDA_ASSERT(cfg != nullptr);
    ES2PANDA_ASSERT(cfg->diagnosticEngine != nullptr);
    auto allMessages = cfg->diagnosticEngine->PrintAndFlushErrorDiagnostic();
    size_t bufferSize = allMessages.length() + 1;
    char *cStringMessages = reinterpret_cast<char *>(ctx->allocator->Alloc(bufferSize));
    [[maybe_unused]] auto err = memcpy_s(cStringMessages, bufferSize, allMessages.c_str(), bufferSize);
    ES2PANDA_ASSERT(err == EOK);
    return cStringMessages;
}

extern "C" const es2panda_Options *ConfigGetOptions(es2panda_Config *config)
{
    auto options = reinterpret_cast<ConfigImpl *>(config)->options;
    return reinterpret_cast<const es2panda_Options *>(options);
}

static void CompileJob(public_lib::Context *context, varbinder::FunctionScope *scope,
                       compiler::ProgramElement *programElement)
{
    compiler::StaticRegSpiller regSpiller;
    ArenaAllocator allocator {SpaceType::SPACE_TYPE_COMPILER, nullptr, true};
    compiler::ETSCompiler astCompiler {};
    compiler::ETSGen cg {&allocator, &regSpiller, context, std::make_tuple(scope, programElement, &astCompiler)};
    compiler::ETSFunctionEmitter funcEmitter {&cg, programElement};
    funcEmitter.Generate();
}

static void GenerateStdLibCache(es2panda_Config *config, GlobalContext *globalContext, bool LspUsage);

extern "C" __attribute__((unused)) es2panda_GlobalContext *CreateGlobalContext(es2panda_Config *config,
                                                                               const char **externalFileList,
                                                                               size_t fileNum, bool LspUsage)
{
    auto *globalContext = new GlobalContext;
    for (size_t i = 0; i < fileNum; i++) {
        auto fileName = externalFileList[i];
        auto globalAllocator = new ThreadSafeArenaAllocator(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);
        globalContext->cachedExternalPrograms.emplace(fileName, nullptr);
        globalContext->externalProgramAllocators.emplace(fileName, globalAllocator);
    }

    GenerateStdLibCache(config, globalContext, LspUsage);

    return reinterpret_cast<es2panda_GlobalContext *>(globalContext);
}

extern "C" __attribute__((unused)) void RemoveFileCache(es2panda_GlobalContext *globalContext, const char *fileName)
{
    auto globalCtx = reinterpret_cast<GlobalContext *>(globalContext);
    ES2PANDA_ASSERT(globalCtx->cachedExternalPrograms.count(fileName) == 1);
    globalCtx->cachedExternalPrograms.erase(fileName);
    delete globalCtx->externalProgramAllocators[fileName];
    globalCtx->externalProgramAllocators.erase(fileName);
}

extern "C" __attribute__((unused)) void AddFileCache(es2panda_GlobalContext *globalContext, const char *fileName)
{
    auto globalCtx = reinterpret_cast<GlobalContext *>(globalContext);
    ES2PANDA_ASSERT(globalCtx->cachedExternalPrograms.count(fileName) == 0);
    auto globalAllocator = new ThreadSafeArenaAllocator(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);
    globalCtx->cachedExternalPrograms.emplace(fileName, nullptr);
    globalCtx->externalProgramAllocators.emplace(fileName, globalAllocator);
}

extern "C" __attribute__((unused)) void InvalidateFileCache(es2panda_GlobalContext *globalContext, const char *fileName)
{
    RemoveFileCache(globalContext, fileName);
    AddFileCache(globalContext, fileName);
}

static void InitializeContext(Context *res)
{
    res->phaseManager = new compiler::PhaseManager(res, ScriptExtension::ETS, res->allocator);
    res->queue = new compiler::CompileQueue(res->config->options->GetThread());

    auto *varbinder = res->allocator->New<varbinder::ETSBinder>(res->allocator);
    res->parserProgram = res->allocator->New<parser::Program>(res->allocator, varbinder);
    res->parser = new parser::ETSParser(res->parserProgram, *res->config->options, *res->diagnosticEngine,
                                        parser::ParserStatus::NO_OPTS);
    res->parser->SetContext(res);

    res->PushChecker(res->allocator->New<checker::ETSChecker>(res->allocator, *res->diagnosticEngine, res->allocator));
    res->PushAnalyzer(res->allocator->New<checker::ETSAnalyzer>(res->GetChecker()));
    res->GetChecker()->SetAnalyzer(res->GetAnalyzer());

    varbinder->SetProgram(res->parserProgram);
    varbinder->SetContext(res);
    res->codeGenCb = CompileJob;
    res->emitter = new compiler::ETSEmitter(res);
    res->program = nullptr;
    res->state = ES2PANDA_STATE_NEW;
}

static Context *InitContext(es2panda_Config *config)
{
    auto *cfg = reinterpret_cast<ConfigImpl *>(config);
    auto *res = new Context;
    res->compiledByCapi = true;
    if (cfg == nullptr) {
        res->errorMessage = "Config is nullptr.";
        res->state = ES2PANDA_STATE_ERROR;
        return res;
    }

    if (cfg->options->GetExtension() != ScriptExtension::ETS) {
        res->errorMessage = "Invalid extension. Plugin API supports only ETS.";
        res->state = ES2PANDA_STATE_ERROR;
        res->diagnosticEngine = cfg->diagnosticEngine;
        return res;
    }

    res->config = cfg;
    res->diagnosticEngine = cfg->diagnosticEngine;
    return res;
}

__attribute__((unused)) static es2panda_Context *CreateContext(es2panda_Config *config, std::string &&source,
                                                               const char *fileName,
                                                               es2panda_GlobalContext *globalContext, bool isExternal,
                                                               bool genStdLib)
{
    auto res = InitContext(config);
    if (res->state == ES2PANDA_STATE_ERROR) {
        return reinterpret_cast<es2panda_Context *>(res);
    }
    auto *cfg = reinterpret_cast<ConfigImpl *>(config);

    res->isExternal = isExternal;
    res->globalContext = reinterpret_cast<GlobalContext *>(globalContext);

    res->input = std::move(source);
    res->sourceFileName = fileName;
    res->sourceFile = new SourceFile(res->sourceFileName, res->input, cfg->options->IsModule());
    if (isExternal) {
        ir::EnableContextHistory();
        ES2PANDA_ASSERT(res->globalContext != nullptr);
        if (genStdLib) {
            ES2PANDA_ASSERT(res->globalContext->stdLibAllocator != nullptr);
            res->allocator = res->globalContext->stdLibAllocator;
        } else {
            ES2PANDA_ASSERT(res->globalContext->externalProgramAllocators.count(fileName) != 0);
            res->allocator =
                reinterpret_cast<ThreadSafeArenaAllocator *>(res->globalContext->externalProgramAllocators[fileName]);
        }
    } else {
        ir::DisableContextHistory();
        res->allocator = new ThreadSafeArenaAllocator(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);
    }

    InitializeContext(res);
    return reinterpret_cast<es2panda_Context *>(res);
}

__attribute__((unused)) static std::stringstream ReadFile(char const *sourceFileName, Context *&res)
{
    std::ifstream inputStream;
    inputStream.open(sourceFileName);
    if (inputStream.fail()) {
        res = new Context;
        res->errorMessage = "Failed to open file: ";
        res->errorMessage.append(sourceFileName);
    }
    std::stringstream ss;
    ss << inputStream.rdbuf();
    if (inputStream.fail()) {
        res = new Context;
        res->errorMessage = "Failed to read file: ";
        res->errorMessage.append(sourceFileName);
    }
    return ss;
}

extern "C" __attribute__((unused)) es2panda_Context *CreateCacheContextFromFile(es2panda_Config *config,
                                                                                char const *sourceFileName,
                                                                                es2panda_GlobalContext *globalContext,
                                                                                bool isExternal)
{
    Context *res = nullptr;
    auto ss = ReadFile(sourceFileName, res);
    if (res != nullptr) {
        return reinterpret_cast<es2panda_Context *>(res);
    }
    return CreateContext(config, ss.str(), sourceFileName, globalContext, isExternal, false);
}

extern "C" __attribute__((unused)) es2panda_Context *CreateContextFromFile(es2panda_Config *config,
                                                                           char const *sourceFileName)
{
    Context *res = nullptr;
    auto ss = ReadFile(sourceFileName, res);
    if (res != nullptr) {
        return reinterpret_cast<es2panda_Context *>(res);
        ;
    }

    return CreateContext(config, ss.str(), sourceFileName, nullptr, false, false);
}

extern "C" __attribute__((unused)) es2panda_Context *CreateCacheContextFromString(es2panda_Config *config,
                                                                                  const char *source,
                                                                                  char const *fileName,
                                                                                  es2panda_GlobalContext *globalContext,
                                                                                  bool isExternal)
{
    return CreateContext(config, std::string(source), fileName, globalContext, isExternal, false);
}

extern "C" __attribute__((unused)) es2panda_Context *CreateContextFromMultiFile(es2panda_Config *config,
                                                                                char const *sourceFileNames)
{
    return CreateContext(config, "", sourceFileNames, nullptr, false, false);
}

extern "C" __attribute__((unused)) es2panda_Context *CreateContextFromString(es2panda_Config *config,
                                                                             const char *source, char const *fileName)
{
    // NOTE: gogabr. avoid copying source.
    return CreateContext(config, std::string(source), fileName, nullptr, false, false);
}

extern __attribute__((unused)) es2panda_Context *CreateContextGenerateAbcForExternalSourceFiles(
    es2panda_Config *config, int fileNamesCount, char const *const *fileNames)
{
    auto res = InitContext(config);
    if (res->state == ES2PANDA_STATE_ERROR) {
        return reinterpret_cast<es2panda_Context *>(res);
    }
    auto *cfg = reinterpret_cast<ConfigImpl *>(config);

    ES2PANDA_ASSERT(cfg->options->IsSimultaneous());
    for (size_t i = 0; i < static_cast<size_t>(fileNamesCount); ++i) {
        const char *cName = *(fileNames + i);
        std::string fileName(cName);
        res->sourceFileNames.emplace_back(std::move(fileName));
    }

    res->input = "";
    res->sourceFileName = "";
    res->sourceFile = new SourceFile(res->sourceFileName, res->input, cfg->options->IsModule());
    ir::DisableContextHistory();
    res->allocator = new ThreadSafeArenaAllocator(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);

    InitializeContext(res);
    return reinterpret_cast<es2panda_Context *>(res);
}

extern "C" __attribute__((unused)) es2panda_Context *CreateContextFromStringWithHistory(es2panda_Config *config,
                                                                                        const char *source,
                                                                                        char const *fileName)
{
    // NOTE: gogabr. avoid copying source.
    es2panda_Context *context = CreateContextFromString(config, source, fileName);
    ir::EnableContextHistory();
    return context;
}

__attribute__((unused)) static Context *Parse(Context *ctx)
{
    if (ctx->state != ES2PANDA_STATE_NEW) {
        ctx->state = ES2PANDA_STATE_ERROR;
        ctx->errorMessage = "Bad state at entry to Parse, needed NEW";
        return ctx;
    }

    ctx->phaseManager->Reset();

    if (ctx->isExternal && ctx->allocator != ctx->globalContext->stdLibAllocator) {
        ctx->sourceFileNames.emplace_back(ctx->sourceFileName);
        parser::ETSParser::AddGenExtenralSourceToParseList(ctx);
    } else if (ctx->config->options->IsSimultaneous()) {
        parser::ETSParser::AddGenExtenralSourceToParseList(ctx);
        std::unordered_set<std::string> sourceFileNamesSet(ctx->sourceFileNames.begin(), ctx->sourceFileNames.end());
        ctx->MarkGenAbcForExternal(sourceFileNamesSet, ctx->parserProgram->ExternalSources());
    } else {
        ctx->parser->ParseScript(*ctx->sourceFile,
                                 ctx->config->options->GetCompilationMode() == CompilationMode::GEN_STD_LIB);
    }
    ctx->state = ES2PANDA_STATE_PARSED;
    ctx->phaseManager->SetCurrentPhaseIdToAfterParse();
    return ctx;
}

__attribute__((unused)) static bool SetProgramGenAbc(Context *ctx, const char *path)
{
    util::StringView pathView(path);
    public_lib::Context *context = reinterpret_cast<public_lib::Context *>(ctx);
    for (auto &[_, extPrograms] : context->externalSources) {
        (void)_;
        for (auto *prog : extPrograms) {
            if (prog->AbsoluteName() == pathView) {
                prog->SetGenAbcForExternalSources();
                return true;
            }
        }
    }
    return false;
}

__attribute__((unused)) static Context *Bind(Context *ctx)
{
    if (ctx->state < ES2PANDA_STATE_PARSED) {
        ctx = Parse(ctx);
    }
    if (ctx->state == ES2PANDA_STATE_ERROR) {
        return ctx;
    }

    ES2PANDA_ASSERT(ctx->state == ES2PANDA_STATE_PARSED);
    while (auto phase = ctx->phaseManager->NextPhase()) {
        if (phase->Name() == "plugins-after-bind") {
            break;
        }
        phase->Apply(ctx, ctx->parserProgram);
    }
    ctx->state = ES2PANDA_STATE_BOUND;
    return ctx;
}

__attribute__((unused)) static Context *Check(Context *ctx)
{
    if (ctx->state < ES2PANDA_STATE_PARSED) {
        ctx = Parse(ctx);
    }

    if (ctx->state == ES2PANDA_STATE_ERROR) {
        return ctx;
    }

    ES2PANDA_ASSERT(ctx->state >= ES2PANDA_STATE_PARSED && ctx->state < ES2PANDA_STATE_CHECKED);
    while (auto phase = ctx->phaseManager->NextPhase()) {
        if (phase->Name() == "plugins-after-check") {
            break;
        }
        phase->Apply(ctx, ctx->parserProgram);
    }
    ctx->phaseManager->SetCurrentPhaseIdToAfterCheck();
    ctx->state = !ctx->diagnosticEngine->IsAnyError() ? ES2PANDA_STATE_CHECKED : ES2PANDA_STATE_ERROR;
    return ctx;
}

__attribute__((unused)) static void SaveCache(Context *ctx)
{
    if (ctx->allocator == ctx->globalContext->stdLibAllocator) {
        return;
    }
    ES2PANDA_ASSERT(ctx->globalContext != nullptr &&
                    ctx->globalContext->cachedExternalPrograms.count(ctx->sourceFileName) != 0);
    ctx->globalContext->cachedExternalPrograms[ctx->sourceFileName] = &(ctx->parserProgram->ExternalSources());

    // cycle dependencies
    for (auto &[_, extPrograms] : ctx->parserProgram->ExternalSources()) {
        for (auto extProgram : extPrograms) {
            auto absPath = std::string {extProgram->AbsoluteName()};
            auto &cacheMap = ctx->globalContext->cachedExternalPrograms;
            if (cacheMap.count(absPath) == 1 && cacheMap[absPath] == nullptr) {
                cacheMap[absPath] = &(ctx->parserProgram->ExternalSources());
            }
        }
    }
}

__attribute__((unused)) static Context *Lower(Context *ctx)
{
    if (ctx->state < ES2PANDA_STATE_CHECKED) {
        ctx = Check(ctx);
    }

    if (ctx->state == ES2PANDA_STATE_ERROR) {
        return ctx;
    }

    ES2PANDA_ASSERT(ctx->state == ES2PANDA_STATE_CHECKED);
    while (auto phase = ctx->phaseManager->NextPhase()) {
        phase->Apply(ctx, ctx->parserProgram);
    }
    ctx->state = !ctx->diagnosticEngine->IsAnyError() ? ES2PANDA_STATE_LOWERED : ES2PANDA_STATE_ERROR;

    for (auto &[_, extPrograms] : ctx->parserProgram->ExternalSources()) {
        for (auto &extProgram : extPrograms) {
            if (!extProgram->IsASTLowered()) {
                extProgram->MarkASTAsLowered();
            }
        }
    }
    if (ctx->isExternal) {
        SaveCache(ctx);
    }

    return ctx;
}

__attribute__((unused)) static Context *GenerateAsm(Context *ctx)
{
    if (ctx->state < ES2PANDA_STATE_LOWERED) {
        ctx = Lower(ctx);
    }

    if (ctx->state == ES2PANDA_STATE_ERROR) {
        return ctx;
    }

    ES2PANDA_ASSERT(ctx->state == ES2PANDA_STATE_LOWERED);

    auto *emitter = ctx->emitter;
    emitter->GenAnnotation();

    // Handle context literals.
    uint32_t index = 0;
    for (const auto &buff : ctx->contextLiterals) {
        emitter->AddLiteralBuffer(buff, index++);
    }

    emitter->LiteralBufferIndex() += ctx->contextLiterals.size();

    /* Main thread can also be used instead of idling */
    ctx->queue->Schedule(ctx);
    ctx->queue->Consume();
    ctx->queue->Wait([emitter](compiler::CompileJob *job) { emitter->AddProgramElement(job->GetProgramElement()); });
    ES2PANDA_ASSERT(ctx->program == nullptr);
    ctx->program = emitter->Finalize(ctx->config->options->IsDumpDebugInfo(), compiler::Signatures::ETS_GLOBAL);
    ctx->state = !ctx->diagnosticEngine->IsAnyError() ? ES2PANDA_STATE_ASM_GENERATED : ES2PANDA_STATE_ERROR;
    return ctx;
}

__attribute__((unused)) Context *GenerateBin(Context *ctx)
{
    if (ctx->state < ES2PANDA_STATE_ASM_GENERATED) {
        ctx = GenerateAsm(ctx);
    }

    if (ctx->state == ES2PANDA_STATE_ERROR) {
        return ctx;
    }

    ES2PANDA_ASSERT(ctx->state == ES2PANDA_STATE_ASM_GENERATED);

    ES2PANDA_ASSERT(ctx->program != nullptr);
    util::GenerateProgram(ctx->program, *ctx->config->options,
                          [ctx](const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &params) {
                              ctx->diagnosticEngine->LogDiagnostic(kind, params);
                          });
    ctx->state = !ctx->diagnosticEngine->IsAnyError() ? ES2PANDA_STATE_BIN_GENERATED : ES2PANDA_STATE_ERROR;
    return ctx;
}

extern "C" __attribute__((unused)) es2panda_Context *ProceedToState(es2panda_Context *context,
                                                                    es2panda_ContextState state)
{
    auto *ctx = reinterpret_cast<Context *>(context);
    switch (state) {
        case ES2PANDA_STATE_NEW:
            break;
        case ES2PANDA_STATE_PARSED:
            ctx = Parse(ctx);
            break;
        case ES2PANDA_STATE_BOUND:
            ctx = Bind(ctx);
            break;
        case ES2PANDA_STATE_CHECKED:
            ctx = Check(ctx);
            break;
        case ES2PANDA_STATE_LOWERED:
            ctx = Lower(ctx);
            break;
        case ES2PANDA_STATE_ASM_GENERATED:
            ctx = GenerateAsm(ctx);
            break;
        case ES2PANDA_STATE_BIN_GENERATED:
            ctx = GenerateBin(ctx);
            break;
        default:
            ctx->errorMessage = "It does not make sense to request stage";
            ctx->state = ES2PANDA_STATE_ERROR;
            break;
    }

    return reinterpret_cast<es2panda_Context *>(ctx);
}

extern "C" __attribute__((unused)) void DestroyContext(es2panda_Context *context)
{
    auto *ctx = reinterpret_cast<Context *>(context);
    delete ctx->program;
    delete ctx->emitter;
    delete ctx->parser;
    delete ctx->queue;
    delete ctx->sourceFile;
    delete ctx->phaseManager;
    if (!ctx->isExternal) {
        delete ctx->allocator;
    }
    delete ctx;
}

extern "C" __attribute__((unused)) void DestroyGlobalContext(es2panda_GlobalContext *globalContext)
{
    auto *globalCtx = reinterpret_cast<GlobalContext *>(globalContext);
    for (auto [_, alloctor] : globalCtx->externalProgramAllocators) {
        delete alloctor;
    }

    delete globalCtx->stdLibAllocator;
    delete globalCtx;
}

extern "C" __attribute__((unused)) es2panda_ContextState ContextState(es2panda_Context *context)
{
    auto *s = reinterpret_cast<Context *>(context);
    return s->state;
}

extern "C" __attribute__((unused)) char const *ContextErrorMessage(es2panda_Context *context)
{
    auto *s = reinterpret_cast<Context *>(context);
    return s->errorMessage.c_str();
}

extern "C" __attribute__((unused)) es2panda_Program *ContextProgram(es2panda_Context *context)
{
    auto *ctx = reinterpret_cast<Context *>(context);
    return reinterpret_cast<es2panda_Program *>(ctx->parserProgram);
}

using ExternalSourceEntry = std::pair<char *, ArenaVector<parser::Program *> *>;

__attribute__((unused)) static es2panda_ExternalSource **ExternalSourcesToE2p(
    ArenaAllocator *allocator, const parser::Program::ExternalSource &externalSources, size_t *lenP)
{
    auto *vec = allocator->New<ArenaVector<ExternalSourceEntry *>>(allocator->Adapter());

    for (auto &[e_name, e_programs] : externalSources) {
        vec->push_back(allocator->New<ExternalSourceEntry>(StringViewToCString(allocator, e_name),
                                                           const_cast<ArenaVector<parser::Program *> *>(&e_programs)));
    }

    *lenP = vec->size();
    return reinterpret_cast<es2panda_ExternalSource **>(vec->data());
}

extern "C" __attribute__((unused)) char const *ExternalSourceName(es2panda_ExternalSource *eSource)
{
    auto *entry = reinterpret_cast<ExternalSourceEntry *>(eSource);
    return entry->first;
}

extern "C" __attribute__((unused)) es2panda_Program **ExternalSourcePrograms(es2panda_ExternalSource *eSource,
                                                                             size_t *lenP)
{
    auto *entry = reinterpret_cast<ExternalSourceEntry *>(eSource);
    *lenP = entry->second->size();
    return reinterpret_cast<es2panda_Program **>(entry->second->data());
}

extern "C" void AstNodeForEach(es2panda_AstNode *ast, void (*func)(es2panda_AstNode *, void *), void *arg)
{
    auto *node = reinterpret_cast<ir::AstNode *>(ast);
    func(ast, arg);
    node->IterateRecursively([=](ir::AstNode *child) { func(reinterpret_cast<es2panda_AstNode *>(child), arg); });
}

#define SET_NUMBER_LITERAL_IMPL(name, type)                                        \
    extern "C" bool NumberLiteralSet##name(es2panda_AstNode *node, type new_value) \
    {                                                                              \
        auto &n = reinterpret_cast<ir::NumberLiteral *>(node)->Number();           \
        if (!n.Is##name()) {                                                       \
            /* CC-OFFNXT(G.PRE.05) function gen */                                 \
            return false;                                                          \
        }                                                                          \
        n.SetValue<type>(std::move(new_value));                                    \
        /* CC-OFFNXT(G.PRE.05) The macro is used to generate a function. */        \
        return true;                                                               \
    }

SET_NUMBER_LITERAL_IMPL(Int, int32_t)
SET_NUMBER_LITERAL_IMPL(Long, int64_t)
SET_NUMBER_LITERAL_IMPL(Double, double)
SET_NUMBER_LITERAL_IMPL(Float, float)

#undef SET_NUMBER_LITERAL_IMPL

template <typename T>
es2panda_AstNode *CreateNumberLiteral(es2panda_Context *ctx, T value)
{
    auto number = ark::es2panda::lexer::Number(value);
    auto allocator = reinterpret_cast<Context *>(ctx)->allocator;
    auto node = allocator->New<ir::NumberLiteral>(number);
    return reinterpret_cast<es2panda_AstNode *>(node);
}

template <typename T>
es2panda_AstNode *UpdateNumberLiteral(es2panda_Context *ctx, es2panda_AstNode *original, T value)
{
    auto number = ark::es2panda::lexer::Number(value);
    auto allocator = reinterpret_cast<Context *>(ctx)->allocator;
    auto node = allocator->New<ir::NumberLiteral>(number);
    auto *e2pOriginal = reinterpret_cast<ir::AstNode *>(original);
    node->SetOriginalNode(e2pOriginal);
    node->SetParent(e2pOriginal->Parent());
    node->SetRange(e2pOriginal->Range());
    return reinterpret_cast<es2panda_AstNode *>(node);
}

extern "C" const char *NumberLiteralStrConst(es2panda_Context *context, es2panda_AstNode *classInstance)
{
    auto str = reinterpret_cast<const ir::NumberLiteral *>(classInstance)->Str();
    return StringViewToCString(reinterpret_cast<Context *>(context)->allocator, str);
}

extern "C" void *AllocMemory(es2panda_Context *context, size_t numberOfElements, size_t sizeOfElement)
{
    auto *allocator = reinterpret_cast<Context *>(context)->allocator;
    void *ptr = allocator->Alloc(numberOfElements * sizeOfElement);
    return ptr;
}

extern "C" es2panda_SourcePosition *CreateSourcePosition(es2panda_Context *context, size_t index, size_t line)
{
    auto *allocator = reinterpret_cast<Context *>(context)->allocator;
    return reinterpret_cast<es2panda_SourcePosition *>(
        allocator->New<lexer::SourcePosition>(index, line, reinterpret_cast<Context *>(context)->parserProgram));
}

extern "C" es2panda_SourceRange *CreateSourceRange(es2panda_Context *context, es2panda_SourcePosition *start,
                                                   es2panda_SourcePosition *end)
{
    auto *allocator = reinterpret_cast<Context *>(context)->allocator;
    auto startE2p = *(reinterpret_cast<lexer::SourcePosition *>(start));
    auto endE2p = *(reinterpret_cast<lexer::SourcePosition *>(end));
    return reinterpret_cast<es2panda_SourceRange *>(allocator->New<lexer::SourceRange>(startE2p, endE2p));
}

extern "C" const es2panda_DiagnosticKind *CreateDiagnosticKind(es2panda_Context *context, const char *dmessage,
                                                               es2panda_PluginDiagnosticType etype)
{
    auto ctx = reinterpret_cast<Context *>(context);
    auto id = ctx->config->diagnosticKindStorage.size() + 1;
    auto type = util::DiagnosticType::SUGGESTION;
    if (etype == ES2PANDA_PLUGIN_WARNING) {
        type = util::DiagnosticType::PLUGIN_WARNING;
    } else if (etype == ES2PANDA_PLUGIN_ERROR) {
        type = util::DiagnosticType::PLUGIN_ERROR;
    }
    ctx->config->diagnosticKindStorage.emplace_back(type, id, dmessage);
    return reinterpret_cast<const es2panda_DiagnosticKind *>(&ctx->config->diagnosticKindStorage.back());
}

extern "C" es2panda_DiagnosticInfo *CreateDiagnosticInfo(es2panda_Context *context, const es2panda_DiagnosticKind *kind,
                                                         const char **args, size_t argc, es2panda_SourcePosition *pos)
{
    auto *allocator = reinterpret_cast<Context *>(context)->allocator;
    auto diagnosticInfo = allocator->New<es2panda_DiagnosticInfo>();
    diagnosticInfo->kind = kind;
    diagnosticInfo->args = args;
    diagnosticInfo->argc = argc;
    diagnosticInfo->pos = pos;
    return diagnosticInfo;
}

extern "C" es2panda_SuggestionInfo *CreateSuggestionInfo(es2panda_Context *context, const es2panda_DiagnosticKind *kind,
                                                         const char **args, size_t argc, const char *substitutionCode,
                                                         const char *title, es2panda_SourceRange *range)
{
    auto *allocator = reinterpret_cast<Context *>(context)->allocator;
    auto suggestionInfo = allocator->New<es2panda_SuggestionInfo>();
    suggestionInfo->kind = kind;
    suggestionInfo->args = args;
    suggestionInfo->argc = argc;
    suggestionInfo->substitutionCode = substitutionCode;
    suggestionInfo->title = title;
    suggestionInfo->range = range;
    return suggestionInfo;
}

extern "C" void LogDiagnosticWithSuggestion(es2panda_Context *context, const es2panda_DiagnosticInfo *diagnosticInfo,
                                            const es2panda_SuggestionInfo *suggestionInfo)
{
    auto ctx = reinterpret_cast<Context *>(context);
    auto diagnostickind = reinterpret_cast<const diagnostic::DiagnosticKind *>(diagnosticInfo->kind);
    auto suggestionkind = reinterpret_cast<const diagnostic::DiagnosticKind *>(suggestionInfo->kind);
    util::DiagnosticMessageParams diagnosticParams;
    for (size_t i = 0; i < diagnosticInfo->argc; ++i) {
        diagnosticParams.push_back(diagnosticInfo->args[i]);
    }

    std::vector<std::string> suggestionParams;

    for (size_t i = 0; i < suggestionInfo->argc; ++i) {
        suggestionParams.push_back(suggestionInfo->args[i]);
    }

    auto E2pRange = reinterpret_cast<lexer::SourceRange *>(suggestionInfo->range);
    auto posE2p = reinterpret_cast<lexer::SourcePosition *>(diagnosticInfo->pos);
    auto suggestion = ctx->diagnosticEngine->CreateSuggestion(
        suggestionkind, suggestionParams, suggestionInfo->substitutionCode, suggestionInfo->title, E2pRange);
    ctx->diagnosticEngine->LogDiagnostic(*diagnostickind, diagnosticParams, *posE2p, suggestion);
}

extern "C" void LogDiagnostic(es2panda_Context *context, const es2panda_DiagnosticKind *ekind, const char **args,
                              size_t argc, es2panda_SourcePosition *pos)
{
    auto ctx = reinterpret_cast<Context *>(context);
    auto kind = reinterpret_cast<const diagnostic::DiagnosticKind *>(ekind);
    util::DiagnosticMessageParams params;
    for (size_t i = 0; i < argc; ++i) {
        params.push_back(args[i]);
    }
    auto posE2p = reinterpret_cast<lexer::SourcePosition *>(pos);
    ctx->diagnosticEngine->LogDiagnostic(*kind, params, *posE2p);
}

const es2panda_DiagnosticStorage *GetDiagnostics(es2panda_Context *context, size_t etype)
{
    auto ctx = reinterpret_cast<Context *>(context);
    auto type = static_cast<util::DiagnosticType>(etype);
    return reinterpret_cast<const es2panda_DiagnosticStorage *>(&ctx->diagnosticEngine->GetDiagnosticStorage(type));
}

extern "C" const es2panda_DiagnosticStorage *GetSemanticErrors(es2panda_Context *context)
{
    return GetDiagnostics(context, util::DiagnosticType::SEMANTIC);
}

extern "C" const es2panda_DiagnosticStorage *GetSyntaxErrors(es2panda_Context *context)
{
    return GetDiagnostics(context, util::DiagnosticType::SYNTAX);
}

extern "C" const es2panda_DiagnosticStorage *GetPluginErrors(es2panda_Context *context)
{
    return GetDiagnostics(context, util::DiagnosticType::PLUGIN_ERROR);
}

extern "C" const es2panda_DiagnosticStorage *GetPluginWarnings(es2panda_Context *context)
{
    return GetDiagnostics(context, util::DiagnosticType::PLUGIN_WARNING);
}

extern "C" const es2panda_DiagnosticStorage *GetWarnings(es2panda_Context *context)
{
    return GetDiagnostics(context, util::DiagnosticType::WARNING);
}

extern "C" bool IsAnyError(es2panda_Context *context)
{
    return reinterpret_cast<Context *>(context)->diagnosticEngine->IsAnyError();
}

extern "C" size_t SourcePositionCol([[maybe_unused]] es2panda_Context *context, es2panda_SourcePosition *position)
{
    static const size_t EMPTY = 1;
    auto es2pandaPosition = reinterpret_cast<lexer::SourcePosition *>(position);
    if (es2pandaPosition->Program() == nullptr) {
        return EMPTY;
    }
    return es2pandaPosition->ToLocation().col;
}

extern "C" size_t SourcePositionIndex([[maybe_unused]] es2panda_Context *context, es2panda_SourcePosition *position)
{
    return reinterpret_cast<lexer::SourcePosition *>(position)->index;
}

extern "C" size_t SourcePositionLine([[maybe_unused]] es2panda_Context *context, es2panda_SourcePosition *position)
{
    return reinterpret_cast<lexer::SourcePosition *>(position)->line;
}

extern "C" es2panda_SourcePosition *SourceRangeStart([[maybe_unused]] es2panda_Context *context,
                                                     es2panda_SourceRange *range)
{
    auto *allocator = reinterpret_cast<Context *>(context)->allocator;
    auto E2pRange = reinterpret_cast<lexer::SourceRange *>(range);
    return reinterpret_cast<es2panda_SourcePosition *>(allocator->New<lexer::SourcePosition>(E2pRange->start));
}

extern "C" es2panda_SourcePosition *SourceRangeEnd([[maybe_unused]] es2panda_Context *context,
                                                   es2panda_SourceRange *range)
{
    auto *allocator = reinterpret_cast<Context *>(context)->allocator;
    auto E2pRange = reinterpret_cast<lexer::SourceRange *>(range);
    return reinterpret_cast<es2panda_SourcePosition *>(allocator->New<lexer::SourcePosition>(E2pRange->end));
}

extern "C" es2panda_Scope *AstNodeFindNearestScope([[maybe_unused]] es2panda_Context *ctx, es2panda_AstNode *node)
{
    auto E2pNode = reinterpret_cast<ir::AstNode *>(node);
    return reinterpret_cast<es2panda_Scope *>(compiler::NearestScope(E2pNode));
}

extern "C" es2panda_Scope *AstNodeRebind(es2panda_Context *ctx, es2panda_AstNode *node)
{
    auto E2pNode = reinterpret_cast<ir::AstNode *>(node);
    auto context = reinterpret_cast<Context *>(ctx);
    auto varbinder = context->parserProgram->VarBinder()->AsETSBinder();
    auto phaseManager = context->phaseManager;
    if (E2pNode->IsScriptFunction() ||
        E2pNode->FindChild([](ir::AstNode *n) { return n->IsScriptFunction(); }) != nullptr) {
        while (!E2pNode->IsProgram()) {
            E2pNode = E2pNode->Parent();
        }
    }
    return reinterpret_cast<es2panda_Scope *>(compiler::Rebind(phaseManager, varbinder, E2pNode));
}

extern "C" void AstNodeRecheck(es2panda_Context *ctx, es2panda_AstNode *node)
{
    auto E2pNode = reinterpret_cast<ir::AstNode *>(node);
    auto context = reinterpret_cast<Context *>(ctx);
    auto varbinder = context->parserProgram->VarBinder()->AsETSBinder();
    auto checker = context->GetChecker()->AsETSChecker();
    auto phaseManager = context->phaseManager;
    if (E2pNode->IsScriptFunction() ||
        E2pNode->FindChild([](ir::AstNode *n) { return n->IsScriptFunction(); }) != nullptr) {
        while (!E2pNode->IsProgram()) {
            E2pNode = E2pNode->Parent();
        }
    }
    compiler::Recheck(phaseManager, varbinder, checker, E2pNode);
    context->state = !context->diagnosticEngine->IsAnyError() ? ES2PANDA_STATE_CHECKED : ES2PANDA_STATE_ERROR;
    return;
}

#include "generated/es2panda_lib/es2panda_lib_impl.inc"

extern "C" Es2pandaEnum Es2pandaEnumFromString([[maybe_unused]] es2panda_Context *ctx, const char *str)
{
    return IrToE2pEnum(es2panda::util::gen::ast_verifier::FromString(str));
}

extern "C" char *Es2pandaEnumToString(es2panda_Context *ctx, Es2pandaEnum id)
{
    auto *allocator = reinterpret_cast<Context *>(ctx)->allocator;
    return StringViewToCString(allocator, es2panda::util::gen::ast_verifier::ToString(E2pToIrEnum(id)));
}

extern "C" es2panda_AstNode *DeclarationFromIdentifier([[maybe_unused]] es2panda_Context *ctx, es2panda_AstNode *node)
{
    auto E2pNode = reinterpret_cast<ir::Identifier *>(node);
    return reinterpret_cast<es2panda_AstNode *>(compiler::DeclarationFromIdentifier(E2pNode));
}

extern "C" bool IsImportTypeKind([[maybe_unused]] es2panda_Context *context, es2panda_AstNode *node)
{
    auto E2pNode = reinterpret_cast<const ir::AstNode *>(node);
    if (E2pNode->IsETSImportDeclaration()) {
        return E2pNode->AsETSImportDeclaration()->IsTypeKind();
    }

    if (E2pNode->IsImportDeclaration()) {
        return E2pNode->AsETSImportDeclaration()->IsTypeKind();
    }

    auto ctx = reinterpret_cast<Context *>(context);
    auto id = ctx->config->diagnosticKindStorage.size() + 1;
    auto type = util::DiagnosticType::PLUGIN_WARNING;
    util::DiagnosticMessageParams params {};
    diagnostic::DiagnosticKind *kind = &ctx->config->diagnosticKindStorage.emplace_back(type, id, "Insert wrong node!");
    ctx->diagnosticEngine->LogDiagnostic(*kind, params, E2pNode->Start());
    return false;
}

extern "C" char *GetLicenseFromRootNode(es2panda_Context *ctx, es2panda_AstNode *node)
{
    auto E2pNode = reinterpret_cast<const ir::AstNode *>(node);
    auto *allocator = reinterpret_cast<Context *>(ctx)->allocator;
    return StringViewToCString(allocator, compiler::GetLicenseFromRootNode(E2pNode));
}

extern "C" char *JsdocStringFromDeclaration([[maybe_unused]] es2panda_Context *ctx, es2panda_AstNode *node)
{
    auto E2pNode = reinterpret_cast<const ir::AstNode *>(node);
    auto *allocator = reinterpret_cast<Context *>(ctx)->allocator;
    return StringViewToCString(allocator, compiler::JsdocStringFromDeclaration(E2pNode));
}

extern "C" es2panda_AstNode *FirstDeclarationByNameFromNode([[maybe_unused]] es2panda_Context *ctx,
                                                            const es2panda_AstNode *node, const char *name)
{
    if (node == nullptr) {
        return nullptr;
    }

    util::StringView nameE2p {name};
    ir::AstNode *res = reinterpret_cast<const ir::AstNode *>(node)->FindChild([&nameE2p](const ir::AstNode *ast) {
        if (ast != nullptr && ast->IsMethodDefinition() && ast->AsMethodDefinition()->Key() != nullptr &&
            ast->AsMethodDefinition()->Key()->IsIdentifier() &&
            ast->AsMethodDefinition()->Key()->AsIdentifier()->Name() == nameE2p) {
            return true;
        }

        return false;
    });

    return reinterpret_cast<es2panda_AstNode *>(res);
}

extern "C" es2panda_AstNode *FirstDeclarationByNameFromProgram([[maybe_unused]] es2panda_Context *ctx,
                                                               const es2panda_Program *program, const char *name)
{
    if (program == nullptr) {
        return nullptr;
    }

    auto programE2p = reinterpret_cast<const parser::Program *>(program);
    es2panda_AstNode *res =
        FirstDeclarationByNameFromNode(ctx, reinterpret_cast<const es2panda_AstNode *>(programE2p->Ast()), name);
    if (res != nullptr) {
        return res;
    }

    for (const auto &ext_source : programE2p->DirectExternalSources()) {
        for (const auto *ext_program : ext_source.second) {
            if (ext_program != nullptr) {
                res = FirstDeclarationByNameFromNode(
                    ctx, reinterpret_cast<const es2panda_AstNode *>(ext_program->Ast()), name);
            }
            if (res != nullptr) {
                return res;
            }
        }
    }

    return nullptr;
}

static ArenaSet<ir::AstNode *> AllDeclarationsByNameFromNodeHelper(ArenaAllocator *const allocator,
                                                                   const ir::AstNode *node,
                                                                   const util::StringView &name)
{
    auto result = ArenaSet<ir::AstNode *> {allocator->Adapter()};

    if (node == nullptr) {
        return result;
    }

    node->IterateRecursively([&result, &name](ir::AstNode *ast) {
        if (ast != nullptr && ast->IsMethodDefinition() && ast->AsMethodDefinition()->Key() != nullptr &&
            ast->AsMethodDefinition()->Key()->IsIdentifier() &&
            ast->AsMethodDefinition()->Key()->AsIdentifier()->Name() == name) {
            result.insert(ast);
        }
    });

    return result;
}

extern "C" es2panda_AstNode **AllDeclarationsByNameFromNode([[maybe_unused]] es2panda_Context *ctx,
                                                            const es2panda_AstNode *node, const char *name,
                                                            size_t *declsLen)
{
    util::StringView nameE2p {name};
    auto nodeE2p = reinterpret_cast<const ir::AstNode *>(node);
    auto allocator = reinterpret_cast<Context *>(ctx)->allocator;
    auto result = AllDeclarationsByNameFromNodeHelper(allocator, nodeE2p, nameE2p);
    *declsLen = result.size();
    auto apiRes = allocator->New<es2panda_AstNode *[]>(*declsLen);
    size_t i = 0;
    for (auto elem : result) {
        auto toPush = reinterpret_cast<es2panda_AstNode *>(elem);
        apiRes[i++] = toPush;
    };
    return apiRes;
}

extern "C" es2panda_AstNode **AllDeclarationsByNameFromProgram([[maybe_unused]] es2panda_Context *ctx,
                                                               const es2panda_Program *program, const char *name,
                                                               size_t *declsLen)
{
    auto allocator = reinterpret_cast<Context *>(ctx)->allocator;
    if (program == nullptr) {
        *declsLen = 0;
        return allocator->New<es2panda_AstNode *[]>(0);
    }

    util::StringView nameE2p {name};
    auto programE2p = reinterpret_cast<const parser::Program *>(program);
    auto result = ArenaSet<ir::AstNode *> {allocator->Adapter()};

    ArenaSet<ir::AstNode *> res = AllDeclarationsByNameFromNodeHelper(allocator, programE2p->Ast(), nameE2p);
    result.insert(res.begin(), res.end());

    for (const auto &ext_source : programE2p->DirectExternalSources()) {
        for (const auto *ext_program : ext_source.second) {
            if (ext_program != nullptr) {
                res = AllDeclarationsByNameFromNodeHelper(allocator, ext_program->Ast(), nameE2p);
                result.insert(res.begin(), res.end());
            }
        }
    }

    *declsLen = result.size();
    auto apiRes = allocator->New<es2panda_AstNode *[]>(*declsLen);
    size_t i = 0;
    for (auto elem : result) {
        auto toPush = reinterpret_cast<es2panda_AstNode *>(elem);
        apiRes[i++] = toPush;
    };

    return apiRes;
}

extern "C" __attribute__((unused)) int GenerateTsDeclarationsFromContext(es2panda_Context *ctx,
                                                                         const char *outputDeclEts,
                                                                         const char *outputEts, bool exportAll,
                                                                         bool isolated, const char *recordFile)
{
    auto *ctxImpl = reinterpret_cast<Context *>(ctx);
    auto *checker = reinterpret_cast<ark::es2panda::checker::ETSChecker *>(ctxImpl->GetChecker());

    ark::es2panda::declgen_ets2ts::DeclgenOptions declgenOptions;
    declgenOptions.exportAll = exportAll;
    declgenOptions.outputDeclEts = outputDeclEts ? outputDeclEts : "";
    declgenOptions.outputEts = outputEts ? outputEts : "";
    declgenOptions.isolated = isolated;
    declgenOptions.recordFile = recordFile ? recordFile : "";

    return ark::es2panda::declgen_ets2ts::GenerateTsDeclarations(checker, ctxImpl->parserProgram, declgenOptions) ? 0
                                                                                                                  : 1;
}

// #28937 Will be removed after binary import support is fully implemented.
__attribute__((unused)) static std::string GetFileNameByPath(const std::string &path)
{
    auto lastSlash = path.find_last_of("/\\");
    std::string filename = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);

    auto lastDot = filename.find_last_of('.');
    if (lastDot != std::string::npos) {
        filename = filename.substr(0, lastDot);
    }

    lastDot = filename.find_last_of('.');
    if (lastDot != std::string::npos) {
        filename = filename.substr(0, lastDot);
    }

    return filename;
}

// #28937 Will be removed after binary import support is fully implemented.
__attribute__((unused)) static bool HandleMultiFileMode(Context *ctxImpl, const std::string &outputPath)
{
    std::string outputStem = GetFileNameByPath(outputPath);
    auto &externalSources = ctxImpl->parserProgram->DirectExternalSources();

    for (const auto &entry : externalSources) {
        for (auto *prog : entry.second) {
            if (prog == nullptr || !prog->IsGenAbcForExternal()) {
                continue;
            }

            if (prog->FileName().Mutf8() == outputStem) {
                compiler::HandleGenerateDecl(*prog, *ctxImpl->diagnosticEngine, outputPath);
                return !ctxImpl->diagnosticEngine->IsAnyError();
            }
        }
    }

    return false;
}

// #28937 Will be removed after binary import support is fully implemented.
extern "C" __attribute__((unused)) int GenerateStaticDeclarationsFromContext(es2panda_Context *ctx,
                                                                             const char *outputPath)
{
    auto *ctxImpl = reinterpret_cast<Context *>(ctx);
    if (ctxImpl->state != ES2PANDA_STATE_CHECKED) {
        return 1;
    }

    // Multiple file mode
    if (ctxImpl->config->options->IsSimultaneous()) {
        bool success = HandleMultiFileMode(ctxImpl, outputPath);
        return success ? 0 : 1;
    }

    // Single file mode
    compiler::HandleGenerateDecl(*ctxImpl->parserProgram, *ctxImpl->diagnosticEngine, outputPath);
    return ctxImpl->diagnosticEngine->IsAnyError() ? 1 : 0;
}

extern "C" void InsertETSImportDeclarationAndParse(es2panda_Context *context, es2panda_Program *program,
                                                   es2panda_AstNode *importDeclaration)
{
    auto *ctx = reinterpret_cast<Context *>(context);
    auto *parserProgram = reinterpret_cast<parser::Program *>(program);
    auto *importDeclE2p = reinterpret_cast<ir::ETSImportDeclaration *>(importDeclaration);
    importDeclE2p->AddAstNodeFlags(ir::AstNodeFlags::NOCLEANUP);

    auto &stmt = parserProgram->Ast()->StatementsForUpdates();
    bool hasUseStatic = !stmt.empty() && stmt.front()->IsExpressionStatement();
    if (hasUseStatic) {
        auto *expansion = stmt.front()->AsExpressionStatement()->GetExpression();
        hasUseStatic = hasUseStatic && expansion->IsStringLiteral() &&
                       expansion->AsStringLiteral()->Str() == compiler::Signatures::STATIC_PROGRAM_FLAG;
    }
    size_t insertIndex = hasUseStatic ? 1 : 0;

    stmt.insert(stmt.begin() + insertIndex, importDeclE2p);
    importDeclE2p->SetParent(parserProgram->Ast());

    ctx->parser->AsETSParser()->AddExternalSource(ctx->parser->AsETSParser()->ParseSources());

    for ([[maybe_unused]] auto *specific : importDeclE2p->Specifiers()) {
        ES2PANDA_ASSERT(specific->Parent() != nullptr);
    }
}

__attribute__((unused)) static void GenerateStdLibCache(es2panda_Config *config, GlobalContext *globalContext,
                                                        bool LspUsage)
{
    auto cfg = reinterpret_cast<ConfigImpl *>(config);
    globalContext->stdLibAllocator = new ThreadSafeArenaAllocator(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);
    auto ctx = CreateContext(config, std::move(""), cfg->options->SourceFileName().c_str(),
                             reinterpret_cast<es2panda_GlobalContext *>(globalContext), true, true);
    ProceedToState(ctx, es2panda_ContextState::ES2PANDA_STATE_CHECKED);
    if (!LspUsage) {
        AstNodeRecheck(ctx,
                       reinterpret_cast<es2panda_AstNode *>(reinterpret_cast<Context *>(ctx)->parserProgram->Ast()));
        AstNodeRecheck(ctx,
                       reinterpret_cast<es2panda_AstNode *>(reinterpret_cast<Context *>(ctx)->parserProgram->Ast()));
    }
    ProceedToState(ctx, es2panda_ContextState::ES2PANDA_STATE_LOWERED);
    globalContext->stdLibAstCache = &(reinterpret_cast<Context *>(ctx)->parserProgram->ExternalSources());
    DestroyContext(ctx);
}

es2panda_Impl g_impl = {
    ES2PANDA_LIB_VERSION,

    MemInitialize,
    MemFinalize,
    CreateConfig,
    DestroyConfig,
    GetAllErrorMessages,
    ConfigGetOptions,
    CreateContextFromFile,
    CreateCacheContextFromFile,
    CreateContextFromString,
    CreateContextFromStringWithHistory,
    CreateCacheContextFromString,
    CreateContextGenerateAbcForExternalSourceFiles,
    ProceedToState,
    DestroyContext,
    CreateGlobalContext,
    DestroyGlobalContext,
    ContextState,
    ContextErrorMessage,
    ContextProgram,
    ExternalSourceName,
    ExternalSourcePrograms,
    AstNodeForEach,
    NumberLiteralSetInt,
    NumberLiteralSetLong,
    NumberLiteralSetDouble,
    NumberLiteralSetFloat,
    CreateNumberLiteral<int32_t>,
    UpdateNumberLiteral<int32_t>,
    CreateNumberLiteral<int64_t>,
    UpdateNumberLiteral<int64_t>,
    CreateNumberLiteral<double>,
    UpdateNumberLiteral<double>,
    CreateNumberLiteral<float>,
    UpdateNumberLiteral<float>,
    NumberLiteralStrConst,
    AllocMemory,
    CreateSourcePosition,
    CreateSourceRange,
    SourcePositionCol,
    SourcePositionIndex,
    SourcePositionLine,
    SourceRangeStart,
    SourceRangeEnd,
    CreateDiagnosticKind,
    CreateDiagnosticInfo,
    CreateSuggestionInfo,
    LogDiagnosticWithSuggestion,
    LogDiagnostic,
    GetSemanticErrors,
    GetSyntaxErrors,
    GetPluginErrors,
    GetWarnings,
    IsAnyError,
    AstNodeFindNearestScope,
    AstNodeRebind,
    AstNodeRecheck,
    Es2pandaEnumFromString,
    Es2pandaEnumToString,
    DeclarationFromIdentifier,
    IsImportTypeKind,
    JsdocStringFromDeclaration,
    GetLicenseFromRootNode,
    FirstDeclarationByNameFromNode,
    FirstDeclarationByNameFromProgram,
    AllDeclarationsByNameFromNode,
    AllDeclarationsByNameFromProgram,
    GenerateTsDeclarationsFromContext,
    InsertETSImportDeclarationAndParse,
    GenerateStaticDeclarationsFromContext,
    InvalidateFileCache,
    RemoveFileCache,
    AddFileCache,

#include "generated/es2panda_lib/es2panda_lib_list.inc"

};

}  // namespace ark::es2panda::public_lib

extern "C" es2panda_Impl const *es2panda_GetImpl(int version)
{
    if (version != ES2PANDA_LIB_VERSION) {
        return nullptr;
    }
    return &ark::es2panda::public_lib::g_impl;
}

// NOLINTEND
