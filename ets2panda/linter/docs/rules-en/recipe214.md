## Objects Have No constructor

**Rule:** `arkts-obj-no-constructor`

**Severity: error**

ArkTS1.2 supports native sharing capabilities, requiring type information to be determined at runtime. The implementation is no longer prototype-based but class-based.

**ArkTS1.1**

```typescript
class A {}
let a = new A().constructor;   // ArkTS1.2 compile error
```

**ArkTS1.2**

```typescript
class A {}
let a = new A();
let cls = Type.of(a); 
```