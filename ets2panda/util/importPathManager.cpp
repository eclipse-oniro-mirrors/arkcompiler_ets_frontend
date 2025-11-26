/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "parser/context/parserContext.h"
#include "parser/ETSparser.h"
#include "parser/program/DeclarationCache.h"
#include "parser/program/program.h"
#include "ir/expressions/literals/stringLiteral.h"

#include "compiler/lowering/ets/declGenPhase.h"

#include "libarkfile/class_data_accessor-inl.h"
#include "libarkfile/file-inl.h"
#include "libarkbase/utils/logger.h"
#include "util/helpers.h"
#include <algorithm>
#include <chrono>
#include <utility>
#include <fcntl.h>

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
#include <synchapi.h>
#else
#include <sys/stat.h>
#include <semaphore.h>
#endif
namespace ark::es2panda::util {

constexpr size_t SUPPORTED_INDEX_FILES_SIZE = 8;

static bool IsCompatibleExtension(const std::string &extension)
{
    return extension == ImportPathManager::ETS_SUFFIX || extension == ".ts" || extension == ".sts";
}

static bool IsAbsolute(const std::string &path)
{
#ifndef ARKTSCONFIG_USE_FILESYSTEM
    return !path.empty() && path[0] == '/';
#else
    return fs::path(path).is_absolute();
#endif  // ARKTSCONFIG_USE_FILESYSTEM
}

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

std::string DeleteEscapeSymbols(const std::string &input)
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

void RemoveEscapedNewlines(std::string &s)
{
    std::string pattern = "\\n";
    size_t pos = 0;
    while ((pos = s.find(pattern, pos)) != std::string::npos) {
        s.erase(pos, pattern.size());
    }
}

ImportPathManager::ImportPathManager(const parser::ETSParser *parser)
    : parser_(parser),
      allocator_(parser->Allocator()),
      arktsConfig_(parser->GetOptions().ArkTSConfig()),
      absoluteEtsPath_(parser->GetOptions().GetEtsPath().empty()
                           ? ""
                           : util::Path(parser->GetOptions().GetEtsPath(), allocator_).GetAbsolutePath()),
      stdLib_(parser->GetOptions().GetStdlib()),
      parseList_(parser->Allocator()->Adapter()),
      globalProgram_(parser->GetProgram()),
      diagnosticEngine_ {parser->DiagnosticEngine()}
{
    arktsConfig_->GenerateSourcePathMap();
}

std::string ExtractModuleName(const panda_file::File &pf, const panda_file::File::EntityId &classId)
{
    // processing name to get ohmUrl
    std::string name = utf::Mutf8AsCString(pf.GetStringData(classId).data);
    auto type = pandasm::Type::FromDescriptor(name);
    type = pandasm::Type(type.GetNameWithoutRank(), type.GetRank());
    auto recordName = type.GetPandasmName();

    // rely on the following mangling: <moduleName>.ETSGLOBAL
    auto etsGlobalSuffix = std::string(".") + std::string(compiler::Signatures::ETS_GLOBAL);
    ES2PANDA_ASSERT(Helpers::EndsWith(recordName, etsGlobalSuffix));
    return recordName.substr(0, recordName.size() - etsGlobalSuffix.size());  // moduleName
}

std::chrono::system_clock::time_point GetFileCreationTime([[maybe_unused]] const std::string &filePath)
{
    // hack for builds when no filesystem is included
#ifdef USE_UNIX_SYSCALL
    return std::chrono::system_clock::now();
#else
    auto ftime = fs::last_write_time(filePath);
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    return sctp;
#endif
}

bool ImportPathManager::DeclarationIsInCache([[maybe_unused]] ImportMetadata &importData,
                                             [[maybe_unused]] bool isStdlib)
{
#ifdef USE_UNIX_SYSCALL
    // hack for builds when no filesystem is included
    return false;
#else
    auto resSource = std::string(importData.resolvedSource);
    auto it = arktsConfig_->Dependencies().find(std::string(importData.resolvedSource));
    ES2PANDA_ASSERT(it != arktsConfig_->Dependencies().cend());
    const auto &externalModuleImportData = it->second;
    std::replace(resSource.begin(), resSource.end(), '/', '.');
    auto fileNameToCheck = arktsConfig_->CacheDir() + "/" + resSource + CACHE_SUFFIX.data();
    // memory cache checking
    if (parser::DeclarationCache::GetFromCache(fileNameToCheck) != parser::DeclarationCache::ABSENT) {
        importData.declPath = util::UString(fileNameToCheck, allocator_).View().Utf8();
        importData.importFlags &= ~ImportFlags::EXTERNAL_BINARY_IMPORT;
        importData.importFlags |= ImportFlags::EXTERNAL_SOURCE_IMPORT;
        return true;
    }
    if (!isStdlib) {
        return false;
    }

    // since was not found in memory cache, take from disk cache
    if (!GetCacheCanBeUpdated()) {
        return false;
    }
    if (fs::exists(fileNameToCheck) && fs::is_regular_file(fileNameToCheck)) {
        if (fs::file_size(fileNameToCheck) == 0) {
            return false;
        }
        // means that etsstdlib.abc file was NOT updated, so we DO NOT need to update declarations, and can cache
        // existed files
        if (GetFileCreationTime(fileNameToCheck) > GetFileCreationTime(std::string(externalModuleImportData.Path()))) {
            importData.declPath = util::UString(fileNameToCheck, allocator_).View().Utf8();
            importData.importFlags |= ImportFlags::EXTERNAL_SOURCE_IMPORT;
            importData.ohmUrl = util::UString(resSource, allocator_).View().Utf8();
            std::ifstream declFile(fileNameToCheck);
            std::stringstream ss {};
            ss << declFile.rdbuf();
            parser::DeclarationCache::CacheIfPossible(std::move(fileNameToCheck),
                                                      std::make_shared<std::string>(ss.str()));
            return true;
        }
        // means that etsstdlib.abc file was updated, so we need to update declarations
        // and block futher disk cache checking
        SetCacheCannotBeUpdated();
        for (const auto &entry : fs::directory_iterator(arktsConfig_->CacheDir())) {
            fs::remove_all(entry.path());
        }
    }
    return false;
#endif
}

void ImportPathManager::ProcessExternalLibraryImportSimple(ImportMetadata &importData)
{
    // take the classes that contain ModuleDeclaration annotation only
    auto it = arktsConfig_->Dependencies().find(std::string(importData.resolvedSource));
    ES2PANDA_ASSERT(it != arktsConfig_->Dependencies().cend());
    const auto &externalModuleImportData = it->second;

    if (DeclarationIsInCache(importData, false)) {
        return;
    }

    importData.importFlags |= ImportFlags::EXTERNAL_BINARY_IMPORT;

    auto pf = panda_file::OpenPandaFile(std::string {externalModuleImportData.Path()});
    if (!pf) {
        diagnosticEngine_.LogDiagnostic(diagnostic::OPEN_FAILED,
                                        util::DiagnosticMessageParams {externalModuleImportData.Path()});
    }

    ES2PANDA_ASSERT(pf->GetExported().size() == 1);

    for (auto id : pf->GetExported()) {
        panda_file::File::EntityId classId(id);
        panda_file::ClassDataAccessor cda(*pf, classId);

        // processing annotation to extract string with declaration text
        auto success =
            cda.EnumerateAnnotation(ANNOTATION_MODULE_DECLARATION.data(),
                                    [&importData, &pf, this](panda_file::AnnotationDataAccessor &annotationAccessor) {
                                        auto elem = annotationAccessor.GetElement(0);
                                        auto value = elem.GetScalarValue();
                                        const auto idAnno = value.Get<panda_file::File::EntityId>();
                                        std::stringstream ss;
                                        ss << panda_file::StringDataToString(pf->GetStringData(idAnno));
                                        std::string declText = ss.str();
                                        if (!declText.empty()) {
                                            RemoveEscapedNewlines(declText);
                                            importData.declText = util::UString(declText, allocator_).View().Utf8();
                                            return true;
                                        }
                                        return false;
                                    });
        if (!success) {
            return;
        }
        // processing name to get ohmUrl
        importData.ohmUrl = util::UString(ExtractModuleName(*pf, classId), allocator_).View().Utf8();
    }
}

#ifdef PANDA_TARGET_WINDOWS
std::wstring s2ws(const std::string &str)
{
    int sizeNeeded = MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstrTo(sizeNeeded, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), (int)str.size(), &wstrTo[0], sizeNeeded);
    return wstrTo;
}

static void CreateDeclarationFileWindows(const std::string &processed, const std::string &absDecl)
{
    std::string semName = "/decl_sem_" + fs::path(absDecl).filename().string();
    std::wstring semNameWide = s2ws(semName);
    LPCWSTR lpwcstr = semNameWide.c_str();
    HANDLE sem = CreateSemaphoreExW(nullptr, 1, 1, lpwcstr, 0, SEMAPHORE_MODIFY_STATE | SYNCHRONIZE);
    if (!sem) {
        LOG(FATAL, ES2PANDA) << "Unexpected error while creating declaration file: " << absDecl;
        return;
    }
    DWORD waitRes = WaitForSingleObject(sem, INFINITE);
    if (waitRes != WAIT_OBJECT_0) {
        LOG(FATAL, ES2PANDA) << "Unexpected error while creating declaration file: " << absDecl;
        CloseHandle(sem);
        return;
    }
    HANDLE fd =
        CreateFile(absDecl.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fd == INVALID_HANDLE_VALUE) {
        ReleaseSemaphore(sem, 1, nullptr);
        CloseHandle(sem);
        return;
    }
    DWORD written;
    if (!WriteFile(fd, processed.data(), processed.size(), &written, NULL)) {
        LOG(FATAL, ES2PANDA) << "Failed to write a file for declaration: " << absDecl;
    }
    CloseHandle(fd);

    ReleaseSemaphore(sem, 1, nullptr);
    CloseHandle(sem);
}
#else
#ifndef USE_UNIX_SYSCALL
#include <sys/file.h>
static void CreateDeclarationFileLinux(const std::string &processed, const std::string &absDecl)
{
    std::string semName = "/decl_sem_" + fs::path(absDecl).filename().string();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,readability-magic-numbers)
    sem_t *sem = sem_open(semName.c_str(), O_CREAT, 0644, 1);
    sem_wait(sem);
    if (sem == SEM_FAILED) {
        LOG(FATAL, ES2PANDA) << "Unexpected error while creating declaration file: " << absDecl;
        return;
    }
    auto flags = O_CREAT | O_WRONLY | O_EXCL;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,readability-magic-numbers)
    int fd = open(absDecl.c_str(), flags, 0644);
    if (fd == -1) {
        sem_post(sem);
        sem_close(sem);
        sem_unlink(semName.c_str());
        return;
    }
    flock(fd, LOCK_EX);

    int res = write(fd, processed.data(), processed.size());
    if (res == -1) {
        LOG(FATAL, ES2PANDA) << "Failed to write a file for declaration: " << absDecl;
    }
    int fdres = fsync(fd);
    if (fdres == -1) {
        LOG(FATAL, ES2PANDA) << "Failed to sync a file for declaration: " << absDecl;
    }
    flock(fd, LOCK_UN);
    close(fd);
    sem_post(sem);
    sem_close(sem);
    sem_unlink(semName.c_str());
}
#endif
#endif

void CreateDeclarationFile([[maybe_unused]] const std::string &declFileName,
                           [[maybe_unused]] const std::string &processed)
{
#ifdef USE_UNIX_SYSCALL
    return;
#else
    const std::string absDecl = fs::absolute(declFileName).string();
    fs::create_directories(fs::path(absDecl).parent_path());
#ifdef PANDA_TARGET_WINDOWS
    CreateDeclarationFileWindows(processed, absDecl);
#else
    CreateDeclarationFileLinux(processed, absDecl);
#endif
#endif
}

void ImportPathManager::ProcessExternalLibraryImportFromEtsstdlib(ImportMetadata &importData,
                                                                  const std::string_view &externalModuleImportData)
{
    auto pf = panda_file::OpenPandaFile(std::string {externalModuleImportData});
    if (!pf) {
        diagnosticEngine_.LogDiagnostic(diagnostic::OPEN_FAILED,
                                        util::DiagnosticMessageParams {externalModuleImportData});
    }
    ES2PANDA_ASSERT(pf->GetExported().size() > 1);  // currently works for etsstdlib only
    // need to split for several d.ets. Currently works only for etsstdlib.abc
    for (auto id : pf->GetExported()) {
        panda_file::File::EntityId classId(id);
        panda_file::ClassDataAccessor cda(*pf, classId);
        auto moduleName = ExtractModuleName(*pf, classId);
        auto ohmUrl = moduleName;

        // create a name for d.ets file - related on module name and cache dir
        auto declFileName = arktsConfig_->CacheDir() + "/" + moduleName + CACHE_SUFFIX.data();
        // the module name`s separators are '.' now, but for resolvedSource we have '/'
        std::replace(moduleName.begin(), moduleName.end(), '.', '/');
        if (importData.resolvedSource != moduleName) {
            // if the current resolvedSource is not the same as current class, just go to the next class
            continue;
        }

        importData.ohmUrl = util::UString(ohmUrl, allocator_).View().Utf8();

        std::stringstream ss;
        // a class can contain more than one ModuleAnnotation annotation,
        // so we need to search in all of them in order to take all the declarations
        cda.EnumerateAnnotations([&pf, &ss](panda_file::File::EntityId annId) {
            panda_file::AnnotationDataAccessor annotationAccessor(*pf, annId);
            auto annName =
                std::string(ark::utf::Mutf8AsCString(pf->GetStringData(annotationAccessor.GetClassId()).data));
            if (annName == ANNOTATION_MODULE_DECLARATION.data()) {
                auto elem = annotationAccessor.GetElement(0);
                auto value = elem.GetScalarValue();
                const auto idAnno = value.Get<panda_file::File::EntityId>();
                // collecting the string values of annotations
                ss << panda_file::StringDataToString(pf->GetStringData(idAnno));
            }
            return true;
        });
        if (!ss.str().empty()) {
            importData.importFlags |= ImportFlags::EXTERNAL_SOURCE_IMPORT;
            importData.declPath = util::UString(declFileName, allocator_).View().Utf8();
            importData.ohmUrl = util::UString(ohmUrl, allocator_).View().Utf8();

            const std::string processed = DeleteEscapeSymbols(ss.str());
            CreateDeclarationFile(declFileName, processed);
        }
        return;  // if we reach this line, we already took that one class and created that one d.ets, no need to
                 // continue
    }
}

void ImportPathManager::ProcessExternalLibraryImport(ImportMetadata &importData)
{
    auto resSource = std::string(importData.resolvedSource);
    ES2PANDA_ASSERT(!IsAbsolute(resSource));
    auto it = arktsConfig_->Dependencies().find(resSource);
    ES2PANDA_ASSERT(it != arktsConfig_->Dependencies().cend());
    const auto &externalModuleImportData = it->second;
    importData.lang = externalModuleImportData.GetLanguage().GetId();

    // process .d.ets "path" in "dependencies"
    // process empty "path" in dependencies, since in interop we allow imports without typecheck
    if (!Helpers::EndsWith(std::string(externalModuleImportData.Path()), ABC_SUFFIX)) {
        importData.importFlags |= ImportFlags::EXTERNAL_SOURCE_IMPORT;
        importData.ohmUrl = util::UString(externalModuleImportData.OhmUrl(), allocator_).View().Utf8();
        importData.declPath = externalModuleImportData.Path();
        return;
    }

    if (arktsConfig_->CacheDir().empty()) {
        diagnosticEngine_.LogDiagnostic(diagnostic::NO_CACHE_DIRECTORY,
                                        util::DiagnosticMessageParams {util::StringView(arktsConfig_->ConfigPath())});
        return;
    }

    // process .abc "path" in "dependencies"
    ES2PANDA_ASSERT(Helpers::EndsWith(std::string(externalModuleImportData.Path()), ABC_SUFFIX));

    if (!Helpers::EndsWith(std::string(externalModuleImportData.Path()), ETSSTDLIB_ABC_SUFFIX)) {
        // currently only two modes are supported:
        // 1. import from .abc file with one package (simple case)
        // 2. import from etstdlib.abc
        // so, for this case, if it is not etsstdlib.abc -> handle simple case
        importData.declPath = externalModuleImportData.Path();
        return ProcessExternalLibraryImportSimple(importData);
    }
    {
        std::scoped_lock<std::shared_mutex> processStdlib(m_);
        // trying to find declaration in memory and disk caches
        if (DeclarationIsInCache(importData, true)) {
            return;
        }

        ProcessExternalLibraryImportFromEtsstdlib(importData, externalModuleImportData.Path());
    }
}

// If needed, the result of this function can be cached
std::string_view ImportPathManager::TryImportFromDeclarationCache(std::string_view resolvedImportPath) const
{
    // if package or unresolved file, just skip
    if (ark::os::file::File::IsDirectory(std::string(resolvedImportPath)) ||
        !ark::os::file::File::IsRegularFile(std::string(resolvedImportPath))) {
        return resolvedImportPath;
    }
    const auto &rootDir = ArkTSConfig()->RootDir();
    const auto &cacheDir = ArkTSConfig()->CacheDir();
    // if already in cache, return
    if (Helpers::StartsWith(resolvedImportPath, cacheDir)) {
        return resolvedImportPath;
    }
    if (cacheDir.empty() || rootDir.empty()) {
        return resolvedImportPath;
    }
    // declaration cache is used only for .ets files, located in the same application as compiling file
    if (!Helpers::EndsWith(resolvedImportPath, ETS_SUFFIX) || !Helpers::StartsWith(resolvedImportPath, rootDir)) {
        return resolvedImportPath;
    }
    const auto &relativeFilePath =
        resolvedImportPath.substr(rootDir.size(), resolvedImportPath.size() - rootDir.size());
    const auto &declarationCacheFile =
        cacheDir + std::string(relativeFilePath.substr(0, relativeFilePath.size() - ETS_SUFFIX.size())) +
        CACHE_SUFFIX.data();

    if (!ark::os::file::File::IsRegularFile(declarationCacheFile)) {
        return resolvedImportPath;
    }
    return UString(declarationCacheFile, allocator_).View().Utf8();
}

ImportPathManager::ImportMetadata ImportPathManager::GatherImportMetadata(parser::Program *program,
                                                                          ImportFlags importFlags,
                                                                          ir::StringLiteral *importPath)
{
    srcPos_ = importPath->Start();
    // NOTE(dkofanov): The code below expresses the idea of 'dynamicPaths' defining separated, virtual file system.
    // Probably, paths of common imports should be isolated from the host fs as well, being resolved by 'ModuleInfo'
    // instead of 'AbsoluteName'.
    isDynamic_ = program->IsDeclForDynamicStaticInterop();
    auto curModulePath = isDynamic_ ? program->ModuleInfo().moduleName : program->AbsoluteName();
    auto [resolvedImportPath, resolvedIsExternalModule] = ResolvePath(curModulePath.Utf8(), importPath);
    if (resolvedImportPath.empty()) {
        ES2PANDA_ASSERT(diagnosticEngine_.IsAnyError());
        return ImportMetadata {util::ImportFlags::NONE, Language::Id::COUNT, ERROR_LITERAL};
    }

    globalProgram_->AddFileDependencies(std::string(curModulePath), std::string(resolvedImportPath));

    ImportMetadata importData {importFlags};
    importData.resolvedSource = resolvedImportPath;
    if (resolvedIsExternalModule) {
        ProcessExternalLibraryImport(importData);
    } else {
        importData.lang = ToLanguage(program->Extension()).GetId();
        importData.declPath = util::ImportPathManager::DUMMY_PATH;
        importData.ohmUrl = util::ImportPathManager::DUMMY_PATH;
    }

    if (!parser_->HasParserStatus(parser::ParserStatus::DEPENDENCY_ANALYZER_MODE)) {
        importData.resolvedSource = TryImportFromDeclarationCache(importData.resolvedSource);
    }

    if (globalProgram_->AbsoluteName() != resolvedImportPath) {
        AddToParseList(importData);
    }

    LOG(DEBUG, ES2PANDA) << "[" << curModulePath << "] "
                         << "Import " << importPath->ToString() << " resolved to " << importData.resolvedSource;
    return importData;
}

static bool IsRelativePath(std::string_view path)
{
    std::string currentDirReferenceLinux = "./";
    std::string parentDirReferenceLinux = "../";
    std::string currentDirReferenceWindows = ".\\";
    std::string parentDirReferenceWindows = "..\\";

    return ((path.find(currentDirReferenceLinux) == 0) || (path.find(parentDirReferenceLinux) == 0) ||
            (path.find(currentDirReferenceWindows) == 0) || (path.find(parentDirReferenceWindows) == 0));
}

util::StringView ImportPathManager::ResolvePathAPI(StringView curModulePath, ir::StringLiteral *importPath) const
{
    srcPos_ = importPath->Start();
    // NOTE(dkofanov): #23698 related. In case of 'dynamicPaths', resolved path is "virtual" module-path, may be not
    // what the plugin expecting.
    // NOTE(itrubachev) import path manager should be refactored
    auto resolvedPath = ResolvePath(curModulePath.Utf8(), importPath).resolvedPath;
    auto cachedResolvedPath = TryImportFromDeclarationCache(resolvedPath);
    return cachedResolvedPath;
}

void ImportPathManager::TryMatchStaticResolvedPath(ImportPathManager::ResolvedPathRes &result) const
{
    auto paths = arktsConfig_->Paths().find(result.resolvedPath.data());
    if (paths != arktsConfig_->Paths().cend()) {
        result.resolvedPath = *paths->second.begin();
        result.resolvedIsExternalModule = false;
    }
}

void ImportPathManager::TryMatchDynamicResolvedPath(ImportPathManager::ResolvedPathRes &result) const
{
    auto packagePathPair = arktsConfig_->SourcePathMap().find(result.resolvedPath);
    if (packagePathPair != arktsConfig_->SourcePathMap().cend()) {
        result.resolvedPath = packagePathPair->second;
        result.resolvedIsExternalModule = true;
    }
}

ImportPathManager::ResolvedPathRes ImportPathManager::ResolvePath(std::string_view curModulePath,
                                                                  ir::StringLiteral *importPath) const
{
    if (importPath->Str().Empty()) {
        diagnosticEngine_.LogDiagnostic(diagnostic::EMPTY_IMPORT_PATH, util::DiagnosticMessageParams {});
        return {*importPath};
    }
    ResolvedPathRes result {};
    if (IsRelativePath(*importPath)) {
        size_t pos = curModulePath.find_last_of("/\\");
        auto currentDir = (pos != std::string::npos) ? curModulePath.substr(0, pos) : ".";

        auto resolvedPath = UString(currentDir, allocator_);
        resolvedPath.Append(pathDelimiter_);
        resolvedPath.Append(*importPath);

        result = AppendExtensionOrIndexFileIfOmitted(resolvedPath.View());
        if (result.resolvedIsExternalModule) {
            TryMatchStaticResolvedPath(result);
        } else {
            TryMatchDynamicResolvedPath(result);
        }
    } else {
        result = ResolveAbsolutePath(*importPath);
    }

    return result;
}

ImportPathManager::ResolvedPathRes ImportPathManager::ResolveAbsolutePath(const ir::StringLiteral &importPathNode) const
{
    std::string_view importPath {importPathNode};
    ES2PANDA_ASSERT(!IsRelativePath(importPath));

    if (importPath.at(0) == pathDelimiter_.at(0)) {
        std::string baseUrl = arktsConfig_->BaseUrl();
        baseUrl.append(importPath, 0, importPath.length());

        return AppendExtensionOrIndexFileIfOmitted(UString(baseUrl, allocator_).View());
    }

    const size_t pos = importPath.find_first_of("/\\");
    bool containsDelim = (pos != std::string::npos);
    auto rootPart = containsDelim ? importPath.substr(0, pos) : importPath;
    if (!stdLib_.empty() &&
        ((rootPart == "std") || (rootPart == "escompat"))) {  // Get std or escompat path from CLI if provided
        auto baseUrl = std::string(GetRealPath(StringView(stdLib_))) + pathDelimiter_.at(0) + std::string(rootPart);

        if (containsDelim) {
            baseUrl.append(1, pathDelimiter_.at(0));
            baseUrl.append(importPath, rootPart.length() + 1, importPath.length());
        }
        return {UString(baseUrl, allocator_).View().Utf8()};
    }

    ES2PANDA_ASSERT(arktsConfig_ != nullptr);
    auto resolvedPath = arktsConfig_->ResolvePath(importPath, isDynamic_);
    if (!resolvedPath) {
        diagnosticEngine_.LogDiagnostic(
            diagnostic::IMPORT_CANT_FIND_PREFIX,
            util::DiagnosticMessageParams {util::StringView(importPath), util::StringView(arktsConfig_->ConfigPath())},
            srcPos_);
        return {""};
    }
    return AppendExtensionOrIndexFileIfOmitted(UString(resolvedPath.value(), allocator_).View());
}

#ifdef USE_UNIX_SYSCALL
void ImportPathManager::UnixWalkThroughDirectoryAndAddToParseList(const ImportMetadata importMetadata)
{
    const auto directoryPath = std::string(importMetadata.resolvedSource);
    DIR *dir = opendir(directoryPath.c_str());
    if (dir == nullptr) {
        diagnosticEngine_.LogDiagnostic(diagnostic::OPEN_FOLDER_FAILED, util::DiagnosticMessageParams {directoryPath},
                                        srcPos_);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        std::string fileName = entry->d_name;
        std::string::size_type pos = fileName.find_last_of('.');
        if (pos == std::string::npos || !IsCompatibleExtension(fileName.substr(pos))) {
            continue;
        }

        std::string filePath = directoryPath + "/" + entry->d_name;
        auto globElemImportMetadata = importMetadata;
        globElemImportMetadata.resolvedSource = UString(filePath, allocator_).View().Utf8();
        AddToParseList(globElemImportMetadata);
    }

    closedir(dir);
    return;
}
#endif

void ImportPathManager::AddImplicitPackageImportToParseList(StringView packageDir, const lexer::SourcePosition &srcPos)
{
    srcPos_ = srcPos;
    ES2PANDA_ASSERT(
        IsAbsolute(packageDir.Mutf8()));  // This should be an absolute path for 'AddToParseList' be able to resolve it.
    auto importMetadata = ImportMetadata {util::ImportFlags::IMPLICIT_PACKAGE_IMPORT, Language::Id::ETS,
                                          packageDir.Utf8(), util::ImportPathManager::DUMMY_PATH};
    AddToParseList(importMetadata);
}

void ImportPathManager::AddToParseList(const ImportMetadata &importMetadata)
{
    auto resolvedPath = importMetadata.resolvedSource;
    bool isDeclForDynamic = !IsAbsolute(std::string(resolvedPath));  // Avoiding interpreting dynamic-path as directory.
    if (!isDeclForDynamic && ark::os::file::File::IsDirectory(std::string(resolvedPath))) {
#ifdef USE_UNIX_SYSCALL
        UnixWalkThroughDirectoryAndAddToParseList(importMetadata);
#else
        for (auto const &entry : fs::directory_iterator(std::string(resolvedPath))) {
            if (!fs::is_regular_file(entry) || !IsCompatibleExtension(entry.path().extension().string())) {
                continue;
            }
            auto globElemImportMetadata = importMetadata;
            globElemImportMetadata.resolvedSource = UString(entry.path().string(), allocator_).View().Utf8();
            AddToParseList(globElemImportMetadata);
        }
        return;
#endif
    }

    // Check if file has been already added to parse list
    if (const auto &found = std::find_if(
            // CC-OFFNXT(G.FMT.06) project code style
            parseList_.begin(), parseList_.end(),
            [&resolvedPath](const ParseInfo &info) { return (info.importData.resolvedSource == resolvedPath); });
        found != parseList_.end()) {
        // The 'parseList_' can contain at most 1 record with the same source file path (else it'll break things).
        //
        // If a file is added as implicit package imported before, then we may add it again without the implicit import
        // directive (and remove the other one), to handle when an implicitly package imported file explicitly imports
        // it. Re-parsing it is necessary, because if the implicitly package imported file contains a syntax error, then
        // it'll be ignored, but we must not ignore it if an explicitly imported file contains a parse error. Also this
        // addition can happen during parsing the files in the parse list, so re-addition is necessary in order to
        // surely re-parse it.
        //
        // If a file was already not implicitly package imported, then it's just a duplicate, return
        if (!found->importData.IsImplicitPackageImported() || importMetadata.IsImplicitPackageImported()) {
            return;
        }

        parseList_.erase(found);
    }

    if (!isDeclForDynamic && !ark::os::file::File::IsRegularFile(std::string(resolvedPath))) {
        diagnosticEngine_.LogDiagnostic(diagnostic::UNAVAILABLE_SRC_PATH, util::DiagnosticMessageParams {resolvedPath},
                                        srcPos_);
        return;
    }

    // 'Object.ets' must be the first in the parse list
    // NOTE (mmartin): still must be the first?
    const std::size_t position = resolvedPath.find_last_of("/\\");
    const bool isDefaultImport = (importMetadata.importFlags & ImportFlags::DEFAULT_IMPORT) != 0;
    const auto parseInfo = ParseInfo {false, importMetadata};
    if (isDefaultImport && (resolvedPath.substr(position + 1, resolvedPath.length()) == "Object.ets")) {
        parseList_.emplace(parseList_.begin(), parseInfo);
    } else {
        parseList_.emplace_back(parseInfo);
    }
}

void ImportPathManager::MarkAsParsed(std::string_view const path) noexcept
{
    for (auto &parseInfo : parseList_) {
        if (parseInfo.importData.resolvedSource == path) {
            parseInfo.isParsed = true;
            return;
        }
    }
}

StringView ImportPathManager::GetRealPath(StringView path) const
{
    const std::string realPath = ark::os::GetAbsolutePath(path.Mutf8());
    if (realPath.empty() || realPath == path.Mutf8()) {
        return path;
    }

    return UString(realPath, allocator_).View();
}

std::string ImportPathManager::TryMatchDependencies(std::string_view fixedPath) const
{
    // Probably, 'NormalizePath' should be moved to 'AppendExtensionOrIndexFileIfOmitted'.
    auto normalizedPath = ark::os::NormalizePath(std::string(fixedPath));
    std::replace_if(
        normalizedPath.begin(), normalizedPath.end(), [&](auto &c) { return c == pathDelimiter_[0]; }, '/');
    // NOTE(dkofanov): #23877. See also 'arktsconfig.cpp'.
    if (arktsConfig_->Dependencies().find(normalizedPath) != arktsConfig_->Dependencies().cend()) {
        return normalizedPath;
    }
    return {};
}

ImportPathManager::ResolvedPathRes ImportPathManager::TryResolvePath(std::string_view fixedPath) const
{
    auto normalizedPath = ark::os::NormalizePath(std::string(fixedPath));
    std::replace_if(
        normalizedPath.begin(), normalizedPath.end(), [&](auto &c) { return c == pathDelimiter_[0]; }, '/');
    if (arktsConfig_->Dependencies().find(normalizedPath) != arktsConfig_->Dependencies().cend()) {
        return {UString(normalizedPath, allocator_).View().Utf8(), true};
    }
    if (arktsConfig_->Paths().find(normalizedPath) != arktsConfig_->Paths().cend()) {
        return {UString(normalizedPath, allocator_).View().Utf8(), false};
    }
    return {{}, false};
}

std::string_view ImportPathManager::DirOrDirWithIndexFile(StringView dir) const
{
    // Supported index files: keep this checking order
    std::array<std::string, SUPPORTED_INDEX_FILES_SIZE> supportedIndexFiles = {
        "index.ets", "index.sts", "index.ts", "index.d.ets", "Index.ets", "Index.sts", "Index.ts", "Index.d.ets"};
    for (const auto &indexFile : supportedIndexFiles) {
        std::string indexFilePath = dir.Mutf8() + ark::os::file::File::GetPathDelim().at(0) + indexFile;
        if (ark::os::file::File::IsRegularFile(indexFilePath)) {
            return GetRealPath(UString(indexFilePath, allocator_).View()).Utf8();
        }
    }

    return dir.Utf8();
}
// NOTE(dkofanov): Be cautious: potentially no-op and may retrun the input string view. Make sure 'basePath' won't go
// out of scope.
ImportPathManager::ResolvedPathRes ImportPathManager::AppendExtensionOrIndexFileIfOmitted(StringView basePath) const
{
    auto fixedPath = basePath.Mutf8();
    char delim = pathDelimiter_.at(0);
    std::replace_if(
        fixedPath.begin(), fixedPath.end(), [&](auto &c) { return ((delim != c) && ((c == '\\') || (c == '/'))); },
        delim);
    if (auto metaData = TryResolvePath(fixedPath); !metaData.resolvedPath.empty()) {
        return metaData;
    }

    auto path = UString(fixedPath, allocator_).View();
    StringView realPath = GetRealPath(path);
    if (ark::os::file::File::IsRegularFile(realPath.Mutf8())) {
        return {realPath.Utf8()};
    }

    if (ark::os::file::File::IsDirectory(realPath.Mutf8())) {
        return {DirOrDirWithIndexFile(realPath)};
    }

    for (const auto &extension : supportedExtensions) {
        if (ark::os::file::File::IsRegularFile(path.Mutf8() + std::string(extension))) {
            return {GetRealPath(UString(path.Mutf8().append(extension), allocator_).View()).Utf8()};
        }
    }

    diagnosticEngine_.LogDiagnostic(diagnostic::UNSUPPORTED_PATH,
                                    util::DiagnosticMessageParams {util::StringView(path.Mutf8())}, srcPos_);
    return {""};
}

static std::string FormUnitName(std::string_view name)
{
    // this policy may change
    return std::string(name);
}

// Transform /a/b/c.ets to a.b.c
static std::string FormRelativeModuleName(std::string relPath)
{
    bool isMatched = false;
    for (const auto &ext : ImportPathManager::supportedExtensionsInversed) {
        if (relPath.size() >= ext.size() && relPath.compare(relPath.size() - ext.size(), ext.size(), ext) == 0) {
            relPath = relPath.substr(0, relPath.size() - ext.size());
            isMatched = true;
            break;
        }
    }
    if (relPath.empty()) {
        return "";
    }

    if (!isMatched) {
        ASSERT_PRINT(false, "Invalid relative filename: " + relPath);
    }
    while (relPath[0] == util::PATH_DELIMITER) {
        relPath = relPath.substr(1);
    }
    std::replace(relPath.begin(), relPath.end(), util::PATH_DELIMITER, '.');
    return relPath;
}

util::StringView ImportPathManager::FormModuleNameSolelyByAbsolutePath(const util::Path &path)
{
    std::string filePath(path.GetAbsolutePath());
    if (filePath.rfind(absoluteEtsPath_.Utf8(), 0) != 0) {
        diagnosticEngine_.LogDiagnostic(diagnostic::SOURCE_OUTSIDE_ETS_PATH,
                                        util::DiagnosticMessageParams {util::StringView(filePath)}, srcPos_);
        return "";
    }
    auto name = FormRelativeModuleName(filePath.substr(absoluteEtsPath_.Length()));
    return util::UString(name, allocator_).View();
}

// should be implemented with a stable name -> path mapping list
static std::optional<std::string> TryFormModuleName(const std::string &filePath, std::string_view unitName,
                                                    std::string_view unitPath, std::string_view cachePath)
{
    if (cachePath.empty() && filePath.rfind(unitPath, 0) != 0) {
        return std::nullopt;
    }
    if (!cachePath.empty() && filePath.rfind(cachePath, 0) != 0 && filePath.rfind(unitPath, 0) != 0) {
        return std::nullopt;
    }
    std::string_view actualUnitPath = unitPath;
    if (!cachePath.empty() && filePath.rfind(cachePath, 0) == 0) {
        actualUnitPath = cachePath;
    }
    auto relativePath = FormRelativeModuleName(filePath.substr(actualUnitPath.size()));
    if (relativePath.empty() || FormUnitName(unitName).empty()) {
        return FormUnitName(unitName) + relativePath;
    }
    return FormUnitName(unitName) + "." + relativePath;
}

template <typename DynamicPaths>
static std::string TryFormDynamicModuleName(const DynamicPaths &dynPaths, std::string const filePath)
{
    for (auto const &[unitName, did] : dynPaths) {
        if (did.Path().empty()) {
            // NOTE(dkofanov): related to #23698. Current assumption: if 'declPath' is absent, it is a pure-dynamic
            // source, and, as soon it won't be parsed, no module should be created.
            continue;
        }
        if (auto res = TryFormModuleName(filePath, unitName, did.Path(), ""); res) {
            return res.value();
        }
    }
    return "";
}

util::StringView ImportPathManager::FormModuleName(const util::Path &path, const lexer::SourcePosition &srcPos)
{
    srcPos_ = srcPos;
    return FormModuleName(path);
}

util::StringView ImportPathManager::FormModuleName(const util::Path &path)
{
    if (!absoluteEtsPath_.Empty()) {
        return FormModuleNameSolelyByAbsolutePath(path);
    }

    if (!parseList_.empty() && parseList_[0].importData.IsExternalBinaryImport()) {
        return util::StringView(parseList_[0].importData.ohmUrl);
    }

    if (arktsConfig_->Package().empty() && !arktsConfig_->UseUrl()) {
        return path.GetFileName();
    }

    std::string const filePath(path.GetAbsolutePath());
    if (auto dmn = TryFormDynamicModuleName(arktsConfig_->Dependencies(), filePath); !dmn.empty()) {
        return util::UString(dmn, allocator_).View();
    }
    if (auto res = TryFormModuleName(filePath, arktsConfig_->Package(), arktsConfig_->BaseUrl() + pathDelimiter_.data(),
                                     arktsConfig_->CacheDir());
        res) {
        return util::UString(res.value(), allocator_).View();
    }
    if (!stdLib_.empty()) {
        if (auto res =
                TryFormModuleName(filePath, "std", stdLib_ + pathDelimiter_.at(0) + "std", arktsConfig_->CacheDir());
            res) {
            return util::UString(res.value(), allocator_).View();
        }
        if (auto res = TryFormModuleName(filePath, "escompat", stdLib_ + pathDelimiter_.at(0) + "escompat",
                                         arktsConfig_->CacheDir());
            res) {
            return util::UString(res.value(), allocator_).View();
        }
    }
    for (auto const &[unitName, unitPath] : arktsConfig_->Paths()) {
        if (auto res = TryFormModuleName(filePath, unitName, unitPath[0], arktsConfig_->CacheDir()); res) {
            return util::UString(res.value(), allocator_).View();
        }
    }
    // NOTE (hurton): as a last step, try resolving using the BaseUrl again without a path delimiter at the end
    if (auto res =
            TryFormModuleName(filePath, arktsConfig_->Package(), arktsConfig_->BaseUrl(), arktsConfig_->CacheDir());
        res) {
        return util::UString(res.value(), allocator_).View();
    }

    diagnosticEngine_.LogDiagnostic(diagnostic::UNRESOLVED_MODULE,
                                    util::DiagnosticMessageParams {util::StringView(filePath)}, srcPos_);
    return "";
}

bool ImportPathManager::ImportMetadata::IsValid() const
{
    return resolvedSource != ERROR_LITERAL;
}

util::StringView ImportPathManager::FormRelativePath(const util::Path &path)
{
    std::string filePath(path.GetAbsolutePath());
    util::Helpers::CheckValidFileName(path.GetFileNameWithExtension(), diagnosticEngine_);
    auto const tryFormRelativePath = [&filePath](std::string const &basePath,
                                                 std::string const &prefix) -> std::optional<std::string> {
        if (filePath.rfind(basePath, 0) != 0) {
            return std::nullopt;
        }
        return filePath.replace(0, basePath.size(), prefix);
    };

    if (!absoluteEtsPath_.Empty()) {
        if (auto res = tryFormRelativePath(absoluteEtsPath_.Mutf8() + pathDelimiter_.data(), ""); res) {
            return util::UString(res.value(), allocator_).View();
        }
    }

    if (arktsConfig_->Package().empty() && !arktsConfig_->UseUrl()) {
        return path.GetFileNameWithExtension();
    }

    if (auto res = tryFormRelativePath(arktsConfig_->BaseUrl(), arktsConfig_->Package()); res) {
        return util::UString(res.value(), allocator_).View();
    }

    for (auto const &[unitName, unitPath] : arktsConfig_->Paths()) {
        if (auto res = tryFormRelativePath(unitPath[0], unitName); res) {
            return util::UString(res.value(), allocator_).View();
        }
    }

    for (auto const &[unitName, unitPath] : arktsConfig_->Dependencies()) {
        if (auto res = tryFormRelativePath(unitName, unitName); res) {
            return util::UString(res.value(), allocator_).View();
        }
    }

    return path.GetFileNameWithExtension();
}

}  // namespace ark::es2panda::util
#undef USE_UNIX_SYSCALL
