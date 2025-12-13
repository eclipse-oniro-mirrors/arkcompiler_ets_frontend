## Restricted Keywords

**Rule:** `arkts-invalid-identifier`

**Severity:** error

ArkTS1.2 strictly defines keywords and reserved words. These keywords cannot be used as variable names.

**ArkTS1.1**
```typescript
let as: number = 1;
const abstract: string = "abstract";
```

**ArkTS1.2**
```typescript
let a = 1;
const abstract1: string = "abstract";
```