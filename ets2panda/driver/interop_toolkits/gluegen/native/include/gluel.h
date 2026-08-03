/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef APPS_GLUEGEN_NATIVE_INCLUDE_GLUEL_H
#define APPS_GLUEGEN_NATIVE_INCLUDE_GLUEL_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "nlohmann/json.hpp"
#include "gluec.h"
#include "symbol.h"
#include "utils.h"

namespace ark::es2panda::gluegen {

struct GlueFileConfig {
    std::unordered_map<std::string, SymbolNode *> root;
    std::string globalClassDescriptor;
};

inline void to_json(nlohmann::json &j, const GlueFileConfig &fileConfig)
{
    auto rootJson = nlohmann::json::object();
    for (const auto &[name, node] : fileConfig.root) {
        rootJson[name] = SymbolNodeToJson(*node);
    }
    j["root"] = std::move(rootJson);
    j["globalClassDescriptor"] = fileConfig.globalClassDescriptor;
}

inline void GlueFileConfigFromJson(const nlohmann::json &j, GlueFileConfig &fileConfig,
                                   SymbolNodeManager &symbolNodeManager)
{
    fileConfig.root.clear();
    for (const auto &[name, nodeJson] : j.at("root").items()) {
        fileConfig.root[name] = SymbolNodeFromJson(nodeJson, symbolNodeManager);
    }
    // Tolerate a GlueConfig written by an older gluegen build, before this field existed.
    fileConfig.globalClassDescriptor =
        j.contains("globalClassDescriptor") ? j.at("globalClassDescriptor").get<std::string>() : std::string();
}

struct GlueConfig {
    // Keyed by each linked target's source file path, always in `ToForwardSlashPath` form (a
    // forward-slash path regardless of host platform) so the emitted JSON's keys are portable and
    // deterministic across the platform that produced them and whatever platform later reads them
    // back -- unlike every other path in this codebase (e.g. `IntermediateCache::sourceFile`,
    // `ExternalLinks::sourceFile`), which intentionally follows the current platform's native
    // separator via plain `NormalizePath`. See `Gluel::Link()`/`GlueConfigFromJson`.
    std::unordered_map<std::string, GlueFileConfig> files;
    // "success" for a normal Gluegen::Run() (the only way a GlueConfig is ever actually
    // constructed by Gluel::Link() -- see Gluegen::Run(), which writes a separate, GlueConfig-less
    // `{"status": "syntax-error"}` file directly when Initialize() fails due to a source syntax
    // error, without ever reaching Link()/GlueConfig at all).
    std::string status = "success";

    static std::string serialize(const GlueConfig &config);
    static GlueConfig deserialize(const std::string &jsonStr, SymbolNodeManager &symbolNodeManager);
};

// GlueConfig's `files` map is serializable via GlueFileConfig's (manual) to_json above, so
// nlohmann's automatic container serialization handles it once GlueConfig's own to_json is
// written. from_json cannot use the same NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE macro this used to,
// though: the macro would generate an ADL from_json that internally needs GlueFileConfig's own
// from_json, which no longer exists in ADL form (see GlueFileConfigFromJson above). Deserializing
// a GlueConfig therefore goes through the plain GlueConfigFromJson function below instead.
inline void to_json(nlohmann::json &j, const GlueConfig &config)
{
    j["files"] = config.files;
    j["status"] = config.status;
}

inline void GlueConfigFromJson(const nlohmann::json &j, GlueConfig &config, SymbolNodeManager &symbolNodeManager)
{
    config.files.clear();
    for (const auto &[name, fileJson] : j.at("files").items()) {
        GlueFileConfig fileConfig;
        GlueFileConfigFromJson(fileJson, fileConfig, symbolNodeManager);
        // Force the forward-slash form even though Gluel::Link() already writes keys this way --
        // defends against a JSON produced by some other/older producer that didn't, so the
        // in-memory `files` map key is always forward-slash regardless of where the JSON came from.
        config.files[ToForwardSlashPath(name)] = std::move(fileConfig);
    }
    j.at("status").get_to(config.status);
}

// Maps an exported symbol name to its SymbolNode. Used while resolving/merging re-exports.
using ExportedSymbols = std::unordered_map<std::string, SymbolNode *>;

class Gluel {
public:
    Gluel(const std::vector<std::shared_ptr<IntermediateCache>> &intermediateCaches, Context &context);
    ~Gluel() = default;

    NO_COPY_SEMANTIC(Gluel);

    Expected<std::unique_ptr<GlueConfig>, std::string> Link(const std::vector<std::string> &targets);

private:
    // Fully resolves the set of symbols exported by the module backed by `cache`, expanding its
    // re-export externals (recursively into the modules they re-export from). The result maps an
    // exported name to its SymbolNode. Memoized in `resolvedExports_`; cycles are broken via the
    // `resolving_` guard (a module already on the resolution stack contributes only its own
    // directly-declared exports, not its re-exports, to avoid infinite recursion).
    ExportedSymbols ResolveExports(const IntermediateCache *cache);

    // Applies a single static re-export link `ext` (whose target module's fully-resolved exports
    // are `otherExports`) onto the `result` symbol map being built, following the merge rules for
    // `export *`, `export * as ns`, and `export { a as b, c }` re-exports.
    void ApplyStaticExternal(const ExternalLinks &ext, const ExportedSymbols &otherExports, ExportedSymbols &result);

    // Handles the NAMESPACE kind of `ext` (`export *` / `export * as ns`), extracted from
    // ApplyStaticExternal to keep the nesting depth of that function within limits.
    void ApplyStaticNamespaceExternal(const ExternalLinks &ext, const ExportedSymbols &otherExports,
                                      ExportedSymbols &result);

    // Applies a single dynamic (interop) re-export link `ext` onto `result`. The target module has
    // no intermediate cache to expand, so instead of splicing in resolved symbols this records
    // SymbolKind::DYNAMIC_RE_EXPORT nodes that carry the raw source specifier and the re-exported
    // names (or the namespace form for `export *` / `export * as ns`).
    void ApplyDynamicExternal(const ExternalLinks &ext, ExportedSymbols &result);

    // Creates a copy of `src` exposed under `newName` (used when a re-export renames a symbol,
    // e.g. `export { B as C }`), preserving its kind/runtimeName/localName/children.
    SymbolNode *CloneSymbolWithName(const SymbolNode *src, const std::string &newName);

    std::vector<std::shared_ptr<IntermediateCache>> intermediateCaches_;
    // Owns the SymbolNodeManager every SymbolNode created while linking is allocated through --
    // see CloneSymbolWithName/ApplyStaticExternal/ApplyDynamicExternal.
    Context &context_;
    // Index of every known intermediate cache, keyed by its (normalized, absolute) source file
    // path, so re-export links (ExternalLinks::sourceFile) can be resolved to the cache of the
    // module they point at.
    std::unordered_map<std::string, const IntermediateCache *> cacheIndex_;
    // Memoized ResolveExports() results, keyed by source file path.
    std::unordered_map<std::string, ExportedSymbols> resolvedExports_;
    // Source files currently being resolved (the ResolveExports recursion stack), used to detect
    // and break re-export cycles.
    std::unordered_set<std::string> resolving_;
};
}  // namespace ark::es2panda::gluegen
#endif  // APPS_GLUEGEN_NATIVE_INCLUDE_GLUEL_H
