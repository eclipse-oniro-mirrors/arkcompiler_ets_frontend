# Design Package Component: arkts_driver_sequential_orchestrator

This file is copied from the approved Triborg design package during implementator preflight.

# Component Design: ArkTS Driver Sequential Orchestrator

## Summary
The ArkTS Driver Sequential Orchestrator owns the first-iteration build lifecycle for the ported driver: sequential mode admission, dependency-ordered module compilation, interop runtime dispatch, and ABC output validation. Existing ArkTS orchestrator implementation for this component is absent, while the TypeScript `BaseMode` and `BuildMode` concepts remain behavioral references only.

## Top-Level Alignment
This component implements the approved sequential-only `demo_hap` runtime path. It consumes resolved configuration and generated compiler-config data from the config model, invokes the interop runtime for native compilation, and produces non-empty ABC artifacts without worker, process-pool, thread-pool, or TypeScript-driver fallback behavior.

## Tasks

### Task 1: Change BaseMode sequential build
Outcome IDs: outcome-8
Outcome Role: supporting_evidence
Decommission IDs: decommission-2, decommission-3, decommission-4
Change Type: change
Description: Add the ArkTS `BaseMode` orchestration entity that executes the approved first-iteration sequential module build. `BaseMode` takes a resolved `BuildConfig`, consumes module metadata and generated compiler config data from the config model, walks dependencies in topological order, sends compile requests to the interop runtime, and validates ABC artifacts. This replaces worker-style orchestration inside the ArkTS driver while leaving legacy TypeScript worker files intact as reference-only code.
Existing Behavior / Reuse: The repository already has TypeScript `BaseMode`, compile process worker, compile thread worker, and task-manager concepts that define reference scheduling behavior. No existing ArkTS `ets_src` build implementation for this component is available, so the ArkTS `BaseMode` work is new and should reuse only reference semantics, not Node.js worker, thread, task-manager, or subprocess machinery.
Detailed Design: Add an ArkTS `BaseMode` class with a `run()` method that accepts a resolved `BuildConfig` and component collaborators for graph ordering, compiler-config generation access, logger/error reporting, and the interop `Ets2panda` adapter. Add component-local data shapes such as `SequentialCompileUnit { moduleName: string; modulePath: string; sourceFiles: string[]; outputKind: string; outputPaths: string[]; compilerConfigText: string; dependencyNames: string[] }` and `CompileRequest { unitName: string; sourceFiles: string[]; outputPaths: string[]; compilerConfigText: string; moduleOutputPath: string }`. `collectCompileUnits()` builds dependency module units for `harB` and `harA` from resolved dependency metadata and an entry unit from configured entry sources; `validateGraph()` rejects cycles and unresolved dependency names before any native compiler config, context, or output is created.

The accepted `demo_hap` ordering invariant is concrete: `harB/index.ets`, then `harA/index.ets`, then entry sources in configured dependency-correct order `d.ets`, `c.ets`, `b.ets`, `a.ets`. Dependency modules use module-level compile units: `harB/index.ets` maps to configured dist output `harB.abc`, and `harA/index.ets` maps to configured dist output `harA.abc`. Entry uses a source-list compile unit whose source files map to the entry module dist outputs; if the interop adapter supports module-level generation, `BaseMode` passes the full entry source list and module output path, otherwise it passes per-source output paths and then treats module-output/merge responsibility as owned by the interop runtime contract rather than by workers or external tools.

Add helper methods `collectCompileUnits`, `validateGraph`, `compileModuleUnit`, `compileEntrySources`, `makeCompileRequest`, and `validateAbcOutputs`. `makeCompileRequest()` is the handoff boundary: it receives resolved module paths, source lists, output roots, and generated ArkTS compiler-config text from the config model and produces an interop request containing only source paths, output paths, module output path, and config text. `compileModuleUnit()` and `compileEntrySources()` call the interop `Ets2panda` adapter exactly once per accepted unit or source batch; after each call, `validateAbcOutputs()` checks every expected ABC path exists and has non-zero size, raising deterministic output errors such as `missing-output-abc` or `empty-output-abc`.

For decommission enforcement, the ArkTS `BaseMode` must not instantiate or import process workers, thread workers, or a task-manager pool. `compile_process_worker.ts`, `compile_thread_worker.ts`, and `TaskManager.ts` remain internal to the legacy TypeScript reference path; the ArkTS runtime replaces them with direct in-process interop calls and enforces the negative invariant by having no worker abstraction in `BaseMode` compile dispatch.
Acceptance Criteria: When an operator runs the ported ArkTS driver with the resolved `demo_hap` build config, the `BaseMode.run()` product surface validates the graph before native compilation, rejects cycles or unresolved modules before outputs are created, compiles `harB/index.ets` before `harA/index.ets` before entry `d.ets`, `c.ets`, `b.ets`, `a.ets`, and stops on the first deterministic config, graph, compiler, or output error. The visible outcome is non-empty ABC artifacts for `harB.abc`, `harA.abc`, and the entry module/source outputs in the configured dist output. Executable evidence is a local runtime/compiler test or driver run that exits 0 and then stats the expected ABC artifacts as present and greater than zero bytes, plus a graph-failure runtime test that observes rejection before native compiler contexts are created.
Workload: 2.0 MM

### Task 2: Add BuildMode sequential dispatch
Outcome IDs: outcome-9
Outcome Role: supporting_evidence
Decommission IDs: none
Change Type: add
Description: Add the ArkTS `BuildMode` dispatch entity that admits only the supported sequential build mode and invokes `BaseMode.run()`. `BuildMode` is the component-local boundary between the entry surface and the orchestration lifecycle. It converts unsupported build modes and first-iteration unsupported features into deterministic `DriverError` failures before native compilation starts.
Existing Behavior / Reuse: The TypeScript `BuildMode` reference exists and selects build behavior for the Node.js driver. No ArkTS `BuildMode` implementation exists in the ArkTS source surface, so this task adds the ArkTS dispatcher while reusing the reference concept of a build-mode facade.
Detailed Design: Add an ArkTS `BuildMode` class with a constructor that receives the resolved `BuildConfig`, logger/error collaborators, and a factory or direct instance for `BaseMode`. Add a `run()` method that first calls `assertSequentialMode()` and `assertFirstIterationFeatures()`, then calls `BaseMode.run()` exactly once for an admitted sequential config. The admission invariant is that no compiler config, compiler context, source output, module output, or merge/module-output request is created until all mode and feature checks have passed.

`assertSequentialMode()` raises `DriverError` with rejection reason `unsupported-build-mode` for any non-sequential, simultaneous, parallel, process-worker, thread-worker, or external-project execution mode. `assertFirstIterationFeatures()` raises deterministic rejection reasons from the architecture contract: `unsupported-feature-obfuscation` for enabled obfuscation, `unsupported-feature-declgen-v1` for DeclgenV1/declaration-generation flow, and `unsupported-build-mode` for unsupported external-project or worker-style dispatch when no more specific architecture reason exists. Missing or invalid config remains owned by config initialization, but `BuildMode` preserves deterministic pass-through behavior if such a `DriverError` reaches it.

`BuildMode.run()` logs accepted sequential dispatch through the logger surface, delegates all dependency ordering and compile request creation to `BaseMode`, and returns success only after `BaseMode.run()` completes. Unsupported mode tests should exercise the same runtime route as the entry-dispatched product path, not a separate mock-only admission helper.
Acceptance Criteria: When the compiled ArkTS driver ABC receives the `demo_hap` build-config path from the entry surface, `BuildMode.run()` admits the sequential config, calls `BaseMode.run()`, and the runtime exits successfully with expected ABC artifacts on disk. When a runtime/compiler test supplies unsupported build mode, obfuscation, external-project/worker-style dispatch, or DeclgenV1 feature config to the same product surface, `BuildMode.run()` raises `DriverError` with `unsupported-build-mode`, `unsupported-feature-obfuscation`, or `unsupported-feature-declgen-v1` before invoking native compilation or creating outputs. Executable evidence is a compiled ArkTS test or driver run for the successful demo path plus a runtime/compiler test that observes each expected rejection reason.
Workload: 0.6 MM

## Cross-Cutting Constraints
- Sequential-only orchestration — the component must compile dependency modules before dependents and must not introduce workers or parallel scheduling because the approved first iteration targets deterministic local `demo_hap` compilation.
- Typed failure boundaries — unsupported-mode, graph, compiler, and output failures must surface as deterministic driver errors so the entry/logger surface can report them consistently.
- Product artifacts as acceptance surface — successful orchestration is proven by non-empty ABC outputs rather than internal state files or workflow metadata.
- Legacy TypeScript preservation — the TypeScript driver remains the behavioral reference and fallback selected by the operator, not a runtime dependency of the ArkTS orchestrator.

## Data And Control Flow
- Entry surface passes a resolved `BuildConfig` into `BuildMode` — `BuildMode` owns admission and dispatch — unsupported modes and features fail before native compiler config/context state or ABC outputs are created.
- Config model provides resolved module paths, dependency metadata, entry source lists, output roots, and generated ArkTS compiler-config text — `BaseMode` consumes these as inputs and does not recompute raw config initialization — compile request creation starts only after graph validation succeeds.
- `BaseMode.collectCompileUnits()` converts resolved config data into `SequentialCompileUnit` records — `BaseMode.makeCompileRequest()` converts each unit into an interop `CompileRequest` containing source paths, output path mapping, module output path, and compiler config text — this request is the orchestrator-owned handoff boundary to the interop adapter.
- `BaseMode` sends compile requests to the interop runtime one at a time — interop owns native config/context/state calls and module-output or merge mechanics — `BaseMode` owns dependency order, dispatch sequencing, and post-call ABC existence/size validation.
- `BaseMode` returns success only after output validation — entry/logger surface owns final runtime reporting — missing or empty ABC artifacts become deterministic output errors.

## Component Interactions
- `arkts-driver-source-tree` -> `arkts-driver-sequential-orchestrator` — the entry source calls `BuildMode.run()` with the build-config path already routed through initialization.
- `arkts-driver-config-model` -> `arkts-driver-sequential-orchestrator` — resolved `BuildConfig`, module metadata, dependency lists, entry source lists, output roots, and generated compiler-config text are consumed as orchestration data, not recomputed by `BaseMode`.
- `arkts-driver-sequential-orchestrator` -> `arkts-driver-interop-runtime` — `BaseMode` dispatches each compile request to the native-binding adapter and relies on typed compiler/output failures plus ABC file creation semantics.
- `legacy-typescript-driver` -> `arkts-driver-sequential-orchestrator` — TypeScript `BaseMode` and `BuildMode` remain reference behavior only and are not imported by the ArkTS runtime path.

## Rationale
The approved architecture marks this component as detailed because it owns the sequential build lifecycle and mode admission path. The required ArkTS orchestration is concrete, component-local, and not already present in the ArkTS source surface, while the TypeScript implementation provides only reference behavior.

## Skip Rationale
Not skipped.

## Runner Evidence
- Final message: `logs/agents/component-design-v3-arkts_driver_sequential_orchestrator/attempt-1/final_message.txt`
