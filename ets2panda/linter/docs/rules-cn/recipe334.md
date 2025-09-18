### ArkTS1.2获取js对象类型

**规则：** `arkts-interop-js2s-typeof-js-type`

ArkTS1.2使用ESValue接口获取js对象类型。

**ArkTS1.1**
```typescript
// file1.js
export let foo = { num: 123 };

// file2.ets
import { foo } from './file1';
typeof foo.num; // 'number'
```

**ArkTS1.2**
```typescript
// file1.js
export let foo = 123;

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
let num = foo.getProperty('num');

num.typeOf(); // 'number'
```