# Checker Component

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

- **The checker must not transform the AST tree** (e.g. add/remove/replace nodes or change tree shape). **Node contents** may be updated (e.g. set fields, flags).
- Legacy code in the checker that mutates the AST tree is scheduled for refactor/removal. Do not add or keep AST tree transformations in the checker.

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
- **Type checking**: `typeChecker/` and **ETSChecker** validate and infer types for expressions, statements, and declarations; node contents may be updated but the AST tree is not transformed.
- **ETS semantics**: Boxing/unboxing, widening, arithmetic, assignment analysis, reachability, warnings in `ets/`; again, node contents may be updated, no tree transform.

## Dependencies

- **Used by**: compiler (lowering, core).
- **Depends on**: varbinder (binding results), parser/ir (AST), util.

## Extending or Modifying

- **New ETS type**: Add the type class under `types/ets/`, wire it into `typeRelation` and ETSChecker/ETS analysis.
- **New ETS check rule**: Add branches in ETSAnalyzer/ETSChecker or `ets/` visitors and report diagnostics; node content may be updated and checkerContext extended as needed. **Do not transform the AST tree in the checker.**
- **New AST node kind**: In **ETSAnalyzer.h** declare `Check(ir::NodeType *node)` via `AST_NODE_MAPPING`; implement the check in **ETSAnalyzer.cpp** and recurse via the node’s `Check(checker)`.
