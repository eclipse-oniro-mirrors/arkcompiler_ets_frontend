## Negative Array Indices Not Supported

**Rule:** `arkts-array-index-negative`

**Severity: error**

ArkTS1.2 does not support using negative integers to access array elements.

**ArkTS1.1**

```typescript
let an_array = [1, 2, 3];
let element = an_array [-1];
console.log(getElement(an_array, -1)); // Violates the rule
for (let i: int = -1; i < an_array.length; i++) { // Violates the rule
  console.log(an_array[i]);
}

function getElement(arr: number[], index: int) {
  return arr[index]; // May accept negative indices
}
```

**ArkTS1.2**

```typescript
let an_array = [1, 2, 3];
let element = an_array [1];
console.log(getElement(an_array, 1)); // Pass non-negative indices
for (let i: int = 0; i < an_array.length; i++) { // Only non-negative indices allowed
  console.log(an_array[i]);
}

function getElement(arr: number[], index: int) {
  if (index < 0) throw new Error("Index must be a non-negative integer");
  return arr[index]; // Only non-negative integers allowed
}
```