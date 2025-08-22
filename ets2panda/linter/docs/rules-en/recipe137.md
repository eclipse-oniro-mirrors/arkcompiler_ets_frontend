## globalThis Not Supported

**Rule:** `arkts-no-globalthis`

**Severity: error**

Since ArkTS1.2 does not support dynamically modifying object layouts, it does not support global scope or globalThis.

**ArkTS1.1**

```typescript
// Global file
var abc = 100;

// Reference 'abc' from above
let x = globalThis.abc;
```

**ArkTS1.2**

```typescript
// file1
export let abc: number = 100;

// file2
import * as M from 'file1'

let x = M.abc;
```