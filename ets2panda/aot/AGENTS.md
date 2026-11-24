# aot Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Scope

Contains the static compiler executable entry, allocator initialization, option/diagnostic orchestration, and binary generation.

All paths below are repository-relative. Use the repository `AGENTS.md` and `ets2panda/AGENTS.md` with this guide.

## 2. Code Map

| Path | Responsibility | Risk |
|---|---|---|
| `ets2panda/aot/main.cpp` | CLI flow and source compilation | High: compiler/consumer contract |
| `ets2panda/aot/CMakeLists.txt` | CMake executable target | High: compiler/consumer contract |
| `ets2panda/aot/BUILD.gn` | GN target | High: compiler/consumer contract |
| `ets2panda/util/options.h` | Compiler option interface | High: compiler/consumer contract |
| `ets2panda/util/generateBin.cpp` | Binary output | High: compiler/consumer contract |

## 3. Knowledge Routing

| Task | Read first |
|---|---|
| Static pipeline | `docs/knowledge/ets2panda/Static_Frontend_Knowledge_Base.md` |
| Compiler implementation | `ets2panda/compiler/AGENTS.md` |
| Driver consumers | `docs/knowledge/ets2panda/Driver_Knowledge_Base.md` |

## 4. Constraints

### 4.1 Do not

- Do not confuse this static executable entry with dynamic `es2panda/aot/`.
- Preserve option validation, diagnostic exit status, allocator lifetime, and binary-generation callbacks.
- Do not change shared compilation/semantic behavior only in the CLI and leave public/driver callers inconsistent.
- Keep output and diagnostics on their intended streams and propagate generation failures.
- Inspect both GN and CMake targets before changing build wiring.

### 4.2 Ask before

Follow repository approval gates for public/binding interfaces, serialized formats, dependencies, and legacy compatibility. Notify the user before changes that may alter emitted bytecode.

## 5. Build and Verification

Build the static `es2panda` executable in the configured build directory and run help/invalid-option checks plus a valid compile, verifier, and runtime smoke case using the recipe in `ets2panda/AGENTS.md`. For compilation flow changes, run relevant static parser/runtime and public/plugin tests; CLI success alone does not establish other API callers still work.

A change is done only when intended behavior is implemented, the diff is scoped, relevant checks have run or their absence is explicit, and compatibility/consumer risks are reported. Never describe source inspection or a build-only check as passing runtime tests.
