# scripts Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Scope

Contains static frontend build/test/coverage helpers and application integration scripts.

All paths below are repository-relative. Use the repository `AGENTS.md` and `ets2panda/AGENTS.md` with this guide.

## 2. Code Map

| Path | Responsibility | Risk |
|---|---|---|
| `ets2panda/scripts/check_build_system_consistency.py` | GN/CMake source-list consistency helper | High: compiler/consumer contract |
| `ets2panda/scripts/test_runner.py` | Test runner helper | High: compiler/consumer contract |
| `ets2panda/scripts/run-tests` | Runner entry wrapper | High: compiler/consumer contract |
| `ets2panda/scripts/es2panda_coverage.sh` | Coverage orchestration | High: compiler/consumer contract |
| `ets2panda/scripts/es2panda_pre_test.py` | Pre-test support | High: compiler/consumer contract |
| `ets2panda/scripts/arkui-setup.sh` | Integration setup | High: compiler/consumer contract |

## 3. Knowledge Routing

| Task | Read first |
|---|---|
| Static regression | `docs/knowledge/ets2panda/Static_Frontend_Knowledge_Base.md` |
| Test placement | `ets2panda/test/AGENTS.md` |
| Build integration | `ets2panda/cmake/AGENTS.md` |

## 4. Constraints

### 4.1 Do not

- Do not run setup/download/application scripts as passive validation without inspecting their side effects.
- Preserve subprocess failure status, quoting, working directories, and explicit configuration paths.
- Do not weaken skip lists or overwrite baselines to hide failures.
- Edit script/generator inputs rather than their outputs.
- Keep GN/CMake consistency checks aligned with their actual parser limitations; a textual consistency check does not replace compiling/linking.
- Do not commit local SDK, build, or tool paths.

### 4.2 Ask before

Follow repository approval gates for public/binding interfaces, serialized formats, dependencies, and legacy compatibility. Notify the user before changes that may alter emitted bytecode.

## 5. Build and Verification

Read the selected script's argument parser and caller before invocation. Run its focused help/argument checks when side-effect-free, then exercise the intended caller in a prepared build/test environment. For generated-source or build-list changes, compile/link the affected targets and nearest consumer. For application helpers, compare actual artifacts/results and report external SDK/runtime gaps.

A change is done only when intended behavior is implemented, the diff is scoped, relevant checks have run or their absence is explicit, and compatibility/consumer risks are reported. Never describe source inspection or a build-only check as passing runtime tests.
