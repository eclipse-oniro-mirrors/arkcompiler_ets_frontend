# ETS2Panda Static Frontend Knowledge Base

> Document version: v1.0
> Last updated: 2026-06-01
> Scope: **Default document for all ets2panda parser, checker, varbinder, lowering, and compiler core tasks.** Covers IR (AST nodes), Lexer, Parser, VarBinder, Checker, Lowering, Compiler Core — per-stage hard boundaries, key constraints, anti-patterns, and code/test anchors.
> NOT covered: `es2panda` dynamic path (see `../ES2Panda_Knowledge_Base.md`); `runtime_core/`; AST Verifier (see `AST_Verifier_Knowledge_Base.md`); Public API (see `Public_Knowledge_Base.md`); LSP; DeclGen; Driver; Linter.
> Read before modifying: `AGENTS.md`, `ets2panda/AGENTS.md`, and the per-stage `AGENTS.md` under each modified directory.

## Pipeline Model

The ets2panda pipeline is strictly ordered:

```
IR (AST node definitions) → Lexer (text→tokens) → Parser (tokens→AST, allocates IR nodes) → VarBinder (scope+identifier resolution) → Checker (type checking, writes metadata) → Lowering (~75 AST-to-AST phases) → Compiler Core (AST→bytecode)
```

**Hard boundaries:**
- Checker and VarBinder MUST NOT allocate, remove, replace, or reparent AST nodes. Structural changes belong exclusively to lowering. This boundary has zero exceptions.
- Post-checker lowering MUST call `CheckLoweredNode`/`Recheck`/`Rebind` on every new or rewritten AST subtree. Omission causes null types and downstream crashes.
- NEVER fix upstream errors in downstream stages. Parser bugs in the parser; checker bugs in the checker; lowering bugs in lowering.
- NEVER introduce new AST node flags (`AstNodeFlags`). Adding flags bloats every AST node and affects all pipeline stages. Prefer type-relation-based or lowering-based designs.

**Why checker/varbinder must not mutate AST:** VarBinder's sole responsibility is identifier binding. Checker's sole responsibility is type checking. AST mutation is not their job — it belongs to lowering. Mixing structural changes into binding or type-checking code breaks the single-responsibility boundary and makes every stage harder to reason about, test, and verify independently.

**Conditional reads — only when triggered by the change type:**
- If your change introduces new AST shapes or the AST verifier reports failures after your change, read `AST_Verifier_Knowledge_Base.md`.
- If your change exposes a new C API or modifies the public plugin interface, read `Public_Knowledge_Base.md`.

---

## Per-Stage Guardrails

### 0. IR (AST Nodes)

`ets_frontend/ets2panda/ir/` | **ets2panda's own AST node definitions. NOT shared with es2panda.**

Role: Defines the AST node types that represent the program. All stages read or transform these nodes.

| Rule | Detail |
|---|---|
| All AST nodes via `AllocNode<T>()` (arena allocation) | NEVER use `new`/`delete` — breaks arena lifecycle |
| MUST call `SetParent` and `SetRange` on every new node | Missing = silent wrong scope resolution and source location errors |
| New node type MUST update ALL pipeline stages | Parser (construction), Checker/VarBinder (metadata, no mutation), Lowering (transforms), AST Verifier (invariants), Compiler Core (codegen dispatch), BOTH `CMakeLists.txt` and `BUILD.gn` |
| NEVER introduce new `AstNodeFlags` | Bloats every AST node. Prefer type-relation-based or lowering-based designs |
| Node class hierarchy changes affect `AST_NODE_MAPPING` | Missing macro update = linker errors for any stage that dispatches on node type |
| `ir/` is NOT shared infrastructure with es2panda | es2panda has its own `es2panda/ir/`. Do NOT assume a node in one exists in the other |

### 1. Lexer

`ets_frontend/ets2panda/lexer/` | **Only ETSLexer and ETS entries in keyword/token YAML are in scope.**

Role: Converts raw ETS source text into a token stream. The absolute front of the pipeline.

| Rule | Detail |
|---|---|
| NEVER hand-edit `generated/` files (`tokenType.h`, `keywords.h`, `token.inl`) | Update YAML + templates, then rebuild |
| ETS changes use `ETSLexer` ONLY | Do NOT modify TSLexer, ASLexer, or base `Lexer` for ETS features |
| `TokenType` enum ordering: NEVER reorder between `FIRST_KEYW`/`FIRST_PUNCTUATOR` sentinels | Shifts enum values used throughout parser/checker |
| keyword vs keyword_like: hard = reserved token type; soft = `LITERAL_IDENT` with `keywordType_` set | Confusing these causes syntax errors in valid code or allows keywords in invalid positions |
| `definable_type_name` = CANNOT be defined (name is misleading) | Means "already predefined, forbidden" |
| Do NOT add persistent state across `NextToken()` calls | Lexer is a stateless iterator |
| `Peek()` vs `PeekCp()`: byte vs code point | Wrong choice crashes on multi-byte text |
| Modifying `IsIdentifierStart()`/`IsIdentifierPart()` affects ALL language modes | TS/AS/JS all share these |

**Source of truth:** `scripts/keywords.yaml`, `scripts/tokens.yaml` (NOT the generated output).

### 2. Parser

`ets_frontend/ets2panda/parser/` | **Syntax only. No type checking, no symbol resolution.**

Role: Converts the token stream into an untyped AST. Enforces grammar; does NOT understand types or scopes.

| Rule | Detail |
|---|---|
| ETS-only changes use `ETSparser*.cpp` overrides | `parserImpl.*` affects ETS/TS/JS/AS -- MUST NOT add ETS-specific logic |
| NEVER manually manipulate `context_` or `classPrivateContext_` | Use RAII guards (`SavedParserContext`, `SavedClassPrivateContext`) |
| All AST nodes via `AllocNode<T>()` | NEVER use `new` -- breaks arena lifecycle |
| MUST call `SetParent` and `SetRange` on every new AST node | Missing = silent wrong scope resolution and source locations |
| `ClassPrivateContext::AddElement()` returns false on duplicate -- MUST report syntax error | Silent duplicate members |
| Only 7 `ParserStatus` flags are inherited in `ParserContext` constructor | Adding a flag that needs inheritance without updating the merge expression = flag silently lost |
| `InnerSourceParser` nesting is unsupported | Single saved lexer pointer, overwritten on nest |

### 3. VarBinder

`ets_frontend/ets2panda/varbinder/` | **ETSBinder only. Scope construction and identifier binding metadata. MUST NOT mutate AST.**

Role: Resolves every identifier to its declaration. Builds the scope tree and variable maps the checker consumes.

| Rule | Detail |
|---|---|
| ETSBinder is the ONLY binder to modify | NEVER add ETS code to `VarBinder` or `TypedBinder` |
| Override `HandleCustomNodes()`, NOT `ResolveReferences()` | Base `ResolveReferences()` skips ETS-specific dispatch |
| `BuildExternalProgram()` MUST complete for all dependencies before `BuildProgram()` | Imports resolve against empty scopes otherwise |
| `LexicalScope<T>` is the ONLY way to enter a new scope | NEVER assign `scope_` or `varScope_` directly |
| `RecordTableContext` RAII is REQUIRED to switch active RecordTable | NEVER assign `recordTable_` directly |
| `BoundContext` MUST wrap every class/interface/annotation body | Missing = silent RecordTable corruption |
| Post-lowering rebind MUST use `ResolveReferencesForScopeWithContext(node, scope)` | NOT `ResolveReferences()` directly |
| `CleanUp()` MUST be called before reusing binder for new `IdentifierAnalysis()` | Stale state produces corrupted bindings with no crash |
| `Scope::Find()` walks parent chain; `FindLocal()` does NOT | Use `FindLocal()` for redeclaration checks |

### 4. Checker

`ets_frontend/ets2panda/checker/` | **Hard boundary: AST is read-only. Writes type metadata only. NEVER allocate AST nodes.**

Role: Performs semantic analysis — type checking, type inference, subtype/assignability resolution. Writes type metadata onto AST nodes but never changes tree shape.

| Rule | Detail |
|---|---|
| All type compatibility MUST use `TypeRelation` APIs (`IsSupertypeOf`, `IsIdenticalTo`, `IsAssignableTo`) | NEVER compare `TypeFlag` values or pointer identity |
| Set `TypeRelationFlag` context BEFORE relation calls, RESTORE after | Missing `IN_ASSIGNMENT_CONTEXT`/`IN_CASTING_CONTEXT` = incorrect results |
| NEVER use `Is<TypeName>()` for subtype decisions | Exact flag identity test. Use `TypeRelation` APIs |
| NEVER `new` type objects directly | Use `ETSChecker` factory methods (deduplicate equivalent types for cache correctness) |
| Types must be immutable after construction | Cache key is `type->Id()`; mutating after caching = stale cache entries |
| `Check()` must return `checker::Type *`; never nullptr | Use `GlobalTypeError()` for error cases |
| Do NOT cache `Checker::Scope()` across scope boundaries | Stale scope pointer resolves lookups against wrong scope |
| Do NOT introduce new `CheckerStatus` flags | Prefer relation/lowering-based designs |
| Smart cast maps: use `CloneSmartCasts()`/`RestoreSmartCasts()`/`CombineSmartCasts()` | NEVER manually copy map entries |
| Global types are shared singletons -- NEVER mutate | Mutating corrupts all checkers |

### 5. Lowering

`ets_frontend/ets2panda/compiler/lowering/` | **~75 ordered AST-to-AST phases. Source of truth: `GetETSPhaseList()` in `phase.cpp`.**

Role: The ONLY stage permitted to mutate AST structure. Desugars language features into simpler forms the codegen can emit.

| Rule | Detail |
|---|---|
| Post-checker phases MUST call `CheckLoweredNode` or `Recheck` on every created/rewritten AST subtree | Omission = null types in downstream phases |
| Every new AST node MUST have `SetParent` and `SetRange` called before any scope/type operation | Missing `SetParent` = `NearestScope` silently returns wrong scope |
| `pluginsAfterLowerings` MUST be the last phase | Do NOT insert phases after it |
| NEVER use `PhaseForProgramsWithBodies_LEGACY` | Misleading alias; banned in new code |
| `ClearTypesVariablesAndScopes` ONLY inside controlled Rebind/Recheck flow | No recovery path; causes downstream crashes |
| `Gensym` uses global static counter | MUST call `ResetGenSymCounter()` between tests |
| `RunExternalNode`: varbinder scope MUST point to parent scope before call | Not enforced by signature |
| New `.cpp` MUST register in both CMakeLists.txt and BUILD.gn | Missing one = build break on the other system |
| Do NOT reorder phases without explicit task requirement | Requires full upstream/downstream dependency understanding |
| `CreateFormatted*` placeholders (`@@I`/`@@T`/`@@E`) must match variadic arg types and counts | Mismatch = nullptr or crash |

### 6. Compiler Core

`ets_frontend/ets2panda/compiler/core/` | **Stable endpoint. Lowered AST in, bytecode out. LOW modification frequency.**

Role: Translates the fully-lowered AST into Panda bytecode. Read-only on AST; produces assembly instructions for the Ark runtime.

| Rule | Detail |
|---|---|
| `ETSGen`/`ETSCompiler`/`ETSEmitter` are the ONLY ArkTS paths | NEVER add ArkTS code to `PandaGen`/`JSCompiler`/`JSemitter` (legacy) |
| `SetAccumulatorType()` after every load sequence | `LoadAccumulator*` does NOT set it. Stale type = silent wrong bytecode |
| Use `TargetTypeContext` RAII -- NEVER manually save/restore `targetType_` | Stack-only (`operator new` deleted) |
| Prefer `CompileAndCheck()` over `expr->Compile()` when type must match target | Unchecked = silent type mismatch bytecode |
| NEVER add desugaring or AST transforms in codegen | Belongs in lowering. Only 3 valid reasons for codegen changes: new bytecode, new lowered node shape, verifier-mandated pattern |
| NEVER add optimization logic | Belongs in `bytecode_optimizer/` |
| Dispatch on semantic type (`TsType()`, `Signature`), NEVER on AST node kind | Pattern matching = fragile and incomplete |
| New AST nodes: MUST add UNREACHABLE stubs in BOTH `ETSCompilerUnreachable.cpp` AND `JSCompilerUnreachable.cpp` | Missing stubs = linker errors |
| NEVER modify `RegAllocator` for language needs | Language-agnostic. Prove three-tier design cannot handle it first |
| Codegen is read-only on AST | NEVER create or modify AST nodes |

---

## Cross-Stage Anti-Patterns

| Do NOT | Do Instead |
|---|---|
| Mutate AST in checker or varbinder | Write metadata only; all structural changes in lowering |
| Fix upstream errors in downstream stages | Fix at the source stage |
| Hand-edit generated files (`generated/`, `es2panda_lib.*`, diagnostic headers) | Update source-of-truth (YAML, IDL, templates) and regenerate |
| Add new `.cpp`/`.h` to only one build system | Update both CMakeLists.txt and BUILD.gn |
| Use `ArenaAllocator::New<T>()` where `ctx->AllocNode<T>()` is available | Use `ctx->AllocNode<T>()` for AST nodes |
| Forget `SetParent`/`SetRange` on new AST nodes | Call immediately after allocation in every stage |
| Compare types by pointer identity or `Is<TypeName>()` | Use `TypeRelation` APIs exclusively |
| Add ETS-specific logic to shared base classes (`parserImpl`, `VarBinder`, `TypedBinder`, `PandaGen`, `JSCompiler`) | Use ETS-specific subclass overrides |
| Use `new`/`delete` for compiler-owned objects | Arena allocation only |
| Introduce new status flag enums to work around design issues | Fix the underlying design |
| Add phases after `pluginsAfterLowerings` | Phase never executes |

---

## Actions Requiring Human Approval

Forbidden without team confirmation:

**IR:** Adding new `AstNodeFlags`; modifying AST node class hierarchy; changing `AST_NODE_MAPPING` macros; removing or renaming node types.

**Lexer:** Adding/removing/reordering `TokenType` enum values between sentinels; changing keyword hard/soft mode in a released language mode; modifying `IsIdentifierStart()`/`IsIdentifierPart()`; adding new `TokenFlags`/`NextTokenFlags`; changing `KeywordString` struct layout; adding virtual methods to base `Lexer`.

**Parser:** Modifying `parserImpl.*` for ETS features; adding new `ParserStatus` flags.

**VarBinder:** Adding new `SCOPE_TYPES`/`VARIABLE_TYPES`/`DECLARATION_KINDS` entries; modifying `Scope::Find()`/`FindLocal()`; adding new `VariableFlags` bits; modifying `ClassScope` sub-scope structure; changing `BoundContext` nesting.

**Checker:** Modifying `TypeRelation` core algorithms; adding/removing `TypeFlag` enum values; adding new `CheckerStatus` flags; changing smart cast semantics; modifying `StartChecker()` initialization order; changing function overload resolution.

**Lowering:** Reordering phases in `GetETSPhaseList()`; modifying program selector alias definitions in `phase.h`; modifying `PhaseManager` lifecycle; deleting/merging existing phases; modifying `CheckerPhase` position.

**Compiler Core:** Modifying `RegAllocator`/`RegSpiller`; modifying `CompileQueue` scheduling/threading; adding new bytecode instructions; changing `CodeGen` base class layout; modifying emitter output format.

---

## Pre-Modification Checklist

- [ ] Read `ets2panda/AGENTS.md` and the per-stage `AGENTS.md` under the modified directories?
- [ ] Change confined to ETS-specific code paths (not shared base classes)?
- [ ] Completely identified all downstream pipeline stages affected?
- [ ] For checker/varbinder: no AST mutations introduced?
- [ ] For post-checker lowering: `CheckLoweredNode`/`Recheck`/`Rebind` on every new subtree?
- [ ] `SetParent` and `SetRange` on every new AST node?
- [ ] New `.cpp` registered in both CMakeLists.txt and BUILD.gn?
- [ ] No hand-edits to any generated file?
- [ ] No new status flag enums or `VariableFlags` bits?
- [ ] All type operations through `TypeRelation` APIs? (checker changes)
- [ ] At least one test exercising the new/fixed behavior?
- [ ] For new AST node types: parser + checker + lowering + verifier + codegen all updated?

---

## Code Anchors

| Stage | Primary Entry Files (under `ets_frontend/ets2panda/`) |
|---|---|
| IR | `ir/astNode.h`, `ir/astNodeFlags.h`, `ir/astNodeMapping.h`, `ir/irnode.h`, `ir/ets/` |
| Lexer | `lexer/ETSLexer.h/.cpp`, `lexer/scripts/keywords.yaml`, `lexer/scripts/tokens.yaml` |
| Parser | `parser/ETSparser.h/.cpp`, `parser/ETSparserClasses.cpp`, `parser/ETSparserTypes.cpp`, `parser/ETSparserExpressions.cpp`, `parser/context/parserContext.h`, `parser/context/classPrivateContext.h` |
| VarBinder | `varbinder/ETSBinder.h/.cpp`, `varbinder/varbinder.h/.cpp` (base), `varbinder/scope.h/.cpp`, `varbinder/variable.h/.cpp`, `varbinder/recordTable.h/.cpp` |
| Checker | `checker/ETSchecker.h/.cpp`, `checker/ETSAnalyzer.h/.cpp`, `checker/checker.h/.cpp` (base), `checker/types/typeRelation.h/.cpp`, `checker/types/ets/`, `checker/ets/` |
| Lowering | `compiler/lowering/phase.h/.cpp`, `compiler/lowering/phase_id.h`, `compiler/lowering/util.h/.cpp`, `compiler/lowering/ets/` |
| Compiler Core | `compiler/core/ETSGen.h/.cpp`, `compiler/core/ETSCompiler.h/.cpp`, `compiler/core/ETSEmitter.h/.cpp`, `compiler/core/regAllocator.h/.cpp`, `compiler/core/compilerImpl.h/.cpp` |

---

## Test Anchors

| Test Type | Directory (under `ets_frontend/ets2panda/test/`) | Coverage |
|---|---|---|
| Parser regression | `parser/ets/` | Lexer, Parser |
| AST diagnostics | `ast/` | Parser, Checker (negative compile-time errors) |
| Runtime | `runtime/ets/` | Full pipeline end-to-end |
| Lowering unit | `unit/lowerings/` | Lowering phase correctness |
| CTS (spec compliance) | `cts/` | Broad regression across all stages |

---

## Verification Commands

All commands run from repository root.

```sh
# Build
cmake --build <out_dir> -j8    # es2panda (covers all frontend stages)

# Parser tests (syntax, AST construction)
runtime_core/static_core/tests/tests-u-runner-2/runner.sh es2panda-verifier parser --extension=ets --load-runtimes=ets --force-generate --processes=all

# Runtime tests (compile + verify + execute)
runtime_core/static_core/tests/tests-u-runner-2/runner.sh es2panda-verifier ets-runtime --extension=ets --load-runtimes=ets --force-generate --processes=all --es2panda-args=--simultaneous=true

# AST checker tests (diagnostics, semantic errors)
runtime_core/static_core/tests/tests-u-runner-2/runner.sh es2panda-verifier astchecker --extension=ets --load-runtimes=ets --force-generate --processes=all

# CTS tests (spec compliance, broad regression)
runtime_core/static_core/tests/tests-u-runner-2/runner.sh es2panda-verifier ets-cts --extension=ets --load-runtimes=ets --force-generate --processes=all --es2panda-args=--simultaneous=true
```

### Verification By Change Type

| Change Type | Minimum Verification |
|---|---|
| IR change (new/modified AST node) | All affected suites — parser + astchecker + ets-runtime + ets-cts |
| Lexer change | `parser` test suite |
| Parser change | `parser` + `astchecker` test suites |
| VarBinder change | `parser` + `ets-runtime` test suites |
| Checker change | `astchecker` + `ets-runtime` test suites |
| Lowering change | `unit/lowerings` + affected `ets-runtime` tests |
| Compiler core change | `ets-runtime` test suite |
| Cross-stage change | All affected suites + `ets-cts` |

---

## Related Documents

- `AGENTS.md` -- repository entry, routing rules, constraints, and verification expectations
- `docs/ets2panda/AST_Verifier_Knowledge_Base.md` -- AST invariant validation
- `docs/ets2panda/Public_Knowledge_Base.md` -- public C API and plugin interface
- `ets2panda/AGENTS.md` -- ets2panda route split and verification routing
- `ets2panda/checker/AGENTS.md`, `ets2panda/parser/AGENTS.md`, `ets2panda/varbinder/AGENTS.md`, `ets2panda/compiler/lowering/AGENTS.md`, `ets2panda/compiler/core/AGENTS.md`, `ets2panda/lexer/AGENTS.md` — per-component rules
