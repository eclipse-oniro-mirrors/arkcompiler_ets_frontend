## Non-Decimal bigint Literals Not Supported

**Rule:** `arkts-only-support-decimal-bigint-literal`

**Severity: error**

 Currently, non-decimal bigint literals are not supported. The migration tool will prompt developers to modify the source code but does not provide automatic fixes.

**ArkTS1.1**

```typescript
let a1: bigint = 0xBAD3n;  // Hexadecimal literal, not supported in ArkTS1.2
let a2: bigint = 0o777n;   // Octal literal, not supported in ArkTS1.2
let a3: bigint = 0b101n;  // Binary literal, not supported in ArkTS1.2
```

**ArkTS1.2**

```typescript
let a1: bigint = BigInt(0xBAD3);
let a2: bigint = BigInt(0o777);
let a3: bigint = BigInt(0b101);
```