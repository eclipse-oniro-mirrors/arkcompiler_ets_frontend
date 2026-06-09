# Outcome Contract

Schema Version: `triborg.outcome-contract.v1`

## outcome-1
- Request Task: 1
- Role: `supporting_evidence`
- Request Item: Create ported ArkTS build system source structure with compilable .ets source tree
- Target Surface: driver/build_system/ets_src/ directory and driver/build_system/arktsconfig.json
- Actor/Trigger: Operator runs es2panda --arktsconfig on the ported source tree
- Expected Outcome: All .ets files in ets_src/ parse without errors; es2panda exits 0 with no diagnostic errors
- Evidence Type: CLI command
- Freshness: current source
- Mock Policy: no_mocks

## outcome-2
- Request Task: 2
- Role: `supporting_evidence`
- Request Item: Port types and pre-defined constants to ArkTS
- Target Surface: ets_src/types.ets and ets_src/pre_define.ets compilation unit
- Actor/Trigger: Test driver imports BuildConfig from types.ets, constructs a sample config, prints packageName
- Expected Outcome: Compiled ABC runs and prints the expected packageName string
- Evidence Type: runtime/compiler test
- Freshness: current build
- Mock Policy: no_mocks

## outcome-3
- Request Task: 3
- Role: `supporting_evidence`
- Request Item: Port logger to ArkTS
- Target Surface: ets_src/logger.ets Logger singleton
- Actor/Trigger: Test module calls Logger.getInstance().printInfo('build started')
- Expected Outcome: Compiled ABC outputs 'build started' to runtime stdout
- Evidence Type: runtime/compiler test
- Freshness: current build
- Mock Policy: no_mocks

## outcome-4
- Request Task: 4
- Role: `supporting_evidence`
- Request Item: Port utility modules (graph, error, utils) to ArkTS
- Target Surface: ets_src/util/graph.ets Graph.topologicalSort()
- Actor/Trigger: Test module creates Graph with two nodes and a dependency edge, calls topological sort
- Expected Outcome: Returns nodes in correct dependency order
- Evidence Type: runtime/compiler test
- Freshness: current build
- Mock Policy: no_mocks

## outcome-5
- Request Task: 5
- Role: `supporting_evidence`
- Request Item: Port generate_arktsconfig to ArkTS
- Target Surface: ets_src/build/generate_arktsconfig.ets ArkTSConfigGenerator
- Actor/Trigger: Test module calls generator with demo_hap build_config.json input
- Expected Outcome: Generated ArkTSConfig object has compilerOptions, files, and paths matching TypeScript generator output
- Evidence Type: runtime/compiler test
- Freshness: current build
- Mock Policy: no_mocks

## outcome-6
- Request Task: 6
- Role: `supporting_evidence`
- Request Item: Port process_build_config to ArkTS
- Target Surface: ets_src/init/process_build_config.ets initBuildConfig()
- Actor/Trigger: Test module passes demo_hap build_config.json to initBuildConfig
- Expected Outcome: Resolved BuildConfig with absolute module paths and buildSdkPath matching TypeScript driver output
- Evidence Type: runtime/compiler test
- Freshness: current build
- Mock Policy: no_mocks

## outcome-7
- Request Task: 7
- Role: `supporting_evidence`
- Request Item: Port es2panda invocation to use ArkTS native bindings instead of subprocess
- Target Surface: ets_src/util/ets2panda.ets compile through ANI bindings
- Actor/Trigger: Test driver calls ported Ets2panda to compile harB/index.ets
- Expected Outcome: ABC bytecode file produced on disk with non-zero size for harB module
- Evidence Type: runtime/compiler test
- Freshness: current build
- Mock Policy: no_mocks

## outcome-8
- Request Task: 8
- Role: `supporting_evidence`
- Request Item: Port base_mode orchestration to ArkTS (sequential mode)
- Target Surface: ets_src/build/base_mode.ets module compilation scheduling
- Actor/Trigger: Run ported driver on demo_hap build config in sequential mode
- Expected Outcome: Modules compiled in dependency order (harB, harA, entry); merged ABC files in dist/
- Evidence Type: CLI command
- Freshness: current build
- Mock Policy: no_mocks

## outcome-9
- Request Task: 9
- Role: `supporting_evidence`
- Request Item: Port entry point and build_mode to ArkTS
- Target Surface: ets_src/entry.ets build() function routing to BuildMode.run()
- Actor/Trigger: Operator runs compiled driver ABC with demo_hap build_config.json path
- Expected Outcome: Driver executes sequential compilation for all three modules, exits 0 with ABC outputs
- Evidence Type: CLI command
- Freshness: current build
- Mock Policy: no_mocks

## outcome-10
- Request Task: 10
- Role: `supporting_evidence`
- Request Item: End-to-end demo_hap compilation smoke test with cross-module import resolution
- Target Surface: Compiled entry module ABC with cross-module symbol resolution
- Actor/Trigger: Run compiled ArkTS driver on demo_hap, then inspect entry ABC with ark_disasm
- Expected Outcome: Entry ABC shows resolved imports for strA (from harA) and strB (from harB)
- Evidence Type: CLI command
- Freshness: current build
- Mock Policy: no_mocks

## outcome-11
- Request Task: 11
- Role: `supporting_evidence`
- Request Item: Add ported build system tests
- Target Surface: test/ets_ut/ test .ets files covering config parsing, graph sort, single-file compile, multi-module orchestration
- Actor/Trigger: Script iterates over test/ets_ut/ .ets files, compiles and runs each
- Expected Outcome: All test ABCs compile and run with exit code 0
- Evidence Type: CLI command
- Freshness: current build
- Mock Policy: no_mocks
