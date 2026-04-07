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

#ifndef ES2PANDA_UTIL_IMPORT_PATH_MANAGER_H
#define ES2PANDA_UTIL_IMPORT_PATH_MANAGER_H

#include "language.h"
#if defined PANDA_TARGET_MOBILE
#define USE_UNIX_SYSCALL
#endif

#include "util/ustring.h"
#include "util/enumbitops.h"
#include "util/path.h"
#include "util/options.h"
#include "util/diagnosticEngine.h"
#include "parser/program/ImportCache.h"

namespace ark::panda_file {
class File;
}  // namespace ark::panda_file

namespace ark::es2panda {
class ArkTsConfig;
}  // namespace ark::es2panda

namespace ark::es2panda::util {
namespace gen::extension {
enum Enum : size_t;
}  // namespace gen::extension

using ENUMBITOPS_OPERATORS;

}  // namespace ark::es2panda::util

namespace ark::es2panda::ir {
class StringLiteral;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::util {

template <ModuleKind KIND>
constexpr parser::CacheType SelectCacheType()
{
    return KIND == ModuleKind::METADATA_DECL ? parser::CacheType::METADATA : parser::CacheType::SOURCES;
}

}  // namespace ark::es2panda::util

namespace ark::es2panda::parser {
template <util::ModuleKind KIND>
class ProgramAdapter;
class Program;

using PackageProgram = ProgramAdapter<util::ModuleKind::PACKAGE>;
class NonPackageProgram;

// Source-level modules:
using SourceProgram = ProgramAdapter<util::ModuleKind::MODULE>;
using SourceDeclarationProgram = ProgramAdapter<util::ModuleKind::SOURCE_DECL>;
// Based on the loaded metadata from abc file
using MetadataBasedProgram = ProgramAdapter<util::ModuleKind::METADATA_DECL>;
// Internal, generated modules (for caching purpose):
using LowDeclarationProgram = ProgramAdapter<util::ModuleKind::ETSCACHE_DECL>;

}  // namespace ark::es2panda::parser

namespace ark::es2panda::util {

inline bool IsAbsolute(const std::string &path)
{
#ifndef ARKTSCONFIG_USE_FILESYSTEM
    return !path.empty() && path[0] == '/';
#else
    return fs::path(path).is_absolute();
#endif  // ARKTSCONFIG_USE_FILESYSTEM
}

struct ModuleInfo {
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    ArenaString moduleName {};
    ArenaString modulePrefix {};
    ModuleKind kind {};
    // NOTE(dkofanov): Should be refactored and aligned with 'ModuleKind' and
    // 'Program::MaybeTransformToDeclarationModule'.
    bool isDeclForDynamicStaticInterop {};
    // NOLINTEND(misc-non-private-member-variables-in-classes)
    Language lang = Language(Language::Id::ETS);
};

class ImportPathManager;

class ImportInfo : public parser::CacheReference<> {
private:
    static constexpr std::string_view ABC_SUFFIX = ".abc";

public:
    // NOTE(dkofanov): 'lang' and 'isExternalModule' are to be reduced.
    ImportInfo(const ImportPathManager &ipm, std::string_view resolvedSource, Language::Id lang = Language::Id::ETS,
               bool isExternalModule = false);
    ImportInfo() = default;
    ImportInfo(const ImportInfo &other);
    const ImportInfo &operator=(const ImportInfo &other);  // NOLINT(misc-unconventional-assign-operator)
    NO_MOVE_SEMANTIC(ImportInfo);
    ~ImportInfo() = default;

    static constexpr auto DUMMY_PATH = "dummy_path";  // CC-OFF(G.NAM.03-CPP) project code style

public:
    std::string_view ResolvedSource() const
    {
        return resolvedSource_;
    }
    std::string_view ModuleName() const
    {
        return moduleName_;
    }

    std::string_view OhmUrl() const;

    bool PointsToPackage() const
    {
        // External-library check is intended to avoid interpreting dynamic-path as directory.
        return !ResolvedPathIsVirtual() && ark::os::file::File::IsDirectory(std::string(resolvedSource_));
    }

    bool ReferencesABC() const
    {
        return (extModuleData_ != nullptr) && Helpers::EndsWith(extModuleData_->Path(), ABC_SUFFIX);
    }

    const std::string &AbcPath() const
    {
        ES2PANDA_ASSERT(ReferencesABC());
        return extModuleData_->Path();
    }

    const parser::CacheReference<parser::AnyCacheType> &Data() const
    {
        return *this;
    }

    bool IsValid() const;

private:
    template <ModuleKind KIND, bool SHOULD_CACHE = true>
    void SetTextFile(const std::string &file, util::DiagnosticEngine *de)
    {
        static_assert(KIND != ModuleKind::METADATA_DECL);  // metadata is not stored as a text

        std::ifstream inputStream {file};
        if (!inputStream) {
            de->LogDiagnostic(diagnostic::OPEN_FAILED, util::DiagnosticMessageParams {file});
            return;
        }

        std::stringstream ss {};
        ss << inputStream.rdbuf();
        auto text = std::move(ss).str();
        if (text.empty()) {
            de->LogDiagnostic(diagnostic::EMPTY_SOURCE_FILE, util::DiagnosticMessageParams {file});
        }
        SetData<KIND, SHOULD_CACHE>(file, std::move(text));
    }

    template <bool SHOULD_CACHE = true>
    void SetBinFile(const panda_file::File &pf)
    {
        auto metadataSpan = GetMetadata(pf);
        std::vector<uint8_t> metadata;
        metadata.insert(metadata.begin(), metadataSpan.begin(), metadataSpan.end());
        SetData<ModuleKind::METADATA_DECL, SHOULD_CACHE>(AbcPath(), std::move(metadata));
    }

    template <ModuleKind KIND, bool SHOULD_CACHE = true>
    void SetData(std::string textSource, parser::SelectCacheDataType<SelectCacheType<KIND>(), true> contents)
    {
        ES2PANDA_ASSERT(Data().Kind() == ModuleKind::UNKNOWN);
        Set(parser::ImportCache<SelectCacheType<KIND>()>::template StoreContents<KIND, SHOULD_CACHE>(
            *this, textSource, std::move(contents)));
    }

    void LinkFractionInfoToPackage(const parser::PackageProgram &package);

    bool ResolvedPathIsVirtual() const
    {
        ES2PANDA_ASSERT(!resolvedSource_.empty());
        return !IsAbsolute(std::string(resolvedSource_));
    }

    inline static Span<const uint8_t> GetMetadata(const panda_file::File &pf);

private:
    ArenaString resolvedSource_ {ERROR_LITERAL};
    ArenaString moduleName_ {};

    // NOTE(dkofanov): #32416 These fields should be refactored:
    const ArkTsConfig::ExternalModuleData *extModuleData_ {};
    Language::Id lang_ {Language::Id::ETS};

    // NOTE(dkofanov): #32416 These interfaces are deprecated:
public:
    std::string_view DeclPath() const;
    bool HasSpecifiedDeclPath() const;

    auto Lang() const
    {
        return lang_;
    }

    friend ImportPathManager;
};

class ImportPathManager {
public:
    static constexpr std::string_view ANNOTATION_MODULE_DECLARATION =
        "Larkruntime/annotation/ModuleDeclaration;";  // CC-OFF(G.NAM.03-CPP) project code style
    static constexpr std::string_view ABC_SUFFIX = ImportInfo::ABC_SUFFIX;
    static constexpr std::string_view ETS_SUFFIX = ".ets";
    static constexpr std::string_view D_ETS_SUFFIX = ".d.ets";
    static constexpr std::string_view CACHE_SUFFIX = ".etscache";
    static constexpr std::string_view ETSSTDLIB_ABC_SUFFIX = "etsstdlib.abc";

    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr size_t extensionsSize = 9;
    // declaration file must follow source file according to spec
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr std::array<std::string_view, extensionsSize> supportedExtensions = {
        ETS_SUFFIX, D_ETS_SUFFIX, CACHE_SUFFIX, ".sts", ".d.sts", ".ts", ".d.ts", ".js"};
    // source file must follow declaration file so that extension "best match" will succeed
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr std::array<std::string_view, extensionsSize> supportedExtensionsInversed = {
        D_ETS_SUFFIX, ETS_SUFFIX, CACHE_SUFFIX, ".d.sts", ".sts", ".d.ts", ".ts", ".js"};

    struct ParseInfo {
        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        bool isParsed {};
        parser::Program *program {nullptr};
        // NOLINTEND(misc-non-private-member-variables-in-classes)
    };

    using FileDependenciesMap = ArenaUnorderedMap<ArenaString, ArenaUnorderedSet<ArenaString>>;
    using FileOutputMatching = ArenaUnorderedMap<ArenaString, ArenaString>;

    explicit ImportPathManager(public_lib::Context *context);

    NO_COPY_SEMANTIC(ImportPathManager);
    NO_MOVE_SEMANTIC(ImportPathManager);
    ImportPathManager() = delete;
    ~ImportPathManager() = default;

    [[nodiscard]] const ArenaVector<ParseInfo> &GetParseQueue() const
    {
        return parseQueue_;
    }

    [[nodiscard]] ArenaVector<ParseInfo> &GetParseQueue()
    {
        return parseQueue_;
    }

    void ClearParseList()
    {
        parseQueue_.clear();
    }

    parser::Program *GatherImportInfo(parser::Program *importer, ir::StringLiteral *importPath);
    parser::Program *EnsurePackageIsRegisteredByPackageFraction(parser::Program *fraction,
                                                                ir::ETSPackageDeclaration *packageDecl);
    // API version for resolving paths. Kept only for API compatibility. Doesn't support 'dependencies'.
    util::StringView ResolvePathAPI(parser::Program *importer, ir::StringLiteral *importPath) const;

    const ArkTsConfig &ArkTSConfig() const;

    void InitParseQueueForSimult();
    void IntroduceMainProgramForSimult();

    void SetupGlobalProgram();

    parser::Program *IntroduceStdlibImportProgram(std::string &&contents);

    parser::Program *SetupProgramForDebugInfoPlugin(std::string_view sourceFilePath, std::string_view moduleName);

    parser::Program *SearchResolved(const ImportInfo &importInfo) const;

    static std::string FormEtscacheFilePath(std::string moduleName, const std::string &cacheDir);
    static void ExtractEtscacheToFile(const panda_file::File &pf, const std::string &abcPath,
                                      const std::string &cacheDir);
    std::string FormAbcFilePath(const ImportInfo &imd) const;

    auto *Context() const
    {
        return &ctx_;
    }

    const auto &SrcPos() const
    {
        return srcPos_;
    }

    const auto &GetFileDependencies() const
    {
        return fileDependencies_;
    }

    void AddFileDependencies(std::string_view file, std::string_view depFile)
    {
        fileDependencies_[ArenaString {file}].emplace(depFile);
    }

    const auto &GetOutputMatching() const
    {
        return outputMatching_;
    }

    void AddOutputMatching(std::string_view file, std::string_view outPath)
    {
        outputMatching_[ArenaString {file}] = outPath;
    }

private:
    template <typename VarBinderT, Language::Id LANG_ID>
    void SetupGlobalProgram(public_lib::Context *ctx);

    struct ResolvedPathRes {
        // On successfull resolving, 2 variants are possible:
        // `resolvedPath` is a module-path - if dynamic path was resolved;
        // `resolvedPath` is a realpath - if static path was resolved.
        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        std::string resolvedPath;
        bool resolvedIsExternalModule {false};
        // NOLINTEND(misc-non-private-member-variables-in-classes)
    };
    ImportInfo ResolvePath(parser::Program *importer, std::string_view importPath) const;
    ResolvedPathRes ResolveAbsolutePath(std::string_view importPathNode) const;
    std::string DirOrDirWithIndexFile(std::string resolvedPathPrototype) const;
    ResolvedPathRes AppendExtensionOrIndexFileIfOmitted(std::string resolvedPathPrototype) const;
    std::string TryMatchDependencies(std::string_view fixedPath) const;
    ResolvedPathRes TryResolvePath(std::string resolvedPathPrototype) const;
    void TryMatchStaticResolvedPath(ResolvedPathRes *result) const;
    void TryMatchDynamicResolvedPath(ResolvedPathRes *result) const;
    bool DeclarationIsInCache(ImportInfo &importInfo);

    template <ModuleKind KIND, typename VarBinderT = void>
    parser::ProgramAdapter<KIND> *IntroduceProgram(const ImportInfo &importInfo);
    parser::Program *IntroduceProgram(const ImportInfo &importInfo);

    parser::Program *LookupImportDataAndIntroduceProgram(ImportInfo *importInfo);
    parser::Program *LookupProgramCaches(const ImportInfo &importInfo);
    void LookupMemCache(ImportInfo *importInfo);
    void LookupDiskData(ImportInfo *importInfo);
    void LookupEtscacheFile(ImportInfo *importInfo);

    void LookupSourceFile(ImportInfo *importInfo);
    void RegisterSourceFile(const ImportInfo &importInfo);

    void RegisterPackageFraction(parser::PackageProgram *package, ImportInfo *importInfo);

    parser::PackageProgram *RegisterSourcesForPackageFromGlobbedDirectory(const ImportInfo &importInfo);
#ifdef USE_UNIX_SYSCALL
    void UnixRegisterSourcesForPackageFromGlobbedDirectory(parser::ProgramAdapter<ModuleKind::PACKAGE> *pkg,
                                                           const ImportInfo &importInfo);
#endif

    parser::PackageProgram *NewEmptyPackage(const ImportInfo &importInfo);

    void RegisterProgram(parser::Program *program);

    parser::Program *GetGlobalProgram() const;

    DiagnosticEngine *DE() const;

private:
    public_lib::Context &ctx_;
    ArenaVector<ParseInfo> parseQueue_;

    class ResolvedSources;
    ResolvedSources &resolvedSources_;

    std::string_view pathDelimiter_ {ark::os::file::File::GetPathDelim()};
    mutable lexer::SourcePosition srcPos_ {};
    bool isDynamic_ = false;
    std::unordered_set<std::string> processedAbcFiles_;

    FileDependenciesMap fileDependencies_;
    FileOutputMatching outputMatching_;
};

}  // namespace ark::es2panda::util

#endif  // ES2PANDA_UTIL_IMPORT_PATH_MANAGER_H