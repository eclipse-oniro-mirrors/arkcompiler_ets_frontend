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
#include "parser/program/DeclarationCache.h"

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
enum class ModuleKind : uint8_t {
    UNKNOWN,

    PACKAGE,
    MODULE,
    SOURCE_DECL,
    ETSCACHE_DECL,
    DECLLESS_DYNAMIC,

    SIMULT_MAIN,
};
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
// Internal, generated modules (for caching purpose):
using LowDeclarationProgram = ProgramAdapter<util::ModuleKind::ETSCACHE_DECL>;

}  // namespace ark::es2panda::parser

class ArkTsConfig;

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

class ImportMetadata : public parser::DeclarationCache::CacheReference {
private:
    static constexpr std::string_view ABC_SUFFIX = ".abc";

public:
    // NOTE(dkofanov): 'lang' and 'isExternalModule' are to be reduced.
    ImportMetadata(const ImportPathManager &ipm, std::string_view resolvedSource, Language::Id lang = Language::Id::ETS,
                   bool isExternalModule = false);
    ImportMetadata() = default;
    ImportMetadata(const ImportMetadata &other);
    const ImportMetadata &operator=(const ImportMetadata &other);
    NO_MOVE_SEMANTIC(ImportMetadata);

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

    const parser::DeclarationCache::CacheReference &Text() const
    {
        return *this;
    }

    bool IsValid() const;

private:
    template <ModuleKind KIND, bool SHOULD_CACHE = true>
    void SetFile(const std::string &file, util::DiagnosticEngine *de)
    {
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
        SetText<KIND, SHOULD_CACHE>(file, std::move(text));
    }

    template <ModuleKind KIND, bool SHOULD_CACHE = true>
    void SetText(std::string textSource, std::string &&contents)
    {
        ES2PANDA_ASSERT(Text().Kind() == ModuleKind::UNKNOWN);
        Set<KIND, SHOULD_CACHE>(textSource, std::move(contents));
    }

    void LinkFractionMetadataToPackage(const parser::PackageProgram &package);

    bool ResolvedPathIsVirtual() const
    {
        ES2PANDA_ASSERT(!resolvedSource_.empty());
        return !IsAbsolute(std::string(resolvedSource_));
    }

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
        "Lstd/annotations/ModuleDeclaration;";  // CC-OFF(G.NAM.03-CPP) project code style
    static constexpr std::string_view ABC_SUFFIX = ImportMetadata::ABC_SUFFIX;
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
        parser::Program *program;
        // NOLINTEND(misc-non-private-member-variables-in-classes)
    };

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

    parser::Program *GatherImportMetadata(parser::Program *importer, ir::StringLiteral *importPath);
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

    parser::Program *SearchResolved(const ImportMetadata &importMetadata) const;

    std::string FormEtscacheFilePath(const ImportMetadata &imd) const;

    auto *Context() const
    {
        return &ctx_;
    }

    const auto &SrcPos() const
    {
        return srcPos_;
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
    ImportMetadata ResolvePath(parser::Program *importer, std::string_view importPath) const;
    ResolvedPathRes ResolveAbsolutePath(std::string_view importPathNode) const;
    std::string DirOrDirWithIndexFile(std::string resolvedPathPrototype) const;
    ResolvedPathRes AppendExtensionOrIndexFileIfOmitted(std::string resolvedPathPrototype) const;
    std::string TryMatchDependencies(std::string_view fixedPath) const;
    ResolvedPathRes TryResolvePath(std::string resolvedPathPrototype) const;
    void TryMatchStaticResolvedPath(ResolvedPathRes *result) const;
    void TryMatchDynamicResolvedPath(ResolvedPathRes *result) const;
    bool DeclarationIsInCache(ImportMetadata &importData);

    template <ModuleKind KIND, typename VarBinderT = void>
    parser::ProgramAdapter<KIND> *IntroduceProgram(const ImportMetadata &importMetadata);
    parser::Program *IntroduceProgram(const ImportMetadata &importMetadata);

    parser::Program *LookupCachesAndIntroduceProgram(ImportMetadata *importMetadata);
    parser::Program *LookupProgramCaches(const ImportMetadata &importData);
    void LookupMemCache(ImportMetadata *importMetadata);
    void LookupDiskCache(ImportMetadata *importMetadata);
    void MaybeUnpackAbcAndEmplaceInCacheDir(const ImportMetadata &importMetadata);
    void LookupEtscacheFile(ImportMetadata *importData);

    void LookupSourceFile(ImportMetadata *importMetadata);
    void RegisterSourceFile(const ImportMetadata &importMetadata);

    void RegisterPackageFraction(parser::PackageProgram *package, ImportMetadata *importMetadata);

    parser::PackageProgram *RegisterSourcesForPackageFromGlobbedDirectory(const ImportMetadata &importMetadata);
#ifdef USE_UNIX_SYSCALL
    void UnixRegisterSourcesForPackageFromGlobbedDirectory(parser::ProgramAdapter<ModuleKind::PACKAGE> *pkg,
                                                           const ImportMetadata &importMetadata);
#endif

    parser::PackageProgram *NewEmptyPackage(const ImportMetadata &importMetadata);

    void RegisterProgram(parser::Program *program);

    parser::Program *GetGlobalProgram() const;

    DiagnosticEngine *DE() const;

private:
    public_lib::Context &ctx_;
    ArenaVector<ParseInfo> parseQueue_;

    class ResolvedSources;
    ResolvedSources &resolvedSources_;

    std::vector<util::StringView> directImportsFromMainSource_ {};
    std::string_view pathDelimiter_ {ark::os::file::File::GetPathDelim()};
    mutable lexer::SourcePosition srcPos_ {};
    bool isDynamic_ = false;
    std::unordered_set<std::string> processedAbcFiles_;
};

}  // namespace ark::es2panda::util

#endif  // ES2PANDA_UTIL_IMPORT_PATH_MANAGER_H
