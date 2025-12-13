## Comparisons Between Numeric and bigint Types Not Supported

**Rule:** `arkts-numeric-bigint-compare`

**Severity: error**

 Currently, comparisons between numeric and bigint types are not supported. The migration tool will prompt developers to modify the source code but does not provide automatic fixes.

**ArkTS1.1**

```typescript
let n1: number = 123;
let n2: bigint = 456n;

n1 <= n2;   // Compiles
n1 == n2;   // Fails to compile
n1 >= n2;   // Compiles
```

**ArkTS1.2**

```typescript
let n1: number = 123;
let n2: bigint = 456n;

BigInt(n1) <= n2;
BigInt(n1) == n2;
BigInt(n1) >= n2;
```