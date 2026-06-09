# Decommission Ledger

Schema Version: `triborg.decommission-ledger.v1`

## decommission-1
- Request Task: 7
- Target: driver/build_system/src/util/ets2panda.ts subprocess-based es2panda invocation (child_process.spawn)
- Category: helper
- Action: `replace`
- Verification: New ArkTS ets2panda.ets uses global.es2panda._CreateConfig() ANI calls; TypeScript ets2panda.ts child_process.spawn code is still present in src/ but unused by the new ArkTS driver
- Allowlist: none
- Keep Reason: n/a

## decommission-2
- Request Task: 8
- Target: driver/build_system/src/build/compile_process_worker.ts subprocess worker for compilation
- Category: helper
- Action: `replace`
- Verification: Ported base_mode.ets uses direct in-process ANI calls; compile_process_worker.ts worker file is untouched in src/ but not referenced by the ArkTS driver
- Allowlist: none
- Keep Reason: n/a

## decommission-3
- Request Task: 8
- Target: driver/build_system/src/build/compile_thread_worker.ts thread worker for compilation
- Category: helper
- Action: `replace`
- Verification: Ported base_mode.ets uses direct ANI calls; compile_thread_worker.ts is untouched in src/ but not referenced by the ArkTS driver
- Allowlist: none
- Keep Reason: n/a

## decommission-4
- Request Task: 8
- Target: driver/build_system/src/util/TaskManager.ts multi-process/multi-thread task dispatching
- Category: helper
- Action: `replace`
- Verification: Ported base_mode.ets performs sequential in-process compilation; TaskManager.ts process/thread pool is untouched in src/ but not referenced by the ArkTS driver
- Allowlist: none
- Keep Reason: n/a

## decommission-5
- Request Task: 7
- Target: driver/build_system/src/dependency_analyzer.ts external dependency_analyzer binary invocation
- Category: helper
- Action: `replace`
- Verification: Ported base_mode.ets uses direct file parsing via InteropNativeModule bindings instead of spawning dependency_analyzer subprocess; dependency_analyzer.ts is untouched in src/ but not referenced by the ArkTS driver
- Allowlist: none
- Keep Reason: n/a

## decommission-6
- Request Task: 1
- Target: Node.js package.json 'build' script (tsc compilation for TypeScript driver) as the sole build target for the ArkTS build system component
- Category: surface
- Action: `replace`
- Verification: New CMakeLists.txt or build script compiles ets_src/ to ABC using es2panda; original tsc-based build still works for the TypeScript driver but is not the path for the ArkTS driver
- Allowlist: none
- Keep Reason: n/a
