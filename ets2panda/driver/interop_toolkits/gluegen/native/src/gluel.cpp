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
#include "gluel.h"
#include "utils.h"

namespace ark::es2panda::gluegen {
namespace {
constexpr const char *DEFAULT_EXPORT_NAME = "default";
// localName marker on a DYNAMIC_RE_EXPORT node meaning "the whole module namespace"
// (`export *` / `export * as ns`), as opposed to a single named binding.
constexpr const char *NAMESPACE_MARKER = "*";
}  // namespace

std::string GlueConfig::serialize(const GlueConfig &config)
{
    nlohmann::json j = config;
    constexpr int indent = 2;  // human-readable, not compact
    return j.dump(indent);
}

GlueConfig GlueConfig::deserialize(const std::string &jsonStr, SymbolNodeManager &symbolNodeManager)
{
    nlohmann::json j = nlohmann::json::parse(jsonStr);
    GlueConfig config;
    GlueConfigFromJson(j, config, symbolNodeManager);
    return config;
}

Gluel::Gluel(const std::vector<std::shared_ptr<IntermediateCache>> &intermediateCaches, Context &context)
    : intermediateCaches_(intermediateCaches), context_(context)
{
}

SymbolNode *Gluel::CloneSymbolWithName(const SymbolNode *src, const std::string &newName)
{
    auto *copy = context_.symbolNodeManager.CreateSymbolNode(newName, src->kind, src->runtimeName, src->localName,
                                                             src->initModuleParam);
    copy->children = src->children;
    return copy;
}

void Gluel::ApplyStaticNamespaceExternal(const ExternalLinks &ext, const ExportedSymbols &otherExports,
                                         ExportedSymbols &result)
{
    if (ext.exportedName.has_value()) {
        // export * as ns from 'other': add a namespace node whose children are all of
        // 'other's non-default exports.
        auto *nsNode = context_.symbolNodeManager.CreateSymbolNode(*ext.exportedName, SymbolKind::NAMESPACE);
        for (const auto &[name, node] : otherExports) {
            if (name != DEFAULT_EXPORT_NAME) {
                nsNode->children[name] = node;
            }
        }
        result[*ext.exportedName] = nsNode;
        return;
    }
    // export * from 'other': splice 'other's non-default exports directly into result.
    // Locally-declared symbols (and earlier-applied re-exports) take precedence on a name
    // clash, so use emplace rather than overwrite.
    for (const auto &[name, node] : otherExports) {
        if (name != DEFAULT_EXPORT_NAME) {
            result.emplace(name, node);
        }
    }
}

void Gluel::ApplyStaticExternal(const ExternalLinks &ext, const ExportedSymbols &otherExports, ExportedSymbols &result)
{
    if (ext.kind == ExternalLinkKind::NAMESPACE) {
        ApplyStaticNamespaceExternal(ext, otherExports, result);
        return;
    }

    // NAMED: export { importedName as exportedName, ... } from 'other'. Take exactly the listed
    // symbols (including `default` when explicitly named), exposing each under its exportedName.
    for (const auto &binding : ext.bindings) {
        auto it = otherExports.find(binding.importedName);
        if (it == otherExports.end()) {
            context_.diagnosticEngine.Warning(DiagnosticCode::MISSING_EXPORT_BINDING,
                                              "Named re-export binding '" + binding.importedName + "' not found in '" +
                                                  ext.sourceFile + "' (re-exported as '" + binding.exportedName + "')");
            continue;
        }
        SymbolNode *node = it->second;
        if (binding.exportedName != node->name) {
            node = CloneSymbolWithName(node, binding.exportedName);
        }
        result[binding.exportedName] = node;
    }
}

void Gluel::ApplyDynamicExternal(const ExternalLinks &ext, ExportedSymbols &result)
{
    if (ext.kind == ExternalLinkKind::NAMESPACE) {
        // export * as ns from 'dyn'  (exportedName = "ns")  OR  export * from 'dyn' (no alias).
        // A whole-namespace re-export can't be enumerated (the dynamic module has no cache), so
        // record a single DYNAMIC_RE_EXPORT node marked with localName "*". When aliased it is
        // keyed by the alias; the bare `export *` form is keyed by the source specifier so distinct
        // dynamic modules don't collide.
        const std::string key = ext.exportedName.value_or(ext.sourceFile);
        auto *node =
            context_.symbolNodeManager.CreateSymbolNode(key, SymbolKind::DYNAMIC_RE_EXPORT, std::nullopt,
                                                        std::string(NAMESPACE_MARKER), std::nullopt, ext.sourceFile);
        result[key] = node;
        return;
    }

    // NAMED: export { importedName as exportedName, ... } from 'dyn'. Each re-exported name becomes
    // a root-level DYNAMIC_RE_EXPORT node recording its original imported name (localName) and the
    // raw dynamic source specifier.
    for (const auto &binding : ext.bindings) {
        auto *node = context_.symbolNodeManager.CreateSymbolNode(binding.exportedName, SymbolKind::DYNAMIC_RE_EXPORT,
                                                                 std::nullopt, binding.importedName, std::nullopt,
                                                                 ext.sourceFile);
        result[binding.exportedName] = node;
    }
}

ExportedSymbols Gluel::ResolveExports(const IntermediateCache *cache)
{
    auto memoIt = resolvedExports_.find(cache->sourceFile);
    if (memoIt != resolvedExports_.end()) {
        return memoIt->second;
    }
    // Cycle guard: this module is already being resolved further up the stack, so only its own
    // directly-declared exports are available here -- returning them (without recursing into its
    // re-exports again) breaks the cycle.
    if (resolving_.count(cache->sourceFile) != 0) {
        return cache->root;
    }
    resolving_.insert(cache->sourceFile);

    // Start from the module's own locally-declared exported symbols.
    ExportedSymbols result = cache->root;

    // Then expand each re-export link.
    for (const auto &ext : cache->externals) {
        if (ext.isDynamic) {
            // Dynamic (interop) re-export: no intermediate cache to expand, so record it verbatim
            // as DYNAMIC_RE_EXPORT node(s) instead of splicing in resolved symbols.
            ApplyDynamicExternal(ext, result);
            continue;
        }
        auto otherIt = cacheIndex_.find(ext.sourceFile);
        if (otherIt == cacheIndex_.end()) {
            // Re-export from a static module we have no intermediate cache for (e.g. a file outside
            // the analyzed set): it can't be expanded statically, so skip it.
            context_.diagnosticEngine.Warning(DiagnosticCode::UNRESOLVED_REEXPORT_SOURCE,
                                              "Re-export source '" + ext.sourceFile + "' has no intermediate cache");
            continue;
        }
        auto otherExports = ResolveExports(otherIt->second);
        ApplyStaticExternal(ext, otherExports, result);
    }

    resolving_.erase(cache->sourceFile);
    resolvedExports_[cache->sourceFile] = result;
    return result;
}

Expected<std::unique_ptr<GlueConfig>, std::string> Gluel::Link(const std::vector<std::string> &targets)
{
    // Index every intermediate cache by its normalized source file path so re-export links can be
    // resolved to their target module's cache.
    for (const auto &cache : intermediateCaches_) {
        cacheIndex_[cache->sourceFile] = cache.get();
    }

    auto config = std::make_unique<GlueConfig>();
    for (const auto &target : targets) {
        // Normalize the target the same way cache source paths are normalized so lookups match
        // regardless of how the caller spelled the path. This stays in the current platform's
        // native form (like every other cache/link path) since it is only used for the
        // cacheIndex_ lookup below, never written out itself.
        const auto normalizedTarget = NormalizePath(target);
        auto it = cacheIndex_.find(normalizedTarget);
        if (it == cacheIndex_.end()) {
            const std::string message = "Link target '" + normalizedTarget + "' has no intermediate cache";
            context_.diagnosticEngine.Error(DiagnosticCode::LINK_TARGET_NOT_FOUND, message);
            return Unexpected(message);
        }
        GlueFileConfig fileConfig;
        fileConfig.root = ResolveExports(it->second);
        fileConfig.globalClassDescriptor = it->second->globalClassDescriptor;
        // Unlike normalizedTarget above, the emitted `files` map key must be portable (forward
        // slashes on every platform) -- see GlueConfig::files' comment in gluel.h.
        config->files[ToForwardSlashPath(normalizedTarget)] = std::move(fileConfig);
    }
    return config;
}
}  // namespace ark::es2panda::gluegen