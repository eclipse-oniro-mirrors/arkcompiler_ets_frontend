# ETS2Panda Public API Knowledge Base

**Version:** v1.0 | **Last updated:** 2026-06-01

This document covers the C API code generation pipeline under `ets_frontend/ets2panda/public/`. It does NOT cover AST node semantics, checker internals, parser internals, varbinder, or lowering phases (see `Static_Frontend_Knowledge_Base.md`), nor LSP API behavior (see `LSP_Knowledge_Base.md`).

**Before modifying:** read `ets2panda/public/AGENTS.md` and `ets2panda/public/README.md`.

## Core Model

`public/` exposes ets2panda's internal C++ API as a **stable, auto-generated C interface** (`es2panda_lib`). This is the single public contract between compiler internals and all external consumers. The C API must never be hand-edited.

| Step | Input | Tool | Output | Failure mode |
|---|---|---|---|---|
| 1. Parse | C++ headers in `HEADERS_TO_BE_PARSED` | `headers_parser/main.py` | YAML under `<build>/gen/headers/` | Silent empty YAML -- check `<build>/gen/logs/` |
| 2. Generate | YAML + `cppToCTypes.yaml` + `ignoredAllowed.yaml` | `es2panda_lib.rb` | `es2panda_lib.h/.cpp/.inc/.idl` | UNSUPPORTED TYPE error |
| 3. Compile | Generated C code | `ninja es2panda-public` | Linked into `es2panda` binary | Compilation failure from bad cast expressions |

## Responsibility Boundaries

| Responsible for | Not responsible for |
|---|---|
| Generating the stable C API via the automated codegen pipeline | Defining semantics of AST nodes, types, or compiler operations |
| Maintaining `cppToCTypes.yaml` as the single source of truth for C-to-C++ type translation | Fixing C++ source headers to make them "codegen-friendly" |
| Maintaining `ignoredAllowed.yaml` export filter | Maintaining backward compatibility without bumping `ES2PANDA_LIB_VERSION` |
| Generating `es2panda_lib.idl` synchronized with the C API | Removing C API methods without checking downstream consumers |
| Parsing C++ headers into structured YAML | Hand-editing generated files or bypassing the codegen pipeline |
| Providing `public_lib::Context` bridging C API to internal compiler state | Adding exported API for types outside the intended public surface |

## Constraint Rules

### MUST

- **MUST** run `ninja gen_api` after any change to headers, `cppToCTypes.yaml`, `ignoredAllowed.yaml`, or ERB templates.
- **MUST** add the corresponding `ES2PANDA_API_GENERATED` entry when adding a header to `HEADERS_TO_BE_PARSED` (one-to-one).
- **MUST** update `es2panda_lib.idl.erb` in the same patch as any `es2panda_lib.h` change.
- **MUST** place specific type entries before wildcard entries in `cppToCTypes.yaml` (first-match-wins).
- **MUST** set `min_ptr_depth` and `max_ptr_depth` precisely on every `cppToCTypes.yaml` entry.
- **MUST** check `<build>/gen/logs/` after `ninja gen_yamls` -- parser silently produces empty YAML on failure.
- **MUST** increment `ES2PANDA_LIB_VERSION` on A/B-incompatible API changes.

### NEVER / DO NOT

- **NEVER** hand-edit generated files: `es2panda_lib.h`, `es2panda_lib.cpp`, or any `.inc` file.
- **NEVER** create manual C API function wrappers outside the codegen pipeline.
- **NEVER** place specific type mappings after wildcard (`\|AstNode\|`) rules in `cppToCTypes.yaml`.
- **NEVER** add a class to `ignoredAllowed.yaml`'s `call_class` without checking downstream consumers.
- **DO NOT** modify `es2panda_lib.rb` without a full `ninja gen_api` + `ninja es2panda-plugin-test` cycle.
- **DO NOT** hand-edit generated files (`es2panda_lib.h`, `es2panda_lib.cpp`, `.inc` files). Whether generated files are tracked in the source tree is a repo-level decision — check the current tree state. If tracked, they MUST be regenerated and committed together with the source-of-truth change, never patched manually.

### Key file traps

Source-of-truth priority when debugging a mismatch: 1) C++ header in `HEADERS_TO_BE_PARSED` (what is exported) → 2) `cppToCTypes.yaml` (how types translate) → 3) `ignoredAllowed.yaml` (what is filtered out) → 4) ERB templates (how C code is structured). Always fix the earliest step in this chain.

| File | Trap |
|---|---|
| `es2panda_lib.h/.cpp` | Auto-generated. Opaque typedefs must not be dereferenced in C code. |
| `es2panda_lib.rb` | Hand-written. `@classes`/`@ast_nodes` are module-global; state accumulates across YAML files. |
| `cppToCTypes.yaml` | Missing `call_cast` breaks method calls. Missing `constructor_cast` breaks constructors. `return_args` only when return expands to multiple parameters. |
| `ignoredAllowed.yaml` | Filter order: postfix_contains → args → template_types → return_type → call_class. `ignored_list` overrides `allowed_list`. |
| `CMakeLists.txt` | `HEADERS_TO_BE_PARSED` is the single source of truth for exported headers. |
| `headers_parser/` | Silent failure on parse errors (check `${LIBGEN_DIR}/gen/logs/`). |
| `public.h` | `Context::AllocNode<T>()` uses `ForceSetParent`. `ClearCheckers()`/`ClearAnalyzers()` do not deallocate -- caller must release. |

## Actions Forbidden Without Team Confirmation

| Action | Reason |
|---|---|
| Hand-edit `es2panda_lib.h` non-generated portions | Will be overwritten; breaks contract |
| Change `ES2PANDA_LIB_VERSION` | Signals A/B breakage; downstream must be notified |
| Remove entries from `ignoredAllowed.yaml` `call_class` or `allowed_list` | Expands/shinks API surface silently |
| Modify `Arg`/`Type`/`ClassData` matching logic in `es2panda_lib.rb` | Affects all type codegen |
| Rewrite `headers_parser/` parsing strategy | Affects all YAML generation |
| Add entire new subsystem to `HEADERS_TO_BE_PARSED` | Requires maintenance cost evaluation |
| Delete/rename published C API functions | Use deprecation instead |
| Modify `Context` struct layout in `public.h` | Affects all C API implementation functions |
| Add third-party dependencies to codegen pipeline | Requires team review |
| Hand-patch generated files instead of regenerating | Regenerate via `ninja gen_api` and commit the result; never apply manual fixes to generated output |

## Anti-Patterns

| Anti-Pattern | Correct Approach |
|---|---|
| Hand-editing generated files | Modify source of truth and regenerate |
| Updating `es2panda_lib.h` without `es2panda_lib.idl.erb` | Same patch, always |
| Omitting `min_ptr_depth`/`max_ptr_depth` in `cppToCTypes.yaml` | Always set precise ranges |
| Specific type after wildcard in `cppToCTypes.yaml` | Specific before `\|AstNode\|` |
| `HEADERS_TO_BE_PARSED` without `ES2PANDA_API_GENERATED` | Must be one-to-one |
| Skipping header parser log review after `ninja gen_yamls` | Always check `<build>/gen/logs/` |
| Manual C API wrappers outside codegen pipeline | All C API through codegen |
| Modifying `es2panda_lib.rb` without full rebuild cycle | `ninja gen_api` + compile verification |

## Pre-Modification Checklist

- [ ] Is this a generated file? (If so, hand-editing is forbidden.)
- [ ] New class/method: in correct C++ header, header in `HEADERS_TO_BE_PARSED`?
- [ ] New C++ type: correct `cppToCTypes.yaml` entry with ptr_depth and cast expressions?
- [ ] Methods/constructors to hide: correct `ignoredAllowed.yaml` filters?
- [ ] `es2panda_lib.h` changed: `es2panda_lib.idl.erb` updated in same patch?
- [ ] `ninja gen_api` run and generated code compiles?
- [ ] New methods have unit tests (`test/unit/public/e2p_test_plugin*`)?
- [ ] `es2panda_lib.rb`/`enums.rb` modified: `ninja es2panda-plugin-test` passed?

## Code and Test Anchors

### Code entry points

| Component | Path |
|---|---|
| Codegen engine | `ets_frontend/ets2panda/public/es2panda_lib.rb` |
| Enum extraction | `ets_frontend/ets2panda/public/enums.rb` |
| Type mappings | `ets_frontend/ets2panda/public/cppToCTypes.yaml` |
| Export filters | `ets_frontend/ets2panda/public/ignoredAllowed.yaml` |
| Context bridge | `ets_frontend/ets2panda/public/public.h` / `public.cpp` |
| Build pipeline | `ets_frontend/ets2panda/public/CMakeLists.txt` |
| ERB templates | `ets_frontend/ets2panda/public/*.inc.erb`, `*.idl.erb` |
| Header parser entry | `ets_frontend/ets2panda/public/headers_parser/main.py` |
| Generated output | `<build>/tools/es2panda/generated/es2panda_lib/` |

### Test commands

```bash
# From build directory
ninja es2panda-plugin-test    # Unit tests for C API plugin
ninja gen_yamls               # Parse headers → YAML
ninja gen_api                 # YAML → generated C/IDL
ninja es2panda-public         # Compile generated C code
```

Generated output: `<build>/tools/es2panda/generated/es2panda_lib/`.
Parser logs: `<build>/tools/es2panda/generated/es2panda_lib/gen/logs/`.

## New Public API — Complete Chain Quick-Reference

1. Add class/method to correct C++ header under `ets_frontend/ets2panda/`.
2. Add header to `HEADERS_TO_BE_PARSED` in `CMakeLists.txt` + corresponding `ES2PANDA_API_GENERATED` entry.
3. Add type mapping in `cppToCTypes.yaml` (specific before wildcard, precise `min_ptr_depth`/`max_ptr_depth`, required cast expressions).
4. Hide unwanted methods via `ignoredAllowed.yaml` if needed.
5. Update `es2panda_lib.idl.erb` if API surface changes.
6. Run `ninja gen_api` from build directory → verify generated output.
7. Run `ninja es2panda-public` → verify compilation.
8. Add unit test in `test/unit/public/e2p_test_plugin*`.
9. Run `ninja es2panda-plugin-test` → verify all plugin tests pass.

## Verification Commands

**Minimal local** (from build directory, after each change):
```bash
ninja gen_yamls && ninja gen_api && ninja es2panda-public
```

**Release-grade** (before submitting):
```bash
ninja gen_yamls && ninja gen_api && ninja es2panda-public && ninja es2panda-plugin-test
```

### Done Definition

A public API task is done only when: (1) C++ header correctly expresses intended export; (2) `HEADERS_TO_BE_PARSED` / `cppToCTypes.yaml` / `ignoredAllowed.yaml` correctly updated; (3) `es2panda_lib.idl.erb` in same patch as `es2panda_lib.h` change; (4) `ninja gen_api` produces expected output without errors; (5) generated code compiles; (6) all unit tests pass; (7) no hand-patches remain in generated files.

## Related Documents

- `ets2panda/public/AGENTS.md` -- Directory development rules
- `ets2panda/public/README.md` -- Detailed codegen docs and type-adding guide
- `ets2panda/public/headers_parser/README.md` -- Header parser documentation
- `docs/ets2panda/Static_Frontend_Knowledge_Base.md` -- Checker types exposed through C API
- `AGENTS.md` -- Repository-level routing, constraints, and verification expectations
