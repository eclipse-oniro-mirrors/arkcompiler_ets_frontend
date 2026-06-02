# ArkGuard Source Obfuscation Knowledge Base

> Scope: ArkGuard source obfuscation for agent code changes and troubleshooting (guardrails + code map).
> Source trees: `arkcompiler/ets_frontend/arkguard` (engine) and `developtools/ace_ets2bundle/compiler` (Release build integration).

## When to Read / When Not

| Situation | Action |
|-----------|--------|
| `-keep-*` / `obfuscation-rules.txt` / `nameCache.json` / `keptNames.json` / property or export obfuscation behavior | **Read this doc** (engine + rule mapping + cache lifecycle) |
| Release-only obfuscation not applied, Hvigor `arkOptions.obfuscation`, ace Rollup plugin / `ObConfigResolver` merge issues | **Read this doc** (ACE Integration section) |
| Runtime crash after obfuscated Release build (wrong symbol at runtime) | **Read this doc** (Cache / State + reserved-name tables), then verify `keptNames.json` and API cache |
| General `ets_frontend` `./build.sh` targets, GN module wiring, or “how to build the repo” | **Do not start here** → `architecture/Build_Test_Knowledge_Base.md` |
| `es2panda` / `ets2panda` parser, checker, lowering, bytecode emission | **Do not start here** → matching compiler route knowledge base |
| `merge_abc`, `.abc` merge, protobuf serialization | **Do not start here** → `architecture/Build_Test_Knowledge_Base.md` |
| Bytecode-only obfuscation enabled (source obfuscation skipped by design) | Skim **ACE Integration** mutual-exclusion notes only; engine AST path may not run |

---

## Engine (`arkcompiler/ets_frontend/arkguard`)

In-repo obfuscation engine: rule parsing, AST transformers, name generation, whitelist extraction, and cache files written by `ArkObfuscator`.

### Transformer Execution Order

`TransformerManager` loads plugins in ascending `order` and runs them in `obfuscateAst()`:

| order | Plugin | Responsibility |
|-------|--------|----------------|
| `SHORTHAND_PROPERTY` | `ShorthandPropertyTransformer` | Expand shorthand properties |
| `RENAME_IDENTIFIER` | `RenameIdentifierTransformer` | Identifiers / parameters / locals / toplevel names |
| `VIRTUAL_CONSTRUCTOR` | `VirtualConstructorTransformer` | Constructor-related renaming |
| `DISABLE_CONSOLE` | `DisableConsoleTransformer` | `-remove-log` |
| `RENAME_PROPERTIES` | `RenamePropertiesTransformer` | Property name obfuscation |
| `RENAME_FILE_NAME` | `RenameFileNameTransformer` | File name obfuscation |

### Core Concepts (Common Misuse)

| Concept | What it is | Common misuse |
|---------|------------|---------------|
| `ArkObfuscator` | Obfuscation executor | Confused with `ObConfigResolver` in ace (rule parsing / merge only) |
| `MergedConfig` | Merged rules + reserved names | Treated as raw `obfuscation-rules.txt` on disk |
| `ruleOptions.files` | Rules for current module | Confused with `consumerFiles` (passed downstream) |
| `nameCache.json` | Stable obfuscated names across builds | Confused with `keptNames.json` (unobfuscated names + reasons) |
| Source / bytecode obfuscation | Mutually exclusive paths | Assumed to run together (`Initializer` picks one) |
| `process.env.compiler === 'on'` | Rollup **first-pass** collection | Assumed `collectReservedNameForObf` runs on all paths |

### Obfuscation Dimensions

| Dimension | Default | Enable option | Primary code |
|-----------|---------|---------------|--------------|
| Local / parameter names | **On** | `-keep-parameter-names` to keep | `arkcompiler/ets_frontend/arkguard/src/transformers/RenameIdentifierTransformer.ts` |
| Toplevel names | Off | `-enable-toplevel-obfuscation` | Same as above |
| Property names | Off | `-enable-property-obfuscation` | `arkcompiler/ets_frontend/arkguard/src/transformers/RenamePropertiesTransformer.ts` |
| String properties | Kept | `-enable-string-property-obfuscation` | `mKeepStringProperty = false` |
| Exported members | Off | `-enable-export-obfuscation` | Identifier + Properties together |
| File names | Off | `-enable-filename-obfuscation` | `arkcompiler/ets_frontend/arkguard/src/transformers/RenameFileNameTransformer.ts`, ace `mangleFilePath` |

### Reserved Name Sources (Common Misuse)

| Source | Protects | Common misuse |
|--------|----------|---------------|
| `-keep-property-name` | Explicit property names | Expecting to keep dynamic `obj[key]` |
| `-keep-global-name` | Toplevel / exported symbols | Keep fails after property obf mutates AST (Identifier order matters) |
| `systemApiCache.json` | SDK API names | Expecting string constant values in declarations (e.g. Want action) |
| ArkUI Struct fields | `@Component` members | Treated as ordinary class properties |
| Direct export fields | e.g. `export class { data }` | Nested `person.name` still needs `-keep-property-name` |
| `@Keep` | Annotation-marked symbols | Used without `-enable-at-keep` |
| Native `.d.ts` | SO exported APIs | Not included in scan paths |

### Rule Line → Code Mapping (High Frequency)

| Rule line | `ObOptions` | `IOptions` / behavior |
|-----------|-------------|----------------------|
| `-disable-obfuscation` | `disableObfuscation` | Early return, no obfuscation |
| (default) | — | Local / parameter name obfuscation |
| `-enable-property-obfuscation` | `enablePropertyObfuscation` | `mRenameProperties` |
| `-enable-string-property-obfuscation` | `enableStringPropertyObfuscation` | Obfuscate string literal properties |
| `-enable-toplevel-obfuscation` | `enableToplevelObfuscation` | `mTopLevel` |
| `-enable-export-obfuscation` | `enableExportObfuscation` | `mExportObfuscation` |
| `-enable-filename-obfuscation` | `enableFileNameObfuscation` | `mRenameFileName` |
| `-keep-global-name` | `reservedGlobalNames` | `mReservedToplevelNames` |
| `-keep-property-name` | `reservedPropertyNames` | `mReservedProperties` |
| `-keep-file-name` | `reservedFileNames` | `mReservedFileNames` |
| `-print-namecache` / `-apply-namecache` | — | `NameCacheUtil` |
| `-print-kept-names` | `printKeptNames` | Write / read `keptNames.json` |

Merge policy (applied in ace `ObConfigResolver`): `enable` options OR-merge across dependency chain; `-keep-*` union; wildcards via `wildcardTransformer`. HAR builds emit consumer config; HAP does not generate `obfuscation.txt`.

### Engine Constraints

- Do not edit `arkcompiler/ets_frontend/arkguard/lib/` by hand; change `arkcompiler/ets_frontend/arkguard/src/` then `npm run build`; OpenHarmony integration uses `arkcompiler/ets_frontend/arkguard/BUILD.gn` → `arkcompiler/ets_frontend/arkguard/compile_arkguard.py`.
- Do not change `OptionType` without updating parsing, merge logic (ace + engine), and unit tests.
- `-enable-string-property-obfuscation` applies **only** when property names are valid identifiers; do not enable for literals with special characters.
- Do not enable property/toplevel obfuscation by default in unit test baselines.
- `RENAME_IDENTIFIER` `order` **must** precede plugins that `factory.update*` the AST. Reason: cleared `NodeFlags` breaks `-keep-global-name` in `.d.ts` namespace scenarios.
- Do not add plugins with `order < RENAME_IDENTIFIER` that rewrite nodes.
- Do not read rule files from disk inside Transformers; use `IOptions` / global collections only.
- Struct collection belongs in ace `collectReservedNameForObf` (compiler plugin); do not rescan the whole project inside Properties transformer.
- Do not remove or weaken `needKeepSystemApi` without adding tests.
- Do not manually edit `systemApiCache.json` long-term; fix scan logic and regenerate via Release build.
- When changing `ApiExtractor` scan scope, balance performance vs whitelist completeness.

### Cache / State / Lifecycle (Engine Artifacts)

| State | Location | Clear trigger |
|-------|----------|---------------|
| Obfuscated name map | `{obfuscationCacheDir}/nameCache.json` | New project, `-apply-namecache` change |
| Unobfuscated names | `{obfuscationCacheDir}/keptNames.json` | End of compile, `clearHistoryUnobfuscatedMap` |
| System API | `{obfuscationCacheDir}/systemApiCache.json` | SDK upgrade, related option changes |
| Incremental paths | `FilePathManager` / `FileContentManager` | Deleted source files → `updateIncrementalCaches` |
| Global collections | `PropCollections`, `UnobfuscationCollections`, etc. | `clearGlobalCaches()` |

`obfuscationCacheDir` is set by ace from the project build output (under the module `build/` tree). Inspect that directory on device HAP/HAR builds, not only unit-test output.

Runtime crash triage: check `keptNames.json` first, then missing `-keep-*` or stale API cache.

### Engine Pre-Change Checklist

- [ ] New rule line updated in `OptionType`, parsing, merge, and unit tests?
- [ ] New Transformer `order` correct? default export / namespace scenarios intact?
- [ ] Property/export obfuscation synced with whitelist and `UnobfuscationCollections`?
- [ ] Incremental path updates `shouldReObfuscate` / `fileNamesMap`?
- [ ] NameCache / SourceMap behavior consistent with official docs?

### Engine Code Anchors

| Scenario | Path |
|----------|------|
| Engine entry | `arkcompiler/ets_frontend/arkguard/src/ArkObfuscator.ts` |
| Init / config mapping | `arkcompiler/ets_frontend/arkguard/src/initialization/Initializer.ts`, `arkcompiler/ets_frontend/arkguard/src/initialization/ConfigResolver.ts` |
| Transformer plugins | `arkcompiler/ets_frontend/arkguard/src/transformers/` |
| Symbol / Struct collection | `arkcompiler/ets_frontend/arkguard/src/utils/NodeUtils.ts`, `arkcompiler/ets_frontend/arkguard/src/utils/ScopeAnalyzer.ts` |
| API whitelist | `arkcompiler/ets_frontend/arkguard/src/common/ApiExtractor.ts`, `arkcompiler/ets_frontend/arkguard/src/common/ApiReader.ts` |
| Name generation | `arkcompiler/ets_frontend/arkguard/src/generator/NameFactory.ts` |
| Global collections | `arkcompiler/ets_frontend/arkguard/src/utils/CommonCollections.ts` |
| OpenHarmony packaging | `arkcompiler/ets_frontend/arkguard/BUILD.gn`, `arkcompiler/ets_frontend/arkguard/compile_arkguard.py` |

| Tests | Path |
|-------|------|
| Engine | `arkcompiler/ets_frontend/arkguard/test/ut/arkobfuscator/ArkObfuscator.spec.ts` |
| Config | `arkcompiler/ets_frontend/arkguard/test/ut/initialization/ConfigResolver.spec.ts`, `arkcompiler/ets_frontend/arkguard/test/ut/initialization/ConfigResolver-hsp.spec.ts` |
| Transformer | `arkcompiler/ets_frontend/arkguard/test/ut/transformer/RenameIdentifierTransformer.spec.ts`, `arkcompiler/ets_frontend/arkguard/test/ut/transformer/RenamePropertiesTransformer.spec.ts` |
| Whitelist | `arkcompiler/ets_frontend/arkguard/test/ut/utils/ApiExtractor.spec.ts`, `arkcompiler/ets_frontend/arkguard/test/ut/utils/ApiReader.spec.ts`, `arkcompiler/ets_frontend/arkguard/test/ut/utils/NodeUtils.spec.ts` |

---

## ACE Integration (`developtools/ace_ets2bundle/compiler`)

Out-of-tree (relative to `ets_frontend`) but required for end-to-end obfuscation: Hvigor Release wiring, rule merge, Rollup transform hook, and cache directory setup.

### Main Pipeline

| Stage | Owner | Description |
|-------|-------|-------------|
| 1. Hvigor project parsing | `ace_ets2bundle` | `build-profile.json5` → `arkOptions.obfuscation`; enabled only in **Release** when `ruleOptions.enable` is true (including dependency chain) |
| 2. Rule merging | `ObConfigResolver.resolveObfuscationConfigs()` | Merge self / dependency / HAR `obfuscation.txt` → `MergedConfig` → `obfuscationMergedObConfig` |
| 3. Engine init | `initObfuscationConfig()` | Construct `ArkObfuscator`, read `nameCache.json` / `keptNames.json`; skip ArkGuard when bytecode obfuscation is enabled |
| 4. Whitelist scan | `ApiReader` + `ApiExtractor` | Scan SDK/project declarations → `systemApiCache.json` |
| 5. Per-file compile | Rollup plugin | `collectReservedNameForObf` → TS transform → `obfuscate()` |
| 6. Emit | `ArkObfuscator` + ace resolver | Obfuscated source, NameCache, keptNames, SourceMap |

```text
obfuscation-rules.txt / consumer-rules.txt / HAR obfuscation.txt
  → ObConfigResolver (ace) merge → Initializer (arkguard) → ArkObfuscator.init()
  → [optional] systemApiCache.json
  → collectReservedNameForObf → transform(ast, transformers) → write to disk

Hvigor Release → ace_ets2bundle → initObfuscationConfig → obfuscate each source file
```

Interop builds mirror several files under `compiler/src/interop/src/fast_build/`; when debugging interop-only behavior, check both the `fast_build/` and `interop/src/fast_build/` copies listed below.

### ACE Integration Constraints

- Do not assume FA model or non-Stage projects support obfuscation.
- Source and bytecode obfuscation are **mutually exclusive**; verify both paths when changing option merge logic in ace.
- When `-enable-export-obfuscation` and `-enable-toplevel-obfuscation` are both on, system API whitelist **must** be loaded. Reason: collisions with user-defined symbols cause mass incorrect obfuscation.
- End-to-end validation applies **only** to ace Release builds with obfuscation enabled; `npm test` under arkguard covers engine unit tests only.
- HAP/HAR device builds must inspect artifacts under `obfuscationOptions.obfuscationCacheDir`; unit tests alone are insufficient.
- Call `clearGlobalCaches()` at module compile end; do not leak global collections between unit tests.

### ACE Pre-Change Checklist

- [ ] ace call order or Rollup `collectReservedNameForObf` conditions affected?
- [ ] `ObConfigResolver` merge behavior changed for HAP/HAR/HSP dependency chains?

### ACE Code Anchors

| Scenario | Path |
|----------|------|
| Rule merge / resolver | `developtools/ace_ets2bundle/compiler/src/fast_build/ark_compiler/common/ob_config_resolver.ts` |
| Interop rule merge (mirror) | `developtools/ace_ets2bundle/compiler/src/interop/src/fast_build/ark_compiler/common/ob_config_resolver.ts` |
| Init hook (`initObfuscationConfig`) | `developtools/ace_ets2bundle/compiler/src/fast_build/ark_compiler/common/process_ark_config.ts` |
| Interop init (mirror) | `developtools/ace_ets2bundle/compiler/src/interop/src/fast_build/ark_compiler/common/process_ark_config.ts` |
| Interop static config filter | `developtools/ace_ets2bundle/compiler/src/interop/src/fast_build/ark_compiler/interop/process_obfuscation_config.ts` |
| Rollup plugin | `developtools/ace_ets2bundle/compiler/src/fast_build/ets_ui/rollup-plugin-ets-typescript.ts` |
| Interop Rollup plugin (mirror) | `developtools/ace_ets2bundle/compiler/src/interop/src/fast_build/ets_ui/rollup-plugin-ets-typescript.ts` |

| Tests | Path |
|-------|------|
| Obfuscation config (interop) | `developtools/ace_ets2bundle/compiler/test/ark_compiler_ut/interop/process_obfucation_config.test.ts` |
| ObConfigResolver | `developtools/ace_ets2bundle/compiler/test/ark_compiler_ut/common/ob_config_resolver.test.ts` |
| process_ark_config | `developtools/ace_ets2bundle/compiler/test/ark_compiler_ut/common/process_ark_config.test.ts` |

---

## Verification

### Build Commands

```sh
# OpenHarmony full build (from OH source root)
./build.sh --product-name <product> --build-target //arkcompiler/ets_frontend/arkguard:build_arkguard

# Engine module only (from arkcompiler/ets_frontend/arkguard/)
npm install && npm run build    # compile src/ → lib/
npm run test:ut                 # unit tests
npm run test:grammar            # grammar regression
npm run test:combinations       # combination regression
```

### Verification Tiers

1. **Engine**: `npm run test:ut` / grammar / combinations under `arkcompiler/ets_frontend/arkguard/`
2. **ACE unit**: `developtools/ace_ets2bundle/compiler/test/ark_compiler_ut/` cases above
3. **End-to-end**: Release HAP/HAR build with obfuscation enabled; validate `nameCache.json`, `keptNames.json`, obfuscated sources, and SourceMap under `obfuscationCacheDir`

General frontend build routing (not obfuscation-specific): `architecture/Build_Test_Knowledge_Base.md`.

---

## Known Drift

| Item | Notes |
|------|-------|
| `arkcompiler/ets_frontend/arkguard/README-cn.md` | Unmaintained; on conflict with behavior, prefer official product docs + `ConfigResolver` / `ObConfigResolver` implementation |
| Interop duplicate sources | `compiler/src/fast_build/` vs `compiler/src/interop/src/fast_build/` — fixes may need both paths for interop-enabled projects |
| Test filename typo | `process_obfucation_config.test.ts` (missing “s” in obfuscation) is the on-disk name; do not “fix” in docs only |
