# Scenarios: 010 ArkTS driver sequential orchestrator BaseMode build

Validation is offline, deterministic, uses no network, and performs local compiler/runtime-toolchain checks only. Runtime execution is gated behind a same-toolchain `ark` console baseline so toolchain crashes are reported as environment blockers rather than product defects.

## 010-arkts_driver_sequential_orchestrator-task-1-change-basemode-sequential-build-scenario-1

When an operator runs the ported ArkTS driver with the resolved `demo_hap` build config, `BaseMode.run()` validates the graph before native compilation, rejects cycles or unresolved modules before outputs are created, compiles `harB/index.ets` before `harA/index.ets` before entry `d.ets`, `c.ets`, `b.ets`, `a.ets`, and stops on the first deterministic config, graph, compiler, or output error.

Executable validation materializes this by compiling the canonical ArkTS driver entrypoint and the BaseMode sequential contract test that imports the production `BaseMode`, `Graph`, type, and error surfaces. The test source asserts compile-unit collection order, request shaping, unresolved dependency rejection, cycle rejection, and config error rejection. If the local `ark` runtime baseline succeeds, the same compiled test ABC is executed; if the baseline fails, the harness records a toolchain/runtime blocker.

## 010-arkts_driver_sequential_orchestrator-task-1-change-basemode-sequential-build-scenario-2

The visible outcome is non-empty ABC artifacts for `harB.abc`, `harA.abc`, and the entry module/source outputs in the configured dist output.

Executable validation compiles the canonical ArkTS product entrypoint and BaseMode sequential test to non-empty ABC files, disassembles those ABCs to prove bytecode readability, and statically checks the production output-validation path requires non-empty ABC files with the accepted `missing-output-abc` / `empty-output-abc` failure reasons. When `ark` runtime is healthy and `ARK_VALIDATE_RUNTIME=1` is set, the harness runs a driver smoke and stats the configured `demo_hap/dist/harB.abc`, `demo_hap/dist/harA.abc`, and `demo_hap/dist/entry.abc` artifacts.

## 010-arkts_driver_sequential_orchestrator-task-1-change-basemode-sequential-build-scenario-3

Executable evidence is a local runtime/compiler test or driver run that exits 0 and then stats the expected ABC artifacts as present and greater than zero bytes, plus a graph-failure runtime test that observes rejection before native compiler contexts are created.

Executable validation always performs local compiler tests against production ArkTS sources and emits non-empty ABC/disassembly evidence. Runtime execution and artifact stat checks are attempted only after a same-toolchain console baseline succeeds, preventing environment crashes from being misclassified as product failures. Graph-failure coverage is materialized by the BaseMode sequential contract test path, which invokes `validateGraph()` directly and never reaches `Ets2panda` config/context creation for unresolved dependency or cycle failures.
