## Lazy Loading for Classes

**Rule:** `arkts-class-lazy-import`

**Severity: error**

ArkTS1.2 loads or initializes classes when they are used, improving startup performance and reducing memory usage.

**ArkTS1.1**

```typescript
class C {
  static {
    console.info('init');  // n ArkTS1.2, this does not execute immediately
  }
}
```

**ArkTS1.2**

```typescript
// ArkTS1.2   If logic depends on unused classes, move that logic outside the class
class C {
  static {}
}
console.info('init');
```