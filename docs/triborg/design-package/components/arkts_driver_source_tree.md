# Design Package Component: arkts_driver_source_tree

This file is copied from the approved Triborg design package during implementator preflight.

# Component Design: ArkTS Driver Source Tree

## Summary
This component adds the ArkTS build-system source scaffold, logger surface, and runtime entry placement required by the approved ArkTS driver architecture. The existing TypeScript driver remains the reference path and is not modified or imported by this component.

## Top-Level Alignment
The ArkTS driver source tree owns the compilable module boundary for the new in-process driver path. It provides the `ets_src` layout, build-system `arktsconfig`, logger, and entry source needed by config, interop, orchestrator, and test-harness components.

## Tasks

### Task 1: Add ArkTS source scaffold
Outcome IDs: outcome-1
Outcome Role: supporting_evidence
Decommission IDs: decommission-6
Change Type: add
Description: Add the parallel ArkTS build-system source tree and module configuration owned by this component. The scaffold gives the ArkTS driver a compilable source boundary that mirrors the legacy TypeScript build-system layout while keeping the TypeScript build path intact.
Existing Behavior / Reuse: The existing TypeScript source tree already contains the reference build-system modules, but no `ets_src` source tree or build-system ArkTS config currently exists for this component. Reuse the TypeScript directory concepts as layout anchors only; do not import TypeScript modules from ArkTS.
Detailed Design: Add the ArkTS source root with component-owned modules for `entry`, `logger`, `init`, `build`, `util`, `obfuscation`, and `plugins` placement so downstream component files have stable import destinations. Add the build-system ArkTS module configuration that makes `entry.ets` the driver source entry, includes all `ets_src` compilation units, and resolves `@arkts-bindings` as an external ArkTS package dependency. The invariant is that the ArkTS build target is independent from the TypeScript `tsc`/package build target: the new config compiles ArkTS driver sources with `es2panda`, while the legacy package build remains available for the TypeScript fallback. For `decommission-6`, replace the idea that the Node.js package build is the only driver build surface by adding this ArkTS build surface; enforce the negative invariant by keeping the ArkTS config rooted only in ArkTS sources and not referencing TypeScript build outputs.
Acceptance Criteria: Operator trigger: run `es2panda --ets-module --arktsconfig driver/build_system/arktsconfig.json --output driver/build_system/dist/build_system.abc driver/build_system/ets_src/entry.ets`. Target surface: the ArkTS build-system source tree and ArkTS module config. Expected outcome: all `.ets` sources in the scaffold parse, imports resolve including `@arkts-bindings`, the command exits 0, and `build_system.abc` is produced without diagnostics. Executable evidence: successful local compiler command against the production ArkTS driver sources.
Workload: 0.6 MM

### Task 2: Add Logger runtime surface
Outcome IDs: outcome-3
Outcome Role: supporting_evidence
Decommission IDs: none
Change Type: add
Description: Add the ArkTS logger surface owned by this component. The logger provides the common runtime diagnostics route used by the entry source and later orchestration errors without depending on Node.js process APIs.
Existing Behavior / Reuse: The TypeScript `Logger` and `LogDataFactory` concepts exist in the legacy driver and can be reused as the behavioral contract. No ArkTS logger implementation currently exists in this component.
Detailed Design: Add an ArkTS `Logger` singleton with `getInstance`, info, warning, error, and fatal-print methods matching the legacy public shape where ArkTS language constraints allow it. Add a `LogDataFactory` data surface for formatting message payloads consistently with the TypeScript reference, but emit through ArkTS `console.log`, `console.warn`, and `console.error`. Implement fatal handling by throwing an ArkTS exception after error output so the entry module can catch or propagate failures through normal runtime control flow. The logger invariant is that all component-local diagnostics use ArkTS console and exception semantics, never Node.js `process.stderr`, color control, or `process.exit`.
Acceptance Criteria: Operator trigger: compile and run a standalone ArkTS test driver that imports `Logger` and calls `Logger.getInstance().printInfo("build started")`. Target surface: the `Logger` runtime API. Expected outcome: the ABC executes and stdout contains `build started` exactly as the visible diagnostic. Executable evidence: `es2panda` compilation of the test driver succeeds and the ArkTS runtime execution exits 0 with the expected output.
Workload: 0.4 MM

### Task 3: Add entry dispatch source
Outcome IDs: outcome-9
Outcome Role: supporting_evidence
Decommission IDs: none
Change Type: add
Description: Add the ArkTS entry source placement owned by this component. The entry module is the runtime route that accepts a demo build-config argument, invokes config initialization, dispatches to sequential build mode, and reports driver failures through the logger.
Existing Behavior / Reuse: The TypeScript `entry.ts` and `build_mode.ts` define the reference startup and build dispatch concepts. No ArkTS entry module currently exists, and the sequential orchestration implementation remains owned by `arkts-driver-sequential-orchestrator`.
Detailed Design: Add an ArkTS `build` entry function that obtains the runtime build-config path through the supported ArkTS runtime argument surface and validates that one config path was supplied. The function calls the config-model-owned `initBuildConfig` API, constructs or invokes the orchestrator-owned `BuildMode`, and routes successful completion to normal runtime exit. Catch `DriverError` from utility/config/orchestrator code and print it through `Logger`; unexpected exceptions are also logged before rethrowing or failing the runtime. The entry invariant is that this component owns only startup, imports, argument handoff, and logging; dependency ordering, native compilation, output validation, and unsupported-mode policy remain delegated to their owning components.
Acceptance Criteria: Operator trigger: run the compiled ArkTS driver ABC with the demo `build_config.json` path as its runtime argument. Target surface: the `entry.ets` runtime route in the compiled driver ABC. Expected outcome: the entry route accepts the argument, dispatches to sequential `BuildMode`, logs failures through `Logger` if they occur, and exits 0 when downstream compilation produces non-empty demo ABC artifacts. Executable evidence: local runtime/compiler smoke command exits 0 and the demo output directory contains the expected `.abc` artifacts produced by the orchestrator path.
Workload: 0.5 MM

## Cross-Cutting Constraints
- Keep the TypeScript driver intact — this component adds a parallel ArkTS build surface while preserving the reference/fallback implementation.
- Use ArkTS and binding-compatible APIs only — component-local sources must not introduce Node.js `child_process`, `fs`, `path`, `os`, `process.exit`, or stderr-color dependencies.
- Maintain clear ownership boundaries — entry and logger live here, while config processing, native compiler calls, sequential orchestration, and tests remain owned by their configured components.

## Data And Control Flow
- Operator compiles the ArkTS build-system module — `es2panda` reads the ArkTS config and source scaffold — source-tree ownership ends at a produced driver ABC.
- Operator runs the compiled driver ABC with a build-config path — `entry.ets` receives the argument and calls config initialization — startup state is not persisted by this component.
- Runtime diagnostics flow through `Logger` — entry and later components call logger methods — fatal conditions become ArkTS exceptions instead of process termination APIs.

## Component Interactions
- `arkts-driver-source-tree` -> `arkts-driver-config-model` — entry imports the config initialization surface and passes the runtime build-config path without owning config resolution.
- `arkts-driver-source-tree` -> `arkts-driver-sequential-orchestrator` — entry dispatches to `BuildMode.run()` after config initialization and does not implement module scheduling itself.
- `arkts-driver-source-tree` -> `arkts-driver-test-harness` — logger and entry are exercised by standalone ArkTS tests and smoke commands owned by the test harness.
- `legacy-typescript-driver` -> `arkts-driver-source-tree` — TypeScript source layout and logger/entry concepts are used as behavioral references only, not as ArkTS runtime imports.

## Rationale
This component is affected because the approved architecture requires a new ArkTS source boundary, logger, and runtime entry route before config, interop, orchestration, or test components can be integrated. The planned tasks are limited to the three component-impact deltas owned by `arkts_driver_source_tree`.

## Skip Rationale
Not skipped.

## Runner Evidence
- Final message: `logs/agents/component-design-v2-arkts_driver_source_tree/attempt-1/final_message.txt`
