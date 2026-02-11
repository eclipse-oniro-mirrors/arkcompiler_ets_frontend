# Arkguard – AI Knowledge Base (Source Code Obfuscation)

---

## 1. Basic Information

| Attribute | Value |
|-----------|--------|
| Repository name | arkguard |
| Subsystem | arkcompiler / ets_frontend |
| Primary language | TypeScript |
| Version | 1.1.3 |
| Last updated | 2026-02-11 |

---

## 2. Directory Structure

```
arkguard/
├── bin/
│   └── secharmony                    # CLI entry; invokes the obfuscator with rule files
├── src/
│   ├── ArkObfuscator.ts              # Main obfuscator: parses options, runs pipeline, returns obfuscated output
│   ├── IObfuscator.ts                # Contract for obfuscation (e.g. obfuscate(sourceFile, options))
│   ├── cli/
│   │   └── SecHarmony.ts             # Commander-based CLI; reads rule file, builds options, calls ArkObfuscator
│   ├── common/
│   │   ├── ApiExtractor.ts           # Extracts public API names from OpenHarmony SDK for whitelist
│   │   ├── ApiReader.ts              # Reads API list (e.g. from JSON) used to avoid obfuscating SDK symbols
│   │   └── type.ts                   # Shared types (options, result shapes, etc.)
│   ├── configs/
│   │   ├── IOptions.ts               # Full obfuscation options (identifiers, properties, files, console, etc.)
│   │   ├── INameObfuscationOption.ts # Options for name obfuscation (generator, reserved names)
│   │   └── preset/
│   │       ├── es_reserved_properties.json           # ES built-in property names that must not be renamed
│   │       └── es_reserved_properties_optimized.json  # Optimized version of the reserved properties list
│   ├── generator/
│   │   ├── INameGenerator.ts         # Interface for generating short obfuscated names
│   │   ├── NameFactory.ts            # Creates the appropriate name generator from config
│   │   └── OrderedNameGenerator.ts   # Deterministic sequential names (a, b, c, …) for stable builds
│   ├── initialization/
│   │   ├── ConfigResolver.ts         # Parses obfuscation rule file into IOptions
│   │   ├── Initializer.ts            # One-time setup (API reader, config resolution)
│   │   └── utils.ts                 # Helpers for initialization
│   ├── transformers/                 # AST transformation plugins (core obfuscation logic)
│   │   ├── TransformPlugin.ts        # Interface and TransformerOrder enum defining execution order
│   │   ├── TransformerManager.ts     # Loads and runs transformers in a fixed order
│   │   ├── layout/
│   │   │   └── DisableConsoleTransformer.ts   # Removes or stubs console.log/error/warn
│   │   └── rename/
│   │       ├── RenameIdentifierTransformer.ts  # Parameters, locals, globals, exports
│   │       ├── RenamePropertiesTransformer.ts   # Object/class property names
│   │       ├── RenameFileNameTransformer.ts     # Source file and directory names
│   │       ├── ShorthandPropertyTransformer.ts # Expands a: a to full form before property rename
│   │       └── VirtualConstructorTransformer.ts # Handles struct (ETS) constructors
│   └── utils/
│       ├── ScopeAnalyzer.ts          # Builds scope tree and binding map for safe identifier rename
│       ├── NameCacheUtil.ts          # Load/save namecache.json for incremental obfuscation
│       ├── SourceMapUtil.ts          # Generates source maps for obfuscated output
│       ├── TransformUtil.ts          # Shared helpers for AST transformation
│       ├── CommonCollections.ts      # Shared caches (e.g. name → new name)
│       └── FileUtils.ts              # Reading/writing files and directories
├── test/
│   ├── ut/                           # Unit tests (e.g. Mocha)
│   ├── grammar/                      # Integration tests with expected obfuscated output
│   ├── combinations/                 # Tests for option combinations
│   └── benchmark/                    # Performance benchmarks
├── scripts/
│   ├── grammar_test.py               # Runs grammar/integration tests
│   └── combination_test.py           # Runs combination tests
├── package.json
├── BUILD.gn
├── tsconfig.json
├── README.md
└── README-cn.md
```

### 2.1 Key Directories

| Directory | Role |
|-----------|------|
| **src/transformers/** | All AST-based obfuscation: identifier rename, property rename, file name rename, shorthand expansion, console removal. Order of execution is defined in TransformerManager. |
| **src/utils/** | ScopeAnalyzer is required before RenameIdentifierTransformer. NameCacheUtil ensures consistent names across builds. SourceMapUtil produces source maps for debugging. |
| **src/initialization/** | ConfigResolver turns the obfuscation rule file (and any CLI overrides) into IOptions. Initializer wires config, API reader, and optional name cache. |
| **src/generator/** | Produces short, unique names for identifiers and properties. OrderedNameGenerator gives reproducible names for incremental builds. |
| **src/common/** | ApiReader/ApiExtractor provide the SDK whitelist so framework and system API names are not obfuscated. |
| **test/ut/** | Unit tests for individual components (e.g. ConfigResolver, transformers, ScopeAnalyzer). |
| **test/grammar/** | End-to-end tests: input source + options → expected obfuscated code (60+ cases). |

---

## 3. Repository Overview

### 3.1 Introduction

Arkguard is a **source-code obfuscation tool** for OpenHarmony applications. It operates on **ETS, TypeScript, and JavaScript** source code and performs **AST-level transformations** to protect intellectual property and reduce readability of shipped code. It integrates with DevEco Studio and the OpenHarmony build system and supports **HAP**, **HAR**, and **HSP** module types.

### 3.2 Core Features (Detailed)

| Feature | Description | Notes |
|---------|-------------|--------|
| **Identifier renaming** | Renames function parameters, local variables, top-level (global) names, and exported symbols. | Respects scope; SDK and reserved names are excluded by whitelist. |
| **Property obfuscation** | Renames object literal and class property names. | ShorthandPropertyTransformer expands `{ a }` to `{ a: a }` so the property can be renamed consistently. |
| **File name obfuscation** | Renames source file names and directory names in the output. | Helps hide module layout; use with care for HAR/HSP public APIs. |
| **Export name obfuscation** | Renames exported names. | Can break cross-module imports unless `-keep-dts` or `-keep-global-name` is used for public API. |
| **Console removal** | Removes or stubs `console.log`, `console.error`, `console.warn` (and optionally other console methods). | Controlled by `-remove-log`. |
| **Code compaction** | Removes unnecessary whitespace and newlines. | Option `-compact`. |
| **Comment removal** | Removes single-line, multi-line, and JSDoc comments. | Option `-remove-comments`; `-keep-comments` can preserve JSDoc. |
| **Source map** | Emits source maps for the obfuscated output. | Enables debugging with original source; path and options are configurable. |
| **Incremental builds** | Uses a name cache file (`namecache.json` or path from `-print-namecache` / `-apply-namecache`). | Ensures the same original name always maps to the same obfuscated name across builds. |

---

## 4. Tech Stack

- **Languages:** TypeScript (main), Python (test/script runner).
- **Libraries:** TypeScript Compiler API (AST parse/transform), Commander.js (CLI), `source-map` (source map generation), `magic-string` (efficient string edits during transform).
- **Build:** GN + Ninja (OpenHarmony), npm/tsc for development and tests.

---

## 5. Build and Test

### 5.1 Prerequisites (First-time Setup)

```bash
npm install                  # Install npm dependencies
./scripts/install_tsc.sh     # Replace with OpenHarmony TypeScript (from third_party/typescript)
```

**Note:** `install_tsc.sh` builds and installs the OpenHarmony-specific TypeScript compiler, which is required for ETS language support. Run this script from the arkguard root directory.

### 5.2 Build

```bash
npm run build
# Or full OpenHarmony build:
# ./build.sh --product-name rk3568 --build-target arkguard
```

### 5.3 Test

```bash
npm run pre_run_test   # Install test dependencies
npm run test           # Unit tests and grammar/integration tests
npm run test:ut        # Unit tests only
npm run test:grammar   # Grammar/integration tests
npm run test:combinations   # Combination tests
npm run coverage       # Coverage report
npm run benchmark      # Performance
```

### 5.4 Artifacts

- Compiled JavaScript: `lib/`
- Packed npm package: e.g. `arkguard-1.1.3.tgz`

---

## 6. Architecture

### 6.1 Processing Pipeline

1. **Input:** Source file set and obfuscation rule file (and optionally name cache).
2. **ConfigResolver:** Parses the rule file into `IOptions` (which features are on/off, whitelists, paths).
3. **ApiReader:** Loads SDK/public API whitelist so those names are not renamed.
4. **TypeScript parser:** Builds AST for each source file.
5. **ScopeAnalyzer:** Builds scope tree and bindings for each file so renames are scope-correct and consistent.
6. **TransformerManager:** Runs transformers in order:
   - ShorthandPropertyTransformer (expand shorthand properties)
   - RenameIdentifierTransformer (parameters, locals, globals, exports)
   - VirtualConstructorTransformer (handles struct / ETS constructors)
   - DisableConsoleTransformer (remove console calls)
   - RenamePropertiesTransformer (property names)
   - RenameFileNameTransformer (file/directory names)
7. **Output:** Obfuscated source, optional source map, and optional updated name cache.

### 6.2 Design Patterns

- **Plugin-style transformers:** Each transformation is a separate plugin; TransformerManager orders and runs them.
- **Scope-based renaming:** ScopeAnalyzer ensures that only names in the correct scope are renamed and that references stay consistent.
- **Whitelist-first:** By default, names that might affect runtime or public API are preserved; aggressive obfuscation is opt-in via options and whitelists.

---

## 7. Configuration Reference

### 7.1 Obfuscation Rule File (Summary)

| Option | Effect |
|--------|--------|
| `-enable-property-obfuscation` | Enable renaming of object/class properties. |
| `-enable-filename-obfuscation` | Enable renaming of source file and directory names. |
| `-enable-export-obfuscation` | Enable renaming of exported names. |
| `-compact` | Compress output (e.g. single line). |
| `-remove-log` | Remove or stub console.* calls. |
| `-remove-comments` | Remove all comments. |
| `-disable-obfuscation` | Turn off all obfuscation. |
| `-keep-property-name [names]` | Do not rename these property names (supports wildcards). |
| `-keep-global-name [names]` | Do not rename these global/exported names. |
| `-keep-file-name [names]` | Do not rename these file names. |
| `-keep-dts <path>` | Preserve names that appear in this declaration file (e.g. public API). |
| `-keep <path>` | Do not obfuscate files under this path. |
| `-print-namecache <path>` | Write the current name mapping to this file (for next build). |
| `-apply-namecache <path>` | Load name mapping from this file for consistent renames. |
| `-keep-comments [names]` | Preserve certain JSDoc comments. |

### 7.2 Wildcards in Keep Options

- `?` – single character.
- `*` – any characters except path separators.
- `**` – any characters including path separators.

### 7.3 Programmatic API

```typescript
import { ArkObfuscator } from 'arkguard';

const obfuscator = new ArkObfuscator();
// Set options (same as rule file) then:
const result = obfuscator.obfuscate(sourceFile, options);
// result: { content, sourceMap?, nameCache?, filePath? }
```

---

## 8. Common Pitfalls and Solutions

### 8.1 Property obfuscation breaks dynamic property access

- **Problem:** With `-enable-property-obfuscation`, any property accessed by a **runtime-computed** key (e.g. `obj[variable]`) will not be renamed in sync; if the key is a string that was obfuscated elsewhere, runtime access can break.
- **Solution:** Add property names that are used dynamically to `-keep-property-name` so they are not renamed.

### 8.2 Export obfuscation breaks cross-module imports

- **Problem:** With `-enable-export-obfuscation`, HAR/HSP modules’ public exports may be renamed, so other modules that import those names will fail at runtime or at link time.
- **Solution:** Use `-keep-dts` pointing at the public API declaration file, or list exported names in `-keep-global-name`.

### 8.3 Incremental build inconsistency

- **Problem:** Without a name cache, each run can assign different obfuscated names to the same identifier, causing inconsistencies across builds or between modules.
- **Solution:** In production, use `-print-namecache` on the first (or reference) build and `-apply-namecache` on subsequent builds so renames are stable.

### 8.4 UI / decorator properties must not be obfuscated

- **Problem:** ArkUI decorators (e.g. `@Component`, `@State`, `@Prop`) rely on fixed property names. Built-in decorators are usually preserved; custom decorators are not.
- **Solution:** Add any custom decorator property names to `-keep-property-name`.

### 8.5 Reserved and built-in names

- **Problem:** Renaming ES reserved properties (e.g. `constructor`, `prototype`) or OpenHarmony SDK API names can break runtime.
- **Solution:** Arkguard uses `es_reserved_properties.json` and the ApiReader whitelist to avoid renaming these. Do not remove names from these sets unless you know the impact.

---

## 9. References

- README.md (English) and README-cn.md (Chinese) in the arkguard repository root
- Version history (e.g. Version.md in the repo)

---

## 10. Version History

| Version | Date | Changes |
|---------|------|--------|
| 1.0 | 2026-02-11 | Initial AI knowledge base version. |
