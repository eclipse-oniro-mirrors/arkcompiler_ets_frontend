## Side-Effect Imports Not Supported

**Rule:** `arkts-no-side-effect-import`

**Severity: error**

ArkTS1.2 supports lazy loading by default and cannot achieve side effects during imports.

**ArkTS1.1**

```typescript
// logger.ets
console.log("Logger initialized!");

// main.ets
import "./logger";
console.log("Main program running...");
```

**ArkTS1.2**

```typescript
// main.ets
console.log("Logger initialized!");
console.log("Main program running...");
```