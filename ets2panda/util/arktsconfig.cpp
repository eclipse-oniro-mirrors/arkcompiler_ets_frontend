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

#include "arktsconfig.h"
#include "os/filesystem.h"
#include "util/language.h"
#include "util/diagnosticEngine.h"
#include "generated/signatures.h"

#include <fstream>
#include <memory>
#include <regex>
#include <sstream>
#include <system_error>

namespace ark::es2panda {

bool ArkTsConfig::Check(bool cond, const diagnostic::DiagnosticKind &diag, const util::DiagnosticMessageParams &params)
{
    if (!cond) {
        diagnosticEngine_.LogDiagnostic(diag, params);
        return false;
    }
    return true;
}

static bool IsAbsolute(const std::string &path)
{
#ifndef ARKTSCONFIG_USE_FILESYSTEM
    return !path.empty() && path[0] == '/';
#else
    return fs::path(path).is_absolute();
#endif  // ARKTSCONFIG_USE_FILESYSTEM
}

#ifndef ARKTSCONFIG_USE_FILESYSTEM
#else
fs::path NormalizePath(const fs::path &p)
{
    fs::path result;
    for (const auto &part : p) {
        if (part == ".") {
            continue;
        }
        if (part == "..") {
            if (!result.empty() && result.filename() != "..") {
                result = result.parent_path();
            } else {
                result /= part;
            }
        } else {
            result /= part;
        }
    }
    if (fs::exists(result)) {
        return fs::canonical(result);
    }
    return result;
}
#endif  // ARKTSCONFIG_USE_FILESYSTEM

std::string JoinPaths(const std::string &a, const std::string &b)
{
#ifndef ARKTSCONFIG_USE_FILESYSTEM
    return a + '/' + b;
#else
    return NormalizePath(fs::path(a) / b).string();
#endif  // ARKTSCONFIG_USE_FILESYSTEM
}

std::string ParentPath(const std::string &path)
{
#ifndef ARKTSCONFIG_USE_FILESYSTEM
    auto pos = path.find('/');
    return pos == std::string::npos ? path : path.substr(0, pos);
#else
    return fs::path(path).parent_path().string();
#endif  // ARKTSCONFIG_USE_FILESYSTEM
}

static std::string MakeAbsolute(const std::string &path, const std::string &base)
{
    return IsAbsolute(path) ? path : JoinPaths(base, path);
}

#ifdef ARKTSCONFIG_USE_FILESYSTEM

ArkTsConfig::Pattern::Pattern(std::string value, std::string base) : value_(std::move(value)), base_(std::move(base))
{
    ES2PANDA_ASSERT(fs::path(base_).is_absolute());
}

bool ArkTsConfig::Pattern::IsPattern() const
{
    return (value_.find('*') != std::string::npos) || (value_.find('?') != std::string::npos);
}

std::string ArkTsConfig::Pattern::GetSearchRoot() const
{
    fs::path relative;
    if (!IsPattern()) {
        relative = value_;
    } else {
        auto foundStar = value_.find_first_of('*');
        auto foundQuestion = value_.find_first_of('?');
        relative = value_.substr(0, std::min(foundStar, foundQuestion));
        relative = relative.parent_path();
    }
    return MakeAbsolute(relative.string(), base_);
}

bool ArkTsConfig::Pattern::Match(const std::string &path) const
{
    ES2PANDA_ASSERT(fs::path(path).is_absolute());
    fs::path value = fs::path(value_);
    std::string pattern = value.is_absolute() ? value.string() : (base_ / value).string();

    // Replace arktsconfig special symbols with regular expressions
    if (IsPattern()) {
        // '**' matches any directory nested to any level
        pattern = std::regex_replace(pattern, std::regex("\\*\\*/"), ".*");
        // '*' matches zero or more characters (excluding directory separators)
        pattern = std::regex_replace(pattern, std::regex("([^\\.])\\*"), "$1[^/]*");
        // '?' matches any one character (excluding directory separators)
        pattern = std::regex_replace(pattern, std::regex("\\?"), "[^/]");
        // './src' -> 'src'
        pattern = std::regex_replace(pattern, std::regex("\\.\\/"), "");
        // '[^/]*.' -> '[^/]*\.'
        pattern = std::regex_replace(pattern, std::regex(R"(\[\^\/\]\*\.)"), "[^/]*\\.");
    }
    if (!value.has_extension()) {
        // default extensions to match
        pattern += R"(.*(\.ts|\.ets|\.sts)$)";
    }
    std::smatch m;
    auto res = std::regex_match(path, m, std::regex(pattern));
    return res;
}

static std::string ResolveConfigLocation(const std::string &relPath, const std::string &base)
{
    auto resolvedPath = MakeAbsolute(relPath, base);
    auto newBase = base;
    while (!fs::exists(resolvedPath)) {
        resolvedPath = MakeAbsolute(relPath, JoinPaths(newBase, "node_modules"));
        if (newBase == ParentPath(newBase)) {
            return "";
        }
        newBase = ParentPath(newBase);
    }
    return resolvedPath;
}

std::optional<ArkTsConfig> ArkTsConfig::ParseExtends(const std::string &configPath, const std::string &extends,
                                                     const std::string &configDir,
                                                     std::unordered_set<std::string> &parsedConfigPath)
{
    auto basePath = ResolveConfigLocation(extends, configDir);
    if (!Check(!basePath.empty(), diagnostic::UNRESOLVABLE_CONFIG_PATH, {extends})) {
        return {};
    }

    if (!Check(basePath != configPath, diagnostic::CYCLIC_IMPORT, {})) {
        return {};
    }

    auto base = ArkTsConfig(basePath, diagnosticEngine_);
    if (!Check(base.Parse(parsedConfigPath), diagnostic::WRONG_BASE_CONFIG, {extends})) {
        return {};
    }

    return base;
}
#endif  // ARKTSCONFIG_USE_FILESYSTEM

static std::string ValidDynamicLanguages()
{
    JsonArrayBuilder builder;
    for (auto &l : Language::All()) {
        if (l.IsDynamic()) {
            builder.Add(l.ToString());
        }
    }
    return std::move(builder).Build();
}

bool ArkTsConfig::ParsePaths(const JsonObject::JsonObjPointer *options, PathsMap &pathsMap, const std::string &baseUrl)
{
    auto paths = options->get()->GetValue<JsonObject::JsonObjPointer>("paths");
    if (paths == nullptr) {
        return true;
    }

    for (size_t keyIdx = 0; keyIdx < paths->get()->GetSize(); ++keyIdx) {
        auto &key = paths->get()->GetKeyByIndex(keyIdx);
        if (pathsMap.count(key) == 0U) {
            pathsMap.insert({key, {}});
        }

        auto values = paths->get()->GetValue<JsonObject::ArrayT>(key);
        if (!Check(values != nullptr, diagnostic::INVALID_VALUE, {"path", key})) {
            return false;
        }

        if (!Check(!values->empty(), diagnostic::EMPTY_ARRAY_SUBSTITOTIONS, {key})) {
            return false;
        }

        for (auto &v : *values) {
            auto p = *v.Get<JsonObject::StringT>();
            pathsMap[key].emplace_back(MakeAbsolute(p, baseUrl));
        }
    }

    return true;
}

static constexpr auto LANGUAGE = "language";   // CC-OFF(G.NAM.03-CPP) project code style
static constexpr auto DECL_PATH = "declPath";  // CC-OFF(G.NAM.03-CPP) project code style
static constexpr auto OHM_URL = "ohmUrl";      // CC-OFF(G.NAM.03-CPP) project code style

bool ArkTsConfig::ParseDynamicPaths(const JsonObject::JsonObjPointer *options,
                                    std::map<std::string, DynamicImportData, CompareByLength> &dynamicPathsMap,
                                    const std::string &baseUrl)
{
    if (options == nullptr) {
        return true;
    }
    auto dynamicPaths = options->get()->GetValue<JsonObject::JsonObjPointer>("dynamicPaths");
    if (dynamicPaths == nullptr) {
        return true;
    }
    for (size_t keyIdx = 0; keyIdx < dynamicPaths->get()->GetSize(); ++keyIdx) {
        auto &key = dynamicPaths->get()->GetKeyByIndex(keyIdx);
        auto data = dynamicPaths->get()->GetValue<JsonObject::JsonObjPointer>(key);
        if (!ParseSingleDynamicPath(key, data, dynamicPathsMap, baseUrl)) {
            return false;
        }
    }
    return true;
}

bool ArkTsConfig::ParseSingleDynamicPath(const std::string &key, const JsonObject::JsonObjPointer *data,
                                         std::map<std::string, DynamicImportData, CompareByLength> &dynamicPathsMap,
                                         const std::string &baseUrl)
{
    if (IsAbsolute(key)) {
        diagnosticEngine_.LogDiagnostic(diagnostic::DYNAMIC_PATHS_ABSOLUTE, util::DiagnosticMessageParams {key});
    }
    if (!Check(data != nullptr, diagnostic::INVALID_VALUE, {"dynamic path", key})) {
        return false;
    }
    auto langValue = data->get()->GetValue<JsonObject::StringT>(LANGUAGE);
    if (!Check(langValue != nullptr, diagnostic::INVALID_LANGUAGE, {LANGUAGE, key, ValidDynamicLanguages()})) {
        return false;
    }
    auto lang = Language::FromString(*langValue);
    if (!Check(lang && lang->IsDynamic(), diagnostic::INVALID_LANGUAGE, {LANGUAGE, key, ValidDynamicLanguages()})) {
        return false;
    }
    auto isSupportLang = compiler::Signatures::Dynamic::IsSupported(*lang);
    if (!Check(isSupportLang, diagnostic::UNSUPPORTED_LANGUAGE_FOR_INTEROP, {lang->ToString()})) {
        return false;
    }
    auto ohmUrl = data->get()->GetValue<JsonObject::StringT>(OHM_URL);
    if (ohmUrl == nullptr) {
        diagnosticEngine_.LogDiagnostic(diagnostic::NO_OHMURL, util::DiagnosticMessageParams {key});
    }
    std::string ohmUrlValue = (ohmUrl == nullptr) ? "" : *ohmUrl;
    auto declPathValue = data->get()->GetValue<JsonObject::StringT>(DECL_PATH);
    std::string normalizedDeclPath {};
    if (declPathValue != nullptr) {
        normalizedDeclPath = IsAbsolute(*declPathValue) ? ark::os::GetAbsolutePath(*declPathValue)
                                                        : MakeAbsolute(*declPathValue, baseUrl);
        if (!Check(ark::os::IsFileExists(normalizedDeclPath), diagnostic::INVALID_DYNAMIC_PATH, {key})) {
            return false;
        }
    }
    auto res = dynamicPathsMap.insert(
        {ark::os::NormalizePath(key), ArkTsConfig::DynamicImportData(*lang, normalizedDeclPath, ohmUrlValue)});
    return Check(res.second, diagnostic::DUPLICATED_DYNAMIC_PATH, {normalizedDeclPath, key});
}

template <class Collection, class Function>
bool ArkTsConfig::ParseCollection(const JsonObject *config, Collection &out, const std::string &target,
                                  Function &&constructor)
{
    auto *arr = config->GetValue<JsonObject::ArrayT>(target);
    if (!Check(arr != nullptr, diagnostic::INVALID_JSON_TYPE, {target, "array"})) {
        return false;
    }

    out = {};
    if (!Check(!arr->empty(), diagnostic::EMPTY_LIST, {target})) {
        return false;
    }

    for (auto &i : *arr) {
        out.emplace_back(constructor(*i.Get<JsonObject::StringT>()));
    }

    return true;
}

void ArkTsConfig::ResolveConfigDependencies(std::unordered_map<std::string, std::shared_ptr<ArkTsConfig>> &dependencies,
                                            std::vector<std::string> &dependencyPaths,
                                            std::unordered_set<std::string> &parsedConfigPath)
{
    for (auto dependency : dependencyPaths) {
        auto config = std::make_shared<ArkTsConfig>(std::string_view(dependency), diagnosticEngine_);
        if (!Check(config->Parse(parsedConfigPath), diagnostic::EMPTY_LIST, {dependency})) {
            continue;
        }
        dependencies.emplace(config->Package(), std::move(config));
    }
}

std::optional<std::string> ArkTsConfig::ReadConfig(const std::string &path)
{
    std::ifstream inputStream(path);
    if (!Check(!inputStream.fail(), diagnostic::FAILED_TO_OPEN_FILE, {path})) {
        return {};
    }

    std::stringstream ss;
    ss << inputStream.rdbuf();
    return ss.str();
}

static std::string ValueOrEmptyString(const JsonObject::JsonObjPointer *json, const std::string &key)
{
    ES2PANDA_ASSERT(json != nullptr);
    auto res = json->get()->GetValue<JsonObject::StringT>(key);
    return (res != nullptr) ? *res : "";
}

static void ResolvePathInDependenciesImpl(ArkTsConfig *arktsConfig,
                                          std::map<std::string, std::vector<std::string>, CompareByLength> &paths,
                                          std::unordered_map<std::string, std::string> &entries)
{
    for (const auto &dependencyPath : arktsConfig->Paths()) {
        paths.emplace(dependencyPath.first, dependencyPath.second);
    }
    if (!arktsConfig->Entry().empty()) {
        entries.emplace(arktsConfig->Package(), arktsConfig->Entry());
    }
    for (const auto &config : arktsConfig->Dependencies()) {
        ResolvePathInDependenciesImpl(config.second.get(), paths, entries);
    }
}

void ArkTsConfig::ResolveAllDependenciesInArkTsConfig()
{
    ResolvePathInDependenciesImpl(this, paths_, entries_);
}

bool ArkTsConfig::ParseCompilerOptions(std::string &arktsConfigDir, std::unordered_set<std::string> &parsedConfigPath,
                                       const JsonObject *arktsConfig)
{
    auto compilerOptions = arktsConfig->GetValue<JsonObject::JsonObjPointer>(COMPILER_OPTIONS);
    // Parse "package"
    package_ = ValueOrEmptyString(compilerOptions, PACKAGE);

    // Parse "baseUrl", "outDir", "rootDir"
    baseUrl_ = MakeAbsolute(ValueOrEmptyString(compilerOptions, BASE_URL), arktsConfigDir);
    outDir_ = MakeAbsolute(ValueOrEmptyString(compilerOptions, OUT_DIR), arktsConfigDir);
    rootDir_ = MakeAbsolute(ValueOrEmptyString(compilerOptions, ROOT_DIR), arktsConfigDir);

    // Parse "entry"
    if (compilerOptions->get()->HasKey(ENTRY)) {
        entry_ = MakeAbsolute(ValueOrEmptyString(compilerOptions, ENTRY), baseUrl_);
    }

    // Parse "useUrl"
    if (compilerOptions->get()->HasKey(USE_EMPTY_PACKAGE)) {
        auto *useUrl = compilerOptions->get()->GetValue<JsonObject::BoolT>(USE_EMPTY_PACKAGE);
        ES2PANDA_ASSERT(useUrl != nullptr);
        useUrl_ = *useUrl;
    }

    // Parse "dependencies"
    auto concatPath = [this](const auto &val) { return MakeAbsolute(val, baseUrl_); };
    std::vector<std::string> dependencyPaths;
    if (compilerOptions->get()->HasKey(DEPENDENCIES)) {
        ParseCollection(compilerOptions->get(), dependencyPaths, DEPENDENCIES, concatPath);
    }
    if (!dependencyPaths.empty()) {
        ResolveConfigDependencies(dependencies_, dependencyPaths, parsedConfigPath);
    }
    // Parse "paths"
    if (!ParsePaths(compilerOptions, paths_, baseUrl_)) {
        return false;
    }
    // Parse "dynamicPaths"
    if (!ParseDynamicPaths(compilerOptions, dynamicPaths_, baseUrl_)) {
        return false;
    }
    return true;
}

// CC-OFFNXT(huge_method[C++], G.FUN.01-CPP, G.FUD.05) solid logic
bool ArkTsConfig::Parse(std::unordered_set<std::string> &parsedConfigPath)
{
    // For circurlar dependencies, just skip parsing
    if (parsedConfigPath.find(configPath_) != parsedConfigPath.end()) {
        return true;
    }
    parsedConfigPath.emplace(configPath_);

    ES2PANDA_ASSERT(!isParsed_);
    isParsed_ = true;
    auto arktsConfigDir = ParentPath(ark::os::GetAbsolutePath(configPath_));

    // Read input
    auto tsConfigSource = ReadConfig(configPath_);
    if (!Check(tsConfigSource.has_value(), diagnostic::UNRESOLVABLE_CONFIG_PATH, {configPath_})) {
        return false;
    }

    // Parse json
    auto arktsConfig = std::make_unique<JsonObject>(*tsConfigSource);
    if (!Check(arktsConfig->IsValid(), diagnostic::INVALID_JSON, {})) {
        return false;
    }

#ifdef ARKTSCONFIG_USE_FILESYSTEM
    // Parse "extends"
    if (arktsConfig->HasKey(EXTENDS)) {
        auto *extends = arktsConfig->GetValue<JsonObject::StringT>(EXTENDS);
        if (!Check(extends != nullptr, diagnostic::INVALID_JSON_TYPE, {EXTENDS, "string"})) {
            return false;
        }
        const auto &base = ParseExtends(configPath_, *extends, arktsConfigDir, parsedConfigPath);
        if (!base.has_value()) {
            return false;
        }
        Inherit(*base);
    }
#endif  // ARKTSCONFIG_USE_FILESYSTEM

    // Parse "compilerOptions"
    if (arktsConfig->HasKey(COMPILER_OPTIONS) &&
        !ParseCompilerOptions(arktsConfigDir, parsedConfigPath, arktsConfig.get())) {
        return false;
    }

    // Parse "files"
    auto concatPath = [&arktsConfigDir](const auto &val) { return MakeAbsolute(val, arktsConfigDir); };
    if (arktsConfig->HasKey(FILES) && !ParseCollection(arktsConfig.get(), files_, FILES, concatPath)) {
        return false;
    }

#ifdef ARKTSCONFIG_USE_FILESYSTEM
    // Parse "include" and "exclude"
    auto consPattern = [&arktsConfigDir](const auto &val) { return Pattern {val, arktsConfigDir}; };
    if (arktsConfig->HasKey(INCLUDE) && !ParseCollection(arktsConfig.get(), include_, INCLUDE, consPattern)) {
        return false;
    }
    if (arktsConfig->HasKey(EXCLUDE) && !ParseCollection(arktsConfig.get(), exclude_, EXCLUDE, consPattern)) {
        return false;
    }
#endif  // ARKTSCONFIG_USE_FILESYSTEM

    return true;
}

void ArkTsConfig::Inherit(const ArkTsConfig &base)
{
    package_ = base.package_;
    baseUrl_ = base.baseUrl_;
    outDir_ = base.outDir_;
    rootDir_ = base.rootDir_;
    paths_ = base.paths_;
    files_ = base.files_;
#ifdef ARKTSCONFIG_USE_FILESYSTEM
    include_ = base.include_;
    exclude_ = base.exclude_;
#endif  // ARKTSCONFIG_USE_FILESYSTEM
}

// Remove '/' and '*' from the end of path
static std::string TrimPath(const std::string &path)
{
    std::string trimmedPath = path;
    while (!trimmedPath.empty() && (trimmedPath.back() == '*' || trimmedPath.back() == '/')) {
        trimmedPath.pop_back();
    }
    return trimmedPath;
}

std::optional<std::string> ArkTsConfig::ResolvePath(std::string_view path, bool isDynamic) const
{
    auto tryResolveWithPaths = [this, &path]() -> std::optional<std::string> {
        for (const auto &[alias, paths] : paths_) {
            auto trimmedAlias = TrimPath(alias);
            size_t pos = path.rfind(trimmedAlias, 0);
            if (pos == 0) {
                auto resolved = std::string(path);
                // NOTE(ivagin): arktsconfig contains array of paths for each prefix, for now just get first one
                std::string newPrefix = TrimPath(paths[0]);
                resolved.replace(pos, trimmedAlias.length(), newPrefix);
                return resolved;
            }
        }
        return std::nullopt;
    };

    auto tryResolveWithDynamicPaths = [this, &path]() -> std::optional<std::string> {
        auto normalizedPath = ark::os::NormalizePath(std::string(path));
        for (const auto &[dynPath, _] : dynamicPaths_) {
            // NOTE(dkofanov): #23877. Fail, if there is no direct match of normalized dynamic module path.
            // It may be worth to take an attempt to resolve 'path' as relative to some defined dynamicPath in order to
            // keep 'arktsconfig.json's smaller.
            if (normalizedPath == dynPath) {
                return dynPath;
            }
        }
        return std::nullopt;
    };

    if (isDynamic) {
        auto result = tryResolveWithDynamicPaths();
        if (result != std::nullopt) {
            return result;
        }
        // NOTE: #26150. we fall to tryResolveWithPaths in this case 1.2->1.0->1.2
        return tryResolveWithPaths();
    }
    auto result = tryResolveWithPaths();
    if (result != std::nullopt) {
        return result;
    }
    return tryResolveWithDynamicPaths();
}

#ifdef ARKTSCONFIG_USE_FILESYSTEM
static bool MatchExcludes(const fs::path &path, const std::vector<ArkTsConfig::Pattern> &excludes)
{
    for (auto &e : excludes) {
        if (e.Match(path.string())) {
            return true;
        }
    }
    return false;
}

std::vector<fs::path> GetSourceList(const std::shared_ptr<ArkTsConfig> &arktsConfig)
{
    auto includes = arktsConfig->Include();
    auto excludes = arktsConfig->Exclude();
    auto files = arktsConfig->Files();

    // If "files" and "includes" are empty - include everything from tsconfig root
    auto configDir = fs::absolute(fs::path(arktsConfig->ConfigPath())).parent_path();
    if (files.empty() && includes.empty()) {
        includes = {ArkTsConfig::Pattern("**/*", configDir.string())};
    }
    // If outDir in not default add it into exclude
    if (fs::exists(arktsConfig->OutDir()) && !fs::equivalent(arktsConfig->OutDir(), configDir)) {
        excludes.emplace_back("**/*", arktsConfig->OutDir());
    }

    // Collect "files"
    std::vector<fs::path> sourceList;
    for (auto &f : files) {
        if (!arktsConfig->Check(fs::exists(f) && fs::path(f).has_filename(), diagnostic::NO_FILE, {f})) {
            return {};
        }

        sourceList.emplace_back(f);
    }

    // Collect "include"
    // TSC traverses folders for sources starting from 'include' rather than from 'rootDir', so we do the same
    for (auto &include : includes) {
        auto traverseRoot = fs::path(include.GetSearchRoot());
        if (!fs::exists(traverseRoot)) {
            continue;
        }
        if (!fs::is_directory(traverseRoot)) {
            if (include.Match(traverseRoot.string()) && !MatchExcludes(traverseRoot, excludes)) {
                sourceList.emplace_back(traverseRoot);
            }
            continue;
        }
        for (const auto &dirEntry : fs::recursive_directory_iterator(traverseRoot)) {
            if (include.Match(dirEntry.path().string()) && !MatchExcludes(dirEntry, excludes) &&
                !fs::is_directory(dirEntry)) {
                sourceList.emplace_back(dirEntry);
            }
        }
    }
    return sourceList;
}

// Analogue of 'std::filesystem::relative()'
// Example: Relative("/a/b/c", "/a/b") returns "c"
static fs::path Relative(const fs::path &src, const fs::path &base)
{
    fs::path tmpPath = src;
    fs::path relPath;
    while (!fs::equivalent(tmpPath, base)) {
        relPath = relPath.empty() ? tmpPath.filename() : tmpPath.filename() / relPath;
        if (tmpPath == tmpPath.parent_path()) {
            return fs::path();
        }
        tmpPath = tmpPath.parent_path();
    }
    return relPath;
}

// Compute path to destination file and create subfolders
fs::path ArkTsConfig::ComputeDestination(const fs::path &src, const fs::path &rootDir, const fs::path &outDir)
{
    auto rel = Relative(src, rootDir);
    std::stringstream sRootDir;
    sRootDir << rootDir;
    std::stringstream sSrc;
    sSrc << src;
    if (!Check(!rel.empty(), diagnostic::NOT_ROOT_DIR, {sRootDir.str(), sSrc.str()})) {
        return {};
    }

    // CC-OFFNXT(G.EXP.22-CPP) false positive, overloaded operator
    auto dst = outDir / rel;
    fs::create_directories(dst.parent_path());
    return dst.replace_extension("abc");
}

std::vector<std::pair<std::string, std::string>> FindProjectSources(const std::shared_ptr<ArkTsConfig> &arktsConfig)
{
    auto sourceFiles = GetSourceList(arktsConfig);
    std::vector<std::pair<std::string, std::string>> compilationList;
    for (auto &src : sourceFiles) {
        auto dst = arktsConfig->ComputeDestination(src, arktsConfig->RootDir(), arktsConfig->OutDir());
        if (!arktsConfig->Check(!dst.empty(), diagnostic::INVALID_DESTINATION_FILE, {})) {
            return {};
        }

        compilationList.emplace_back(src.string(), dst.string());
    }

    return compilationList;
}
#else
std::vector<std::pair<std::string, std::string>> FindProjectSources(
    // CC-OFFNXT(G.FMT.06-CPP) project code style
    [[maybe_unused]] const std::shared_ptr<ArkTsConfig> &arkts_config)
{
    ES2PANDA_ASSERT(false);
    return {};
}
#endif  // ARKTSCONFIG_USE_FILESYSTEM

}  // namespace ark::es2panda
