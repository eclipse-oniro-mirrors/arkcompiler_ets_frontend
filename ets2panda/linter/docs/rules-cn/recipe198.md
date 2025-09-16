## 不支持类TS重载

**规则：**`arkts-no-ts-overload`

**级别：error**

ArkTS1.2不支持TS-like的重载，使用不同的函数体可以提高执行效率。

**ArkTS1.1**

```typescript
function foo(): void
function foo(x: string): void
function foo(x?: string): void { // 违反规则
  /*body*/
}

function sum(x: number, y: number): number;
function sum(x: number, y: number, z: number): number;
function sum(x: number, y: number, z?: number): number {  // 违反规则
  return z ? x + y + z : x + y;
}

function foo(): string;
function foo(x: number): number;
function foo(x?: number): string | number {  // 违反规则
  return x !== undefined ? x * 2 : "default";
}
```

**ArkTS1.2**

```typescript
function foo(x?: string): void {
  /*body*/
}

function sumTwo(x: number, y: number): number {  // 独立实现
  return x + y;
}

function sumThree(x: number, y: number, z: number): number {  // 独立实现
  return x + y + z;
}

function fooString(): string {  // 独立实现
  return "default";
}

function fooNumber(x: number): number {  // 独立实现
  return x * 2;
}
```
