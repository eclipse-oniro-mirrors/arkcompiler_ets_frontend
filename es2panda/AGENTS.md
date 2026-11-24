# ES2Panda - AGENTS.md

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Project overview

`es2panda` is the dynamic JavaScript/TypeScript frontend that compiles source into ARK bytecode (`.abc`). Its main executable is `es2abc`; the implementation namespace is `panda::es2panda`.

The primary tools are C++17, GN, Ruby/ERB generation, gtest, and Python. The static ArkTS frontend is implemented separately under `ets2panda`; do not apply its checker/lowering design to this route.

Core capabilities:

- Source scanning, parsing, declaration/identifier binding, TS AST transformation, bytecode generation, and emission.
- JavaScript/TypeScript source input and explicitly enabled ABC recompilation.
- File-, function-, and ABC-class-level parallel work.
- Program-owned arena allocation for AST and supporting data.
- Module records, merged ABC output, patch/reload support, and target-API-sensitive behavior.

Read the repository `AGENTS.md` and `docs/knowledge/ES2Panda_Knowledge_Base.md` before planning or editing. This guide preserves the dynamic route's build, test, modification, and troubleshooting procedures; subdirectory guides provide implementation detail.

## 2. Path conventions

Source and build commands below use two roots:

- **OpenHarmony source root** contains `arkcompiler/`, the build system, runtime repositories, and `build.sh`.
- **Repository root** is `arkcompiler/ets_frontend/` within that source tree.

Paths beginning with `es2panda/`, `test262/`, `testTs/`, `test_ecma_bcopt/`, or `docs/` are repository-relative. Commands with `cd` use the resulting working directory. The test procedure uses absolute variables to avoid resolving build outputs relative to the wrong root.

The sample output layout is Linux x64 host tooling for product `rk3568`: `out/rk3568/clang_x64/arkcompiler/ets_frontend/`. Adapt product and host-toolchain paths to the actual build; this layout does not establish support for an untested host.

## 3. Core module guides

| Module | Guide |
|---|---|
| CLI and compilation driver | `es2panda/aot/AGENTS.md` |
| AST / IR nodes | `es2panda/ir/AGENTS.md` |
| Lexer | `es2panda/lexer/AGENTS.md` |
| Parser and TS transformation | `es2panda/parser/AGENTS.md` |
| Binder | `es2panda/binder/AGENTS.md` |
| Bytecode compiler and emitter | `es2panda/compiler/AGENTS.md` |
| Build helpers | `es2panda/scripts/AGENTS.md` |

There is no in-tree TypeScript checker directory. TS syntax handling and transformation remain in `parser/`, `ir/ts/`, and `parser/transformer/`.

## 4. Quick start

### 4.1 Build the frontend

From the OpenHarmony source root:

```bash
./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages --build-target ets_frontend_build --no-prebuilt-sdk
out/rk3568/clang_x64/arkcompiler/ets_frontend/es2abc --help
```

| Build setting | Purpose |
|---|---|
| `ark_js_host_linux_tools_packages` | JS runtime host tools needed for execution tests |
| `ets_frontend_build` | Frontend build including es2abc |
| `--no-prebuilt-sdk` | Skip prebuilt SDK use for this host-tools workflow |
| `--fast-rebuild` | Incremental rebuild when supported by the surrounding build checkout |
| `--gn-args is_debug=true` | Debug build when needed for investigation |

Use Release by default. For a small implementation change, an incremental frontend rebuild can be run from the same root:

```bash
./build.sh --product-name rk3568 --build-target ets_frontend_build --no-prebuilt-sdk --fast-rebuild
```

Confirm the intended target and product in the surrounding OpenHarmony build configuration before adapting these commands.

### 4.2 Compile and inspect source

```bash
# Run from the OpenHarmony source root.
es2abc=out/rk3568/clang_x64/arkcompiler/ets_frontend/es2abc

"$es2abc" --output output.abc input.js
"$es2abc" --extension ts --output output.abc input.ts
"$es2abc" --dump-ast input.js
"$es2abc" --dump-assembly input.js
```

TS input is parsed and transformed. There is no supported `--enable-type-check` CLI option in the current tree.

## 5. Project structure

```text
es2panda/
├── aot/                       # CLI and output orchestration
│   ├── main.cpp
│   ├── options.cpp
│   ├── emitFiles.cpp
│   └── resolveDepsRelation.cpp
├── lexer/                     # Source → tokens
│   ├── lexer.cpp
│   ├── templates/             # Generated-keyword source templates
│   └── token/                 # Token kinds, payload, source positions
├── parser/                    # Tokens → AST and program metadata
│   ├── parserImpl.cpp
│   ├── expressionParser.cpp
│   ├── statementParser.cpp
│   ├── program/
│   ├── module/
│   └── transformer/            # TS AST transformation
├── binder/                    # Declarations, scopes, identifier binding
├── compiler/                  # AST → instructions and assembled program
│   ├── core/
│   │   ├── compilerImpl.cpp
│   │   ├── pandagen.cpp
│   │   └── emitter/
│   ├── base/                  # Conditions, assignment, environments, exceptions
│   ├── function/              # Normal, async, generator builders
│   └── templates/             # ISA and operand-format templates
├── ir/                        # AST node definitions and node compilation
│   ├── astNode.h
│   ├── astNodeMapping.h
│   ├── expressions/
│   ├── statements/
│   └── ts/
├── util/                      # Strings, workers, caches, patch helpers
├── scripts/                   # Generation and frontend wrappers
├── unittest/                  # C++ unit tests
├── test/
│   ├── runner.py              # Python regression entry
│   └── config.py              # Current target API/version mappings
├── es2panda.cpp               # Compiler orchestration, TS transform/rebind
├── es2panda.h                 # Compiler API, options, input extension
├── BUILD.gn                   # GN build graph
└── es2abc_config.gni           # Bytecode generation build template
```

## 6. Critical project conventions

### 6.1 Generated code

Edit source inputs rather than generated files:

| Generated file | Source of truth |
|---|---|
| `isa.h` | `es2panda/compiler/templates/isa.h.erb` plus runtime ISA data/helpers |
| `formats.h` | `es2panda/compiler/templates/formats.h.erb` plus runtime ISA data/helpers |
| `keywords.h` | `es2panda/lexer/templates/keywords.h.erb` and `lexer/scripts/keywords.rb` |
| `keywordsMap.h` | `es2panda/lexer/templates/keywordsMap.h.erb` and `lexer/scripts/keywords.rb` |

GN actions provide generator arguments, dependencies, prepared input data, and output paths. Normal development should rebuild those actions instead of manually invoking wrappers with guessed inputs. Inspect `BUILD.gn` to locate the actual generated directory for the product/toolchain.

Workflow:

1. Identify and edit the relevant ERB template, keyword input, or ISA schema/helper.
2. Rebuild the frontend through GN.
3. Inspect generated changes without manually editing them.
4. Compile/link the consumer and run focused regressions.

### 6.2 ScriptExtension and compilation order

`ScriptExtension` is declared in `es2panda.h` with `JS`, `TS`, `AS`, and `ABC` values. A value's existence does not imply every input path accepts it; inspect CLI validation and dispatch.

```text
Source → lexer → parser/binder
       → TS transformation and identifier rebinding when applicable
       → compiler / PandaGen → emitter → ABC output
```

`Compiler` constructs `parser::Transformer` for TS input. `Compiler::ProcessAstForTS` performs transformation and `Binder::IdentifierAnalysis(TS_AFTER_TRANSFORM)`. There is no optional in-tree TS checker stage after transformation.

ABC input follows its own recompilation path and requires `--enable-abc-input`.

### 6.3 API and bytecode versions

Bytecode behavior can depend on target API and bytecode version. Inspect current gates and `es2panda/test/config.py`; do not freeze its supported versions to an old API 9–20 list.

Notify the user before changing generated bytecode. Ask before changing serialization/ABC compatibility behavior. Validate both affected and adjacent supported API versions, and coordinate ISA/version changes with the runtime owners where required.

## 7. Test selection and required coverage

Read `docs/knowledge/Build_Environment.md` to set and validate paths, then follow `docs/knowledge/ES2Panda_Testing.md` for the complete ordered procedure, commands, prerequisites, result comparisons, and acceptance criteria.

| Change / check | Select first | Required supporting coverage |
|---|---|---|
| Lexer/parser/binder | C++ unit tests and parser regression | AST/bindings plus emitted behavior if changed |
| Compiler/TS transformation | Compiler and affected runtime cases | Both Test262 modes and route regressions |
| Patch/reload | Hot/cold patch and reload suites | Matching baseline, symbols and runtime |
| Encoding/debug/version | Base64, bytecode, debugger, version suites | Matching target API/runtime |
| Runtime/verifier/toolchain | Runtime UT, verifier, disassembler/fuzz build | Distinguish build checks from execution |
| Instruction/ABC format | ISA version self-check | Bytecode compatibility and domain coordination |
| Legacy TS / optimizer | Legacy TestTs or optimizer fixture procedure | Actual compatible tools; report missing consumers |

For dynamic compiler code changes, the full testing procedure retains the required unit/integration suites, both Test262 modes, runtime_core UT, verifier, disassembler and fuzz build checks, and applicable version self-check. Focused tests supplement that coverage. Use Release unless Debug is required; record configuration, commands, logs, failures, skips, and unexecuted checks.

## 8. Compiler option reference

Use the actual built binary and its `--help` output. Common command shapes:

```bash
es2abc --dump-ast input.js
es2abc --dump-transformed-ast --extension ts input.ts
es2abc --dump-assembly input.js
es2abc --dump-debug-info input.js
es2abc --dump-size-stat input.js
es2abc --target-api-version=<supported_api> input.js
es2abc --enable-abc-input input.abc
es2abc --extension ts input.ts
es2abc --opt-level 2 input.js
es2abc --module input.js
```

Optimization currently defaults to level 2. Verify target API values against the current compiler and runtime; do not assume an older example API is suitable for a new feature.

## 9. Common implementation tasks

### 9.1 Add an AST node

1. Add the actual node/class mapping in `ir/astNodeMapping.h`; inspect enum/check consumers in `ir/astNode.h`.
2. Implement the node under the appropriate IR directory.
3. Implement `Iterate`, `Dump`, `Compile`, and `UpdateSelf` following adjacent nodes.
4. Update parser construction, binder behavior, TS transformation, and build source registration as applicable.
5. Run AST/regression tests and emitted-behavior tests.

Do not invent a `PandaGen::CompileMyExpression` or `AstDumper::Visit` API when the existing node-specific interfaces already own that behavior.

### 9.2 Add or modify an instruction

1. Notify the user of generated-bytecode impact and identify schema/template/runtime consumers.
2. Update the actual runtime ISA inputs and/or frontend ERB templates.
3. Rebuild generated headers through GN rather than editing generated files or invoking wrappers with guessed arguments.
4. Update node/PandaGen/emitter consumers where required.
5. Check instruction formats, target API/version gates, verifier behavior, runtime results, and ABC compatibility.

## 10. Dependencies

### 10.1 External dependencies

| Dependency | Responsibility |
|---|---|
| Runtime base libraries | Allocation, strings, platform support |
| ICU integration | Unicode support; inspect current GN labels and linkage |
| Assembler | Assembled program representation and ABC emission |
| ABC-to-program support | ABC input and recompilation |
| Bytecode optimizer | Optimization support for enabled build paths |

### 10.2 Internal target relationships

Inspect `es2panda/BUILD.gn` for the actual `es2panda_lib` and executable target definitions, source groups, generated-header dependencies, and conditional runtime/optimizer dependencies. The executable is named `es2abc`; target labels and artifact filenames need not be identical.

Before changing GN, trace generated inputs and the nearest downstream consumer. Compile and link with the intended host toolchain; a header-only check does not prove final link compatibility.

## 11. Troubleshooting

### 11.1 GN build failure

1. Confirm the working directory is the OpenHarmony source root and the product/host paths match the intended output.
2. Inspect the first failing target and command, not just the final aggregate failure.
3. Check build tool availability and the generated action's input/dependency graph.
4. Rebuild the affected target after fixing the cause.
5. Report unavailable host branches or companion runtime/build prerequisites explicitly.

```bash
command -v gn
gn --version
```

Do not delete the entire `out/` tree as a routine fix. Investigate the failing action first; destructive cleanup requires the applicable approval and should be scoped to the diagnosed issue.

### 11.2 Incorrect generated output

Check source template/schema, actual action arguments, generated directory, consuming includes, and dependency ordering. A manually edited generated file can hide the root cause and disappear at the next build.

### 11.3 Test environment failure

Distinguish compiler diagnostics from a missing runtime, shared library, external TS fixture, legacy Node frontend, or incompatible bytecode consumer. Fix/report the prerequisite rather than weakening the expected result.

## 12. Performance and ownership

AST nodes use the program's arena allocator. Preserve arena ownership and source-backed string lifetimes; do not add per-node heap ownership that outlives the program accidentally.

Before changing parallelism, caches, register allocation, or source scanning, measure representative inputs and verify deterministic output and failure propagation.

## 13. Version and artifact information

- Build system: GN within the OpenHarmony source tree.
- C++ standard: inspect the active build configuration; the frontend is developed with C++17 conventions.
- License: Apache License 2.0.
- Main artifact: `es2abc`, with frontend library and generated-header build targets.
- Compatibility is shared with runtime/assembler/optimizer consumers and is not fully defined by this repository alone.
