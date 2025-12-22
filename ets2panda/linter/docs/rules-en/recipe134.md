## Definite Assignment Assertions Not Supported

**Rule:** `arkts-no-definite-assignment`

**Severity: error**

ArkTS1.2 does not support definite assignment assertions. Initialize variables at declaration instead.

**ArkTS1.1**

```typescript
let x!: number // Hint: Initialize 'x' before use

initialize();

function initialize() {
  x = 10;
}

console.log('x = ' + x);
```

**ArkTS1.2**

```typescript
function initialize(): number {
  return 10;
}

let x: number = initialize();

console.log('x = ' + x);
```