## lazy Keyword Not Supported

**Rule:** `arkts-no-lazy-import`

**Severity: error**

ArkTS1.2 supports lazy loading by default, so the lazy keyword is unnecessary.

**ArkTS1.1**

```typescript
import lazy { m } from 'module'
import lazy { a, b } from 'module1'; // Violates the rule
import { c } from 'module2';
```

**ArkTS1.2**

```typescript
import { m } from 'module'
import { a, b } from 'module1'; // Remove `lazy`
import { c } from 'module2';
```