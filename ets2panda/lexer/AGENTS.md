# Lexer Component

## Core Metadata

| Attribute | Value |
|-----------|--------|
| **Name** | Lexer |
| **Purpose** | Converts source text into a stream of tokens and provides a unified lexical interface for multiple language modes. Day-to-day changes apply only to **ETS** (ETSLexer and ETS token tables); TS/AS/JS paths are **out of scope**. |
| **Primary Language** | C++ |

## Change Frequency and Scope

- **Lexer is the foundation for Parser**: it turns source into a token stream that Parser consumes. This directory is at the front of the pipeline; once its interface is stable, all later stages depend on it.
- **This directory is rarely modified**. Most work happens in parser, varbinder, checker, and compiler/lowering. Lexer is touched only when adding or changing keywords, token kinds, or lexical rules; such changes require updating `scripts/` and regenerating code, plus careful regression testing.
- **In-scope for changes**: only **ETSLexer** and ETS-related token/keyword tables. TSLexer, ASLexer, and JS lexer paths are **out of scope**.

## Directory Layout

```
lexer/
├── *.cpp, *.h           # Lexer core and per-language implementations; [in scope] ETSLexer; TSLexer/ASLexer rarely changed
├── token/               # Token, source location, numeric literal parsing
├── regexp/              # Regex literal lexing
├── scripts/             # Token and keyword tables (keywords.yaml, tokens.yaml, Ruby)
└── templates/           # Codegen templates (tokenType, keywords, token.inl, etc. .erb)
```

## Responsibilities

- **Token kinds and keywords**: Generated from YAML + Ruby under `scripts/` into `keywords*.cpp/h`, `tokenType`, etc.; scripts are the single source of truth.
- **Multi-language lexing**: **ETSLexer** (ArkTS/ETS) is the active target for changes; TSLexer, ASLexer, and shared token/regexp/number logic are rarely modified.
- **Source locations**: `token/sourceLocation` records line/column and offsets for parser and diagnostics.

## Dependencies

- **Used by**: parser, util (diagnostics, options, paths).
- **Depends on**: no other ets2panda front-end modules (only C++ stdlib and project infrastructure).

## Extending or Modifying

- **New or changed token kind or keyword**: Update `scripts/tokens.yaml` or `scripts/keywords.yaml`, run the corresponding Ruby scripts to regenerate `.h`/`.cpp`, and run regression (Parser and later stages depend on the token stream).
- **ETS lexing or keywords**: Change ETS-related tables and **ETSLexer**; extend ETS entries in `scripts/` if needed. Other language modes (TS/AS/JS) are out of scope; avoid changing this layer unless necessary.
