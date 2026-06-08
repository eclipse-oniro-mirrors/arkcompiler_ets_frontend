# ETS2Panda DeclGen ETS2TS Knowledge Base

> Document Version: v1.0
> Last Updated: 2026-05-28
> Scope: `ets_frontend` frontend compiler knowledge base entry
> Before modifying, please read: `AGENTS.md`, the corresponding compiler route `AGENTS.md`, and the nearest directory-level `AGENTS.md`

## Overview

`declgen_ets2ts` generates Dynamic ArkTS declaration files (`.d.ets`) and glue code (`.ts`) from a Program AST that has completed type checking (`CHECKED` state). It is a key component of the interoperability chain between the ArkTS static frontend and the TypeScript ecosystem.

## Directory Structure and Code Map

- Upstream: ets2panda compiler pipeline (Program AST in `CHECKED` state) + `ETSChecker` (type information)
- Downstream: `.d.ets` declaration files (consumed by the TypeScript side) + `.ts` glue code (interop runtime bridge); invoked by the declgen phase of `build_system` (driver)

## Directory Explanation

- `declgenEts2Ts.h` / `declgenEts2Ts.cpp` — Core generation engine, containing the `TSDeclGen` class and top-level entry function `GenerateTsDeclarations()`
- `isolatedDeclgenChecker.h` / `isolatedDeclgenChecker.cpp` — Isolated-mode validator `IsolatedDeclgenChecker`; does not depend on the global symbol table; checks whether properties/parameters/functions have explicit type annotations
- `main.cpp` — Standalone CLI entry point; parses `--export-all`, `--output-dets`, `--output-ets`, and other arguments; drives the full compilation pipeline via the `es2panda_lib` public API before calling `GenerateTsDeclarations()`
- `declgen_ets2ts_error.yaml` — Error diagnostic definitions (IDENT_KEY_SUPPORT, UNSUPPORTED_LITERAL_TYPE, etc.; 10 entries total)
- `declgen_ets2ts_warning.yaml` — Warning diagnostic definitions (EMPTY_TYPE_NAME, UNTYPED_METHOD; 2 entries total)
- `isolated_declgen.yaml` — Diagnostic definitions for isolated declaration generation mode (variables/properties/parameters must have explicit type annotations, etc.; 8 entries total)
- `CMakeLists.txt` / `BUILD.gn` — Build configuration

## Key Files and Responsibilities

- `declgenEts2Ts.h` / `declgenEts2Ts.cpp`
  - `TSDeclGen` class: Core declaration generation engine; holds `ETSChecker*`, `IsolatedDeclgenChecker*`, `parser::Program*`; maintains `dependencySet_` (type dependencies), `importSet_` (import statements), `exportSet_` (export statements), `glueCodeImportSet_` (glue code imports), `paramDefaultMap_` (parameter default values); outputs declaration content and glue code into two string streams `outputDts_` and `outputTs_` respectively
  - `GenerateTsDeclarations(checker, program, declgenOptions)` — Top-level entry: initializes `TSDeclGen`, executes the full generation pipeline, calls `WriteOutputFiles()` to write output files
  - `ValidateDeclgenOptions()` — Validates `DeclgenOptions` parameters
  - `WriteOutputFiles()` — Writes the generated declaration content to `.d.ets`/`.d.ts` and `.ets` files
  - `DeclgenOptions` struct: `exportAll` (treat all top-level statements as exported), `isolated` (enable isolated mode), `outputDeclEts` (output declaration file path), `outputEts` (output glue code path), `recordFile` (declaration record file), `genAnnotations` (whether to generate annotations, default true)
- `isolatedDeclgenChecker.h` / `isolatedDeclgenChecker.cpp`
  - `IsolatedDeclgenChecker` class: Performs isolated checks on `ir::ScriptFunction`, `ir::ClassProperty`, `ir::ETSParameterExpression`, `ir::ExportDefaultDeclaration`, `ir::ArrayExpression`; enforces explicit type annotations; reports `isolated_declgen` diagnostics for cases that cannot be inferred
- `main.cpp` — CLI tool: filters out declgen-specific arguments, obtains the compiler API via `es2panda_GetImpl()`, drives the `PARSED → CHECKED` state transition, then calls `GenerateTsDeclarations()`

## Responsibility Boundaries

- Responsible for: executing declaration generation on a `CHECKED`-state AST; generating `.d.ets` type declaration files and `.ets` interop glue code; handling `@noninterop` annotation; validating explicit type annotations in isolated mode
- Not responsible for: parser/lexer phase (AST construction); type checking itself (`ETSChecker`); lowering/IR/emit phases of the main compiler pipeline; fixing semantic errors outside `declgen_ets2ts_error.yaml`

## Core Data Flow / Control Flow

```
ets2panda compiler (CHECKED state) / CLI (parse → check)
    → GenerateTsDeclarations(ETSChecker*, Program*, DeclgenOptions)
        → ValidateDeclgenOptions()
        → TSDeclGen::Generate()
            1. GenGlobalDescriptorInit()      (initialize ETSGLOBAL, runtime interop descriptor)
            2. Traverse AST to collect dependencySet_   (type dependency collection)
            3. Collect glueCodeImportSet_               (glue code runtime imports)
            4. GenExportNamedDeclarations()             (class/interface/type alias declaration generation)
            5. Generate export statements               (exportSet_)
            6. GenInitModuleGlueCode()                  (glue code module init calls)
            7. GenImportDeclarations()                  (import statement generation, done last to ensure correct ordering)
        → WriteOutputFiles()
            → write outputDeclEts (.d.ets / .d.ts)
            → write outputEts (.ets glue code)
```

Isolated mode additional flow:
```
IsolatedDeclgenChecker::Check()
    → Traverse functions/properties/parameters, check for explicit type annotations
    → Report isolated_declgen diagnostics (VARABLE_MUST_HAVE_EXPLICIT_TYPE_ANNOTATION, etc.)
```

## Knowledge Routing

- Generated declaration content is incorrect or missing → this document (inspect `TSDeclGen::GenExportNamedDeclarations` and the corresponding `GenType`)
- Isolated mode reports "must have explicit type annotation" → this document (`IsolatedDeclgenChecker`, see `isolated_declgen.yaml`)
- Type resolution error during declaration generation → first check whether checker has completed (whether the AST is in `CHECKED` state)
- Output file path / parameter configuration issues → check `DeclgenOptions` and `declgenV1OutPath`/`declgenV2OutPath` passed by `build_system`
- Upstream AST issues (parse errors, checker not completed) → `Static_Frontend_Knowledge_Base.md`
- declgen trigger logic on the `build_system` side → `Driver_Knowledge_Base.md`

## Expert Tips

- First confirm the input AST is in `CHECKED` state (`ctxImpl->lazyCheck = false` and `ProceedToState(ES2PANDA_STATE_CHECKED)`); otherwise type information in `ETSChecker` is incomplete and the generated output will be missing or incorrect
- To generate `.d.ts` (TypeScript-format declarations), `outputDeclEts` must end with `.d.ts` (determined by `IsTypeScriptDeclarationOutput()`); `.d.ets` is the ArkTS declaration format
- Modules annotated with `@noninterop` (`NON_INTEROP_FLAG`) do not participate in interop glue code generation; be aware of the distinction
- In isolated mode (`isolated: true`), `IsolatedDeclgenChecker` requires explicit type annotations on all properties, parameters, and function return values; this enables fast validation of whether a source file is declaration-ready without running the full global checker
- When `genAnnotations: true` (default), UI and local annotations (`@Component`, `@State`, etc.) are generated; if unexpected annotations appear in the declaration file, check this flag

## Anti-Patterns

- Using declgen to paper over errors from upstream phases (parser or checker issues should be fixed at the respective phase; declgen assumes the input AST is correct and complete)
- Introducing dependencies on checker internal private state inside `TSDeclGen` (type information should be accessed via `ETSChecker`'s public interfaces)
- Calling `GenerateTsDeclarations()` in the CLI tool while bypassing `ProceedToState(CHECKED)` (type information will be incomplete, output will be wrong)

## Debugging and Verification

- Run CLI tool standalone: `declgen_ets2ts --output-dets=out.d.ets --output-ets=out.ets input.ets`
- Invocation via `build_system`: set `enableDeclgenEts2Ts: true` and configure `declgenV1OutPath`, then trigger through the build system
- Related tests: declgen-related test cases under `ets2panda/test/` (including `.d.ets` and `.ets` expected output comparisons)
- Diagnostic definitions: `declgen_ets2ts_error.yaml` (10 errors), `declgen_ets2ts_warning.yaml` (2 warnings), `isolated_declgen.yaml` (8 isolated-mode diagnostics)

## Debugging Methods

- Compare the input AST (output via the `--dump-ast` flag) with the generated declarations to identify which AST node corresponds to which declaration output
- Enable `#define DEBUG_PRINT 1` in `declgenEts2Ts.cpp` to turn on internal debug printing
- Check `diagnosticEngine` for any diagnostics of type `declgen_ets2ts_error`/`declgen_ets2ts_warning`/`isolated_declgen`
- Isolated mode debugging: call `IsolatedDeclgenChecker::Check(scriptFunction)` directly and inspect the returned inferred type string

## Common Issues

- Generated declaration types do not match actual types: usually caused by checker not running fully (`lazyCheck` not disabled) or AST not reaching `CHECKED` state
- Isolated mode reports `FUNCTION_MUST_HAVE_AN_EXPLICIT_RETURN_TYPE_ANNOTATION_WITH_ISOLATED_DECL`: function is missing an explicit return type annotation; add it in the source
- `UNSUPPORTED_LOCAL_BINDINGS` (error id 9): declgen encountered a binding valid only in local scope that cannot be expressed in a declaration file
- Output `.d.ets` file is empty or contains only imports: no exported top-level declarations in the source file; try the `--export-all` argument
- `WriteOutputFiles` fails: the output path directory does not exist; ensure the parent directories of `outputDeclEts` and `outputEts` are created beforehand

## Related Documents

- `docs/ets2panda/Static_Frontend_Knowledge_Base.md` -- upstream parser/checker/lowering pipeline context
- `docs/ets2panda/Driver_Knowledge_Base.md` -- build-system trigger path for declgen
- `ets2panda/declgen_ets2ts/AGENTS.md` -- directory-level architecture overview and constraints
- `ets2panda/docs/lowering-phases.md` -- compiler pipeline phase overview
