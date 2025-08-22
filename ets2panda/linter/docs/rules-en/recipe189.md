## Semantic Changes for Numeric Types

**Rule:** `arkts-numeric-semantic`

**Severity:** error

In ArkTS1.2, for better execution efficiency, integer literals default to the int type.

**ArkTS1.1**
```typescript
let n = 1;
console.log(n / 2)  // output: 0.5

let arr = [1, 2, 3];

function multiply(x = 2, y = 3) { // Explicit types needed
  return x * y;
}

function divide(x: number, y: number) {
  return x / y;
} // Function return value

let num = Math.floor(4.8); // num may be int
let value = parseInt("42"); // value may be int

function identity<T>(value: T): T {
  return value;
}
identity(42); // 42 may be inferred as int
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