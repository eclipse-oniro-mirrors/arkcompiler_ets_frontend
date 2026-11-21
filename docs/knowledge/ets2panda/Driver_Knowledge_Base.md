# ETS2Panda Driver Knowledge Base

> Knowledge-base paths beginning with `docs/knowledge/` are relative to the repository root.


> Document Version: v1.0
> Last Updated: 2026-05-28
> Scope: `ets_frontend` frontend compiler knowledge base entry
> Before modifying, please read: `AGENTS.md`, the corresponding compiler route `AGENTS.md`, and the nearest directory-level `AGENTS.md`

## Overview

`build_system` is the core module of the `driver` layer. It translates the `BuildConfig` passed in by hvigor or the CLI into compilation tasks for ArkTS 1.2 source files, drives ets2panda to complete the full build pipeline from `.ets`/`.ts` sources to bytecode (`.abc`), and supports declgen declaration file generation, plugin transforms, and obfuscation config generation. `dependency_analyzer` is a C++ dependency analysis sub-module in the driver layer that analyzes import dependencies between ArkTS files and outputs a file list sorted by dependency order.

## Directory Structure and Code Map

- Upstream: hvigor (passes `BuildConfig` object via the `build()` API) / CLI (passes a JSON file via `node entry.js build_config.json`)
- Downstream: ets2panda compiler (invoked via koala-wrapper / libarkts API), ark_link linker, arkguard obfuscation tool

## Directory Explanation

- `build_system/src/` — Core TypeScript source code
  - `entry.ts` — Exposes the `build()` entry function; dispatches to `BuildMode` or `BuildFrameworkMode`
  - `types.ts` — Core type definitions (`BuildConfig`, `ES2PANDA_MODE`, `WorkerMessageType`, `BUILD_MODE`, `OHOS_MODULE_TYPE`, etc.)
  - `logger.ts` — Logger singleton (`Logger`); supports hvigor console logger integration
  - `pre_define.ts` — Predefined constants (paths, file names, SDK directory structure, etc.)
  - `build/` — Compilation mode implementations: `BaseMode` (core logic), `BuildMode` (mode dispatch), `BuildFrameworkMode` (framework mode), worker processes/threads (`compile_process_worker.ts`, `compile_thread_worker.ts`, `declgen_process_worker.ts`), `generate_arktsconfig.ts` (generates `arktsconfig.json`)
  - `init/` — Config initialization: `process_build_config.ts` (`initBuildConfig()`, path normalization, cache comparison), `init_koala_modules.ts` (loads koala-wrapper)
  - `util/` — Utilities: `error.ts` (`ErrorCode` enum and `DriverError`), `ets2panda.ts` (`Ets2panda` singleton, wraps parse/check/declgen/emit calls), `graph.ts` (`Graph`/`GraphNode`, DAG dependency graph), `TaskManager.ts` (multi-process/multi-thread task scheduling), `statsRecorder.ts` (performance statistics), `utils.ts` (general utilities), `worker_exit_handler.ts` (worker exit handling)
  - `plugins/` — Plugin system: `PluginDriver` (singleton, manages loading and execution hooks for AST transform plugins), `KitImportTransformer`
  - `obfuscation/` — Obfuscation config processing
- `build_system/test/` — Test directory: `ut/` (unit tests), `e2e/` (end-to-end tests), `e2e_obfuscation/` (obfuscation E2E tests), demo projects (`demo_hap/`, `demo_mix_hap/`, etc.)
- `docs/` — Driver config documentation: `ets2panda/driver/docs/build-config.en.md` (`BuildConfig` field descriptions), `ets2panda/driver/docs/target-build.en.md` (multi-target sourceRoots build documentation)
- `dependency_analyzer/` — C++ file dependency analyzer: `dep_analyzer.h`/`dep_analyzer.cpp` (core implementation, analyzes import dependencies, outputs file path list sorted by import priority), `main.cpp` (CLI entry point)
- `docs/` — Driver layer architecture documentation

## Key Files and Responsibilities

- `src/entry.ts` — Exposes the `build(BuildConfig)` entry point: handles backward-compatibility adaptation, dispatches to the appropriate mode based on `frameworkMode` and `enableDeclgenEts2Ts`
- `src/types.ts` — Defines all core types: `BuildConfig` (containing sub-structures `BuildBaseConfig`, `DeclgenConfig`, `ModuleConfig`, `PathConfig`, `FrameworkConfig`, etc.), `ES2PANDA_MODE` (`RUN_PARALLEL`/`RUN_CONCURRENT`/`RUN_SIMULTANEOUS`/`RUN`), `WorkerMessageType`, `OHOS_MODULE_TYPE` (`hap`/`har`/`feature`/`shared`/`entry`)
- `src/init/process_build_config.ts` — `initBuildConfig()`: fully normalizes the input config (SDK path derivation, dependency module Map reconstruction, config cache comparison and write, platform-specific config, environment initialization, alias config, interop SDK info, obfuscation config initialization)
- `src/build/base_mode.ts` — `BaseMode` abstract class: implements core build logic including collecting module info (`collectModuleInfos`), generating arktsconfigs (`generateArktsconfigs`), processing entry files, running parallel/concurrent/simultaneous/sequential modes, triggering declgen v1 and v2, running the linker (ark_link), and managing declFileMap
- `src/build/build_mode.ts` — `BuildMode`: dispatches to `runParallel`/`runConcurrent`/`runSimultaneous`/`run` based on `ES2PANDA_MODE`, and records statistics
- `src/build/generate_arktsconfig.ts` — `ArkTSConfigGenerator`: generates `arktsconfig.json` for each module, handling paths (including sourceRoots multi-target priority mapping) and dependencies (two-phase transformed/remaining parsing of interop dependencies)
- `src/util/ets2panda.ts` — `Ets2panda` singleton: wraps the full ets2panda compiler API call chain (parse → plugin(parse) → declgen → check → plugin(check) → emit)
- `src/util/error.ts` — `ErrorCode` enum (current codes in this file) and `DriverError` class, defining all errors reportable by the build system
- `src/util/graph.ts` — `Graph`/`GraphNode`: DAG directed graph for representing module dependency relationships and topological scheduling
- `src/util/TaskManager.ts` — `TaskManager`: multi-process/multi-thread task scheduling, manages worker pool and task queue
- `src/util/statsRecorder.ts` — `StatisticsRecorder`: event-level performance timing, outputs `bs_record_perf.csv` report
- `src/plugins/plugins_driver.ts` — `PluginDriver` singleton: manages loading of external AST transform plugins (`BUILDSYSTEM_LOAD_PLUGIN_FAIL`) and hook execution
- `dependency_analyzer/dep_analyzer.h` — `DepAnalyzer` class: analyzes `directDependencies_`, `directDependants_`, `outputMatching_`, and outputs the file path list

## Responsibility Boundaries

- Responsible for: `BuildConfig` parsing and normalization, module info collection, `arktsconfig.json` generation (including paths/dependencies/sourceRoots), multi-mode parallel/concurrent/simultaneous/sequential compilation scheduling, triggering declgen v1/v2 and managing `declFileMap`, bytecode linking (ark_link), obfuscation config generation (arkguard), plugin driving (PluginDriver), build cache management, performance statistics
- Not responsible for: ets2panda static semantics (type checker core), compiler IR and lowering logic, bytecode format itself, runtime behavior

## Core Data Flow / Control Flow

```
hvigor / CLI
    → build(BuildConfig)                    [entry.ts]
        → initBuildConfig()                 [process_build_config.ts]
            (path normalization, cache comparison, obfuscation config initialization)
        → BuildMode / BuildFrameworkMode    [build_mode.ts / build_framework_mode.ts]
            → collectModuleInfos()          [base_mode.ts]
            → generateArktsconfigs()        [generate_arktsconfig.ts]
                (paths, sourceRoots priority mapping, interop dependencies)
            → DependencyAnalyzer            [dependency_analyzer/dep_analyzer]
                (inter-file import dependency analysis, build compilation order graph)
            → TaskManager / workers         [TaskManager.ts, *_worker.ts]
                → Ets2panda.compile()       [ets2panda.ts → libarkts]
                    (parse → plugin → declgen → check → plugin → emit)
                → Ets2panda.declgen()       [ets2panda.ts → libarkts]
            → ark_link                      (link multi-module .abc files)
            → PluginDriver hooks            [plugins_driver.ts]
        → StatisticsRecorder.write()        [statsRecorder.ts]
```

## Knowledge Routing

- `BuildConfig` field meanings and required fields → `ets2panda/driver/docs/build-config.en.md`
- sourceRoots multi-target build and paths generation algorithm → `ets2panda/driver/docs/target-build.en.md`
- Inter-file import dependency analysis → `ets2panda/driver/dependency_analyzer/README.md`
- Compilation errors (type/semantic errors) → checker Knowledge Base
- declgen declaration output issues → `docs/knowledge/ets2panda/DeclGen_ETS2TS_Knowledge_Base.md`
- Plugin-related issues → `src/plugins/`
- ErrorCode quick reference → `src/util/error.ts`
- Performance analysis toggle and reports → `src/util/statsRecorder.ts` and README.md

## Expert Tips

- First verify whether input `BuildConfig` fields are incorrect before investigating compiler core issues; set `enableDebugOutput: true` to print the full config
- The `isBuildConfigModified` flag affects incremental cache hits; if behavior is unexpected after a config change, check whether `project_build_config.json` under `cachePath` has been correctly updated
- `ES2PANDA_MODE` choice directly impacts compilation performance: `RUN_PARALLEL` (multi-process, suitable for full builds of large projects), `RUN_CONCURRENT` (multi-thread + AST cache, suitable for HAR incremental compilation), `RUN_SIMULTANEOUS` (special simultaneous mode, used for BuildFrameworkMode), `RUN` (sequential, suitable for debugging)
- During local development, environment variables `USE_KOALA_LIBARKTS`, `USE_KOALA_UI_PLUGIN`, `USE_KOALA_MEMO_PLUGIN` control the source of koala plugins; before testing, correctly configure `koalaWrapperPath` in `initKoalaWrapper`
- declgen v1 is triggered via `enableDeclgenEts2Ts: true` + `buildMode.generateDeclarationV1Parallel()`; v2 is triggered via the `declgenV2OutPath` config

## Anti-Patterns

- Adding semantic special-casing inside the driver (type or semantic errors should be fixed at the checker layer, not worked around in the driver)
- Bypassing `initBuildConfig()` and using raw `projectConfig` directly (paths are not normalized, cache comparison logic breaks)
- Modifying compilation parameters to work around obfuscation config errors instead of fixing the obfuscation config file
- Calling non-thread-safe singletons from workers without protection

## Debugging and Verification

Run npm commands from `ets2panda/driver/build_system/` (repository-relative). Dependency-analyzer targets run from the configured static frontend build directory.

- Unit tests: `npm run ut_test` (`test/ut/`, Jest-based)
- E2E tests: `TEST=${test_script_name} npm run build_system_Etest` (`test/e2e/`)
- Full local test steps: see `README.md` (mock SDK → mock koala-wrapper → `npm run build` → `npm run demo_hap:gen_abc`)
- Dependency graph visualization: set `dumpDependencyGraph: true`, then render the `.dot` files generated in `cachePath` with Graphviz
- dependency_analyzer tests: `es2panda_depanalyz_tests`

## Debugging Methods

- Enable `enableDebugOutput: true` to print the full `BuildConfig` and per-phase event logs
- Inspect `project_build_config.json` under `cachePath` to confirm whether the config has been correctly persisted
- Use the `ErrorCode` carried by `DriverError` (see `src/util/error.ts`) to quickly pinpoint the failing phase
- Performance analysis: change `recordType` to `ON_TYPE`, or set `dumpPerf: true` in the build config, to output `bs_record_perf.csv` and the `--dump-perf-metrics` flag
- `dumpDependencyGraph: true` generates `.dot` files for visualizing module dependency relationships

## Common Issues

- `BUILDSYSTEM_SDK_NOT_EXIST_FAIL`: `pandaSdkPath`/`buildSdkPath` misconfigured or SDK not installed
- `BUILDSYSTEM_LOAD_PLUGIN_FAIL`: koala-wrapper path is wrong or koala plugin is not properly configured
- `BUILDSYSTEM_DEPENDENT_MODULE_INFO_NOT_CORRECT_FAIL`: `packageName`/`moduleType`/`sourceRoots` fields missing or inconsistent in the dependency module's `DependencyModuleConfig`
- `BUILDSYSTEM_SOURCEROOTS_NOT_SET_FAIL`: `sourceRoots` field is not set or is empty
- `BUILDSYSTEM_DEPENDENCY_ANALYZE_FAIL`: circular dependency detected or imported file not found during dependency analysis
- `BUILDSYSTEM_DECLGEN_FAIL` / `BUILDSYSTEM_DECLGEN_FAILED_IN_WORKER`: declgen phase failed; check whether the input AST has reached the CHECKED state
- Incremental cache not invalidated after config change: check `isBuildConfigModified` and the contents of `project_build_config.json`

## Related Documents

- `AGENTS.md` -- repository-level routing and constraints
- `ets2panda/AGENTS.md` -- ets2panda-wide frontend rules
- `ets2panda/driver/docs/build-config.en.md` -- full BuildConfig field descriptions
- `ets2panda/driver/docs/target-build.en.md` -- sourceRoots multi-target build and paths generation algorithm
- `ets2panda/driver/build_system/README.md` -- local run and test steps
- `docs/knowledge/ets2panda/DeclGen_ETS2TS_Knowledge_Base.md` -- declaration-generation downstream behavior
