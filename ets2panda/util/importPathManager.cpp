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
#include "util/diagnosticEngine.h"
#include "generated/diagnostic.h"

#include "parser/ETSparser.h"
#include "parser/program/program.h"
#include "ir/expressions/literals/stringLiteral.h"

#include "compiler/lowering/ets/declGenPhase.h"

#include "libarkfile/class_data_accessor-inl.h"
#include "libarkfile/file-inl.h"
#include "libarkbase/utils/logger.h"

#include "util/es2pandaMacros.h"
#include "util/path.h"
#include "varbinder/ETSBinder.h"
#include "varbinder/TSBinder.h"
#include "varbinder/ASBinder.h"
#include "varbinder/JSBinder.h"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string_view>
#include <utility>

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

static std::string ExtractMnameFromPandafile(const panda_file::File &pf, const panda_file::File::EntityId &classId)
{
    // processing name to get ohmUrl
    std::string name = utf::Mutf8AsCString(pf.GetStringData(classId).data);
    auto type = pandasm::Type::FromDescriptor(name);
    type = pandasm::Type(type.GetNameWithoutRank(), type.GetRank());
    auto recordName = type.GetPandasmName();

    // rely on the following mangling: <moduleName>.ETSGLOBAL
    auto etsGlobalSuffix = std::string(".") + std::string(compiler::Signatures::ETS_GLOBAL);
    ES2PANDA_ASSERT(Helpers::EndsWith(recordName, etsGlobalSuffix));
    auto mname = recordName.substr(0, recordName.size() - etsGlobalSuffix.size());
    return mname;
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
    if ((importedProgram == importer) && !importer->IsStdLib()) {
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

void ImportPathManager::TryMatchDynamicResolvedPath(ImportPathManager::ResolvedPathRes *result) const
{
    auto packagePathPair = ArkTSConfig().SourcePathMap().find(result->resolvedPath);
    if (packagePathPair != ArkTSConfig().SourcePathMap().cend()) {
        result->resolvedPath = packagePathPair->second;
        result->resolvedIsExternalModule = true;
        return;
    }
    auto paths = ArkTSConfig().Paths().find(result->resolvedPath);
    if (paths != ArkTSConfig().Paths().cend()) {
        result->resolvedPath = *paths->second.begin();
        result->resolvedIsExternalModule = false;
    }
}

ImportInfo ImportPathManager::ResolvePath(parser::Program *importer, std::string_view importPath) const
{
    if (importPath.empty()) {
        DE()->LogDiagnostic(diagnostic::EMPTY_IMPORT_PATH, util::DiagnosticMessageParams {});
        return {};
    }
    ResolvedPathRes result {};
    if (IsRelativePath(importPath)) {
        auto curModulePath = isDynamic_ ? importer->GetImportInfo().ResolvedSource() : importer->AbsoluteName().Utf8();
        size_t pos = curModulePath.find_last_of("/\\");
        auto currentDir = (pos != std::string::npos) ? curModulePath.substr(0, pos) : curModulePath;
        std::string resolvedPathPrototype {currentDir};
        resolvedPathPrototype += pathDelimiter_;
        resolvedPathPrototype += importPath;
        result = AppendExtensionOrIndexFileIfOmitted(resolvedPathPrototype);
        if (result.resolvedIsExternalModule) {
            TryMatchStaticResolvedPath(&result);
        } else {
            TryMatchDynamicResolvedPath(&result);
        }
    } else {
        result = ResolveAbsolutePath(importPath);
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

    auto resolvedPath = ArkTSConfig().ResolvePath(importPath, isDynamic_);
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

std::string GetRealPath(const std::string &path)
{
    return ark::os::GetAbsolutePath(path);
}

template <typename VarBinderT, Language::Id LANG_ID>
void ImportPathManager::SetupGlobalProgram(public_lib::Context *ctx)
{
    ES2PANDA_ASSERT(Context()->config->options->GetCompilationMode() != CompilationMode::GEN_STD_LIB);
    // NOTE(dkofanov): this code tries to handle pseudo-files provided by unrelated 'ctx->sourceFile->filePath' and
    // 'ctx->input'.

    auto normalizedPathForGlobalProg = GetRealPath(std::string(ctx->sourceFile->filePath));
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
    Context()->parserProgram->GetExternalDecls()->Add(program);
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
    GetFileDependencies().erase(ArenaString {program->AbsoluteName().Utf8()});
    parseQueue_.emplace_back(ParseInfo {false, program});
    srcPos_.SetProgram(program);
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

inline Span<const uint8_t> ImportInfo::GetMetadata(const panda_file::File &pf)
{
    return pf.GetMetadata();
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
        auto &modules = GetGlobalProgram()->GetExternalDecls()->Get<ModuleKind::MODULE>();
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
                ::CreateFileW(Utf8ToWString(filename).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
            FD fd = ::CreateFileW(Utf8ToWString(filename).c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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
    for (auto id : pf.GetExported()) {
        panda_file::File::EntityId classId(id);
        auto mname = ExtractMnameFromPandafile(pf, classId);
        std::string dstPath {ImportPathManager::FormEtscacheFilePath(mname, cacheDir)};
        if (EtscacheFileLock lock {dstPath, abcPath}; lock.ShouldWriteDeclfile()) {
            std::stringstream ss;
            panda_file::ClassDataAccessor {pf, classId}.EnumerateAnnotation(
                ImportPathManager::ANNOTATION_MODULE_DECLARATION.data(),
                [&pf, &ss](panda_file::AnnotationDataAccessor &annotationAccessor) {
                    auto elemDeclaration = annotationAccessor.GetElement(0);
                    auto valueDeclaration = elemDeclaration.GetScalarValue();
                    const auto idAnnoDeclaration = valueDeclaration.Get<panda_file::File::EntityId>();
                    ss << panda_file::StringDataToString(pf.GetStringData(idAnnoDeclaration));
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
        case ModuleKind::METADATA_DECL:
            if (!ctx_.config->options->IsReadMetadata()) {
                DE()->LogDiagnostic(diagnostic::UNSUPPORTED_IMPORT_WITH_METADATA,
                                    DiagnosticMessageParams {importInfo.AbcPath()});
                return nullptr;
            }
            ES2PANDA_ASSERT(importInfo.ReferencesABC());
            return IntroduceProgram<ModuleKind::METADATA_DECL>(importInfo);
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
            // Replace effective source lookup, but keep exact declaration programs in ExternalDecls so their export
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

    void MaybeAddToExternalSources(parser::Program *newProg, parser::Program::ExternalDecls *extDecls)
    {
        auto *globalProgram = ipm_->GetGlobalProgram();
        if (newProg == globalProgram) {
            return;
        }

        [[maybe_unused]] bool isPackageFraction =
            (newProg->ModuleInfo().kind == ModuleKind::PACKAGE) && newProg->Is<ModuleKind::MODULE>();
        ES2PANDA_ASSERT(!isPackageFraction);
        if (auto pointedProgram = SearchResolved(newProg->GetImportInfo()); pointedProgram == newProg) {
            auto alreadyInExternalSources = [newProg, extDecls]() -> bool {
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
            };
            if (alreadyInExternalSources()) {
                return;
            }
            extDecls->Add(newProg);
        } else {
            [[maybe_unused]] const auto &imd = newProg->GetImportInfo();
            ES2PANDA_ASSERT((imd.Kind() == ModuleKind::SOURCE_DECL) || (imd.Kind() == ModuleKind::ETSCACHE_DECL));
            ES2PANDA_ASSERT(!pointedProgram->IsDeclarationModule());
        }
    }

    void MaybeAddExactToExternalSources(parser::Program *newProg, parser::Program::ExternalDecls *extDecls)
    {
        auto *globalProgram = ipm_->GetGlobalProgram();
        if (newProg == nullptr || newProg == globalProgram) {
            return;
        }
        if (!IsReplacedExactSource(newProg)) {
            MaybeAddToExternalSources(newProg, extDecls);
            return;
        }

        const auto &programs = extDecls->Get<ModuleKind::SOURCE_DECL>();
        if (std::find(programs.begin(), programs.end(), newProg) != programs.end()) {
            return;
        }
        extDecls->Add(newProg);
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
        ImportInfo pkgMetadata {};
        pkgMetadata.resolvedSource_ = packageName;
        pkgMetadata.moduleName_ = packageName;
        pkgMetadata.SetData<ModuleKind::PACKAGE, false>(std::string(packageName), "");
        auto newPkg = ipm_->NewEmptyPackage(pkgMetadata);
        newPkg->AppendFraction(fractionBeingParsed->As<ModuleKind::MODULE>());

        // fixup externalSources:
        auto &modulePrograms = ipm_->GetGlobalProgram()->GetExternalDecls()->Get<ModuleKind::MODULE>();
        auto newEndIt = std::remove(modulePrograms.begin(), modulePrograms.end(), fractionBeingParsed);
        if (newEndIt != modulePrograms.end()) {
            modulePrograms.erase(newEndIt, modulePrograms.end());
            ipm_->GetGlobalProgram()->GetExternalDecls()->Add(newPkg);
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
        util::ImportInfo importInfo {*this, sourceName};
        importInfo.SetTextFile<ModuleKind::MODULE>(std::string(sourceName), DE());
        auto *program = IntroduceProgram(importInfo);
        resolvedSources_.MaybeAddToExternalSources(program, GetGlobalProgram()->GetExternalDecls());
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
            resolvedSources_.MaybeAddToExternalSources(resolved, GetGlobalProgram()->GetExternalDecls());
            auto *exact = SearchResolvedExact(*importInfo);
            if (exact != nullptr && exact != resolved) {
                resolvedSources_.MaybeAddExactToExternalSources(exact, GetGlobalProgram()->GetExternalDecls());
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
            resolvedSources_.MaybeAddToExternalSources(resolvedProgram, GetGlobalProgram()->GetExternalDecls());
            if (program != resolvedProgram) {
                resolvedSources_.MaybeAddExactToExternalSources(program, GetGlobalProgram()->GetExternalDecls());
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

    auto abcPath = importInfo->AbcPath();
    if (processedAbcFiles_.count(abcPath) != 0) {
        LookupEtscacheFile(importInfo);
        return;
    }
    processedAbcFiles_.insert(abcPath);

    const auto pf = panda_file::OpenPandaFile(abcPath);
    if (pf == nullptr) {
        DE()->LogDiagnostic(diagnostic::OPEN_FAILED, DiagnosticMessageParams {abcPath});
        LookupEtscacheFile(importInfo);
        return;
    }

    if (pf->IsMetadataUsed()) {
        importInfo->SetBinFile(*pf);
        return;
    }

    ExtractEtscacheToFile(*pf, abcPath, ArkTSConfig().CacheDir());
    ES2PANDA_ASSERT(importInfo->Data().Kind() == ModuleKind::UNKNOWN);
    LookupEtscacheFile(importInfo);
}

void ImportPathManager::LookupEtscacheFile(ImportInfo *importInfo)
{
    if (ArkTSConfig().CacheDir().empty()) {
        return;
    }
    auto cachefile = FormEtscacheFilePath(std::string {importInfo->ModuleName()}, ArkTSConfig().CacheDir());
    ES2PANDA_ASSERT(cachefile.find(ArkTSConfig().CacheDir()) == 0);
    if (!ark::os::file::File::IsRegularFile(cachefile)) {
        return;
    }

    importInfo->SetTextFile<ModuleKind::ETSCACHE_DECL>(cachefile, DE());
}

void ImportPathManager::LookupSourceFile(ImportInfo *importInfo)
{
    if (importInfo->HasSpecifiedDeclPath() && !importInfo->ReferencesABC()) {
        importInfo->SetTextFile<ModuleKind::SOURCE_DECL>(std::string(importInfo->DeclPath()), DE());
    } else if (Helpers::EndsWith(importInfo->ResolvedSource(), D_ETS_SUFFIX)) {
        importInfo->SetTextFile<ModuleKind::SOURCE_DECL>(std::string(importInfo->ResolvedSource()), DE());
    } else if (importInfo->Lang() != Language::Id::ETS) {
        importInfo->SetData<ModuleKind::DECLLESS_DYNAMIC, false>(std::string(importInfo->ResolvedSource()), "");
    } else {
        importInfo->SetTextFile<ModuleKind::MODULE>(std::string(importInfo->ResolvedSource()), DE());
    }
}

ImportPathManager::ResolvedPathRes ImportPathManager::TryResolvePath(std::string resolvedPathPrototype) const
{
    auto delim = pathDelimiter_[0];
    std::replace_if(
        resolvedPathPrototype.begin(), resolvedPathPrototype.end(), [delim](auto c) { return c == delim; }, '/');
    if (ArkTSConfig().FindInDependencies(resolvedPathPrototype) != std::nullopt) {
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
#if defined(PANDA_TARGET_WINDOWS)
        bool fileExists = ark::os::file::File::IsRegularFileCaseSensitive(indexFilePath);
#else
        bool fileExists = ark::os::file::File::IsRegularFile(indexFilePath);
#endif
        if (fileExists) {
            return indexFilePath;
        }
    }

    return resolvedPathPrototype;
}

ImportPathManager::ResolvedPathRes ImportPathManager::AppendExtensionOrIndexFileIfOmitted(
    std::string resolvedPathPrototype) const
{
    char delim = pathDelimiter_.at(0);
    std::replace_if(
        resolvedPathPrototype.begin(), resolvedPathPrototype.end(),
        [delim](char c) { return ((delim != c) && ((c == '\\') || (c == '/'))); }, delim);

    resolvedPathPrototype = ark::os::NormalizePath(resolvedPathPrototype);
    if (auto resPathInfo = TryResolvePath(resolvedPathPrototype); !resPathInfo.resolvedPath.empty()) {
        return resPathInfo;
    }

#if defined(PANDA_TARGET_WINDOWS)
    bool fileExists = ark::os::file::File::IsRegularFileCaseSensitive(resolvedPathPrototype);
#else
    bool fileExists = ark::os::file::File::IsRegularFile(resolvedPathPrototype);
#endif
    if (fileExists) {
        return {GetRealPath(resolvedPathPrototype)};
    }

    for (const auto &extension : supportedExtensions) {
        auto pathWithExtension = resolvedPathPrototype + std::string(extension);
#if defined(PANDA_TARGET_WINDOWS)
        bool fileExistsExt = ark::os::file::File::IsRegularFileCaseSensitive(pathWithExtension);
#else
        bool fileExistsExt = ark::os::file::File::IsRegularFile(pathWithExtension);
#endif
        if (fileExistsExt) {
            return {GetRealPath(pathWithExtension)};
        }
    }

    if (ark::os::file::File::IsDirectory(resolvedPathPrototype)) {
        return {GetRealPath(DirOrDirWithIndexFile(resolvedPathPrototype))};
    }

    DE()->LogDiagnostic(diagnostic::UNSUPPORTED_PATH, util::DiagnosticMessageParams {resolvedPathPrototype}, srcPos_);
    return {""};
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

ImportInfo::ImportInfo(const ImportPathManager &ipm, std::string_view resolvedSource, Language::Id lang,
                       bool isExternalModule)
    : resolvedSource_ {resolvedSource}
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
      resolvedSources_(*ArenaAllocator::New<ResolvedSources>(this))
{
}

}  // namespace ark::es2panda::util
#undef USE_UNIX_SYSCALL
