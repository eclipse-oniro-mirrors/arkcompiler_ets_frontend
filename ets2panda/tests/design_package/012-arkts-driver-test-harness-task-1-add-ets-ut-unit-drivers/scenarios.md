# Validation Scenarios: 012 ets_ut unit drivers

Offline deterministic validation: network disabled; no external devices; no provider credentials. These scenarios exercise the ArkTS build-system test harness product path by compiling and running the `driver/build_system/test/ets_ut/` unit drivers with the local ArkTS compiler/runtime when the same-toolchain runtime baseline is usable.

## 012-arkts_driver_test_harness-task-1-add-ets_ut-unit-drivers-scenario-1

Trigger: an operator or script compiles and runs each `ets_ut` unit driver with the local ArkTS compiler/runtime.

Materialized by `run.sh` as:
- compiling each required standalone unit driver with `es2panda --ets-module --arktsconfig driver/build_system/arktsconfig.json`;
- running each compiled ABC with `ark` after a same-toolchain console baseline succeeds;
- failing non-zero for any missing driver, compiler diagnostic, empty ABC, runtime assertion failure, or missing expected stdout marker.

The required unit drivers are `types_config_test.ets`, `logger_test.ets`, `graph_test.ets`, `generate_arktsconfig_test.ets`, `process_build_config_test.ets`, and `native_single_file_compile_test.ets`.

## 012-arkts_driver_test_harness-task-1-add-ets_ut-unit-drivers-scenario-2

Target surface: ArkTS test harness runtime drivers importing `types`, `logger`, `graph`, `generate_arktsconfig`, `process_build_config`, and `Ets2panda`.

Materialized by `run.sh` as product compile/runtime coverage for:
- `driver/build_system/ets_src/types.ets` through `types_config_test.ets`;
- `driver/build_system/ets_src/logger.ets` through `logger_test.ets`;
- `driver/build_system/ets_src/util/graph.ets` through `graph_test.ets`;
- `driver/build_system/ets_src/build/generate_arktsconfig.ets` through `generate_arktsconfig_test.ets`;
- `driver/build_system/ets_src/init/process_build_config.ets` through `process_build_config_test.ets`;
- `driver/build_system/ets_src/util/ets2panda.ets` through `native_single_file_compile_test.ets`.

The scenario rejects validation that only inspects source inventory; the command must compile the actual ArkTS test drivers and, when the local runtime baseline passes, execute them.

## 012-arkts_driver_test_harness-task-1-add-ets_ut-unit-drivers-scenario-3

Expected outcome: each driver exits 0; stdout includes the expected package name, `build started`, dependency-correct graph order, golden config comparison success, resolved demo paths, and fresh single-file ABC success; stale or pre-existing ABC output causes failure.

Materialized by `run.sh` as:
- checking stdout markers `entry`, `build started`, `harB,harA`, `golden arktsconfig comparison success`, `resolved demo paths`, and `fresh single-file ABC success` after runtime execution;
- pre-creating a stale `driver/build_system/test/ets_ut/native_single_file_out/harB/index.abc` in an isolated validation run and requiring `native_single_file_compile_test.ets` to reject it with a non-zero exit;
- failing if the stale-output rejection path exits 0 or fails without the expected `stale or pre-existing ABC output` evidence.

If the local ArkTS runtime cannot pass the same-toolchain console baseline, runtime/stdout assertions are reported as a toolchain environment blocker rather than product failures; compile/import/API coverage still runs and must pass.

## 012-arkts_driver_test_harness-task-1-add-ets_ut-unit-drivers-scenario-4

Executable evidence: a local compile/run command sequence for all unit `.ets` drivers reports exit code 0 and shows a current-run non-empty `harB/index.abc` artifact in the isolated test output.

Materialized by `run.sh` as:
- cleaning `driver/build_system/test/ets_ut/native_single_file_out` before the accepted native compile run;
- recording `started_at_epoch` before the accepted run;
- requiring `driver/build_system/test/ets_ut/native_single_file_out/harB/index.abc` to exist, be non-empty, and have an mtime greater than or equal to `started_at_epoch`;
- writing the artifact stat to the validation output directory.

## ets_ut_unit_drivers_compile_and_run

This scenario id covers the command-level product evidence that all required unit `.ets` drivers compile, and run when the same-toolchain runtime is available.

## ets_ut_golden_config_snapshots

This scenario id covers golden config comparison evidence from `generate_arktsconfig_test.ets` and resolved build-config snapshot evidence from `process_build_config_test.ets`.

## ets_ut_native_single_file_fresh_abc

This scenario id covers native `Ets2panda` single-file compile evidence that creates a current-run non-empty `harB/index.abc` artifact.

## ets_ut_rejects_stale_or_preexisting_abc

This scenario id covers the stale/pre-existing ABC rejection path before accepting native single-file output.

## ets_ut_no_legacy_subprocess_or_dependency_analyzer_runtime

This scenario id covers decommission evidence that the ArkTS test drivers import the ported ArkTS driver modules and do not use the legacy TypeScript subprocess `ets2panda.ts` or external `dependency_analyzer.ts` runtime path.
