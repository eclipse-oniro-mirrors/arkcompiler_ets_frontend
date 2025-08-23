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
#include <os/filesystem.h>
#include "util/arktsconfig.h"
#include "util/diagnostic.h"
#include "util/diagnosticEngine.h"
#include "generated/diagnostic.h"

#include "parser/context/parserContext.h"
#include "parser/program/program.h"
#include "ir/expressions/literals/stringLiteral.h"

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
namespace ark::es2panda::util {

constexpr size_t SUPPORTED_INDEX_FILES_SIZE = 3;
constexpr size_t SUPPORTED_EXTENSIONS_SIZE = 6;
constexpr size_t ALLOWED_EXTENSIONS_SIZE = 8;

static bool IsCompatibleExtension(const std::string &extension)
{
    return extension == ".ets" || extension == ".ts" || extension == ".sts";
}

static bool IsAbsolute(const std::string &path)
{
#ifndef ARKTSCONFIG_USE_FILESYSTEM
    return !path.empty() && path[0] == '/';
#else
    return fs::path(path).is_absolute();
#endif  // ARKTSCONFIG_USE_FILESYSTEM
}

ImportPathManager::ImportMetadata ImportPathManager::GatherImportMetadata(parser::Program *program,
                                                                          ImportFlags importFlags,
                                                                          ir::StringLiteral *importPath)
{
    srcPos_ = &importPath->Start();
    // NOTE(dkofanov): The code below expresses the idea of 'dynamicPaths' defining separated, virtual file system.
    // Probably, paths of common imports should be isolated from the host fs as well, being resolved by 'ModuleInfo'
    // instead of 'AbsoluteName'.
    isDynamic_ = program->ModuleInfo().isDeclForDynamicStaticInterop;
    auto curModulePath = isDynamic_ ? program->ModuleInfo().moduleName : program->AbsoluteName();
    auto [resolvedImportPath, resolvedIsDynamic] = ResolvePath(curModulePath.Utf8(), importPath);
    if (resolvedImportPath.empty()) {
        ES2PANDA_ASSERT(diagnosticEngine_.IsAnyError());
        return ImportMetadata {util::ImportFlags::NONE, Language::Id::COUNT, ERROR_LITERAL};
    }

    globalProgram_->AddFileDependencies(std::string(curModulePath), std::string(resolvedImportPath));

    ImportMetadata importData {importFlags};
    importData.resolvedSource = resolvedImportPath;
    if (resolvedIsDynamic) {
        ES2PANDA_ASSERT(!IsAbsolute(std::string(importData.resolvedSource)));
        auto it = arktsConfig_->DynamicPaths().find(std::string(importData.resolvedSource));
        ES2PANDA_ASSERT(it != arktsConfig_->DynamicPaths().cend());
        const auto &dynImportData = it->second;
        importData.lang = dynImportData.GetLanguage().GetId();
        importData.declPath = dynImportData.DeclPath();
        importData.ohmUrl = dynImportData.OhmUrl();
    } else {
        ES2PANDA_ASSERT(IsAbsolute(std::string(importData.resolvedSource)));
        importData.lang = ToLanguage(program->Extension()).GetId();
        importData.declPath = util::ImportPathManager::DUMMY_PATH;
        importData.ohmUrl = util::ImportPathManager::DUMMY_PATH;
    }

    if (globalProgram_->AbsoluteName() != resolvedImportPath) {
        AddToParseList(importData);
    }

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
    srcPos_ = &importPath->Start();
    // NOTE(dkofanov): #23698 related. In case of 'dynamicPaths', resolved path is "virtual" module-path, may be not
    // what the plugin expecting.
    return ResolvePath(curModulePath.Utf8(), importPath).resolvedPath;
}

ImportPathManager::ResolvedPathRes ImportPathManager::ResolvePath(std::string_view curModulePath,
                                                                  ir::StringLiteral *importPath) const
{
    if (importPath->Str().Empty()) {
        diagnosticEngine_.LogDiagnostic(diagnostic::EMPTY_IMPORT_PATH, util::DiagnosticMessageParams {});
        return {*importPath};
    }
    const auto &entriesMap = arktsConfig_->Entries();
    if (auto it = entriesMap.find(importPath->Str().Mutf8()); it != entriesMap.cend()) {
        return {UString(it->second, allocator_).View().Utf8()};
    }
    if (IsRelativePath(*importPath)) {
        const size_t pos = curModulePath.find_last_of("/\\");
        ES2PANDA_ASSERT(pos != std::string::npos);

        auto currentDirectory = curModulePath.substr(0, pos);
        auto resolvedPath = UString(currentDirectory, allocator_);
        resolvedPath.Append(pathDelimiter_);
        resolvedPath.Append(*importPath);
        // NOTE(dkofanov): Suspicious shortcut: shouldn't it fallthrough into `ResolveAbsolutePath`?
        return AppendExtensionOrIndexFileIfOmitted(resolvedPath.View());
    }

    return ResolveAbsolutePath(*importPath);
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
            *srcPos_);
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
                                        *srcPos_);
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
    srcPos_ = &srcPos;
    ES2PANDA_ASSERT(
        IsAbsolute(packageDir.Mutf8()));  // This should be an absolute path for 'AddToParseList' be able to resolve it.
    AddToParseList({util::ImportFlags::IMPLICIT_PACKAGE_IMPORT, Language::Id::ETS, packageDir.Utf8(),
                    util::ImportPathManager::DUMMY_PATH});
}

void ImportPathManager::AddToParseList(const ImportMetadata importMetadata)
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
                                        *srcPos_);
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

void ImportPathManager::MarkAsParsed(StringView path)
{
    for (auto &parseInfo : parseList_) {
        if (parseInfo.importData.resolvedSource == path.Utf8()) {
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

std::string ImportPathManager::TryMatchDynamicPath(std::string_view fixedPath) const
{
    // Probably, 'NormalizePath' should be moved to 'AppendExtensionOrIndexFileIfOmitted'.
    auto normalizedPath = ark::os::NormalizePath(std::string(fixedPath));
    std::replace_if(
        normalizedPath.begin(), normalizedPath.end(), [&](auto &c) { return c == pathDelimiter_[0]; }, '/');
    // NOTE(dkofanov): #23877. See also 'arktsconfig.cpp'.
    if (arktsConfig_->DynamicPaths().find(normalizedPath) != arktsConfig_->DynamicPaths().cend()) {
        return normalizedPath;
    }
    return {};
}

std::string_view ImportPathManager::DirOrDirWithIndexFile(StringView dir) const
{
    // Supported index files: keep this checking order
    std::array<std::string, SUPPORTED_INDEX_FILES_SIZE> supportedIndexFiles = {"index.ets", "index.sts", "index.ts"};
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
    if (auto resolvedDynamic = TryMatchDynamicPath(fixedPath); !resolvedDynamic.empty()) {
        return {UString(resolvedDynamic, allocator_).View().Utf8(), true};
    }

    auto path = UString(fixedPath, allocator_).View();
    StringView realPath = GetRealPath(path);
    if (ark::os::file::File::IsRegularFile(realPath.Mutf8())) {
        return {realPath.Utf8()};
    }

    if (ark::os::file::File::IsDirectory(realPath.Mutf8())) {
        return {DirOrDirWithIndexFile(realPath)};
    }

    // Supported extensions: keep this checking order, and header files should follow source files
    std::array<std::string, SUPPORTED_EXTENSIONS_SIZE> supportedExtensions = {".ets",   ".d.ets", ".sts",
                                                                              ".d.sts", ".ts",    ".d.ts"};
    for (const auto &extension : supportedExtensions) {
        if (ark::os::file::File::IsRegularFile(path.Mutf8() + extension)) {
            return {GetRealPath(UString(path.Mutf8().append(extension), allocator_).View()).Utf8()};
        }
    }

    diagnosticEngine_.LogDiagnostic(diagnostic::UNSUPPORTED_PATH,
                                    util::DiagnosticMessageParams {util::StringView(path.Mutf8())}, *srcPos_);
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
    // Supported extensions: keep this checking order, and source files should follow header files
    std::array<std::string, ALLOWED_EXTENSIONS_SIZE> supportedExtensionsDesc = {".d.ets", ".ets", ".d.sts", ".sts",
                                                                                ".d.ts",  ".ts",  ".js",    ".abc"};
    for (const auto &ext : supportedExtensionsDesc) {
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
    if (filePath.rfind(absoluteEtsPath_, 0) != 0) {
        diagnosticEngine_.LogDiagnostic(diagnostic::SOURCE_OUTSIDE_ETS_PATH,
                                        util::DiagnosticMessageParams {util::StringView(filePath)}, *srcPos_);
        return "";
    }
    auto name = FormRelativeModuleName(filePath.substr(absoluteEtsPath_.size()));
    return util::UString(name, allocator_).View();
}

template <typename DynamicPaths, typename ModuleNameFormer>
static std::string TryFormDynamicModuleName(const DynamicPaths &dynPaths, const ModuleNameFormer &tryFormModuleName)
{
    for (auto const &[unitName, did] : dynPaths) {
        if (did.DeclPath().empty()) {
            // NOTE(dkofanov): related to #23698. Current assumption: if 'declPath' is absent, it is a pure-dynamic
            // source, and, as soon it won't be parsed, no module should be created.
            continue;
        }
        if (auto res = tryFormModuleName(unitName, did.DeclPath()); res) {
            return res.value();
        }
    }
    return "";
}

util::StringView ImportPathManager::FormModuleName(const util::Path &path, const lexer::SourcePosition &srcPos)
{
    srcPos_ = &srcPos;
    return FormModuleName(path);
}

util::StringView ImportPathManager::FormModuleName(const util::Path &path)
{
    if (!absoluteEtsPath_.empty()) {
        return FormModuleNameSolelyByAbsolutePath(path);
    }

    if (arktsConfig_->Package().empty() && !arktsConfig_->UseUrl()) {
        return path.GetFileName();
    }

    std::string const filePath(path.GetAbsolutePath());

    // should be implemented with a stable name -> path mapping list
    auto const tryFormModuleName = [filePath](std::string_view unitName,
                                              std::string_view unitPath) -> std::optional<std::string> {
        if (filePath.rfind(unitPath, 0) != 0) {
            return std::nullopt;
        }
        auto relativePath = FormRelativeModuleName(filePath.substr(unitPath.size()));
        return FormUnitName(unitName) +
               (relativePath.empty() || FormUnitName(unitName).empty() ? relativePath : ("." + relativePath));
    };

    for (auto const &[unitName, unitPath] : arktsConfig_->Entries()) {
        if (unitPath == filePath) {
            return util::UString(unitName, allocator_).View();
        }
    }

    if (auto res = tryFormModuleName(arktsConfig_->Package(), arktsConfig_->BaseUrl() + pathDelimiter_.data()); res) {
        return util::UString(res.value(), allocator_).View();
    }
    if (!stdLib_.empty()) {
        if (auto res = tryFormModuleName("std", stdLib_ + pathDelimiter_.at(0) + "std"); res) {
            return util::UString(res.value(), allocator_).View();
        }
        if (auto res = tryFormModuleName("escompat", stdLib_ + pathDelimiter_.at(0) + "escompat"); res) {
            return util::UString(res.value(), allocator_).View();
        }
    }
    for (auto const &[unitName, unitPath] : arktsConfig_->Paths()) {
        if (auto res = tryFormModuleName(unitName, unitPath[0]); res) {
            return util::UString(res.value(), allocator_).View();
        }
    }
    if (auto dmn = TryFormDynamicModuleName(arktsConfig_->DynamicPaths(), tryFormModuleName); !dmn.empty()) {
        return util::UString(dmn, allocator_).View();
    }
    // NOTE (hurton): as a last step, try resolving using the BaseUrl again without a path delimiter at the end
    if (auto res = tryFormModuleName(arktsConfig_->Package(), arktsConfig_->BaseUrl()); res) {
        return util::UString(res.value(), allocator_).View();
    }

    diagnosticEngine_.LogDiagnostic(diagnostic::UNRESOLVED_MODULE,
                                    util::DiagnosticMessageParams {util::StringView(filePath)}, *srcPos_);
    return "";
}

bool ImportPathManager::ImportMetadata::IsValid() const
{
    return resolvedSource != ERROR_LITERAL;
}

util::StringView ImportPathManager::FormRelativePath(const util::Path &path)
{
    std::string filePath(path.GetAbsolutePath());
    auto const tryFormRelativePath = [&filePath](std::string const &basePath,
                                                 std::string const &prefix) -> std::optional<std::string> {
        if (filePath.rfind(basePath, 0) != 0) {
            return std::nullopt;
        }
        return filePath.replace(0, basePath.size(), prefix);
    };

    if (!absoluteEtsPath_.empty()) {
        if (auto res = tryFormRelativePath(absoluteEtsPath_ + pathDelimiter_.data(), ""); res) {
            return util::UString(res.value(), allocator_).View();
        }
    }

    if (arktsConfig_->Package().empty() && !arktsConfig_->UseUrl()) {
        return path.GetFileNameWithExtension();
    }

    for (auto const &[unitName, unitPath] : arktsConfig_->Entries()) {
        if (unitPath == filePath) {
            return util::UString(unitName, allocator_).View();
        }
    }

    if (auto res = tryFormRelativePath(arktsConfig_->BaseUrl(), arktsConfig_->Package()); res) {
        return util::UString(res.value(), allocator_).View();
    }

    for (auto const &[unitName, unitPath] : arktsConfig_->Paths()) {
        if (auto res = tryFormRelativePath(unitPath[0], unitName); res) {
            return util::UString(res.value(), allocator_).View();
        }
    }

    for (auto const &[unitName, unitPath] : arktsConfig_->DynamicPaths()) {
        if (auto res = tryFormRelativePath(unitName, unitName); res) {
            return util::UString(res.value(), allocator_).View();
        }
    }

    return path.GetFileNameWithExtension();
}

}  // namespace ark::es2panda::util
#undef USE_UNIX_SYSCALL
