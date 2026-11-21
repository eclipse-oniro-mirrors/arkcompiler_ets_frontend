# legacy_bin Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Project Overview

`legacy_bin` packages the legacy API8 compiler toolchain for existing OpenHarmony SDK consumers. It contains platform binaries and a bundled Node frontend under `api8/`; current compiler development follows the appropriate dynamic or static route instead.

### 1.1 Core capabilities

| Dimension | Source of truth |
|---|---|
| Compatibility boundary | API8 package and existing SDK consumers |
| Node entry | `api8/src/index.js` |
| Native backend | Platform artifacts under `api8/bin/` |
| npm package/dependencies | `api8/package.json` and `api8/package-lock.json` |
| SDK packaging | `BUILD.gn` |
| Historical reproduction | `prebuilts-readme-legacy.md` |

The directory name records a compatibility responsibility. The presence of a platform binary or a dependency version does not establish that every language feature or host architecture has been tested.

## 2. Code Map

Paths below are relative to `legacy_bin/` unless explicitly repository-relative.

```text
legacy_bin/
├── api8/
│   ├── bin/linux/             # Linux native artifacts
│   ├── bin/win/               # Windows native artifacts
│   ├── bin/mac/               # macOS native artifacts
│   ├── src/index.js           # Bundled Node frontend
│   ├── package.json           # Package metadata and scripts
│   ├── package-lock.json      # Locked dependency graph
│   └── manifest_tag.xml       # Packaged manifest metadata
├── BUILD.gn                   # Prebuilt SDK packaging
└── prebuilts-readme-legacy.md  # Historical checkout/build instructions
```

Installed `node_modules/` and local build output may be absent in a clean checkout. GN packaging also uses external prebuilt dependencies; inspect the actual `source` entries before claiming everything is bundled in this directory.

### 2.1 Compilation and consumer chain

```text
Legacy JS/TS input
    → bundled Node frontend
    → matching legacy native backend
    → legacy ABC
    → matching SDK/runtime consumer
```

`BUILD.gn` copies package files and native binaries into host-specific SDK layouts. `js-loader`/`ets-loader` consumers depend on the package location and invocation contract, not only the native file's existence.

### 2.2 Components

| Component | Responsibilities / review points |
|---|---|
| Native binary | File format, architecture, linked libraries, runtime compatibility |
| Bundled frontend | CLI behavior, backend discovery, emitted representation |
| npm metadata | Package identity, entry point, dependency versions |
| GN packaging | Source/destination paths, host selection, external prebuilts |
| Manifest | SDK package integration |

## 3. Knowledge Routing

| Task | Read first |
|---|---|
| Compatibility/packaging | Repository `AGENTS.md`, local `BUILD.gn` |
| Legacy build provenance | `prebuilts-readme-legacy.md` |
| Current dynamic compiler behavior | Repository-relative `docs/knowledge/ES2Panda_Knowledge_Base.md` |
| ABC merging | Repository-relative `merge_abc/AGENTS.md` |
| Obfuscation | Repository-relative `docs/knowledge/ArkGuard_Knowledge_Base.md` |

## 4. Constraints and Compatibility

### 4.1 Do not

- Do not remove or replace legacy artifacts merely because current compilers exist.
- Do not hand-edit bundled/generated frontend output; locate the original build inputs and reproduction process.
- Do not infer macOS arm64 support from a directory named `mac`; inspect the actual binary architecture and consumer requirements.
- Do not claim historical platform/feature tests passed without current execution evidence.
- Do not replace the bundled TypeScript dependency with an unreviewed current version.
- Do not document a `require(...).compile()` API unless the bundled module actually exports that contract.
- Do not describe current dynamic es2panda as a full TypeScript semantic checker; its removed in-tree checker is unavailable.

### 4.2 Ask before

Ask before deleting compatibility shims/legacy behavior, replacing packaged binaries, changing public SDK layout, or adding dependencies. Serialized compatibility changes require approval and coordinated consumer validation.

### 4.3 Compatibility assessment

The supported contract is determined by the legacy package, its producing toolchain, and target runtime. Test syntax/features against those artifacts rather than maintaining an unsupported table claiming complete ES/TS/decorator/async support.

Dependency versions belong in the package manifest/lockfile. Historical OpenHarmony release information in `prebuilts-readme-legacy.md` is a reproduction input, not a statement of the current repository version.

## 5. Setup, Usage, and Verification

### 5.1 Prerequisites

Use the Node/npm versions required by the package-producing environment and consumer SDK. Verify the host binary's architecture, executable permissions, and dynamic dependencies. Do not infer the minimum supported Node version from an old documentation example.

### 5.2 Local package setup

From the ets_frontend repository root:

```bash
cd legacy_bin/api8
npm install
npm ls
node src/index.js --help
```

Review lockfile differences if setup changes dependency resolution. A dependency update requires the matching lockfile and nearby package/SDK validation.

### 5.3 Basic compilation

Run from `legacy_bin/api8/` after preparing its matching backend and dependencies:

```bash
node src/index.js input.js -o output.abc
node src/index.js input.ts -o output.abc
```

Confirm option spellings and combinations with the bundled frontend's actual help. Do not copy current `es2abc` flags such as output-directory, source-map, or optimization options into legacy instructions without checking they are implemented.

### 5.4 Programmatic invocation

Use the supported CLI/loader contract. Before adding a programmatic example, inspect actual module exports and the consumer. A hypothetical `compile(path, options)` export is not evidence that the bundled frontend provides one.

### 5.5 Binary inspection

Inspect each selected binary using host tools such as `file` and the platform's dependency inspection tool. Record OS/architecture, backend provenance, dynamic-library availability, and the actual compile/execute result.

Byte size and bundled-source size change with rebuilds; consult the artifacts rather than freezing approximate sizes in this guide. Do not label every platform as tested based on one host run.

### 5.6 SDK build and output

`prebuilts-readme-legacy.md` describes the historical release checkout, prebuilt preparation, and SDK build used to reproduce these artifacts. Run its build commands from that checkout's OpenHarmony root, not from `legacy_bin/`.

Inspect `BUILD.gn` for SDK output paths (`js_linux`, `js_win`, `js_mac`) and external `node_modules` sources. The SDK package directory is the consumer contract; `/usr/local/bin/js2abc` is not a guaranteed installation destination.

### 5.7 Maintenance scenarios

| Scenario | Required work |
|---|---|
| API8 regression | Reproduce on legacy artifacts; preserve compatible output/consumer behavior |
| Dependency security fix | Update source manifest/lockfile, rebuild package, validate consumers |
| Host packaging failure | Check binary architecture, copied paths, permissions, loader lookup |
| New current-language feature | Route to the appropriate current compiler; do not silently extend legacy ABI |
| Rebuilt artifacts | Record provenance and compare package contents/legacy behavior |

### 5.8 Version management

Change package versions according to the release/consumer process. Update `package.json`, lockfile, packaged manifest, and SDK wiring together where applicable. Do not invent an example patch version as a required value.

### 5.9 Verification checklist

1. Build/reproduce the affected package using its source-of-truth workflow.
2. Inspect the selected host artifacts and dependency graph.
3. Compile representative legacy JS/TS inputs and an expected-error input.
4. Run the resulting ABC on the matching runtime and compare diagnostics/behavior.
5. Build the nearest SDK/loader consumer and inspect packaged paths/content.
6. Record unsupported hosts, absent historical toolchain, and untested runtime combinations.

For help and issue tracking, use the repository's configured upstream and the relevant consumer owners. A successful npm install alone does not establish legacy runtime compatibility.
