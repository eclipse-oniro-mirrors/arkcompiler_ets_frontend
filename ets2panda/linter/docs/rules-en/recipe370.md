## Empty/Sparse Arrays Not Supported

**Rule:** `arkts-no-sparse-array`

**Severity: error**

1. ArkTS1.2 follows static typing. Empty arrays must infer the element type from the context; otherwise, a compile error occurs.

2. Arrays in ArkTS1.2 are stored contiguously. Gaps (e.g., [1, , , 2]) waste memory.

3. ArkTS1.2 follows null-safety principles and cannot use undefined by default to represent gaps.

**ArkTS1.1**

```typescript
let a = []; // ArkTS1.2, compile error, array type must be inferred from context
let b = [1, , , 2]; // Gaps in arrays are not supported
b[1];  // undefined 
```

**ArkTS1.2**

```typescript
let a: number[] = [];  // Supported, ArkTS1.2 can infer the type from context
let b = [1, undefined, undefined, 2];
```