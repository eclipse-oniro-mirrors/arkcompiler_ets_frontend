# Frontend Tests Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

Use this file for work under `test/` together with the repository-level `AGENTS.md`.

## 1. Core Metadata

| Attribute | Value |
|-----------|--------|
| **Name** | Frontend Tests |
| **Purpose** | Covers parser, compile-time diagnostics, runtime behavior, and regressions for frontend changes. |
| **Primary Language** | ETS test sources and Python runner tooling |

## 2. Test Placement Rules

- `test/ast/`: negative compile-time diagnostics and warning checks.
- `test/parser/ets/`: legacy parser tests; maintain existing tests, avoid extending for new behavior.
- `test/runtime/ets/`: runnable positive tests; must compile, verify, and execute.

## 3. Mandatory Rules

- Every behavior change must include tests.
- Include minimal bug-report snippets with the fix whenever available.
- Keep tests minimal and focused to one root cause.
- Split tests when multiple error causes exist.
- Minimize fuzz-derived repros before landing.
- Negative tests must include expected diagnostics (`expected.err`).
- Runnable tests must use `arktest.*` assertions.
- Positive tests must compile, verify, and execute (compile-only is for negative tests).
- Prefer `instanceof` for runtime type checks; use `typeof` only when validating `typeof` feature semantics.

## 4. Runtime-Error Repro Policy

If the original bug caused a runtime exception and the exception is still reachable, add a runnable test that reproduces and asserts that exception (for example `ClassCastError`, `NullPointerError`, `AbstractMethodError`).

## 5. Smart-Cast Testing Guidance

- Avoid tests that can be resolved purely at compile time.
- Use helper functions/lambdas to isolate runtime conversions and avoid hidden smart-cast shortcuts.

## 6. QA Suite Coordination

- Frontend contributors usually add tests in `test/ast` and `test/runtime/ets`.
- For QA-owned suites (CTS / functional suites), frontend contributors typically update skip-lists or enablement status rather than adding direct tests there.

## 7. Helpful Runner Command

From the configured static frontend build directory, after validating variables with `docs/knowledge/Build_Environment.md`:

```bash
# Set this parameter to the absolute OpenHarmony checkout root.
: "${OH_ROOT:?Set and validate OH_ROOT using Build_Environment.md}"
python3 "$OH_ROOT/arkcompiler/runtime_core/static_core/tests/tests-u-runner/main.py" --force-generate --ets-cts --build-dir .
```

Common options: `--processes 6`, `--verbose short`.
