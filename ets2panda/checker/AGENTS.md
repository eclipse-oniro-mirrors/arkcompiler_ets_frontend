# Checker Agent Guide

Use this file for work under `checker/` together with the repository-level `AGENTS.md`.

## Core Metadata

| Attribute | Value |
|-----------|--------|
| **Name** | Checker |
| **Purpose** | Performs semantic analysis on the bound AST: type checking, type inference, conversions and type relations, and **ETS-specific** semantics (reachability, assignment, boxing, etc.), and reports semantic errors and warnings. Day-to-day work is limited to **ETSChecker** and **ETS semantics**. |
| **Primary Language** | C++ |

## In-Scope (Primary Modification Target)

- **Only ETSChecker**: The only checker entry modified in practice is **ETSChecker**; TSChecker, JSChecker, ASchecker are **out of scope**.
- **Only ETS semantics**: Semantic and type-related changes apply to the ETS path only; TS semantics and the corresponding files/directories are **out of scope**.

## Hard Constraint: No AST Tree Transformations

- **The checker must not transform the AST tree** (no add/remove/replace/reparent).
- **The checker must not allocate new AST nodes**.
- Checker updates are limited to semantic metadata: set node type info and resolved variable references.
- Legacy code in the checker that mutates the AST tree is scheduled for refactor/removal. Do not add or keep AST tree transformations in the checker.

## Type-Checking Rules (Spec-Aligned)

- Use `TypeRelation` APIs (`IsSupertypeOf`, `IsIdenticalTo`, ...) for subtyping/compatibility/conversion logic.
- Do not hardcode type names (for example `"escompat.Record"`) and do not compare types by pointer identity.
- Avoid `Is<SomeType>` checks when subtype semantics are involved; express the rule through `TypeRelation`.
- Avoid hard-coded AST-shape checks with `Is<SomeExpression>`/`Is<SomeStatement>` as semantic shortcuts.
  - Limited accepted patterns are context-specific and rare (for example call-site `a.b()` member-call shape, numeric operand handling where required by semantics).
- Do not introduce new checker state flags (`CheckerStatus`, `AstNodeFlags`, `ETSObjectFlags`, similar) as workaround logic; prefer relation/lowering-based designs.
- If spec and implementation/tests diverge for type compatibility, report the mismatch and keep behavior spec-first.

## Directory Layout

```
checker/
├── ETSchecker.*                    # [in scope] ETS checker entry
├── ETSAnalyzer.cpp, ETSAnalyzer.h  # [in scope] Per-node Check(...) entry; traverses via child Check() calls
├── ETSAnalyzerHelpers.*            # [in scope] ETSAnalyzer helpers
├── ETSAnalyzerUnreachable.cpp      # [in scope] Unreachable code analysis
├── ets/                 # [in scope] ETS-specific submodules (see ets/ list below)
├── types/               # Type representation and relations (ETS types in scope only)
│   ├── type.*, typeRelation.*, typeFlag.h, typeFacts.h, typeError.h, typeMapping.h
│   ├── ets/             # [in scope] ETS types (primitive, object, union, tuple, function, etc.)
│   ├── ts/              # Rarely modified
│   └── globalTypesHolder.*, signature.*
├── checkerContext.*     # Shared checker context
├── typeChecker/         # Type-checking core (TypeChecker)
├── checker.*            # Base (Checker abstraction)
├── *checker*.cpp (non-ETS)  # TSChecker, JSchecker, ASchecker — out of scope
├── *Analyzer*.cpp (TS)  # TSAnalyzer, TSAnalyzerUnreachable — out of scope
├── ts/                  # TS-specific — out of scope
└── resolveResult.h, SemanticAnalyzer.h
```

### ets/ Submodule Summary (In Scope)

| File | Role |
|------|------|      
| `function.cpp` | Function calls, overload/signature selection |
| `arithmetic.*` | Arithmetic expressions and operation types |
| `assignAnalyzer.*` | Assignment semantics and liveness |
| `aliveAnalyzer.*` | Liveness / reachability |
| `boxingConverter.*`, `unboxingConverter.*`, `wideningConverter.*` | Boxing, unboxing, widening |
| `typeConverter.*`, `conversion.*`, `typeCreation.*` | Type conversion and creation |
| `castingContext.*`, `typeRelationContext.*` | Cast and type-relation context |
| `object.cpp` | Object type / literal checking |
| `etsWarningAnalyzer.*` | ETS warning analysis |
| `helpers.cpp`, `typeCheckingHelpers.cpp`, `validateHelpers.cpp` | General / type-check / validation helpers |
| `utilityTypeHandlers.cpp` | Utility types (e.g. Partial, ReturnType) |

## Check Flow and Entry Points

- **Per-node entry**: Each AST node is checked from **ETSAnalyzer.cpp**; the tree is traversed by calling the **Check** method on child nodes.
- **Function-related logic**: Files under `ets/` named `*function*` (e.g. `function.cpp`) handle function calls and call signature selection.

## Responsibilities

- **Type system**: ETS type hierarchy and relations in `types/`; ETS-related subtyping and assignment compatibility in `typeRelation`.
- **Type checking**: `typeChecker/` and **ETSChecker** validate and infer types for expressions, statements, and declarations; checker writes semantic metadata but does not transform AST shape.
- **ETS semantics**: Boxing/unboxing, widening, arithmetic, assignment analysis, reachability, warnings in `ets/`; keep implementations relation-based and avoid AST rewrites.

## Dependencies

- **Used by**: compiler (lowering, core).
- **Depends on**: varbinder (binding results), parser/ir (AST), util.

## Extending or Modifying

- **New ETS type**: Add the type class under `types/ets/`, wire it into `typeRelation` and ETSChecker/ETS analysis.
- **New ETS check rule**: Add branches in ETSAnalyzer/ETSChecker or `ets/` visitors and report diagnostics. Checker-side updates must stay metadata-only (types/variables), with no AST shape changes.
- **Stdlib symbol lookup**: use `TopScope()->FindLocal(name, ResolveBindingOptions::ALL)` — not `Bindings().find()` — stdlib symbols may live in shared foreign maps rather than the scope's own bindings.
- **New AST node kind**: In **ETSAnalyzer.h** declare `Check(ir::NodeType *node)` via `AST_NODE_MAPPING`; implement the check in **ETSAnalyzer.cpp** and recurse via the node’s `Check(checker)`.
