## Enum Members Cannot Be Accessed via Index

**Rule:** `arkts-enum-no-props-by-index`

**Severity: error**

1. ArkTS1.1 already restricts syntax for accessing elements via index. ArkTS1.2 strengthens constraints for enum scenarios. For details, refer to Unsupported Field Access via Index.

2. Enums in ArkTS1.1 are dynamic objects, while ArkTS1.2 uses static typing, giving enums runtime types. For better performance, [] access is restricted.

**ArkTS1.1**

```typescript
enum TEST {
  A,
  B,
  C
}

TEST['A'];       // This syntax is not supported in ArkTS1.2
TEST[0];    // This syntax is not supported in ArkTS1.2
```

**ArkTS1.2**

```typescript
enum TEST {
  A,
  B,
  C
}

TEST.A;          // Use `.` or enum values
TEST.A.getName();  // Use enum methods to get the enum key
```