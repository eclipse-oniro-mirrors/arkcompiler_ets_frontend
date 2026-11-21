# Lexer Module - AGENTS.md

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Module responsibilities

The lexer converts JavaScript, TypeScript, and supported ArkTS-dynamic source syntax into tokens. It handles identifiers, keywords, literals, operators, comments, whitespace, template strings, regular expressions, and source positions.

Read the repository `AGENTS.md`, `es2panda/AGENTS.md`, and `docs/knowledge/ES2Panda_Knowledge_Base.md` before changes. Paths below are repository-relative.

## 2. Directory structure

```text
es2panda/lexer/
├── lexer.cpp                   # Main scanner
├── lexer.h                     # Lexer state and operations
├── keywordsUtil.cpp            # Keyword helpers
├── keywordsUtil.h              # Keyword helper declarations
├── keywordString.h             # Keyword strings
├── templates/keywords.h.erb     # Keyword definitions template
├── templates/keywordsMap.h.erb  # Keyword lookup template
├── scripts/keywords.rb          # Keyword generator and input definitions
├── token/token.h               # Token payload and flags
├── token/token.cpp             # Token helpers
├── token/tokenType.h            # TokenType definitions
├── token/sourceLocation.h       # Source ranges and line indexing
└── regexp/regexp.cpp            # Regular-expression parsing
```

## 3. Core classes and APIs

### 3.1 Lexer

Selected declarations from the current `lexer.h`:

```cpp
    explicit Lexer(const parser::ParserContext *parserContext);
    NO_COPY_SEMANTIC(Lexer);
    NO_MOVE_SEMANTIC(Lexer);
    ~Lexer() = default;

    void NextToken(LexerNextTokenFlags flags = LexerNextTokenFlags::NONE);

    Token &GetToken();
    const Token &GetToken() const;
    size_t Line() const;

    LexerPosition Save() const;
    void Rewind(const LexerPosition &pos);
    void BackwardToken(TokenType type, size_t offset);
    void ForwardToken(TokenType type, size_t offset);

    char32_t Lookahead();
    bool CheckArrow();

    RegExp ScanRegExp();
    template <char32_t end>
    void ScanString();
    void ResetTokenEnd();
    LexerTemplateString ScanTemplateString();
    void ScanTemplateStringEnd();
    void PushTemplateContext(TemplateLiteralParserContext *ctx);
    void AssignTokenTaggedTemplate();
```

`NextToken()` updates the lexer state and returns `void`; read the token with `GetToken()`. `Save()` and `Rewind()` handle speculative parsing. Do not use the old guide's nonexistent `PeekToken()`, `SavePosition()`, or token-returning `NextToken()` signatures.

### 3.2 Token

`token/token.h` defines token payload and source range. `TokenType` is declared separately in `token/tokenType.h`; it is not a nested `Token::TokenType` enum.

Selected accessors:

```cpp
    TokenType Type() const
    {
        return type_;
    }

    TokenFlags Flags() const
    {
        return flags_;
    }

    void SetTokenType(TokenType type)
    {
        type_ = type;
    }

    TokenType KeywordType() const
    {
        return keywordType_;
    }

    const SourcePosition &Start() const
    {
        return loc_.start;
    }

    const SourcePosition &End() const
    {
        return loc_.end;
    }

    const SourceRange &Loc() const
    {
        return loc_;
    }

    const util::StringView &Ident() const
    {
        return src_;
    }
```

Read payload accessors only for the corresponding token kind and flags. Numeric tokens distinguish BigInt from ordinary numeric values; escaped identifiers and tagged templates need their original contextual information.

## 4. Keyword generation

### 4.1 Templates and input

```text
lexer/scripts/keywords.rb
    + lexer/templates/keywords.h.erb
    + lexer/templates/keywordsMap.h.erb
    → generated keyword definitions and lookup map
```

The build invokes `es2panda/scripts/gen_keywords.sh`. Inspect the actions and dependencies in `es2panda/BUILD.gn`; generated output paths depend on the selected product/toolchain, so do not hardcode a single `out/sdk/` path.

### 4.2 Modification workflow

1. Determine whether the keyword list, lookup behavior, or emitted definitions need to change.
2. Update `keywords.rb` and/or the corresponding ERB template.
3. Rebuild `ets_frontend_build` through GN.
4. Check both generated outputs and the scanner's use of them.
5. Run lexer and parser regressions, including contextual keywords used as identifiers.

Never edit generated `keywords.h` or `keywordsMap.h` directly. Adding a keyword does not automatically make it reserved in every JS/TS context.

## 5. Token type system

### 5.1 TokenType

`token/tokenType.h` contains the current enum and generated keyword integration. Use actual names such as `EOS` and the existing punctuator/literal names; do not copy the old guide's invented `EOF_TOKEN`, `NEW_LINE`, or literal categories.

When adding a token, check token predicates and operator tables as well as scanner and parser dispatch. Changes to enum ordering may affect range-based token classification.

### 5.2 TokenFlags and next-token flags

`TokenFlags` in `token/token.h` describes properties of the current token. `LexerNextTokenFlags` in `lexer.h` controls scanning, including keyword-to-identifier conversion, numeric separators, and BigInt handling. These two enums have different owners and uses.

Preserve newline and escape information because automatic semicolon insertion, restricted productions, and template behavior depend on it. Verify a flag's consumers before changing its meaning.

## 6. Regular-expression scanning

The parser and lexer cooperate to interpret `/` in context. `Lexer::ScanRegExp()` and its pattern/flag helpers scan a regular expression when the parser selects that route; do not decide every slash from the previous token alone.

`lexer/regexp/regexp.cpp` implements the regular-expression parser. There is no documented `RegExpLexer::ParseRegExp` wrapper to call.

Test these cases together:

- Division and division assignment.
- Regex literals at expression starts and after statement boundaries.
- Escaped slashes and character classes.
- Valid and invalid flags, including duplicates.
- Unterminated patterns and source positions in diagnostics.

## 7. Source location tracking

`token/sourceLocation.h` defines separate representations:

| Type | Fields / purpose |
|---|---|
| `SourcePosition` | `index`, `line`: position in source |
| `SourceRange` | `start`, `end`: source extent |
| `SourceLocation` | `line`, `col`: mapped diagnostic position |
| `LineIndex` | Line/offset mapping support |

`Token::Loc()` returns a `SourceRange`, not a filename-bearing object with `ToString()`. Preserve offsets through Unicode text, escapes, CRLF, comments, and multiline templates. Confirm indexing conventions at the diagnostic/output consumer before changing them.

## 8. Modification guidance

### 8.1 Add a token

1. Update the actual token definitions or their source generator.
2. Add scanner handling in `lexer.cpp`.
3. Update parser consumers and token classification helpers.
4. Cover valid, invalid, boundary, and contextual uses.

### 8.2 Add a keyword

1. Update the keyword source of truth and generation templates as needed.
2. Rebuild the generated headers and frontend.
3. Test reserved-word and identifier contexts separately.
4. Check JS/TS and module/script differences.

### 8.3 Improve performance

Keep source-backed string lifetimes valid and use the existing arena/Unicode helpers. Preserve saved-position state and avoid introducing inconsistent token payloads during lookahead. Measure representative input rather than assuming fewer branches improves scanning.

## 9. Debugging tokens

Set breakpoints in `Lexer::NextToken()` and inspect `GetToken().Type()`, `Ident()` when valid, and `Start()`/`End()`. Use `Save()`/`Rewind()` when tracing speculative parsing.

Use `es2abc --dump-ast input.js` to verify the downstream parse result. The old loop based on `PeekToken()` and a token-returning `NextToken()` was not a valid example for this API.

## 10. Related modules

- Upstream: source text and parser context.
- Downstream: `es2panda/parser/` and `es2panda/parser/context/`.
- Supporting code: `es2panda/util/ustring.h`, program-owned allocation, token locations, and IR source ranges.

## 11. Verification and evidence

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
