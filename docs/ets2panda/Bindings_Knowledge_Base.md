# ETS2Panda Bindings Knowledge Base

> Document version: v2.0
> Last updated: 2026-06-04
> Scope: non-obvious constraints for `ets_frontend/ets2panda/bindings`, including TypeScript wrappers, Node native modules, generated bindings, LSP wrappers, and bindings tests

This file is intentionally short. Do not use it as a directory map. Read the code for file-level responsibilities; use this document for rules that are easy to miss.

## Must Do

- Read the closest rules before editing:
  - `ets_frontend/ets2panda/bindings/README.md`
  - nearest directory-level rules for `public`, `lsp`, or tests when those areas are involved
- First locate the failing layer before changing code:
  - TypeScript wrapper
  - native module loading
  - N-API conversion
  - native bridge implementation
  - generated binding declarations
  - public C API
  - C++ LSP API
  - parser / binder / checker upstream data
- Keep the native and TypeScript call chain synchronized for exposed APIs:
  - `native/src/bridges.cpp` or generated native bridge
  - `native/src/lsp.cpp` for LSP APIs
  - `src/common/Es2pandaNativeModule.ts`
  - `src/generated/*` when the API is generated
  - `src/lsp/lspNode.ts` when result objects are pointer-backed or LSP-specific
  - `src/lsp/lsp_helper.ts` for user-facing LSP methods
  - `src/index.ts` or `src/lsp/index.ts` when exports change
  - bindings tests and expected JSON when user-visible behavior changes
- Keep native bridge signatures and TypeScript declarations exactly aligned:
  - method name
  - argument count
  - argument order
  - numeric width
  - boolean conversion
  - string encoding
  - pointer ownership
  - array encoding
- Treat `KNativePointer` values as owned native resources with lifecycle constraints. Do not assume a pointer remains valid after its owning context, result container, or native object has been destroyed.
- Reuse existing string, string-array, pointer-array, and typed-array encoding helpers. The boundary format is part of the ABI.
- Confirm the upstream public or LSP API result before changing wrapper logic for semantic mismatches.
- For LSP APIs, confirm the context state and source file setup before investigating conversion code.
- For cross-file LSP APIs, verify arktsconfig/build config generation, fixture file paths, dependency cache, and source/context construction.
- For native module load failures, check the produced `.node` files and registered library names before changing API code.
- When adding a native source file, update both GN and CMake where applicable.
- Build or regenerate TypeScript output through npm scripts; do not hand-edit compiled outputs.
- Add or update focused bindings tests for every TS-visible API or behavior change.
- For expected JSON changes, compare the old and new result structurally before accepting the update.

## Ask Before

- Ask before changing public C API signatures, public context lifecycle, or exported C structures to satisfy a bindings symptom.
- Ask before changing C++ LSP API signatures or result structures from bindings code.
- Ask before changing generated binding sources when the generator input or source of truth is unclear.
- Ask before changing native module library names, load paths, output names, or packaging layout.
- Ask before changing pointer ownership conventions or object destruction behavior.
- Ask before adding TS-side compatibility behavior that hides a native, public API, LSP, or compiler bug.

## Must Not Do

- Do not reimplement parser, binder, checker, public API, or LSP semantics in TypeScript wrappers.
- Do not hide upstream semantic defects by wrapping them as bindings compatibility behavior.
- Do not bypass `public` or `lsp` formal APIs to call private C++ internals from bindings.
- Do not add a TypeScript method without adding or verifying the matching native bridge export.
- Do not add a native bridge function without a TypeScript declaration/wrapper and test coverage.
- Do not hand-edit `dist`, `dist-test`, `node_modules`, or compiled JavaScript outputs.
- Do not treat generated binding output as the source of truth when generator inputs should be changed.
- Do not pass raw pointers through TypeScript without a clear wrapper and owner contract.
- Do not invent one-off string, array, or pointer encodings.
- Do not update expected JSON to match a regression without tracing the native and upstream result.
- Do not add source files to GN but not CMake, or to CMake but not GN.
- Do not commit local absolute build paths, SDK paths, or temporary library paths.

## Validation Rules

- If only TypeScript wrapper behavior changed, run the nearest bindings unit or e2e test.
- If native bridge, N-API conversion, or `.node` output changed, build the native binding target and run related bindings tests.
- If `src/lsp/*` changed, run the relevant LSP bindings e2e test and consider the matching C++ LSP unit test.
- If a public API wrapper changed, verify the public C API result independently when practical.
- If result conversion changed, test at least:
  - empty result
  - one result
  - multiple results
  - optional/missing fields
  - path and range/offset fields where applicable
- If pointer-backed wrappers changed, test that getters still work only while the owning native object is valid.
- If cross-module behavior changed, test:
  - single-file project
  - two-file import/export
  - cross-module fixture
  - stale cache or deleted file path when relevant
- If expected JSON changed, run the specific test once before and after update when possible.

Common commands:

```sh
cd ets_frontend/ets2panda/bindings
npm run run
npm run test:build
npm run test
npm run ut_test
```

Use only after confirming behavior is intended:

```sh
cd ets_frontend/ets2panda/bindings
npm run test:update
```

Native target examples:

```sh
ninja -C <out-dir> ts_bindings
cmake --build <build-dir> --target public
```

## Routing Rules

- Native module cannot load -> inspect `.node` output, `loadLibraries.ts`, platform suffix/prefix, build output copy, and library-name registration.
- Method exists in TS but throws `Not implemented` -> inspect TypeScript declarations, native bridge exports, generated bindings, and loaded native module version.
- LSP method exists in C++ but fails through TS -> inspect `native/src/lsp.cpp`, `src/common/Es2pandaNativeModule.ts`, `src/lsp/lspNode.ts`, and `src/lsp/lsp_helper.ts`.
- Public API signature or context behavior is wrong -> route to `ets_frontend/ets2panda/public`.
- LSP result is already wrong before conversion -> route to `ets_frontend/ets2panda/lsp`.
- Pointer, string, array, boolean, or numeric values are corrupted across the boundary -> inspect `native/include/panda_types.h`, `native/src/convertors-napi.cpp`, and TypeScript encoding helpers.
- Cross-module LSP behavior fails -> inspect TS project/config/cache setup first, then upstream LSP indexing/context behavior.
- Expected JSON changed -> decide whether behavior changed intentionally before running `npm run test:update`.
