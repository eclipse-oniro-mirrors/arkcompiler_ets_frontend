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
#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <system_error>
#include <unordered_set>
#include <utility>
#include "gluec.h"
#include "utils.h"
#include "checker/ETSchecker.h"
#include "ir/ets/etsReExportDeclaration.h"
#include "ir/ets/etsStructDeclaration.h"
#include "varbinder/ETSBinder.h"
#include "varbinder/exportFacts.h"

namespace ark::es2panda::gluegen {
namespace {
// Filters out synthetic/compiler-generated method names that should not be treated as
// user-visible top-level functions (mirrors TSDeclGen::ShouldSkipMethodDeclaration's filters).
bool IsSkippableTopLevelFunctionName(const std::string &name)
{
    return name.find('#') != std::string::npos || name.find("%%async-") != std::string::npos ||
           name == compiler::Signatures::INIT_METHOD || name == compiler::Signatures::INITIALIZER_BLOCK_INIT;
}

// `struct Foo {}` (used for ArkUI @Component structs) parses as an `ETSStructDeclaration`, a
// subclass of `ClassDeclaration` with its own `AstNodeType::STRUCT_DECLARATION` (rather than
// `AstNodeType::CLASS_DECLARATION`) -- so `AstNode::IsClassDeclaration()`, which only compares
// `Type()` against `CLASS_DECLARATION`, returns false for it, even though both node kinds share
// the exact same `ClassDefinition` payload (accessible via the `Definition()` they both inherit
// from `ClassDeclaration`). Symbol collection intentionally treats a struct exactly like a class
// (per gluegen's requirements), so this is the single place that recognizes either spelling.
bool IsClassLikeDeclaration(const ir::AstNode *node)
{
    return node->IsClassDeclaration() || node->IsETSStructDeclaration();
}

// Returns the `ClassDefinition` shared by a `ClassDeclaration`/`ETSStructDeclaration` statement --
// see `IsClassLikeDeclaration` above for why `AsClassDeclaration()` alone cannot be used for both.
const ir::ClassDefinition *ClassLikeDefinition(const ir::AstNode *node)
{
    if (node->IsETSStructDeclaration()) {
        return node->AsETSStructDeclaration()->Definition();
    }
    return node->AsClassDeclaration()->Definition();
}

// Accumulates, per source module, everything re-exported from it (either directly via
// `export ... from 'xx'`, or via the "import-then-export" passthrough pattern). A single source
// can be re-exported through several forms at once (e.g. both `export * from 'xx'` and
// `export { A } from 'xx'`), so namespace and named re-exports are kept side by side instead of
// collapsing into one.
struct ExternalAccumulator {
    // One entry per `export *` / `export * as ns` re-export of this source: the namespace alias, or
    // std::nullopt for a bare `export * from 'xx'`.
    std::vector<std::optional<std::string>> namespaceExports;
    std::vector<NamedBinding> namedBindings;
    bool isDynamic = false;  // whether the source module is a dynamic (interop) module
};

void AddExternalBinding(std::unordered_map<std::string, ExternalAccumulator> &bySource, const std::string &source,
                        const std::string &original, const std::string &alias, bool isDynamic)
{
    auto &accum = bySource[source];
    accum.isDynamic = isDynamic;
    if (original == "*") {
        accum.namespaceExports.push_back(alias.empty() ? std::nullopt : std::optional<std::string>(alias));
        return;
    }
    accum.namedBindings.push_back(NamedBinding {original, alias});
}

// Reads `<cacheDir>/manifest.json` (written by `IntermediateCacheWriter::Wait`) into a
// sourceFile -> CacheManifestEntry map. Tolerates a missing, empty, or corrupt/partial file (e.g.
// left behind by a run that crashed before the writer's rename completed) by simply returning an
// empty map -- i.e. treating it the same as a cold cache -- rather than throwing, but reports a
// Warning diagnostic (via `diagnosticEngine`) whenever the file exists yet is unparsable, since
// that specific case is a genuine anomaly rather than the ordinary "no cache yet" case. Shared by
// `IntermediateCacheWriter` (which additionally seeds `usedCacheFiles_` from the result -- see
// `LoadExistingManifest`) and `IntermediateCacheReader` (which only needs mtime/cache-file
// lookups).
std::unordered_map<std::string, CacheManifestEntry> LoadManifestFile(const std::string &cacheDir,
                                                                     DiagnosticEngine &diagnosticEngine)
{
    std::unordered_map<std::string, CacheManifestEntry> manifest;
    const auto manifestPath = fs::path(cacheDir) / "manifest.json";
    std::ifstream in(ToLongPathIfNeeded(manifestPath), std::ios::binary);
    if (!in.is_open()) {
        return manifest;
    }
    std::ostringstream contentStream;
    contentStream << in.rdbuf();

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(contentStream.str());
    } catch (const nlohmann::json::exception &e) {
        diagnosticEngine.Warning(DiagnosticCode::CACHE_MANIFEST_CORRUPT,
                                 "Failed to parse cache manifest file: " + manifestPath.string(), e.what());
        return manifest;
    }
    if (!j.is_object()) {
        diagnosticEngine.Warning(DiagnosticCode::CACHE_MANIFEST_CORRUPT,
                                 "Cache manifest file is not a valid JSON object: " + manifestPath.string());
        return manifest;
    }

    for (const auto &[sourceFile, entryJson] : j.items()) {
        CacheManifestEntry entry;
        try {
            entryJson.get_to(entry);
        } catch (const nlohmann::json::exception &e) {
            diagnosticEngine.Warning(
                DiagnosticCode::CACHE_MANIFEST_CORRUPT,
                "Failed to parse cache manifest entry '" + sourceFile + "' in " + manifestPath.string(), e.what());
            continue;
        }
        manifest[sourceFile] = entry;
    }
    return manifest;
}
}  // namespace

std::string IntermediateCache::serialize(const IntermediateCache &cache)
{
    constexpr size_t jsonIndent = 2;
    nlohmann::json j = cache;
    return j.dump(jsonIndent);
}

IntermediateCache IntermediateCache::deserialize(const std::string &jsonStr, SymbolNodeManager &symbolNodeManager)
{
    nlohmann::json j = nlohmann::json::parse(jsonStr);
    IntermediateCache cache;
    IntermediateCacheFromJson(j, cache, symbolNodeManager);
    return cache;
}

SymbolNode *SymbolNodeFromJson(const nlohmann::json &j, SymbolNodeManager &symbolNodeManager)
{
    auto *node = symbolNodeManager.CreateSymbolNode(
        j.at("name").get<std::string>(), j.at("kind").get<SymbolKind>(),
        j.at("runtimeName").get<std::optional<std::string>>(), j.at("localName").get<std::optional<std::string>>(),
        j.at("initModuleParam").get<std::optional<std::string>>(),
        j.contains("source") ? j.at("source").get<std::optional<std::string>>() : std::nullopt);
    const auto &childrenJson = j.at("children");
    // Tolerate a `null` "children" (as written by an older gluegen build, back when the field
    // was optional) by treating it the same as an empty object -- only non-null values are
    // walked into actual children.
    if (childrenJson.is_object()) {
        for (const auto &[childName, childJson] : childrenJson.items()) {
            node->children[childName] = SymbolNodeFromJson(childJson, symbolNodeManager);
        }
    }
    return node;
}

void IntermediateCacheFromJson(const nlohmann::json &j, IntermediateCache &cache, SymbolNodeManager &symbolNodeManager)
{
    j.at("sourceFile").get_to(cache.sourceFile);
    j.at("sourceMTime").get_to(cache.sourceMTime);
    // Tolerate a cache written by an older gluegen build, before this field existed.
    cache.globalClassDescriptor =
        j.contains("globalClassDescriptor") ? j.at("globalClassDescriptor").get<std::string>() : std::string();
    cache.root.clear();
    for (const auto &[name, nodeJson] : j.at("root").items()) {
        cache.root[name] = SymbolNodeFromJson(nodeJson, symbolNodeManager);
    }
    j.at("externals").get_to(cache.externals);
}

// Real implementation of IntermediateCacheWriter; kept out of gluec.h so consumers of that
// header don't need to pull in <thread>/<condition_variable> (and every include of them
// transitively) just to declare an IntermediateCacheWriter.
class IntermediateCacheWriter::Impl {
public:
    Impl(std::string cacheDir, Context &context, std::size_t threadCount)
        : cacheDir_(std::move(cacheDir)), context_(context), pool_(threadCount)
    {
        // Seed `manifest_`/`usedCacheFiles_` from any manifest.json left by a previous run,
        // *before* any `Enqueue` call can happen: collision-avoidance in `AssignCacheFileName`
        // only works if it can see every name already claimed, including ones from files this
        // run never touches (see that function's comment, and `LoadExistingManifest` below).
        LoadExistingManifest();
    }

    ~Impl()
    {
        Join();
    }

    void Enqueue(std::shared_ptr<IntermediateCache> cache)
    {
        // The cache file name (and, transitively, the manifest entry for `sourceFile`) is
        // assigned eagerly here, on the calling thread, rather than inside `WriteToDisk` on the
        // pool thread: `AssignCacheFileName` mutates `manifest_`, which every `Enqueue` call
        // shares, so doing it up front keeps that mutation off the background threads (only the
        // actual disk I/O -- and `RecordError` -- happens there).
        auto cacheFileName = AssignCacheFileName(cache->sourceFile, cache->sourceMTime);
        auto outputPath = fs::path(cacheDir_).append("intermediates").append(cacheFileName);
        WritingTask task {std::move(cache), std::move(outputPath)};
        pool_.Post([this, task = std::move(task)]() { WriteToDisk(task); });
    }

    std::vector<std::string> Wait()
    {
        Join();
        WriteManifest();
        std::lock_guard<std::mutex> lock(errorsMutex_);
        return errors_;
    }

private:
    // A single scheduled write: the cache to serialize and the on-disk path already resolved for
    // it (see `Enqueue`/`AssignCacheFileName`), bundled together so `WriteToDisk` needs nothing
    // else.
    struct WritingTask {
        std::shared_ptr<IntermediateCache> cache;
        fs::path outputPath;
    };

    void Join()
    {
        if (!joined_) {
            pool_.Join();
            joined_ = true;
        }
    }

    // `description` is the human-readable summary (e.g. "Failed to write cache manifest file:
    // <path>"); `cause` is an optional lower-level detail (an OS `std::error_code::message()`, ...)
    // kept as its own DiagnosticEngine field rather than concatenated into `description`, so a
    // consumer/report can tell "what failed" apart from "why" precisely.
    void RecordError(std::string description, std::string cause = "")
    {
        // Every write failure recorded here also fails the overall Gluegen run (Gluegen::Parse()
        // folds Wait()'s returned messages into its own error count), so it is reported at ERROR
        // severity, not WARNING.
        context_.diagnosticEngine.Error(DiagnosticCode::CACHE_IO_ERROR, description, cause);
        std::string combined = cause.empty() ? description : description + ": " + cause;
        std::lock_guard<std::mutex> lock(errorsMutex_);
        errors_.push_back(std::move(combined));
    }

    // Seeds `manifest_`/`usedCacheFiles_` from `<cacheDir_>/manifest.json`, if one exists (from a
    // previous run). Tolerates a missing, empty, or corrupt/partial file (e.g. left behind by a
    // run that crashed before `WriteManifest`'s rename completed) by treating it the same as a
    // cold cache -- i.e. simply not seeding anything -- rather than failing construction.
    void LoadExistingManifest()
    {
        manifest_ = LoadManifestFile(cacheDir_, context_.diagnosticEngine);
        for (const auto &[sourceFile, entry] : manifest_) {
            usedCacheFiles_.insert(entry.cacheFile);
        }
    }

    // Derives a cache file name for `sourceFile` (a normalized absolute path) of the form
    // "<16-hex-digit hash>_<basename>.json", records it in `manifest_`, and returns it.
    // The hash is `std::hash<std::string>` (64 bits on every target platform), not a
    // cryptographic hash — `manifest.json` is the actual index, keyed by `sourceFile` directly.
    // Uniqueness is guaranteed by the `usedCacheFiles_` check below: a source file that is
    // unchanged across runs is never passed to `Enqueue`/`AssignCacheFileName` at all, so a
    // name assigned here must not collide with that untouched file's name. That's why
    // `usedCacheFiles_`/`manifest_` are seeded from a pre-existing `manifest.json` by
    // `LoadExistingManifest` rather than starting empty each run.
    // Storing every cache file under a flat `intermediates/` directory (derived from a hash of
    // `sourceFile` rather than mirroring its on-disk path) sidesteps both the
    // `..`-escaping-`cacheDir_` problem and Windows' MAX_PATH limit.
    std::string AssignCacheFileName(const std::string &sourceFile, const std::string &sourceMTime)
    {
        std::ostringstream hashHex;
        constexpr std::size_t kHexDigitsPerByte = 2;
        hashHex << std::hex << std::setfill('0') << std::setw(sizeof(std::size_t) * kHexDigitsPerByte)
                << std::hash<std::string> {}(sourceFile);
        const auto baseName = fs::path(sourceFile).filename().string();

        std::lock_guard<std::mutex> lock(manifestMutex_);
        // A previous Enqueue() for the same sourceFile (should not normally happen -- each
        // sourceFile is only ever passed to Gluec once -- but is harmless/idempotent to handle)
        // simply reuses/refreshes its existing entry instead of assigning a new name.
        if (auto it = manifest_.find(sourceFile); it != manifest_.end()) {
            it->second.sourceMTime = sourceMTime;
            return it->second.cacheFile;
        }

        // Resolve a hash collision against a *different* sourceFile deterministically, by trying
        // successive numeric suffixes -- this is what actually guarantees every cache file gets a
        // unique name, regardless of how wide/strong the hash itself is.
        std::string candidate = hashHex.str() + "_" + baseName + ".json";
        for (std::size_t suffix = 1; usedCacheFiles_.count(candidate) != 0; ++suffix) {
            candidate = hashHex.str() + "_" + std::to_string(suffix) + "_" + baseName + ".json";
        }

        usedCacheFiles_.insert(candidate);
        manifest_[sourceFile] = CacheManifestEntry {candidate, sourceMTime};
        return candidate;
    }

    // Writes `manifest_` to `<cacheDir_>/manifest.json`, via a temp-file-then-rename so a reader
    // never observes a partially-written manifest (`fs::rename` replaces the
    // destination atomically on POSIX, and via `MoveFileExW`/`MOVEFILE_REPLACE_EXISTING` on
    // Windows). Called once, from `Wait()`, after every enqueued write has finished -- not
    // incrementally per-file -- so it never needs its own background scheduling.
    void WriteManifest()
    {
        std::lock_guard<std::mutex> lock(manifestMutex_);
        if (manifest_.empty()) {
            return;
        }

        const auto manifestPath = fs::path(cacheDir_) / "manifest.json";
        std::error_code ec;
        fs::create_directories(ToLongPathIfNeeded(manifestPath.parent_path()), ec);
        if (ec) {
            RecordError("Failed to create cache manifest directory: " + manifestPath.parent_path().string(),
                        ec.message());
            return;
        }

        auto tmpPath = manifestPath;
        tmpPath += ".tmp";
        {
            std::ofstream out(ToLongPathIfNeeded(tmpPath), std::ios::binary | std::ios::trunc);
            if (!out.is_open()) {
                RecordError("Failed to open cache manifest file for writing: " + tmpPath.string());
                return;
            }
            nlohmann::json j = manifest_;
            constexpr size_t jsonIndent = 2;
            out << j.dump(jsonIndent);
            if (!out) {
                RecordError("Failed to write cache manifest file: " + tmpPath.string());
                return;
            }
        }

        fs::rename(ToLongPathIfNeeded(tmpPath), ToLongPathIfNeeded(manifestPath), ec);
        if (ec) {
            RecordError("Failed to finalize cache manifest file: " + manifestPath.string(), ec.message());
        }
    }

    void WriteToDisk(const WritingTask &task)
    {
        std::error_code ec;
        fs::create_directories(ToLongPathIfNeeded(task.outputPath.parent_path()), ec);
        if (ec) {
            RecordError("Failed to create intermediate cache directory: " + task.outputPath.parent_path().string(),
                        ec.message());
            return;
        }

        std::ofstream out(ToLongPathIfNeeded(task.outputPath), std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            RecordError("Failed to open intermediate cache file for writing: " + task.outputPath.string());
            return;
        }

        out << IntermediateCache::serialize(*task.cache);
        if (!out) {
            RecordError("Failed to write intermediate cache file: " + task.outputPath.string());
        }
    }

    std::string cacheDir_;
    Context &context_;
    ThreadPool pool_;
    bool joined_ = false;

    std::mutex manifestMutex_;
    std::unordered_map<std::string, CacheManifestEntry> manifest_;  // sourceFile -> cache file entry
    std::unordered_set<std::string> usedCacheFiles_;                // cache file names already assigned

    std::mutex errorsMutex_;
    std::vector<std::string> errors_;
};

IntermediateCacheWriter::IntermediateCacheWriter(std::string cacheDir, Context &context, std::size_t threadCount)
    : impl_(std::make_unique<Impl>(std::move(cacheDir), context, threadCount))
{
}

IntermediateCacheWriter::~IntermediateCacheWriter() = default;

void IntermediateCacheWriter::Enqueue(std::shared_ptr<IntermediateCache> cache)
{
    impl_->Enqueue(std::move(cache));
}

std::vector<std::string> IntermediateCacheWriter::Wait()
{
    return impl_->Wait();
}

// Real implementation of IntermediateCacheReader; kept out of gluec.h for the same reason as
// IntermediateCacheWriter::Impl (avoids pulling <thread>/<condition_variable> into every includer
// of gluec.h).
class IntermediateCacheReader::Impl {
public:
    Impl(std::string cacheDir, Context &context, std::size_t threadCount)
        : cacheDir_(std::move(cacheDir)),
          context_(context),
          pool_(threadCount),
          manifest_(LoadManifestFile(cacheDir_, context.diagnosticEngine))
    {
    }

    ~Impl()
    {
        Join();
    }

    // `manifest_` is populated once, here in the constructor, and never mutated afterward, so
    // reading it needs no synchronization even though `CachedMTime` may be called concurrently
    // with reads that are in flight on the thread pool.
    std::optional<std::string> CachedMTime(const std::string &sourceFile) const
    {
        auto it = manifest_.find(sourceFile);
        if (it == manifest_.end()) {
            return std::nullopt;
        }
        return it->second.sourceMTime;
    }

    void Enqueue(const std::string &sourceFile, std::function<void(std::shared_ptr<IntermediateCache>)> onLoaded)
    {
        auto it = manifest_.find(sourceFile);
        if (it == manifest_.end()) {
            pool_.Post([onLoaded = std::move(onLoaded)]() { onLoaded(nullptr); });
            return;
        }
        auto cachePath = fs::path(cacheDir_) / "intermediates" / it->second.cacheFile;
        pool_.Post([this, cachePath = std::move(cachePath), onLoaded = std::move(onLoaded)]() {
            onLoaded(ReadFromDisk(cachePath, context_));
        });
    }

    void Wait()
    {
        Join();
    }

private:
    void Join()
    {
        if (!joined_) {
            pool_.Join();
            joined_ = true;
        }
    }

    // Reads and deserializes the cache file. Safe to call concurrently from multiple pool threads:
    // IntermediateCache::deserialize's SymbolNode allocations go through
    // `context.symbolNodeManager`, which guards its internal state with its own mutex (see
    // symbol.h). A read/parse failure here means the manifest had a matching entry but the cache
    // file itself is missing/corrupt -- a genuine (if non-fatal) anomaly, so it's reported as a
    // Warning via `context.diagnosticEngine`; Gluegen::Parse() falls back to a fresh Gluec parse.
    static std::shared_ptr<IntermediateCache> ReadFromDisk(const fs::path &cachePath, Context &context)
    {
        std::ifstream in(ToLongPathIfNeeded(cachePath), std::ios::binary);
        if (!in.is_open()) {
            context.diagnosticEngine.Warning(DiagnosticCode::CACHE_IO_ERROR,
                                             "Failed to open intermediate cache file for reading: " +
                                                 cachePath.string());
            return nullptr;
        }
        std::ostringstream contentStream;
        contentStream << in.rdbuf();
        try {
            return std::make_shared<IntermediateCache>(
                IntermediateCache::deserialize(contentStream.str(), context.symbolNodeManager));
        } catch (const nlohmann::json::exception &e) {
            context.diagnosticEngine.Warning(DiagnosticCode::CACHE_IO_ERROR,
                                             "Failed to parse intermediate cache file: " + cachePath.string(),
                                             e.what());
            return nullptr;
        }
    }

    std::string cacheDir_;
    Context &context_;
    ThreadPool pool_;
    bool joined_ = false;
    const std::unordered_map<std::string, CacheManifestEntry> manifest_;  // sourceFile -> cache file entry
};

IntermediateCacheReader::IntermediateCacheReader(std::string cacheDir, Context &context, std::size_t threadCount)
    : impl_(std::make_unique<Impl>(std::move(cacheDir), context, threadCount))
{
}

IntermediateCacheReader::~IntermediateCacheReader() = default;

std::optional<std::string> IntermediateCacheReader::CachedMTime(const std::string &sourceFile) const
{
    return impl_->CachedMTime(sourceFile);
}

void IntermediateCacheReader::Enqueue(const std::string &sourceFile,
                                      std::function<void(std::shared_ptr<IntermediateCache>)> onLoaded)
{
    impl_->Enqueue(sourceFile, std::move(onLoaded));
}

void IntermediateCacheReader::Wait()
{
    impl_->Wait();
}

Gluec::Gluec(parser::Program *program, Context &context) : program_(program), context_(context) {}

void Gluec::CollectDefaultImport(const ir::AstNode *specifier, const std::string &source, bool isDynamic,
                                 std::unordered_map<std::string, ImportBinding> &target)
{
    auto importDefaultSpecifier = specifier->AsImportDefaultSpecifier();
    auto variable = importDefaultSpecifier->Local()->Variable();
    const auto local = importDefaultSpecifier->Local()->Name().Mutf8();
    bool isTypeDeclaration = false;
    if (variable != nullptr && variable->Declaration() != nullptr && variable->Declaration()->Node() != nullptr) {
        auto *node = variable->Declaration()->Node();
        isTypeDeclaration = node->IsTSTypeAliasDeclaration() || node->IsTSInterfaceDeclaration();
    }
    if (!isTypeDeclaration) {
        // Default imports have no explicit "as" alias: the local name is the alias (Key),
        // and the well-known original name is "default" (Value).
        target[local] = ImportBinding {local, "default", source, isDynamic};
    }
}

void Gluec::CollectNamespaceImport(const ir::AstNode *specifier, const std::string &source, bool isDynamic,
                                   std::unordered_map<std::string, ImportBinding> &target)
{
    auto importNamespaceSpecifier = specifier->AsImportNamespaceSpecifier();
    auto variable = importNamespaceSpecifier->Local()->Variable();
    const auto local = importNamespaceSpecifier->Local()->Name().Mutf8();
    bool isTypeDeclaration = false;
    if (variable != nullptr && variable->Declaration() != nullptr && variable->Declaration()->Node() != nullptr) {
        auto *node = variable->Declaration()->Node();
        isTypeDeclaration = node->IsTSTypeAliasDeclaration() || node->IsTSInterfaceDeclaration();
    }
    if (isTypeDeclaration) {
        return;
    }
    // Namespace imports (`import * as ns from 'xx'`) bind the whole module, there is no
    // single "original" symbol name, so we record it as "*".
    if (!local.empty()) {
        target[local] = ImportBinding {local, "*", source, isDynamic};
        return;
    }
    // A bare `export * from 'xx'` re-export has no alias at all (regular `import * as ns from`
    // always has a non-empty local name). Key by source instead, so re-exports of distinct
    // modules do not collide with each other under the same empty-string key.
    target[source] = ImportBinding {"", "*", source, isDynamic};
}

void Gluec::CollectNamedImports(const ArenaVector<ir::AstNode *> &specifiers, const std::string &source, bool isDynamic,
                                std::unordered_map<std::string, ImportBinding> &target)
{
    for (auto *specifier : specifiers) {
        if (!specifier->IsImportSpecifier()) {
            continue;
        }
        auto importSpecifier = specifier->AsImportSpecifier();
        auto variable = importSpecifier->Imported()->Variable();
        bool isTypeDeclaration = false;
        if (variable != nullptr && variable->Declaration() != nullptr && variable->Declaration()->Node() != nullptr) {
            auto *node = variable->Declaration()->Node();
            isTypeDeclaration = node->IsTSTypeAliasDeclaration() || node->IsTSInterfaceDeclaration();
        }
        if (!isTypeDeclaration) {
            const auto alias = importSpecifier->Local()->Name().Mutf8();
            const auto original = importSpecifier->Imported()->Name().Mutf8();
            target[alias] = ImportBinding {alias, original, source, isDynamic};
        }
    }
}

// Handles the second specifier of `import Def, ... from 'xx'`: it is either a namespace
// specifier (`import Def, * as ns from 'xx'`) or a list of named specifiers
// (`import Def, { a, b } from 'xx'`).
void Gluec::CollectSpecifiersAfterDefault(const ArenaVector<ir::AstNode *> &specifiers, const std::string &source,
                                          bool isDynamic, std::unordered_map<std::string, ImportBinding> &target)
{
    if (specifiers.size() <= 1) {
        return;
    }
    if (specifiers[1]->IsImportNamespaceSpecifier()) {
        CollectNamespaceImport(specifiers[1], source, isDynamic, target);
        return;
    }
    CollectNamedImports(specifiers, source, isDynamic, target);
}

void Gluec::CollectSpecifiersFromImport(const ir::AstNode *importStatement,
                                        std::unordered_map<std::string, ImportBinding> &target)
{
    auto importDeclaration = importStatement->AsETSImportDeclaration();
    if (importDeclaration->IsTypeKind()) {
        return;
    }
    const auto &specifiers = importDeclaration->Specifiers();
    if (specifiers.empty()) {
        return;
    }
    // For static modules use the resolved (real, on-disk) file path rather than the raw specifier
    // text (e.g. './foo' or 'std/core') so that ImportBinding::source -- and, transitively,
    // ExternalLinks::sourceFile built from it in BuildExternals -- names an actual file that can be
    // matched against an intermediate cache. Dynamic (interop) modules have no static file on disk
    // to point at, so keep the original raw import specifier instead: that is what the downstream
    // consumer needs to regenerate the dynamic re-export.
    const bool isDynamic = importDeclaration->IsPureDynamic();
    const auto source =
        isDynamic ? importDeclaration->Source()->Str().Mutf8() : std::string(importDeclaration->ResolvedSource());
    const auto specifierFirst = specifiers[0];
    if (specifierFirst->IsImportDefaultSpecifier()) {
        CollectDefaultImport(specifierFirst, source, isDynamic, target);
        CollectSpecifiersAfterDefault(specifiers, source, isDynamic, target);
        return;
    }
    if (specifierFirst->IsImportSpecifier()) {
        CollectNamedImports(specifiers, source, isDynamic, target);
        return;
    }
    if (specifierFirst->IsImportNamespaceSpecifier()) {
        CollectNamespaceImport(specifierFirst, source, isDynamic, target);
    }
}

// Collects `export { A, B as C }` / `export default A;` style local named exports: an alias
// (possibly identical to the original name) for something that is NOT itself directly declared
// with an `export`/`export default` modifier in this file -- either a plain re-aliasing of a
// local declaration (`class A {}; export { A as B };`) or the "import-then-export" passthrough
// pattern (`import { A } from 'xx'; export { A };`).
// This can NOT be collected by scanning `program_->Ast()->Statements()` for a raw
// `ExportNamedDeclaration` node the way re-exports/imports are: by the time `Gluec` runs (after
// the program has reached `ES2PANDA_STATE_BOUND`), `GlobalDeclTransformer::FilterDeclarations`
// has already erased every top-level `ExportNamedDeclaration` from `Statements()` (it only ever
// leaves an `EmptyStatement` placeholder behind when the program has parse errors) -- the
// alias/original name pairs it carries survive only inside the `ETSBinder`'s `ExportFactStore`,
// populated by `ImportExportDecls`/`ETSBinder::AddSelectiveExportAlias` during Bind(). So this
// queries that store instead, via `ExportFact::isLocalAlias` (true for exactly this alias case,
// false for a declaration's own direct export, which `BuildRootSymbols`'s
// `DirectlyExportedName`/`TopLevelExportedName` already picks up straight off the declaration's
// own modifiers).
// The exported (alias) name is used as the Key, and the local (original) name as the Value,
// mirroring the alias->original convention used for import specifiers.
void Gluec::CollectExportSpecifiers()
{
    const auto &facts = program_->VarBinder()->AsETSBinder()->GetExportFacts(program_);
    for (const auto &fact : facts.locals) {
        if (!fact.isLocalAlias) {
            continue;
        }
        // Mirrors the old TSTypeAliasDeclaration/TSInterfaceDeclaration skip: a name that only
        // exists on the type surface is not a runtime symbol gluegen needs to link.
        if (fact.isTypeOnlySurface) {
            continue;
        }
        exportSpecifiers_[fact.exportedName.Mutf8()] = fact.localName.Mutf8();
    }
}

// Collects re-exports (`export { A, B as C } from 'xx'`, `export * as ns from 'xx'`,
// `export * from 'xx'`). `ETSReExportDeclaration` wraps an `ETSImportDeclaration` whose
// specifiers/source describe exactly what is being re-exported, so the existing import
// specifier collection logic is reused as-is (the bare `export * from 'xx'` case, which has no
// alias, is keyed by `source` instead -- see `CollectNamespaceImport`).
void Gluec::CollectSpecifiersFromReExport(const ir::AstNode *reExportStatement)
{
    auto *importDeclaration = reExportStatement->AsETSReExportDeclaration()->GetETSImportDeclarations();
    auto &target = importDeclaration->IsPureDynamic() ? dynamicReExportSpecifiers_ : staticReExportSpecifiers_;
    CollectSpecifiersFromImport(importDeclaration, target);
}

// Collects top-level function/property/namespace symbols from the synthetic global class body
// (`Definition()->IsGlobal()`). Namespaces are only recorded by name at this point; their
// members are recursively collected later, in `CollectNamespaceChildren`, only if the namespace
// itself turns out to be exported.
void Gluec::CollectGlobalClassMembers(const ir::ClassDefinition *globalClassDef)
{
    for (auto *member : globalClassDef->Body()) {
        if (member->IsMethodDefinition()) {
            const auto *key = member->AsMethodDefinition()->Key();
            if (key->IsIdentifier() && !IsSkippableTopLevelFunctionName(key->AsIdentifier()->Name().Mutf8())) {
                topLevelFunctions_[key->AsIdentifier()->Name().Mutf8()] = member;
            }
            continue;
        }
        if (member->IsClassProperty()) {
            auto *classProperty = member->AsClassProperty();
            const auto *key = classProperty->Key();
            if (key->IsIdentifier() && (classProperty->Modifiers() & ir::ModifierFlags::GETTER_SETTER) == 0U) {
                topLevelProperties_[key->AsIdentifier()->Name().Mutf8()] = classProperty;
            }
            continue;
        }
        if (member->IsClassDeclaration() && member->AsClassDeclaration()->Definition()->IsNamespaceTransformed()) {
            auto *namespaceDef = member->AsClassDeclaration()->Definition();
            topLevelNamespaces_[namespaceDef->Ident()->Name().Mutf8()] = namespaceDef;
        }
    }
}

// Classifies a top-level `ClassDeclaration` statement: the synthetic global class (holding
// top-level functions/properties), a top-level namespace (lowered into a class), or a regular
// top-level class.
void Gluec::CollectClassLikeSymbol(const ir::AstNode *stmt)
{
    auto *classDef = ClassLikeDefinition(stmt);
    if (classDef->IsGlobal()) {
        CollectGlobalClassMembers(classDef);
        return;
    }
    if (classDef->IsNamespaceTransformed()) {
        topLevelNamespaces_[classDef->Ident()->Name().Mutf8()] = classDef;
        return;
    }
    topLevelClasses_[classDef->Ident()->Name().Mutf8()] = classDef;
}

void Gluec::CollectTopLevelSymbols()
{
    // Populated from the ExportFactStore rather than by scanning statements below -- see
    // CollectExportSpecifiers's comment for why a raw ExportNamedDeclaration AST scan can't work
    // here.
    CollectExportSpecifiers();
    for (auto *globalStatement : program_->Ast()->Statements()) {
        if (globalStatement->IsETSImportDeclaration()) {
            auto importDeclaration = globalStatement->AsETSImportDeclaration();
            auto &target = importDeclaration->IsPureDynamic() ? dynamicImportSpecifiers_ : staticImportSpecifiers_;
            CollectSpecifiersFromImport(globalStatement, target);
            continue;
        }
        if (globalStatement->IsETSReExportDeclaration()) {
            CollectSpecifiersFromReExport(globalStatement);
            continue;
        }
        if (globalStatement->IsClassDeclaration() || globalStatement->IsETSStructDeclaration()) {
            CollectClassLikeSymbol(globalStatement);
        }
    }
}

Expected<std::unique_ptr<IntermediateCache>, std::string> Gluec::Run()
{
    auto cache = std::make_unique<IntermediateCache>();
    auto sourceFilePath = NormalizePath(program_->SourceFilePath().Mutf8());
    auto mtime = GetLastModifiedTime(sourceFilePath);
    if (!mtime) {
        const std::string message = "Failed to get last modified time for source file: " + sourceFilePath;
        context_.diagnosticEngine.Error(DiagnosticCode::IO_ERROR, message);
        return Unexpected(message);
    }
    cache->sourceFile = sourceFilePath;
    cache->sourceMTime = FileTimeToString(*mtime);
    cache->globalClassDescriptor = GlobalClassDescriptor();
    CollectTopLevelSymbols();
    BuildExternals(*cache);
    BuildRootSymbols(*cache);
    return cache;
}

// Computes the bytecode descriptor of `classDef` (e.g. "Lentry/some/ETSGLOBAL;"), following the
// same convention as TSDeclGen::PrepareClassDeclaration: "L" + InternalName + ";", with '.'
// replaced by '/'.
std::string Gluec::ClassDescriptor(const ir::ClassDefinition *classDef)
{
    std::string descriptor = "L" + classDef->InternalName().Mutf8() + ";";
    std::replace(descriptor.begin(), descriptor.end(), '.', '/');
    return descriptor;
}

// The descriptor of the file's synthetic global class (ETSGLOBAL), which is the runtime
// container of every top-level function/property.
std::string Gluec::GlobalClassDescriptor() const
{
    return ClassDescriptor(program_->GlobalClass());
}

// Looks up `alias` in the (static or dynamic) import maps. Returns nullptr if `alias` is not an
// imported binding (e.g. it names a genuine local top-level declaration instead).
const ImportBinding *Gluec::FindImportBinding(const std::string &alias) const
{
    auto staticIt = staticImportSpecifiers_.find(alias);
    if (staticIt != staticImportSpecifiers_.end()) {
        return &staticIt->second;
    }
    auto dynamicIt = dynamicImportSpecifiers_.find(alias);
    if (dynamicIt != dynamicImportSpecifiers_.end()) {
        return &dynamicIt->second;
    }
    return nullptr;
}

// Returns the exported alias for `node` (declared locally as `declaredName`) if it carries a
// direct `export`/`export default` modifier, or std::nullopt otherwise. Does not consider a
// separate `export { declaredName as alias };` list -- see `TopLevelExportedName`.
std::optional<std::string> Gluec::DirectlyExportedName(const ir::AstNode *node, const std::string &declaredName) const
{
    if (node->IsDefaultExported()) {
        return std::string("default");
    }
    if (node->IsExported()) {
        return declaredName;
    }
    return std::nullopt;
}

// Same as `DirectlyExportedName`, but additionally covers the `class A {}; export { A };`
// pattern, where the declaration itself has no export modifier, but a separate top-level
// `export { ... };` list names it (recorded in `exportSpecifiers_`).
std::optional<std::string> Gluec::TopLevelExportedName(const ir::AstNode *node, const std::string &declaredName) const
{
    auto direct = DirectlyExportedName(node, declaredName);
    if (direct.has_value()) {
        return direct;
    }
    for (const auto &entry : exportSpecifiers_) {
        if (entry.second == declaredName) {
            return entry.first;
        }
    }
    return std::nullopt;
}

// Builds IntermediateCache::externals from every re-export of the file, merging direct re-exports
// (`export ... from 'xx'`) with the "import-then-export" passthrough pattern (`import { A } from
// 'xx'; export { A };`, which is equivalent to `export { A } from 'xx'`). Re-exports are grouped by
// source module, but a single source may still yield several ExternalLinks: one NAMESPACE link per
// `export *` / `export * as ns` form, plus one NAMED link for all of its `export { ... }` bindings.
void Gluec::BuildExternals(IntermediateCache &cache)
{
    std::unordered_map<std::string, ExternalAccumulator> bySource;

    for (const auto &entry : staticReExportSpecifiers_) {
        const auto &binding = entry.second;
        AddExternalBinding(bySource, binding.source, binding.original, binding.alias, binding.isDynamic);
    }
    for (const auto &entry : dynamicReExportSpecifiers_) {
        const auto &binding = entry.second;
        AddExternalBinding(bySource, binding.source, binding.original, binding.alias, binding.isDynamic);
    }

    for (const auto &entry : exportSpecifiers_) {
        const auto &alias = entry.first;
        const auto &original = entry.second;
        const auto *importBinding = FindImportBinding(original);
        if (importBinding == nullptr) {
            // Not backed by an import: a genuine local declaration, handled by BuildRootSymbols.
            continue;
        }
        AddExternalBinding(bySource, importBinding->source, importBinding->original, alias, importBinding->isDynamic);
    }

    for (auto &entry : bySource) {
        const auto &source = entry.first;
        auto &accum = entry.second;
        // For dynamic modules `source` is the raw import specifier (not an on-disk path), so keep
        // it verbatim; only static, file-backed sources are normalized to a canonical path.
        const auto sourceFile = accum.isDynamic ? source : NormalizePath(source);

        // A source re-exported both as a namespace (`export *` / `export * as ns`) and by name
        // (`export { A } from 'xx'`) must keep both: emit one NAMESPACE link per namespace form and
        // a single NAMED link carrying all named bindings, rather than dropping one of them.
        for (const auto &namespaceAlias : accum.namespaceExports) {
            ExternalLinks link;
            link.isDynamic = accum.isDynamic;
            link.sourceFile = sourceFile;
            link.kind = ExternalLinkKind::NAMESPACE;
            link.exportedName = namespaceAlias;
            cache.externals.push_back(std::move(link));
        }
        if (!accum.namedBindings.empty()) {
            ExternalLinks link;
            link.isDynamic = accum.isDynamic;
            link.sourceFile = sourceFile;
            link.kind = ExternalLinkKind::NAMED;
            link.bindings = std::move(accum.namedBindings);
            cache.externals.push_back(std::move(link));
        }
    }
}

void Gluec::CollectNamespaceMethod(const ir::AstNode *member, const std::string &containerDescriptor,
                                   std::unordered_map<std::string, SymbolNode *> &children)
{
    const auto *key = member->AsMethodDefinition()->Key();
    if (!key->IsIdentifier() || IsSkippableTopLevelFunctionName(key->AsIdentifier()->Name().Mutf8())) {
        return;
    }
    const auto declaredName = key->AsIdentifier()->Name().Mutf8();
    auto exportedName = DirectlyExportedName(member, declaredName);
    if (!exportedName.has_value()) {
        return;
    }
    children[*exportedName] = context_.symbolNodeManager.CreateSymbolNode(*exportedName, SymbolKind::FUNCTION,
                                                                          containerDescriptor, declaredName);
}

void Gluec::CollectNamespaceProperty(const ir::AstNode *member, const std::string &containerDescriptor,
                                     std::unordered_map<std::string, SymbolNode *> &children)
{
    auto *classProperty = member->AsClassProperty();
    const auto *key = classProperty->Key();
    if (!key->IsIdentifier() || (classProperty->Modifiers() & ir::ModifierFlags::GETTER_SETTER) != 0U) {
        return;
    }
    const auto declaredName = key->AsIdentifier()->Name().Mutf8();
    auto exportedName = DirectlyExportedName(classProperty, declaredName);
    if (!exportedName.has_value()) {
        return;
    }
    children[*exportedName] = context_.symbolNodeManager.CreateSymbolNode(*exportedName, SymbolKind::PROPERTY,
                                                                          containerDescriptor, declaredName);
}

void Gluec::CollectNamespaceClassLike(const ir::AstNode *member,
                                      std::unordered_map<std::string, SymbolNode *> &children)
{
    auto *memberClassDef = ClassLikeDefinition(member);
    const auto declaredName = memberClassDef->Ident()->Name().Mutf8();
    auto exportedName = DirectlyExportedName(memberClassDef, declaredName);
    if (!exportedName.has_value()) {
        return;
    }
    if (memberClassDef->IsNamespaceTransformed()) {
        auto *nestedNode = context_.symbolNodeManager.CreateSymbolNode(*exportedName, SymbolKind::NAMESPACE,
                                                                       ClassDescriptor(memberClassDef), declaredName);
        CollectNamespaceChildren(memberClassDef, nestedNode);
        children[*exportedName] = nestedNode;
    } else {
        children[*exportedName] = context_.symbolNodeManager.CreateSymbolNode(
            *exportedName, SymbolKind::CLASS, ClassDescriptor(memberClassDef), declaredName);
    }
}

// Recursively builds SymbolNode children for a namespace's exported members (functions,
// properties, nested classes, nested namespaces). Only direct `export` modifiers are
// considered here -- the top-level-only `export { X };` list passthrough does not apply inside
// a namespace body.
void Gluec::CollectNamespaceChildren(const ir::ClassDefinition *namespaceDef, SymbolNode *parentNode)
{
    const auto containerDescriptor = ClassDescriptor(namespaceDef);
    std::unordered_map<std::string, SymbolNode *> children;
    for (auto *member : namespaceDef->Body()) {
        if (member->IsMethodDefinition()) {
            CollectNamespaceMethod(member, containerDescriptor, children);
        } else if (member->IsClassProperty()) {
            CollectNamespaceProperty(member, containerDescriptor, children);
        } else if (IsClassLikeDeclaration(member)) {
            CollectNamespaceClassLike(member, children);
        }
    }
    parentNode->children = std::move(children);
}

// Builds IntermediateCache::root: one SymbolNode per exported top-level class/function/
// property/namespace (recursing into exported namespace members), plus one
// SymbolKind::INIT_MODULE node per initModule() call.
void Gluec::BuildRootSymbols(IntermediateCache &cache)
{
    // `cache.globalClassDescriptor` is already populated by Run() before this is called.
    const auto &globalDescriptor = cache.globalClassDescriptor;
    auto &manager = context_.symbolNodeManager;

    for (const auto &entry : topLevelClasses_) {
        auto exportedName = TopLevelExportedName(entry.second, entry.first);
        if (!exportedName.has_value()) {
            continue;
        }
        cache.root[*exportedName] = manager.CreateSymbolNode(
            *exportedName, SymbolKind::CLASS, ClassDescriptor(entry.second->AsClassDefinition()), entry.first);
    }

    for (const auto &entry : topLevelFunctions_) {
        auto exportedName = TopLevelExportedName(entry.second, entry.first);
        if (!exportedName.has_value()) {
            continue;
        }
        cache.root[*exportedName] =
            manager.CreateSymbolNode(*exportedName, SymbolKind::FUNCTION, globalDescriptor, entry.first);
    }

    for (const auto &entry : topLevelProperties_) {
        auto exportedName = TopLevelExportedName(entry.second, entry.first);
        if (!exportedName.has_value()) {
            continue;
        }
        cache.root[*exportedName] =
            manager.CreateSymbolNode(*exportedName, SymbolKind::PROPERTY, globalDescriptor, entry.first);
    }

    for (const auto &entry : topLevelNamespaces_) {
        auto exportedName = TopLevelExportedName(entry.second, entry.first);
        if (!exportedName.has_value()) {
            continue;
        }
        const auto *namespaceDef = entry.second->AsClassDefinition();
        auto *node =
            manager.CreateSymbolNode(*exportedName, SymbolKind::NAMESPACE, ClassDescriptor(namespaceDef), entry.first);
        CollectNamespaceChildren(namespaceDef, node);
        cache.root[*exportedName] = node;
    }
}
}  // namespace ark::es2panda::gluegen