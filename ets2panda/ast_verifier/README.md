# ASTVerifier

The `ASTVerifier` is a class that checks whether specific invariants hold over an Abstract Syntax Tree (AST) from a source code perspective. For `es2panda` users, it provides a set of configurable checks that can be controlled through CLI options.

## (Some of) Currently Implemented Checks

- [ ] For every node, each child has its `Parent()` pointer correctly set to that node
- [ ] (By the time of lowerings) Every typed node has a valid `TsType`
- [ ] Every identifier has a non-null `Variable()` reference
- [ ] All `LocalVariables` referred to by identifiers have scopes that properly enclose those identifiers
- [ ] No visibility rules are violated
- [ ] Scopes are properly nested
- [ ] Operands of arithmetic expressions have numeric types (except for `+` operator on strings)
- [ ] In `forIn`, `forOf`, and `forUpdate` statements:
  - The `left_` field is either an expression or a single variable declaration
  - In `forUpdate`, the variable has an initializer
  - In `forIn` and `forOf`, the variable has no initializer
- [ ] In sequence expressions, the type of the expression matches the type of its last member

## CLI Options

For complete and up-to-date options, run `es2panda --help`.

### Usage Examples

```sh
# Run verifier after each phase
es2panda --ast-verifier:each
```
Note:
- `before` runs only after parsing
- `after` runs only after all lowerings
- `each` runs after each phase

```sh
# Customize warning/error behavior
es2panda --ast-verifier:warnings=NodeHasParent:errors=ArithmeticOperationsValid,NodeHasType,NoPrimitiveTypes
```

## Adding a New Invariant

### Invariant Design Principles

An invariant should:
- Match patterns in AST subtrees that will be processed
- Explicitly handle exceptions by skipping non-matching subtrees
- Determine subtree correctness and report errors when needed

An invariant should not:
- Iterate over the AST or apply itself recursively (this would slow down verification as all invariants run in parallel)
- Call other invariants directly
  - If invariant `A` depends on invariant `B`:
    - Explicitly mark the dependency
    - `B` should execute first and prepare required data
    - Example: `NoPrimitiveType` depends on `NodeHasType`, so `NodeHasType` executes first and provides type information

### Implementation Steps

1. **Register the invariant**:
   - Add to `VerifierInvariants` enum in `util/options.yaml`
   - This generates the corresponding C++ enum entry

2. **Implement the check class**:
   - Location: `<repo_root>/ets2panda/ast_verifier/invariants`
   - Structure:
     ```cpp
     class SomeCheck : public InvariantBase<VerifierInvariants::SOME_CHECK> {
     public:
         using Base::Base;
         [[nodiscard]] CheckResult operator()(const ir::AstNode *ast) {...}
     };
     ```
   - Naming convention: Use descriptive names (e.g., `VariableInitialized` for variable initialization checks)

3. **Add documentation**:
   - Describe the expected condition
   - Specify when the check becomes relevant (which phase)
     - Control execution timing via `ASTVerifier::IntroduceNewInvariants(phaseName)` in `ASTVerifier.h`
   - Indicate whether it applies to:
     - Main program only
     - External sources as well

4. **Error handling**:
   - Use `AddCheckMessage` for error reporting
   - Return `CheckResult` tuples:
     - `CheckDecision::CORRECT` for passing analysis
     - `CheckDecision::INCORRECT` for failures
     - `CheckAction::CONTINUE` to proceed to children (in `ForAll` mode)
     - `CheckAction::SKIP_SUBTREE` to skip subtree processing

5. **Best practices**:
   - Use descriptive error messages
   - Study existing checks for implementation patterns
   - Maintain clear separation of concerns between invariants
