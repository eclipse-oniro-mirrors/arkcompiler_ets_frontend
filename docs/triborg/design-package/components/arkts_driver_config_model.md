# Design Package Component: arkts_driver_config_model

This file is copied from the approved Triborg design package during implementator preflight.

# Component Design: ArkTS Driver Config Model

## Summary
The ArkTS Driver Config Model is affected because the approved architecture assigns it ownership of build-system schemas, constants, dependency graph metadata, generated compiler configuration, and resolved build configuration records. The existing TypeScript reference modules exist, but no ArkTS config-model implementation is present yet, so the component requires detailed component-local tasks.

## Top-Level Alignment
This component provides ArkTS-native data contracts and pure transformations consumed by the ArkTS sequential orchestrator and interop runtime. It preserves TypeScript-driver parity while avoiding Node.js-specific APIs in the ArkTS driver path.

## Tasks

### Task 1: Add ArkTS config schemas
Outcome IDs: outcome-2
Outcome Role: supporting_evidence
Decommission IDs: none
Change Type: add
Description: Add the ArkTS-native schema and constant surface for build configuration, dependency modules, module metadata, compiler config records, and process compile task records. This is the component-local contract that later config generation and orchestration code depend on. The work mirrors the TypeScript type and pre-define concepts without importing or modifying the TypeScript driver.
Existing Behavior / Reuse: The TypeScript reference already defines enums, constants, and interfaces in `types.ts` and `pre_define.ts`; no corresponding ArkTS `types.ets` or `pre_define.ets` implementation currently exists in the ArkTS source tree.
Detailed Design: Add ArkTS enum definitions for build mode, build type, OHOS module type, es2panda mode, language version, record type, worker message type, and log level using string or numeric values matching the TypeScript reference contract. Add ArkTS classes or exportable typed records for `BuildConfig`, `DependencyModuleConfig`, `ModuleInfo`, `ProcessCompileTask`, `ArkTSConfigObject`, `CompilerOptions`, `DependencyItem`, `AliasConfig`, and related path/module config records; native pointer and integer fields use `EtsNativePointer`, `EtsInt`, and equivalent binding types from the ArkTS binding interop type surface. Keep collection fields explicit as ArkTS arrays, maps, or sets and define conversion expectations for raw JSON objects that later initialization code normalizes. Add constants for `.abc`, `.d.ets`, `.ets`, SDK path fragments, SDK alias sections, dynamic prefixes, and build cache names, excluding Node.js environment expressions from constant initialization.
Acceptance Criteria: Trigger: an operator compiles and runs a standalone ArkTS test driver that imports `BuildConfig` and `DependencyModuleConfig`, constructs a dependency module record, embeds it in a sample build config, and prints `packageName`. Target surface: the ArkTS config schema and constants module. Expected outcome: the test ABC compiles, runs, and prints the expected package name with no schema construction failure. Evidence: an es2panda compile/run command for the test driver exits 0 and stdout contains the expected package name.
Workload: 1.2 MM

### Task 2: Add graph and model utilities
Outcome IDs: outcome-4
Outcome Role: supporting_evidence
Decommission IDs: none
Change Type: add
Description: Add ArkTS graph, driver error, and pure path/string utility model surfaces used by the config model. The graph owns dependency ordering invariants for resolved module metadata, while file-system and environment-backed helpers remain delegated to the interop runtime. This task gives the orchestrator a deterministic acyclic ordering primitive without introducing process or worker behavior.
Existing Behavior / Reuse: The TypeScript reference provides `Graph`, `GraphNode`, `DriverError`, error codes, hashing, extension changes, and path normalization helpers; no ArkTS equivalents currently exist in the ArkTS source tree.
Detailed Design: Add `GraphNode<T>` with stable `id`, `data`, predecessor ids, and descendant ids, and add `Graph<T>` with node lookup, adjacency storage, add/remove/filter/find operations, verification, merge, and topological sort. Implement topological sort with deterministic Kahn-style traversal: initialize in-degree from predecessor sets, enqueue zero-predecessor nodes in insertion order, emit each node once, decrement descendants, and throw `DriverError` when emitted count differs from graph size. Add `DriverError`, `ErrorCode`, and log-data-compatible error payload records for config and graph failures such as corrupted graph, cyclic dependencies, invalid config, missing module path, and unsupported feature. Add pure utilities for file suffix replacement, Unix-style path normalization, path joining/resolution over strings, stable hashing, subpath checks, and environment placeholder traversal hooks; native file existence and environment lookup are represented as injected helper calls rather than direct Node.js APIs.
Acceptance Criteria: Trigger: an operator compiles and runs a standalone ArkTS graph test that creates two module nodes where `harA` depends on `harB`, builds a graph, and calls topological sort. Target surface: the ArkTS graph and error utility modules. Expected outcome: runtime output or assertion shows `harB` before `harA`, and a separate cyclic fixture raises a deterministic `DriverError`. Evidence: the ArkTS test ABC compile/run command exits 0 for the valid graph and the negative test observes the expected error class.
Workload: 1.0 MM

### Task 3: Add ArkTSConfigGenerator model
Outcome IDs: outcome-5
Outcome Role: supporting_evidence
Decommission IDs: none
Change Type: add
Description: Add the ArkTS compiler config generator as an in-memory producer of compiler options, files, paths, SDK roots, dependencies, and module metadata. This entity is the config-model bridge between resolved build configuration and native compiler config creation. It must match the TypeScript generator structure for the demo project while staying independent from disk temp config generation.
Existing Behavior / Reuse: The TypeScript reference implements `ArkTSConfig` and `ArkTSConfigGenerator` with path mapping, dependency merging, SDK path initialization, and module-specific config creation; no ArkTS generator implementation currently exists.
Detailed Design: Add `ArkTSConfig` as a mutable in-memory object wrapper with `compilerOptions`, `packageName`, `dependencies`, and path-section accessors plus methods to add path mappings, add dependencies, merge another config, and recursively merge configs by dependency sets. Add `ArkTSConfigGenerator` that accepts a resolved `BuildConfig`, computes system SDK and stdlib locations from normalized config paths, initializes SDK path mappings and dependency sections, and creates one config per module. Preserve invariants that each module config has a package name, base URL, root directory, cache directory, file list, dependency records, and path mappings; dependency merging must be idempotent and must not duplicate aliases or path entries. Expose methods to get config by package name, generate config for a module, serialize config to JSON text for the native config request, and compare generated `compilerOptions`, `files`, and `paths` with a reference snapshot in tests.
Acceptance Criteria: Trigger: an operator compiles and runs a standalone ArkTS test that loads the demo build config through resolved model data, constructs `ArkTSConfigGenerator`, and generates the entry and dependency module configs. Target surface: the ArkTS config generator API. Expected outcome: generated `compilerOptions`, `files`, and `paths` match the TypeScript-driver golden snapshot for the same demo input. Evidence: the runtime/compiler test exits 0 after structural assertions against the golden snapshot.
Workload: 1.8 MM

### Task 4: Add build config resolution model
Outcome IDs: outcome-6
Outcome Role: supporting_evidence
Decommission IDs: none
Change Type: add
Description: Add ArkTS build-config initialization transformations that convert raw demo configuration into resolved config-model records. This work owns schema normalization, absolute module and SDK paths, alias metadata, obfuscation boundary fields, and first-iteration compatibility rejection. Native library lookup and file existence checks are consumed through interop-runtime helpers rather than owned by this component.
Existing Behavior / Reuse: The TypeScript reference resolves SDK paths, tool paths, alias maps, interop SDK paths, cache config, and obfuscation config using Node.js `fs`, `path`, and `process`; no ArkTS `initBuildConfig` model currently exists.
Detailed Design: Add `initBuildConfig` that accepts a parsed raw `BuildConfig`, returns a normalized `BuildConfig`, and never mutates caller-owned raw JSON objects in place. Normalize dependency module entries, object-backed maps, optional arrays, module paths, project root, cache path, build SDK path, panda SDK path, SDK alias maps, interop API paths, and language-version fields using the pure path utilities and injected interop checks. Replace binary path resolution with a native-library-path field supplied by the interop runtime and record deterministic errors for unsupported build modes, unsupported obfuscation or DeclgenV1 requirements, missing SDK stubs, invalid alias records, unresolved dependency module paths, and malformed raw config. Preserve invariants that all accepted dependency modules have absolute module paths, the build SDK path is absolute, all required SDK stub paths are checkable, and unsupported first-iteration features are rejected before the orchestrator compiles modules.
Acceptance Criteria: Trigger: an operator compiles and runs a standalone ArkTS test that parses `demo_hap` build config JSON and calls `initBuildConfig` with interop-backed file/path helpers. Target surface: the ArkTS build-config initialization API. Expected outcome: returned config contains absolute `buildSdkPath`, absolute dependency `modulePath` values for `harA` and `harB`, normalized alias/interop metadata, and deterministic rejection for an unsupported-mode fixture. Evidence: the compiled test ABC exits 0 after asserting resolved paths against the TypeScript-driver reference output and observing the expected error code for the negative fixture.
Workload: 1.4 MM

## Cross-Cutting Constraints
- TypeScript driver parity — generated schemas, paths, and compiler config structures must remain comparable to the existing TypeScript reference while the reference remains intact
- ArkTS-only runtime model — config-model code must not rely on Node.js `fs`, `path`, `os`, `process`, dynamic imports, or subprocess behavior because those are outside the ArkTS driver constraints
- Sequential-first metadata — dependency and config records only need to support accepted sequential `demo_hap` compilation for the first iteration
- Deterministic rejection — invalid config, unsupported modes, missing module paths, and missing SDK stubs must produce stable driver errors before native compilation

## Data And Control Flow
- Raw build config is parsed by the caller and passed into `initBuildConfig` — config model owns normalization and resolved records, interop runtime owns file/native checks
- Resolved `BuildConfig` feeds `ArkTSConfigGenerator` — config model owns compiler options, files, paths, dependencies, and JSON serialization for native config creation
- Dependency module records feed `Graph` — config model owns acyclic ordering primitives, while sequential compile dispatch remains orchestrator-owned
- Golden parity tests compare config-model outputs to the TypeScript reference — test harness owns execution, config model owns deterministic product data

## Component Interactions
- `arkts-driver-config-model` -> `arkts-driver-sequential-orchestrator` — provides resolved `BuildConfig`, dependency module records, graph ordering results, and generated per-module config objects for sequential compile dispatch
- `arkts-driver-config-model` -> `arkts-driver-interop-runtime` — passes native compiler config text and consumes injected file, environment, and native-library helper results without owning binding calls
- `legacy-typescript-driver` -> `arkts-driver-config-model` — remains the reference for schema, generator, and path-resolution parity; it is not imported by the ArkTS driver runtime
- `arkts-driver-test-harness` -> `arkts-driver-config-model` — compiles and runs standalone ArkTS tests for schemas, graph ordering, generator parity, and config resolution

## Rationale
The component is materially affected because four approved deltas require new ArkTS-native config-model product entities and no corresponding ArkTS implementation exists yet. Keeping this work inside the config model preserves the approved separation between data/config ownership, interop side effects, and sequential orchestration.

## Skip Rationale
Not skipped.

## Runner Evidence
- Final message: `logs/agents/component-design-v2-arkts_driver_config_model/attempt-1/final_message.txt`
