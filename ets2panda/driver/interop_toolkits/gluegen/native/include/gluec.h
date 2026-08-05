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

#ifndef APPS_GLUEGEN_NATIVE_INCLUDE_GLUEC_H
#define APPS_GLUEGEN_NATIVE_INCLUDE_GLUEC_H

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "libarkbase/utils/expected.h"
#include "./context.h"
#include "./symbol.h"
#include "parser/program/program.h"
#include "checker/ETSchecker.h"

namespace ark::es2panda::gluegen {

/**
 * @brief kind of an external (re-export) link.
 *
 * NAMESPACE covers:
 *   export * as x from 'xx'   (exportedName = "x")
 *   export * from 'xx'        (exportedName = std::nullopt)
 *
 * NAMED covers:
 *   export { default as D } from 'xx'    (bindings = [{"default", "D"}])
 *   export { name1, name2 } from 'xx'    (bindings = [{"name1","name1"}, {"name2","name2"}])
 */
enum class ExternalLinkKind {
    NAMESPACE,
    NAMED,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ExternalLinkKind, {
                                                   {ExternalLinkKind::NAMESPACE, "namespace"},
                                                   {ExternalLinkKind::NAMED, "named"},
                                               })

/**
 * @brief a single binding inside `export { importedName as exportedName } from 'xx'`.
 * For `export { name1 }`, importedName == exportedName == "name1".
 * For `export { default as D }`, importedName == "default", exportedName == "D".
 */
struct NamedBinding {
    std::string importedName;
    std::string exportedName;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NamedBinding, importedName, exportedName)

/**
 * @brief a resolved import binding: what local alias a symbol was imported as, what its
 * original (exported) name in the source module is, and which module it came from.
 */
struct ImportBinding {
    std::string alias;       // local binding name (also used as the containing map's Key)
    std::string original;    // original (imported) name; "default" for default imports, "*" for namespace imports
    std::string source;      // for static modules the resolved on-disk path; for dynamic modules the raw specifier
    bool isDynamic = false;  // whether `source` names a dynamic (interop) module
};

struct ExportBinding {
    std::string alias;     // local binding name (also used as the containing map's Key)
    std::string original;  // original (imported) name; "default" for default imports, "*" for namespace imports
    std::string source;    // module specifier this import came from, e.g. 'xx'
};

/**
 * @brief external links, re-export from other dynamic/static sources.
 */
struct ExternalLinks {
    // for static modules the normalized resolved on-disk path; for dynamic modules the raw import
    // specifier of the original re-export statement (e.g. 'xx').
    std::string sourceFile;
    ExternalLinkKind kind;

    // whether sourceFile names a dynamic (interop) module. Static links are expanded against the
    // target module's intermediate cache; dynamic links are recorded as DYNAMIC_RE_EXPORT nodes.
    bool isDynamic = false;

    // valid when kind == ExternalLinkKind::NAMESPACE
    std::optional<std::string> exportedName;

    // valid when kind == ExternalLinkKind::NAMED
    std::vector<NamedBinding> bindings;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ExternalLinks, sourceFile, kind, isDynamic, exportedName, bindings)

/**
 * @brief intermediates representation for glue informations
 */
struct IntermediateCache {
    std::string sourceFile;   // Source file path
    std::string sourceMTime;  // Source file mtime
    // Bytecode descriptor of the file's synthetic global class (ETSGLOBAL), e.g.
    // "Lentry/some/ETSGLOBAL;" -- see Gluec::GlobalClassDescriptor.
    std::string globalClassDescriptor;
    std::unordered_map<std::string, SymbolNode *> root;
    std::vector<ExternalLinks> externals;

    static std::string serialize(const IntermediateCache &cache);
    static IntermediateCache deserialize(const std::string &jsonStr, SymbolNodeManager &symbolNodeManager);
};

/**
 * @brief a single entry of `<cacheDir>/manifest.json`: maps one source file (the manifest's Key,
 * a normalized absolute path) to the cache file that holds its `IntermediateCache`, plus that
 * source file's mtime at the time the cache was written (duplicated here so a freshness check
 * only needs to read the small manifest, not open every per-file cache to read
 * `IntermediateCache::sourceMTime`).
 */
struct CacheManifestEntry {
    // File name (not a path) of the cache under `<cacheDir>/intermediates/`, e.g.
    // "3f2a9c1d5b6e7f01_a.ets.json" -- see `IntermediateCacheWriter`'s class comment for how this
    // is derived and kept unique.
    std::string cacheFile;
    std::string sourceMTime;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CacheManifestEntry, cacheFile, sourceMTime)

/**
 * @brief serializes a single SymbolNode (and, recursively, its `children`) into a json object.
 */
inline nlohmann::json SymbolNodeToJson(const SymbolNode &node)
{
    nlohmann::json j;
    j["name"] = node.name;
    j["kind"] = node.kind;
    j["runtimeName"] = node.runtimeName;
    j["localName"] = node.localName;
    j["initModuleParam"] = node.initModuleParam;
    j["source"] = node.source;
    auto childrenJson = nlohmann::json::object();
    for (const auto &[childName, childNode] : node.children) {
        childrenJson[childName] = SymbolNodeToJson(*childNode);
    }
    j["children"] = std::move(childrenJson);
    return j;
}

/**
 * @brief deserializes a single SymbolNode (and, recursively, its `children`) from a json object.
 * The returned SymbolNode* (and all of its children, transitively) is owned by
 * `symbolNodeManager`, not by the caller.
 */
SymbolNode *SymbolNodeFromJson(const nlohmann::json &j, SymbolNodeManager &symbolNodeManager);

inline void to_json(nlohmann::json &j, const IntermediateCache &cache)
{
    j["sourceFile"] = cache.sourceFile;
    j["sourceMTime"] = cache.sourceMTime;
    j["globalClassDescriptor"] = cache.globalClassDescriptor;
    auto rootJson = nlohmann::json::object();
    for (const auto &[name, node] : cache.root) {
        rootJson[name] = SymbolNodeToJson(*node);
    }
    j["root"] = std::move(rootJson);
    j["externals"] = cache.externals;
}

/**
 * @brief deserializes an IntermediateCache from `j`, allocating its SymbolNodes via
 * `symbolNodeManager`. Not an ADL from_json (see the comment above) -- called directly by
 * IntermediateCache::deserialize.
 */
void IntermediateCacheFromJson(const nlohmann::json &j, IntermediateCache &cache, SymbolNodeManager &symbolNodeManager);

// Asynchronously persists `IntermediateCache`s (produced by `Gluec::Run()`) to disk on a
// background thread pool (a small std-library-only pool, not a third-party dependency -- see
// gluec.cpp's `ThreadPool`), so the caller doesn't block on serializing/writing each cache. Each
// cache is written to a flat, hash-named file:
//   <cacheDir>/intermediates/<hash-of-sourceFile>_<sourceFile's basename>.json
// rather than mirroring `sourceFile`'s path relative to `workingDirectory`. A relative-path mirror
// breaks in two cases: a dependency resolved from outside `workingDirectory` produces a
// `..`-escaping relative path (which would write outside `cacheDir` entirely), and a deeply
// nested project can exceed Windows' legacy MAX_PATH (260 char) limit even when no single
// component is long. The flat, hash-named scheme sidesteps both, at the cost of needing an
// explicit index -- `<cacheDir>/manifest.json` (one `CacheManifestEntry` per source file) --
// since the cache file name can no longer be recomputed from `sourceFile` alone (see `Enqueue`
// for how collisions on the hash are detected and resolved). The manifest is written once, in
// `Wait()`, after every enqueued write has completed.
// The thread pool is hidden behind a pImpl (defined in gluec.cpp) so including this header does
// not require pulling in <thread>/<condition_variable>.
class IntermediateCacheWriter {
public:
    // `context` is retained by reference and must outlive this writer (and every write scheduled
    // via Enqueue below) -- callers construct it from the same Context whose lifetime already
    // spans the whole Gluegen run. Used to report a Warning/Error diagnostic (via
    // `context.diagnosticEngine`) for every write failure, in addition to the messages returned
    // by Wait().
    IntermediateCacheWriter(std::string cacheDir, Context &context, std::size_t threadCount = 0);

    NO_COPY_SEMANTIC(IntermediateCacheWriter);
    NO_MOVE_SEMANTIC(IntermediateCacheWriter);

    // Waits for all pending writes to finish before destruction.
    ~IntermediateCacheWriter();

    // Schedules `cache` to be serialized and written to disk on the background thread pool.
    // Takes a `shared_ptr` (rather than `unique_ptr`) because the caller (Gluegen::Parse())
    // keeps its own reference to the same cache for the subsequent Link phase, so ownership is
    // shared between "written to disk" and "kept in memory for later use" rather than
    // transferred. Returns immediately without blocking; the write happens asynchronously.
    void Enqueue(std::shared_ptr<IntermediateCache> cache);

    // Blocks until every previously-`Enqueue`d write has finished, then returns the error
    // messages (if any) collected from writes that failed. After this returns, the writer no
    // longer accepts new work.
    std::vector<std::string> Wait();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// Asynchronously reads previously-written `IntermediateCache`s back from disk -- the read
// counterpart to `IntermediateCacheWriter` -- so `Gluegen::Parse()` can reuse a source file's
// cached analysis instead of re-running `Gluec` on it, whenever that file hasn't changed since
// the cache was written. "Hasn't changed" is determined entirely by the caller (compare
// `CachedMTime`'s result against the file's current mtime); this class only knows how to look up
// and load a cache once the caller has decided to.
//
// Reads its index from the same `<cacheDir>/manifest.json` written by `IntermediateCacheWriter`,
// loaded once, at construction time. If that file doesn't exist yet (first run, or a cold cache),
// every `CachedMTime` lookup simply misses, which callers should treat the same as "needs a fresh
// Gluec parse", not as an error.
class IntermediateCacheReader {
public:
    // `context` is retained by reference and must outlive this reader (and every read scheduled
    // via Enqueue below) -- callers construct it from the same Context whose lifetime already
    // spans the whole Gluegen run. `context.symbolNodeManager` is used to allocate every SymbolNode
    // read back from disk; `context.diagnosticEngine` is used to report a Warning for a cache file
    // that could not be read/parsed back despite a matching manifest entry.
    IntermediateCacheReader(std::string cacheDir, Context &context, std::size_t threadCount = 0);

    NO_COPY_SEMANTIC(IntermediateCacheReader);
    NO_MOVE_SEMANTIC(IntermediateCacheReader);

    // Waits for all pending reads to finish before destruction.
    ~IntermediateCacheReader();

    // Returns the `sourceMTime` recorded in the manifest for `sourceFile` (a normalized absolute
    // path -- see `NormalizePath`/`Gluec::Run`, which is how `sourceFile` keys are produced in the
    // first place), or `std::nullopt` if there is no manifest entry for it. Safe to call from any
    // thread and at any time (the manifest is loaded once, at construction, and never mutated
    // afterward).
    std::optional<std::string> CachedMTime(const std::string &sourceFile) const;

    // Schedules an asynchronous read+deserialize of `sourceFile`'s on-disk `IntermediateCache` on
    // the background thread pool. `onLoaded` runs on a pool thread once the read completes,
    // receiving the loaded cache on success or `nullptr` if it failed for any reason (no manifest
    // entry, missing/corrupt cache file, ...) -- callers should treat a `nullptr` result as a
    // cache miss (fall back to a fresh Gluec parse), not as a fatal error. Returns immediately
    // without blocking.
    // Safe to run concurrently across pool threads: `IntermediateCache::deserialize` allocates
    // every `SymbolNode` via the `SymbolNodeManager` this reader was constructed with, whose
    // mutations are synchronized internally by that class's own mutex.
    void Enqueue(const std::string &sourceFile, std::function<void(std::shared_ptr<IntermediateCache>)> onLoaded);

    // Blocks until every previously-`Enqueue`d read has finished. After this returns, the reader
    // no longer accepts new work.
    void Wait();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

class Gluec {
public:
    Gluec(parser::Program *program, Context &context);
    // Returns the built cache via `unique_ptr` (rather than by value) so callers that queue it
    // up for asynchronous disk serialization can move/take ownership of it without copying.
    Expected<std::unique_ptr<IntermediateCache>, std::string> Run();

private:
    parser::Program *program_;
    // Owns the SymbolNodeManager (and diagnostics) every SymbolNode created while analyzing
    // `program_` is allocated through -- see CollectNamespaceChildren/BuildRootSymbols.
    Context &context_;
    // alias (local binding name) to tuple `{alias, original name, source file path}`, for
    // statically-resolved imports. For default imports the original name is recorded as
    // "default"; for namespace imports (`import * as ns from 'xx'`) it is recorded as "*".
    std::unordered_map<std::string, ImportBinding> staticImportSpecifiers_;
    // Same alias->binding mapping, but for dynamic (interop) imports.
    std::unordered_map<std::string, ImportBinding> dynamicImportSpecifiers_;

    // export statements that are not re-export stmts, e.g export { A, B }
    // alias -> original name, for statically-resolved exports.
    std::unordered_map<std::string, std::string> exportSpecifiers_;

    // export statements that are re-export stmts, e.g. export { A } from 'xx'
    std::unordered_map<std::string, ImportBinding> staticReExportSpecifiers_;
    // Same alias->binding mapping, but for dynamic (interop) imports.
    std::unordered_map<std::string, ImportBinding> dynamicReExportSpecifiers_;

    // top level symbols, now we do not consider namespaces.
    // name -> declaration node (ClassDefinition* / MethodDefinition* / ClassProperty* / ClassDefinition* for
    // namespaces), kept so exported-ness and runtime descriptors can be resolved later in Run().
    std::unordered_map<std::string, const ir::AstNode *> topLevelClasses_;
    std::unordered_map<std::string, const ir::AstNode *> topLevelFunctions_;
    std::unordered_map<std::string, const ir::AstNode *> topLevelProperties_;
    std::unordered_map<std::string, const ir::AstNode *> topLevelNamespaces_;

    // Collect top level symbols
    void CollectTopLevelSymbols();

    // Collect imports
    void CollectSpecifiersFromImport(const ir::AstNode *importStatement,
                                     std::unordered_map<std::string, ImportBinding> &target);
    void CollectSpecifiersAfterDefault(const ArenaVector<ir::AstNode *> &specifiers, const std::string &source,
                                       bool isDynamic, std::unordered_map<std::string, ImportBinding> &target);
    void CollectDefaultImport(const ir::AstNode *specifier, const std::string &source, bool isDynamic,
                              std::unordered_map<std::string, ImportBinding> &target);
    void CollectNamedImports(const ArenaVector<ir::AstNode *> &specifiers, const std::string &source, bool isDynamic,
                             std::unordered_map<std::string, ImportBinding> &target);
    void CollectNamespaceImport(const ir::AstNode *specifier, const std::string &source, bool isDynamic,
                                std::unordered_map<std::string, ImportBinding> &target);

    // Collect exports: `export { A, B as C };` and `export default A;`, where A/B are not
    // themselves directly declared with an export modifier in this file (see gluec.cpp's
    // CollectExportSpecifiers doc comment for why this must read the ETSBinder's ExportFactStore
    // rather than scan `ExportNamedDeclaration` AST nodes).
    void CollectExportSpecifiers();

    // Collect re-exports, e.g. `export { A } from 'xx'`, `export * from 'xx'`,
    // `export * as ns from 'xx'`
    void CollectSpecifiersFromReExport(const ir::AstNode *reExportStatement);

    // Collect top level classes/functions/properties/namespaces
    void CollectClassLikeSymbol(const ir::AstNode *stmt);
    void CollectGlobalClassMembers(const ir::ClassDefinition *globalClassDef);

    // Build IntermediateCache::externals: re-export links, both direct (`export ... from 'xx'`)
    // and "import-then-export" (`import { A } from 'xx'; export { A };`) style.
    void BuildExternals(IntermediateCache &cache);
    const ImportBinding *FindImportBinding(const std::string &alias) const;

    // Build IntermediateCache::root: SymbolNode for every exported top-level class/function/
    // property/namespace/initModule() call.
    void BuildRootSymbols(IntermediateCache &cache);
    void CollectNamespaceChildren(const ir::ClassDefinition *namespaceDef, SymbolNode *parentNode);
    // Helpers extracted from CollectNamespaceChildren: one per member kind.
    void CollectNamespaceMethod(const ir::AstNode *member, const std::string &containerDescriptor,
                                std::unordered_map<std::string, SymbolNode *> &children);
    void CollectNamespaceProperty(const ir::AstNode *member, const std::string &containerDescriptor,
                                  std::unordered_map<std::string, SymbolNode *> &children);
    void CollectNamespaceClassLike(const ir::AstNode *member, std::unordered_map<std::string, SymbolNode *> &children);
    std::optional<std::string> DirectlyExportedName(const ir::AstNode *node, const std::string &declaredName) const;
    std::optional<std::string> TopLevelExportedName(const ir::AstNode *node, const std::string &declaredName) const;
    std::string GlobalClassDescriptor() const;
    static std::string ClassDescriptor(const ir::ClassDefinition *classDef);
};

}  // namespace ark::es2panda::gluegen
#endif  // APPS_GLUEGEN_NATIVE_INCLUDE_GLUEC_H
