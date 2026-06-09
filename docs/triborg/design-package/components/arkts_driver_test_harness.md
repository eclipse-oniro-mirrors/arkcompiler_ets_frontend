# Design Package Component: arkts_driver_test_harness

This file is copied from the approved Triborg design package during implementator preflight.

# Component Design: ArkTS Driver Test Harness

## Summary
The ArkTS Driver Test Harness is affected as a validation-only component. It provides standalone ArkTS runtime drivers and smoke runners that prove the ported build-system surfaces compile, run, and produce fresh product-visible ABC artifacts.

## Top-Level Alignment
This component owns `driver/build_system/test/ets_ut/` validation for the approved ArkTS build-system migration. It validates config schemas, logger behavior, graph ordering, config generation, build-config resolution, native single-file compilation, sequential orchestration, and cross-module import visibility without owning product driver implementation.

## Tasks

### Task 1: Add ets_ut unit drivers
Outcome IDs: outcome-2, outcome-3, outcome-4, outcome-5, outcome-6, outcome-7, outcome-11
Outcome Role: supporting_evidence
Decommission IDs: decommission-1, decommission-5
Change Type: validate
Description: Add standalone ArkTS unit test drivers under the component-local `ets_ut` harness. These drivers validate the imported ArkTS build-system type, logger, graph, config-generation, config-resolution, and native single-file compile surfaces after product components provide them. The harness proves that the ArkTS path exercises native bindings instead of the legacy subprocess and dependency-analyzer paths.
Existing Behavior / Reuse: Existing build-system tests cover TypeScript and sample project fixtures, but there is no `ets_ut` ArkTS harness and no existing ArkTS unit driver set under this component. Reuse `demo_hap`, mock SDK fixtures, and TypeScript reference output concepts as inputs and durable baselines; keep the legacy TypeScript driver untouched as a reference path only.
Detailed Design: Add named unit drivers: `types_config_test.ets` for `BuildConfig` and `DependencyModuleConfig` construction, `logger_test.ets` for `Logger.getInstance().printInfo`, `graph_test.ets` for two-node topological sorting, `generate_arktsconfig_test.ets` for `ArkTSConfigGenerator`, `process_build_config_test.ets` for `initBuildConfig`, and `native_single_file_compile_test.ets` for `Ets2panda` native compilation. Add shared `assertions.ets` with `assertTrue`, `assertEquals`, `assertStringContains`, and `fail` helpers that throw on failure and print deterministic success markers; add harness artifacts `golden/demo_hap_arktsconfig.json` and `golden/demo_hap_resolved_build_config.json` as durable TypeScript-reference baselines. The golden baselines are generated or refreshed outside the ArkTS runtime from the legacy TypeScript reference for the same `demo_hap` input, then stored with normalized absolute-path placeholders, stable path separators, deterministic module ordering, and only the compared fields: `compilerOptions`, `files`, `paths`, `buildSdkPath`, and dependency `modulePath` entries; the ArkTS runtime tests must read and compare against these durable snapshots and must not call or import the TypeScript driver. The native single-file compile driver uses a fresh isolated output area created or cleaned by the harness runner before each run, asserts the target ABC is absent before compilation, invokes the ArkTS `Ets2panda` adapter for `harB/index.ets`, then fails if the resulting ABC is missing, empty, or older than the current run marker; this rejects stale artifacts. `decommission-1` and `decommission-5` are enforced by making accepted unit evidence come from `Ets2panda` native binding calls and local config/graph inputs only, while subprocess `es2panda` and external `dependency_analyzer` remain reference-path concepts outside the ArkTS test runtime.
Acceptance Criteria: Trigger: an operator or script compiles and runs each `ets_ut` unit driver with the local ArkTS compiler/runtime. Target surface: ArkTS test harness runtime drivers importing `types`, `logger`, `graph`, `generate_arktsconfig`, `process_build_config`, and `Ets2panda`. Expected outcome: each driver exits 0; stdout includes the expected package name, `build started`, dependency-correct graph order, golden config comparison success, resolved demo paths, and fresh single-file ABC success; stale or pre-existing ABC output causes failure. Executable evidence: a local compile/run command sequence for all unit `.ets` drivers reports exit code 0 and shows a current-run non-empty `harB/index.abc` artifact in the isolated test output.
Workload: 1.8 MM

### Task 2: Add demo_hap smoke runner
Outcome IDs: outcome-8, outcome-9, outcome-10, outcome-11
Outcome Role: supporting_evidence
Decommission IDs: decommission-2, decommission-3, decommission-4
Change Type: validate
Description: Add an ArkTS-compatible smoke runner surface for the sequential `demo_hap` driver path. The runner compiles the ported driver ABC, runs it with the demo build config, validates fresh product ABC outputs, and inspects entry-module imports. This task validates sequential orchestration behavior without owning the orchestrator implementation.
Existing Behavior / Reuse: Existing sample projects and TypeScript reference flows provide baseline data, but no ArkTS smoke harness currently validates the compiled ArkTS driver ABC end to end. Reuse `demo_hap` source ordering, HAR dependency relationships, TypeScript baseline expectations for `strA` and `strB`, and available dump/disassembly tooling as executable evidence inputs.
Detailed Design: Add a named smoke runner concept `demo_hap_smoke_runner` under the `ets_ut` harness command surface. Its ordered sequence is: compile the ArkTS driver ABC from `ets_src` using the build-system `arktsconfig`, clean or allocate a fresh demo output directory and record a run marker, run the compiled ArkTS driver ABC with `demo_hap/build_config.json`, validate fresh `harB.abc`, `harA.abc`, and entry ABC artifacts for existence, non-zero size, and current-run ownership, then inspect the entry ABC. The import inspection step uses `ark_disasm` when available or a configured approved equivalent dump command; if neither tool is available, the smoke fails deterministically with a missing-dump-tool error rather than passing silently. The inspection output must contain references or imports corresponding to `strA` and `strB`, matching the TypeScript-driver baseline expectation, but TypeScript-driver outputs are comparison baselines only and cannot satisfy ArkTS product artifact checks. `decommission-2`, `decommission-3`, and `decommission-4` are enforced by accepting evidence only from the compiled ArkTS driver sequential run, not from TypeScript process workers, thread workers, or `TaskManager` output.
Acceptance Criteria: Trigger: an operator or CI smoke command runs `demo_hap_smoke_runner`. Target surface: the compiled ArkTS driver ABC, `demo_hap` runtime route, generated production ABC artifacts, and entry ABC dump output. Expected outcome: the driver exits 0; fresh `harB`, `harA`, and entry ABC artifacts are present and non-empty; dump or disassembly output shows `strA` and `strB` references consistent with the demo sources and TypeScript baseline; missing dump tooling fails the smoke with a deterministic error. Executable evidence: local compile/run/smoke command output showing driver success, fresh artifact-size assertions passing, and import inspection passing.
Workload: 1.4 MM

## Cross-Cutting Constraints
- Standalone ArkTS tests must run without real external services — default validation is local compiler/runtime evidence and must not require Node.js APIs in the ArkTS path.
- Test evidence must remain product-visible — acceptance is based on runtime output, fresh ABC artifacts, and dump/disassembly results, not source review alone.
- Legacy TypeScript driver remains reference-only — harness may compare against durable baselines from it but must not make it part of the ArkTS runtime path.
- Fresh artifact validation is mandatory — missing, empty, stale, or TypeScript-produced ABC files cannot satisfy ArkTS driver evidence.

## Data And Control Flow
- Unit driver compile/run — operator or script -> `ets_ut` driver ABC -> ported ArkTS module API — each driver owns assertions for one behavior and exits non-zero on failure.
- Golden baseline comparison — legacy TypeScript reference output -> durable normalized golden artifact -> ArkTS runtime test comparison — the ArkTS test reads the baseline but never calls the TypeScript driver.
- Native single-file validation — `native_single_file_compile_test.ets` -> ArkTS native binding adapter -> isolated harB ABC artifact — fresh output existence, size, and current-run ownership are the accepted state signals.
- End-to-end smoke — `demo_hap_smoke_runner` -> compiled ArkTS driver ABC -> fresh `demo_hap` outputs -> dump/disasm inspection — sequential completion and fresh artifact validation must precede import inspection.
- Reference separation — TypeScript-driver outputs may define expected config fields, paths, symbol names, and size/structure comparisons, but accepted product artifact checks must use ArkTS-driver outputs from the current run.

## Component Interactions
- `arkts-driver-test-harness` -> `arkts-driver-config-model` — imports config schemas, graph utility, config generator, and config resolver for runtime validation.
- `arkts-driver-test-harness` -> `arkts-driver-source-tree` — imports logger and compiles the driver source tree for smoke execution.
- `arkts-driver-test-harness` -> `arkts-driver-interop-runtime` — invokes native single-file compile and validates fresh ABC artifact creation.
- `arkts-driver-test-harness` -> `arkts-driver-sequential-orchestrator` — runs the compiled driver against `demo_hap` and validates ordered module outputs.
- `legacy-typescript-driver` -> `arkts-driver-test-harness` — provides durable golden config and symbol baselines only, not an imported ArkTS runtime dependency.

## Rationale
The approved architecture assigns this component the validation surface for ArkTS unit drivers and end-to-end smoke evidence. The component does not implement compiler orchestration or native binding logic; it proves those surfaces through executable ArkTS tests, fresh artifact checks, and deterministic smoke inspection.

## Skip Rationale
Not skipped.

## Runner Evidence
- Final message: `logs/agents/component-design-v2-arkts_driver_test_harness/attempt-1/final_message.txt`
