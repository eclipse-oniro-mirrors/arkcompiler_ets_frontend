### ArkTS1.2遍历js对象

**规则：** `arkts-interop-js2s-traverse-js-instance`

ArkTS1.2遍历js对象时，使用ESValue接口访问索引和属性。

**ArkTS1.1**
```typescript
// file1.js
export let foo = { arr: [1, 2, 3] };
// file2.ets
import { foo } from './file1';
let arr = foo.arr;
let len = arr.length as number;
for (let i = 0; i < len; ++i) {
  arr[i] as number;
  arr[i] = 0;
}
```

**ArkTS1.2**
```typescript
// file1.js
export let foo = { arr: [1, 2, 3] };

// file2.ets  ArkTS1.2
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
let arr = foo.getProerpty('arr');
let len = arr.getProerpty('length').toNumber();
for (let i = 0; i < len; ++i) {
  arr.getProperty(i).toNumber();
  arr.setProperty(i, ESValue.wrap(0));
}
```