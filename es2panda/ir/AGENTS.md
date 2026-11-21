# IR Module - AGENTS.md

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Module responsibilities

IR defines AST node kinds, node classes, traversal/update operations, AST dumping, and node-specific compilation. Parser, binder, TS transformation, and bytecode compilation all depend on these structures.

Read the repository `AGENTS.md`, `es2panda/AGENTS.md`, and `docs/knowledge/ES2Panda_Knowledge_Base.md`. Paths below are repository-relative.

## 2. Directory structure

```text
es2panda/ir/
├── astNode.h                 # Base node, enums, generated kind checks
├── astNodeMapping.h          # Node-kind/class mappings
├── expression.h              # Expression base
├── statement.h               # Statement base
├── astDump.cpp               # AST serialization support
├── astDump.h                 # AstDumper interface
├── base/                     # Shared node structures
├── expressions/              # Expression nodes
│   └── literals/             # Literal nodes
├── statements/               # Statement nodes
├── ts/                       # TypeScript syntax nodes
└── module/                   # Module declarations
```

## 3. AstNode base class

### 3.1 Kinds and casts

`AstNodeType` is a namespace-level enum populated from `AST_NODE_MAPPING` and related mappings in `astNodeMapping.h`. It is not an `AstNode::Type` nested enum.

The mapping macros generate `IsX()` and checked `AsX()` operations. Check the node kind before downcasting; do not bypass the assertions with `reinterpret_cast`.

### 3.2 Positions, parent, original node, and binding

`Start()`, `End()`, and `Range()` expose `SourcePosition`/`SourceRange`. `Parent()`/`SetParent()` manage parent links; `Original()`/`SetOriginal()` track origin where used. `Variable()`/`SetVariable()` attach binder information.

Preserve this metadata when constructing or replacing nodes. `IsProgram()` uses a null parent, so an unexpectedly detached node may be mistaken for a root by consumers.

### 3.3 Required node operations

Current abstract declarations from `astNode.h`:

```cpp
    virtual void Iterate(const NodeTraverser &cb) const = 0;
    virtual void Dump(ir::AstDumper *dumper) const = 0;
    virtual void Compile([[maybe_unused]] compiler::PandaGen *pg) const = 0;
    virtual void UpdateSelf([[maybe_unused]] const NodeUpdater &cb, [[maybe_unused]] binder::Binder *binder) = 0;
```

There is no universal base `Clone`/recursive `Transform` API matching the former guide's examples. Inspect the actual node and transformer contracts before implementing a rewrite.

## 4. Expression base class

`expression.h` defines the expression base and expression-specific state. TS annotations are syntax structures; there is no supported `typescript::Type *` checker payload as previously illustrated.

Bindings belong to the actual AST/binder interfaces. Parenthesization/grouping and annotation fields must survive relevant parser/transformer operations without being confused with inferred semantic types.

### 4.1 Identifier

`expressions/identifier.h` owns identifier syntax and name access. A resolved variable is available through the node's binding interface. Distinguish references from declarations, property names, labels, and generated identifiers before changing binding behavior.

### 4.2 Literals

`expressions/literals/` contains string, number, boolean, null, BigInt, regex, and tagged literals. Read the concrete `Tag()` implementation before using it as a discriminator.

`NULL_VALUE` does not mean an absent value: null, BigInt, and regex nodes can share that tag while having different semantics. Numbers may preserve their original source spelling; tagged literals encode method and affiliate data. Avoid forcing these into the old guide's invented nested `LiteralType` enum.

### 4.3 BinaryExpression

Current constructor and accessors begin as follows:

```cpp
    explicit BinaryExpression(Expression *leftExpr, Expression *rightExpr, lexer::TokenType operatorType)
        : Expression(AstNodeType::BINARY_EXPRESSION), left_(leftExpr), right_(rightExpr), operator_(operatorType)
    {
        if (right_->IsPrivateIdentifier()) {
            throw Error{ErrorType::SYNTAX, "Unexpect private identifier", right_->Start().line, right_->Start().index};
        }
    }

    const Expression *Left() const
    {
        return left_;
    }

    Expression *Left()
    {
        return left_;
    }

    const Expression *Right() const
    {
        return right_;
    }

    Expression *Right()
    {
        return right_;
    }

    lexer::TokenType OperatorType() const
    {
        return operator_;
    }
```

The accessor names are `Left()`, `Right()`, and `OperatorType()`, not `GetLeft()`/`GetRight()`/`GetOperator()`. Preserve operand order and ranges in updates; do not infer general constant-folding support from a documentation example.

### 4.4 CallExpression

`expressions/callExpression.h` defines callee, argument, optional-call, and TS type-argument representation. Inspect its actual constructor and accessors rather than copying the old guide's abbreviated signature.

Calls must preserve receiver binding, argument evaluation order, spreads, optional-chain short circuiting, and type-only argument removal during TS transformation.

## 5. Statement base class

`statement.h` defines the statement base. Statement nodes implement the same traversal/dump/compile/update contract as expressions.

### 5.1 BlockStatement

`statements/blockStatement.h` owns the statement list and associated scope state. When inserting or replacing statements, preserve order, source metadata, scope ownership, and parent/update conventions.

### 5.2 IfStatement

`statements/ifStatement.h` defines the test, consequent, and optional alternate. Traversal and updates must include all present children; compilation must preserve branch behavior and evaluation order.

### 5.3 VariableDeclaration

Current declaration kind and constructor:

```cpp
    enum class VariableDeclarationKind { CONST, LET, VAR };

    explicit VariableDeclaration(VariableDeclarationKind kind, ArenaVector<VariableDeclarator *> &&declarators,
                                 bool declare)
        : Statement(AstNodeType::VARIABLE_DECLARATION),
          kind_(kind),
          declarators_(std::move(declarators)),
          declare_(declare)
    {
    }
```

Use `Kind()`, `Declarators()`, and `Declare()` from the actual header. Distinguish declaration syntax, binder variable identity, initialization, hoisting, and runtime storage.

### 5.4 FunctionDeclaration

`statements/functionDeclaration.h` and the script-function representation coordinate name, parameters, body, flags, and scope. A function declaration is not adequately described by a standalone name-plus-body pseudo-constructor.

Test anonymous/default export cases, nested declarations, async/generator flags, and TS-transformed functions when changing related structures.

## 6. TypeScript nodes

### 6.1 TSTypeReference

`ts/tsTypeReference.h` represents a syntax reference and its type arguments. Check qualified names, generic instantiation, and traversal/update behavior. It is not a resolved semantic type object.

### 6.2 TSInterfaceDeclaration

`ts/tsInterfaceDeclaration.h` represents interface syntax, body, and heritage. Parser acceptance and transformed runtime erasure are separate verification points; an accepted interface must not accidentally emit runtime code.

### 6.3 TSEnumDeclaration

`ts/tsEnumDeclaration.h` represents enum declarations and members. Unlike interfaces, enums can require generated runtime structures and bindings. Verify member values, generated identifiers, exports, and rebinding after transformation.

## 7. AST traversal and transformation

`NodeTraverser` visits node children. `NodeUpdater` returns `UpdateNodes`, which can be a node pointer or a vector of nodes. Each concrete `Iterate`/`UpdateSelf` implementation determines how its children are visited or replaced.

Do not treat `Iterate()` as an automatic recursive whole-tree traversal: callers must follow the actual traversal contract. Do not mutate a parent vector from a callback without considering replacement/insertion and allocator lifetime rules.

A rewrite must preserve or deliberately update:

- Node kind and flags.
- Child order and optional children.
- Parent links and source ranges.
- Scope/binder associations.
- Origin metadata where consumed.
- Runtime evaluation order and side effects.

## 8. Adding a new AST node

1. Add the node kind/class mapping in `astNodeMapping.h` and inspect generated enum/check consumers in `astNode.h`.
2. Add the node header/implementation in the appropriate IR directory.
3. Implement `Iterate`, `Dump`, `Compile`, and `UpdateSelf` according to adjacent nodes.
4. Update parser construction and binder handling where needed.
5. Update TS transformation if the node is transformed or generated.
6. Register compilation units in the actual build source list.
7. Test dumps, traversal/update behavior, and emitted behavior.

Do not renumber existing kinds casually. Check every consumer before claiming an enum change is serialization-safe or serialization-breaking; AST kinds and runtime bytecode tags are different contracts.

## 9. AST dumping

`astDump.h`/`astDump.cpp` provide `AstDumper` support, and concrete nodes implement `Dump(AstDumper *)`. Add the node's dump properties where adjacent node implementations do, rather than inventing `AstDumper::Visit` overloads.

```bash
es2abc --dump-ast input.js
es2abc --extension ts --dump-transformed-ast input.ts
```

Compare expected child names, values, source-sensitive data, and nested structure. A correct dump alone does not prove binding or runtime correctness.

## 10. Related modules

- Producers: `es2panda/parser/` and `es2panda/parser/transformer/`.
- Consumers: `es2panda/binder/`, node compilation, and `es2panda/compiler/`.
- Supporting types: lexer source locations, strings, and program-owned arena allocation.
- The removed `es2panda/typescript/` checker is not a consumer in this checkout.

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
