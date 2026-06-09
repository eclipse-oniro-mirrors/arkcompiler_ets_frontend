# Validation Scenarios: Add graph and model utilities

Scenario root: `005-arkts_driver_config_model-task-2-add-graph-and-model-utilities`

Validation mode: offline, deterministic, no network, no external provider, no device, and no mocks. The scenarios exercise the production ArkTS graph, error, logger, constants, and utility modules through local compiler/runtime commands when the toolchain is available.

## 005-arkts_driver_config_model-task-2-add-graph-and-model-utilities-scenario-1

Trigger: an operator compiles and runs a standalone ArkTS graph test that creates two module nodes where `harA` depends on `harB`, builds a graph, and calls topological sort.

Expected evidence:
- The harness generates a transient ArkTS validation driver outside production source.
- The driver imports the production `driver/build_system/ets_src/util/graph.ets` API through a copied validation package that preserves imported production dependencies.
- The driver creates `GraphNode<string>` instances for `harB` and `harA`, records `harA.predecessors.add('harB')`, builds `Graph.createGraphFromNodes`, and calls `Graph.topologicalSort`.
- The validation ABC compiles with the local `es2panda` toolchain and production-compatible `arktsconfig` settings.

## 005-arkts_driver_config_model-task-2-add-graph-and-model-utilities-scenario-2

Target surface: the ArkTS graph and error utility modules.

Expected evidence:
- `driver/build_system/ets_src/util/graph.ets` compiles as part of the validation package.
- `driver/build_system/ets_src/util/error.ets` compiles as part of the validation package.
- Required production dependencies `logger.ets`, `pre_define.ets`, and `util/utils.ets` compile as part of the same package.
- The validation does not satisfy acceptance with source inventory or documentation-only checks.

## 005-arkts_driver_config_model-task-2-add-graph-and-model-utilities-scenario-3

Expected outcome: runtime output or assertion shows `harB` before `harA`, and a separate cyclic fixture raises a deterministic `DriverError`.

Expected evidence:
- A same-toolchain console baseline is compiled and run before attributing stdout failure to product code.
- If the baseline cannot run or cannot produce stdout, the harness reports a toolchain/runtime environment blocker rather than a product graph failure.
- When the baseline passes, the graph validation ABC must exit 0.
- Runtime stdout must contain `harB,harA` and `cycle-error:11410030` after normalization.

## 005-arkts_driver_config_model-task-2-add-graph-and-model-utilities-scenario-4

Evidence: the ArkTS test ABC compile/run command exits 0 for the valid graph and the negative test observes the expected error class.

Expected evidence:
- The harness captures compile command, runtime command, stdout, stderr, and assertions in a transient artifact directory.
- The compile command fails non-zero if the production ArkTS graph/error utility modules or their imported production dependencies do not compile.
- The runtime command fails non-zero if topological sort does not order `harB` before `harA` or if the cycle path does not throw `DriverError` with `ErrorCode.BUILDSYSTEM_GRAPH_ERROR`.
- If the local toolchain/runtime is missing, the harness reports an environment blocker rather than fabricating pass evidence.
