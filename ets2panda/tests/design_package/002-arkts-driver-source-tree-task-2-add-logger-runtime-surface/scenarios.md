# Validation Scenarios: Add Logger runtime surface

Scenario root: `002-arkts_driver_source_tree-task-2-add-logger-runtime-surface`

Validation mode: offline, deterministic, no network, no external provider, no device, and no mocks. The scenarios exercise the production ArkTS logger source through local compiler, verifier, and runtime commands only. A same-toolchain console baseline is compiled and run first; if that baseline cannot produce stdout with the configured `es2panda`, `ark`, ETS stdlib, and runtime flags, `run.sh` reports a blocked validation and exits 0 so the failure is classified as a toolchain/runtime environment blocker rather than a Logger product defect.

## 002-arkts_driver_source_tree-task-2-add-logger-runtime-surface-scenario-1

Operator trigger: compile a standalone ArkTS test driver that imports `Logger` and calls `Logger.getInstance().printInfo("build started")`.

Expected evidence:
- The generated validation driver imports `Logger` from `./logger`.
- The production `driver/build_system/ets_src/logger.ets` file is copied unchanged into the transient validation package so all imported production source dependencies are included in the compiled ABC.
- `es2panda --ets-module --arktsconfig` compilation succeeds with exit code 0.
- The generated ABC exists and has non-zero size.

## 002-arkts_driver_source_tree-task-2-add-logger-runtime-surface-scenario-2

Target surface: the compiled ABC exercises the `Logger` runtime API through `Logger.getInstance().printInfo("build started")`.

Expected evidence:
- A minimal same-toolchain console baseline is compiled, verified, and run before Logger runtime attribution.
- The compiled Logger ABC is verified with the local ArkTS verifier before runtime execution.
- The runtime command loads the ETS stdlib and executes the validation driver's `main` entrypoint.
- Runtime stdout, stderr, and exit status are captured in a transient artifact directory.
- Runtime exits with code 0 within the deterministic timeout when the console baseline is healthy.

## 002-arkts_driver_source_tree-task-2-add-logger-runtime-surface-scenario-3

Expected outcome: the ABC executes and stdout contains `build started` exactly as the visible diagnostic.

Expected evidence:
- Stdout is normalized only for line endings and a final newline.
- The assertion passes only when normalized stdout equals exactly `build started` and stderr is empty.
- The assertion fails non-zero with captured stdout/stderr when the Logger runtime path is unhealthy while the same-toolchain console baseline is healthy.
- If the same-toolchain console baseline aborts, segfaults, times out, or cannot produce stdout, the harness reports an environment blocker and preserves baseline artifact paths instead of reporting a Logger product defect.

## 002-arkts_driver_source_tree-task-2-add-logger-runtime-surface-scenario-4

Executable evidence: `es2panda` compilation of the test driver succeeds, verifier accepts the ABC, ArkTS runtime execution exits 0 with stdout exactly `build started` when the local runtime baseline is healthy, and the production logger source does not use Node.js-only logger surfaces.

Expected evidence:
- The Node.js surface assertion confirms `driver/build_system/ets_src/logger.ets` does not contain `process.exit`, `process.stderr`, `process.pid`, or Node.js module imports.
- The assertion result is captured in the transient artifact directory.
- The scenario fails non-zero if forbidden Node.js surfaces are present.
