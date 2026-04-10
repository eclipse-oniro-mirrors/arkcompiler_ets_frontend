# Benchmark Anchor Notes

## Shared Entry Anchors
- File: `entry.ets`
- Symbol: `targetAdminValue`
- Typical use: references/rename/quick-info anchor

## Definition Cross-File
- Call site: `entry.ets`, `createAdmin("Mika")`
- Definition target: `services.ets`, `createAdmin`

## Completion
- Common completion context can be placed after:
  - `localRegistry.`
  - `targetAdminValue.`
  - `CreateSymbolRegistry(`

## Formatting
- Input file: `formatting_cases.ets`
- Expected file: `expected/formatting_expected.ets`

## Size Benchmarks
- Small file: `shared_small_100.ets` (~100 lines)
- Large file: `shared_large_1000.ets` (~1000 lines)
