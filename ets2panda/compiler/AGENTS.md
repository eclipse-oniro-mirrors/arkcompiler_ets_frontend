# compiler Agent Guide

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Scope

Routes static frontend work between lowering, bytecode generation, shared helpers, instruction templates, and metadata emission.

All paths below are repository-relative. Use the repository `AGENTS.md` and `ets2panda/AGENTS.md` with this guide.

## 2. Code Map

| Path | Responsibility | Risk |
|---|---|---|
| `ets2panda/compiler/lowering/` | AST-to-AST phases | High: compiler/consumer contract |
| `ets2panda/compiler/core/` | Compilation, registers, instruction emission | High: compiler/consumer contract |
| `ets2panda/compiler/templates/` | Generated instruction source templates | High: compiler/consumer contract |
| `ets2panda/compiler/base/` | Compilation helpers | High: compiler/consumer contract |
| `ets2panda/compiler/function/` | Function builders | High: compiler/consumer contract |
| `ets2panda/compiler/metadata/` | Metadata emission | High: compiler/consumer contract |

## 3. Knowledge Routing

| Task | Read first |
|---|---|
| Structural feature transform | `ets2panda/compiler/lowering/AGENTS.md` |
| Instruction/register/codegen change | `ets2panda/compiler/core/AGENTS.md` |
| Pipeline contract | `docs/knowledge/ets2panda/Static_Frontend_Knowledge_Base.md` |

## 4. Constraints

### 4.1 Do not

- Do not repair parser/checker semantics by injecting compensating codegen behavior.
- Keep structural transforms in the appropriate lowering phase; preserve ordering and body/declaration coverage.
- Rebind/recheck newly created or rewritten post-checker subtrees using the existing lowering helpers.
- Preserve evaluation order, exception paths, accumulator types, register lifetimes, and metadata.
- Do not hand-edit generated instructions; update templates/schema and rebuild.
- Inspect runtime ISA and verifier consumers before changing bytecode conventions.

### 4.2 Ask before

Follow repository approval gates for public/binding interfaces, serialized formats, dependencies, and legacy compatibility. Notify the user before changes that may alter emitted bytecode.

## 5. Build and Verification

From the configured static build directory, compile and link the affected frontend/library targets. Lowering changes need the nearby cases in `ets2panda/test/unit/lowerings/`, AST validity checks, and affected runtime cases. Codegen changes additionally need matching runtime/verifier and bytecode/target-version comparisons. Use the core/lowering guides for component-specific checks; a dump comparison does not replace execution.

A change is done only when intended behavior is implemented, the diff is scoped, relevant checks have run or their absence is explicit, and compatibility/consumer risks are reported. Never describe source inspection or a build-only check as passing runtime tests.
