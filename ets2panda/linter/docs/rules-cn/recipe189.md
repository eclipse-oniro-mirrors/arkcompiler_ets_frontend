## 数值类型语义变化

**规则：** `arkts-numeric-semantic`

**级别：** error

在ArkTS1.2中，为了获得更好的执行效率，整型数字字面量默认是int类型。

**ArkTS1.1**
```typescript
let n = 1;
console.log(n / 2)  // output: 0.5

let arr = [1, 2, 3];

function multiply(x = 2, y = 3) { // 需要明确类型
  return x * y;
}

function divide(x: number, y: number) {
  return x / y;
} // 函数返回值

let num = Math.floor(4.8); // num 可能是 int
let value = parseInt("42"); // value 可能是 int

function identity<T>(value: T): T {
  return value;
}
identity(42); // 42 可能推导为 int
```

**ArkTS1.2**
```typescript
let n: number = 1;
console.log(n / 2)  // output: 0.5

let m = 1;
console.log(m / 2)  // output: 0

let arr: number[] = [1, 2, 3];

function multiply(x: number = 2, y: number = 3): number {
  return x * y;
}

function divide(x: number, y: number): number {
  return x / y;
}

let num: number = Math.floor(4.8);
let value: number = parseInt("42");

function identity<T>(value: T): T {
  return value;
}
identity(42 as number);
```
