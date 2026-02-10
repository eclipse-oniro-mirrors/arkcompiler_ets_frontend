# Parser Component

## Core Metadata

| Attribute | Value |
|-----------|--------|
| **Name** | Parser |
| **Purpose** | Parses a token stream into an AST and maintains parse context and the program root. Day-to-day changes apply only to **ETS** (ETSParser, ETSparser*.cpp); TS/JS/AS are **out of scope**. |
| **Primary Language** | C++ |

## In-Scope (Primary Modification Target)

- **New syntax and features start here**: When adding ETS syntax or language features, **first** add parsing in **Parser** (especially **ETSParser** / **ETSParser*.cpp**) to produce the corresponding AST; varbinder, checker, and lowering then consume it.
- **Only ETSParser and ETS parse logic** are modified in practice; TSParser, JSParser, ASparser, and shared expression/statement parsers are **out of scope**.

## Class Hierarchy

```
ParserImpl                    # Base; parserImpl.* is shared by all parsers
├── JSParser                  # JS
└── TypedParser               # Type-annotated parsing (shared)
    ├── ETSParser (final)     # ETS [primary modification target]
    └── ThrowingTypedParser   # Throws on parse failure
        ├── TSParser          # TS
        └── ASParser         # AS
```

- **Changes in parserImpl affect all languages**: `parserImpl.cpp` / `parserImpl.h` implement shared logic (e.g. ParseClassDefinition, ParseClassElement, modifier parsing). All parsers inherit from ParserImpl or a derived base. When changing parserImpl, consider regression for **ETS/TS/JS/AS**; for ETS-only work, prefer changing only **ETSParser*.cpp**.

## Syntax Only; Error Kind

- **This component does syntax parsing only**: token stream → AST. It does not perform type checking or symbol resolution (no semantic analysis).
- **All errors reported here are syntax errors** (invalid token, grammar violation, bracket mismatch, etc.). **No semantic errors**; those (type mismatch, undefined variable, duplicate definition, etc.) are reported in the **checker** stage.

## Directory Layout

```
parser/
├── *Parser*.cpp, *.h    # Per-language parser entry; [in scope] ETSParser; TSParser/JSParser/ASparser rarely changed
├── ETSparser*.cpp       # [in scope] ETS: expressions, statements, types, classes, enums, namespaces, annotations
├── context/             # Parse context (parserContext, classPrivateContext)
├── program/             # Program root, declaration cache, entity name access
├── expressionParser.cpp / statementParser.cpp   # Shared expression/statement parsing
├── TypedParser*.cpp     # Typed parsing and error handling
├── JsdocHelper.*        # JSDoc parsing
└── parserImpl.* / forwardDeclForParserImpl.h   # Shared implementation; changes affect all languages—modify with care
```

## Responsibilities

- **Grammar**: ETSParser parses ETS declarations, expressions, statements, and type annotations into AST nodes matching `ir`; other language parsers are rarely changed.
- **Context and scope**: `context/` holds parse state and private-member context; `program/` holds the root Program and declaration cache.
- **Errors and recovery**: ThrowingTypedParser and related types unify error handling and recovery.

## Dependencies

- **Used by**: varbinder, checker, compiler, driver.
- **Depends on**: lexer (token stream), ir (AST node definitions), util.

## Extending or Modifying

- **New ETS syntax**: Add parse branches in **ETSParser*.cpp**; add or reuse AST node types in `ir` if needed. Other language parse paths are out of scope.
- **Parse context**: When extending `context/` or `program/`, keep behavior consistent with varbinder/checker; if changes are ETS-only, limit them to ETS parse entry points and context.
