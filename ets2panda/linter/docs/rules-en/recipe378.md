## Logical Assignment Operators Not Supported

**Rule:** `arkts-unsupport-operator`

**Severity: error**

1. Currently, &&=, ||=, and ??= logical assignment operators are not supported. The migration tool will prompt developers to modify the source code but does not provide automatic fixes.

**ArkTS1.1**

```typescript
let a = 1;
a &&= 2;    // Result: 2, not supported in ArkTS1.2
a ||= 3;   // Result: 2, not supported in ArkTS1.2
a ??= 4;  // Result: 2, not supported in ArkTS1.2
```

**ArkTS1.2**

```typescript
let a = 1;
a = a && 2;   // Result: 2
a = a || 3;   // Result: 2
a = a ?? 4;   // Result: 2
```