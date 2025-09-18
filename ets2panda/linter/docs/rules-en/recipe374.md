## new Number/Boolean/String No Longer of Type "object"

**Rule:** `arkts-primitive-type-normalization`

**Severity: error**

1. In ArkTS1.2, primitive types and boxed types are the same type, improving language consistency and performance.
   Comparisons of Number/Boolean/String compare values, not objects.

2. In ArkTS1.1, boxed types are created via new. When obtaining their type or comparing boxed type objects, unexpected behavior occurs because object comparisons are by reference, 
   not value. Using primitive types directly is more efficient and consumes less memory.

**ArkTS1.1**

```typescript
typeof new Number(1) // Result: "object"
new Number(1) == new Number(1);  //Result: false
if (new Boolean(false)) {} // In `if` statements, `new Boolean(false)` is `true`
```

**ArkTS1.2**

```typescript
typeof new Number(1)// Result: "number"
new Number(1) == new Number(1);  //Result: true
if (new Boolean(false)) {}      // In `if` statements, `new Boolean(false)` is `false`
```