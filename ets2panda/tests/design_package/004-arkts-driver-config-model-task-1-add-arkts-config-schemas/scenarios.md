# Validation Scenarios: Add ArkTS config schemas

Scenario root: `004-arkts_driver_config_model-task-1-add-arkts-config-schemas`

Validation mode: offline, deterministic, no network, no external provider, no device, and no mocks. The scenarios exercise the production ArkTS config schema and constants modules through local compiler/runtime commands when the toolchain is available.

## 004-arkts_driver_config_model-task-1-add-arkts-config-schemas-scenario-1

Trigger: an operator compiles and runs a standalone ArkTS test driver that imports `BuildConfig` and `DependencyModuleConfig`, constructs a dependency module record, embeds it in a sample build config, and prints `packageName`.

Expected evidence:
- The harness generates a transient ArkTS validation driver outside production source.
- The driver imports the production `driver/build_system/ets_src/types.ets` module and references `BuildConfig` and `DependencyModuleConfig` as runtime-constructible schema types.
- The driver also imports the production constants module `driver/build_system/ets_src/pre_define.ets` so the target constants surface participates in compilation.
- The validation ABC compiles with the local `es2panda` toolchain and production-compatible `arktsconfig` settings.

## 004-arkts_driver_config_model-task-1-add-arkts-config-schemas-scenario-2

Target surface: the ArkTS config schema and constants module.

Expected evidence:
- `driver/build_system/ets_src/types.ets` compiles as part of the validation package.
- `driver/build_system/ets_src/pre_define.ets` compiles as part of the validation package.
- The production schema module supports construction of a `DependencyModuleConfig` record.
- The production schema module supports construction of a `BuildConfig` record that embeds that dependency in `dependencyModuleList`.
- The validation does not satisfy acceptance with source inventory or documentation-only checks.

## 004-arkts_driver_config_model-task-1-add-arkts-config-schemas-scenario-3

Expected outcome: the test ABC compiles, runs, and prints the expected package name with no schema construction failure.

Expected evidence:
- A same-toolchain console baseline is compiled and run before attributing stdout failure to product code.
- If the baseline cannot run or cannot produce stdout, the harness reports a toolchain/runtime environment blocker rather than a product schema failure.
- When the baseline passes, the schema validation ABC must exit 0.
- Runtime stdout must contain exactly the expected package name `entry.pkg` after normalization.

## 004-arkts_driver_config_model-task-1-add-arkts-config-schemas-scenario-4

Evidence: an es2panda compile/run command for the test driver exits 0 and stdout contains the expected package name.

Expected evidence:
- The harness captures compile command, runtime command, stdout, stderr, and assertions in a transient artifact directory.
- The compile command fails non-zero if the production ArkTS schemas or constants do not compile.
- The runtime command fails non-zero if schema construction fails or the printed `packageName` is not the expected value.
- If the local toolchain/runtime is missing, the harness reports an environment blocker rather than fabricating pass evidence.
