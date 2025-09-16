## void类型只能用在返回类型的场景

**规则：** `arkts-limited-void-type`

**级别：** error

在ArkTS1.2中，void仅作为类型使用。void类型没有实体。

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
