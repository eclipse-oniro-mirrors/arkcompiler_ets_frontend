### ArkTS1.2对js对象进行条件判断

**规则：** arkts-interop-js2s-condition-judgment

**级别：** error

**ArkTS1.1**
```typescript
// file1.js
export let foo = { isGood: true };

// file2.ets
import { foo } from './file1';

if (foo.isGood) {}
```

**ArkTS1.2**
```typescript
// file1.js
export let foo = { isGood: true };

// file2.ets
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');

let isGood = foo.getProperty('isGood').toBoolean();
if (isGood) {}
```
