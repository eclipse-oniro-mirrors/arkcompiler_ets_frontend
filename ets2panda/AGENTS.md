# ets2panda Frontend Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

This file defines repository-wide rules for AI/code agents working in `ets2panda`.
Use it together with the closest component-level `AGENTS.md`; when rules differ, the closest component guide applies for that scope.

## 1. Spec-First Policy

- Latest technical preview release feed: <https://gitcode.com/igelhaus/arkcompiler_runtime_core/releases/>.
- For frontend changes, a behavior is considered "in spec" only if it is described in the latest technical preview specification.
- Tests, apps, and legacy behavior are not accepted as rationale when they conflict with the spec.
- If spec and implementation diverge, report the inconsistency and avoid speculative behavior.

## 2. Hard Review Gates

The patch is blocked if any of the following is true:

- No tests are included for behavior changes.
- The patch implements a feature that is not in the spec.
- Existing assertions are removed (`ES2PAND_ASSERT`, `arktest.assert*`, similar).

## 3. Reviewer Checklist

- Keep frontend behavior strictly aligned with the spec; avoid introducing ad-hoc compile-time errors beyond spec-defined behavior.
- Avoid hard-coded AST/type pattern checks (`Is<Node>`, `Is<Type>`) unless a spec rule explicitly requires them.
- Prefer file-local `static`/private helpers over growing large public class interfaces.
- Avoid introducing new state flags and workaround enums when a type-relation or lowering-based design is possible.
- Include tests that fully cover each behavior change.

## 4. Core Engineering Rules

- Do not manage memory with explicit `new` / `delete`.
  - Use `std::*` containers for local structures.
  - Use arena allocation for compiler-owned objects.
- Do not bloat public interfaces when a file-local `static` helper is enough.
- Do not hardcode type names (`"escompat.Record"`, etc.) or compare types by pointers.
- Use `TypeRelation` APIs (`IsSupertypeOf`, `IsIdenticalTo`, ...) for type logic.
- Do not introduce new state flags (`CheckerStatus`, `AstNodeFlags`, `ETSObjectFlags`, ...).
- Treat frontend structures as immutable unless explicitly designed otherwise.
  - Checker and binder must not mutate AST shape.
  - Checker may only set type/variable metadata and must not create or rewrite AST structure.
  - Structural changes belong in lowering phases.
- Plugin API changes must stay minimal and intentional.
  - Avoid bloating plugin API when existing API can express the feature.
  - Any change in `public/es2panda_lib.h` must be accompanied by corresponding updates in `public/es2panda_lib.idl.erb`.

## 5. Debugging Aids

- AST dump: `node->DumpEtsSrc()`.
- Type dump: `type->ToString()`.
- Signature dump: `sig->ToString()`.
- Dump source after a specific phase with `--dump-ets-src-after-phases=<PhaseName>`.
- Useful local tools: `ark_disasm` and `ark_asm` (build with `ninja ark_disasm` / `ninja ark_asm`; roundtrip may be incomplete in some cases).

## 6. Build and Smoke Commands

Run from the build directory that contains `./bin`:

```bash
./bin/es2panda --extension=ats --opt-level=0 --output=out.abc fault.ets
./bin/verifier --boot-panda-files=./plugins/ets/etsstdlib.abc --load-runtimes=ets out.abc
./bin/ark --boot-panda-files=./plugins/ets/etsstdlib.abc --load-runtimes=ets --panda-files=out.abc out.abc fault.ETSGLOBAL::main
```

Runner suite (from the configured static frontend build directory; validate path variables using `docs/knowledge/Build_Environment.md`):

```bash
# Set this parameter to the absolute OpenHarmony checkout root.
: "${OH_ROOT:?Set and validate OH_ROOT using Build_Environment.md}"
python3 "$OH_ROOT/arkcompiler/runtime_core/static_core/tests/tests-u-runner/main.py" --force-generate --ets-cts --build-dir .
```

Useful options: `--processes 6`, `--verbose short`.

## 7. Test Placement

- Negative compile-time diagnostics: `test/ast/`.
- Legacy parser tests (maintain only): `test/parser/ets/`.
- Runnable positive tests with `arktest.*` assertions: `test/runtime/ets/`.
- C++ import path manager and ArkTsConfig regressions: `test/unit/import_path_manager/`.

## 8. Test Design Notes

- Keep tests minimal and focused on one root cause.
- Add runnable positive tests for runtime/verifier regressions when the original bug manifested at runtime.
- Prefer `instanceof` for runtime type checks; use `typeof` only when the feature under test is `typeof` itself.
- Avoid test patterns that can be folded by compile-time smart-cast behavior; use helper functions/lambdas to isolate runtime behavior.

## 9. Component Guides

Use the closest `AGENTS.md` first. Paths in the following table are repository-relative.

| Area | Guide |
|---|---|
| `ets2panda/aot/` | `ets2panda/aot/AGENTS.md` |
| `ets2panda/arkts_bindings/` | `ets2panda/arkts_bindings/AGENTS.md` |
| `ets2panda/ast_verifier/` | `ets2panda/ast_verifier/AGENTS.md` |
| `ets2panda/bindings/` | `ets2panda/bindings/AGENTS.md` |
| `ets2panda/checker/` | `ets2panda/checker/AGENTS.md` |
| `ets2panda/cmake/` | `ets2panda/cmake/AGENTS.md` |
| `ets2panda/compiler/` | `ets2panda/compiler/AGENTS.md` |
| `ets2panda/declgen_ets2ts/` | `ets2panda/declgen_ets2ts/AGENTS.md` |
| `ets2panda/docs/` | `ets2panda/docs/AGENTS.md` |
| `ets2panda/driver/` | `ets2panda/driver/AGENTS.md` |
| `ets2panda/evaluate/` | `ets2panda/evaluate/AGENTS.md` |
| `ets2panda/ir/` | `ets2panda/ir/AGENTS.md` |
| `ets2panda/lexer/` | `ets2panda/lexer/AGENTS.md` |
| `ets2panda/linter/` | `ets2panda/linter/AGENTS.md` |
| `ets2panda/lsp/` | `ets2panda/lsp/AGENTS.md` |
| `ets2panda/parser/` | `ets2panda/parser/AGENTS.md` |
| `ets2panda/public/` | `ets2panda/public/AGENTS.md` |
| `ets2panda/scripts/` | `ets2panda/scripts/AGENTS.md` |
| `ets2panda/staexpr2abc/` | `ets2panda/staexpr2abc/AGENTS.md` |
| `ets2panda/test/` | `ets2panda/test/AGENTS.md` |
| `ets2panda/util/` | `ets2panda/util/AGENTS.md` |
| `ets2panda/varbinder/` | `ets2panda/varbinder/AGENTS.md` |
| Lowering | `ets2panda/compiler/lowering/AGENTS.md` |
| Compiler core | `ets2panda/compiler/core/AGENTS.md` |

Full nested/companion inventory: `docs/knowledge/Guide_Index.md`.

## 10. Additional Docs

- Onboarding guide: `docs/frontend-onboarding.md`
- Documentation index: `docs/README.md`
