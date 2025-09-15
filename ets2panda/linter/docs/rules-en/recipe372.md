## Smart Type Differences

**Rule:** `arkts-no-ts-like-smart-type`

**Severity: error**

In ArkTS1.1, since objects are not shared between threads, the compiler does not need to consider concurrency during type inference and analysis.

In ArkTS1.2, since objects are shared across threads, the compiler must consider changes in variable types/values in concurrent scenarios during type inference and analysis.

**Smart Casting：** The compiler automatically casts variables to specific types in certain scenarios (e.g., instanceof, null checks, context inference) without manual casting.

**ArkTS1.1**

```typescript
class AA {
  public static instance?: number;
  getInstance(): number {
    if (!AA.instance) {
      return 0;
    }
    return AA.instance;       // ArkTS1.2 compile error, return value and type mismatch
  }
}
```

**ArkTS1.2**

```typescript
class AA {
  public static instance?: number;
  getInstance(): number {
    let a = AA.instance       // In ArkTS1.2, use local variables for smart casting
    if (!a) {
      return 0;
    }
    return a;
  }
}
```