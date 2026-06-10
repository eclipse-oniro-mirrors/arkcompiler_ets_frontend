# Validation Scenarios: 013 demo_hap smoke runner

Offline deterministic validation: network disabled; no external devices; no hidden credentials; no external providers. These scenarios exercise the product `driver/build_system/test/ets_ut/demo_hap_smoke_runner.sh` command against the local ArkTS compiler/runtime toolchain and fail non-zero when the current task implementation does not satisfy acceptance.

## 013-arkts_driver_test_harness-task-2-add-demo_hap-smoke-runner-scenario-1

Trigger: an operator or CI smoke command runs `demo_hap_smoke_runner`.

Materialized by `run.sh` as:
- requiring the product smoke runner at `driver/build_system/test/ets_ut/demo_hap_smoke_runner.sh`;
- invoking it with an isolated `ARTIFACT_DIR` and local tool paths;
- requiring exit code 0 and the stdout marker `demo_hap_smoke_runner: PASS`.

## 013-arkts_driver_test_harness-task-2-add-demo_hap-smoke-runner-scenario-2

Target surface: the compiled ArkTS driver ABC, `demo_hap` runtime route, generated production ABC artifacts, and entry ABC dump output.

Materialized by `run.sh` as product-path validation that:
- compiles `driver/build_system/ets_src/entry.ets` through the smoke runner into `build_system.abc`;
- compiles and runs the smoke entry that imports the production `build()` function;
- runs against `driver/build_system/test/demo_hap/build_config.json`;
- inspects `driver/build_system/test/demo_hap/dist/entry.abc` through `ark_disasm` or `DEMO_HAP_DUMP_TOOL`.

## 013-arkts_driver_test_harness-task-2-add-demo_hap-smoke-runner-scenario-3

Expected outcome: the driver exits 0; fresh `harB`, `harA`, and entry ABC artifacts are present and non-empty; dump or disassembly output shows `strA` and `strB` references consistent with the demo sources and TypeScript baseline; missing dump tooling fails the smoke with a deterministic error.

Materialized by `run.sh` as:
- requiring `demo_hap_smoke_runner.run.log` to contain `Accepted sequential ArkTS build dispatch.` and `demo-hap-smoke-runner-driver-ok`;
- requiring `demo_hap_artifacts.stat` to record non-zero `harB.abc`, `harA.abc`, and `entry.abc` sizes from the current run;
- requiring the entry dump evidence to contain both `strA` and `strB`;
- running a second negative product smoke with `ARK_DISASM` unset to an unavailable executable and `DEMO_HAP_DUMP_TOOL` empty, requiring a non-zero exit and `missing-dump-tool` evidence.

## 013-arkts_driver_test_harness-task-2-add-demo_hap-smoke-runner-scenario-4

Executable evidence: local compile/run/smoke command output showing driver success, fresh artifact-size assertions passing, and import inspection passing.

Materialized by `run.sh` as:
- storing the product smoke command, stdout/stderr, compile logs, run logs, ABC stat file, and dump output in the validation artifact directory;
- failing if any expected evidence file is missing or empty;
- failing if the ArkTS driver source tree references decommissioned process/thread worker or `TaskManager` surfaces.

## demo_hap_smoke_runner_success

This scenario id covers the successful command-level product run of `driver/build_system/test/ets_ut/demo_hap_smoke_runner.sh`.

## demo_hap_smoke_runner_fresh_abc_artifacts

This scenario id covers fresh non-empty `harB.abc`, `harA.abc`, and `entry.abc` product artifacts under `driver/build_system/test/demo_hap/dist/`.

## demo_hap_smoke_runner_import_inspection

This scenario id covers entry ABC dump/disassembly evidence for `strA` and `strB`.

## demo_hap_smoke_runner_missing_dump_tool_deterministic_failure

This scenario id covers deterministic `missing-dump-tool` failure when dump tooling is unavailable.

## demo_hap_smoke_runner_no_decommissioned_workers

This scenario id covers decommission evidence that ArkTS sources do not reference `compile_process_worker`, `compile_thread_worker`, or `TaskManager`.
