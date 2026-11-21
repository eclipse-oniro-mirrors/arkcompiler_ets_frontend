# Binder Module - AGENTS.md

> Maintenance: When using an agent to modify code, any new code structure or functional feature must be reflected in this AGENTS.md in the same change.

## 1. Module responsibilities

Binder resolves declarations and identifier references, builds scopes, tracks function scopes, and supplies binding information to bytecode compilation. It also handles TS declaration bindings and the binding changes needed after TS transformation.

Read the repository `AGENTS.md`, `es2panda/AGENTS.md`, and `docs/knowledge/ES2Panda_Knowledge_Base.md`. This is the dynamic route; static `ets2panda/varbinder` and checker assumptions do not define its behavior.

## 2. Directory structure

```text
es2panda/binder/
├── binder.cpp             # Identifier and declaration analysis
├── binder.h               # Binder interface and helpers
├── scope.cpp              # Scope behavior and lookup
├── scope.h                # Scope hierarchy and binding maps
├── variable.cpp           # Variable behavior
├── variable.h             # Variable hierarchy and storage bindings
├── declaration.cpp        # Declaration behavior
├── declaration.h          # Declaration kinds
├── variableFlags.h        # Binding flags and scope-related enums
├── tsBinding.h            # TypeScript binding helpers
└── enumMemberResult.h     # Enum member evaluation results
```

## 3. Core classes and APIs

### 3.1 Binder

Selected current declarations from `binder.h`:

```cpp
    void InitTopScope();
    void IdentifierAnalysis(ResolveBindingFlags flags = ResolveBindingFlags::ALL);

    template <typename T, typename... Args>
    T *AddDecl(const lexer::SourcePosition &pos, bool isDeclare, Args &&... args);

    template <typename T, typename... Args>
    T *AddDecl(const lexer::SourcePosition &pos, DeclarationFlags flag, bool isDeclare, Args &&... args);

    template <typename T, typename... Args>
    T *AddTsDecl(const lexer::SourcePosition &pos, bool isDeclare, Args &&... args);

    template <typename T, typename... Args>
    T *AddTsDecl(const lexer::SourcePosition &pos, DeclarationFlags flag, bool isDeclare, Args &&... args);

    ParameterDecl *AddParamDecl(const ir::AstNode *param);

    Scope *GetScope() const
    {
        return scope_;
    }

    FunctionScope *TopScope() const
    {
        return topScope_;
    }
```

`Binder` is constructed with a `parser::Program *` and `ScriptExtension`. Its allocator comes from the program. `GetScope()` returns the active scope; `TopScope()` returns the top function scope. Avoid adding documentation-only helpers such as `PushScope()` or `LookupVariable()` without checking whether they exist.

### 3.2 Scope

`scope.h` owns the scope hierarchy, parent relationships, and binding maps. Consult it for concrete scope constructors and lookup-result types. A scope lookup may carry more information than a variable pointer; callers must preserve that information.

`VariableScope`, `FunctionScope`, `ModuleScope`, `BlockScope`, `CatchScope`, and loop-related scopes serve different purposes. Use the existing scope-management helpers rather than manually changing the binder's current-scope pointer.

### 3.3 Variable

`variable.h` distinguishes local, global, module, enum, namespace, and TS-specific variables. Selected current base-class operations:

```cpp
    Decl *Declaration() const
    {
        return decl_;
    }

    VariableFlags Flags() const
    {
        return flags_;
    }

    void AddFlag(VariableFlags flag)
    {
        flags_ |= flag;
    }

    bool HasFlag(VariableFlags flag) const
    {
        return (flags_ & flag) != 0;
    }

    void RemoveFlag(VariableFlags flag)
    {
        flags_ &= ~flag;
    }

    void Reset(Decl *decl, VariableFlags flags)
    {
        decl_ = decl;
        flags_ = flags;
    }
```

`LocalVariable` can bind a virtual register or a lexical-environment slot. `ModuleVariable` carries a module index. The variable's declaration, flags, and storage choice must remain consistent.

Flags are defined in `variableFlags.h`; they are not the illustrative nested enum previously shown in this guide. Check existing flag meanings before adding a new one.

## 4. Identifier resolution flow

```text
Parsing and declaration creation
    → identifier analysis in scope context
    → resolved variable references
    → TS transformation and rebinding when applicable
    → register / lexical / module access during compilation
```

### 4.1 Example to trace

```javascript
let x = 1;
function foo() {
    console.log(x);
    let y = x + 1;
    return y;
}
```

For `x` inside `foo`, inspect the declaration's scope, the reference's scope chain, and whether the access crosses a function boundary. For `y`, inspect the local declaration and initialization order. Global `console` must not be treated as an ordinary captured local.

`IdentifierAnalysis` is the real analysis entry. Trace its traversal and resolution helpers in `binder.cpp`; do not implement the former guide's simplistic recursive lookup that creates a global for every missing declaration.

## 5. Scope categories and behavior

| Scope category | What to inspect |
|---|---|
| Global/top-level | Script bindings, global access, function declarations |
| Function/parameters | Parameters, function-local declarations, `arguments`, nested functions |
| Block | Lexical `let`/`const` declarations and visibility |
| Class-related | Class names, private names, methods, and surrounding scopes |
| Catch | Catch parameter and body visibility |
| Loop | Per-iteration bindings and captured loop variables |
| Module | Import/export bindings and module record indices |

Scope construction and declaration collection interact with parsing. A behavior change often needs both `parser/` and binder inspection.

## 6. Hoisting

### 6.1 Language examples

```javascript
console.log(x);  // undefined for an ordinary var declaration
var x = 1;

foo();          // ordinary function declaration can be called before its text
function foo() {}
```

```javascript
console.log(y);  // temporal-dead-zone access
let y = 2;
```

Lexical declarations also participate in scope setup; saying they are simply absent until their declaration is misleading. Visibility, initialization state, and runtime access checks are distinct.

### 6.2 Implementation locations

Inspect declaration creation and scope insertion in `binder.cpp`/`scope.cpp`, then inspect `compiler/base/hoisting.cpp` and lexical-variable loads. Do not document all TDZ failures as binder-thrown errors: emitted checks and runtime behavior matter.

Tests should cover `var`, function declarations, lexical declarations, nested scopes, shadowing, redeclarations, and accesses before initialization.

## 7. Closures and captured variables

```javascript
function outer() {
    let x = 1;
    return function inner() {
        return x;
    };
}
```

The reference to `x` crosses a function boundary. Follow the resolution path and `LocalVariable::SetLexical`, then inspect lexical-environment allocation in `compiler/base/lexenv.cpp` and `compiler/core/envScope.cpp`.

Preserve the declaration identity while changing storage. Captured loop variables need per-iteration semantics; a change that passes the simple nested-function example may still break `for` loops, `catch`, arrows, or parameter defaults.

## 8. TypeScript bindings

`tsBinding.h`, TS declarations in `declaration.h`, and TS-specific variables in `variable.h` support names such as enums and namespaces. They do not constitute a full TypeScript type checker.

TS input is analyzed before transformation and rebound afterward. Inspect `ResolveBindingFlags::TS_BEFORE_TRANSFORM`, `TS_AFTER_TRANSFORM`, and `Compiler::ProcessAstForTS` in `es2panda.cpp`. Generated identifiers must resolve in their intended scope, and transformed enum/namespace bindings must remain stable.

## 9. Modification guidance

### 9.1 Add a variable or declaration kind

1. Inspect the existing declaration, variable subclass, and flags.
2. Update scope insertion and redeclaration rules.
3. Update identifier resolution and storage allocation consumers.
4. Cover local, captured, module, and TS-transformed uses as applicable.

### 9.2 Change scope rules

1. Locate scope construction and lookup callers before changing lookup behavior.
2. Preserve shadowing and initialization-state handling.
3. Test nested scopes, parameters, loops, catch bindings, and module exports.
4. Check generated bytecode and runtime behavior if storage or access changes.

### 9.3 Change TS bindings

1. Update the actual helper in `tsBinding.h` or binder implementation.
2. Keep the JS path intact.
3. Test both pre-transform and post-transform identifiers.
4. Include enums/namespaces and relevant export cases.

## 10. Related modules

- Upstream: `es2panda/parser/` creates AST nodes and declarations.
- Downstream: `es2panda/parser/transformer/` and `es2panda/compiler/` consume bindings.
- Shared structures: `es2panda/ir/`, program-owned arena allocation, and `es2panda/util/ustring.h`.
- There is no downstream `es2panda/typescript/` checker in this checkout.

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
