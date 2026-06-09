# Validation Scenarios: Add entry dispatch source

Scenario root: `003-arkts_driver_source_tree-task-3-add-entry-dispatch-source`

Validation mode: offline, deterministic, no network, no external provider, no device, and no mocks. The scenarios exercise the production ArkTS entry dispatch path through local compiler/runtime commands when the toolchain is available, and through direct product-source compile/API assertions when downstream orchestration remains scaffolded.

## 003-arkts_driver_source_tree-task-3-add-entry-dispatch-source-scenario-1

Operator trigger: run the compiled ArkTS driver ABC with the demo `build_config.json` path as its runtime argument.

Expected evidence:
- The harness compiles the canonical product entrypoint `driver/build_system/ets_src/entry.ets` with the production `driver/build_system/arktsconfig.json`.
- The generated `build_system.abc` exists and has non-zero size.
- When the local ArkTS runtime can execute the compiled driver entrypoint, the harness invokes it with `driver/build_system/test/demo_hap/build_config.json` as the runtime argument.
- Runtime stdout, stderr, exit status, and command lines are captured in a transient artifact directory.

## 003-arkts_driver_source_tree-task-3-add-entry-dispatch-source-scenario-2

Target surface: the `entry.ets` runtime route in the compiled driver ABC.

Expected evidence:
- `entry.ets` exports `build(buildConfigPath: string): boolean`.
- The entry route imports and calls `initBuildConfig` from `./init/process_build_config`.
- The entry route imports and constructs `BuildMode` from `./build/build_mode`.
- The entry route calls `BuildMode.run()` and returns success only after dispatch.
- The canonical product entrypoint/module is compiled, not a grep-only synthetic source inventory check.

## 003-arkts_driver_source_tree-task-3-add-entry-dispatch-source-scenario-3

Expected outcome: the entry route accepts the argument, dispatches to sequential `BuildMode`, logs failures through `Logger` if they occur, and exits 0 when downstream compilation produces non-empty demo ABC artifacts.

Expected evidence:
- Empty build-config path validation is exercised by a compiled validation driver that imports the production `build()` function and calls `build('')`.
- The validation driver includes every imported production source dependency in the compile package.
- The validation driver expects `build('')` to return `false` after logging a `DriverError` through `Logger`, not throw an unexpected exception.
- The harness inspects the production entry path for the specific `DriverError` catch and `Logger.getInstance().printError(error.logData)` route.
- The full demo smoke fails non-zero as a product failure if the driver returns success but the demo output directory lacks non-empty `.abc` artifacts.

## 003-arkts_driver_source_tree-task-3-add-entry-dispatch-source-scenario-4

Executable evidence: local runtime/compiler smoke command exits 0 and the demo output directory contains the expected `.abc` artifacts produced by the orchestrator path.

Expected evidence:
- The harness attempts the canonical compile command for `driver/build_system/ets_src/entry.ets`.
- The harness removes stale demo `.abc` files before a live runtime smoke when runtime execution is possible.
- The harness verifies expected non-empty demo artifacts: `harB.abc`, `harA.abc`, and at least one entry module `.abc` under the demo output tree.
- If the local toolchain/runtime is missing, the harness reports an environment blocker rather than fabricating pass evidence.
- If toolchain/runtime is present but the product entry dispatch or demo artifacts do not satisfy the scenario, the harness exits non-zero with a product failure.
