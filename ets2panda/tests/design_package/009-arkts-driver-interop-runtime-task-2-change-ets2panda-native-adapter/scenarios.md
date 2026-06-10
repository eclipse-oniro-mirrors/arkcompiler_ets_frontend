# Validation Scenarios: 009 native adapter

This validation is offline, deterministic, uses no network, has live/device validation disabled, and does not modify production source. Native/compiler execution is attempted only with local toolchain binaries; missing or broken baseline toolchain is reported as an environment blocker.

## 009-arkts_driver_interop_runtime-task-2-change-ets2panda-native-adapter-scenario-1

Trigger: a compiled ArkTS test instantiates the ported `Ets2panda` adapter and compiles the demo `harB` source.

Concrete validation:
- Compile the canonical ArkTS build-system entrypoint from `driver/build_system/ets_src/entry.ets` with the product `arktsconfig.json`.
- Compile `driver/build_system/test/ets_ut/ets2panda_native_adapter_contract_test.ets` with the product `arktsconfig.json`.
- Inspect the adapter contract test to confirm it constructs `Ets2panda`, passes `NativeCompilerConfigRequest { argv, configText }`, targets `driver/build_system/test/demo_hap/harB/index.ets`, calls `compileFile()`, and checks/prints the contract success marker.
- If Ark runtime execution is locally usable, run a same-toolchain console baseline before runtime attribution; baseline failure is an environment blocker.

## 009-arkts_driver_interop_runtime-task-2-change-ets2panda-native-adapter-scenario-2

Target product surface: native compiler lifecycle route in the ArkTS adapter.

Concrete validation:
- Inspect `driver/build_system/ets_src/util/ets2panda.ets` for the direct in-process native lifecycle calls: `_MemInitialize`, `_CreateConfig`, `_CreateContextFromFile`, `_CreateContextGenerateAbcForExternalSourceFiles`, `_ProceedToState`, `_ContextState`, `_ContextErrorMessage`, `_GetAllErrorMessages`, `_DestroyContext`, `_DestroyConfig`, and `_MemFinalize`.
- Fail if the ArkTS path routes through subprocess or shell APIs.

## 009-arkts_driver_interop_runtime-task-2-change-ets2panda-native-adapter-scenario-3

Expected outcome: the adapter initializes native memory, receives generated ArkTS config as `NativeCompilerConfigRequest { argv, configText }`, maps it to `_CreateConfig(argc, argv)` through the direct in-process binding contract, creates config/context handles, progresses to codegen, destroys all native handles, finalizes native memory, and produces a non-empty `.abc`; invalid source reports requested state, observed state, and native error text.

Concrete validation:
- Require `_MemInitialize()` before `_CreateConfig(argv.length, argv)` and cleanup through `_DestroyContext`, `_DestroyConfig`, and `_MemFinalize` in `finally`-owned cleanup logic.
- Require default state progression through `ES2PANDA_STATE_PARSED`, `ES2PANDA_STATE_BOUND`, `ES2PANDA_STATE_CHECKED`, `ES2PANDA_STATE_ASM_GENERATED`, and `ES2PANDA_STATE_BIN_GENERATED`.
- Require diagnostics to include `requestedState`, `observedState`, `contextErrorMessage`, and `allErrorMessages`.
- Require binary output validation through binding-backed `ensureNonEmptyBinaryFile()` with distinct `missing-output-abc` and `empty-output-abc` outcomes.

## 009-arkts_driver_interop_runtime-task-2-change-ets2panda-native-adapter-scenario-4

Evidence: runtime/compiler test ABC asserts output existence and non-zero byte size through binding-backed file existence plus binary stat/size behavior, asserts `_MemFinalize` is reached on success and compiler-error paths through observable cleanup-safe repeated compile runs, and the sequential demo driver consumes the adapter without any dependency-analyzer subprocess to produce downstream non-empty artifacts.

Concrete validation:
- Require `driver/build_system/ets_src/util/interop_helper.ets` to expose binding-backed `fileExists`, `fileSize`, and `ensureNonEmptyBinaryFile` behavior.
- Require the native adapter contract test source to exist and compile.
- Require `driver/build_system/ets_src/build/base_mode.ets` to construct/call `Ets2panda` through `compileExternalSourceSet()` for sequential downstream orchestration.
- Fail if `BaseMode.run()` is empty or if it does not consume the native adapter, because outcome-8 cannot be satisfied.
- Fail if any `driver/build_system/ets_src/` file imports or invokes `child_process`, `spawn`, shell command construction, or `dependency_analyzer`.
