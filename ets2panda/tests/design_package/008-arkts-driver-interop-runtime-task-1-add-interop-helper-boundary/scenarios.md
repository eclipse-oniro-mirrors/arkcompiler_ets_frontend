# Validation Scenarios: 008 ArkTS driver interop runtime task 1

Validation is offline, deterministic, mock-backed only where local runtime/toolchain availability is probed, and uses no network, external provider, live device, or hidden credentials.

## 008-arkts_driver_interop_runtime-task-1-add-interop-helper-boundary-scenario-1

Trigger: a compiled ArkTS config-resolution test loads the demo build config through the helper-backed `initBuildConfig` route.

Executable validation:
- Compile the canonical product entrypoint `driver/build_system/ets_src/entry.ets` with `driver/build_system/arktsconfig.json` so production imports and the helper boundary are included in the compilation unit.
- Compile `driver/build_system/test/ets_ut/process_build_config_test.ets`, which calls production `initBuildConfig` on `driver/build_system/test/demo_hap/build_config.json`.
- Run the produced ABC locally through the Ark runtime when the same-toolchain console baseline succeeds.

## 008-arkts_driver_interop_runtime-task-1-add-interop-helper-boundary-scenario-2

Target product surface: interop helper calls consumed by config initialization and utility code.

Executable validation:
- The runtime/compiler test executes production `initBuildConfig` from `driver/build_system/ets_src/init/process_build_config.ets`.
- That code must consume `readTextFile`, `fileExists`, `ensureFileExists`, `ensurePathExists`, `substituteEnvVarsInJSON`, and `resolveNativeCompilerLibraryPath` from `driver/build_system/ets_src/util/interop_helper.ets`.
- The validation includes a source/API check that `process_build_config.ets` imports the helper boundary and that ArkTS product sources do not import Node.js `fs`, `path`, `os`, `child_process`, or `process` APIs.

## 008-arkts_driver_interop_runtime-task-1-add-interop-helper-boundary-scenario-3

Expected outcome: normalized absolute module paths and SDK stub paths supplied by the config-model are confirmed to exist, environment placeholders are substituted, native library resolution succeeds or fails deterministically, binary size checks distinguish missing from empty files, and missing path fixtures raise the expected `DriverError` reason.

Executable validation:
- The compiled test asserts `buildSdkPath`, `pandaSdkPath`, `moduleRootPath`, and dependency module paths for `harA` and `harB` are absolute and match the TypeScript reference paths for `demo_hap`.
- It asserts alias, SDK alias, interop SDK metadata, and native-library metadata are initialized through the helper-backed route.
- It asserts missing build config, missing module path, and missing SDK stub fixtures raise `DriverError` causes `missing-build-config`, `missing-module-path`, and `missing-sdk-stub-path`.
- It asserts `ensureNonEmptyBinaryFile` reports `missing-output-abc` for an absent ABC and `empty-output-abc` for a zero-byte ABC using the production helper boundary.

## 008-arkts_driver_interop_runtime-task-1-add-interop-helper-boundary-scenario-4

Evidence: runtime/compiler test ABC asserts resolved paths against the TypeScript reference output, asserts missing SDK/module/native-library fixtures fail through the helper boundary, and asserts `ensureNonEmptyBinaryFile` reports `missing-output-abc` for absent files and `empty-output-abc` for zero-byte binary files.

Executable validation:
- `run.sh` stores compile/runtime logs in a deterministic temporary artifact directory on failure or environment blocker.
- Before attributing stdout/runtime failures to product code, `run.sh` compiles and runs a minimal same-toolchain console baseline with the same `es2panda`, `ark`, `etsstdlib`, and runtime flags.
- If the baseline cannot compile, run, or emit stdout, validation reports an environment blocker and preserves compile/import/API/no-Node evidence.
- If the baseline succeeds, `process_build_config_test.abc` must exit 0 and print `config-resolution-ok`.
