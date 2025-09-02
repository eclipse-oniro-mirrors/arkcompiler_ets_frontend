/**
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_AOT_ARKTSCONFIG_H
#define ES2PANDA_AOT_ARKTSCONFIG_H

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "util/language.h"
#include "util/diagnostic.h"
#include "utils/json_builder.h"
#include "utils/json_parser.h"

// NOTE(ivagin): If ARKTSCONFIG_USE_FILESYSTEM is not defined part of ArkTsConfig functionality is disabled.
//       Only build configuration which prevents us from usage of std::filesystem is "MOBILE" build
//       because of lack of std::filesystem in our current version of mobile nativ development kit.
//       nativ development kit version should be updated and ARKTSCONFIG_USE_FILESYSTEM removed
#if not defined PANDA_TARGET_MOBILE
#define ARKTSCONFIG_USE_FILESYSTEM
#endif

#ifndef ARKTSCONFIG_USE_FILESYSTEM
#include <dirent.h>
#include <sys/types.h>
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
class DiagnosticEngine;
}  // namespace ark::es2panda::util

namespace ark::es2panda {

struct CompareByLength {
    bool operator()(const std::string &x, const std::string &y) const
    {
        return x.size() == y.size() ? x > y : x.size() > y.size();
    }
};

class ArkTsConfig {
    using PathsMap = std::map<std::string, std::vector<std::string>, CompareByLength>;

public:
#ifdef ARKTSCONFIG_USE_FILESYSTEM
    // Pattern describes arktsconfig path pattern for 'include' or 'exclude' properties
    // e.g. src/**, src/**/*, src/util?.ts
    class Pattern {
    public:
        Pattern() = default;
        Pattern(std::string value, std::string base);

        bool IsPattern() const;

        // Get root from which sources file search should be started
        // e.g. src/** -> src; src/index?.ts -> src; src/component/*/index.ts -> src/component; src/index* -> src/
        std::string GetSearchRoot() const;

        // Test if absolute path is matched by pattern
        bool Match(const std::string &path) const;

    private:
        std::string value_ {};
        std::string base_ {};
    };
#endif  // ARKTSCONFIG_USE_FILESYSTEM

    class DynamicImportData {
    public:
        explicit DynamicImportData(Language lang, std::string declPath, std::string ohmUrl)
            : lang_(lang), declPath_(std::move(declPath)), ohmUrl_(std::move(ohmUrl))
        {
        }

        Language GetLanguage() const
        {
            return lang_;
        }

        std::string_view DeclPath() const
        {
            return declPath_;
        }

        std::string_view OhmUrl() const
        {
            return ohmUrl_;
        }

    private:
        Language lang_;
        std::string declPath_ {};
        std::string ohmUrl_ {};
    };

    explicit ArkTsConfig(std::string_view configPath, util::DiagnosticEngine &de)
        : configPath_(configPath), diagnosticEngine_(de)
    {
    }
    bool Parse(std::unordered_set<std::string> &parsedConfigPath);

    std::optional<std::string> ResolvePath(std::string_view path, bool isDynamic = false) const;

    void ResolveAllDependenciesInArkTsConfig();

    const std::string &ConfigPath() const
    {
        return configPath_;
    }

    const std::string &Package() const
    {
        return package_;
    }
    const std::string &BaseUrl() const
    {
        return baseUrl_;
    }
    const std::string &RootDir() const
    {
        return rootDir_;
    }
    const std::string &OutDir() const
    {
        return outDir_;
    }
    const std::unordered_map<std::string, std::shared_ptr<ArkTsConfig>> &Dependencies() const
    {
        return dependencies_;
    }
    void ResetDependencies()
    {
        return dependencies_.clear();
    }
    const std::string &Entry() const
    {
        return entry_;
    }
    const std::unordered_map<std::string, std::string> &Entries() const
    {
        return entries_;
    }
    const std::vector<std::string> &Files() const
    {
        return files_;
    }
    const bool &UseUrl() const
    {
        return useUrl_;
    }
    const PathsMap &Paths() const
    {
        return paths_;
    }
    const std::map<std::string, DynamicImportData, CompareByLength> &DynamicPaths() const
    {
        return dynamicPaths_;
    }
#ifdef ARKTSCONFIG_USE_FILESYSTEM
    const std::vector<Pattern> &Include() const
    {
        return include_;
    }
    const std::vector<Pattern> &Exclude() const
    {
        return exclude_;
    }
    fs::path ComputeDestination(const fs::path &src, const fs::path &rootDir, const fs::path &outDir);
#endif  // ARKTSCONFIG_USE_FILESYSTEM
    bool Check(bool cond, const diagnostic::DiagnosticKind &diag, const util::DiagnosticMessageParams &params);

private:
    bool ParseCompilerOptions(std::string &arktsConfigDir, std::unordered_set<std::string> &parsedConfigPath,
                              const JsonObject *arktsConfig);
    std::optional<ArkTsConfig> ParseExtends(const std::string &configPath, const std::string &extends,
                                            const std::string &configDir,
                                            std::unordered_set<std::string> &parsedConfigPath);
    bool ParsePaths(const JsonObject::JsonObjPointer *options, PathsMap &pathsMap, const std::string &baseUrl);
    bool ParseDynamicPaths(const JsonObject::JsonObjPointer *options,
                           std::map<std::string, DynamicImportData, CompareByLength> &dynamicPathsMap,
                           const std::string &baseUrl);
    bool ParseSingleDynamicPath(const std::string &key, const JsonObject::JsonObjPointer *data,
                                std::map<std::string, DynamicImportData, CompareByLength> &dynamicPathsMap,
                                const std::string &baseUrl);
    template <class Collection, class Function>
    bool ParseCollection(const JsonObject *config, Collection &out, const std::string &target, Function &&constructor);
    void ResolveConfigDependencies(std::unordered_map<std::string, std::shared_ptr<ArkTsConfig>> &dependencies,
                                   std::vector<std::string> &dependencyPaths,
                                   std::unordered_set<std::string> &parsedConfigPath);
    std::optional<std::string> ReadConfig(const std::string &path);

private:
    static constexpr const char *PACKAGE = "package";  // CC-OFF(G.NAM.03,G.NAM.03-CPP) project code style
    static constexpr const char *BASE_URL = "baseUrl";
    static constexpr const char *COMPILER_OPTIONS = "compilerOptions";
    static constexpr const char *EXCLUDE = "exclude";
    static constexpr const char *EXTENDS = "extends";
    static constexpr const char *FILES = "files";
    static constexpr const char *USE_EMPTY_PACKAGE = "useEmptyPackage";  // CC-OFF(G.NAM.03-CPP) project code style
    static constexpr const char *INCLUDE = "include";
    static constexpr const char *OUT_DIR = "outDir";
    static constexpr const char *ROOT_DIR = "rootDir";
    static constexpr const char *DEPENDENCIES = "dependencies";  // CC-OFF(G.NAM.03,G.NAM.03-CPP) project code style
    static constexpr const char *ENTRY = "entry";                // CC-OFF(G.NAM.03,G.NAM.03-CPP) project code style

    void Inherit(const ArkTsConfig &base);

    bool isParsed_ = false;
    std::string configPath_;

    std::string package_ {};
    std::string baseUrl_ {};
    bool useUrl_ = false;
    std::string outDir_ {};
    std::string rootDir_ {};
    std::string entry_ {};
    PathsMap paths_ {};
    std::map<std::string, DynamicImportData, CompareByLength> dynamicPaths_ {};
    std::vector<std::string> files_ {};
    std::unordered_map<std::string, std::shared_ptr<ArkTsConfig>> dependencies_ {};
    // key: package name; value: entry's absolute path
    std::unordered_map<std::string, std::string> entries_;
#ifdef ARKTSCONFIG_USE_FILESYSTEM
    std::vector<Pattern> include_ {};
    std::vector<Pattern> exclude_ {};
#endif  // ARKTSCONFIG_USE_FILESYSTEM
    util::DiagnosticEngine &diagnosticEngine_;
};

// Find source files and compute destination locations
// Return: vector of path pairs <source file, destination abc file>
std::vector<std::pair<std::string, std::string>> FindProjectSources(const std::shared_ptr<ArkTsConfig> &arktsConfig);

std::string JoinPaths(const std::string &a, const std::string &b);
std::string ParentPath(const std::string &path);

}  // namespace ark::es2panda

#endif  // ES2PANDA_AOT_TSCONFIG_H
