## TypeScript-Like Overloading Not Supported

**Rule:** `arkts-no-ts-overload`

**Severity: error**

ArkTS1.2 does not support TypeScript-like overloading. Using different function bodies improves execution efficiency.

**ArkTS1.1**

```typescript
function foo(): void
function foo(x: string): void
function foo(x?: string): void { // Violates the rule
  /*body*/
}

function sum(x: number, y: number): number;
function sum(x: number, y: number, z: number): number;
function sum(x: number, y: number, z?: number): number {  // Violates the rule
  return z ? x + y + z : x + y;
}

function foo(): string;
function foo(x: number): number;
function foo(x?: number): string | number {  // Violates the rule
  return x !== undefined ? x * 2 : "default";
}
```

**ArkTS1.2**

```typescript
function foo(x?: string): void {
  /*body*/
}

function sumTwo(x: number, y: number): number {  // Independent implementation
  return x + y;
}

function sumThree(x: number, y: number, z: number): number {  // Independent implementation
  return x + y + z;
}

function fooString(): string {  // Independent implementation
  return "default";
}

function fooNumber(x: number): number {  // Independent implementation
  return x * 2;
}
```