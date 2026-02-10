# VarBinder Component

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

## Dependencies

- **Used by**: checker, compiler (lowering, core).
- **Depends on**: parser/ir (AST), util.

## Extending or Modifying

- **New scope/decl/variable kind**: Extend SCOPE_TYPES, DECLARATION_KINDS, or VARIABLE_TYPES in **variableFlags.h**, implement the corresponding subclass in scope.h, declaration.h, or variable.h, and create/attach or register it in **ETSBinder**.
- **New binding rule**: Implement binding for the new AST node in **ETSBinder.cpp** (ResolveReference or the appropriate visitor).
