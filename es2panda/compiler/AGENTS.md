# Compiler Module - AGENTS.md

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Module responsibilities

The dynamic compiler converts AST nodes and binder information into Panda instructions and assembled programs. It handles register allocation, lexical environments, control flow, functions, exceptions, module metadata, parallel compilation, and debug information.

Read the repository `AGENTS.md`, `es2panda/AGENTS.md`, and `docs/knowledge/ES2Panda_Knowledge_Base.md`. Notify the user before implementing changes that may alter emitted bytecode.

## 2. Directory structure

```text
es2panda/compiler/
├── core/
│   ├── compilerImpl.cpp        # Compilation orchestration
│   ├── compilerContext.cpp     # Per-compilation context
│   ├── pandagen.cpp            # Instruction generation helpers
│   ├── emitter/               # Program, function, literal, and module emission
│   ├── regAllocator.cpp        # Register allocation
│   ├── regScope.cpp            # Register lifetime scopes
│   ├── envScope.cpp            # Environment scopes
│   ├── compileQueue.cpp        # Source/function/ABC class queues
│   ├── function.cpp            # Function compilation state
│   └── labelTarget.cpp         # Jump targets
├── base/
│   ├── condition.cpp          # Conditions and branches
│   ├── iterators.cpp          # Iterator protocol
│   ├── destructuring.cpp      # Destructuring
│   ├── hoisting.cpp           # Declaration initialization
│   ├── lexenv.cpp             # Lexical environments
│   ├── lreference.cpp         # Assignment/reference handling
│   └── catchTable.cpp         # Exception tables
├── function/
│   ├── functionBuilder.cpp
│   ├── asyncFunctionBuilder.cpp
│   ├── generatorFunctionBuilder.cpp
│   └── asyncGeneratorFunctionBuilder.cpp
├── templates/isa.h.erb         # Instruction classes
├── templates/formats.h.erb     # Operand formats
└── debugger/debuginfoDumper.cpp
```

## 3. Core classes and APIs

### 3.1 CompilerImpl

`core/compilerImpl.h` declares `Compile`; the implementation creates a `CompilerContext`, schedules `CompileFuncQueue`, consumes work on the main thread, waits for workers, and finalizes the emitted program. Inspect the current arguments, including source/package metadata, before adding callers.

A selected current implementation excerpt:

```cpp
panda::pandasm::Program *CompilerImpl::Compile(parser::Program *program, const es2panda::CompilerOptions &options,
    const std::string &debugInfoSourceFile, const std::string &pkgName)
{
    CompilerContext context(program->Binder(), options.isDebug, options.isDebuggerEvaluateExpressionMode,
                            options.mergeAbc, false, options.recordDebugSource, debugInfoSourceFile, pkgName,
                            program->RecordName(), patchFixHelper_, options.enableColumn, options.enableCallableName);

    ArenaAllocator localAllocator(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);

    queue_ = new CompileFuncQueue(threadCount_, &context);
    queue_->Schedule();

    /* Main thread can also be used instead of idling */
    queue_->Consume();
    queue_->Wait();
```

### 3.2 CompilerContext

`core/compilerContext.h` owns the compilation context and exposes the emitter and compiler settings. It coordinates binder/program metadata, debug settings, merge mode, and patch helpers. Use existing context accessors rather than the old guide's speculative `GetPandaGen()`/`GetRegAllocator()` API.

### 3.3 PandaGen

`core/pandagen.h` defines the instruction-generation interface. AST nodes dispatch through their `Compile(PandaGen *)` implementations in `ir/`; PandaGen supplies loads, stores, calls, branches, register state, inline caches, and related helpers.

Do not assume a single public `PandaGen::Compile()` visitor implements every node. Trace the concrete node's compilation method and the helpers it calls.

### 3.4 Emitter

The emitter lives in `core/emitter/`, not a flat `core/emitter.cpp`. `Emitter::Finalize(bool dumpDebugInfo, util::PatchFix *patchFixHelper)` produces the assembled program. Function, module, literal, and debug information have distinct emission paths.

Preserve record names, source metadata, target API gates, and patch information. ABC serialization and compatibility changes require approval under the repository rules.

## 4. Bytecode instruction generation

### 4.1 Constants and binary operations

Start with the corresponding node in `ir/expressions/` or `ir/expressions/literals/`, then follow PandaGen helpers and generated instruction classes.

For a binary expression, preserve the language's operand evaluation order, intermediate value lifetime, conversion semantics, and exceptional behavior. Do not introduce a hand-written `Opcode` switch based on the former guide's illustrative API.

### 4.2 Assignment and lvalues

`base/lreference.cpp` centralizes variable/property assignment handling. Changes must account for property evaluation, compound assignment, destructuring, and optional-chain restrictions. Bypassing LReference can evaluate receivers or keys twice or lose side effects.

### 4.3 Control flow

`base/condition.cpp`, `core/labelTarget.cpp`, and statement-node implementations cooperate on conditional branches, loops, and jump targets.

Test `if` with/without `else`, short-circuit expressions, nested loops, labeled jumps, `break`, and `continue`. A branch change also needs exceptional-path and lexical-scope checks.

## 5. Register allocation

`RegAllocator`, `VReg`, and register-scope helpers have different roles. `RegScope` is constructed with `PandaGen *`; it is not the old guide's container that exposes `AllocReg()`/`FreeReg()` around a bitmap.

Use existing RAII scope patterns (`RegScope`, `LocalRegScope`, `LoopRegScope`, and `FunctionRegScope`) and the appropriate PandaGen allocation helpers. Review accumulator state, temporary lifetime, call-range registers, and register-width/format constraints.

A change that works for a short function may fail when register indices cross an encoding boundary. Include a nearby high-register-count regression when allocation or operand encoding changes.

## 6. Lexical environments

`core/envScope.*` and `base/lexenv.*` translate binder decisions into runtime storage. Check closure capture, lexical depth, environment slots, scope entry/exit, and initialization checks together.

Do not conflate binder declaration identity with storage location. Captured parameters, arrow functions, nested blocks, loops, and exception unwinding may require different environment behavior.

## 7. Function compilation

### 7.1 Builders

| Builder | Responsibility |
|---|---|
| `FunctionBuilder` | Common function setup and cleanup |
| `AsyncFunctionBuilder` | Async completion and suspension behavior |
| `GeneratorFunctionBuilder` | Generator state and yield behavior |
| `AsyncGeneratorFunctionBuilder` | Combined async/generator protocol |

The common lifecycle uses `Prepare` and `CleanUp` around node/body compilation. Inspect `function/functionBuilder.h` and concrete builders before adding lifecycle methods.

### 7.2 Parameters, return, await, and yield

Preserve parameter initialization order, defaults, rest/destructuring, `this`/`super`, completion values, and exceptions. For async/generator changes, test ordinary completion, explicit return, throw, suspension/resumption, and cleanup in `finally`.

## 8. Exception handling

`base/catchTable.*`, dynamic-context helpers, and try/catch/finally node compilation cooperate on handlers and abrupt completions.

A `CatchTable` is not the old guide's invented `BeginTry`/`EndTry` facade. Read the actual class before modifying handler boundaries. Cover nested handlers, rethrow, return/break/continue through `finally`, and lexical environment cleanup.

## 9. ISA instruction system

### 9.1 Source templates and generation

```text
es2panda/compiler/templates/isa.h.erb
es2panda/compiler/templates/formats.h.erb
    + runtime_core ISA data and helpers
    → generated instruction classes and operand formats
```

1. Identify the ISA/schema or template source of truth.
2. Edit source inputs, not generated headers.
3. Rebuild through GN so dependencies regenerate outputs.
4. Compile and link the instruction consumers.
5. Check register formats, inline caches, ranges, assembly, and runtime behavior.

The generated directory depends on product/toolchain configuration. Locate the output through the GN action instead of assuming `arkcompiler/ets_frontend/out/gen/isa.h`.

### 9.2 Instruction review points

Review loads/stores, arithmetic, comparisons, branches, calls, returns, inline-cache operands, and range forms according to the current ISA. Instruction names and operand encodings must come from current schema/generated classes; the old uppercase pseudo-opcode list was not an API specification.

## 10. Parallel compilation

```text
File compilation          → independent source/program work
CompileFuncQueue          → function work within compilation
CompileAbcClassQueue      → class work for ABC recompilation
```

Use `--file-threads`, `--function-threads`, and `--abc-class-threads` to configure supported worker counts. Inspect queue ownership and dependencies before changing scheduling or sharing mutable state.

Check single-worker and multi-worker results for determinism. Merged output and cached programs must not be finalized before all required jobs complete.

## 11. Modification guidance

### 11.1 Add an instruction

1. Verify the runtime instruction/schema and compatibility requirements.
2. Update ISA/template inputs and regenerate through the build.
3. Add or adapt PandaGen and node-compilation consumers.
4. Update emission/format support only where required by the actual instruction.
5. Run bytecode, runtime, verifier, and target-API regressions.

### 11.2 Add AST compilation behavior

1. Implement the node's real `Compile` method and required helpers.
2. Preserve evaluation order and register/environment lifetimes.
3. Handle control-flow and exception boundaries.
4. Compare assembly and runtime results against the intended semantics.
5. Include JS and TS-transformed cases where the same node can occur.

## 12. Related modules

- Upstream: `es2panda/parser/`, `es2panda/binder/`, and `parser/transformer/`.
- Shared AST: `es2panda/ir/`.
- Downstream: runtime assembler, bytecode optimizer, ABC consumers, and VM/verifier repositories.
- There is no upstream in-tree TypeScript checker in this checkout.

## 13. Verification and evidence

Build the affected dynamic frontend target from the OpenHarmony source root before running behavior tests:

```bash
./build.sh --product-name rk3568 --build-target ets_frontend_build --no-prebuilt-sdk
./build.sh --product-name rk3568 --build-target arkcompiler/ets_frontend/es2panda:es2abc_tests --no-prebuilt-sdk
```

Set and validate paths using `docs/knowledge/Build_Environment.md`. Prepare `RUNTIME_DIR` and `TEST_LD_LIBRARY_PATH` using `docs/knowledge/ES2Panda_Testing.md`, then run the relevant suite from the repository root:

```bash
python3 es2panda/test/runner.py --regression "$FRONTEND_BUILD_DIR"
python3 es2panda/test/runner.py --compiler --js-runtime "$RUNTIME_DIR" --LD_LIBRARY_PATH "$TEST_LD_LIBRARY_PATH" "$FRONTEND_BUILD_DIR"
```

Use the complete dynamic-route verification procedure in `es2panda/AGENTS.md` for code changes. Record the build mode, commands, logs, unexpected failures, skipped cases, and missing prerequisites. Do not claim an unexecuted test passed. Changes to emitted bytecode require user notification and runtime/verifier coverage.
