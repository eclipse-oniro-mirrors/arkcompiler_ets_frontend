# Validation Scenarios: 007 ArkTS driver config model task 4

Validation is offline, deterministic, mock-backed only where the production ArkTS interop boundary already provides local file/path helpers, and uses no network, external provider, or device access.

## 007-arkts_driver_config_model-task-4-add-build-config-resolution-model-scenario-1

Trigger: an operator compiles and runs a standalone ArkTS test that parses `demo_hap` build config JSON and calls `initBuildConfig` with interop-backed file/path helpers.

Executable validation:
- Compile the canonical product entrypoint `driver/build_system/ets_src/entry.ets` with the product `arktsconfig.json` to verify the production ArkTS build-system module still compiles with all imported production dependencies.
- Compile `driver/build_system/test/ets_ut/process_build_config_test.ets`, which imports and executes the production `initBuildConfig` API and its production dependencies.
- Run the produced ABC locally through the Ark runtime when available.

## 007-arkts_driver_config_model-task-4-add-build-config-resolution-model-scenario-2

Target surface: the ArkTS build-config initialization API.

Executable validation:
- The test calls `initBuildConfig(root + '/test/demo_hap/build_config.json')` from `ets_src/init/process_build_config.ets`.
- The validation also runs a source no-Node check over `driver/build_system/ets_src` to catch forbidden Node runtime imports in the ArkTS product path.

## 007-arkts_driver_config_model-task-4-add-build-config-resolution-model-scenario-3

Expected outcome: returned config contains absolute `buildSdkPath`, absolute dependency `modulePath` values for `harA` and `harB`, normalized alias/interop metadata, and deterministic rejection for an unsupported-mode fixture.

Executable validation:
- The standalone ArkTS test asserts absolute/resolved `buildSdkPath`, `pandaSdkPath`, `moduleRootPath`, and dependency `modulePath` values against the TypeScript-driver reference paths for `demo_hap`.
- It asserts `harA` and `harB` language values, `harA -> harB` dependency metadata, initialized alias/SDK alias/interop metadata, and recorded native-library metadata.
- It calls the unsupported-mode fixture and requires `ErrorCode.BUILDSYSTEM_UNSUPPORTED_FEATURE`.

## 007-arkts_driver_config_model-task-4-add-build-config-resolution-model-scenario-4

Evidence: the compiled test ABC exits 0 after asserting resolved paths against the TypeScript-driver reference output and observing the expected error code for the negative fixture.

Executable validation:
- `run.sh` writes all compile/runtime logs under a deterministic temporary validation artifact directory.
- If the local ArkTS runtime is unavailable or the minimal console baseline fails under the same toolchain, the harness reports an environment blocker for runtime evidence and still preserves compile/import/API/no-Node evidence.
- If the runtime is available, the process-build-config ABC must exit 0 and print `config-resolution-ok`.
