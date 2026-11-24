# AOT Module - AGENTS.md

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Module responsibilities

The ahead-of-time compiler entry provides the command-line interface and compilation driver. It parses options, coordinates source and ABC inputs, schedules compilation, emits output files, handles patch/reload modes, and resolves module dependencies.

Read the repository `AGENTS.md`, `es2panda/AGENTS.md`, and `docs/knowledge/ES2Panda_Knowledge_Base.md` before modifying this module. Paths in this guide are repository-relative unless a block states another working directory.

## 2. Directory structure

```text
es2panda/aot/
├── main.cpp                  # Entry point and orchestration
├── options.cpp               # Option parsing and validation
├── options.h                 # Options accessors and input collection
├── emitFiles.cpp             # File emission jobs and queues
├── emitFiles.h               # Emission job declarations
├── resolveDepsRelation.cpp   # Dependency traversal and pruning
└── resolveDepsRelation.h     # Dependency resolver declarations
```

## 3. Main entry point

Follow `main.cpp` and `es2panda.cpp` together: options are parsed, inputs are collected, compilation produces cached programs, dependency information is processed where enabled, and emission jobs write the requested outputs. Check return values and diagnostics along each path.

`Compiler` is declared in `es2panda/es2panda.h`; do not replace its orchestration with a separate hand-written file loop. Source compilation and ABC recompilation have different input requirements.

## 4. Command-line options

### 4.1 Options class

`aot/options.h` defines the real `Options` class. Selected declarations from that header:

```cpp
    bool Parse(int argc, const char **argv);

    const es2panda::CompilerOptions &CompilerOptions() const
    {
        return compilerOptions_;
    }

    es2panda::CompilerOptions &CompilerOptions()
    {
        return compilerOptions_;
    }

    es2panda::parser::ScriptKind ScriptKind() const
    {
        return scriptKind_;
    }
```

Use `Options::CompilerOptions()` for the compiler configuration. Query existing accessors before adding fields or duplicating state; the CLI options object is not the same type as `es2panda::CompilerOptions`.

### 4.2 Command-line help and supported flags

`aot/options.cpp` is the option-registration source of truth. Confirm built-binary behavior with `es2abc --help`.

| Flag | Purpose |
|---|---|
| `--output <file>` | ABC output path |
| `--extension <js|ts|as|abc>` | Input extension; check validation for the selected input route |
| `--parse-only` | Parse without bytecode compilation |
| `--dump-ast` | Dump the parsed AST |
| `--dump-transformed-ast` | Dump the transformed AST |
| `--dump-assembly` | Dump generated assembly |
| `--opt-level <0|1|2>` | Select optimization level; currently defaults to 2 |
| `--module` | Parse source as an ES module |
| `--commonjs` | Select CommonJS handling |
| `--file-threads <n>` | File worker count |
| `--function-threads <n>` | Function worker count |
| `--abc-class-threads <n>` | ABC class worker count |
| `--merge-abc` | Emit merged ABC output |
| `--enable-abc-input` | Permit ABC input |

There is no supported `--enable-type-check` flag. TS syntax is parsed and transformed without the removed in-tree checker.

## 5. Multiple-file emission

`emitFiles.h` and `emitFiles.cpp` define `EmitFileQueue` and the `EmitSingleAbcJob`, `EmitMergedAbcJob`, and `EmitCacheJob` jobs. They use the worker dependency system in `util/workerQueue.h`.

- Single-output jobs consume the matching compiled program.
- Merged output must wait for all required compilation jobs.
- Cache emission must preserve the program cache and output mapping contracts.
- Error propagation must reach the CLI exit status; producing a partial output is not success.

Inspect these classes instead of introducing the old documentation's nonexistent `EmitFiles` wrapper.

## 6. Dependency resolution

`resolveDepsRelation.cpp` implements `DepsRelationResolver`. Trace its entry points from `main.cpp` and inspect compile-context inputs in `options.cpp` before changing dependency traversal or record pruning.

This path can remove records from generated ABC files. Check lazy imports, entry records, shared/HSP records, and record replacement together. Do not assume it is a generic topological-sort graph or reject cycles without checking the actual module semantics.

## 7. Parallel compilation scheduling

There are three levels of work:

1. File compilation is scheduled through the source/ABC compilation queues.
2. Function compilation uses `CompileFuncQueue` in `compiler/core/compileQueue.*`.
3. ABC recompilation can use class-level workers.

The main thread may consume work as well as wait for workers. Preserve queue dependencies, ownership of cached programs, and completion before emission; changing a thread count must not introduce a second ownership model.

## 8. Patch and reload support

The CLI registers `--dump-symbol-table`, `--input-symbol-table`, `--generate-patch`, `--hot-reload`, `--cold-reload`, and `--cold-fix`. Inspect mode validation and `util/patchFix.*` before combining these options.

The symbol-table input flag is `--input-symbol-table`, not `--symbol-table`. Use the symbol table generated from the corresponding baseline; do not reuse unrelated build metadata.

```bash
es2abc --dump-symbol-table baseline.symbols --output baseline.abc input.js
es2abc --generate-patch --input-symbol-table baseline.symbols --output patch.abc input.js
```

These command shapes illustrate baseline and patch inputs. Actual patch validation also needs baseline/new sources and the matching runtime.

## 9. Usage examples

```bash
# Basic source compilation
es2abc --output output.abc input.js
es2abc --extension ts --output output.abc input.ts

# AST and assembly diagnostics
es2abc --dump-ast input.js
es2abc --dump-transformed-ast --extension ts input.ts
es2abc --dump-assembly input.js

# Optimization and modules
es2abc --opt-level 2 --output output.abc input.js
es2abc --module --output module.abc input.js

# ABC recompilation requires explicit opt-in
es2abc --enable-abc-input --output rebuilt.abc input.abc
```

For multiple inputs, follow `Options` input-file/list collection and merge-mode validation in the current source. Do not assume arbitrary positional source files form a supported merged invocation.

## 10. Modification guidance

### 10.1 Add a command-line option

1. Register the flag and its help text in `options.cpp`.
2. Add storage/accessors only where the parsed value is owned.
3. Validate combinations and transfer the setting into `CompilerOptions` where needed.
4. Trace the setting to compilation/emission consumers.
5. Add positive and invalid-option cases near existing CLI regressions.

### 10.2 Add or modify an output path

1. Identify the emission job and cache ownership involved.
2. Preserve dependencies between compilation and output jobs.
3. Check merged and non-merged modes and diagnostic exit codes.
4. Obtain approval before changing ABC serialization or compatibility behavior.

## 11. Related modules

- Upstream: command-line callers and build integrations.
- Downstream: `es2panda/es2panda.h`, `es2panda/compiler/`, and `es2panda/util/programCache.h`.
- Supporting structures: `es2panda/ir/`, `es2panda/binder/`, patch helpers, and worker queues.

## 12. Verification and evidence

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
