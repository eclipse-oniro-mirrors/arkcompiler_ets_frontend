## Enum Elements Cannot Be Used as Types

**Rule:** `arkts-no-enum-prop-as-type`

**Severity: error**

In ArkTS1.1, enums are compile-time concepts and remain ordinary objects at runtime. ArkTS1.2 follows static typing, requiring runtime types for enums. 
Therefore, each element of an enum in ArkTS1.2 is an instance of the enum class (determined at runtime) and cannot serve as compile-time static type information. 
This contradicts ArkTS1.2's overall type design, which does not support instance types.

**ArkTS1.1**

```typescript
enum A { E = 'A' }
function foo(a: A.E) {}
```

**ArkTS1.2**

```typescript
enum A { E = 'A' }
function foo(a: 'A') {}

// ...
enum A { E = 'A' }
function foo(a: A) {}
```