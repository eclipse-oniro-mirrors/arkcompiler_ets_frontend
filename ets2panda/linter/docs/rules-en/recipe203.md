## Exponentiation Operator Not Supported

**Rule:** `arkts-no-exponent-op`

**Severity: error**

ArkTS1.2 does not support the exponentiation operator (** and **=). Use the language's built-in library instead.

**ArkTS1.1**

```typescript
let x = 2 ** 5;

let y = 3;
y **= 4; // Violates the rule

let result = (1 + 2) ** (3 * 2); // Violates the rule

function power(base: number, exponent: number) {
  return base ** exponent; // Violates the rule
}

let values = [1, 2, 3];
let squared = values.map(v => v ** 2); // Violates the rule
```

**ArkTS1.2**

```typescript
let x = Math.pow(2, 5);

let y = 3;
y = Math.pow(y, 4); // Use `Math.pow()` directly

let result = Math.pow(1 + 2, 3 * 2); // Use `Math.pow()` directly

function power(base: number, exponent: number) {
  return Math.pow(base, exponent); // Use `Math.pow()`
}

let values = [1, 2, 3];
let squared = values.map(v => Math.pow(v, 2)); // Use `Math.pow()`
```