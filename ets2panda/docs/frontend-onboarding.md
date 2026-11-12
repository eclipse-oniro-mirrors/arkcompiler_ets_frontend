# ArkTS Frontend Onboarding

This document summarizes day-to-day onboarding guidance for contributors working in `ets2panda`.

## Specification Source of Truth

- Latest technical preview release: <https://gitcode.com/igelhaus/arkcompiler_runtime_core/releases/>
- Frontend changes must follow the latest technical preview specification.
- Tests, applications, and legacy implementation are not the source of truth when they conflict with the spec.

## Build, Run, and Verify

Run from a build directory that contains `./bin`:

```bash
./bin/es2panda --extension=ets --opt-level=0 --output=out.abc fault.ats
./bin/verifier --boot-panda-files=./plugins/ets/etsstdlib.abc --load-runtimes=ets out.abc
./bin/ark --boot-panda-files=./plugins/ets/etsstdlib.abc --load-runtimes=ets --panda-files=out.abc out.abc fault.ETSGLOBAL::main
```

Run tests-u-runner suites from `static_core`:

```bash
static_core/tests/tests-u-runner/main.py --force-generate --ets-cts --build-dir .
```

Useful options:

- `--processes 6`
- `--verbose short`

## Debugging Aids

### AST and Type Dumps

- AST node source dump: `node->DumpEtsSrc()`
- Type dump: `type->ToString()`
- Signature dump: `sig->ToString()`
- Dump program source after a phase: `--dump-ets-src-after-phases=PhaseName`

### gdb Helpers

Put this in `~/.config/gdb/gdbinit`:

```gdb
define pj
    printf "%s\n", (char*)(($arg0) ? ($arg0)->DumpJSON() : "**null**")
end
define ps
    printf "%s\n", (char*)(($arg0) ? ($arg0)->DumpEtsSrc() : "**null**")
end
define pt
    printf "%s\n", (char*)(($arg0) ? ((ark::es2panda::checker::Type*)($arg0))->ToString() : "**null**")
end
define psig
    printf "%s\n", (char*)(($arg0) ? ((ark::es2panda::checker::Signature*)($arg0))->ToString() : "**null**")
end
```

### Assembler/Disassembler

- `ark_disasm` dumps bytecode from `.abc`
- `ark_asm` generates `.abc` from text dump
- Build tools with `ninja ark_disasm` and `ninja ark_asm`
- Note: `ark_asm` cannot always roundtrip `ark_disasm` output

### TypeScript AST Viewer

- <https://ts-ast-viewer.com>

## Frontend Development Rules

### Hard gates (review blockers)

- Behavior changes without tests.
- Implementing features not present in spec.
- Removing assertions (`ES2PAND_ASSERT`, `arktest.assert*`, etc.).

### Mandatory engineering rules

- Do not use explicit `new`/`delete` for manual memory management.
  - Local temporary containers: use `std::*`.
  - Compiler-owned objects: use arena allocation.
- Do not bloat public interfaces; prefer file-local `static` helpers where possible.
- Do not hardcode type names (`"escompat.Record"`, `"escompat.Array"`, etc.).
- Do not compare types by raw pointers.
- Use `TypeRelation` (`IsSupertypeOf`, `IsIdenticalTo`, ...) for type logic.
- Do not introduce new flags (`CheckerStatus`, `AstNodeFlags`, `ETSObjectFlags`, similar).
- Do not mutate AST shape in checker/binder.
  - Checker may set node type and variable metadata only.
  - Structural transformation belongs to lowering.

## Type Checker Rules

- Checker must not allocate new AST nodes.
- Checker must not rewrite AST structure.
- Avoid hardcoded `Is<SomeExpression>`/`Is<SomeStatement>` pattern checks for semantic logic.
  - Limited exceptions include call-site `a.b()` member-call pattern and numeric operand handling in binary operators.
- Avoid `Is<SomeType>` checks when subtyping is involved; use `TypeRelation`.
- Avoid narrow special-casing (`IsETSObjectType`, `IsETSUnionType`, etc.) where general type-relation or type-construction logic is required.

## Lowering Rules

- Add a lowering for features best modeled as code transformation.
- Define preconditions and postconditions for lowering phases.
- Use AST traversal/transformation APIs (`Iterate*`, `TransformChildren*`).
- Create nodes with allocator helpers (`AllocNode<T>()`).
- Rebind and re-check lowered subtrees instead of manually assigning scope/type internals.

## AST Verifier Rules

- Never disable AST verifier checks during development.
- If a new feature adds AST node kinds or lowering patterns, add corresponding verifier checks.

## Testing Rules

- Add bug-report snippets with fixes.
- Keep tests minimal and focused to one failure cause.
- Split tests when multiple bugs are mixed.
- Negative tests must include expected diagnostics (`expected.err`) and be compile-time focused.
- Positive tests must compile, verify, and execute.
- Runtime behavior must be checked with `arktest.*` assertions.
- Prefer `instanceof` for runtime type checks; use `typeof` only when testing `typeof` semantics itself.

### Runtime-error regressions

If the original bug produced a runtime exception (for example `ClassCastError`, `NullPointerError`, `AbstractMethodError`), the added runnable test should reproduce and assert that runtime error when still applicable.

### Bypassing smart-cast side effects in tests

- Avoid writing tests that can be fully folded by compile-time smart typing.
- Wrap scenarios in helper functions/lambdas to isolate conversions and runtime behavior.

## Test Placement

For frontend contributors:

- `test/ast/`: negative compile-time diagnostics and warnings.
- `test/parser/ets/`: legacy parser tests (maintain only, do not extend for new behavior).
- `test/runtime/ets/`: runnable positive tests with `arktest.*` assertions.

## Architecture Overview

```text
Lexer + Parser + Binder
          |
          v
        AST
          |
          v
Checker + Lowerings
          |
          v
     Checked AST
          |
          v
    Code Generator
          |
          v
      bytecode (.abc)
```

Component locations:

- Parser: `parser/`
- Binder: `varbinder/`
- Checker: `checker/`
- Lowerings: `compiler/lowering/`
- AST verifier: `ast_verifier/`
- Code generation and register allocation: `compiler/core/`
- Tests: `test/`

## Related Docs

- Repo-level rules: `AGENTS.md`
- Component-level guides: `*/AGENTS.md`
- Lowering details: `docs/lowering-phases.md`
- Import/export design: `docs/import_export.md`
