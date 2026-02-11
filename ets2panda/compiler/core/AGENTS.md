# Compiler Core Component

## Core Metadata

| Attribute | Value |
|-----------|--------|
| **Name** | Compiler Core |
| **Purpose** | Compiles the post-lowering AST into Panda bytecode: control-flow graph, register allocation, instruction emission, and module/function compile scheduling. This is the core code-generation layer of ets2panda. |
| **Primary Language** | C++ |

## Change Frequency and Scope

- **Logic in this directory is stable**. It is **modified infrequently**; most changes happen in lexer, parser, varbinder, checker, and compiler/lowering.
- When changes are needed, they are usually limited to the **ETS path**: ETSCompiler, ETSemitter, ETSGen, ETSfunction. The shared pipeline (scheduling, register allocation, CFG) is left unchanged.

## Directory Layout

```
compiler/core/
├── *Compiler*.cpp, *.h  # Per-language entry (ETSCompiler, JSCompiler)
├── *emitter*.cpp, *.h   # Bytecode emission (ETSemitter, JSemitter, emitter, pandagen)
├── *Gen*.cpp, *.h       # ETS code generation (ETSGen, ETSGen-inl)
├── compileJob.*, compileQueue.*, compilerImpl.*   # Compile jobs and implementation
├── CFG.*, codeGen.*     # Control-flow graph and codegen driver
├── function.*, ETSfunction.*   # Function compilation and ETS function context
├── regAllocator.*, regSpiller.*, regScope.*, vReg.*   # Register allocation and vregs
├── envScope.*, moduleContext.*, dynamicContext.*   # Scopes and module/dynamic context
├── labelTarget.*, switchBuilder.*, labelPair.h    # Labels and switch generation
├── programElement.*, targetTypeContext.*           # Program elements and target type context
└── ASTCompiler.h
```

## Responsibilities

- **Compile scheduling**: compileJob/compileQueue manage compile units and concurrency; compilerImpl orchestrates parser → varbinder → checker → lowering → this module.
- **Instruction emission**: emitter/pandagen turn AST into Panda instructions; ETS/JS emitters handle language-specific instructions and conventions.
- **Registers and CFG**: regAllocator, regSpiller, vReg, CFG manage virtual registers and basic blocks for backend constraints.
- **Module and function**: moduleContext, envScope, ETSfunction manage module- and function-level state and generation.

## Dependencies

- **Used by**: driver, aot, LSP, etc.; no other ets2panda front-end modules depend on this directory’s internals.
- **Depends on**: compiler/lowering, checker, varbinder, ir, util; interfaces with runtime/panda for bytecode output.

## Extending or Modifying

- **New instruction or emission pattern**: Extend the appropriate emitter and ETSGen; add constraints in CFG/regAllocator if needed. Preserve the existing scheduling/register design.
- **New compile entry or language**: Add a branch in compilerImpl and *Compiler; reuse the existing emitter/regAllocator design.
