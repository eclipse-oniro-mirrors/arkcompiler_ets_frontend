### ArkTS1.2访问js索引

**规则：** arkts-interop-js2s-access-js-index

**级别：** error

**ArkTS1.1**
```typescript
// file1.js
export let foo = { arr: [1, 2, 3] };
// file2.ets
import { foo } from './file1';
let arr = foo.arr;
arr[1];
arr[3] = 4;
```

**ArkTS1.2**
```typescript
// file1.js
export let foo = [1, 2, 3];

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
let arr = foo.getProperty('arr');
arr.getProperty(1);
arr.setProperty(3, ESValue.wrap(4));
```
