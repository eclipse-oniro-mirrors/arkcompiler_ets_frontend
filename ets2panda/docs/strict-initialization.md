# Experimental strict mode

Issue: [#12306](https://gitcode.com/openharmony/arkcompiler_ets_frontend/issues/12306).

`--strict=true` enables both scalar initialization checking and boolean-only
conditions. Ordinary compilation defaults to false. `--gen-stdlib=true` defaults
to strict mode; an explicit `--strict=false` overrides that default for manual
diagnostics. Official CMake/GN standard-library builds pass `--strict=true`.
Importing std does not change the consuming compilation's default.
No arktsconfig field is required:

```sh
es2panda --extension=ets --strict=true input.ats --output=input.abc
```

## Initialization

`number`, `byte`, `short`, `int`, `long`, `float`, `double`, `char`, and `boolean`
variables and ordinary fields follow the existing non-null Object assignment
analysis. Declaration-time initializers are optional if later assignments meet
the existing requirements. Unread locals remain permitted. Nullish defaults,
late-initialized fields, array components and external declarations retain their
existing rules. Existing Object analysis boundaries remain, including incomplete
analysis of some abrupt loop exits; this option is not a new whole-program
assignment analysis.

An uninitialized variable's loose null/undefined comparison is treated as a read.
The strict policy merges assignment states after short-circuit expression
statements and checks operands of constant-result conditions. These additions
are disabled when strict mode is false. Runtime storage clearing is unchanged.

## Boolean expressions

Conditions of if/while/do/for/ternary, both built-in &&/|| operands and the !
operand must have an effective boolean type. Aliases, boxed boolean forms,
boolean-constrained generics and existing use-site narrowing are supported.
Unnarrowed nullable booleans, Any, objects and numeric/string truthiness are
rejected. A void right operand of &&/|| is rejected; `!!value` is not a truthiness
conversion. Runtime short-circuit behavior is preserved. Omitted for conditions,
ternary result types, ??, optional chaining, switch and integer bitwise operations
retain their existing rules. No new operator syntax is introduced.

Use explicit comparisons or type checks to obtain boolean values.

The pre-existing ES2PANDA_STRICT diagnostic policy, hard/soft lists and werror are
independent of this option.
