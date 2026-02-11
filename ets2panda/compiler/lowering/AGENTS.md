# Compiler Lowering Component

## Core Metadata

| Attribute | Value |
|-----------|--------|
| **Name** | Compiler Lowering |
| **Purpose** | Transforms **high-level syntax** into **low-level syntax**; both input and output are **AST trees** (AST → AST). Desugaring (e.g. syntax sugar, lambdas) is implemented here. Phases are split into **pre-checker** and **post-checker** phases. |
| **Primary Language** | C++ |

## Directory Layout

```
compiler/lowering/
├── phase.*, checkerPhase.*, plugin_phase.*   # Phase framework and plugins
├── resolveIdentifiers.*, scopesInit/        # Identifier resolution and scope initialization
├── util.*, util-inl.h, phase_id.h          # Utilities and phase IDs
└── ets/               # ETS-specific lowering
    ├── *Lowering.cpp/h    # Literals, lambda, enum, optional, rest/spread, boxing, etc.
    ├── *Phase.cpp/h      # CFG build, destructuring, decl gen, primitive conversion, etc.
    ├── topLevelStmts/    # Top-level statement handling
    └── Other helpers (capturedVariables, boxingForLocals, dynamicImport, etc.)
```

## Ways to Create AST Nodes

| Method | Description | Typical use |
|--------|--------------|-------------|
| **ArenaAllocator::New\<T\>(...)** | Allocate node via static allocator API (no allocator instance needed) | `ArenaAllocator::New<ir::Identifier>(...)`, `ArenaAllocator::New<ir::OpaqueTypeNode>(...)`; e.g. boxingForLocals, restArgsLowering |
| **ctx->AllocNode\<T\>(...) / checker->AllocNode\<T\>(...)** | Allocate via Context/Checker allocator | arrayLiteralLowering, enumPostCheckLowering, fixedarrayLowering (OpaqueTypeNode, BlockExpression, etc.) |
| **parser->CreateFormattedExpression(str, nodes) / CreateFormattedStatement** | Template string → AST: source snippet with placeholders (e.g. `"let @@I1 = new Array<@@T2>(@@E3);"`), `@@I`/`@@T`/`@@E` for identifier/type/expression; parser parses to AST | arrayLiteralLowering, objectLiteralLowering, restArgsLowering, defaultParametersLowering, fixedarrayLowering |
| **Gensym(allocator)** | Unique temporary identifier node (`ir::Identifier`), prefix `gensym%%_` | Use with CreateFormatted* as temp names; see util.h/util.cpp |

Prefer **CreateFormatted*** + **Gensym** for full statements/expressions; use **AllocNode** / **ArenaAllocator::New** for single nodes or small trees. Set `SetParent`/`SetRange` on new nodes, then re-bind/re-check as needed (see below).

## Re-binding and Re-checking New Nodes After Checker

Post-checker phases that **create new AST nodes** must run **scope setup, identifier binding, and type checking** on them. Options (all in `compiler/lowering/util.h` / `util.cpp`):

| Method | When to use | Steps (summary) |
|--------|-------------|----------------|
| **CheckLoweredNode(varBinder, checker, node)** | **Brand-new subtree** (no existing type/binding); full bind + check | RefineSourceRanges → RunExternalNode → ResolveReferencesForScopeWithContext → set SavedCheckerContext/ScopeContext → node->Check(checker). No ClearTypesVariablesAndScopes. |
| **BindLoweredNode(varBinder, node)** | Binding only, no type check | RefineSourceRanges → RunExternalNode → ResolveReferencesForScopeWithContext. |
| **Manual**: scope + RunExternalNode + ResolveReferences + Check | Custom checker state or scope entry | LexicalScope::Enter; RunExternalNode; ResolveReferencesForScope or WithContext; set SavedCheckerContext/ScopeContext; node->Check(checker). See objectLiteralLowering, lambdaLowering. |
| **Rebind(phaseManager, varBinder, node)** | **Existing node**: clear then re-bind (for Program: CleanUp + RebindPhases) | Program: clear externals, ClearHelper, varBinder->CleanUp, RebindPhases. Else: ClearTypesVariablesAndScopes → RunExternalNode → ResolveReferencesForScopeWithContext. Returns scope. |
| **Recheck(phaseManager, varBinder, checker, node)** | **Existing node**: full re-bind + re-check (subtree rewritten) | RefineSourceRanges; if Program then RecheckProgram; else Rebind then set checker context and node->Check(checker). See declGenPhase, spreadLowering, lateInitialization, destructuringPhase. |

**RunExternalNode** builds scope for a new subtree; **ResolveReferencesForScope(WithContext)** resolves identifiers in that subtree; before **Check**, set correct Scope and CheckerStatus (e.g. IN_CLASS) via SavedCheckerContext/ScopeContext.

## Phase: Core AST Transformation Mechanism

- **Phase**: Each phase is an **AST transformer**: input AST → (transform) → output AST. An ordered sequence of phases lowers high-level AST to a lower-level form.
- **Execution**: Precondition → Perform → Postcondition. Ensures the phase can run and its output is valid for the next phase or compiler/core.
- **Timing**: **Pre-checker phases** (scope/identifier setup, some desugaring) and **post-checker phases** (code injection, restructuring, CFG using type info).
- **By role**: Core (TopLevelStatements, InitScopesPhaseETS, ResolveIdentifiers, CheckerPhase); desugaring (DefaultParametersLowering, OptionalLowering, SpreadConstructionPhase, …); code injection (ObjectLiteralLowering, UnboxPhase, UnionLowering, …); restructuring (LambdaConversionPhase, AsyncMethodLowering, GenericBridgesPhase, …).
- **Plugins**: plugins-after-parse, plugins-after-bind, plugins-after-check, plugins-after-lowering.
- **Debug helper**: `--dump-ets-src-before-phases=PhaseId`, `--dump-ets-src-after-phases=PhaseId`; 

### Phase Quick Reference

**1. Core**: TopLevelStatements, InitScopesPhaseETS, ResolveIdentifiers, CheckerPhase.  
**2. Desugaring**: DefaultParametersLowering, OptionalLowering, OpAssignmentLowering, SpreadConstructionPhase, BinaryExpressionLowering, OptionalArgumentsLowering, RestArgsLowering.  
**3. Code injection**: UnboxPhase, BoxingForLocals, ArrayLiteralLowering, ObjectLiteralLowering, InterfaceObjectLiteralLowering, UnionLowering.  
**4. Restructuring**: LambdaConversionPhase, AsyncMethodLowering, EnumLoweringPhase, GenericBridgesPhase.  
**5. Special/optimization**: ConstantExpressionLowering, ResizableArrayConvert, StringConstantsLowering, DynamicImport, RelaxedAnyLoweringPhase, RestTupleConstructionPhase.  
**6. Language features**: ObjectIndexLowering, ObjectIteratorLowering, LateInitializationConvert.

## Responsibilities

- **Phase ordering**: Pre-checker and post-checker phases; order and dependencies via phase/checkerPhase.
- **High-level → low-level**: AST-to-AST transforms (desugaring, lambda, literals, rest/spread, optional args, enum, record, generic bridges).
- **CFG and scope**: Post-checker phases (cfgBuilderPhase, scopesInit, savedBindingsCtx) feed compiler/core for register allocation and emission.

## Dependencies

- **Used by**: compiler/core.
- **Depends on**: checker, varbinder, ir (AST), util.

## Extending or Modifying

- **New phase**: Add `*Phase.cpp` under `ets/` or this dir; implement Precondition/Perform/Postcondition; register and set order (pre- vs post-checker).
- **New syntax transform**: Implement AST→AST in the right `*Lowering.cpp` or in a phase’s Perform; keep consistency with checker type information.
