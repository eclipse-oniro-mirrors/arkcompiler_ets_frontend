# Util Component

## Core Metadata

| Attribute | Value |
|-----------|--------|
| **Name** | Util |
| **Purpose** | Shared infrastructure for the ets2panda front end: diagnostics and error recovery, configuration and options, paths and strings, AST builders, name mangling, plugins, and build helpers. |
| **Primary Language** | C++ (some scripts and config: YAML/Ruby/Python) |

## Directory Layout

```
util/
├── diagnostic*.cpp, *.h, diagnosticEngine.*   # Diagnostics and engine
├── diagnostic/         # Diagnostic definitions and codegen (syntax/semantic/warning/fatal YAML + .erb)
├── options.*, options.yaml   # Compile option parsing and definition
├── arktsconfig.*       # ArkTS config
├── path.*, importPathManager.*   # Path and import path management
├── ustring.*, helpers.*, nameMangler.*   # Strings, helpers, name mangling
├── ast-builders/       # AST node builders (*Builder.h; used by parser/tests)
├── errorRecovery.*, recursiveGuard.h, bitset.*   # Error recovery and recursion guard
├── generateBin.*, es2pandaMacros.*, language.h   # Generation, macros, language enum
├── plugin.*, perfMetrics.*, dtoa_helper.*, eheap.*   # Plugin, perf, numeric, heap
└── Other utilities (doubleLinkedList.h, enumbitops.h, etc.)
```

## Responsibilities

- **Diagnostics**: Unified error codes and messages; YAML under diagnostic/ is the single source of truth; templates generate C++ constants and messages.
- **Options and config**: options.yaml and arktsconfig drive command-line and project config; read by driver and all stages.
- **AST builders**: ast-builders/ provides typed, fluent AST construction to reduce manual node construction errors.
- **Shared utilities**: Paths, strings, name mangling, error recovery, and plugin interface are used by lexer, parser, checker, compiler, etc.

## Dependencies

- **Used by**: Nearly all ets2panda modules (lexer, parser, varbinder, checker, compiler, driver).
- **Depends on**: No other ets2panda front-end modules (only C++ stdlib and build/script environment).

## Extending or Modifying

- **New diagnostic**: Add an entry in the appropriate YAML under diagnostic/, run the generator to update diagnostic.h/.cpp.
- **New option**: Define in options.yaml, parse and expose in options.cpp.
- **New AST builder**: Add *Builder.h under ast-builders/ following existing naming and style.
