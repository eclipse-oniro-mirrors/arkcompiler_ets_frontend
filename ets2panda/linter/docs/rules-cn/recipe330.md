### ArkTS1.2导入js文件

**规则：** arkts-interop-js2s-import-js

**级别：** error

**ArkTS1.1**
```typescript
// file1.js
export function foo() {}

// file2.ets
import { foo } from './file1';
```

**ArkTS1.2**
```typescript
// file1.js
export function foo() {}

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
```
