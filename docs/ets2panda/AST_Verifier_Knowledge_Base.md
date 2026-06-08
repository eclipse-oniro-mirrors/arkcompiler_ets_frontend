# ETS2Panda AST Verifier Knowledge Base

> v1.0 | 2026-06-01 | Scope: `ets_frontend/ets2panda/ast_verifier/`
> Not covered: parser AST definitions (see `../parser/`), lowering (see `../lowering/`), checker type logic (see `../checker/`), codegen (see `../compiler/`).
> Read before modifying: `ast_verifier/AGENTS.md`, `ast_verifier/README.md`.

## Core Model

**Read-only. Debug-only. Phase-string-driven.** A validation gate between lowering and codegen. Never mutates AST. Runs only in debug builds. Invariants activate progressively via exact phase string matching (`IntroduceNewInvariants("PhaseName")`).

- **Invoke**: `compiler/core/compilerImpl.cpp::RunVerifierAndPhases`; `IntroduceNewInvariants("PhaseName")` enables invariants progressively. Phase matching is by exact string.
- **Hard gate**: Destructor asserts `!HasErrors() && !HasWarnings()`. `After()` MUST be called to dump messages before destruction; skipped `After()` = assertion crash with zero diagnostic output.
- **Traversal**: `SinglePassVerifier` walks all nodes. Each invariant sees every node. Both `allowed_` (phase-gated) and `enabled_` (CLI-configured) must be true.
- **Dependency ordering**: `RequiredInvariants<ENUM, Deps...>` enforces at compile time that dependencies precede dependents in the registry.

**Activation groups** (see constructor + `IntroduceNewInvariants` calls in `compiler/core/compilerImpl.cpp` for the canonical list). Guardrail: invariants in Base group MUST NOT depend on types or scopes — those go in CheckerPhase / ScopesInitPhase groups. Dependencies are enforced at compile time via `RequiredInvariants<ENUM, Deps...>`.

## Constraints & Traps

### ASTVerifier (entry point)

| Constraint | Detail |
|---|---|
| Destructor is a hard gate | `After()` must be called before destruction. If skipped, assertion fires with zero diagnostic info. |
| Phase string matching is exact | Typos, renames, or reordering silently break activation. Verify against `compiler/lowering/phase.cpp::GetETSPhaseList()`. |
| `InvariantsRegistry` order == enum order | Mismatch = compile error. Both derived from `util/options.yaml`. |
| Traversal depends on `Iterate` | Custom `Iterate` that skips children = unverified children. `Iterate` that visits extra nodes = false positives. |

### Invariants (all `invariants/*`)

| Constraint | Detail |
|---|---|
| Must NOT self-iterate AST | `SinglePassVerifier` handles traversal. Self-iteration = O(N^2). |
| Must NOT call other invariants | Use `RequiredInvariants` + `Get<Dep>()`. Direct calls bypass `allowed_`/`enabled_` gating. |
| Must return `{CORRECT, CONTINUE}` for non-applicable nodes | Framework contract. Any other return breaks skip-tree logic. |
| Must never crash | Null-check before `As<T>()`, `TsType()`, `Variable()`, `Parent()`. |
| `SKIP_SUBTREE` scope: children only | Re-enables after subtree. Incorrect use silences child violations. |
| No per-node data structure allocation | All invariants run on every node. O(N) per-invariant = O(N^2) total. |

### Helpers (`helpers.h`/`helpers.cpp`)

| Function | Trap |
|---|---|
| `IsContainedIn` | Returns false silently on nullptr args |
| `TryGetLocalScopeVariable` | LOCAL flag does not guarantee valid scope |
| `ValidateVariableAccess` | Must check `HasFlag(PROPERTY)` first |
| `IsValidTypeForBinaryOp` | Must check `IsTyped()` first |
| `GetClassDefinitionType` / `GetTSInterfaceDeclarationType` | Returns nullptr for nodes not inside a class/interface |

## Anti-Patterns

| Anti-Pattern | Consequence | Fix |
|---|---|---|
| Disabling/weakening a check to pass a patch | Safety-net gap | Fix the AST producer |
| Type/scope-dependent invariant in Base group | Null crash | Use CheckerPhase / ScopesInitPhase |
| Invariant self-iterates AST children | O(N^2) | Rely on SinglePassVerifier |
| Enum+registry mismatch | Compile error | Add to both `options.yaml` + `ASTVerifier.h` |
| Typo in `IntroduceNewInvariants` phase name | Silent deactivation | Verify against `GetETSPhaseList()` |
| Exceptions masking producer bugs | Masks root cause | Fix upstream; mark TODO |
| Destructor fires without `After()` | Zero-info crash | Always call `After()` |

## Human Approval Required

- Disable/weaken an invariant (comment out, severity change, suppress default)
- Remove/weaken destructor assertions (`ES2PANDA_ASSERT`)
- Reorder `VerifierInvariants` enum without reordering registry
- Add non-canonical phase names to `IntroduceNewInvariants`
- Modify `SinglePassVerifier` traversal logic or `InvariantBase`/`InvariantsRegistryImpl` templates

## Adding a New Invariant — Minimum Commit Flow

1. Add enum entry in `util/options.yaml` (`ast-verifier` section) + assign to activation group.
2. Register in `ASTVerifier.h` (`InvariantsRegistry`) in same order as enum.
3. Create `invariants/YourCheck.h` + `.cpp`: inherit `InvariantBase`, implement `Check()`. Return `{CORRECT, CONTINUE}` for non-applicable nodes. Never self-iterate.
4. If reading another invariant's data, declare `RequiredInvariants<YOUR_ENUM, DepEnum>`.
5. Add to both `CMakeLists.txt` and `BUILD.gn`.
6. Add unit test: `test/unit/public/ast_verifier_yourcheck.cpp` (positive + negative case).
7. Verify phase name matches `compiler/lowering/phase.cpp::GetETSPhaseList()` exactly. Typo = silent deactivation.

Active invariant count is ~18 (may drift — check `invariants/` directory for current count).

## Pre-Modification Checklist

- [ ] Read `ast_verifier/AGENTS.md` and `ast_verifier/README.md`
- [ ] New invariant registered in all four: `options.yaml` (enum + group), `ASTVerifier.h` (registry), `CMakeLists.txt`, `BUILD.gn`
- [ ] Dependencies declared via `RequiredInvariants` if the invariant reads another's data
- [ ] Activation group matches data dependencies (no type checks before CheckerPhase, no scope checks before ScopesInitPhase)
- [ ] Returns `{CORRECT, CONTINUE}` for non-applicable nodes
- [ ] Phase name exact match to `compiler/lowering/phase.cpp::GetETSPhaseList()`
- [ ] Unit test with positive case (valid AST passes) and negative case (violation detected)

## Code Anchors

Read order for understanding the verifier: `compilerImpl.cpp` (invocation) → `ASTVerifier.h` (registry + traversal) → `invariantBase.h` (CheckDecision, RequiredInvariants) → `invariants/*.h` (individual checks).

| What | Where |
|---|---|
| Verifier invoke + activation | `compiler/core/compilerImpl.cpp` (`RunVerifierAndPhases`) |
| Main class (registry, traversal) | `ast_verifier/ASTVerifier.h`, `ASTVerifier.cpp` |
| Base types, CheckDecision, RequiredInvariants | `ast_verifier/invariantBase.h` |
| Invariant checks (19 classes, 18 active) | `ast_verifier/invariants/*.h`, `*.cpp` |
| Shared utility predicates | `ast_verifier/helpers.h`, `helpers.cpp` |
| CLI options, enum definitions | `util/options.yaml` (section `ast-verifier`) |
| Canonical phase names | `compiler/lowering/phase.cpp` (`GetETSPhaseList()`) |
| Unit tests (16 files) | `test/unit/public/ast_verifier_*.cpp` |
| Test base class | `test/utils/ast_verifier_test.h` |

## Build & Verification

Build (from repository root):
```sh
cmake --build <out_dir> -j8    # es2panda including ast_verifier
```

Test commands:
```sh
# AST checker / verifier unit tests
runtime_core/static_core/tests/tests-u-runner-2/runner.sh es2panda-verifier astchecker --extension=ets --load-runtimes=ets --force-generate --processes=all
# Parser tests
runtime_core/static_core/tests/tests-u-runner-2/runner.sh es2panda-verifier parser --extension=ets --load-runtimes=ets --force-generate --processes=all
# Runtime tests
runtime_core/static_core/tests/tests-u-runner-2/runner.sh es2panda-verifier ets-runtime --extension=ets --load-runtimes=ets --force-generate --processes=all --es2panda-args=--simultaneous=true
# CTS tests
runtime_core/static_core/tests/tests-u-runner-2/runner.sh es2panda-verifier ets-cts --extension=ets --load-runtimes=ets --force-generate --processes=all --es2panda-args=--simultaneous=true
```

New files must be added to both `CMakeLists.txt` and `BUILD.gn`.

## Symptom -> Check

| Symptom | Check |
|---|---|
| Verifier rejects valid AST after lowering | Isolate `--ast-verifier:phases=<Phase>`. New pattern not in `ExceptionsMatcher`. |
| Null dereference crash | Missing `Is<T>()` before `As<T>()`; null `TsType()`/`Variable()`/`Parent()`. |
| New invariant never fires | Node filter excludes target; `allowed_` false; wrong phase group. |
| `IntroduceNewInvariants` silent failure | Phase name mismatch. Verify against `GetETSPhaseList()`. |
| Messages leak between source files | `Init()` not called or override doesn't clear all state. |

## Related Documents

- `ets2panda/ast_verifier/AGENTS.md` -- development rules and directory-level constraints
- `ets2panda/ast_verifier/README.md` -- CLI options, adding-invariant guide
- `docs/ets2panda/Static_Frontend_Knowledge_Base.md` -- covers all upstream/downstream stages: AST nodes (IR), parser, checker (type data), lowering (AST producer), compiler core (runs after verifier)
- `AGENTS.md` -- build/test expectations and repository-level guidance
