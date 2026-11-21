# ES2Panda Knowledge Base

Path setup and validation: `docs/knowledge/Build_Environment.md` (repository-relative).


> Knowledge-base paths beginning with `docs/knowledge/` are relative to the repository root.


> [!NOTE]
> Version: v2.1 | Updated: 2026-06-03
> Scope: all project-specific knowledge for the `es2panda` dynamic compilation route
> Repo index: `arkcompiler/ets_frontend/es2panda/AGENTS.md` and per-subdirectory `AGENTS.md`

---

**TAG: ARCH**
## 1. Route Architecture

**Facts:**
- `es2panda` and `ets2panda` are **completely independent codebases** — separate source directories, separate build systems, separate namespaces (`panda::es2panda` vs `ark::es2panda`). They share no compiled source code, despite having conceptually similar module structures (lexer/parser/IR/compiler) due to common heritage
  → Do not cross-apply experience; do not port `ets2panda` checker/lowering logic to `es2panda`
- Output binary: `es2abc`, namespace: `panda::es2panda`
- No lowering main path — AST goes directly to bytecode generation
- `ScriptExtension` enum: `JS | TS | AS(ArkTS) | ABC(recompilation)` — `es2panda/es2panda.h`
- `ScriptKind` enum: `SCRIPT | MODULE | COMMONJS` — `es2panda/parser/program/program.h`

### 1.1 Compilation Pipeline

```
source → lexer → parser → binder → [parser/transformer + rebinding] → compiler(PandaGen) → emitter → .abc
```

Optional paths:
- ABC recompilation: `.abc` → `CompileAbcFile` → optimize/re-emit
- Patch/hot-reload: `PatchFix` + `SymbolTable` compare differences
- Tree-shaking: `DepsRelationResolver` BFS prunes unused records

### 1.2 Three-Level Parallelism

| Level | CLI Flag | Scope |
|-------|----------|-------|
| File-level | `--file-threads` | Parallel compilation of multiple source files |
| Function-level | `--function-threads` | Function-granularity parallelism |
| Class-level | `--abc-class-threads` | ABC recompilation only |

---

**TAG: AOT**
## 2. AOT — CLI Entry

| Description | File |
|-------------|------|
| Program entry, full orchestration flow | `aot/main.cpp` |
| `Options` class, parses the registered CLI flags into `CompilerOptions` | `aot/options.cpp` |
| File emission hierarchy: `EmitFileQueue` / `EmitSingleAbcJob` / `EmitMergedAbcJob` / `EmitCacheJob`, all inherit `WorkerJob` | `aot/emitFiles.cpp` |
| `DepsRelationResolver` — BFS tree-shaking | `aot/resolveDepsRelation.cpp` |
| `Compiler` public API (`CompileFiles` / `Compile` / `CompileAbcFile`) | `es2panda.h` |

**Facts:**
- `MemManager` initializes the Panda memory pool; inspect its constants and configuration in `es2panda/aot/main.cpp` for the current sizing policy
- `--merge-abc` mode merges all programs into a single ABC; `EmitMergedAbcJob` waits for all dependencies before executing
- `--compile-context-info` accepts a JSON file with: compile entries, HSP package names, version info, replace records
- Hot-reload/patch modes: `--generate-patch` / `--hot-reload` / `--cold-reload` / `--cold-fix` + `--input-symbol-table`
- `CheckMergeModeConsistency()` validates merge/non-merge consistency
- `RemoveRedundantRecord()` optional optimization step
- Control flow: `main()` → `Options::Parse()` → `Compiler::CompileFiles()` → parallel compile → `DepsRelationResolver::Resolve()` → `EmitFileQueue` → `.abc`

---

**TAG: LEXER**
## 3. Lexer

| Description | File |
|-------------|------|
| `Lexer` class, scans source into token stream, manages `LineIndex` | `lexer/lexer.cpp` |
| `Token` class, `TokenType` enum | `lexer/token/token.cpp` |
| `SourceLocation` / `SourceRange` source position tracking | `lexer/token/sourceLocation.h` |
| Regex literal pattern/flags parsing | `lexer/regexp/regexp.cpp` |
| Generates `keywords.h` (keyword token definitions) | `lexer/templates/keywords.h.erb` |
| Generates `keywordsMap.h` (string→Token::Type mapping) | `lexer/templates/keywordsMap.h.erb` |
| Ruby script defining JS/TS keyword list | `lexer/scripts/keywords.rb` |

**Facts:**
- Keyword generation pipeline: `keywords.rb` → `gen_keywords.sh` → `.erb` templates → `keywords.h` + `keywordsMap.h` → referenced by `lexer.cpp`
- Adding a token requires updating `TokenType` enum
- `LineIndex` is built in Lexer and used by all subsequent stages for line/column lookup
- **Regex vs division disambiguation**: parsing context selects the regex route. In `parser/expressionParser.cpp`, the primary-expression handler for divide tokens invokes `Lexer::ScanRegExp()` and `RegExpParser`; ordinary division follows expression operator parsing. Preserve the parser/lexer contract rather than deciding every slash from the previous token alone.

---

**TAG: PARSER**
## 4. Parser

| Description | File |
|-------------|------|
| `ParserImpl` recursive descent core | `parser/parserImpl.cpp` |
| `ParserImpl` expression parsing implementation, operator precedence | `parser/expressionParser.cpp` |
| `ParserImpl` statement parsing implementation, control flow and declarations | `parser/statementParser.cpp` |
| `ParserContext` — manages parse flags and error collection | `parser/context/parserContext.cpp` |
| TS AST → ES AST transformation (executes during parse stage) | `parser/transformer/transformer.cpp` |
| CommonJS module wrapper (**independent from ES Module system**) | `parser/commonjs.cpp` |
| `Program` compilation unit top-level container | `parser/program/program.cpp` |
| `SourceTextModuleRecord` — ES Module Record | `parser/module/sourceTextModuleRecord.cpp` |

**Facts:**
- `ScriptKind::MODULE` triggers ES Module path, `ScriptKind::COMMONJS` triggers CommonJS path
- `Program` is created at parser stage and spans the entire pipeline
- TS transformation is orchestrated by `es2panda.cpp` after parsing and before bytecode compilation
- `ParserImpl` is shared by expression, statement, and CommonJS parsing — always assess global impact before modifying

---

**TAG: MODULE**
## 5. Module — ES Module Record

| Description | Data Structure / File |
|-------------|----------------------|
| Module dependency record (source_ path + isLazy_ flag) | `ModuleRequestRecord` |
| Import binding (moduleRequestIdx_ + localName_ + importName_) | `ImportEntry` |
| Export binding (moduleRequestIdx_ + exportName_ + localName_ + importName_ + isConstant_) | `ExportEntry` |
| All defined in | `parser/module/sourceTextModuleRecord.h` |

**Facts:**
- Default export special names: `DEFAULT_LOCAL_NAME = "*default*"`, `DEFAULT_EXTERNAL_NAME = "default"`
- Lazy imports flagged via `ModuleRequestRecord::isLazy_`
- **A Program may own two** `SourceTextModuleRecord` instances: `moduleRecord_` (value imports) and `typeModuleRecord_` (type imports, `import type` syntax)
- `localExportEntries_` uses `ArenaMultiMap`, allowing same-name multiple exports (for detecting duplicate export errors)
- `AssignIndexToModuleVariable`: assigns indices to all module variables at compile time
- Do not assume `moduleRequestIdx_ == -1` means a local export — check `ExportEntry` semantics explicitly

---

**TAG: PROGRAM**
## 6. Program — Compilation Unit Container

| Description | Field |
|-------------|-------|
| AST root node | `ast_: ir::BlockStatement*` |
| Full source text | `sourceCode_: util::UString` |
| Source file path | `sourceFile_: util::UString` |
| ABC record name | `recordName_: util::UString` |
| Record name with trailing dot (ABC output identifier) | `formatedRecordName_` |
| Module system type | `kind_: ScriptKind {SCRIPT, MODULE, COMMONJS}` |
| Language feature type | `extension_: ScriptExtension {JS, TS, AS, ABC}` |
| Semantic binder | `binder_: binder::Binder*` |
| Arena allocator (all AST nodes must use this) | `allocator_: unique_ptr<ArenaAllocator>` |
| ES module record | `moduleRecord_: SourceTextModuleRecord*` |
| Type-import module record | `typeModuleRecord_: SourceTextModuleRecord*` |
| Line-to-offset mapping | `lineIndex_: lexer::LineIndex` |
| Patch / hot-reload helper | `patchFixHelper_: util::PatchFix*` |
| .d.ts declaration file flag | `isDtsFile_` |
| Top-level await flag | `hasTLA_` |
| Debug mode | `isDebug_` |
| Use define semantics | `useDefineSemantic_` |
| Shared module flag | `isShared_` |
| Enable annotation processing | `enableAnnotations_` |
| Target API version | `targetApiVersion_` |

Defined in: `parser/program/program.h`

**Facts:**
- Program is **movable but non-copyable**; lifetime managed by `Compiler`
- `formatedRecordName_` is the record name with a trailing dot, used as identifier in ABC output

---

**TAG: BINDER**
## 7. Binder

| Description | File |
|-------------|------|
| `Binder` class — binding entry point, traverses AST to establish variable-reference mappings | `binder/binder.cpp` |
| Scope hierarchy: `Scope` → `VariableScope` → `FunctionScope` / `ModuleScope` / `BlockScope` / `CatchScope` / `LoopScope` etc., each holding a `VariableMap` | `binder/scope.cpp` |
| `Variable` class — subclasses: `LocalVariable` / `GlobalVariable` / `ModuleVariable` etc. | `binder/variable.cpp` |
| `Declaration` class — records AST position and type | `binder/declaration.cpp` |

**Facts:**
- Binding result: all `Identifier` nodes have `Variable_` pointer set to the resolved `Variable`
- `ModuleScope` manages module-level import/export variable bindings, works with `SourceTextModuleRecord`
- Closure capture: when inner functions reference outer variables, binder marks them for lexical environment allocation

---

**TAG: TS**
## 8. TypeScript — Transformation

The removed `es2panda/typescript` checker is not available in this checkout. TypeScript syntax remains supported by `parser/`, `ir/ts/`, and `parser/transformer/`.

`es2panda.cpp` constructs `parser::Transformer` for TS input; `Compiler::ProcessAstForTS` transforms the AST and runs `Binder::IdentifierAnalysis` with `TS_AFTER_TRANSFORM`. Inspect this implementation and parser binding order before changing TS behavior. There is no supported `--enable-type-check` CLI flag or in-tree TS checker to debug.

---

**TAG: IR**
## 9. IR — AST/IR Node Definitions

| Description | File / Directory |
|-------------|-----------------|
| `AstNode` base — node kinds from `AstNodeType` and `ir/astNodeMapping.h`, `AstNodeFlags` / `ModifierFlags` / `ScriptFunctionFlags` | `ir/astNode.h` |
| `AstDumper` class (used by `--dump-ast`) | `ir/astDump.cpp` |
| `Expression` base (adds `grouped_` flag and type annotation support) | `ir/expression.h` |
| Expression node types (current inventory in this directory) | `ir/expressions/` |
| Statement node types (current inventory in this directory) | `ir/statements/` |
| TypeScript-specific node types (current inventory in this directory) | `ir/ts/` |
| ES module declaration nodes | `ir/module/` |
| Literal node types (see next section) | `ir/expressions/literals/` |

**Facts:**
- `AstNode::IsProgram()` returns true when `parent_ == nullptr` — root node's parent is null
- `ModifierFlags` supports bitwise composition via `DEFINE_BITOPS` macro
- Auto-generated `Is##Type()` and `As##Type()` methods — **As methods have assert guards**, do not use `reinterpret_cast`
- Adding a new AST node: append to `AstNodeType` enum (**do not modify existing enum values** — breaks serialization compatibility), implement four virtuals (`Iterate`, `Dump`, `Compile`, `UpdateSelf`), update parser and compiler
- `AstNodeFlags` values: `STRICT`, `PARAMETER`, etc.
- `ModifierFlags` values: `STATIC`, `ASYNC`, `PUBLIC`, `PRIVATE`, `DECLARE`, `READONLY`, etc.
- `ScriptFunctionFlags` values: `GENERATOR`, `ASYNC`, `ARROW`, `OVERLOAD`, `CONSTRUCTOR`, etc.

---

**TAG: LITERAL**
## 10. Literals — Literal Nodes

| Literal Type | Stored Fields | `Tag()` Return | File |
|-------------|---------------|----------------|------|
| `StringLiteral` | `StringView str_` | `STRING` | `ir/expressions/literals/stringLiteral.h` |
| `NumberLiteral` | `double number_` + `StringView str_`(original text) | `INTEGER` / `DOUBLE` | `ir/expressions/literals/numberLiteral.h` |
| `BooleanLiteral` | `bool boolean_` | `BOOLEAN` | `ir/expressions/literals/booleanLiteral.h` |
| `NullLiteral` | no data members | `NULL_VALUE` | `ir/expressions/literals/nullLiteral.h` |
| `BigIntLiteral` | `StringView src_`(raw text) | `NULL_VALUE` | `ir/expressions/literals/bigIntLiteral.h` |
| `RegExpLiteral` | `pattern_` + `flags_` | `NULL_VALUE` | `ir/expressions/literals/regExpLiteral.h` |
| `TaggedLiteral` | `LiteralTag tag_` + `uint16_t num_` + `StringView str_` | determined by `tag_` | `ir/expressions/literals/taggedLiteral.h` |

**Facts:**
- **`Tag() == NULL_VALUE` does NOT mean "no value"**: `NullLiteral`, `BigIntLiteral`, and `RegExpLiteral` all return `NULL_VALUE` with completely different semantics
- `NumberLiteral` preserves original string — needed for exact representation (integers exceeding `Number.MAX_SAFE_INTEGER` require `BigIntLiteral`)
- `TaggedLiteral` determines semantics via `tag_`: method references (`METHOD`/`GENERATOR_METHOD`/`ASYNC_GENERATOR_METHOD`/`GETTER`/`SETTER`) and affiliate data (`METHODAFFILIATE`)
- `TaggedLiteral::Method()` will **assert-crash** if tag type is not checked first
- `LiteralTag` enum must stay in sync with `panda::panda_file::LiteralTag`

---

**TAG: COMPILER**
## 11. Compiler — Bytecode Compilation

| Description | File |
|-------------|------|
| `CompilerImpl` — compilation orchestration, manages compilation context and parallel compilation | `compiler/core/compilerImpl.cpp` |
| `PandaGen` — compilation core visitor, generates Panda IR instructions | `compiler/core/pandagen.cpp` |
| `Emitter` — serialization: `Panda IR → pandasm::Program → AsmEmitter::Emit() → .abc` | `compiler/core/emitter/emitter.cpp` |
| `RegAllocator` — VReg allocation and release | `compiler/core/regAllocator.cpp` |
| Compile-time function representation | `compiler/core/function.cpp` |
| Parallel compilation queue | `compiler/core/compileQueue.cpp` |

### 11.1 base/ Compilation Helper Patterns

| Pattern | Description | File |
|---------|-------------|------|
| `CatchTable` | try/catch labels | `compiler/base/catchTable.cpp` |
| `Condition` | Conditional compilation | `compiler/base/condition.cpp` |
| `Destructuring` | Destructuring assignment | `compiler/base/destructuring.cpp` |
| `Hoisting` | Variable/function hoisting | `compiler/base/hoisting.cpp` |
| `Iterators` | Iterator protocol (SYNC/ASYNC, for..of/spread/destructuring) | `compiler/base/iterators.cpp` |
| `LexEnv` | Lexical environment variable access (`Expand()`) | `compiler/base/lexenv.cpp` |
| `Literals` | Literal buffers | `compiler/base/literals.cpp` |
| `LReference` | Unified lvalue abstraction (MEMBER / VAR_OR_GLOBAL / DESTRUCTURING) | `compiler/base/lreference.cpp` |
| `OptionalChain` | Optional chaining short-circuit | `compiler/base/optionalChain.cpp` |

### 11.2 function/ Function Builders

| Builder | Description |
|---------|-------------|
| `FunctionBuilder` | Base class (NORMAL) |
| `AsyncFunctionBuilder` | async — wraps return values as Promises |
| `GeneratorFunctionBuilder` | generator — creates generator objects, yield suspend/resume |
| `AsyncGeneratorFunctionBuilder` | async + generator combined |

All builders in `compiler/function/`

**Facts:**
- `LReference` is the unified lvalue abstraction — all assignment operations go through it; understanding it is key to understanding the compiler
- `FunctionBuilder` lifecycle: `Prepare()` → compile body → `CleanUp()`
- Closure variables accessed via `LoadLexicalVar`/`StoreLexicalVar` (marked by binder)
- Inline cache managed via `GetCurrentSlot`/`IncreaseCurrentSlot`
- `PandaGen` core methods: `LoadVar`/`StoreVar`, `LoadObjProperty`/`StoreObjProperty`, `Branch`/`Condition`, `CallThis`/`Call`/`SuperCall`
- Do not manipulate VReg directly — use PandaGen's register management
- Do not bypass LReference for property assignment — misses optional chaining / destructuring scenarios

---

**TAG: TEMPLATES**
## 12. Templates — ERB Auto-Generation

| Description | File |
|-------------|------|
| Generates `IRNode` subclass per bytecode instruction (e.g. `Lda`, `Sta`, `Callthis0`, `Add2`), with constructor / `GetFormats()` / `Registers()` / `Transform()` / inline cache (`SetIcSlot`/`GetIcSlot`) / range instruction support + `Label` class | `compiler/templates/isa.h.erb` |
| Defines operand format tables per instruction (`FormatItem` arrays: `SRC_VREG`, `DST_VREG`, `IMM`, `LABEL`, `ID`, `STRING_ID` and bit widths) | `compiler/templates/formats.h.erb` |

**Facts:**
- Input from `arkcompiler/runtime_core/isa/isa.yaml` + `isapi.rb` (Ruby helper)
- Inline cache conditionally generated based on instruction properties: `jit_ic_slot`, `ic_slot`, `eight_bit_ic`, etc.
- Range instructions (`callrange`, `newobjrange`) supported via `IsRangeInst()` and `RangeRegsCount()`
- Build integration: `BUILD.gn` `gen_isa_headers` target
- Adding a new bytecode instruction: modify `isa.yaml` → rebuild to auto-generate

---

**TAG: DEBUGGER**
## 13. Debugger — Debug Info

| Description | File |
|-------------|------|
| `DebugInfoDumper` (namespace `panda::es2panda::debuginfo`) — serializes `pandasm::Program` to JSON debug dump | `compiler/debugger/debuginfoDumper.cpp` |

**Facts:**
- Local variable info output in **reverse order** (consistent with ts2panda)
- Each instruction includes `debug_pos_info` (with `sourceLineNum`) for source line mapping
- Output is JSON (stdout), **distinct from** the binary debug section inside `.abc` files
- Triggered indirectly via `--record-debug-source` and similar CLI flags
- For development debugging only, not for production source mapping

---

**TAG: UTIL**
## 14. Util — Utilities

`util/` is flat — no subdirectories.

| Description | File |
|-------------|------|
| `StringView` (**non-owning** UTF-8 string view) + `UString` (arena-allocated mutable Unicode string builder) | `util/ustring.h` |
| `Helpers` — AST node queries / number conversion / file I/O / directive scanning / API version checks | `util/helpers.h` |
| `PatchFix` — incremental bytecode patching (**operates at pandasm level, not AST level**) | `util/patchFix.h` |
| `SymbolTable` — reads/writes symbol table files for the patch system | `util/symbolTable.h` |
| `WorkerJob` + `WorkerQueue` — dependency-tracked parallel task scheduling (`DependsOn()` DAG scheduling) | `util/workerQueue.h` |
| `Concurrent` — handles `use concurrent` ECMAScript directive | `util/concurrent.h` |
| `CommonUtil` — OHM URL parsing / string splitting / static/dynamic import traversal templates | `util/commonUtil.h` |
| `ModuleHelpers` — compiles npm module entry lists | `util/moduleHelpers.h` |
| `DEFINE_BITOPS(T)` macro — generates bitwise operators for scoped enums | `util/enumbitops.h` |
| `ProgramCache` + `AbcProgramsCache` — cached compiled program outputs | `util/programCache.h` |
| Base64 encode/decode | `util/base64.h` |
| `BitSet` — compact dynamic bit array | `util/bitset.h` |
| `Dumper` — debug dump of literal table and string table | `util/dumper.h` |

**Facts:**
- **`StringView` lifetime**: non-owning, must not outlive backing data. Calling `Mutf8()` produces a new `std::string`; the original view may become invalid
- `PatchFixKind` five modes: `DUMPSYMBOLTABLE`, `HOTFIX`, `COLDFIX`, `HOTRELOAD`, `COLDRELOAD`
- `Helpers::ScanDirectives()` handles **OpenHarmony extension directives**: `use sendable`, `use concurrent`, `use shared`, `implements static:`
- `WorkerJob::DependsOn()` used by `EmitMergedAbcJob` to wait for all per-file compilations

---

**TAG: SCRIPTS**
## 15. Scripts — Build and Test Scripts

| Description | File |
|-------------|------|
| Invokes Ruby `isa_gen` to generate ISA headers | `scripts/gen_isa.sh` |
| Invokes `keywords.rb` to generate keyword headers | `scripts/gen_keywords.sh` |
| Integration test runner — supports `--compiler` / `--bytecode` / `--tsc` / `--regression` / `--hotfix` / `--hotreload` / `--coldfix` / `--coldreload` / `--version-control` | `test/runner.py` |
| API version mapping (supported versions in the current map) | `test/config.py` |

**Facts:**
- `runner.py --help` lists all test modes
- API version (`config.py`) affects test behavior — some features only supported after specific API versions
- Modifying `.erb` templates requires rebuilding to trigger code generation

---

**TAG: CONSTRAINTS**
## 16. Global Constraints and Pitfalls

### Top 10 Don'ts

> The most critical mistakes AI agents make when working on `es2panda`. Read this before any code change.

| # | Don't | Why |
|---|-------|-----|
| 1 | Do NOT port `ets2panda` checker/lowering logic to `es2panda` | Completely independent codebases; no shared source code between the two |
| 2 | Do NOT hand-edit `.erb` template output (e.g. `isa.h`, `formats.h`) | Auto-generated; edit `.erb` source templates instead, then rebuild |
| 3 | Do NOT modify existing `AstNodeType` enum values | Breaks serialization compatibility; always append new values |
| 4 | Do NOT use `reinterpret_cast` for AST node downcasting | Use auto-generated `As##Type()` methods which have assert guards |
| 5 | Do NOT bypass `LReference` for property assignment | Misses optional chaining / destructuring scenarios |
| 6 | Do NOT manipulate VReg directly | Use `PandaGen`'s register management to avoid register corruption |
| 7 | Do NOT assume `Tag() == NULL_VALUE` means "no value" | `NullLiteral`, `BigIntLiteral`, and `RegExpLiteral` all return `NULL_VALUE` with different semantics |
| 8 | Do NOT invent an in-tree TypeScript checker | This checkout supports TS parsing/transformation, without the removed checker |
| 9 | Do NOT mix ES Module and CommonJS handling logic | Two independent module systems with different `ScriptKind` paths |
| 10 | Do NOT let `StringView` outlive its backing data | Non-owning view; `Mutf8()` produces a new `std::string`, original view may become invalid |

**Warnings:**
### Constraints

- `LiteralTag` enum must stay in sync with `panda::panda_file::LiteralTag`
- Auto-generated code (`.erb` template output) **must never be hand-edited** — always modify `.erb` source templates
- `parserImpl.cpp` shared parser implementation — always assess global impact before modifying
- ES Module and CommonJS are two independent module systems — do not mix their handling logic
- The removed dynamic TypeScript checker is unavailable; do not route changes to it
- All AST nodes must be allocated via Program's `allocator_` arena

### Pitfalls

- Porting `ets2panda` lowering/checker logic to `es2panda`
- Editing template-generated output instead of `.erb` files
- Treating all TS issues as parser issues — may be transformer or binder issues
- Assuming TypeScript transformation performs full semantic type checking
- Manipulating VReg directly instead of using PandaGen's register management
- Bypassing LReference for property assignment
- Using `reinterpret_cast` instead of `As##Type()` for node downcasting
- Modifying `AstNodeType` enum values instead of appending new ones
- `Tag() == NULL_VALUE` does NOT mean "no value" — `NullLiteral`/`BigIntLiteral`/`RegExpLiteral` have completely different semantics
- `TaggedLiteral::Method()` will assert-crash if tag type is not checked first
- `StringView` is non-owning, must not outlive backing string data
- `Mutf8()` on a `StringView` produces a new `std::string`
- `ModuleRequestIdx_ == -1` does not necessarily mean a local export — check `ExportEntry` semantics explicitly

---

**TAG: DEBUG**
## 17. Debug Reference

### 17.1 Stable Reference — Key Breakpoint Locations

> These are code-level anchors tied to class/method names. They are stable across versions and suitable for long-term reference.

| Purpose | Breakpoint |
|---------|------------|
| Observe token sequence | `Lexer::NextToken()` |
| Observe syntax parsing | `ParserImpl::ParseStatement()` / `ParseExpression()` |
| Observe identifier resolution | `Binder::IdentifierAnalysis` |
| Observe instruction generation | `CompilerImpl::Compile()` |
| Observe literal compilation | `Literal::Compile()` |
| Observe node structure | Node's `Dump()` method |
| Observe child node traversal | `Iterate()` |
| Observe import/export binding | `AddImportEntry` / `AddLocalExportEntry` |

### 17.2 Development Tools — CLI Flags & Build Commands

> ⚠️ CLI flags and build targets may change across versions. Verify against `es2abc --help` and current `BUILD.gn` before use.

**CLI Debug Flags:**

| Flag | Purpose |
|------|---------|
| `--dump-ast` | Inspect AST structure |
| `--dump-transformed-ast` | Inspect TS transformer output |
| `--dump-assembly` | Inspect generated assembly instructions |
| `--dump-asm-program` | Inspect full pandasm program structure |
| `--dump-literal-buffer` | Inspect literal buffer contents |
| `--dump-string` | Inspect string table |
| `--dump-deps-info` | Inspect dependency resolution results |
| `--dump-symbol-table <path>` | Export symbol table file |
| `--debug-info` | Embed debug info during compilation |
| `--record-debug-source` | Enable source recording |
| `--perf-file` + `--perf-level` | Output compilation performance data |
| `--dump-size-stat` / `--dump-file-item-size` | Output ABC file size statistics |
| `--parse-only` | Parse only, do not compile |

**Build and Test Commands:**

Use `docs/knowledge/Build_Environment.md` to set/validate `OH_ROOT` and `FRONTEND_BUILD_DIR`. The complete ordered procedure, including all integration modes and external prerequisites, is in `docs/knowledge/ES2Panda_Testing.md`.

```bash
cd "$OH_ROOT"
./build.sh --product-name rk3568 --build-target ets_frontend_build
cd "$OH_ROOT/arkcompiler/ets_frontend"
python3 es2panda/test/runner.py --regression "$FRONTEND_BUILD_DIR"
```

---

**TAG: INDEX**
## 18. Module Index

> Paths shown in **full form** are relative to the OpenHarmony source root (e.g. `arkcompiler/ets_frontend/…`).
> **In-repo equivalent** (relative to `ets_frontend/`): simply strip the `arkcompiler/ets_frontend/` prefix — e.g. `es2panda/lexer/AGENTS.md`.

| Module | AGENTS.md Path (OpenHarmony root) | In-repo Relative |
|--------|-----------------------------------|------------------|
| Repository root | `arkcompiler/ets_frontend/AGENTS.md` | `AGENTS.md` |
| es2panda top-level entry | `arkcompiler/ets_frontend/es2panda/AGENTS.md` | `es2panda/AGENTS.md` |
| AOT / CLI entry | `arkcompiler/ets_frontend/es2panda/aot/AGENTS.md` | `es2panda/aot/AGENTS.md` |
| Lexer | `arkcompiler/ets_frontend/es2panda/lexer/AGENTS.md` | `es2panda/lexer/AGENTS.md` |
| Parser | `arkcompiler/ets_frontend/es2panda/parser/AGENTS.md` | `es2panda/parser/AGENTS.md` |
| Binder | `arkcompiler/ets_frontend/es2panda/binder/AGENTS.md` | `es2panda/binder/AGENTS.md` |
| Compiler | `arkcompiler/ets_frontend/es2panda/compiler/AGENTS.md` | `es2panda/compiler/AGENTS.md` |
| IR node definitions | `arkcompiler/ets_frontend/es2panda/ir/AGENTS.md` | `es2panda/ir/AGENTS.md` |
| Scripts | `arkcompiler/ets_frontend/es2panda/scripts/AGENTS.md` | `es2panda/scripts/AGENTS.md` |

## 19. Repository-level regression routes

Paths in this section are relative to `ets_frontend/`. These routes have different prerequisites and are not interchangeable.

| Route | Entry / inputs | Validation and prerequisites |
|---|---|---|
| Dynamic regression | `es2panda/test/runner.py --regression "$FRONTEND_BUILD_DIR"` | Compare parser output with regression expectations; requires built `es2abc` |
| TS conformance | `es2panda/test/runner.py --tsc "$FRONTEND_BUILD_DIR" --tsc-path <typescript_tree>` | Uses external TypeScript tests; inspect skip lists and report unexpected failures |
| Legacy TestTs | `testTs/run_testTs.py`, `testTs/config.py` | Run from repository root with `--file <case.ts>` or `--dir <cases>` and `--ark_frontend_tool <index.js>`; requires legacy Node frontend and TS fixtures. The script invokes Node with `--output-type`, not native `es2abc`. Inspect outputs under `out/testTs/` and compare with `testTs/expect/`; report skips and failures |
| ECMAScript optimizer fixture | `test_ecma_bcopt/test_bytecode_optimizer_for_js.js`, `test_ecma_bcopt/test_bytecode_optimizer_for_js.txt` | JS input plus historical assembly reference; no standalone runner or GN target is defined in this directory. Locate a compatible runtime optimizer/disassembler consumer before execution; compare optimized disassembly and behavior. Do not claim coverage from fixture presence or compare different bytecode versions blindly |

For dynamic code changes, also run both Test262 modes (`test262/run_test262.py --es2022 all --ark-frontend=es2panda --ark-frontend-binary=<es2abc> --product-name=rk3568 --timeout=3000000`, with and without `--abc2program`). From the OpenHarmony root build `runtime_core_host_unittest`, `arkcompiler/runtime_core:verifier_host_unittest`, `arkcompiler/runtime_core:ark_host_linux_tools_packages` (disassembler build), and `arkcompiler/runtime_core/tests/fuzztest:fuzztest`. Run `python3 arkcompiler/runtime_core/isa/check_version.py` for instruction/ABC changes; this is a version check, not a compatibility test. Record unavailable runtime, fixtures, toolchains, and host branches explicitly.
