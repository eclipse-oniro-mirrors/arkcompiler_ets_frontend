# project

This is a shared benchmark-case project for LSP API performance tests.

Design goals:
- Reuse one compact project across multiple benchmark APIs.
- Provide cross-file symbols for completion, definition, references, rename, and quick info.
- Keep dedicated files for diagnostics and formatting scenarios.
- Include both a small file and a large file for size-based performance testing.

Key files:
- `entry.ets`: main entry and anchor hub.
- `api_surface.ets`: export aggregation for cross-file navigation.
- `domain.ets`, `services.ets`, `registry.ets`, `helpers.ets`: shared symbol graph.
- `diagnostics_cases.ets`: syntactic/semantic boundary samples.
- `formatting_cases.ets` + `expected/formatting_expected.ets`: formatting scenarios.
- `shared_small_100.ets`, `shared_large_1000.ets`: size-scale workloads.
