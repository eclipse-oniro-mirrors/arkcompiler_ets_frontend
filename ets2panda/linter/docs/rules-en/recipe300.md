## TypeScript-Like Function Type Calls Not Supported

**Rule:** `arkts-no-ts-like-function-call`

**Severity: error**

ArkTS1.2 enforces stricter compiler checks for function types. Function return types must be strictly defined to ensure type safety, so TypeScript-like Function types are not supported.

**ArkTS1.1**

```typescript
let f: Function = () => {} // Violates the rule

function run(fn: Function) {  // Violates the rule
  fn();
}

let fn: Function = (x: number) => x + 1; // Violates the rule

class A {
  func: Function = () => {}; // Violates the rule
}

function getFunction(): Function { // Violates the rule
  return () => {};
}
```

**ArkTS1.2**

```typescript
type F<R> = () => R;
type F1<P, R> = (p:  P) => R

let f: F<void> = () => {}

function run(fn: () => void) {  // Specify return type
  fn();
}

let fn: (x: number) => number = (x) => x + 1; // Explicit parameter type

class A {
  func: () => void = () => {}; // Explicit type
}

function getFunction(): () => void { // Explicit return type
  return () => {};
}
```