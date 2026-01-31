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
ProgramAdapter<KIND> *Program::New(const util::ImportMetadata &importMetadata, public_lib::Context *context)
{
    varbinder::VarBinder *actualVB = nullptr;
    if constexpr (!std::is_same_v<VarBinderT, void>) {
        ES2PANDA_ASSERT(context->parserProgram == nullptr);
        actualVB = context->Allocator()->New<VarBinderT>(context);
    } else {
        actualVB = context->parserProgram->VarBinder();
    }
    ES2PANDA_ASSERT(actualVB != nullptr);
    return context->Allocator()->New<ProgramAdapter<KIND>>(importMetadata, context->Allocator(), actualVB);
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

parser::Program *ImportPathManager::GatherImportMetadata(parser::Program *importer, ir::StringLiteral *importPath)
{
    srcPos_ = importPath->Start();
    isDynamic_ = importer->IsDeclForDynamicStaticInterop();

    auto importData = ResolvePath(importer, *importPath);
    if (importData.ResolvedSource().empty() || !importData.IsValid()) {
        ES2PANDA_ASSERT(DE()->IsAnyError());
        return nullptr;
    }

    GetGlobalProgram()->AddFileDependencies(importer->AbsoluteName().Utf8(), importData.ResolvedSource());
    LOG(DEBUG, ES2PANDA) << "[" << importer->ModuleInfo().moduleName << "] "
                         << "Import " << importPath->ToString() << " resolved to " << importData.ResolvedSource();

    return LookupCachesAndIntroduceProgram(&importData);
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
    auto importMetadata = ResolvePath(importer, *importPath);
    auto resolvedPath = UString(importMetadata.ResolvedSource(), Context()->Allocator());
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
    }
}

ImportMetadata ImportPathManager::ResolvePath(parser::Program *importer, std::string_view importPath) const
{
    if (importPath.empty()) {
        DE()->LogDiagnostic(diagnostic::EMPTY_IMPORT_PATH, util::DiagnosticMessageParams {});
        return {};
    }
    ResolvedPathRes result {};
    if (IsRelativePath(importPath)) {
        auto curModulePath =
            isDynamic_ ? importer->GetImportMetadata().ResolvedSource() : importer->AbsoluteName().Utf8();
        size_t pos = curModulePath.find_last_of("/\\");
        auto currentDir = (pos != std::string::npos) ? curModulePath.substr(0, pos) : ".";
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

parser::PackageProgram *ImportPathManager::NewEmptyPackage(const ImportMetadata &importMetadata)
{
    auto allocator = Context()->allocator;
    auto package = parser::Program::New<ModuleKind::PACKAGE>(importMetadata, Context());
    package->SetPackageInfo(importMetadata.ModuleName(), util::ModuleKind::PACKAGE);

    auto ident = allocator->New<ir::Identifier>(compiler::Signatures::ETS_GLOBAL, allocator);
    ArenaVector<ir::Statement *> stmts(allocator->Adapter());
    auto etsModule = allocator->New<ir::ETSModule>(allocator, std::move(stmts), ident, ir::ModuleFlag::ETSSCRIPT,
                                                   Language::Id::ETS, package);
    package->SetAst(etsModule);
    return package;
}

std::string GetRealPath(std::string path)
{
    const std::string realPath = ark::os::GetAbsolutePath(path);
    return realPath;
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
        util::ImportMetadata importData {*this, normalizedPathForGlobalProg};
        importData.SetText<ModuleKind::MODULE>(normalizedPathForGlobalProg, std::string(ctx->input));
        ctx->parserProgram = IntroduceProgram<ModuleKind::MODULE, VarBinderT>(importData);
    } else {
        util::ImportMetadata importData {};
        importData.moduleName_ = normalizedPathForGlobalProg;
        importData.lang_ = LANG_ID;
        importData.SetText<ModuleKind::MODULE>(normalizedPathForGlobalProg, std::string(ctx->input));
        ctx->parserProgram = IntroduceProgram<ModuleKind::MODULE, VarBinderT>(importData);
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
        util::ImportMetadata importData {*this, STDLIB_MAIN_PROG_NAME};
        importData.SetText<ModuleKind::MODULE, false>(STDLIB_MAIN_PROG_NAME, "");
        Context()->parserProgram = IntroduceProgram<ModuleKind::MODULE, varbinder::ETSBinder>(importData);
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
    ES2PANDA_ASSERT(std::find(ohmurl.begin(), ohmurl.end(), '\\') == ohmurl.end());
    if (ohmurl.at(0) == '/') {
        ohmurl.erase(0, 1);
    }
    ArenaString mnamePrototype {std::move(ohmurl)};
    std::replace(mnamePrototype.begin(), mnamePrototype.end(), '/', '.');
    return mnamePrototype;
}

static ArenaString OhmurlToMname(std::string_view ohmurl)
{
    return OhmurlToMname(ArenaString {ohmurl});
}

parser::Program *ImportPathManager::SetupProgramForDebugInfoPlugin(std::string_view sourceFilePath,
                                                                   [[maybe_unused]] std::string_view moduleName)
{
    util::ImportMetadata importData {
        *this,
        std::string(sourceFilePath),
    };
    importData.SetText<ModuleKind::MODULE, false>(std::string(sourceFilePath), "");
    // NOTE(dkofanov): The new program is added to 'ImportPathManager::resolvedSources_' during this call, so it can be
    // later resolved via 'SearchResolved'. This may be incorrect.
    auto *program = IntroduceProgram<ModuleKind::MODULE>(importData);

    program->SetSource({sourceFilePath, "", "", true, false});
    ES2PANDA_ASSERT(importData.ModuleName() == moduleName);

    auto allocator = Context()->Allocator();
    auto *emptyIdent = allocator->New<ir::Identifier>("", allocator);
    auto *etsModule = allocator->New<ir::ETSModule>(allocator, ArenaVector<ir::Statement *>(allocator->Adapter()),
                                                    emptyIdent, ir::ModuleFlag::ETSSCRIPT, importData.Lang(), program);
    program->SetAst(etsModule);
    Context()->parserProgram->GetExternalSources()->Add(program);
    return program;
}

parser::Program *ImportPathManager::IntroduceStdlibImportProgram(std::string &&contents)
{
    util::ImportMetadata importData {*this, STDLIB_IMPORTS_MAIN_PROG_NAME};
    importData.SetText<ModuleKind::MODULE>(STDLIB_IMPORTS_MAIN_PROG_NAME, std::move(contents));
    return IntroduceProgram<ModuleKind::MODULE>(importData);
}

void ImportPathManager::IntroduceMainProgramForSimult()
{
    ES2PANDA_ASSERT(Context()->parserProgram == nullptr);

    // NOTE(dkofanov): special empty programs for simult and stdlib should be removed.
    util::ImportMetadata importData {*this, SIMULT_MAIN_PROG_NAME};
    importData.SetText<ModuleKind::SIMULT_MAIN, false>(SIMULT_MAIN_PROG_NAME, "");
    auto program = IntroduceProgram<ModuleKind::SIMULT_MAIN, varbinder::ETSBinder>(importData);

    auto allocator = Context()->allocator;
    auto ident = allocator->New<ir::Identifier>(compiler::Signatures::ETS_GLOBAL, allocator);
    ArenaVector<ir::Statement *> stmts(allocator->Adapter());
    auto etsModule = allocator->New<ir::ETSModule>(allocator, std::move(stmts), ident, ir::ModuleFlag::ETSSCRIPT,
                                                   Language::Id::ETS, program);
    program->SetAst(etsModule);
    Context()->parserProgram = program;
}

void ImportPathManager::InitParseQueueForSimult()
{
    ES2PANDA_ASSERT(GetParseQueue().empty());

    IntroduceMainProgramForSimult();
    srcPos_.SetProgram(Context()->parserProgram);

    // NOTE(dkofanov): Looks like this function is called not only for the simult, but for LSP or whatever.
    bool isSimult = Context()->config->options->IsSimultaneous();
    if (isSimult) {
        ES2PANDA_ASSERT(Context()->config->options->GetCompilationMode() ==
                        CompilationMode::GEN_ABC_FOR_EXTERNAL_SOURCE);
        ES2PANDA_ASSERT(Context()->config->options->GetExtension() == ScriptExtension::ETS);
    }

    std::vector<util::StringView> directImportsFromMainSource {};
    for (auto &sourceName : Context()->sourceFileNames) {
        directImportsFromMainSource.emplace_back(sourceName);

        // Build of `importMetadata` should be refined.
        util::ImportMetadata importData {*this, sourceName};
        auto program = LookupCachesAndIntroduceProgram(&importData);
        if (isSimult) {
            program->SetGenAbcForExternalSources();
        }
    }

    ES2PANDA_ASSERT(directImportsFromMainSource_.empty());
    directImportsFromMainSource_ = directImportsFromMainSource;
}

static bool IsExtensionForPackageFraction(const std::string &extension)
{
    return extension == ImportPathManager::ETS_SUFFIX;
}

parser::PackageProgram *ImportPathManager::RegisterSourcesForPackageFromGlobbedDirectory(
    const ImportMetadata &importMetadata)
{
    ES2PANDA_ASSERT(importMetadata.PointsToPackage());
    ES2PANDA_ASSERT(LookupProgramCaches(importMetadata) == nullptr);

    auto *package = NewEmptyPackage(importMetadata);

#ifdef USE_UNIX_SYSCALL
    UnixRegisterSourcesForPackageFromGlobbedDirectory(package, importMetadata);
#else
    for (auto const &entry : fs::directory_iterator(std::string(importMetadata.ResolvedSource()))) {
        if (!fs::is_regular_file(entry) || !IsExtensionForPackageFraction(entry.path().extension().string())) {
            continue;
        }

        ImportMetadata globElemImportMetadata {*this, entry.path().string(), Language::Id::ETS};
        RegisterPackageFraction(package, &globElemImportMetadata);
    }
#endif

    return package;
}

#ifdef USE_UNIX_SYSCALL
void ImportPathManager::UnixRegisterSourcesForPackageFromGlobbedDirectory(parser::PackageProgram *package,
                                                                          const ImportMetadata &importMetadata)
{
    const auto directoryPath = std::string(importMetadata.ResolvedSource());
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
        ImportMetadata globElemImportMetadata {*this, filePath, Language::Id::ETS};
        RegisterPackageFraction(package, &globElemImportMetadata);
    }

    closedir(dir);
    return;
}
#endif

void ImportMetadata::LinkFractionMetadataToPackage(const parser::PackageProgram &package)
{
    moduleName_ = package.ModuleName();
}

void ImportPathManager::RegisterPackageFraction(parser::PackageProgram *package, ImportMetadata *importMetadata)
{
    auto *fraction = SearchResolved(*importMetadata);
    // The fraction may be previously added via direct import of it (by real path). Since the program is parsed later
    // that step, it's impossible to decide, whether it's part of a package or just module until now. So here the
    // already registered fraction is being aligned to the others fractions.
    if (fraction == nullptr) {
        ES2PANDA_ASSERT(!importMetadata->ResolvedPathIsVirtual());
        // This function shouldn't lookup program-cache or lowdecl-cache since package fractions are merged before any
        // cache is stored.
        importMetadata->SetFile<ModuleKind::MODULE, false>(std::string(importMetadata->ResolvedSource()), DE());
        fraction = IntroduceProgram<ModuleKind::MODULE>(*importMetadata);
    } else {
        // remove a package-fraction that was mistakenly added as a module without enclosing package:
        auto &modules = GetGlobalProgram()->GetExternalSources()->Get<ModuleKind::MODULE>();
        auto newEndIt = std::remove(modules.begin(), modules.end(), fraction);
        modules.erase(newEndIt, modules.end());
    }

    const_cast<ImportMetadata *>(&fraction->GetImportMetadata())->LinkFractionMetadataToPackage(*package);

    fraction->SetPackageInfo(package->ModuleName(), ModuleKind::PACKAGE);
    ES2PANDA_ASSERT(fraction->ModuleName().find(package->ModuleName()) == 0);
    package->AppendFraction(fraction->As<ModuleKind::MODULE>());
}

parser::Program *ImportPathManager::LookupProgramCaches(const ImportMetadata &importData)
{
    if (Context()->globalContext == nullptr) {
        return nullptr;
    }
    parser::Program *cachedProg = nullptr;
    auto key = std::string(importData.Key());
    const auto &cachedExtProgs = Context()->globalContext->cachedExternalPrograms;
    if (cachedExtProgs.find(key) != cachedExtProgs.end()) {
        cachedProg = cachedExtProgs.at(key);
        ES2PANDA_ASSERT(key == cachedProg->GetImportMetadata().Key());
    } else if (auto *cachedStdlib = Context()->globalContext->stdLibAstCache; cachedStdlib != nullptr) {
        cachedStdlib->Visit<false>([&cachedProg, &key](auto *prog) {
            if (prog->GetImportMetadata().Key() == key) {
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
std::string ImportPathManager::FormEtscacheFilePath(const ImportMetadata &imd) const
{
    const auto &cacheDir = ArkTSConfig().CacheDir();
    ES2PANDA_ASSERT(!cacheDir.empty());
    ES2PANDA_ASSERT(!imd.ModuleName().empty());

    std::string declarationCacheFile = cacheDir;
    declarationCacheFile += util::PATH_DELIMITER;
    declarationCacheFile += imd.ModuleName();
    declarationCacheFile += CACHE_SUFFIX;

    return declarationCacheFile;
}

class EtscacheFileLock {
public:
    NO_MOVE_SEMANTIC(EtscacheFileLock);
    NO_COPY_SEMANTIC(EtscacheFileLock);
    EtscacheFileLock(const std::string &dstDeclPath, const std::string &srcAbcPath)
        : dstPath_(dstDeclPath), abcPath_(srcAbcPath)
    {
        if ((!os::IsFileExists(dstPath_) || ShouldRewrite(abcPath_, dstPath_))) {
            writer_ = ExlusiveFileWriter::Open(dstPath_);
        } else {
            ExlusiveFileWriter::WaitUnlockForRead(dstPath_);
        }
    }

    void WriteEtscacheFile(std::string_view text) const
    {
        ES2PANDA_ASSERT(bool(writer_));
        writer_->Write(std::move(text));
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
            auto const fd = ::open(filename.c_str(), O_RDONLY);
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
            FlockTrace(filename, "Open for write");
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
            auto const fd = ::open(filename.c_str(), O_WRONLY | O_CREAT, CHMOD);
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

        void Write(std::string_view text) const
        {
            FlockTrace("- Write");
#ifdef PANDA_TARGET_WINDOWS
            DWORD bytesWritten = 0;
            if (!::WriteFile(fd_, text.data(), text.size(), &bytesWritten, NULL)) {
                std::cerr << "Error writing to the file '" << filename_ << "': " << GetErrorMessage(::GetLastError())
                          << std::endl;
            }
#else
            if (::write(fd_, text.data(), text.size()) == -1) {
                std::cerr << "Error writing to the file '" << filename_ << "': " << ::strerror(errno) << std::endl;
            }
#endif
        }

        ~ExlusiveFileWriter()
        {
            FlockTrace("Close");
#ifdef PANDA_TARGET_WINDOWS
            ::SetEndOfFile(fd_);
            OVERLAPPED ov = {};
            if (!::UnlockFileEx(fd_, 0, MAXDWORD, MAXDWORD, &ov)) {
                std::cerr << "File truncate error '" << filename_ << "': " << GetErrorMessage(::GetLastError())
                          << std::endl;
            }
            ::CloseHandle(fd_);
#else
            auto const curPos = ::lseek(fd_, 0, SEEK_CUR);
            if (::ftruncate(fd_, curPos) != 0) {
                std::cerr << "File truncate error '" << filename_ << "': " << ::strerror(errno) << std::endl;
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
    };

private:
    std::string dstPath_;
    std::string abcPath_;
    ExlusiveFileWriter::Pointer writer_ = {};
};

template <typename EtscachePathByMNameGetter>
static void UnpackAbc(DiagnosticEngine *de, const std::string &abcPath, const EtscachePathByMNameGetter &dstGetter)
{
    auto abc = panda_file::OpenPandaFile(abcPath);
    if (abc == nullptr) {
        de->LogDiagnostic(diagnostic::OPEN_FAILED, util::DiagnosticMessageParams {abcPath});
        return;
    }
    for (auto id : abc->GetExported()) {
        panda_file::File::EntityId classId(id);
        auto mname = ExtractMnameFromPandafile(*abc, classId);
        if (EtscacheFileLock lock {dstGetter(mname), abcPath}; lock.ShouldWriteDeclfile()) {
            std::stringstream ss;
            panda_file::ClassDataAccessor {*abc, classId}.EnumerateAnnotation(
                ImportPathManager::ANNOTATION_MODULE_DECLARATION.data(),
                [&abc, &ss](panda_file::AnnotationDataAccessor &annotationAccessor) {
                    auto elemDeclaration = annotationAccessor.GetElement(0);
                    auto valueDeclaration = elemDeclaration.GetScalarValue();
                    const auto idAnnoDeclaration = valueDeclaration.Get<panda_file::File::EntityId>();
                    ss << panda_file::StringDataToString(abc->GetStringData(idAnnoDeclaration));
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

void ImportPathManager::MaybeUnpackAbcAndEmplaceInCacheDir(const ImportMetadata &importMetadata)
{
    ES2PANDA_ASSERT(importMetadata.ResolvedPathIsVirtual());
    ES2PANDA_ASSERT(importMetadata.ReferencesABC());

    if (processedAbcFiles_.count(importMetadata.AbcPath()) == 0) {
        processedAbcFiles_.insert(importMetadata.AbcPath());
        UnpackAbc(DE(), importMetadata.AbcPath(), [this](std::string_view mname) {
            ImportMetadata imdMock {};
            imdMock.moduleName_ = std::string {mname};
            return FormEtscacheFilePath(imdMock);
        });
    }

    // Just unpack, will be loaded by LookupEtscacheFile.
    ES2PANDA_ASSERT(importMetadata.Text().Kind() == ModuleKind::UNKNOWN);
}

parser::Program *ImportPathManager::IntroduceProgram(const ImportMetadata &importMetadata)
{
    switch (importMetadata.Text().Kind()) {
        case ModuleKind::MODULE:
            return IntroduceProgram<ModuleKind::MODULE>(importMetadata);
        case ModuleKind::SOURCE_DECL:
            return IntroduceProgram<ModuleKind::SOURCE_DECL>(importMetadata);
        case ModuleKind::ETSCACHE_DECL:
            return IntroduceProgram<ModuleKind::ETSCACHE_DECL>(importMetadata);
        case ModuleKind::DECLLESS_DYNAMIC:
            return IntroduceProgram<ModuleKind::DECLLESS_DYNAMIC>(importMetadata);
        case ModuleKind::PACKAGE:
            return IntroduceProgram<ModuleKind::PACKAGE>(importMetadata);
        default: {
            ES2PANDA_ASSERT(DE()->IsAnyError());
            return nullptr;
        }
    }
}

template <ModuleKind KIND, typename VarBinderT>
parser::ProgramAdapter<KIND> *ImportPathManager::IntroduceProgram(const ImportMetadata &importMetadata)
{
    ES2PANDA_ASSERT(importMetadata.Text().Kind() == KIND);
    parser::ProgramAdapter<KIND> *newProg = nullptr;
    if constexpr (KIND == ModuleKind::PACKAGE) {
        newProg = RegisterSourcesForPackageFromGlobbedDirectory(importMetadata);
    } else {
        newProg = parser::Program::New<KIND, VarBinderT>(importMetadata, Context());
        ES2PANDA_ASSERT(!importMetadata.ModuleName().empty());
        newProg->SetPackageInfo(importMetadata.ModuleName(), KIND);
    }
    RegisterProgram(newProg);
    return newProg;
}

class ImportPathManager::ResolvedSources {
public:
    explicit ResolvedSources(ImportPathManager *ipm) : ipm_ {ipm} {}

    void Register(parser::Program *program, DiagnosticEngine *de)
    {
        ArenaString key {program->GetImportMetadata().Key()};
        ES2PANDA_ASSERT(progsByResolvedPath_.find(key) == progsByResolvedPath_.end());
        progsByResolvedPath_[key] = program;
        if (program->Is<ModuleKind::PACKAGE>()) {
            progsByResolvedPath_[ArenaString {program->ModuleName()}] = program;
        }

        // Handle clashing. Impl-progs are disallowed to clash, while decl-prog are allowed. The restriction is that
        // entities in clashing declarations shouldn't intersect.
        ArenaString mname {program->GetImportMetadata().ModuleName()};
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
            auto absPath1 = program->GetImportMetadata().TextSource();
            auto absPath2 = modulePrograms.implProg->GetImportMetadata().TextSource();
            de->LogDiagnostic(diagnostic::FILE_RUNTIME_NAME_CLASH,
                              util::DiagnosticMessageParams {absPath1, absPath2, mname});
        }
        modulePrograms.implProg = program;
        if (ipm_->GetGlobalProgram() != nullptr) {
            // replace registered at this point decl-progs with the impl-prog:
            auto &extProgramsDecls = ipm_->GetGlobalProgram()->GetExternalSources()->Get<ModuleKind::SOURCE_DECL>();
            auto extProgramsDeclsNewEnd = extProgramsDecls.end();
            for (auto *declProg : modulePrograms.declProgs) {
                progsByResolvedPath_.at(ArenaString {declProg->GetImportMetadata().Key()}) = program;
                extProgramsDeclsNewEnd = std::remove(extProgramsDecls.begin(), extProgramsDeclsNewEnd, declProg);
            }
            extProgramsDecls.erase(extProgramsDeclsNewEnd, extProgramsDecls.end());
        }
    }

    parser::Program *SearchResolved(const ImportMetadata &importMetadata) const
    {
        if (auto it = progsByResolvedPath_.find(importMetadata.Key()); it != progsByResolvedPath_.end()) {
            ES2PANDA_ASSERT(it->second != nullptr);
            return it->second;
        }
        return nullptr;
    }

    void MaybeAddToExternalSources(parser::Program *newProg, parser::Program::ExternalSources *extSources)
    {
        [[maybe_unused]] bool isPackageFraction =
            (newProg->ModuleInfo().kind == ModuleKind::PACKAGE) && newProg->Is<ModuleKind::MODULE>();
        ES2PANDA_ASSERT(!isPackageFraction);
        if (auto pointedProgram = SearchResolved(newProg->GetImportMetadata()); pointedProgram == newProg) {
            extSources->Add(newProg);
        } else {
            [[maybe_unused]] const auto &imd = newProg->GetImportMetadata();
            ES2PANDA_ASSERT((imd.Kind() == ModuleKind::SOURCE_DECL) || (imd.Kind() == ModuleKind::ETSCACHE_DECL));
            ES2PANDA_ASSERT(!pointedProgram->IsDeclarationModule());
        }
    }

    parser::PackageProgram *FixupPackageByFraction(parser::Program *fractionBeingParsed, ArenaString packageName)
    {
        if (progsByResolvedPath_.count(packageName) != 0) {
            // Already fixed.
            auto *pkg = progsByResolvedPath_.at(packageName)->As<ModuleKind::PACKAGE>();
            auto *pointeeProg = ipm_->SearchResolved(fractionBeingParsed->GetImportMetadata());
            if (pointeeProg == fractionBeingParsed) {
                ArenaString key {fractionBeingParsed->GetImportMetadata().Key()};
                progsByResolvedPath_[key] = pkg;
            }
            ES2PANDA_ASSERT(ipm_->SearchResolved(fractionBeingParsed->GetImportMetadata()) == pkg);
            return pkg;
        }

        const_cast<ImportMetadata *>(&fractionBeingParsed->GetImportMetadata())->moduleName_ = packageName;
        fractionBeingParsed->SetPackageInfo(packageName, util::ModuleKind::PACKAGE);
        ImportMetadata pkgMetadata {};
        pkgMetadata.resolvedSource_ = packageName;
        pkgMetadata.moduleName_ = packageName;
        pkgMetadata.SetText<ModuleKind::PACKAGE, false>(std::string(packageName), "");
        auto newPkg = ipm_->NewEmptyPackage(pkgMetadata);
        newPkg->AppendFraction(fractionBeingParsed->As<ModuleKind::MODULE>());

        // fixup externalSources:
        auto &modulePrograms = ipm_->GetGlobalProgram()->GetExternalSources()->Get<ModuleKind::MODULE>();
        auto newEndIt = std::remove(modulePrograms.begin(), modulePrograms.end(), fractionBeingParsed);
        if (newEndIt != modulePrograms.end()) {
            modulePrograms.erase(newEndIt, modulePrograms.end());
            ipm_->GetGlobalProgram()->GetExternalSources()->Add(newPkg);
        } else {
            ES2PANDA_ASSERT(ipm_->GetGlobalProgram() == fractionBeingParsed);
        }

        ES2PANDA_ASSERT(progsByResolvedPath_.find(packageName) == progsByResolvedPath_.end());
        progsByResolvedPath_.find(fractionBeingParsed->GetImportMetadata().Key())->second = newPkg;
        progsByResolvedPath_[packageName] = newPkg;
        ES2PANDA_ASSERT(ipm_->SearchResolved(fractionBeingParsed->GetImportMetadata()) == newPkg);
        return newPkg;
    }

private:
    ImportPathManager *ipm_ {};
    ArenaMap<ArenaString, parser::Program *, CompareByLength> progsByResolvedPath_;
    struct Module {
        parser::Program *implProg {};
        ArenaVector<parser::Program *> declProgs {};
    };
    ArenaMap<ArenaString, Module, CompareByLength> modules_;
};

void ImportPathManager::RegisterProgram(parser::Program *program)
{
    if (program == nullptr) {
        ES2PANDA_ASSERT(DE()->IsAnyError());
        return;
    }

    resolvedSources_.Register(program, DE());

    // Packages are "synthetic" programs (w/o text), so they can't be parsed.
    // Mind the difference with package-fraction programs, constituting packages.
    switch (program->GetModuleKind()) {
        case ModuleKind::PACKAGE:
        case ModuleKind::SIMULT_MAIN:
        case ModuleKind::DECLLESS_DYNAMIC:
            return;
        default: {
            bool isParsed = program->Ast() != nullptr;
            parseQueue_.emplace_back(ParseInfo {isParsed, program});
        }
    }
}

parser::Program *ImportPathManager::SearchResolved(const ImportMetadata &importMetadata) const
{
    return resolvedSources_.SearchResolved(importMetadata);
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

parser::Program *ImportPathManager::LookupCachesAndIntroduceProgram(ImportMetadata *importMetadata)
{
    ES2PANDA_ASSERT(SearchResolved(GetGlobalProgram()->GetImportMetadata()) != nullptr);
    // NOTE(dkofanov): This step (caching the result) is essential. It is so because:
    // 1. `es2panda` may call `GatherImportMetadata` at any lowering/plugin.
    // 2. Cache-update may occur after some lowering or even by another thread.
    // In order for a source-representation (i.e. "parser::Program") to be consistent during compilation routine, it
    // should always be resolved to the same program.
    if (auto resolved = SearchResolved(*importMetadata); resolved != nullptr) {
        return resolved;
    }

    auto *program = LookupProgramCaches(*importMetadata);
    if (program != nullptr) {
        ES2PANDA_ASSERT(program->Ast() != nullptr);
    } else {
        LookupMemCache(importMetadata);
        if (importMetadata->Text().Kind() == ModuleKind::UNKNOWN) {
            LookupDiskCache(importMetadata);
        }

        if (importMetadata->Text().Kind() == ModuleKind::UNKNOWN) {
            if (importMetadata->PointsToPackage()) {
                importMetadata->SetText<ModuleKind::PACKAGE, false>(std::string(importMetadata->Key()), "");
            } else {
                LookupSourceFile(importMetadata);
            }
        }

        program = IntroduceProgram(*importMetadata);
    }
    if (program != nullptr) {
        resolvedSources_.MaybeAddToExternalSources(program, GetGlobalProgram()->GetExternalSources());
    }
    return program;
}

void ImportPathManager::LookupMemCache(ImportMetadata *importMetadata)
{
    parser::DeclarationCache::GetFromCache(importMetadata);
}

void ImportPathManager::LookupDiskCache(ImportMetadata *importMetadata)
{
    ES2PANDA_ASSERT(importMetadata->Text().Kind() == ModuleKind::UNKNOWN);
    if (importMetadata->ReferencesABC()) {
        MaybeUnpackAbcAndEmplaceInCacheDir(*importMetadata);
    }
    LookupEtscacheFile(importMetadata);
}

void ImportPathManager::LookupEtscacheFile(ImportMetadata *importData)
{
    if (ArkTSConfig().CacheDir().empty()) {
        return;
    }
    auto cachefile = FormEtscacheFilePath(*importData);
    ES2PANDA_ASSERT(cachefile.find(ArkTSConfig().CacheDir()) == 0);
    if (!ark::os::file::File::IsRegularFile(cachefile)) {
        return;
    }

    importData->SetFile<ModuleKind::ETSCACHE_DECL>(cachefile, DE());
}

void ImportPathManager::LookupSourceFile(ImportMetadata *importMetadata)
{
    if (importMetadata->HasSpecifiedDeclPath() && !importMetadata->ReferencesABC()) {
        importMetadata->SetFile<ModuleKind::SOURCE_DECL>(std::string(importMetadata->DeclPath()), DE());
    } else if (Helpers::EndsWith(importMetadata->ResolvedSource(), D_ETS_SUFFIX)) {
        importMetadata->SetFile<ModuleKind::SOURCE_DECL>(std::string(importMetadata->ResolvedSource()), DE());
    } else if (importMetadata->Lang() != Language::Id::ETS) {
        importMetadata->SetText<ModuleKind::DECLLESS_DYNAMIC, false>(std::string(importMetadata->ResolvedSource()), "");
    } else {
        importMetadata->SetFile<ModuleKind::MODULE>(std::string(importMetadata->ResolvedSource()), DE());
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
    if (auto metaData = TryResolvePath(resolvedPathPrototype); !metaData.resolvedPath.empty()) {
        return metaData;
    }

    if (ark::os::file::File::IsRegularFile(resolvedPathPrototype)) {
        return {GetRealPath(resolvedPathPrototype)};
    }

    if (ark::os::file::File::IsDirectory(resolvedPathPrototype)) {
        return {GetRealPath(DirOrDirWithIndexFile(resolvedPathPrototype))};
    }

    for (const auto &extension : supportedExtensions) {
        auto pathWithExtension = resolvedPathPrototype + std::string(extension);
        if (ark::os::file::File::IsRegularFile(pathWithExtension)) {
            return {GetRealPath(pathWithExtension)};
        }
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
    if (!p1.empty() && ((p1.back() == '/') || (p1.back() == '.'))) {
        p1 = p1.substr(0, p1.size() - 1);
    }
    if (!p2.empty() && ((p2.front() == '/') || (p2.front() == '.'))) {
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
    if (!ipm.ArkTSConfig().Package().empty() && Helpers::StartsWith(resolvedSource, ipm.ArkTSConfig().BaseUrl())) {
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
                                                                            const ImportMetadata &imd)
{
    // ES2PANDA_ASSERT(( == '/') || (imd.ResolvedSource().find('\\') == std::string::npos));
    using MatcherT = std::optional<ArenaString> (*)(const ArkTsConfig &, std::string_view, const ImportPathManager &);
    std::vector<MatcherT> matchers {};

    // 1. Try 'dynamicPaths' (aka 'dependencies') field:
    auto dynamicPathMatcher = [](const ArkTsConfig &cfg, std::string_view resolvedSource,
                                 [[maybe_unused]] const ImportPathManager &ipm) {
        std::optional<ArenaString> res {};
        if (cfg.FindInDependencies(resolvedSource) != std::nullopt) {
            res = OhmurlToMname(resolvedSource);
        }
        return res;
    };
    matchers.emplace_back(dynamicPathMatcher);

    // 2. Try 'paths' field:
    auto pathsMatcher = [](const ArkTsConfig &cfg, std::string_view resolvedSource,
                           const ImportPathManager &ipm) -> std::optional<ArenaString> {
        for (auto const &[unitName, unitPaths] : cfg.Paths()) {
            auto it = std::find_if(unitPaths.begin(), unitPaths.end(), [resolvedSource](const auto &unitPath) {
                ES2PANDA_ASSERT(!unitPath.empty());
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
static std::optional<ArenaString> CheckSpecialModuleName(const ImportMetadata &imd)
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

static ArenaString FormModuleNameSolelyByAbsolutePath(const ImportPathManager &ipm, const ImportMetadata &imd)
{
    auto etsPath = ipm.Context()->config->options->GetEtsPath();
    auto absoluteEtsPath = util::Path(etsPath, ipm.Context()->allocator).GetAbsolutePath().Utf8();
    return OhmurlToMname(CheckAndRebaseOhmurl(ipm, imd.ResolvedSource(), absoluteEtsPath, ""));
}

static ArenaString FormModuleName(const ImportPathManager &ipm, const ImportMetadata &imd)
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

static void CheckModuleName(const ImportPathManager &ipm, const ImportMetadata &imd)
{
    CheckNoColonInName(imd.ModuleName(), ipm.Context()->diagnosticEngine);
    if (imd.ModuleName().empty()) {
        ipm.Context()->diagnosticEngine->LogDiagnostic(diagnostic::UNRESOLVED_MODULE,
                                                       DiagnosticMessageParams {imd.ResolvedSource()});
    }
}

ImportMetadata::ImportMetadata(const ImportPathManager &ipm, std::string_view resolvedSource, Language::Id lang,
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

ImportMetadata::ImportMetadata(const ImportMetadata &other)
{
    *this = other;
}

const ImportMetadata &ImportMetadata::operator=(const ImportMetadata &other)
{
    parser::DeclarationCache::CacheReference::operator=(other);
    resolvedSource_ = other.resolvedSource_;
    moduleName_ = other.moduleName_;
    extModuleData_ = other.extModuleData_;
    lang_ = other.lang_;

    SetKey(resolvedSource_);
    return *this;
}

std::string_view ImportMetadata::DeclPath() const
{
    return (extModuleData_ != nullptr) ? extModuleData_->Path() : std::string_view {};
}

std::string_view ImportMetadata::OhmUrl() const
{
    if ((extModuleData_ != nullptr) && !extModuleData_->OhmUrl().empty()) {
        return extModuleData_->OhmUrl();
    }
    if (ReferencesABC()) {
        return AbcPath();
    }

    return "";
}

bool ImportMetadata::HasSpecifiedDeclPath() const
{
    return !DeclPath().empty() && (DeclPath() != DUMMY_PATH);
}

bool ImportMetadata::IsValid() const
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
