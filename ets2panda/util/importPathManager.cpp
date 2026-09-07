/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "importPathManager.h"
#include "es2panda.h"
#include <libarkbase/os/filesystem.h>
#include "util/arktsconfig.h"
#include "util/diagnostic.h"
#include "util/fsQueryCache.h"
#include "util/diagnosticEngine.h"
#include "generated/diagnostic.h"

#include "parser/ETSparser.h"
#include "parser/program/program.h"
#include "ir/expressions/literals/stringLiteral.h"

#include "compiler/lowering/ets/declGenPhase.h"

#include "libarkfile/class_data_accessor-inl.h"
#include "libarkfile/file-inl.h"
#include "libarkbase/utils/logger.h"
#include "schemaMetadataGenerated.h"

#include "util/es2pandaMacros.h"
#include "util/language.h"
#include "util/path.h"
#include "varbinder/ETSBinder.h"
#include "varbinder/TSBinder.h"
#include "varbinder/ASBinder.h"
#include "varbinder/JSBinder.h"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <queue>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef PANDA_TARGET_WINDOWS
#include <io.h>
#endif

#ifdef USE_UNIX_SYSCALL
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#else
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <semaphore.h>
#endif

namespace ark::es2panda::parser {

template <util::ModuleKind KIND, typename VarBinderT>
ProgramAdapter<KIND> *Program::New(const util::ImportInfo &importInfo, public_lib::Context *context)
{
    varbinder::VarBinder *actualVB = nullptr;
    if constexpr (!std::is_same_v<VarBinderT, void>) {
        ES2PANDA_ASSERT(context->parserProgram == nullptr);
        actualVB = context->Allocator()->New<VarBinderT>(context);
    } else {
        actualVB = context->parserProgram->VarBinder();
    }
    ES2PANDA_ASSERT(actualVB != nullptr);
    return context->Allocator()->New<ProgramAdapter<KIND>>(importInfo, context->Allocator(), actualVB);
}
}  // namespace ark::es2panda::parser

namespace ark::es2panda::util {

namespace {

bool ShouldUseMetadata(const public_lib::Context *ctx)
{
    return ctx->config->options->IsReadMetadata() && !ctx->config->options->IsGenStdlib();
}

bool VerifyMetadataModules(const panda_file::MetadataByModules &metadata)
{
    for (const auto &[moduleName, moduleMetadata] : metadata) {
        if (moduleMetadata.empty()) {
            continue;
        }

        flatbuffers::Verifier verifier(moduleMetadata.data(), moduleMetadata.size());
        if (!Metadata::VerifyDeclsBuffer(verifier)) {
            LOG(WARNING, ES2PANDA) << "Rejected malformed metadata blob for module '" << moduleName << "'";
            return false;
        }
    }

    return true;
}

}  // namespace

size_t HandleSpecialSymbols(const std::string &input, std::string &output, const size_t &i)
{
    switch (input[i + 1]) {
        case 'n':
            output.push_back('\n');
            return 1;
        case '?':
            output.push_back('?');
            return 1;
        case '\'': {
            std::string_view pattern = "\\'use static\\'";
            if (input.compare(i, pattern.size(), pattern) == 0) {
                output += "'use static'";
                return (pattern.size() - 1);
            }
            output.push_back('\'');
            return 0;
        }
        default:
            output.push_back(input[i]);
            return 0;
    }
}

static std::string DeleteEscapeSymbols(const std::string &input)
{
    std::string output;
    output.reserve(input.size());
    size_t skip = 0;
    for (size_t i = 0; i < input.size(); ++i) {
        if (skip > 0) {
            --skip;
            continue;
        }
        if (input[i] == '\\' && i + 1 < input.size()) {
            size_t consumed = HandleSpecialSymbols(input, output, i);
            skip = consumed;
        } else {
            output.push_back(input[i]);
        }
    }
    return output;
}

static std::optional<std::string> TryExtractMnameFromEtsGlobal(const panda_file::File &pf,
                                                               const panda_file::File::EntityId &classId)
{
    // processing name to get ohmUrl
    const std::string name = utf::Mutf8AsCString(pf.GetStringData(classId).data);
    const auto descriptorType = pandasm::Type::FromDescriptor(name);
    const auto type = pandasm::Type(descriptorType.GetNameWithoutRank(), descriptorType.GetRank());
    const auto recordName = type.GetPandasmName();

    // rely on the following mangling: <moduleName>.ETSGLOBAL
    const auto etsGlobalSuffix = std::string(".") + std::string(compiler::Signatures::ETS_GLOBAL);
    if (!Helpers::EndsWith(recordName, etsGlobalSuffix)) {
        return std::nullopt;
    }
    return recordName.substr(0, recordName.size() - etsGlobalSuffix.size());
}

const ArkTsConfig &ImportPathManager::ArkTSConfig() const
{
    return ctx_.config->options->ArkTSConfig();
}

DiagnosticEngine *ImportPathManager::DE() const
{
    return ctx_.diagnosticEngine;
}

parser::Program *ImportPathManager::GetGlobalProgram() const
{
    return ctx_.parserProgram;
}

parser::Program *ImportPathManager::GatherImportInfo(parser::Program *importer, ir::StringLiteral *importPath)
{
    srcPos_ = importPath->Start();
    isDynamic_ = importer->IsDeclForDynamicStaticInterop();

    auto importInfo = ResolvePath(importer, *importPath);
    if (importInfo.ResolvedSource().empty() || !importInfo.IsValid()) {
        ES2PANDA_ASSERT(DE()->IsAnyError());
        return nullptr;
    }

    AddFileDependencies(importer->AbsoluteName().Utf8(), importInfo.ResolvedSource());
    AddOutputMatching(importInfo.ResolvedSource(), FormAbcFilePath(importInfo));
    LOG(DEBUG, ES2PANDA) << "[" << importer->ModuleInfo().moduleName << "] "
                         << "Import " << importPath->ToString() << " resolved to " << importInfo.ResolvedSource();
    auto *importedProgram = LookupImportDataAndIntroduceProgram(&importInfo);
    if ((importedProgram == importer) && !importer->IsStdLib() && !importer->Is<ModuleKind::METADATA_DECL>()) {
        DE()->LogDiagnostic(diagnostic::IMPORT_ITSELF, util::DiagnosticMessageParams {importInfo.ResolvedSource()},
                            srcPos_);
    }
    return importedProgram;
}

static bool IsRelativePath(std::string_view path)
{
    for (std::string_view start : {"./", "../", ".\\", "..\\"}) {
        if (Helpers::StartsWith(path, start)) {
            return true;
        }
    }
    return false;
}

static std::string NormalizePathPrototype(std::string resolvedPathPrototype, char pathDelimiter)
{
    std::replace_if(
        resolvedPathPrototype.begin(), resolvedPathPrototype.end(),
        [pathDelimiter](char c) { return ((pathDelimiter != c) && ((c == '\\') || (c == '/'))); }, pathDelimiter);
    return ark::os::NormalizePath(resolvedPathPrototype);
}

static std::string NormalizeArktsconfigLookupPath(std::string path)
{
    std::replace(path.begin(), path.end(), '\\', '/');
    return ark::os::NormalizePath(path);
}

static bool IsSubPathOrSame(std::string_view path, std::string_view base)
{
    if (!Helpers::StartsWith(path, base)) {
        return false;
    }
    return path.size() == base.size() || (!base.empty() && base.back() == '/') || path.at(base.size()) == '/';
}

static std::optional<std::string> FormCacheRelativeModulePath(std::string targetPath, std::string cacheDir)
{
    targetPath = NormalizeArktsconfigLookupPath(std::move(targetPath));
    cacheDir = NormalizeArktsconfigLookupPath(std::move(cacheDir));
    if (cacheDir.empty() || !IsSubPathOrSame(targetPath, cacheDir)) {
        return std::nullopt;
    }

    auto relativePath = targetPath.substr(cacheDir.size());
    if (!relativePath.empty() && relativePath.front() == '/') {
        relativePath.erase(relativePath.begin());
    }
    if (relativePath.empty()) {
        return std::nullopt;
    }
    return relativePath;
}

util::StringView ImportPathManager::ResolvePathAPI(parser::Program *importer, ir::StringLiteral *importPath) const
{
    srcPos_ = importPath->Start();
    // NOTE(dkofanov): #23698 related. In case of 'dynamicPaths', resolved path is "virtual" module-path, may be not
    // what the plugin expecting.
    // NOTE(itrubachev) import path manager should be refactored
    auto importInfo = ResolvePath(importer, *importPath);
    auto resolvedPath = UString(importInfo.ResolvedSource(), Context()->Allocator());
    return resolvedPath.View();
}

void ImportPathManager::TryMatchStaticResolvedPath(ImportPathManager::ResolvedPathRes *result) const
{
    auto paths = ArkTSConfig().Paths().find(result->resolvedPath);
    if (paths != ArkTSConfig().Paths().cend()) {
        result->resolvedPath = *paths->second.begin();
        result->resolvedIsExternalModule = false;
    }
}

static bool ImportFileExists(FsQueryCache &fsQueryCache, const std::string &path)
{
#if defined(PANDA_TARGET_WINDOWS)
    return fsQueryCache.IsRegularFile(path, true);
#else
    return fsQueryCache.IsRegularFile(path);
#endif
}

bool ImportPathManager::CheckDependencyFileExists(const std::string &depPath, std::string_view messageParam) const
{
    // A dynamic dependency without a path is valid for ESValue/declless interop scenarios
    // (the declaration file is not needed); skip the existence check in that case.
    if (depPath.empty()) {
        return true;
    }
    const bool fileExists = ImportFileExists(*fsQueryCache_, depPath);
    if (!fileExists) {
        DE()->LogDiagnostic(diagnostic::INTEROP_DYNAMIC_FILE_NOT_FOUND, util::DiagnosticMessageParams {messageParam},
                            srcPos_);
        return false;
    }
    return true;
}

bool ImportPathManager::IsDepAnalyzerMode() const
{
    return ctx_.depAnalyzer != nullptr;
}

void ImportPathManager::TryMatchDynamicResolvedPath(ImportPathManager::ResolvedPathRes *result,
                                                    std::string_view importPath) const
{
    auto packagePathPair = ArkTSConfig().SourcePathMap().find(result->resolvedPath);
    if (packagePathPair != ArkTSConfig().SourcePathMap().cend()) {
        result->resolvedPath = packagePathPair->second;
        result->resolvedIsExternalModule = true;
        const auto &dependencies = ArkTSConfig().Dependencies();
        if (auto depIt = dependencies.find(result->resolvedPath);
            depIt != dependencies.cend() && !IsDepAnalyzerMode()) {
            if (!CheckDependencyFileExists(depIt->second.Path(), importPath)) {
                result->resolvedPath.clear();
                result->resolvedIsExternalModule = false;
                result->hasError = true;
            }
        }
        return;
    }
    auto paths = ArkTSConfig().Paths().find(result->resolvedPath);
    if (paths != ArkTSConfig().Paths().cend()) {
        result->resolvedPath = *paths->second.begin();
        result->resolvedIsExternalModule = false;
    }
}

std::optional<std::string> ImportPathManager::ResolveMockPath(
    std::string_view curFile, std::string_view importFile, const std::map<std::string, std::string, std::less<>> &mocks,
    const std::set<std::string, std::less<>> &mockSources) const
{
    // check the current file is in mock files.
    if (mockSources.find(curFile) != mockSources.end()) {
        return std::nullopt;
    }
    // check import file is in mock files.
    auto it = mocks.find(importFile);
    if (it != mocks.end()) {
        return it->second;
    }
    return std::nullopt;
}

ImportPathManager::ResolvedPathRes ImportPathManager::ResolveEtscacheRelativePath(
    std::string physicalPathPrototype) const
{
    physicalPathPrototype = NormalizePathPrototype(std::move(physicalPathPrototype), pathDelimiter_.at(0));
    auto physicalResult = ProbeExtensionOrIndexFile(physicalPathPrototype);
    if (!physicalResult.resolvedPath.empty() || physicalResult.hasError) {
        return physicalResult;
    }

    auto relativeModulePath = FormCacheRelativeModulePath(physicalPathPrototype, ArkTSConfig().CacheDir());
    if (!relativeModulePath.has_value()) {
        DE()->LogDiagnostic(diagnostic::UNSUPPORTED_PATH, util::DiagnosticMessageParams {physicalPathPrototype},
                            srcPos_);
        return {"", false, true};
    }

    // The cache-relative module path is a key in arktsconfig paths/dependencies.
    // Dynamic declarations must be located through that mapping, not the HAR declaration output directory.
    auto resolvedPath = ArkTSConfig().ResolvePath(*relativeModulePath, false, fsQueryCache_.get());
    if (resolvedPath) {
        auto result = ProbeExtensionOrIndexFile(std::move(*resolvedPath));
        if (!result.resolvedPath.empty() || result.hasError) {
            return result;
        }
    }

    DE()->LogDiagnostic(diagnostic::UNSUPPORTED_PATH, util::DiagnosticMessageParams {physicalPathPrototype}, srcPos_);
    return {"", false, true};
}

ImportInfo ImportPathManager::ResolvePath(parser::Program *importer, std::string_view importPath) const
{
    if (importPath.empty()) {
        DE()->LogDiagnostic(diagnostic::EMPTY_IMPORT_PATH, util::DiagnosticMessageParams {});
        return {};
    }
    ResolvedPathRes result {};
    auto curModulePath = isDynamic_ ? importer->GetImportInfo().ResolvedSource() : importer->AbsoluteName().Utf8();

    const auto &mocks = ArkTSConfig().MockMap();
    const auto &mockSources = ArkTSConfig().MockSources();

    if (!mocks.empty()) {
        auto rawMockPath = ResolveMockPath(curModulePath, importPath, mocks, mockSources);
        if (rawMockPath.has_value()) {
            return {*this, std::string(*rawMockPath), ToLanguage(importer->Extension()).GetId(), false};
        }
    }

    if (IsRelativePath(importPath)) {
        size_t pos = curModulePath.find_last_of("/\\");
        auto currentDir = (pos != std::string::npos) ? curModulePath.substr(0, pos) : curModulePath;
        std::string resolvedPathPrototype {currentDir};
        resolvedPathPrototype += pathDelimiter_;
        resolvedPathPrototype += importPath;
        result = importer->Is<ModuleKind::ETSCACHE_DECL>()
                     ? ResolveEtscacheRelativePath(std::move(resolvedPathPrototype))
                     : AppendExtensionOrIndexFileIfOmitted(std::move(resolvedPathPrototype));
        if (result.hasError) {
            return {};
        }
        if (result.resolvedIsExternalModule) {
            TryMatchStaticResolvedPath(&result);
        } else {
            TryMatchDynamicResolvedPath(&result, importPath);
        }
        if (result.hasError) {
            return {};
        }
    } else {
        result = ResolveAbsolutePath(importPath);
    }

    if (result.hasError) {
        return {};
    }

    if (!mocks.empty()) {
        auto resolvedMockPath = ResolveMockPath(curModulePath, result.resolvedPath, mocks, mockSources);
        if (resolvedMockPath.has_value()) {
            return {*this, std::string(*resolvedMockPath), ToLanguage(importer->Extension()).GetId(), false};
        }
    }

    return {*this, std::string(result.resolvedPath), ToLanguage(importer->Extension()).GetId(),
            result.resolvedIsExternalModule};
}

ImportPathManager::ResolvedPathRes ImportPathManager::ResolveAbsolutePath(std::string_view importPath) const
{
    ES2PANDA_ASSERT(!IsRelativePath(importPath));

    if (importPath.at(0) == pathDelimiter_.at(0)) {
        std::string resolvedPathPrototype = ArkTSConfig().BaseUrl();
        resolvedPathPrototype += importPath;
        return AppendExtensionOrIndexFileIfOmitted(resolvedPathPrototype);
    }

    const auto arkTsPathCacheKey = BuildResolutionCacheKey(importPath);
    auto arkTsPathCacheIter = arkTsPathCache_.find(arkTsPathCacheKey);
    std::optional<std::string> resolvedPath;
    if (arkTsPathCacheIter != arkTsPathCache_.end()) {
        resolvedPath = arkTsPathCacheIter->second;
    } else {
        resolvedPath = ArkTSConfig().ResolvePath(importPath, isDynamic_, fsQueryCache_.get());
        arkTsPathCache_.emplace(arkTsPathCacheKey, resolvedPath);
    }
    if (!resolvedPath) {
        DE()->LogDiagnostic(
            diagnostic::IMPORT_CANT_FIND_PREFIX,
            util::DiagnosticMessageParams {util::StringView(importPath), util::StringView(ArkTSConfig().ConfigPath())},
            srcPos_);
        return {""};
    }
    return AppendExtensionOrIndexFileIfOmitted(resolvedPath.value());
}

parser::PackageProgram *ImportPathManager::NewEmptyPackage(const ImportInfo &importInfo)
{
    auto allocator = Context()->allocator;
    auto package = parser::Program::New<ModuleKind::PACKAGE>(importInfo, Context());
    package->SetPackageInfo(importInfo.ModuleName(), util::ModuleKind::PACKAGE);

    auto ident = allocator->New<ir::Identifier>(compiler::Signatures::ETS_GLOBAL, allocator);
    ArenaVector<ir::Statement *> stmts(allocator->Adapter());
    auto etsModule = allocator->New<ir::ETSModule>(allocator, std::move(stmts), ident, ir::ModuleFlag::ETSSCRIPT,
                                                   Language::Id::ETS, package);
    package->SetAst(etsModule);
    return package;
}

template <typename VarBinderT, Language::Id LANG_ID>
void ImportPathManager::SetupGlobalProgram(public_lib::Context *ctx)
{
    ES2PANDA_ASSERT(Context()->config->options->GetCompilationMode() != CompilationMode::GEN_STD_LIB);
    // NOTE(dkofanov): this code tries to handle pseudo-files provided by unrelated 'ctx->sourceFile->filePath' and
    // 'ctx->input'.

    auto normalizedPathForGlobalProg = ark::os::GetAbsolutePath(std::string(ctx->sourceFile->filePath));
    if (normalizedPathForGlobalProg.empty()) {
        normalizedPathForGlobalProg = ark::os::NormalizePath(std::string(ctx->sourceFile->filePath));
    }

    if constexpr (LANG_ID == Language::Id::ETS) {
        util::ImportInfo importInfo {*this, normalizedPathForGlobalProg};
        importInfo.SetData<ModuleKind::MODULE>(normalizedPathForGlobalProg, std::string(ctx->input));
        ctx->parserProgram = IntroduceProgram<ModuleKind::MODULE, VarBinderT>(importInfo);
        AddOutputMatching(normalizedPathForGlobalProg, FormAbcFilePath(ctx->parserProgram->GetImportInfo()));
    } else {
        util::ImportInfo importInfo {};
        importInfo.moduleName_ = normalizedPathForGlobalProg;
        importInfo.lang_ = LANG_ID;
        importInfo.SetData<ModuleKind::MODULE>(normalizedPathForGlobalProg, std::string(ctx->input));
        ctx->parserProgram = IntroduceProgram<ModuleKind::MODULE, VarBinderT>(importInfo);
    }
    // NOTE(vpukhov): the *unnamed* modules are to be removed entirely
    if (Context()->config->options->IsEtsUnnamed()) {
        ctx->parserProgram->SetPackageInfo("", util::ModuleKind::MODULE);
    }
}

static constexpr auto STDLIB_MAIN_PROG_NAME = "etsstdlib.ets";
static constexpr auto STDLIB_IMPORTS_MAIN_PROG_NAME = "<default_import>.ets";
static constexpr auto SIMULT_MAIN_PROG_NAME = "<simult>";

void ImportPathManager::SetupGlobalProgram()
{
    if (Context()->config->options->GetCompilationMode() == CompilationMode::GEN_STD_LIB) {
        ES2PANDA_ASSERT(Context()->config->options->GetExtension() == ScriptExtension::ETS);
        util::ImportInfo importInfo {*this, STDLIB_MAIN_PROG_NAME};
        importInfo.SetData<ModuleKind::MODULE, false>(STDLIB_MAIN_PROG_NAME, "");
        Context()->parserProgram = IntroduceProgram<ModuleKind::MODULE, varbinder::ETSBinder>(importInfo);
        return;
    }
    switch (Context()->config->options->GetExtension()) {
        case ScriptExtension::TS:
            return SetupGlobalProgram<varbinder::TSBinder, Language::Id::TS>(Context());
        case ScriptExtension::AS:
            return SetupGlobalProgram<varbinder::ASBinder, Language::Id::AS>(Context());
        case ScriptExtension::ETS:
            return SetupGlobalProgram<varbinder::ETSBinder, Language::Id::ETS>(Context());
        case ScriptExtension::JS:
            return SetupGlobalProgram<varbinder::JSBinder, Language::Id::JS>(Context());
        default:
            ES2PANDA_UNREACHABLE();
    }
}

static ArenaString OhmurlToMname(ArenaString &&ohmurl)
{
    std::replace(ohmurl.begin(), ohmurl.end(), '\\', '/');
    ES2PANDA_ASSERT(std::find(ohmurl.begin(), ohmurl.end(), '\\') == ohmurl.end());
    if (ohmurl.at(0) == '/') {
        ohmurl.erase(0, 1);
    }
    ArenaString mnamePrototype {std::move(ohmurl)};
    size_t start = 0;
    if (!mnamePrototype.empty() && mnamePrototype[0] == '@') {
        size_t slashPos = mnamePrototype.find('/');
        if (slashPos != ArenaString::npos) {
            start = slashPos + 1;
        }
    }
    std::replace(mnamePrototype.begin() + start, mnamePrototype.end(), '/', '.');
    return mnamePrototype;
}

static ArenaString OhmurlToMname(std::string_view ohmurl)
{
    return OhmurlToMname(ArenaString {ohmurl});
}

parser::Program *ImportPathManager::SetupProgramForDebugInfoPlugin(std::string_view sourceFilePath,
                                                                   [[maybe_unused]] std::string_view moduleName)
{
    util::ImportInfo importInfo {
        *this,
        std::string(sourceFilePath),
    };
    importInfo.SetData<ModuleKind::MODULE, false>(std::string(sourceFilePath), "");
    // NOTE(dkofanov): The new program is added to 'ImportPathManager::resolvedSources_' during this call, so it can be
    // later resolved via 'SearchResolved'. This may be incorrect.
    auto *program = IntroduceProgram<ModuleKind::MODULE>(importInfo);

    program->SetSource({sourceFilePath, "", "", true, false});
    ES2PANDA_ASSERT(importInfo.ModuleName() == moduleName);

    auto allocator = Context()->Allocator();
    auto *emptyIdent = allocator->New<ir::Identifier>("", allocator);
    auto *etsModule = allocator->New<ir::ETSModule>(allocator, ArenaVector<ir::Statement *>(allocator->Adapter()),
                                                    emptyIdent, ir::ModuleFlag::ETSSCRIPT, importInfo.Lang(), program);
    program->SetAst(etsModule);
    Context()->parserProgram->GetExternalPrograms()->Add(program);
    return program;
}

parser::Program *ImportPathManager::IntroduceStdlibImportProgram(std::string &&contents)
{
    util::ImportInfo importInfo {*this, STDLIB_IMPORTS_MAIN_PROG_NAME};
    importInfo.SetData<ModuleKind::MODULE>(STDLIB_IMPORTS_MAIN_PROG_NAME, std::move(contents));
    if (auto *existing = SearchResolved(importInfo); existing != nullptr) {
        return existing;
    }
    return IntroduceProgram<ModuleKind::MODULE>(importInfo);
}

void ImportPathManager::ClearResolutionCaches()
{
    fsQueryCache_->Clear();
    appendExtensionOrIndexFileCache_.clear();
    arkTsPathCache_.clear();
}

void ImportPathManager::IntroduceMainProgramForSimult()
{
    ES2PANDA_ASSERT(Context()->parserProgram == nullptr);

    // NOTE(dkofanov): special empty programs for simult and stdlib should be removed.
    util::ImportInfo importInfo {*this, SIMULT_MAIN_PROG_NAME};
    importInfo.SetData<ModuleKind::SIMULT_MAIN, false>(SIMULT_MAIN_PROG_NAME, "");
    auto program = IntroduceProgram<ModuleKind::SIMULT_MAIN, varbinder::ETSBinder>(importInfo);

    auto allocator = Context()->allocator;
    auto ident = allocator->New<ir::Identifier>(compiler::Signatures::ETS_GLOBAL, allocator);
    ArenaVector<ir::Statement *> stmts(allocator->Adapter());
    auto etsModule = allocator->New<ir::ETSModule>(allocator, std::move(stmts), ident, ir::ModuleFlag::ETSSCRIPT,
                                                   Language::Id::ETS, program);
    program->SetAst(etsModule);
    Context()->parserProgram = program;
}

void ImportPathManager::PrepareParseQueueForProgram(parser::Program *program)
{
    ES2PANDA_ASSERT(program != nullptr);
    ClearParseList();
    RemoveFileDependencies(program->AbsoluteName().Utf8());
    ClearResolutionCaches();
    parseQueue_.emplace_back(ParseInfo {false, program});
    srcPos_.SetProgram(program);
}

std::unordered_set<std::string> ImportPathManager::CollectReverseDependencies(std::string_view root) const
{
    std::unordered_set<std::string> visited {};
    visited.reserve(reverseFileDependencies_.size());
    std::queue<std::string_view> queue {};
    queue.emplace(root);
    while (!queue.empty()) {
        auto current = queue.front();
        queue.pop();
        auto it = reverseFileDependencies_.find(ArenaString {current});
        if (it == reverseFileDependencies_.end()) {
            continue;
        }
        for (const auto &dependant : it->second) {
            auto dependantView = std::string_view {dependant.data(), dependant.size()};
            if (dependantView == root || !visited.emplace(dependantView).second) {
                continue;
            }
            queue.push(dependantView);
        }
    }
    return visited;
}

void ImportPathManager::RemoveFileDependencies(std::string_view file)
{
    auto fileKey = ArenaString {file};
    auto depsIt = fileDependencies_.find(fileKey);
    if (depsIt == fileDependencies_.end()) {
        return;
    }

    for (const auto &dep : depsIt->second) {
        RemoveReverseFileDependency(dep, fileKey);
    }
    fileDependencies_.erase(depsIt);
}

void ImportPathManager::RemoveReverseFileDependency(const ArenaString &dependency, const ArenaString &file)
{
    auto reverseIt = reverseFileDependencies_.find(dependency);
    if (reverseIt == reverseFileDependencies_.end()) {
        return;
    }

    reverseIt->second.erase(file);
    if (reverseIt->second.empty()) {
        reverseFileDependencies_.erase(reverseIt);
    }
}

void ImportPathManager::RemoveProgramsFromFileDependencies(const std::unordered_set<std::string> &files)
{
    if (files.empty()) {
        return;
    }

    std::unordered_set<std::string_view> deletedFiles {};
    deletedFiles.reserve(files.size());
    for (const auto &file : files) {
        if (file.empty()) {
            continue;
        }
        deletedFiles.insert(file);
        RemoveFileDependencies(file);
    }

    RemoveDependenciesToFiles(deletedFiles);
}

void ImportPathManager::RemoveDependenciesToFiles(const std::unordered_set<std::string_view> &files)
{
    for (auto &[file, deps] : fileDependencies_) {
        for (auto depIt = deps.begin(); depIt != deps.end();) {
            if (files.count(std::string_view {depIt->data(), depIt->size()}) == 0) {
                ++depIt;
                continue;
            }

            RemoveReverseFileDependency(*depIt, file);
            depIt = deps.erase(depIt);
        }
    }
}

static bool IsExtensionForPackageFraction(const std::string &extension)
{
    return extension == ImportPathManager::ETS_SUFFIX;
}

parser::PackageProgram *ImportPathManager::RegisterSourcesForPackageFromGlobbedDirectory(const ImportInfo &importInfo)
{
    ES2PANDA_ASSERT(importInfo.PointsToPackage());
    ES2PANDA_ASSERT(LookupProgramCaches(importInfo) == nullptr);

    auto *package = NewEmptyPackage(importInfo);

#ifdef USE_UNIX_SYSCALL
    UnixRegisterSourcesForPackageFromGlobbedDirectory(package, importInfo);
#else
    for (auto const &entry : fs::directory_iterator(std::string(importInfo.ResolvedSource()))) {
        if (!fs::is_regular_file(entry) || !IsExtensionForPackageFraction(entry.path().extension().string())) {
            continue;
        }

        ImportInfo globElemImportInfo {*this, entry.path().string(), Language::Id::ETS};
        RegisterPackageFraction(package, &globElemImportInfo);
    }
#endif

    return package;
}

#ifdef USE_UNIX_SYSCALL
void ImportPathManager::UnixRegisterSourcesForPackageFromGlobbedDirectory(parser::PackageProgram *package,
                                                                          const ImportInfo &importInfo)
{
    const auto directoryPath = std::string(importInfo.ResolvedSource());
    DIR *dir = opendir(directoryPath.c_str());
    if (dir == nullptr) {
        DE()->LogDiagnostic(diagnostic::OPEN_FOLDER_FAILED, util::DiagnosticMessageParams {directoryPath}, srcPos_);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        std::string fileName = entry->d_name;
        std::string::size_type pos = fileName.find_last_of('.');
        if (pos == std::string::npos || !IsExtensionForPackageFraction(fileName.substr(pos))) {
            continue;
        }

        std::string filePath = directoryPath + "/" + entry->d_name;
        ImportInfo globElemImportInfo {*this, filePath, Language::Id::ETS};
        RegisterPackageFraction(package, &globElemImportInfo);
    }

    closedir(dir);
    return;
}
#endif

void ImportInfo::LinkFractionInfoToPackage(const parser::PackageProgram &package)
{
    moduleName_ = package.ModuleName();
}

void ImportPathManager::RegisterPackageFraction(parser::PackageProgram *package, ImportInfo *importInfo)
{
    auto *fraction = SearchResolved(*importInfo);
    if (fraction == GetGlobalProgram()) {
        return;
    }
    // The fraction may be previously added via direct import of it (by real path). Since the program is parsed later
    // that step, it's impossible to decide, whether it's part of a package or just module until now. So here the
    // already registered fraction is being aligned to the others fractions.
    if (fraction == nullptr) {
        ES2PANDA_ASSERT(!importInfo->ResolvedPathIsVirtual());
        // This function shouldn't lookup program-cache or lowdecl-cache since package fractions are merged before any
        // cache is stored.
        importInfo->SetTextFile<ModuleKind::MODULE, false>(std::string(importInfo->ResolvedSource()), DE());
        fraction = IntroduceProgram<ModuleKind::MODULE>(*importInfo);
    } else {
        // remove a package-fraction that was mistakenly added as a module without enclosing package:
        auto &modules = GetGlobalProgram()->GetExternalPrograms()->Get<ModuleKind::MODULE>();
        auto newEndIt = std::remove(modules.begin(), modules.end(), fraction);
        modules.erase(newEndIt, modules.end());
    }

    const_cast<ImportInfo *>(&fraction->GetImportInfo())->LinkFractionInfoToPackage(*package);

    fraction->SetPackageInfo(package->ModuleName(), ModuleKind::PACKAGE);
    ES2PANDA_ASSERT(fraction->ModuleName().find(package->ModuleName()) == 0);
    package->AppendFraction(fraction->As<ModuleKind::MODULE>());
}

parser::Program *ImportPathManager::LookupProgramCaches(const ImportInfo &importInfo)
{
    if (Context()->globalContext == nullptr) {
        return nullptr;
    }
    parser::Program *cachedProg = nullptr;
    auto key = std::string(importInfo.Key());
    const auto &cachedExtProgs = Context()->globalContext->cachedExternalPrograms;
    if (cachedExtProgs.find(key) != cachedExtProgs.end()) {
        cachedProg = cachedExtProgs.at(key);
        ES2PANDA_ASSERT(key == cachedProg->GetImportInfo().Key());
    } else if (auto *cachedStdlib = Context()->globalContext->stdLibAstCache; cachedStdlib != nullptr) {
        cachedStdlib->Visit<false>([&cachedProg, &key](auto *prog) {
            if (prog->GetImportInfo().Key() == key) {
                cachedProg = prog;
            }
        });
    }
    if (cachedProg != nullptr) {
        RegisterProgram(cachedProg);
    }
    return cachedProg;
}

// NOTE(dkofanov): #32416 revise for packages caching, etc.
std::string ImportPathManager::FormEtscacheFilePath(std::string moduleName, const std::string &cacheDir)
{
    ES2PANDA_ASSERT(!cacheDir.empty());
    ES2PANDA_ASSERT(!moduleName.empty());
    std::replace(moduleName.begin(), moduleName.end(), '.', util::Path::GetPathDelimiter());
    return cacheDir + util::PATH_DELIMITER + moduleName + std::string {CACHE_SUFFIX};
}

std::string ImportPathManager::FormAbcFilePath([[maybe_unused]] const ImportInfo &imd) const
{
#if not defined PANDA_TARGET_MOBILE
    std::string outputDir;
    ES2PANDA_ASSERT(Context() != nullptr && Context()->config != nullptr);
    auto *opts = Context()->config->options;

    if (opts != nullptr && opts->WasSetOutput()) {
        if (!opts->IsIncremental()) {
            ES2PANDA_ASSERT(!ark::os::file::File::IsDirectory(opts->GetOutput()));
            return fs::absolute(opts->GetOutput()).string();
        }

        if (!ark::os::file::File::IsDirectory(opts->GetOutput())) {
            DE()->LogDiagnostic(diagnostic::SIMULTANEOUS_INCREMENTAL_OUTPUT, util::DiagnosticMessageParams {});
            return "";
        }

        outputDir = fs::absolute(opts->GetOutput()).string();
    } else if (Context()->emitter != nullptr && !Context()->emitter->IsETSEmitter()) {
        return fs::absolute(std::string {ImportInfo::DUMMY_PATH} + std::string {ImportPathManager::ABC_SUFFIX})
            .string();
    } else if (!opts->ArkTSConfig().CacheDir().empty()) {
        outputDir = fs::absolute(opts->ArkTSConfig().CacheDir()).string();
    } else {
        outputDir = fs::absolute("build").string();
    }

    std::string abcFile = outputDir;
    abcFile += util::PATH_DELIMITER;
    abcFile += imd.ModuleName();
    abcFile += ABC_SUFFIX;

    return abcFile;
#endif
    return "";
}

class EtscacheFileLock {
public:
    NO_MOVE_SEMANTIC(EtscacheFileLock);
    NO_COPY_SEMANTIC(EtscacheFileLock);
    EtscacheFileLock(std::string dstDeclPath, std::string srcAbcPath)
        : dstPath_(std::move(dstDeclPath)), abcPath_(std::move(srcAbcPath))
    {
        if ((!os::IsFileExists(dstPath_) || ShouldRewrite(abcPath_, dstPath_))) {
            writer_ = ExlusiveFileWriter::Open(dstPath_);
        } else {
            ExlusiveFileWriter::WaitUnlockForRead(dstPath_);
        }
    }
    ~EtscacheFileLock() = default;

    void WriteEtscacheFile(std::string_view text) const
    {
        ES2PANDA_ASSERT(bool(writer_));
        writer_->Write(text);
    }

    bool ShouldWriteDeclfile() const
    {
        return bool(writer_);
    }

private:
    static bool ShouldRewrite([[maybe_unused]] const std::string &src, [[maybe_unused]] const std::string &dst)
    {
#ifdef USE_UNIX_SYSCALL
        return true;
#else
        return fs::last_write_time(src) > fs::last_write_time(dst);
#endif
    }

    class ExlusiveFileWriter {
    protected:
#ifdef PANDA_TARGET_WINDOWS
        using FD = HANDLE;
#else
        using FD = int;
#endif
    public:
        using Pointer = std::unique_ptr<ExlusiveFileWriter>;

        NO_COPY_SEMANTIC(ExlusiveFileWriter);
        DEFAULT_MOVE_SEMANTIC(ExlusiveFileWriter);

        static void WaitUnlockForRead(const std::string &filename)
        {
            FlockTrace(filename, "Waiting RO unlock");
#ifdef PANDA_TARGET_WINDOWS
            auto const fd =
                ::CreateFileW(Utf8ToWString(ark::os::file::File::GetExtendedFilePath(filename)).c_str(), GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (fd == INVALID_HANDLE_VALUE) {
                std::cerr << "File opening error '" << filename << "': " << GetErrorMessage(::GetLastError())
                          << std::endl;
                return;
            }
            OVERLAPPED ov = {};
            if (::LockFileEx(fd, 0, 0, MAXDWORD, MAXDWORD, &ov)) {
                // Waiting for another process to finish writing the file.
                ::UnlockFileEx(fd, 0, MAXDWORD, MAXDWORD, &ov);
                FlockTrace(filename, "Unlock");
            } else {
                FlockTrace(filename, std::string {"- Waiting error '"} + GetErrorMessage(::GetLastError()) + "'");
            }
            ::CloseHandle(fd);
#else
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            auto const fd = ::open(filename.c_str(), O_RDONLY | O_CLOEXEC);
            if (fd == -1) {
                std::cerr << "File opening error '" << filename << "': " << ::strerror(errno) << std::endl;
                return;
            }
            if (::flock(fd, LOCK_EX) != 0) {
                FlockTrace(filename, std::string {"- Waiting error '"} + ::strerror(errno) + "'");
            } else {
                ::flock(fd, LOCK_UN);
                FlockTrace(filename, "Unlock");
            }
            ::close(fd);
#endif
        }

        static Pointer Open(const std::string &filename)
        {
#ifdef USE_UNIX_SYSCALL
            return {};
#else
            const std::string absDecl = fs::absolute(filename).string();
            fs::create_directories(fs::path(absDecl).parent_path());
            FlockTrace(filename, "Open for write");
#endif
#ifdef PANDA_TARGET_WINDOWS
            FD fd =
                ::CreateFileW(Utf8ToWString(ark::os::file::File::GetExtendedFilePath(filename)).c_str(), GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (fd == INVALID_HANDLE_VALUE) {
                std::cerr << "File opening error '" << filename << "': " << GetErrorMessage(::GetLastError())
                          << std::endl;
                return {};
            }
            OVERLAPPED ov = {};
            if (!::LockFileEx(fd, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, MAXDWORD, MAXDWORD, &ov)) {
                // Waiting for another process to finish writing the file.
                FlockTrace(filename, "- Waiting");
                if (!::LockFileEx(fd, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ov)) {
                    FlockTrace(filename, std::string {"-- Waiting error '"} + GetErrorMessage(::GetLastError()) + "'");
                }
                FlockTrace(filename, "Close (skip)");
                ::UnlockFileEx(fd, 0, MAXDWORD, MAXDWORD, &ov);
                ::CloseHandle(fd);
                return {};
            }
#else
            auto constexpr CHMOD = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            auto const fd = ::open(filename.c_str(), O_WRONLY | O_CREAT | O_CLOEXEC, CHMOD);
            if (fd == -1) {
                std::cerr << "File opening error '" << filename << "': " << ::strerror(errno) << std::endl;
                return {};
            }
            if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
                // Waiting for another process to finish writing the file.
                FlockTrace(filename, "- Waiting");
                if (::flock(fd, LOCK_EX) != 0) {
                    FlockTrace(filename, std::string {"-- Waiting error '"} + ::strerror(errno) + "'");
                }
                FlockTrace(filename, "Close (skip)");
                ::flock(fd, LOCK_UN);
                ::close(fd);
                return {};
            }
#endif
            FlockTrace(filename, "- Locked");
            return Pointer {new ExlusiveFileWriter(fd, filename)};
        }

        void Write(std::string_view text)
        {
            FlockTrace("- Write");
            ES2PANDA_ASSERT(!written_);
#ifdef PANDA_TARGET_WINDOWS
            DWORD bytesWritten = 0;
            if (!::WriteFile(fd_, text.data(), text.size(), &bytesWritten, NULL)) {
                std::cerr << "Error writing to the file '" << filename_ << "': " << GetErrorMessage(::GetLastError())
                          << std::endl;
            } else {
                written_ = true;
            }
#else
            if (::write(fd_, text.data(), text.size()) == -1) {
                std::cerr << "Error writing to the file '" << filename_ << "': " << ::strerror(errno) << std::endl;
            } else {
                written_ = true;
            }
#endif
        }

        ~ExlusiveFileWriter()
        {
            FlockTrace("Close");
#ifdef PANDA_TARGET_WINDOWS
            if (written_) {
                ::SetEndOfFile(fd_);
            }
            OVERLAPPED ov = {};
            if (!::UnlockFileEx(fd_, 0, MAXDWORD, MAXDWORD, &ov)) {
                std::cerr << "File truncate error '" << filename_ << "': " << GetErrorMessage(::GetLastError())
                          << std::endl;
            }
            ::CloseHandle(fd_);
#else
            if (written_) {
                auto const curPos = ::lseek(fd_, 0, SEEK_CUR);
                if (::ftruncate(fd_, curPos) != 0) {
                    std::cerr << "File truncate error '" << filename_ << "': " << ::strerror(errno) << std::endl;
                }
            }
            ::flock(fd_, LOCK_UN);
            ::close(fd_);
#endif
        }

    private:
        explicit ExlusiveFileWriter(FD fd, std::string filename) : fd_(fd), filename_(std::move(filename)) {}

#ifdef PANDA_TARGET_WINDOWS
        static std::string GetErrorMessage(DWORD errorMessageID)
        {
            if (errorMessageID == 0) {
                return {};
            }
            LPSTR messageBuffer = nullptr;
            size_t const size = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                                     FORMAT_MESSAGE_IGNORE_INSERTS,
                                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                                 reinterpret_cast<LPSTR>(&messageBuffer), 0, NULL);
            std::string message(messageBuffer, size);
            ::LocalFree(messageBuffer);
            return message;
        }

        static std::wstring Utf8ToWString(const std::string_view &str)
        {
            if (str.empty()) {
                return {};
            }
            auto const size_needed = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), int(str.size()), NULL, 0);
            auto result = std::wstring(size_t(size_needed), L'\0');
            ::MultiByteToWideChar(CP_UTF8, 0, str.data(), int(str.size()), result.data(), size_needed);
            return result;
        }
#endif
        inline static void FlockTrace(const std::string &filename, const std::string &msg)
        {
            (void)filename;
            (void)msg;
#ifdef IPM_FLOCK_DEBUG_TRACE
            auto const now = std::chrono::steady_clock::now();
            auto const ticks = now.time_since_epoch().count();
            std::cerr << "#FLOCK:" << '\t' << ticks << '\t' << ::getpid() << '\t' << msg << '\t' << filename
                      << std::endl;
#endif  // IPM_FLOCK_DEBUG_TRACE
        }

        inline void FlockTrace(const std::string &msg) const
        {
            FlockTrace(filename_, msg);
        }

    private:
        FD fd_;
        std::string filename_;
        bool written_ = false;
    };

private:
    std::string dstPath_;
    std::string abcPath_;
    ExlusiveFileWriter::Pointer writer_ = {};
};

void ImportPathManager::ExtractEtscacheToFile(const panda_file::File &pf, const std::string &abcPath,
                                              const std::string &cacheDir)
{
    for (auto id : pf.GetClasses()) {
        panda_file::File::EntityId classId(id);
        if (pf.IsExternal(classId)) {
            continue;
        }

        const auto mname = TryExtractMnameFromEtsGlobal(pf, classId);
        if (!mname.has_value()) {
            continue;
        }

        const std::string dstPath {ImportPathManager::FormEtscacheFilePath(*mname, cacheDir)};
        if (EtscacheFileLock lock {dstPath, abcPath}; lock.ShouldWriteDeclfile()) {
            std::stringstream ss;
            panda_file::ClassDataAccessor {pf, classId}.EnumerateAnnotation(
                ImportPathManager::ANNOTATION_MODULE_DECLARATION.data(),
                [&pf, &ss](panda_file::AnnotationDataAccessor &annotationAccessor) {
                    const auto elemDeclaration = annotationAccessor.GetElement(0);
                    const auto valueDeclaration = elemDeclaration.GetScalarValue();
                    const auto idAnnoDeclaration = valueDeclaration.Get<panda_file::File::EntityId>();
                    ss << pf.GetStringData(idAnnoDeclaration).ToString();
                    return true;
                });
            std::string declText = ss.str();
            if (!declText.empty()) {
                auto processed = DeleteEscapeSymbols(declText);
                lock.WriteEtscacheFile(processed);
            }
        }
    }
}

parser::Program *ImportPathManager::IntroduceProgram(const ImportInfo &importInfo)
{
    switch (importInfo.Data().Kind()) {
        case ModuleKind::MODULE:
            return IntroduceProgram<ModuleKind::MODULE>(importInfo);
        case ModuleKind::SOURCE_DECL:
            return IntroduceProgram<ModuleKind::SOURCE_DECL>(importInfo);
        case ModuleKind::ETSCACHE_DECL:
            return IntroduceProgram<ModuleKind::ETSCACHE_DECL>(importInfo);
        case ModuleKind::DECLLESS_DYNAMIC:
            return IntroduceProgram<ModuleKind::DECLLESS_DYNAMIC>(importInfo);
        case ModuleKind::PACKAGE:
            return IntroduceProgram<ModuleKind::PACKAGE>(importInfo);
        case ModuleKind::METADATA_DECL: {
            if (!ShouldUseMetadata(&ctx_)) {
                DE()->LogDiagnostic(diagnostic::UNSUPPORTED_IMPORT_WITH_METADATA,
                                    DiagnosticMessageParams {importInfo.AbcPath()});
                return nullptr;
            }
            ES2PANDA_ASSERT(importInfo.ReferencesABC());
            return IntroduceProgram<ModuleKind::METADATA_DECL>(importInfo);
        }
        default: {
            ES2PANDA_ASSERT(DE()->IsAnyError());
            return nullptr;
        }
    }
}

template <ModuleKind KIND, typename VarBinderT>
parser::ProgramAdapter<KIND> *ImportPathManager::IntroduceProgram(const ImportInfo &importInfo)
{
    ES2PANDA_ASSERT(importInfo.Data().Kind() == KIND);
    parser::ProgramAdapter<KIND> *newProg = nullptr;
    if constexpr (KIND == ModuleKind::PACKAGE) {
        newProg = RegisterSourcesForPackageFromGlobbedDirectory(importInfo);
    } else {
        newProg = parser::Program::New<KIND, VarBinderT>(importInfo, Context());
        ES2PANDA_ASSERT(!importInfo.ModuleName().empty());
        newProg->SetPackageInfo(importInfo.ModuleName(), KIND);
    }
    RegisterProgram(newProg);
    return newProg;
}

class ImportPathManager::ResolvedSources {
public:
    explicit ResolvedSources(ImportPathManager *ipm) : ipm_ {ipm} {}

    void Register(parser::Program *program, DiagnosticEngine *de)
    {
        ArenaString key {program->GetImportInfo().Key()};
        ES2PANDA_ASSERT(progsByResolvedPath_.find(key) == progsByResolvedPath_.end());
        ES2PANDA_ASSERT(exactProgsByResolvedPath_.find(key) == exactProgsByResolvedPath_.end());
        exactProgsByResolvedPath_[key] = program;
        progsByResolvedPath_[key] = program;
        if (program->Is<ModuleKind::PACKAGE>()) {
            progsByResolvedPath_[ArenaString {program->ModuleName()}] = program;
            exactProgsByResolvedPath_[ArenaString {program->ModuleName()}] = program;
        }

        // Handle clashing. Impl-progs are disallowed to clash, while decl-prog are allowed. The restriction is that
        // entities in clashing declarations shouldn't intersect.
        ArenaString mname {program->GetImportInfo().ModuleName()};
        auto &modulePrograms = modules_[mname];
        if (program->IsDeclarationModule()) {
            auto &declProgs = modulePrograms.declProgs;
            ES2PANDA_ASSERT(std::find(declProgs.begin(), declProgs.end(), program) == declProgs.end());
            declProgs.push_back(program);
            if (modulePrograms.implProg != nullptr) {
                progsByResolvedPath_.at(key) = modulePrograms.implProg;
            }
            return;
        }

        if (modulePrograms.implProg != nullptr) {
            ES2PANDA_ASSERT(modulePrograms.implProg != program);
            // NOTE(dkofanov): Fix properly after a decision on index-files. The hack relies on the fact that
            // index-files doesn't contain runtime-entities.
            auto arkuiIndexFileClashException = util::Helpers::EndsWith(mname, ".index");
            if (arkuiIndexFileClashException) {
                return;
            }
            auto absPath1 = program->GetImportInfo().TextSource();
            auto absPath2 = modulePrograms.implProg->GetImportInfo().TextSource();
            de->LogDiagnostic(diagnostic::FILE_RUNTIME_NAME_CLASH,
                              util::DiagnosticMessageParams {absPath1, absPath2, mname});
        }
        modulePrograms.implProg = program;
        if (ipm_->GetGlobalProgram() != nullptr) {
            // Replace effective source lookup, but keep exact declaration programs in ExternalPrograms so their export
            // surfaces are still prepared for checking. The emitter skips replaced exact declarations.
            for (auto *declProg : modulePrograms.declProgs) {
                progsByResolvedPath_.at(ArenaString {declProg->GetImportInfo().Key()}) = program;
            }
        }
    }

    parser::Program *SearchResolved(const ImportInfo &importInfo) const
    {
        if (auto it = progsByResolvedPath_.find(importInfo.Key()); it != progsByResolvedPath_.end()) {
            ES2PANDA_ASSERT(it->second != nullptr);
            return it->second;
        }
        return nullptr;
    }

    parser::Program *SearchResolvedExact(const ImportInfo &importInfo) const
    {
        if (auto it = exactProgsByResolvedPath_.find(importInfo.Key()); it != exactProgsByResolvedPath_.end()) {
            ES2PANDA_ASSERT(it->second != nullptr);
            return it->second;
        }
        return nullptr;
    }

    bool IsReplacedExactSource(const parser::Program *program) const
    {
        if (program == nullptr || !program->IsDeclarationModule()) {
            return false;
        }

        const auto &importInfo = program->GetImportInfo();
        auto exactIt = exactProgsByResolvedPath_.find(importInfo.Key());
        auto effectiveIt = progsByResolvedPath_.find(importInfo.Key());
        return exactIt != exactProgsByResolvedPath_.end() && exactIt->second == program &&
               effectiveIt != progsByResolvedPath_.end() && effectiveIt->second != program;
    }

    void MaybeAddToExternalSources(parser::Program *newProg, parser::Program::ExternalPrograms *extPrograms)
    {
        auto *globalProgram = ipm_->GetGlobalProgram();
        if (newProg == globalProgram) {
            return;
        }

        [[maybe_unused]] bool isPackageFraction =
            (newProg->ModuleInfo().kind == ModuleKind::PACKAGE) && newProg->Is<ModuleKind::MODULE>();
        ES2PANDA_ASSERT(!isPackageFraction);
        if (auto pointedProgram = SearchResolved(newProg->GetImportInfo()); pointedProgram == newProg) {
            if (AlreadyInExternalSources(newProg, extDecls)) {
                return;
            }
            extPrograms->Add(newProg);
        } else {
            [[maybe_unused]] const auto &imd = newProg->GetImportInfo();
            ES2PANDA_ASSERT((imd.Kind() == ModuleKind::SOURCE_DECL) || (imd.Kind() == ModuleKind::ETSCACHE_DECL));
            ES2PANDA_ASSERT(!pointedProgram->IsDeclarationModule());
        }
    }

    void MaybeAddExactToExternalSources(parser::Program *newProg, parser::Program::ExternalPrograms *extPrograms)
    {
        auto *globalProgram = ipm_->GetGlobalProgram();
        if (newProg == nullptr || newProg == globalProgram) {
            return;
        }
        if (!IsReplacedExactSource(newProg)) {
            MaybeAddToExternalSources(newProg, extPrograms);
            return;
        }

        if (AlreadyInExternalSources(newProg, extDecls)) {
            return;
        }
        extPrograms->Add(newProg);
    }

    parser::PackageProgram *FixupPackageByFraction(parser::Program *fractionBeingParsed, const ArenaString &packageName)
    {
        if (progsByResolvedPath_.count(packageName) != 0) {
            // Already fixed.
            auto *pkg = progsByResolvedPath_.at(packageName)->As<ModuleKind::PACKAGE>();
            auto *pointeeProg = ipm_->SearchResolved(fractionBeingParsed->GetImportInfo());
            if (pointeeProg == fractionBeingParsed) {
                ArenaString key {fractionBeingParsed->GetImportInfo().Key()};
                progsByResolvedPath_[key] = pkg;
            }
            ES2PANDA_ASSERT(ipm_->SearchResolved(fractionBeingParsed->GetImportInfo()) == pkg);
            return pkg;
        }

        const_cast<ImportInfo *>(&fractionBeingParsed->GetImportInfo())->moduleName_ = packageName;
        fractionBeingParsed->SetPackageInfo(packageName, util::ModuleKind::PACKAGE);
        ImportInfo pkgImportInfo {};
        pkgImportInfo.resolvedSource_ = packageName;
        pkgImportInfo.moduleName_ = packageName;
        pkgImportInfo.SetData<ModuleKind::PACKAGE, false>(std::string(packageName), "");
        auto newPkg = ipm_->NewEmptyPackage(pkgImportInfo);
        newPkg->AppendFraction(fractionBeingParsed->As<ModuleKind::MODULE>());

        // fixup externalSources:
        auto &modulePrograms = ipm_->GetGlobalProgram()->GetExternalPrograms()->Get<ModuleKind::MODULE>();
        auto newEndIt = std::remove(modulePrograms.begin(), modulePrograms.end(), fractionBeingParsed);
        if (newEndIt != modulePrograms.end()) {
            modulePrograms.erase(newEndIt, modulePrograms.end());
            ipm_->GetGlobalProgram()->GetExternalPrograms()->Add(newPkg);
        } else {
            ES2PANDA_ASSERT(ipm_->GetGlobalProgram() == fractionBeingParsed);
        }

        ES2PANDA_ASSERT(progsByResolvedPath_.find(packageName) == progsByResolvedPath_.end());
        progsByResolvedPath_.find(fractionBeingParsed->GetImportInfo().Key())->second = newPkg;
        progsByResolvedPath_[packageName] = newPkg;
        ES2PANDA_ASSERT(ipm_->SearchResolved(fractionBeingParsed->GetImportInfo()) == newPkg);
        return newPkg;
    }

    void RemoveProgramFromResolvedSources(const ArenaString &filename)
    {
        progsByResolvedPath_.erase(filename);
        exactProgsByResolvedPath_.erase(filename);
        modules_.erase(filename);
    }

private:
    static bool AlreadyInExternalSources(const parser::Program *newProg, const parser::Program::ExternalDecls *extDecls)
    {
        switch (newProg->GetModuleKind()) {
            case ModuleKind::MODULE: {
                const auto &programs = extDecls->Get<ModuleKind::MODULE>();
                return std::find(programs.begin(), programs.end(), newProg) != programs.end();
            }
            case ModuleKind::SOURCE_DECL: {
                const auto &programs = extDecls->Get<ModuleKind::SOURCE_DECL>();
                return std::find(programs.begin(), programs.end(), newProg) != programs.end();
            }
            case ModuleKind::PACKAGE: {
                const auto &programs = extDecls->Get<ModuleKind::PACKAGE>();
                return std::find(programs.begin(), programs.end(), newProg) != programs.end();
            }
            case ModuleKind::ETSCACHE_DECL: {
                const auto &programs = extDecls->Get<ModuleKind::ETSCACHE_DECL>();
                return std::find(programs.begin(), programs.end(), newProg) != programs.end();
            }
            case ModuleKind::METADATA_DECL: {
                const auto &programs = extDecls->Get<ModuleKind::METADATA_DECL>();
                return std::find(programs.begin(), programs.end(), newProg) != programs.end();
            }
            default:
                return false;
        }
    }

    ImportPathManager *ipm_ {};
    ArenaMap<ArenaString, parser::Program *, CompareByLength> exactProgsByResolvedPath_;
    ArenaMap<ArenaString, parser::Program *, CompareByLength> progsByResolvedPath_;
    struct Module {
        parser::Program *implProg {};
        ArenaVector<parser::Program *> declProgs {};
    };
    ArenaMap<ArenaString, Module, CompareByLength> modules_;
};

void ImportPathManager::InitParseQueueForSimult()
{
    ES2PANDA_ASSERT(GetParseQueue().empty());

    IntroduceMainProgramForSimult();
    srcPos_.SetProgram(Context()->parserProgram);

    ES2PANDA_ASSERT(Context()->config->options->GetCompilationMode() >= CompilationMode::SIMULTANEOUS);
    ES2PANDA_ASSERT(Context()->config->options->GetExtension() == ScriptExtension::ETS);
    for (auto &sourceName : Context()->sourceFileNames) {
        // Build of `importInfo` should be refined.
        const std::string sourcePath {sourceName};
        util::ImportInfo importInfo {*this, sourcePath};
        SetEtsTextFileByExtension(&importInfo, sourcePath);
        auto *program = IntroduceProgram(importInfo);
        resolvedSources_.MaybeAddToExternalSources(program, GetGlobalProgram()->GetExternalPrograms());
        program->SetIsBuiltSimultaneously();
    }
}

void ImportPathManager::RegisterProgram(parser::Program *program)
{
    if (program == nullptr) {
        ES2PANDA_ASSERT(DE()->IsAnyError());
        return;
    }

    resolvedSources_.Register(program, DE());

    // Packages are "synthetic" programs (w/o text), so they can't be parsed.
    // Mind the difference with package-fraction programs, constituting packages.
    // Also, metadata-based programs are not source-based so no need to parse them,
    // they are being handled further within a separate compilation phase.
    switch (program->GetModuleKind()) {
        case ModuleKind::PACKAGE:
        case ModuleKind::SIMULT_MAIN:
        case ModuleKind::DECLLESS_DYNAMIC:
        case ModuleKind::METADATA_DECL:
            return;
        default: {
            bool isParsed = program->Ast() != nullptr;
            parseQueue_.emplace_back(ParseInfo {isParsed, program});
        }
    }
}

parser::Program *ImportPathManager::SearchResolved(const ImportInfo &importInfo) const
{
    return resolvedSources_.SearchResolved(importInfo);
}

parser::Program *ImportPathManager::SearchResolvedExact(const ImportInfo &importInfo) const
{
    return resolvedSources_.SearchResolvedExact(importInfo);
}

bool ImportPathManager::IsReplacedExactSource(const parser::Program *program) const
{
    return resolvedSources_.IsReplacedExactSource(program);
}

void ImportPathManager::RemoveProgramFromResolvedSources(const ArenaString &filename)
{
    resolvedSources_.RemoveProgramFromResolvedSources(filename);
}

parser::Program *ImportPathManager::FindOrIntroduceProgramForIncremental(std::string_view absolutePath)
{
    return FindOrIntroduceProgram<false>(absolutePath);
}

template <bool ATTACH_TO_GLOBAL_EXTERNAL_SOURCES>
parser::Program *ImportPathManager::FindOrIntroduceProgram(std::string_view absolutePath)
{
    ImportInfo importInfo {*this, absolutePath, Language::Id::ETS};
    return LookupImportDataAndIntroduceProgram<ATTACH_TO_GLOBAL_EXTERNAL_SOURCES>(&importInfo);
}

// NOTE(dkofanov): Packages are to be removed. Now 'ETSPackageDeclaration' is used to override modulename.
parser::Program *ImportPathManager::EnsurePackageIsRegisteredByPackageFraction(parser::Program *fractionBeingParsed,
                                                                               ir::ETSPackageDeclaration *packageDecl)
{
    ES2PANDA_ASSERT(packageDecl->Program() == fractionBeingParsed);
    auto packageNameNode = packageDecl->Name();
    ArenaString packageName {packageNameNode->IsIdentifier() ? packageNameNode->AsIdentifier()->Name().Utf8()
                                                             : packageNameNode->AsTSQualifiedName()->Name().Utf8()};

    return resolvedSources_.FixupPackageByFraction(fractionBeingParsed, packageName);
}

template <bool ATTACH_TO_GLOBAL_EXTERNAL_SOURCES>
parser::Program *ImportPathManager::LookupImportDataAndIntroduceProgram(ImportInfo *importInfo)
{
    ES2PANDA_ASSERT(SearchResolved(GetGlobalProgram()->GetImportInfo()) != nullptr);
    // NOTE(dkofanov): This step (caching the result) is essential. It is so because:
    // 1. `es2panda` may call `GatherImportInfo` at any lowering/plugin.
    // 2. Cache-update may occur after some lowering or even by another thread.
    // In order for a source-representation (i.e. "parser::Program") to be consistent during compilation routine, it
    // should always be resolved to the same program.
    if (auto resolved = SearchResolved(*importInfo); resolved != nullptr) {
        // #32418.
        if constexpr (ATTACH_TO_GLOBAL_EXTERNAL_SOURCES) {
            resolvedSources_.MaybeAddToExternalSources(resolved, GetGlobalProgram()->GetExternalPrograms());
            auto *exact = SearchResolvedExact(*importInfo);
            if (exact != nullptr && exact != resolved) {
                resolvedSources_.MaybeAddExactToExternalSources(exact, GetGlobalProgram()->GetExternalPrograms());
            }
        }
        return resolved;
    }

    auto *program = LookupProgramCaches(*importInfo);
    if (program != nullptr) {
        ES2PANDA_ASSERT(program->Ast() != nullptr);
    } else {
        LookupMemCache(importInfo);
        if (importInfo->Data().Kind() == ModuleKind::UNKNOWN) {
            LookupDiskData(importInfo);
        }

        if (importInfo->Data().Kind() == ModuleKind::UNKNOWN) {
            if (importInfo->PointsToPackage()) {
                importInfo->SetData<ModuleKind::PACKAGE, false>(std::string(importInfo->Key()), "");
            } else {
                LookupSourceFile(importInfo);
            }
        }

        program = IntroduceProgram(*importInfo);
    }
    if constexpr (ATTACH_TO_GLOBAL_EXTERNAL_SOURCES) {
        if (program != nullptr) {
            auto *resolvedProgram = SearchResolved(*importInfo);
            resolvedSources_.MaybeAddToExternalSources(resolvedProgram, GetGlobalProgram()->GetExternalPrograms());
            if (program != resolvedProgram) {
                resolvedSources_.MaybeAddExactToExternalSources(program, GetGlobalProgram()->GetExternalPrograms());
            }
        }
    }
    return program;
}

void ImportPathManager::LookupMemCache(ImportInfo *importInfo)
{
    if (importInfo->Kind() == ModuleKind::METADATA_DECL) {
        parser::ImportCache<parser::CacheType::METADATA>::GetFromCache(importInfo);
    } else {
        parser::ImportCache<parser::CacheType::SOURCES>::GetFromCache(importInfo);
    }
}

void ImportPathManager::LookupDiskData(ImportInfo *importInfo)
{
    ES2PANDA_ASSERT(importInfo->Data().Kind() == ModuleKind::UNKNOWN);
    if (!importInfo->ReferencesABC()) {
        LookupEtscacheFile(importInfo);
        return;
    }
    ES2PANDA_ASSERT(importInfo->ResolvedPathIsVirtual());

    const auto abcPath = importInfo->AbcPath();
    auto extractEtscache = [this, &abcPath]() {
        const auto pf = panda_file::OpenPandaFile(abcPath);
        if (pf == nullptr) {
            DE()->LogDiagnostic(diagnostic::OPEN_FAILED, DiagnosticMessageParams {abcPath});
            return;
        }
        ExtractEtscacheToFile(*pf, abcPath, ArkTSConfig().CacheDir());
    };
    const auto shouldReadMetadata = ShouldUseMetadata(&ctx_);

    if (const auto processedAbc = processedAbcFiles_.find(abcPath); processedAbc != processedAbcFiles_.end()) {
        if (shouldReadMetadata && processedAbc->second != nullptr && LookupMetadata(importInfo)) {
            return;
        }
        if (processedAbc->second != nullptr) {
            extractEtscache();
        }
        LookupEtscacheFile(importInfo);
        return;
    }

    const auto pf = panda_file::OpenPandaFile(abcPath);
    if (pf == nullptr) {
        DE()->LogDiagnostic(diagnostic::OPEN_FAILED, DiagnosticMessageParams {abcPath});
        LookupEtscacheFile(importInfo);
        return;
    }

    const auto isMetadataEnabled = pf->IsMetadataEnabled();
    processedAbcFiles_.insert({abcPath, shouldReadMetadata && isMetadataEnabled
                                            ? std::make_unique<panda_file::MetadataAccessor>(*pf)
                                            : nullptr});

    if (shouldReadMetadata && isMetadataEnabled && LookupMetadata(importInfo)) {
        return;
    }

    ExtractEtscacheToFile(*pf, abcPath, ArkTSConfig().CacheDir());
    ES2PANDA_ASSERT(importInfo->Data().Kind() == ModuleKind::UNKNOWN);
    LookupEtscacheFile(importInfo);
}

bool ImportPathManager::LookupMetadata(ImportInfo *importInfo) const
{
    const auto metadataAccessor = processedAbcFiles_.at(importInfo->AbcPath()).get();
    std::string pkgName(importInfo->ModuleName());
    if (!pkgName.empty() && pkgName.back() == '.') {
        pkgName.pop_back();
    }
    auto metadata = metadataAccessor->ExtractMetadataForPackage(pkgName);
    if (metadata.empty() || !VerifyMetadataModules(metadata)) {
        return false;
    }
    auto sourceFilePath = std::string(importInfo->extModuleData_->SourceFilePath());
    if (sourceFilePath.empty()) {
        sourceFilePath = importInfo->AbcPath();
    }
    importInfo->SetData<ModuleKind::METADATA_DECL, true>(std::move(sourceFilePath), std::move(metadata));
    return true;
}

void ImportPathManager::LookupEtscacheFile(ImportInfo *importInfo) const
{
    if (ArkTSConfig().CacheDir().empty()) {
        return;
    }
    auto cachefile = FormEtscacheFilePath(std::string {importInfo->ModuleName()}, ArkTSConfig().CacheDir());
    ES2PANDA_ASSERT(cachefile.find(ArkTSConfig().CacheDir()) == 0);
    if (!fsQueryCache_->IsRegularFile(cachefile)) {
        return;
    }

    importInfo->SetTextFile<ModuleKind::ETSCACHE_DECL>(cachefile, DE());
}

void ImportPathManager::SetEtsTextFileByExtension(ImportInfo *importInfo, const std::string &sourcePath) const
{
    ES2PANDA_ASSERT(importInfo != nullptr);
    ES2PANDA_ASSERT(!sourcePath.empty());

    if (Helpers::EndsWith(sourcePath, D_ETS_SUFFIX)) {
        importInfo->SetTextFile<ModuleKind::SOURCE_DECL>(sourcePath, DE());
        return;
    }

    importInfo->SetTextFile<ModuleKind::MODULE>(sourcePath, DE());
}

void ImportPathManager::LookupSourceFile(ImportInfo *importInfo)
{
    if (IsDepAnalyzerMode() && Language(importInfo->Lang()).IsDynamic()) {
        importInfo->SetData<ModuleKind::DECLLESS_DYNAMIC>(std::string(importInfo->ResolvedSource()), "");
        return;
    }
    if (importInfo->HasSpecifiedDeclPath() && !importInfo->ReferencesABC()) {
        importInfo->SetTextFile<ModuleKind::SOURCE_DECL>(std::string(importInfo->DeclPath()), DE());
    } else if (importInfo->Lang() != Language::Id::ETS) {
        importInfo->SetData<ModuleKind::DECLLESS_DYNAMIC, false>(std::string(importInfo->ResolvedSource()), "");
    } else {
        SetEtsTextFileByExtension(importInfo, std::string(importInfo->ResolvedSource()));
    }
}

ImportPathManager::ResolvedPathRes ImportPathManager::TryResolvePath(std::string resolvedPathPrototype) const
{
    auto delim = pathDelimiter_[0];
    std::replace_if(
        resolvedPathPrototype.begin(), resolvedPathPrototype.end(), [delim](auto c) { return c == delim; }, '/');
    if (auto resolvedDependency = ArkTSConfig().FindInDependencies(resolvedPathPrototype);
        resolvedDependency != std::nullopt) {
        // Since declgen is decoupled from the compile process, a dynamic ArkTS declaration file may be declared
        // in arktsconfig but missing on disk. Report an error when such a declaration is referenced but absent.
        if (!IsDepAnalyzerMode() &&
            !CheckDependencyFileExists(resolvedDependency->second.Path(), resolvedPathPrototype)) {
            return {"", false, true};
        }
        return {resolvedPathPrototype, true};
    }
    if (ArkTSConfig().Paths().find(resolvedPathPrototype) != ArkTSConfig().Paths().cend()) {
        return {resolvedPathPrototype, false};
    }
    return {{}, false};
}

// NOTE(dkofanov): Shouldn't directory resolved by 'index'-file be globbed and added to parse list?
std::string ImportPathManager::DirOrDirWithIndexFile(std::string resolvedPathPrototype) const
{
    const auto indexFiles = {"index.ets", "index.sts", "index.ts", "index.d.ets",
                             "Index.ets", "Index.sts", "Index.ts", "Index.d.ets"};
    // Supported index files: keep this checking order
    for (auto indexFile : indexFiles) {
        std::string indexFilePath = resolvedPathPrototype + pathDelimiter_.at(0) + indexFile;
        if (ImportFileExists(*fsQueryCache_, indexFilePath)) {
            return indexFilePath;
        }
    }

    return resolvedPathPrototype;
}

ImportPathManager::ResolutionCacheKey ImportPathManager::BuildResolutionCacheKey(std::string_view path) const
{
    return {isDynamic_, std::string {path}};
}

ImportPathManager::ResolvedPathRes ImportPathManager::ProbeExtensionOrIndexFile(std::string resolvedPathPrototype) const
{
    resolvedPathPrototype = NormalizePathPrototype(std::move(resolvedPathPrototype), pathDelimiter_.at(0));
    const auto cacheKey = BuildResolutionCacheKey(resolvedPathPrototype);
    auto cached = appendExtensionOrIndexFileCache_.find(cacheKey);
    if (cached != appendExtensionOrIndexFileCache_.end()) {
        return cached->second;
    }

    auto cacheAndReturn = [this, &cacheKey](ResolvedPathRes resPathInfo) {
        if (!resPathInfo.hasError && !resPathInfo.resolvedPath.empty()) {
            appendExtensionOrIndexFileCache_.emplace(cacheKey, resPathInfo);
        }
        return resPathInfo;
    };

    if (auto resPathInfo = TryResolvePath(resolvedPathPrototype);
        !resPathInfo.resolvedPath.empty() || resPathInfo.hasError) {
        return cacheAndReturn(resPathInfo);
    }

    if (ImportFileExists(*fsQueryCache_, resolvedPathPrototype)) {
        return cacheAndReturn({ark::os::GetAbsolutePath(resolvedPathPrototype)});
    }

    for (const auto &extension : supportedExtensions) {
        auto pathWithExtension = resolvedPathPrototype + std::string(extension);
        if (ImportFileExists(*fsQueryCache_, pathWithExtension)) {
            return cacheAndReturn({ark::os::GetAbsolutePath(pathWithExtension)});
        }
    }

    if (fsQueryCache_->IsDirectory(resolvedPathPrototype)) {
        return cacheAndReturn({ark::os::GetAbsolutePath(DirOrDirWithIndexFile(resolvedPathPrototype))});
    }

    return {""};
}

ImportPathManager::ResolvedPathRes ImportPathManager::AppendExtensionOrIndexFileIfOmitted(
    std::string resolvedPathPrototype) const
{
    resolvedPathPrototype = NormalizePathPrototype(std::move(resolvedPathPrototype), pathDelimiter_.at(0));
    auto result = ProbeExtensionOrIndexFile(resolvedPathPrototype);
    if (result.resolvedPath.empty() && !result.hasError) {
        DE()->LogDiagnostic(diagnostic::UNSUPPORTED_PATH, util::DiagnosticMessageParams {resolvedPathPrototype},
                            srcPos_);
    }
    return result;
}

// Transform a/b/c.d.ets to a/b/c
static std::string_view RemoveExtensionIfKnown(std::string_view relPath)
{
    for (const auto &ext : ImportPathManager::supportedExtensionsInversed) {
        if (relPath.size() >= ext.size() && (relPath.substr(relPath.size() - ext.size(), ext.size()) == ext)) {
            return relPath.substr(0, relPath.size() - ext.size());
        }
    }
    return relPath;
}

static ArenaString ConcatOhmurl(std::string_view p1, std::string_view p2)
{
    if (!p1.empty() && ((p1.back() == util::PATH_DELIMITER) || (p1.back() == '.'))) {
        p1 = p1.substr(0, p1.size() - 1);
    }
    if (!p2.empty() && ((p2.front() == util::PATH_DELIMITER) || (p2.front() == '.'))) {
        p2 = p2.substr(1, p2.size() - 1);
    }

    if (p1.empty() || p2.empty()) {
        return ArenaString {p1} + ArenaString {p2};
    }
    return ArenaString {p1} + '/' + ArenaString {p2};
}

static ArenaString RebasePathOhmurl(std::string_view pathToRebase, std::string_view oldBase,
                                    std::string_view ohmurlStart)
{
    ES2PANDA_ASSERT(Helpers::StartsWith(pathToRebase, oldBase));
    auto relativePath = pathToRebase.substr(oldBase.size());
    relativePath = RemoveExtensionIfKnown(relativePath);
    auto res = ConcatOhmurl(ohmurlStart, relativePath);
    return res;
}

static ArenaString CheckAndRebaseOhmurl(const ImportPathManager &ipm, std::string_view path, std::string_view oldBase,
                                        std::string_view newBase)
{
    if (!Helpers::StartsWith(path, oldBase)) {
        ipm.Context()->diagnosticEngine->LogDiagnostic(diagnostic::SOURCE_OUTSIDE_ETS_PATH,
                                                       util::DiagnosticMessageParams {path}, ipm.SrcPos());
        ES2PANDA_UNREACHABLE();
    }
    return RebasePathOhmurl(path, oldBase, newBase);
}

static ArenaString CreatePackageModuleName(const ImportPathManager &ipm, std::string_view resolvedSource,
                                           std::string_view unitPath, std::string_view unitName)
{
    if (!ipm.ArkTSConfig().Package().empty() &&
        Helpers::StartsWith(resolvedSource, ipm.ArkTSConfig().BaseUrl() + util::PATH_DELIMITER)) {
        ArenaString mnamePrototype {};

        mnamePrototype =
            CheckAndRebaseOhmurl(ipm, resolvedSource, ipm.ArkTSConfig().BaseUrl(), ipm.ArkTSConfig().Package());
        mnamePrototype = OhmurlToMname(mnamePrototype);
        ES2PANDA_ASSERT(mnamePrototype.at(0) != '.');

        return mnamePrototype;
    }

    return OhmurlToMname(RebasePathOhmurl(resolvedSource, unitPath, unitName));
}

static std::optional<ArenaString> DeduceModuleNameByMatchingWithArktsconfig(const ImportPathManager &importPathManager,
                                                                            const ImportInfo &imd)
{
    // ES2PANDA_ASSERT(( == '/') || (imd.ResolvedSource().find('\\') == std::string::npos));
    using MatcherT = std::optional<ArenaString> (*)(const ArkTsConfig &, std::string_view, const ImportPathManager &);
    std::vector<MatcherT> matchers {};

    // 1. Try 'cacheDir` field:
    auto cacheDirMatcher = [](const ArkTsConfig &cfg, std::string_view resolvedSource,
                              [[maybe_unused]] const ImportPathManager &ipm) -> std::optional<ArenaString> {
        if (cfg.CacheDir().empty() || !Helpers::StartsWith(resolvedSource, cfg.CacheDir())) {
            return std::nullopt;
        }
        // don't append package name, since cacheDir already have it
        std::optional<ArenaString> mName = OhmurlToMname(CheckAndRebaseOhmurl(ipm, resolvedSource, cfg.CacheDir(), ""));
        ES2PANDA_ASSERT(mName->at(0) != '.');
        return mName;
    };
    matchers.emplace_back(cacheDirMatcher);

    // 2. Try 'dynamicPaths' (aka 'dependencies') field:
    auto dynamicPathMatcher = [](const ArkTsConfig &cfg, std::string_view resolvedSource,
                                 [[maybe_unused]] const ImportPathManager &ipm) {
        std::optional<ArenaString> res {};
        if (cfg.FindInDependencies(resolvedSource) != std::nullopt) {
            res = OhmurlToMname(resolvedSource);
        }
        return res;
    };
    matchers.emplace_back(dynamicPathMatcher);

    // 3. Try 'paths' field:
    auto pathsMatcher = [](const ArkTsConfig &cfg, std::string_view resolvedSource,
                           const ImportPathManager &ipm) -> std::optional<ArenaString> {
        for (auto const &[unitName, unitPaths] : cfg.Paths()) {
            auto it = std::find_if(unitPaths.begin(), unitPaths.end(), [resolvedSource](const auto &unitPath) {
                ES2PANDA_ASSERT(!unitPath.empty());
                // NOTE(33661) should be Helpers::StartsWith(resolvedSource, unitName)
                return Helpers::StartsWith(resolvedSource, unitPath);
            });
            if (it != unitPaths.end()) {
                return CreatePackageModuleName(ipm, resolvedSource, *it, unitName);
            }
        }
        return std::nullopt;
    };
    matchers.emplace_back(pathsMatcher);

    for (auto matcher : matchers) {
        if (auto res = (*matcher)(importPathManager.ArkTSConfig(), imd.ResolvedSource(), importPathManager); res) {
            return res;
        }
    }
    return std::nullopt;
}

// NOTE(dkofanov): special empty programs for simult and stdlib should be removed.
// These names shouldn't be emitted to a binary.
static std::optional<ArenaString> CheckSpecialModuleName(const ImportInfo &imd)
{
    if (imd.ResolvedSource() == STDLIB_MAIN_PROG_NAME) {
        return compiler::Signatures::STDLIB_MODULE_NAME.data();
    }
    if (imd.ResolvedSource() == STDLIB_IMPORTS_MAIN_PROG_NAME) {
        return compiler::Signatures::STDLIB_IMPORTS_MODULE_NAME.data();
    }
    if (imd.ResolvedSource() == SIMULT_MAIN_PROG_NAME) {
        return compiler::Signatures::SIMULT_MODULE_NAME.data();
    }

    return std::nullopt;
}

static ArenaString FormModuleNameSolelyByAbsolutePath(const ImportPathManager &ipm, const ImportInfo &imd)
{
    auto etsPath = ipm.Context()->config->options->GetEtsPath();
    auto absoluteEtsPath = util::Path(etsPath, ipm.Context()->allocator).GetAbsolutePath().Utf8();
    return OhmurlToMname(CheckAndRebaseOhmurl(ipm, imd.ResolvedSource(), absoluteEtsPath, ""));
}

static ArenaString FormModuleName(const ImportPathManager &ipm, const ImportInfo &imd)
{
    ES2PANDA_ASSERT(imd.ModuleName().empty());

    if (auto res = CheckSpecialModuleName(imd); res) {
        return *res;
    }

    if (ipm.Context()->config->options->WasSetEtsPath()) {
        return FormModuleNameSolelyByAbsolutePath(ipm, imd);
    }

    if (auto res = DeduceModuleNameByMatchingWithArktsconfig(ipm, imd); res) {
        return *res;
    }

    if (Helpers::StartsWith(imd.ResolvedSource(), ipm.ArkTSConfig().BaseUrl())) {
        ArenaString mnamePrototype {};
        if (!ipm.ArkTSConfig().Package().empty()) {
            mnamePrototype += ipm.ArkTSConfig().Package();
        }
        if (ipm.ArkTSConfig().UseUrl() || !ipm.ArkTSConfig().Package().empty()) {
            auto rebased = RebasePathOhmurl(imd.ResolvedSource(), ipm.ArkTSConfig().BaseUrl(), "");
            if (!mnamePrototype.empty() && !rebased.empty() && (rebased[0] != '/')) {
                mnamePrototype += '.';
            }
            mnamePrototype += OhmurlToMname(rebased);
            ES2PANDA_ASSERT(mnamePrototype.at(0) != '.');
            return mnamePrototype;
        }
    }

    return ArenaString(util::Path(imd.ResolvedSource(), ipm.Context()->allocator).GetFileName().Utf8());
}

static void CheckNoColonInName(std::string_view name, util::DiagnosticEngine *diagnosticEngine)
{
    if (name.find(':') != std::string_view::npos) {
        util::DiagnosticMessageParams diagParams = {name};
        diagnosticEngine->LogDiagnostic(diagnostic::UNSUPPORTED_FILE_NAME, diagParams);
    }
}

static void CheckModuleName(const ImportPathManager &ipm, const ImportInfo &imd)
{
    CheckNoColonInName(imd.ModuleName(), ipm.Context()->diagnosticEngine);
    if (imd.ModuleName().empty()) {
        ipm.Context()->diagnosticEngine->LogDiagnostic(diagnostic::UNRESOLVED_MODULE,
                                                       DiagnosticMessageParams {imd.TextSource()});
    }
}

bool ImportInfo::PointsToPackage() const
{
    if (ResolvedPathIsVirtual()) {
        return false;
    }
    if (ark::os::file::File::IsDirectory(std::string(resolvedSource_))) {
        return true;
    }

    const auto *program = importPathManager_->SearchResolvedExact(*this);
    return program != nullptr && program->IsBuiltSimultaneously();
}

ImportInfo::ImportInfo(const ImportPathManager &ipm, std::string_view resolvedSource, Language::Id lang,
                       bool isExternalModule)
    : resolvedSource_ {resolvedSource}, importPathManager_ {&ipm}
{
    SetKey(resolvedSource_);
    if (isExternalModule) {
        ES2PANDA_ASSERT(ResolvedPathIsVirtual());
        auto dependenciesLookupResult = ipm.ArkTSConfig().FindInDependencies(ResolvedSource());
        ES2PANDA_ASSERT(dependenciesLookupResult != std::nullopt);
        extModuleData_ = &dependenciesLookupResult->second;
        lang_ = extModuleData_->GetLanguage().GetId();
    } else {
        lang_ = lang;
    }
    moduleName_ = FormModuleName(ipm, *this);
    CheckModuleName(ipm, *this);
}

// NOLINTNEXTLINE(bugprone-copy-constructor-init)
ImportInfo::ImportInfo(const ImportInfo &other)
{
    *this = other;
}

// NOLINTNEXTLINE(misc-unconventional-assign-operator,bugprone-unhandled-self-assignment)
const ImportInfo &ImportInfo::operator=(const ImportInfo &other)
{
    parser::CacheReference<>::operator=(other);
    resolvedSource_ = other.resolvedSource_;
    moduleName_ = other.moduleName_;
    importPathManager_ = other.importPathManager_;
    extModuleData_ = other.extModuleData_;
    lang_ = other.lang_;

    SetKey(resolvedSource_);
    return *this;
}

std::string_view ImportInfo::DeclPath() const
{
    return (extModuleData_ != nullptr) ? extModuleData_->Path() : std::string_view {};
}

std::string_view ImportInfo::OhmUrl() const
{
    if ((extModuleData_ != nullptr) && !extModuleData_->OhmUrl().empty()) {
        return extModuleData_->OhmUrl();
    }
    if (ReferencesABC()) {
        return AbcPath();
    }

    return "";
}

bool ImportInfo::HasSpecifiedDeclPath() const
{
    return !DeclPath().empty() && (DeclPath() != DUMMY_PATH);
}

bool ImportInfo::IsValid() const
{
    return !resolvedSource_.empty() && (resolvedSource_ != ERROR_LITERAL);
}

ImportPathManager::ImportPathManager(public_lib::Context *context)
    : ctx_(*context),
      parseQueue_(context->Allocator()->Adapter()),
      resolvedSources_(*ArenaAllocator::New<ResolvedSources>(this)),
      fsQueryCache_(std::make_unique<FsQueryCache>())
{
}

ImportPathManager::~ImportPathManager() = default;

}  // namespace ark::es2panda::util
#undef USE_UNIX_SYSCALL
