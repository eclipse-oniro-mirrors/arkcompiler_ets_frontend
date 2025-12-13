## Runtime Array Bounds Checking Added

**Rule:** `arkts-runtime-array-check`

**Severity: error**

To ensure type safety, ArkTS1.2 validates the legality of indices when accessing array elements.

**ArkTS1.1**

```typescript
let a: number[] = []
a[100] = 5; // May be out of bounds
```

**ArkTS1.2**

```typescript
let a: number[] = []
if (100 < a.length) {
  a[100] = 5  // a[100] is 5
}
```