# staexpr2abc Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Scope

Provides `ark_staexpr2abc`, a CLI that compiles a base64-encoded expression through the static public evaluate API and writes base64 ABC output.

All paths below are repository-relative. Use the repository `AGENTS.md` and `ets2panda/AGENTS.md` with this guide.

## 2. Code Map

| Path | Responsibility | Risk |
|---|---|---|
| `ets2panda/staexpr2abc/main.cpp` | Tool-only option filtering and EvaluateExpression call | High: compiler/consumer contract |
| `ets2panda/staexpr2abc/CMakeLists.txt` | ark_staexpr2abc target | High: compiler/consumer contract |
| `ets2panda/staexpr2abc/BUILD.gn` | GN build registration | High: compiler/consumer contract |
| `ets2panda/test/unit/plugin/test_evaluate_expression.cpp` | Evaluation API regression | High: compiler/consumer contract |

## 3. Knowledge Routing

| Task | Read first |
|---|---|
| Evaluation context/AST | `ets2panda/evaluate/AGENTS.md` |
| Public API ownership | `docs/knowledge/ets2panda/Public_Knowledge_Base.md` |
| Static semantics | `docs/knowledge/ets2panda/Static_Frontend_Knowledge_Base.md` |

## 4. Constraints

### 4.1 Do not

- Do not add expression semantics to argument filtering; use the owning evaluate/compiler stage.
- Preserve `--base64Input=` filtering, static-extension defaulting, and compiler-option forwarding.
- Keep successful stdout as the single-line base64 ABC payload and diagnostics on stderr.
- Preserve cleanup on failed config creation/evaluation and avoid double-finalization.
- Do not silently accept malformed input or convert evaluation failure into an empty successful result.

### 4.2 Ask before

Follow repository approval gates for public/binding interfaces, serialized formats, dependencies, and legacy compatibility. Notify the user before changes that may alter emitted bytecode.

## 5. Build and Verification

From the configured static build directory, build `ninja ark_staexpr2abc`. The fixture `ets2panda/test/unit/plugin/test_evaluate_expression.cpp` is currently commented out of plugin registration, so do not claim the plugin umbrella target exercises it. Check a valid expression and decode the resulting base64 payload; also check missing/malformed inputs, compiler errors, stderr, and nonzero exit status. Use the matching runtime/verifier for output behavior. Source inspection is not a recorded CLI execution.

A change is done only when intended behavior is implemented, the diff is scoped, relevant checks have run or their absence is explicit, and compatibility/consumer risks are reported. Never describe source inspection or a build-only check as passing runtime tests.
