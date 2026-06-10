# Validation Scenarios: 011 BuildMode sequential dispatch

Offline deterministic validation: network disabled; no external devices; no provider credentials. These scenarios exercise the ArkTS build-system product path through compiled ArkTS entry/test modules and disassembled/runtime evidence where the local toolchain permits it.

## 011-arkts_driver_sequential_orchestrator-task-2-add-buildmode-sequential-dispatch-scenario-1

When the compiled ArkTS driver ABC receives `driver/build_system/test/demo_hap/build_config.json` from the entry surface, `entry.build()` initializes the config, `BuildMode.run()` admits the sequential `demo_hap` config, calls `BaseMode.run()`, and the runtime exits successfully with non-empty ABC artifacts for `harB.abc`, `harA.abc`, and `entry.abc` on disk.

Materialized by `run.sh` as:
- compiling the canonical production entrypoint `driver/build_system/ets_src/entry.ets` with the build-system `arktsconfig.json`;
- compiling a product-route smoke wrapper that imports `entry.build()` and passes the real `demo_hap/build_config.json` path;
- running the smoke wrapper when the same-toolchain ArkTS console baseline succeeds;
- asserting expected demo artifacts are non-empty when runtime smoke is available.

If the local ArkTS runtime cannot pass the same-toolchain console baseline, this scenario records a runtime/toolchain environment blocker while still preserving compile/import evidence for the canonical entrypoint.

## 011-arkts_driver_sequential_orchestrator-task-2-add-buildmode-sequential-dispatch-scenario-2

When a runtime/compiler test supplies unsupported build mode, obfuscation, external-project/worker-style dispatch, or DeclgenV1 feature config to the same product surface, `BuildMode.run()` raises `DriverError` with `unsupported-build-mode`, `unsupported-feature-obfuscation`, or `unsupported-feature-declgen-v1` before invoking native compilation or creating outputs.

Materialized by `run.sh` as:
- compiling `driver/build_system/test/ets_ut/build_mode_dispatch_test.ets`, which imports production `BuildMode` and uses a `RecordingBaseMode` product dependency to verify `BaseMode.run()` is not invoked for rejected configs;
- requiring the compiled test ABC to contain the success marker and main entrypoint in disassembly;
- running the test when the same-toolchain runtime baseline succeeds;
- scanning validation output to ensure rejection tests do not create demo or rejection output artifacts.

## 011-arkts_driver_sequential_orchestrator-task-2-add-buildmode-sequential-dispatch-scenario-3

Executable evidence is a compiled ArkTS test or driver run for the successful demo path plus a runtime/compiler test that observes each expected rejection reason.

Materialized by `run.sh` as:
- compiler evidence for the canonical driver entrypoint;
- compiler/disassembly evidence for the BuildMode rejection runtime/compiler test;
- same-toolchain console baseline before attributing stdout/runtime failures to product code;
- optional runtime evidence for the rejection test and demo driver smoke whenever the local ArkTS runtime can execute the compiled ABCs;
- deterministic non-zero failure for missing production surfaces, failed compilation, absent rejection markers, absent entrypoints, or failed runtime tests after a successful runtime baseline.
