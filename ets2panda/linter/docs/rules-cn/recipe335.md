### ArkTS1.2对js对象进行一元运算

**规则：** arkts-interop-js2s-unary-op

**级别：** error

**ArkTS1.1**
```typescript
// file1.js
export let foo = { num: 0 };
// file2.ets
import { foo } from './file1';
+foo.num;
-foo.num;
!foo.num;
~foo.num;
```

**ArkTS1.2**
```typescript
// file1.js
export let foo = { num: 0 };

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
let num = foo.getProperty('num');
// +foo.num
+num.toNumber();
// -foo.num
-num.toNumber();
// !foo.num
!num.toNumber();
// ~foo.num
~num.toNumber();
```
