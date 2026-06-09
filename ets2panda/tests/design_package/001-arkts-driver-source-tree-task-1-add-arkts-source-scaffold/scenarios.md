# Scenarios: Add ArkTS source scaffold

## 001-arkts_driver_source_tree-task-1-add-arkts-source-scaffold-scenario-1

Operator trigger: from the target repository root, run:

```bash
es2panda --ets-module --arktsconfig driver/build_system/arktsconfig.json --output driver/build_system/dist/build_system.abc driver/build_system/ets_src/entry.ets
```

The validation script resolves `es2panda` from `PATH` or common local build output locations, creates `driver/build_system/dist/`, removes any stale `build_system.abc`, then executes the product compiler command against the production ArkTS driver source tree.

## 001-arkts_driver_source_tree-task-1-add-arkts-source-scaffold-scenario-2

Target surface: `driver/build_system/ets_src/`, `driver/build_system/arktsconfig.json`, and `arkts_bindings/src/index.ets` as the `@arkts-bindings` import-resolution target.

The validation script checks that the expected scaffold directories and ArkTS source files exist in the production tree and that the module config is present before invoking the compiler. This inventory check is only a precondition; acceptance is determined by the compiler run and output artifact.

## 001-arkts_driver_source_tree-task-1-add-arkts-source-scaffold-scenario-3

Expected outcome: all `.ets` sources in the scaffold parse, imports resolve including `@arkts-bindings`, the command exits 0, and `driver/build_system/dist/build_system.abc` is produced without diagnostics.

The validation script captures compiler stdout/stderr, fails on any non-zero compiler exit, fails when diagnostic-looking output is produced, and fails when `build_system.abc` is missing or empty.

## 001-arkts_driver_source_tree-task-1-add-arkts-source-scaffold-scenario-4

Executable evidence: successful local compiler command against the production ArkTS driver sources.

The validation script records the exact command, compiler output, and artifact size under `tests/design_package/001-arkts-driver-source-tree-task-1-add-arkts-source-scaffold/artifacts/` for offline review.

## Negative invariant: decommission-6

The ArkTS build-system validation must not use the Node.js `package.json` `build` script, `tsc`, `driver/build_system/src/`, or generated TypeScript artifacts as the ArkTS driver build path. The validation script fails if `driver/build_system/arktsconfig.json` includes or references those TypeScript build surfaces.
