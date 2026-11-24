# cmake Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Scope

Defines frontend CMake wrappers and coverage integration for static frontend builds.

All paths below are repository-relative. Use the repository `AGENTS.md` and `ets2panda/AGENTS.md` with this guide.

## 2. Code Map

| Path | Responsibility | Risk |
|---|---|---|
| `ets2panda/cmake/PandaCmakeFunctions.cmake` | Frontend executable/library wrappers | High: compiler/consumer contract |
| `ets2panda/cmake/coverage.cmake` | Coverage build/test orchestration | High: compiler/consumer contract |
| `ets2panda/CMakeLists.txt` | Consumers and source registration | High: compiler/consumer contract |

## 3. Knowledge Routing

| Task | Read first |
|---|---|
| Component/build constraints | `docs/knowledge/ets2panda/Static_Frontend_Knowledge_Base.md` |
| Compiler consumers | `ets2panda/compiler/AGENTS.md` |

## 4. Constraints

### 4.1 Do not

- Do not modify generated build files instead of CMake inputs.
- Preserve frontend wrapper behavior, rebuild dependencies, host branches, and target ownership.
- Keep source registration consistent with GN where both build routes consume the code.
- Check header availability, compilation, final link symbols, and downstream consumers independently.
- Do not use a coverage target as evidence that all relevant behavior tests passed.

### 4.2 Ask before

Follow repository approval gates for public/binding interfaces, serialized formats, dependencies, and legacy compatibility. Notify the user before changes that may alter emitted bytecode.

## 5. Build and Verification

Reconfigure the intended static frontend build, compile and link affected targets, then build the nearest downstream consumer. Run focused behavior tests after build-graph changes; check reproducibility of generation dependencies. Record host, compiler, standard library, linker, and untested branches as required by the root guide.

A change is done only when intended behavior is implemented, the diff is scoped, relevant checks have run or their absence is explicit, and compatibility/consumer risks are reported. Never describe source inspection or a build-only check as passing runtime tests.
