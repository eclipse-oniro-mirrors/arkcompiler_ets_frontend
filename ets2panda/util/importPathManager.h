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

#ifndef ES2PANDA_UTIL_IMPORT_PATH_MANAGER_H
#define ES2PANDA_UTIL_IMPORT_PATH_MANAGER_H

#include "language.h"
#if defined PANDA_TARGET_MOBILE
#define USE_UNIX_SYSCALL
#endif

#include "util/arktsconfig.h"
#include "util/ustring.h"
#include "util/enumbitops.h"
#include "util/path.h"
#include "util/options.h"

namespace ark::es2panda::util {
namespace gen::extension {
enum Enum : size_t;
}  // namespace gen::extension

using ENUMBITOPS_OPERATORS;

enum class ImportFlags {
    NONE = 0U,
    DEFAULT_IMPORT = 1U << 1U,
    IMPLICIT_PACKAGE_IMPORT = 1U << 2U,
    EXTERNAL_BINARY_IMPORT = 1U << 3U,  // means .abc file in "path" in "dependencies"
    EXTERNAL_SOURCE_IMPORT = 1U << 4U   // means .d.ets file in "path" in "dependencies"
};

}  // namespace ark::es2panda::util

namespace enumbitops {
template <>
struct IsAllowedType<ark::es2panda::util::ImportFlags> : std::true_type {
};
}  // namespace enumbitops

namespace ark::es2panda::ir {
class StringLiteral;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::parser {
class ParserContext;
}  // namespace ark::es2panda::parser

namespace ark::es2panda::util {

enum class ModuleKind { MODULE, DECLARATION, PACKAGE };

struct ModuleInfo {
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    StringView moduleName {};
    StringView modulePrefix {};
    ModuleKind kind {};
    // NOTE(dkofanov): Should be refactored and aligned with 'ModuleKind' and
    // 'Program::MaybeTransformToDeclarationModule'.
    bool isDeclForDynamicStaticInterop {};
    // NOLINTEND(misc-non-private-member-variables-in-classes)
    Language lang = Language(Language::Id::ETS);
};

class ImportPathManager {
public:
    static constexpr auto DUMMY_PATH = "dummy_path";  // CC-OFF(G.NAM.03-CPP) project code style
    struct ImportMetadata {
        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        ImportFlags importFlags {};
        Language::Id lang {Language::Id::COUNT};
        std::string_view resolvedSource {};
        std::string_view declPath {};
        std::string ohmUrl {};
        std::string declText {};
        // NOLINTEND(misc-non-private-member-variables-in-classes)

        bool HasSpecifiedDeclPath() const
        {
            return !declPath.empty() && (declPath != DUMMY_PATH);
        }

        bool IsImplicitPackageImported() const
        {
            return (importFlags & ImportFlags::IMPLICIT_PACKAGE_IMPORT) != 0;
        }

        bool IsExternalBinaryImport() const
        {
            return (importFlags & ImportFlags::EXTERNAL_BINARY_IMPORT) != 0;
        }

        bool IsExternalSourceImport() const
        {
            return (importFlags & ImportFlags::EXTERNAL_SOURCE_IMPORT) != 0;
        }

        bool IsValid() const;
    };

    struct ParseInfo {
        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        bool isParsed {};
        ImportMetadata importData;
        // NOLINTEND(misc-non-private-member-variables-in-classes)
    };

    ImportPathManager(ark::ArenaAllocator *allocator, const util::Options &options, parser::Program *globalProgram,
                      util::DiagnosticEngine &diagnosticEngine)
        : allocator_(allocator),
          arktsConfig_(options.ArkTSConfig()),
          absoluteEtsPath_(
              options.GetEtsPath().empty() ? "" : util::Path(options.GetEtsPath(), allocator_).GetAbsolutePath()),
          stdLib_(options.GetStdlib()),
          parseList_(allocator->Adapter()),
          globalProgram_(globalProgram),
          diagnosticEngine_ {diagnosticEngine}
    {
    }

    NO_COPY_SEMANTIC(ImportPathManager);
    NO_MOVE_SEMANTIC(ImportPathManager);
    ImportPathManager() = delete;
    ~ImportPathManager() = default;

    [[nodiscard]] const ArenaVector<ParseInfo> &ParseList() const
    {
        return parseList_;
    }

    [[nodiscard]] ArenaVector<ParseInfo> &ParseList()
    {
        return parseList_;
    }

    void ClearParseList()
    {
        parseList_.clear();
    }

    util::StringView FormModuleName(const util::Path &path, const lexer::SourcePosition &srcPos);
    ImportMetadata GatherImportMetadata(parser::Program *program, ImportFlags importFlags,
                                        ir::StringLiteral *importPath);
    void AddImplicitPackageImportToParseList(StringView packageDir, const lexer::SourcePosition &srcPos);

    // API version for resolving paths. Kept only for API compatibility. Doesn't support 'dependencies'.
    util::StringView ResolvePathAPI(StringView curModulePath, ir::StringLiteral *importPath) const;

    void MarkAsParsed(StringView path);
    util::StringView FormRelativePath(const util::Path &path);
    std::shared_ptr<const ArkTsConfig> ArkTSConfig() const
    {
        return arktsConfig_;
    }

private:
    util::StringView FormModuleNameSolelyByAbsolutePath(const util::Path &path);
    util::StringView FormModuleName(const util::Path &path);

    struct ResolvedPathRes {
        // On successfull resolving, 2 variants are possible:
        // `resolvedPath` is a module-path - if dynamic path was resolved;
        // `resolvedPath` is a realpath - if static path was resolved.
        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
        std::string_view resolvedPath;
        bool resolvedIsExternalModule {false};
        // NOLINTEND(misc-non-private-member-variables-in-classes)
    };
    ResolvedPathRes ResolvePath(std::string_view curModulePath, ir::StringLiteral *importPath) const;
    ResolvedPathRes ResolveAbsolutePath(const ir::StringLiteral &importPathNode) const;
    std::string_view DirOrDirWithIndexFile(StringView dir) const;
    ResolvedPathRes AppendExtensionOrIndexFileIfOmitted(StringView basePath) const;
    std::string TryMatchDependencies(std::string_view fixedPath) const;
    StringView GetRealPath(StringView path) const;
    void ProcessExternalLibraryImport(ImportMetadata &importData);
    std::string_view tryImportFromDeclarationCache(std::string_view resolvedImportPath) const;

public:
    void AddToParseList(const ImportMetadata &importMetadata);
#ifdef USE_UNIX_SYSCALL
    void UnixWalkThroughDirectoryAndAddToParseList(ImportMetadata importMetadata);
#endif

private:
    ark::ArenaAllocator *const allocator_;
    std::shared_ptr<ArkTsConfig> const arktsConfig_;
    std::string absoluteEtsPath_;
    std::string stdLib_;
    ArenaVector<ParseInfo> parseList_;
    parser::Program *globalProgram_;
    util::DiagnosticEngine &diagnosticEngine_;
    std::string_view pathDelimiter_ {ark::os::file::File::GetPathDelim()};
    mutable lexer::SourcePosition srcPos_ {};
    bool isDynamic_ = false;
};

}  // namespace ark::es2panda::util

#endif  // ES2PANDA_UTIL_IMPORT_PATH_MANAGER_H
