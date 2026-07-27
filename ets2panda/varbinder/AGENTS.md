# VarBinder Agent Guide

Use this file for work under `varbinder/` together with the repository-level `AGENTS.md`.

## Core Metadata

| Attribute | Value |
|-----------|--------|
| **Name** | VarBinder |
| **Purpose** | Binds identifiers in the AST to declarations, builds scopes and variable/declaration maps, and provides symbol information for checker and compiler. Day-to-day changes apply only to **ETSBinder** and **ETS binding** logic. |
| **Primary Language** | C++ |

## In-Scope (Primary Modification Target)

- **Only ETSBinder**: In practice only **ETSBinder** and ETS-specific binding are modified; TSBinder, JSBinder, ASBinder, and TypedBinder are **out of scope**.

## Directory Layout

```
varbinder/
├── ETSBinder.*            # [in scope] ETS binder entry
├── varbinder.*            # Base (IdentifierAnalysis, ResolveReferences)
├── TypedBinder.*          # Shared base for TS/ETS
├── scope.*, variable.*, variableFlags.h   # Scopes, variables, type enums (SCOPE_TYPES, VARIABLE_TYPES, etc.)
├── declaration.*         # Decl hierarchy (DECLARATION_KINDS)
├── recordTable.*         # ETS record type table
├── *Binder.* (non-ETS)   # TSBinder, JSBinder, ASBinder — rarely modified
└── privateBinding.h, tsBinding.h, enumMemberResult.h   # Language-specific helpers
```

## Responsibilities

- **Name resolution**: Walk the AST, look up declarations in the appropriate Scope, bind identifiers to Variable/Decl, and write back to nodes.
- **Scope construction**: Create Scopes for blocks, functions, classes, modules; maintain parent-child relationships and variable/declaration registration.
- **ETS-specific**: RecordTable, package/module imports, dynamic import; **ResolveReferencesForScopeWithContext** is used by lowering to bind newly created nodes locally.

## Stdlib Foreign-Binding Sharing

Stdlib imports use **delegate mode**: `ETSBinder` builds a shared export map per stdlib program (`foreignExportCache_`) and merges it into `GlobalScope::mergedSharedForeign_` via `AddSharedForeign` instead of calling `InsertForeignBinding` for each symbol. `mergedSharedForeign_` is a single flat `ArenaUnorderedMap` — entries from all imported stdlib modules are merged into it on first `AddSharedForeign` call, so lookups remain O(1). `GlobalScope::FindLocal` checks `mergedSharedForeign_` on a local-binding miss; local bindings always win.

**Rule**: use `FindLocal(name, ResolveBindingOptions::ALL)` — not raw `Bindings()` scans — when searching `GlobalScope` for stdlib symbols.

## Hard Constraints

- **No AST shape mutation**: binder must not rewrite AST structure or perform tree transformations.
- **No ad-hoc workaround flags**: avoid introducing new state flags when existing scope/binding flow can express the behavior.
- **Lowering handoff**: structural fixes belong to lowering; binder should resolve declarations/references for the produced nodes.

## Dependencies

- **Used by**: checker, compiler (lowering, core).
- **Depends on**: parser/ir (AST), util.

## Extending or Modifying

- **New scope/decl/variable kind**: Extend SCOPE_TYPES, DECLARATION_KINDS, or VARIABLE_TYPES in **variableFlags.h**, implement the corresponding subclass in scope.h, declaration.h, or variable.h, and create/attach or register it in **ETSBinder**.
- **New binding rule**: Implement binding for the new AST node in **ETSBinder.cpp** (ResolveReference or the appropriate visitor).
