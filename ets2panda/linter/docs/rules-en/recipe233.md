## Dynamic import Not Supported

**Rule:** `arkts-no-dynamic-import`

**Severity: error**

ArkTS1.2 supports lazy loading of modules by default.

**ArkTS1.1**

```typescript
function main(): void {
  import('./file').then((m) => {
    console.log(m.Data.name)
  })
}

document.getElementById("btn")?.addEventListener("click", async () => {
  const module = await import('./utils');  // Error: Dynamic `import()` is not supported in ArkTS.
  module.doSomething();
});

function getModule() {
  return import('./heavyModule')  // Error: Dynamic `import()` is not supported in ArkTS.
    .then((m) => m.default);
}
```

**ArkTS1.2**

```typescript
import { Data } from './file'
import { doSomething } from './utils';  // Static imports are allowed.
import heavyModule from './heavyModule';  // Static imports are allowed.

function main(): void {
  console.log(Data.name)
}

document.getElementById("btn")?.addEventListener("click", () => {
  doSomething();
});

function getModule() {
  return heavyModule;
}
```