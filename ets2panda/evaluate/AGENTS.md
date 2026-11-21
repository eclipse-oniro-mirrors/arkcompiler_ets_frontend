# evaluate Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Scope

Implements debugger expression compilation support, debug-info reconstruction, context/scopes, and expression AST adaptation.

All paths below are repository-relative. Use the repository `AGENTS.md` and `ets2panda/AGENTS.md` with this guide.

## 2. Code Map

| Path | Responsibility | Risk |
|---|---|---|
| `ets2panda/evaluate/evaluateContext.h` | Expression/source/debug context | High: compiler/consumer contract |
| `ets2panda/evaluate/expressionASTTransformer.cpp` | Expression transformation | High: compiler/consumer contract |
| `ets2panda/evaluate/entityDeclarator.cpp` | Debug-backed declarations | High: compiler/consumer contract |
| `ets2panda/evaluate/pathResolver.cpp` | Source/import paths | High: compiler/consumer contract |
| `ets2panda/evaluate/proxyProgramsCache.cpp` | Proxy program cache | High: compiler/consumer contract |
| `ets2panda/evaluate/debugInfoDeserialization/` | Debug-info reconstruction | High: compiler/consumer contract |

## 3. Knowledge Routing

| Task | Read first |
|---|---|
| Pipeline and scopes | `docs/knowledge/ets2panda/Static_Frontend_Knowledge_Base.md` |
| Expression CLI | `ets2panda/staexpr2abc/AGENTS.md` |
| API boundary | `docs/knowledge/ets2panda/Public_Knowledge_Base.md` |

## 4. Constraints

### 4.1 Do not

- Do not invent debugger runtime semantics from compiler-only evidence.
- Preserve source position, debug entity identity, declaration scope, and cache lifetime.
- Do not share one generated identifier node between declaration/write/read positions; follow existing transformer ownership conventions.
- Keep AST construction/adaptation in the evaluate/phase entry that owns it, not a general checker/binder rewrite.
- Follow the current phase ordering and rebind/recheck protocol for changed subtrees.
- Preserve diagnostics and resource cleanup on failed evaluation.

### 4.2 Ask before

Follow repository approval gates for public/binding interfaces, serialized formats, dependencies, and legacy compatibility. Notify the user before changes that may alter emitted bytecode.

## 5. Build and Verification

Build/link the affected static frontend/public library and `ark_staexpr2abc` from the configured static build directory. Inspect `ets2panda/test/unit/plugin/test_evaluate_expression.cpp` as a source fixture. Its registration in `ets2panda/test/unit/plugin/CMakeLists.txt` is currently commented out: running the plugin umbrella target does not execute it. Record this coverage gap, and validate expressions through the built CLI and matching runtime; any test-enablement change needs its own implementation validation. Compare valid/invalid expressions, generated ABC decoding, diagnostics, context cleanup, and execution in the matching debugger/runtime environment when behavior changes. Report any end-to-end debugger gap.

A change is done only when intended behavior is implemented, the diff is scoped, relevant checks have run or their absence is explicit, and compatibility/consumer risks are reported. Never describe source inspection or a build-only check as passing runtime tests.
