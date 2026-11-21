# arkts_bindings Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Scope

Exposes the static frontend plugin API to ArkTS through ANI; it is a separate language binding from the Node/TypeScript package.

All paths below are repository-relative. Use the repository `AGENTS.md` and `ets2panda/AGENTS.md` with this guide.

## 2. Code Map

| Path | Responsibility | Risk |
|---|---|---|
| `ets2panda/arkts_bindings/src/` | ArkTS wrapper implementation | High: compiler/consumer contract |
| `ets2panda/arkts_bindings/native/` | ANI native implementation | High: compiler/consumer contract |
| `ets2panda/arkts_bindings/CMakeLists.txt` | Native and ETS target orchestration | High: compiler/consumer contract |
| `ets2panda/arkts_bindings/arktsconfig.in.json` | Generated build configuration input | High: compiler/consumer contract |
| `ets2panda/test/arkts_bindings/` | ANI/API and conversion regressions | High: compiler/consumer contract |

## 3. Knowledge Routing

| Task | Read first |
|---|---|
| Build and usage | `ets2panda/arkts_bindings/README.md` |
| Public API contract | `docs/knowledge/ets2panda/Public_Knowledge_Base.md` |
| Compiler semantics | `docs/knowledge/ets2panda/Static_Frontend_Knowledge_Base.md` |

## 4. Constraints

### 4.1 Do not

- Do not assume the Node/N-API bridge implementation applies to ANI.
- Keep ArkTS declarations, ANI registration, conversions, and public C API signatures aligned.
- Preserve native object/context ownership across language boundaries.
- Modify configuration templates rather than generated build-directory files.
- Do not replace conversion/API tests with source compilation alone.

### 4.2 Ask before

Follow repository approval gates for public/binding interfaces, serialized formats, dependencies, and legacy compatibility. Notify the user before changes that may alter emitted bytecode.

## 5. Build and Verification

From the configured static frontend build directory, run `ninja arkts_bindings` and `ninja arkts_bindings_tests` when the required runtime/ANI configuration is available. Check the relevant cases under `ets2panda/test/arkts_bindings/` for conversion, object ownership, and API behavior. Record unavailable runtime/ANI prerequisites explicitly.

A change is done only when intended behavior is implemented, the diff is scoped, relevant checks have run or their absence is explicit, and compatibility/consumer risks are reported. Never describe source inspection or a build-only check as passing runtime tests.
