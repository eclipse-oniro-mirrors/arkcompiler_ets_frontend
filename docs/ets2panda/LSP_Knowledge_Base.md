# ETS2Panda LSP Knowledge Base

> Document version: v1.2
> Last updated: 2026-06-04
> Scope: C++ language service implementation under `ets_frontend/ets2panda/lsp`, plus the TypeScript call chain under `ets2panda/bindings/src/lsp`
> Read before modifying: `docs/AGENTS.md`, `ets2panda/AGENTS.md`, `ets2panda/lsp/AGENTS.md`; also read `ets2panda/bindings/Bindings_Knowledge_Base.md` when touching TypeScript wrappers

## Critical Boundary

- Do not add hidden language semantics in the LSP layer. If a result depends on parser, checker, or varbinder semantics, fix or expose that semantic information in the upstream component instead of re-implementing it in `lsp/`.
- LSP feature modules must not call `CreateContext` / `createContext`, reparse files, or construct AST nodes to obtain results. The LSP call chain should use the globally unique context and cache lifecycle maintained by `bindings/src/lsp/lsp_helper.ts` and the native bridge.
- `lsp/` may adapt compiler data for editor behavior, but it must not make compiler results and editor results diverge.

## Overview

- Module role: `lsp/` is the C++ implementation layer for ArkTS/ETS/TypeScript language service features. It exposes go to definition, find references, rename, completions, hover / quick info, signature help, diagnostics, code fixes, refactors, formatting, class hierarchy, inlay hints, and related features.
- Call relationship: `bindings/src/lsp` is the TypeScript-side entry point. It handles project configuration, context/cache management, incremental compilation, cross-file dependencies, path conversion, and position conversion, then calls the C++ LSP API declared in `lsp/include/api.h` through the native bridge.
- Input / output: Inputs are usually `es2panda_Context`, file names, source positions, user preferences, or formatting settings. Outputs are structures that LSP consumers can convert, such as `DefinitionInfo`, `References`, `CompletionInfo`, `QuickInfo`, `DiagnosticReferences`, `FileTextChanges`, and `RefactorEditInfo`.
- Typical issue types: empty LSP API results, incorrect offsets, missing cross-file references, missing completion items, diagnostics not matching code fixes, incorrect refactor edits, unexpected formatting, or unsynchronized TS bindings.

## Global Regression Hotspots

These are the 10 entry points most likely to cause broad regressions. Review their downstream users and run both C++ LSP and bindings tests when changing them.

1. `include/api.h`, `src/api.cpp`, and `g_lspImpl`: public `LSPAPI` shape, offset conversion, `SetPhaseManager`, and return-value wrapping.
2. `include/internal_api.h`, `src/internal_api.cpp`: `GetTouchingTokenRightMatch`, `GetTouchingToken`, owner/declaration lookup, diagnostics conversion, and import path helpers.
3. `include/lsp_utils.h`, `src/lsp_utils.cpp`, `include/line_column_offset.h`, `src/line_column_offset.cpp`: byte offset, code point offset, and line/column conversion.
4. `src/symbol_reference_index.cpp`: global symbol reference index for references, file references, and cross-file symbol lookup.
5. `bindings/src/lsp/lsp_helper.ts`: context/cache lifecycle, incremental compilation, dependency invalidation, declaration file handling, and symbol index build/remove calls.
6. `bindings/native/src/lsp.cpp`, `bindings/src/common/Es2pandaNativeModule.ts`, `bindings/src/lsp/lspNode.ts`: native/TS signature and structure mapping.
7. `include/code_fix_provider.h`, `src/code_fix_provider.cpp`, `include/code_fixes/code_fix_types.h`, `src/register_code_fix/*`: diagnostic-code dispatch, code actions, and fix-all behavior.
8. `include/refactor_provider.h`, `src/refactor_provider.cpp`, `include/refactors/refactor_types.h`, `src/refactors/*`: applicable refactors and edit generation.
9. `include/formatting/*`, `src/formatting/*`: document/range/keystroke formatting rules and token context.
10. `CMakeLists.txt`, `BUILD.gn`, and generated code fix registration: build coverage and generated registration consistency.

## Directory Structure And Code Map

- Upstream stages:
  - `parser/`, `checker/`, `varbinder/`, and `compiler/lowering/` provide ASTs, types, symbols, declaration ownership, and transformed node information.
  - `public/` and `public/es2panda_lib.h` provide public compiler APIs such as context creation, state progression, and context destruction.
  - `util/diagnostic/*.yaml` and `declgen_ets2ts/*.yaml` provide diagnostic codes and code fix generation inputs.
- Current module:
  - `include/`: C++ LSP public headers, data structures, and feature declarations.
  - `src/`: C++ LSP feature implementations. `src/api.cpp` aggregates them into the `LSPAPI` function table.
  - `include/refactors/`, `src/refactors/`: refactor action registration and edit generation.
  - `include/register_code_fix/`, `src/register_code_fix/`: code fix implementations registered by diagnostic code.
  - `include/formatting/`, `src/formatting/`: formatting settings, rules, context, and smart indentation.
  - `include/services/`, `src/services/`: implementation lookup, import utilities, common AST predicates, and text change tracking.
  - `code_fix_register.rb`, `code_fix_register.h.erb`: generate `generated/code_fix_register.h` from diagnostic YAML files.
  - `CMakeLists.txt`, `BUILD.gn`: CMake / GN build entry points. New source files must be added to both when applicable.
- Downstream stages:
  - `bindings/native/src/lsp.cpp`: Node native bridge that consumes `LSPAPI`.
  - `bindings/src/common/Es2pandaNativeModule.ts`: TypeScript-side native declarations and module loading.
  - `bindings/src/lsp/lsp_helper.ts`: main TypeScript-side entry point. It manages file cache, incremental compilation, dependency invalidation, declaration file generation, symbol reference index calls, and LSP method wrappers.
  - `bindings/src/lsp/lspNode.ts`: TypeScript-side LSP data structures.

## Directory Explanation

- `AGENTS.md`: development rules for this directory. It describes cross-component LSP / bindings changes, steps for adding LSP methods, and build/test guidance.
- `CLAUDE.md`: directory-level helper context mainly for external assistant usage.
- `BUILD.gn`: OpenHarmony / GN build definition. `lsp_build_enable` controls `libes2panda_lsp`; `bindings_build_enable` controls bindings-related targets.
- `CMakeLists.txt`: CMake build definition. It generates the code fix registration header and builds `${LSP_LIB}`.
- `code_fix_register.rb`: reads diagnostic YAML files and generates code fix registration code through the ERB template.
- `code_fix_register.h.erb`: code fix registration header template. The output is `${GENERATED_DIR}/code_fix_register.h`.
- `include/api.h`: C ABI-style public API entry point. It defines the `LSPAPI` function table and LSP return structures.
- `src/api.cpp`: `LSPAPI` implementation aggregation layer. It handles `SetPhaseManager`, code point / byte offset conversion, calls into concrete `Impl` functions, deduplication / sorting, and return-value wrapping.
- `include/internal_api.h`, `src/internal_api.cpp`: internal helper collection, including `Initializer`, touching token lookup, definition lookup, diagnostic conversion, owner lookup, scope lookup, import paths, and node traversal.
- `include/types.h`, `src/types.cpp`: common LSP data structures such as `TextSpan`, `TextChange`, `FileTextChanges`, `InlayHint`, and `SignatureHelpItem`.
- `include/user_preferences.h`: user preference settings, mainly passed to code fix / refactor / formatting features.
- `include/cancellation_token.h`, `src/cancellation_token.cpp`: cancellation token wrapper used by long-running flows such as cross-file references, TODO scanning, and code fixes.

## Recommended Reading Order

1. Start with `include/api.h` and `src/api.cpp` to understand the exported `LSPAPI`, offset conversion, phase-manager setup, and return-value wrapping.
2. Read `include/internal_api.h` and `src/internal_api.cpp` for shared AST lookup, token positioning, owner/declaration resolution, diagnostics conversion, and scope helpers.
3. Move to the concrete feature file that owns the behavior, such as `src/completions.cpp`, `src/get_definition_and_bound_span.cpp`, `src/find_rename_locations.cpp`, `src/symbol_reference_index.cpp`, `src/code_fix_provider.cpp`, or `src/refactors/*`.
4. Check the bindings side last: `bindings/native/src/lsp.cpp`, `bindings/src/common/Es2pandaNativeModule.ts`, `bindings/src/lsp/lspNode.ts`, and `bindings/src/lsp/lsp_helper.ts`.
5. If the issue involves project state, incremental changes, or cross-file behavior, inspect `lsp_helper.ts` earlier because it owns cache invalidation and context reuse.

## Core Files And Responsibilities

- `api.h` / `api.cpp`:
  - Aggregates externally callable methods: definition, implementation, references, rename, diagnostics, completion, quick info, formatting, refactor, code fix, symbol index, program deletion, and related APIs.
  - Key convention: the TS side usually passes character offsets; internal C++ AST ranges often use byte offsets. Entry points must use `CodePointOffsetToByteOffset` / `ByteOffsetToCodePointOffset` where needed.
- `internal_api.h` / `internal_api.cpp`:
  - Handles AST location, node owner lookup, declaration resolution, diagnostic conversion, import paths, scope lookup, and text range lookup.
  - Many features first call `GetTouchingToken*` to locate the AST node at the current position, then enter feature-specific logic.
- `lsp_utils.h` / `lsp_utils.cpp`, `line_column_offset.h` / `line_column_offset.cpp`:
  - Handle UTF-8 byte offset, code point offset, and line/column conversion. These are the first files to inspect for position-related bugs.
- `get_node.h` / `get_node.cpp`, `node_matchers.h` / `node_matchers.cpp`:
  - Provide AST node lookup for `NodeInfo` / definition data passed from the TS side. They support node-path-based definition, reference, and rename workflows.
- `symbol_reference_index.h` / `symbol_reference_index.cpp`:
  - Maintains the global per-file symbol index. It provides initialization, clearing, single-file indexing, indexing with external programs, file index removal, indexed reference lookup, and indexed source retrieval.
  - Used to improve cross-file reference and file reference lookup. The TS side tries to build or clean this index from `lsp_helper.ts` when creating contexts, applying incremental updates, or deleting files.
- `get_definition_and_bound_span.h` / `get_definition_and_bound_span.cpp`, `references.h` / `references.cpp`, `find_references.h` / `find_references.cpp`:
  - Implement go to definition, declaration information resolution, and reference lookup.
  - `find_references.cpp` has both context-list-based and `SourceFile`-list-based cross-file lookup paths. References are matched by owner location id.
- `rename.h` / `rename.cpp`, `find_rename_locations.h` / `find_rename_locations.cpp`:
  - `getRenameInfo` determines whether the current position can be renamed. `findRenameLocations*` finds rename locations in the current file or across files. `needsCrossFileRename` determines whether cross-file rename is required.
- `quick_info.h` / `quick_info.cpp`:
  - Hover / quick info entry point. It finds the node at the current position, contextual type, and display text.
- `completions.h` / `completions.cpp`, `completions_details.h` / `completions_details.cpp`, `string_completions.h` / `string_completions.cpp`:
  - Completion items, completion details, and string literal completions.
  - `completions.cpp` also contains external API collection logic. `collectApiInfo` is exposed to the TS side through `api.cpp`.
- `script_element_kind.h` / `script_element_kind.cpp`:
  - Maps aliases / declarations to script element kinds used by completion or navigation features.
- `signature_help.h` / `signature_help.cpp`, `signature_help_items.h` / `signature_help_items.cpp`, `get_signature.h` / `get_signature.cpp`, `create_type_help_items.h` / `create_type_help_items.cpp`:
  - Generate function-call help, type-argument help, candidate signatures, parameter lists, and type help items.
- Diagnostics-related files:
  - `suggestion_diagnostics.h` / `suggestion_diagnostics.cpp`: suggestion diagnostics.
  - `isolated_declaration.h` / `isolated_declaration.cpp`: helper logic for isolated declaration type text.
  - `getSemanticDiagnostics` / `getSyntacticDiagnostics` / `getCompilerOptionsDiagnostics` in `api.cpp` convert compiler diagnostics into LSP structures.
- `code_fix_provider.h` / `code_fix_provider.cpp`, `code_fixes/code_fix_types.h`:
  - Code fix registry, diagnostic-code-to-fix mapping, fix-all dispatch, and types such as `CodeFixContext` / `CodeFixAction`.
- `register_code_fix/*`:
  - Concrete quick fix implementations. File names should use common, industry-standard, unambiguous action descriptions such as add local variable, add missing `new`, add parameter name, add `declare`, convert `const` to `let`, add `super` in derived constructors, remove illegal `await`, remove duplicate export/import, fix spelling, fix property access, import fixes, and UI plugin suggestions.
- `refactor_provider.h` / `refactor_provider.cpp`, `applicable_refactors.h` / `applicable_refactors.cpp`, `get_edits_for_refactor.h` / `get_edits_for_refactor.cpp`, `refactors/refactor_types.h`:
  - Refactor registration, available-action lookup at the current position, action execution, and `RefactorEditInfo` generation.
- `refactors/*`:
  - Concrete refactors: arrow function brace conversion, optional chain conversion, function / arrow function conversion, function-to-class conversion, import conversion, overload-list merging, parameters-to-object conversion, template string conversion, extract symbol/type, generate constructor, generate getters/setters, generate override methods, infer function return type, and move to new file.
- `formatting/*`:
  - `formatting_settings.*` defines editor / formatting settings. `formatting.*` performs document/range/keystroke formatting. `formatting_context.*` maintains token context. `rules.*` / `rules_map.*` manage formatting rules. `smart_indenter.*` handles indentation.
- `services/*`:
  - `services.h` / `services.cpp`: service entry points such as implementation location.
  - `import_utils.*`: import declaration matching, module path normalization, specifier formatting, and specifier merging.
  - `utilities.*`: common AST predicates, such as implementation, `this`, initializer, return, and as type.
  - `text_change/change_tracker.*`, `text_change/text_change_context.h`: core tools for generating text edits in refactors and code fixes.
- Other feature files:
  - `organize_imports.*`: organize imports.
  - `brace_matching.*`: brace matching.
  - `class_hierarchy.*`, `class_hierarchy_info.*`, `class_hierarchies.h`, `class_hierarchy_item.h`: class / interface hierarchy and member information.
  - `get_class_property_info.*`: class property collection and constructor-related edit types.
  - `generate_constructor.*`: class constructor generation information.
  - `get_safe_delete_info.*`, `find_safe_delete_location.*`: safe delete checks and location collection.
  - `get_adjusted_location.*`: adjusts the current position to the node that matches language-service semantics.
  - `get_name_or_dotted_name_span.*`: property access / qualified name ranges.
  - `inlay_hints.*`: type, parameter, enum member, and related inlay hints.
  - `todo_comments.*`: TODO comment scanning.
  - `classifier.*`: syntactic / semantic classification.
  - `navigate_to.*`: pattern matching for named declarations and navigate-to information.

## Responsibility Boundaries

- Responsible for:
  - Providing language service results from an already parsed / checked `es2panda_Context`.
  - Converting compiler-internal ASTs, types, diagnostics, and text ranges into structures consumable by editors / LSP clients.
  - Keeping the external `LSPAPI` function table stable and synchronized with the bindings native / TS layers.
  - Handling editor semantics in the LSP layer, such as current-position tokens, user selection ranges, fix/refactor text changes, and formatting rules.
- Not responsible for:
  - Redefining parser / checker / varbinder language semantics.
  - Silently fixing AST or type errors from earlier compiler stages in the LSP layer.
  - Bypassing `api.h` from the bindings layer to access C++ internals directly.
  - Calling `CreateContext` / `createContext` or constructing new AST nodes inside feature modules. The LSP layer should work around the globally unique context maintained by the TS / native call chain.
  - Manually editing `generated/code_fix_register.h`; it is generated by `code_fix_register.rb`.

## Core Data Flow Or Control Flow

- Normal single-file LSP request:
  1. The TS-side `Lsp` in `lsp_helper.ts` reads file content, generates config, and creates or reuses an `es2panda_Context`.
  2. The TS side passes file name, offset, selection range, and user settings to the native bridge.
  3. The C++ entry point in `api.cpp` sets the phase manager and converts code point offsets to byte offsets when needed.
  4. Concrete features use `internal_api` to locate AST nodes and read varbinder / checker / parser information.
  5. `api.cpp` converts return values back to character offsets, deduplicates, and wraps results into `LSPAPI` structures.
  6. TS-side `lspNode.ts` types receive the result and pass it to the editor / caller.
- Cross-file references / rename:
  1. The TS side merges compile files according to build config, module info, and file dependencies.
  2. C++ may use context-list scanning or the indexed path through `symbol_reference_index`.
  3. When files are deleted or switched, the TS side calls program deletion and symbol index removal to avoid stale programs or references.
- Code fix:
  1. Diagnostic YAML files plus `code_fix_register.rb` generate the registration header.
  2. Each `src/register_code_fix/*.cpp` implementation is registered into `CodeFixProvider`.
  3. `getCodeFixesAtPosition` finds fixes by diagnostic code, span, and context, then uses `ChangeTracker` to generate `FileTextChanges`.
  4. `getCombinedCodeFix` runs fix-all by `fixId`.
- Refactor:
  1. `getApplicableRefactors` builds a `RefactorContext` and queries `RefactorProvider` for available actions at the current position.
  2. `getEditsForRefactor` calls a concrete `Refactor` by `refactorName` and `actionName`.
  3. The concrete refactor uses `TextChangesContext` / `ChangeTracker` to return edits and may also return a rename location or new file name.
- Formatting:
  1. The TS side builds `FormatCodeSettings`.
  2. The C++ side creates a formatting context through `GetFormatContext`.
  3. Document/range/keystroke entry points call `FormatDocument`, `FormatRange`, or the keystroke variant of `FormatRange`.

## Knowledge Routing

- Editor-visible LSP behavior issues or native LSP API result issues -> this document.
- TS-side call failures, cache / incremental compilation / module config / native bridge type mismatches -> `../bindings/Bindings_Knowledge_Base.md`; also inspect `bindings/src/lsp/lsp_helper.ts`.
- `es2panda_Context` creation, state progression, or public C API changes -> `../public/Public_Knowledge_Base.md`.
- AST node structure, source ranges, or parser program issues -> `../parser/AGENTS.md`, `../parser/`, and `../parser/program/`.
- Type and checker result issues -> `../checker/AGENTS.md`, `../checker/`, `../checker/ets/`, `../checker/ts/`, and `../checker/types/`.
- Symbol binding, declaration ownership, or scope lookup issues -> `../varbinder/AGENTS.md` and `../varbinder/`.
- Diagnostic codes, error messages, or YAML definitions -> `../util/AGENTS.md`, `../util/diagnostic/`, and `../declgen_ets2ts/AGENTS.md`.
- Build target issues or missing GN / CMake source entries -> `../../../architecture/Build_Test_Knowledge_Base.md` or this directory's `BUILD.gn` / `CMakeLists.txt`.

## Expert Notes

- First locate whether the issue is in the TS wrapper, native bridge, `api.cpp` entry conversion, or the concrete `src/*.cpp` implementation. Do not jump directly into deep semantic changes.
- For position issues, first check the offset unit: TS / LSP commonly use character offsets or line/column, while AST ranges commonly use byte offsets.
- `GetTouchingTokenRightMatch` and `GetTouchingToken` are common entry points used by many LSP features to find AST nodes by offset. Changes to them can affect go to definition, completion, references, rename, quick info, code fixes, and other modules; evaluate all call sites and boundary behavior before modifying them.
- Prefer `GetTouchingTokenRightMatch` when locating a node by offset. Use `GetTouchingToken` only when its current matching semantics are explicitly required.
- When modifying or adding an LSP method, keep at least these files in sync: `include/api.h`, `src/api.cpp`, `CMakeLists.txt`, `BUILD.gn`, `bindings/native/src/lsp.cpp`, `bindings/src/common/Es2pandaNativeModule.ts`, `bindings/src/lsp/lspNode.ts`, `bindings/src/lsp/lsp_helper.ts`, and nearby tests.
- When changing public interfaces, common positioning logic, or the TS/native call chain, run both `ets2panda/test/unit/lsp` and `ets2panda/bindings/test`. Running only one side can miss C++/TS type or behavior mismatches.
- After adding a new `.cpp` file, update both CMake and GN. These build lists are easy to update on only one side.
- When adding a code fix, do not only write `src/register_code_fix/*.cpp`; also check diagnostic YAML, generated registration, headers, build lists, and fix-all behavior.
- Prefer `ChangeTracker` for refactor / code fix edits to avoid fragile hand-written cross-file text changes.
- If cross-file features return stale, duplicated, or missing results, inspect TS-side `filesMap`, `compiledFileHashes`, dependency invalidation, and whether the C++ `symbol_reference_index` is built / removed / cleared at the right time.
- For reference lookup, file references, cross-file symbol lookup, and similar features, prefer the symbol table / index path in `src/symbol_reference_index.cpp`. Fall back to full-file AST scanning only when the index cannot express the required semantics.
- Do not create new contexts or manually construct AST nodes inside feature modules to bypass the existing call chain. This breaks the globally unique LSP context, incremental cache, external programs, and symbol index consistency.
- `SetPhaseManager(ctx->phaseManager)` is required in many entry points. Missing it can make helpers use the wrong phase state.
- For import-related features, prefer `services/import_utils.*`, `organize_imports.*`, and `ComputeRelativeImportPath` instead of scattering path rules.

## Anti-Patterns

- Duplicating checker type inference or parser syntax logic in the LSP layer, causing compiler results and editor results to diverge.
- Forgetting byte offset / code point offset conversion at entry points, which causes incorrect navigation around Chinese text, emoji, or other multibyte characters.
- Changing only `src/api.cpp` without updating the `LSPAPI` function table in `include/api.h` or bindings declarations.
- Manually editing generated file `generated/code_fix_register.h`.
- Temporarily creating a context, reparsing a file, or constructing AST nodes inside a feature module to obtain a result.
- Fabricating C++ return values on the TS side while bypassing the native bridge, hiding a missing C++ capability.
- Concatenating whole-file text directly in code fixes / refactors instead of using `TextChange` / `FileTextChanges`.
- Adding cross-file features without handling cancellation tokens, cache invalidation, and file deletion.
- Swallowing upstream diagnostics in the LSP layer or packaging compiler errors as "language service compatibility" behavior.

## Debugging And Verification

- Minimal build:
  - CMake: build `${LSP_LIB}` or the frontend target that includes LSP.
  - GN: confirm that `:libes2panda_lsp` / `:libes2panda_lsp_static` includes the new source files when `lsp_build_enable = true`.
- C++ unit tests:
  - Location: `ets2panda/test/unit/lsp/`
  - Common coverage: completion, definition, references, formatting, quick info, rename, refactor, code fix.
- Bindings tests:
  - Location: `ets2panda/bindings/test/`
  - Used to verify TS -> native -> C++ return structures and real project config / cache flows.
- Higher-level verification:
  - Changes to `api.h` / `api.cpp`: run at least one native bindings e2e test and the related C++ LSP unit tests.
  - Changes to public interfaces, `GetTouchingTokenRightMatch` / `GetTouchingToken`, context lifecycle, or TS/native bridge: run both `ets2panda/test/unit/lsp` and `ets2panda/bindings/test`.
  - Changes to cross-file references / symbol index: add multi-file, deleted-file, incremental modification, and external-program scenarios.
  - Changes to code fixes: cover single fix and fix-all.
  - Changes to formatting: cover document, range, and after-keystroke formatting.

## Debugging Methods

- Start from the TS entry point and print / debug whether the corresponding method in `lsp_helper.ts` creates the right context and passes the expected file name and offset.
- In the native bridge, verify parameter types: strings, arrays, pointers, and structure field order must match `api.h`.
- At the `src/api.cpp` entry point, check:
  - whether `context` is null;
  - whether `source` comes from the expected program;
  - offset values before and after conversion;
  - whether `SetPhaseManager` is called;
  - whether results need conversion from byte offsets back to character offsets before return.
- In concrete features, check touching token / owner / declaration:
  - whether `GetTouchingToken*` selects the expected node;
  - whether `GetOwner` / `compiler::DeclarationFromIdentifier` can find the declaration;
  - whether external declarations are included in the search scope.
- For cross-file issues, check:
  - whether TS-side compile files include the target file;
  - whether `symbol_reference_index` contains the target file source;
  - whether stale programs and stale index entries are cleaned after file deletion / switching.
- For code fixes / refactors, inspect the generated `FileTextChanges` before checking the text after editor application. Most issues are caused by incorrect spans, newlines, indentation, or file names.

## FAQ

1. Symptom: go to definition / references work in ASCII files but are shifted near Chinese paths or Chinese source text  
   Quick check: verify that `api.cpp` performs two-way code point / byte offset conversion, and check whether the TS side passes offset or line/column.
2. Symptom: a newly added LSP method cannot be called from TS  
   Quick check: verify the full chain: `LSPAPI`, `g_lspImpl` initialization, native bridge, `Es2pandaNativeModule.ts`, `lspNode.ts`, and `lsp_helper.ts`.
3. Symptom: cross-file references are missing or include deleted files  
   Quick check: inspect `tryBuildSymbolReferenceIndex` / `tryRemoveSymbolReferenceIndex`, `DeleteProgramForFile`, `DeleteDependantProgramsForFiles`, and compile-file merge logic.
4. Symptom: a code fix appears on the wrong diagnostic or fix-all does not work  
   Quick check: inspect the diagnostic YAML code, generated registration header, `CodeFixRegistration::SetErrorCodes` / `SetFixIds`, and `GetAllCodeActions`.
5. Symptom: completions miss exported APIs  
   Quick check: inspect `collectApiInfo`, whether external programs are built, and export / import collection plus path normalization logic in `completions.cpp`.
6. Symptom: formatting works only for the whole document, while range / keystroke formatting is wrong  
   Quick check: inspect `FormatContext`, the input `TextSpan`, and whether `rules_map` matches the expected tokens.

## Related Documents

- `../bindings/Bindings_Knowledge_Base.md`
- `../public/Public_Knowledge_Base.md`
- `../ETS2Panda_Overview_Knowledge_Base.md`
- `SYMBOL_REFERENCE_INDEX_DESIGN.md`
- `AGENTS.md`
- `../bindings/README.md`
- `../bindings/test/README.md`
- `../../../architecture/Build_Test_Knowledge_Base.md`
