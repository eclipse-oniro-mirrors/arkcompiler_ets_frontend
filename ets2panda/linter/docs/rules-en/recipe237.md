## arkts-no-tuples-arrays

**Rule:** `arkts-no-tuples-arrays`

**Severity: error**

In ArkTS1.2, arrays and tuples are distinct types. Using tuple types at runtime can improve performance.

**ArkTS1.1**

```typescript
const tuple: [number, number, boolean] = [1, 3.14, true];
const array: (number|boolean) [] = tuple;

const tuple: Array<number | boolean> = [1, 3.14, true];  // Violates the rule

function getTuple(): (number | boolean)[] {  // Violates the rule
  return [1, 3.14, true];
}
getTuple([1, 3.14, true]);  // Pass a tuple

type Point = (number | boolean)[];  //Violates the rule
const p: Point = [3, 5, true];
```

**ArkTS1.2**

```typescript
const tuple: [number, number, boolean] = [1, 3.14, true];
const array:  [number, number, boolean] = tuple;

const tuple: [number, number, boolean] = [1, 3.14, true];  // Correct tuple usage

function getTuple(): [number, number, boolean] {  // Correct tuple usage
  return [1, 3.14, true];
}
getTuple([1, 3.14, true]);

type Point = [number, number, boolean];  // Use tuples
const p: Point = [3, 5, true];
```