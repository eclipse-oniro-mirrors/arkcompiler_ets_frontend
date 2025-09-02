## 不支持TS-like `Function`类型的调用方式

**规则：**`arkts-no-ts-like-function-call`

**级别：error**

ArkTS1.2会对函数类型进行更严格的编译器检查。函数返回类型需要严格定义来保证类型安全，因此不支持TS-like`Function`类型。

**ArkTS1.1**

```typescript
let f: Function = () => {} // 违反规则

function run(fn: Function) {  // 违反规则
  fn();
}

let fn: Function = (x: number) => x + 1; // 违反规则

class A {
  func: Function = () => {}; // 违反规则
}

function getFunction(): Function { // 违反规则
  return () => {};
}
```

**ArkTS1.2**

```typescript
type F<R> = () => R;
type F1<P, R> = (p:  P) => R

let f: F<void> = () => {}

function run(fn: () => void) {  // 指定返回类型
  fn();
}

let fn: (x: number) => number = (x) => x + 1; // 明确参数类型

class A {
  func: () => void = () => {}; // 明确类型
}

function getFunction(): () => void { // 明确返回类型
  return () => {};
}
```
