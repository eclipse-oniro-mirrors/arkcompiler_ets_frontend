## debugger Not Supported

**Rule:** `arkts-no-debugger`

**Severity: error**

1. Static-typed languages have compile-time checks and strong type constraints, and debugging is typically handled by IDEs, which already provide robust debugging mechanisms.

2. debugger statements intrusively modify source code.

3. debugger statements may be optimized, leading to inconsistent behavior.

**ArkTS1.1**

```typescript
// ArkTS1.1 
// ...
debugger;
// ...
```

**ArkTS1.2**

```typescript
// ArkTS1.2   Remove `debugger` statements
// ...
```