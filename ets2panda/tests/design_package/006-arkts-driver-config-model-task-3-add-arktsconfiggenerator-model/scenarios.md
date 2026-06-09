# Validation Scenarios: Add ArkTSConfigGenerator model

Scenario root: `006-arkts_driver_config_model-task-3-add-arktsconfiggenerator-model`

Validation mode: offline, deterministic, no network, no external provider, no device, and no mocks. The scenarios exercise the production ArkTS config generator model through local ArkTS compiler/runtime commands when the toolchain is available.

## 006-arkts_driver_config_model-task-3-add-arktsconfiggenerator-model-scenario-1

Trigger: an operator compiles and runs a standalone ArkTS test that loads the demo build config through resolved model data, constructs `ArkTSConfigGenerator`, and generates the entry and dependency module configs.

Expected evidence:
- The harness generates a transient ArkTS validation package outside production source.
- The validation package copies the production `driver/build_system/ets_src/build/generate_arktsconfig.ets` module and all production source dependencies it imports.
- The test constructs resolved model data for `driver/build_system/test/demo_hap` covering `harB`, `harA`, and `entry` modules.
- The test constructs `ArkTSConfigGenerator`, then generates configs for `harB`, `harA`, and `entry` through the production API.

## 006-arkts_driver_config_model-task-3-add-arktsconfiggenerator-model-scenario-2

Target surface: the ArkTS config generator API.

Expected evidence:
- `driver/build_system/ets_src/build/generate_arktsconfig.ets` compiles as part of the validation package.
- Required production dependencies `types.ets`, `pre_define.ets`, `logger.ets`, `util/error.ets`, and `util/utils.ets` compile as part of the same package.
- The validation calls `ArkTSConfigGenerator.generateConfigForModule`, `ArkTSConfig.toJSONText`, and map accessors for `compilerOptions`, `files`, and `paths`.
- The validation does not satisfy acceptance with source inventory, grep-only checks, or documentation-only checks.

## 006-arkts_driver_config_model-task-3-add-arktsconfiggenerator-model-scenario-3

Expected outcome: generated `compilerOptions`, `files`, and `paths` match the TypeScript-driver golden snapshot for the same demo input.

Expected evidence:
- The standalone test contains a deterministic golden snapshot for the TypeScript-driver demo shape.
- `harB`, `harA`, and `entry` configs are structurally asserted for package name, base URL, root dir, cache dir, file list, and path mappings.
- The entry config must include path aliases for `entry`, `entry/a`, `entry/b`, `entry/c`, and `entry/d`.
- Dependency configs must include path aliases for `harA`, `harA/index`, `harB`, and `harB/index` as applicable.

## 006-arkts_driver_config_model-task-3-add-arktsconfiggenerator-model-scenario-4

Evidence: the runtime/compiler test exits 0 after structural assertions against the golden snapshot.

Expected evidence:
- A same-toolchain console baseline is compiled and run before attributing stdout failure to product code.
- If the baseline cannot run or cannot produce stdout, the harness reports a toolchain/runtime environment blocker rather than a product generator failure.
- When the baseline passes, the ArkTS generator validation ABC must compile, verify, run, and exit 0.
- Runtime stdout must contain `arktsconfig-generator-golden-ok`.
- The harness captures compile command, runtime command, stdout, stderr, and assertion artifacts in a transient artifact directory.
