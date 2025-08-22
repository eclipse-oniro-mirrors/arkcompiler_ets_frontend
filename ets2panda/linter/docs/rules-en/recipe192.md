## void Type Limited to Return Scenarios

**Rule:** `arkts-limited-void-type`

**Severity:** error

In ArkTS1.2, void is only used as a type and has no concrete value.

**ArkTS1.1**
```typescript
let s: void = foo();
let t: void | number = foo();

function process<T>(input: T): T {
  return input;
}
let result = process<void>(foo()); 

type VoidAlias = void; 

let { x }: { x: void } = { x: foo() };

function execute(callback: void) {
  callback();
}

let x = fun() as void;
```

**ArkTS1.2**
```typescript
function foo(): void {}
foo();

function bar(): void {}

function execute(callback: () => void) {
  callback();
}
fun();
```