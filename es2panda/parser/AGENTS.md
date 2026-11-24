# Parser Module - AGENTS.md

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Module responsibilities

The parser builds AST nodes from the token stream, handles expression precedence and statement/declaration syntax, creates program/module structures, and coordinates declaration binding. TS syntax and TS-to-JS transformation are supported; the removed TypeScript checker is not part of this route.

Read the repository `AGENTS.md`, `es2panda/AGENTS.md`, and `docs/knowledge/ES2Panda_Knowledge_Base.md` before changes. Paths below are repository-relative.

## 2. Directory structure

```text
es2panda/parser/
├── parserImpl.cpp              # Core parser implementation
├── parserImpl.h                # Parser declarations and TS parsing helpers
├── expressionParser.cpp        # Expression parsing
├── statementParser.cpp         # Statements and declarations
├── commonjs.cpp                # CommonJS handling
├── parserFlags.h               # Parser expression/statement flags
├── context/parserContext.h     # Nested parse context and status
├── context/parserContext.cpp   # Context behavior
├── program/program.h           # Compilation unit
├── program/program.cpp         # Program behavior
├── module/sourceTextModuleRecord.h   # Import/export structures
├── module/sourceTextModuleRecord.cpp # Module record behavior
└── transformer/transformer.cpp # TS AST transformation
```

## 3. Core classes and APIs

### 3.1 ParserImpl

Current public entry declarations from `parserImpl.h`:

```cpp
    explicit ParserImpl(es2panda::ScriptExtension extension);
    NO_COPY_SEMANTIC(ParserImpl);
    NO_MOVE_SEMANTIC(ParserImpl);
    ~ParserImpl() = default;

    Program Parse(const SourceFile &sourceFile, const CompilerOptions &options);

    ScriptExtension Extension() const;

    void AddPatchFixHelper(util::PatchFix *patchFixHelper);

    ArenaAllocator *Allocator() const
    {
        return program_.Allocator();
    }
    bool IsDtsFile() const;
```

`Parse()` returns a `Program` value. The program owns the arena, AST, binder, source metadata, and module records. New nodes must use the existing program allocator and preserve source ranges.

Expression and statement parsers are implementation files for `ParserImpl`, not separate public `ExpressionParser` and `StatementParser` interfaces. Most parsing routines are private helpers.

### 3.2 ParserContext

`context/parserContext.h` defines `ParserStatus` and a nested context chain. It exposes status queries such as `IsGenerator()`, `IsAsync()`, `IsModule()`, `IsTsModule()`, and `FindLabel()`.

Use existing context helpers to enter/leave functions, loops, classes, and labels. Binding scope and parse context are related but distinct; do not assume the context owns a generic `PushScope()` API.

## 4. Expression parsing

### 4.1 Precedence and associativity

Inspect `expressionParser.cpp`, token classification, and the relevant `ParserImpl` helpers together. The following groups are useful review points, rather than a replacement grammar:

| Group | Operators / concerns |
|---|---|
| Sequence and assignment | Comma, assignment targets, compound assignments |
| Conditional | `?:` and branch parsing |
| Short-circuit | `||`, `&&`, `??`; nullish/logical mixing restrictions |
| Bitwise | `|`, `^`, `&` |
| Equality | `==`, `!=`, `===`, `!==` |
| Relational | `<`, `>`, `<=`, `>=`, `in`, `instanceof` |
| Shift | `<<`, `>>`, `>>>` |
| Additive / multiplicative | `+`, `-`, `*`, `/`, `%` |
| Exponentiation | `**`, right associativity, unary-left restrictions |
| Unary / update | Prefix unary operators and prefix/postfix updates |
| Member / call | Property access, calls, optional chains, `new` |
| Primary | Identifiers, literals, grouping, arrays, objects, functions |

Assignment, exponentiation, and binary operators do not all share the same associativity. Cover parentheses and mixed operators when changing precedence.

### 4.2 Expression parsing methods

`ParserImpl::ParseExpression` accepts `ExpressionParseFlags`. Other helpers handle arrays, yield, TS assertions, `as`, `satisfies`, arrow parameters, and expression sequences. Check declarations in `parserImpl.h`; the old guide's list of one method per precedence level did not match this implementation.

### 4.3 Binary-expression example

```javascript
const result = a + b * c;
const power = a ** b ** c;
const chosen = test ? left : right;
```

Compare the dumped AST with expected nesting and associativity. A generic loop that repeatedly consumes the same operator would incorrectly model right-associative expressions and miss syntax restrictions.

## 5. Statement parsing

`statementParser.cpp` handles control flow, blocks, declarations, loops, returns, throws, try/catch/finally, and module syntax. Inspect both dispatch and context validation.

### 5.1 If-statement review

```javascript
if (first)
    if (second) left();
    else right();
```

The `else` binds to the nearest unmatched `if`. Preserve source ranges, statement parsing flags, scope setup, and error positions. Also test a block body and an absent alternate branch.

### 5.2 Other statements

For loops, distinguish `for`, `for-in`, `for-of`, lexical declarations, and iteration binding rules. For `return`, `throw`, `break`, and `continue`, test newline restrictions and legal surrounding context. For declarations, check binder insertion as well as AST shape.

## 6. TypeScript-specific parsing

### 6.1 Type annotations

Current helpers use names such as `ParseTsTypeAnnotation`, `ParseTsArrayType`, `ParseTsUnionType`, `ParseTsIntersectionType`, `ParseTsTypeReferenceOrQuery`, and `ParseTsFunctionType`. Some return `ir::Expression *`, so do not impose a nonexistent universal `TypeNode` API.

Inspect type references, generic parameters/arguments, tuples, mapped types, conditional types, predicates, assertions, and function types near the changed syntax. Parsing these types does not mean full semantic type checking is performed.

### 6.2 Decorators and annotations

`ParseDecoratorAndAnnotation` and `ParseDecoratorsAndAnnotations` are declared in `parserImpl.h`. Verify the relevant feature/API gates and transformation consumers. Test placement, invalid targets, multiple decorators, and runtime transformation ordering where supported.

## 7. AST transformation

`parser/transformer/` contains TS transformation implementation, but orchestration belongs to `Compiler::ProcessAstForTS` in `es2panda.cpp`.

The current orchestration body is:

```cpp
void Compiler::ProcessAstForTS(parser::Program *ast, const es2panda::CompilerOptions &options)
{
    if (ast->Extension() != ScriptExtension::TS) {
        return;
    }

    transformer_->Transform(ast);
    ast->Binder()->IdentifierAnalysis(binder::ResolveBindingFlags::TS_AFTER_TRANSFORM);
    if (options.dumpTransformedAst) {
        std::cout << ast->Dump() << std::endl;
    }
    if (options.checkTransformedAstStructure) {
        transformer_->CheckTransformedAstStructure(ast);
    }
}
```

### 7.1 Transformation coverage

- Type-only declarations and annotations must not become unintended runtime operations.
- Enums and namespaces may create runtime objects and generated bindings.
- Parameter properties interact with constructor initialization.
- Decorators may generate runtime calls and affect evaluation order.

Trace each behavior in the concrete transformer implementation before changing it. Test both the original and transformed AST and the generated program; there is no checker stage after transformation to repair invalid bindings.

## 8. Errors and recovery

`ParserImpl` declares `[[noreturn]] ThrowSyntaxError` overloads. Do not assume a general recovery AST or an error-collection mode exists merely because other compiler frontends have one.

For speculative parsing, inspect saved lexer positions and context state. Failed alternatives must not leave stale tokens, scope changes, or source ranges. For invalid syntax, compare the diagnostic and location and ensure the failure is propagated to the caller.

## 9. Parser flags

`parserFlags.h` and `context/parserContext.h` define the current flag families. `ExpressionParseFlags`, statement-related flags, `TypeAnnotationParsingOptions`, and `ParserStatus` are separate mechanisms.

Do not copy the former guide's illustrative `ParseFlags` enum or bit values. A new flag requires an audit of where it is set, inherited, cleared, and consumed, including nested functions and TS parsing.

## 10. Module systems

`ScriptKind::MODULE` and `COMMONJS` are separate paths. Inspect `commonjs.cpp`, program kind, and `module/sourceTextModuleRecord.*` together.

ES module records distinguish requests, imports, and exports, including lazy/type-only information. CommonJS wrapper handling must preserve its own binding semantics. Do not translate `require()` into ES imports as a routine parser fix.

## 11. Modification guidance

### 11.1 Add a statement

1. Define/update the IR node and mapping if a new node kind is required.
2. Add parser dispatch and syntax/context checks.
3. Construct declarations and scopes using the existing binder interfaces.
4. Implement traversal, dumping, transformation, and compilation as applicable.
5. Test valid syntax, invalid syntax, source positions, and runtime behavior.

### 11.2 Add an expression

1. Choose the actual precedence/associativity and allowed contexts from the spec.
2. Update the parser and IR representation.
3. Check assignments, patterns, optional chains, and TS ambiguities.
4. Preserve evaluation order through transformation and compilation.
5. Run adjacent expression and statement regressions.

## 12. Debugging

```bash
es2abc --dump-ast input.js
es2abc --extension ts --dump-transformed-ast input.ts
```

Break in `ParserImpl::Parse` and the actual syntax helper, then follow lexer token/range and binder scope state. Do not infer a parser defect solely from final runtime behavior.

## 13. Related modules

- Upstream: `es2panda/lexer/` supplies tokens.
- Coordinated analysis: `es2panda/binder/` manages declarations and references.
- Downstream: `es2panda/parser/transformer/` and `es2panda/compiler/`.
- Shared structures: `es2panda/ir/`, `binder/scope.h`, and the program allocator.

## 14. Verification and evidence

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
