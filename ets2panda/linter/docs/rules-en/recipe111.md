## Enum Members Cannot Be of Mixed Types

**Rule:** `arkts-no-enum-mixed-types`

**Severity: error**

Enums represent discrete data sets. Using floating-point numbers in enums contradicts their design and may cause precision issues. Therefore, enum values in ArkTS1.2 must be integers.

**ArkTS1.1**

```typescript
enum Size {
  UP = 1.5,
  MIDDLE = 1,
  DOWN = 0.75
}
```

**ArkTS1.2**

```typescript
enum Size{ 
  UP = 1,
  MIDDLE = 2,
  DOWN = 3
}
```