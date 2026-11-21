# Scripts Module - AGENTS.md

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Module responsibilities

Build helpers generate ISA and keyword headers and invoke the native frontend to produce bytecode. The Node wrapper launches `es2abc`; it is not an independent TypeScript checker or transformer.

Read the repository `AGENTS.md`, `es2panda/AGENTS.md`, and `docs/knowledge/ES2Panda_Knowledge_Base.md`. Paths here are repository-relative unless otherwise stated.

## 2. Directory structure

```text
es2panda/scripts/
├── gen_isa.sh                # ISA generator wrapper
├── gen_keywords.sh           # Keyword generator wrapper
├── generate_js_bytecode.py   # Native es2abc invocation helper
└── ts2abc.js                 # Node wrapper around native es2abc
```

## 3. ISA instruction generation

### 3.1 gen_isa.sh

The wrapper receives the generator, template, ISA data, output directory/name, and Ruby helper from its caller. It invokes the supplied generator with `--template`, `--data`, `--output`, and `--require`.

| Wrapper argument | Meaning |
|---|---|
| `-g` | Generator executable |
| `-t` | ERB template |
| `-a` | ISA data |
| `-o` | Output filename |
| `-d` | Output directory |
| `-r` | Required helper |

The ISA templates are `es2panda/compiler/templates/isa.h.erb` and `formats.h.erb`. Runtime ISA inputs and generator code belong to companion `arkcompiler/runtime_core/`.

### 3.2 Modification workflow

1. Determine whether the source of truth is the instruction schema, helper, template, or build wiring.
2. Edit that source, preserving generated instruction interfaces.
3. Rebuild the relevant GN target to regenerate the headers.
4. Compile and link the frontend consumer, then run instruction regressions.

Do not hand-edit generated `isa.h` or `formats.h`. Normal builds provide the correctly prepared inputs; ad hoc wrapper invocation is not a substitute for checking the build dependency graph.

## 4. Keyword generation

### 4.1 gen_keywords.sh

The wrapper runs Ruby with a generator, template, and output path. `-g`, `-t`, `-o`, and `-d` supply those values.

The generator is `es2panda/lexer/scripts/keywords.rb`. Templates are `es2panda/lexer/templates/keywords.h.erb` and `keywordsMap.h.erb`.

### 4.2 Modification workflow

1. Change the keyword input and/or template that owns the behavior.
2. Rebuild through GN.
3. Check both generated definitions and string lookup behavior.
4. Run lexer and parser regressions, including identifier/contextual-keyword cases.

Do not update one generated header while leaving the other stale.

## 5. Python bytecode helper

### 5.1 generate_js_bytecode.py

The helper constructs an `es2abc` command from parsed arguments and runs it with `subprocess.Popen`. It forwards source/destination paths and selected module, debug, patch, annotation, and compatibility options.

The `es2abc_gen_abc` build template uses this helper; inspect its definition in `es2panda/es2abc_config.gni` before changing argument names or path assumptions.

### 5.2 Usage and error handling

Use `python3 es2panda/scripts/generate_js_bytecode.py --help` to inspect the current interface. The frontend-tool argument determines the executable directory and the subprocess working directory; relative paths must be valid there.

A nonzero frontend exit is propagated as failure. Do not swallow that failure or treat the presence of a partial ABC file as success. Check debug/non-debug paths, module/CommonJS options, symbol-table inputs, and target API options when modifying forwarding.

### 5.3 Adding a forwarded option

1. Register the Python argument with the appropriate value or boolean behavior.
2. Forward the exact native option spelling and value.
3. Update the GN caller if the build exposes the setting.
4. Test both unset/default behavior and explicit values.
5. Rebuild the nearest consumer to catch mismatched argument contracts.

## 6. Node frontend wrapper

### 6.1 ts2abc.js

`ts2abc.js` uses Node's `child_process.spawn` to launch native `es2abc` and forward its output. An initial `es2abc=<path>` argument overrides automatic executable discovery; the remaining arguments go to the native frontend.

```bash
node es2panda/scripts/ts2abc.js es2abc=/absolute/path/to/es2abc --output output.abc input.ts
```

Without an explicit binary, the script looks for platform-specific build directories relative to the script directory. Check the current discovery branches before diagnosing a missing-binary error.

This wrapper is separate from the legacy Node frontend expected by `testTs/run_testTs.py`. That suite's `index.js` contract must not be replaced merely because this wrapper also uses Node.

## 7. Related modules and tools

| Input / consumer | Role |
|---|---|
| `es2panda/compiler/templates/` | ISA templates |
| `es2panda/lexer/templates/` | Keyword templates |
| `es2panda/lexer/scripts/keywords.rb` | Keyword generator |
| Companion `arkcompiler/runtime_core/isa/` | ISA data and generator support |
| `es2panda/compiler/core/` | Generated instruction consumers |
| `es2panda/lexer/` | Generated keyword consumers |
| `es2panda/BUILD.gn`, `es2panda/es2abc_config.gni` | Build actions, inputs, dependencies, invocation |

Ruby/ERB runs generation, Python integrates bytecode commands, and Node hosts the native-binary wrapper. Do not add a dependency or assume a host tool is universally installed without checking the intended build environment.

## 8. Verification

Script-only changes still need validation of their actual callers. For generation changes, rebuild the generated headers and compile/link their consumers. For argument-forwarding changes, run the helper with an actual frontend and a small valid input, plus an invalid input that must fail. Run nearby regression cases and report missing binaries or build inputs explicitly.
