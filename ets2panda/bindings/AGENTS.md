# bindings Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Scope

Provides TypeScript wrappers and native Node bridges over the static public API, including editor-facing LSP wrappers.

All paths below are repository-relative. Use the repository `AGENTS.md` and `ets2panda/AGENTS.md` with this guide.

## 2. Code Map

| Path | Responsibility | Risk |
|---|---|---|
| `ets2panda/bindings/src/` | TS wrappers and declarations | High: compiler/consumer contract |
| `ets2panda/bindings/native/` | Native bridges and conversions | High: compiler/consumer contract |
| `ets2panda/bindings/src/lsp/` | LSP wrappers/cache lifecycle | High: compiler/consumer contract |
| `ets2panda/bindings/test/` | Unit/e2e fixtures | High: compiler/consumer contract |
| `ets2panda/bindings/package.json` | Package scripts | High: compiler/consumer contract |
| `ets2panda/bindings/CMakeLists.txt` | Native build registration | High: compiler/consumer contract |

## 3. Knowledge Routing

| Task | Read first |
|---|---|
| Wrapper/native ownership | `docs/knowledge/ets2panda/Bindings_Knowledge_Base.md` |
| Underlying public contract | `docs/knowledge/ets2panda/Public_Knowledge_Base.md` |
| Editor API behavior | `docs/knowledge/ets2panda/LSP_Knowledge_Base.md` |

## 4. Constraints

### 4.1 Do not

- Do not hide compiler or public API semantic failures in a TS wrapper.
- Keep native exports, TS declarations, argument order/width, return structures, and encoding synchronized.
- Preserve context/pointer ownership and resource destruction order.
- Do not hand-edit compiled `dist`, generated bindings, or installed dependencies.
- Use existing LSP context/cache lifecycle; do not create hidden per-feature contexts.
- Add native sources to the applicable GN and CMake build lists.
- Inspect structural JSON differences before changing expectations.

### 4.2 Ask before

Follow repository approval gates for public/binding interfaces, serialized formats, dependencies, and legacy compatibility. Notify the user before changes that may alter emitted bytecode.

## 5. Build and Verification

From `ets2panda/bindings/`, use `npm run run`, `npm run test:build`, `npm run ut_test`, and the relevant `npm test` e2e cases after preparing native libraries/fixtures as described in `ets2panda/bindings/README.md`. Build native changes in the configured static build directory using targets from the current CMake/GN files. LSP changes require relevant `ets2panda/test/unit/lsp/` cases too. Check empty/multiple results, ranges/paths, lifetimes, and cross-file cache behavior. Expectation update scripts require an intended-result comparison first.

A change is done only when intended behavior is implemented, the diff is scoped, relevant checks have run or their absence is explicit, and compatibility/consumer risks are reported. Never describe source inspection or a build-only check as passing runtime tests.
