### ArkTS-Sta遍历JS对象

**规则：** `arkts-interop-js2s-traverse-js-instance`

**规则解释：**

ArkTS-Sta遍历JS对象时，不能直接访问索引和属性。

**变更原因：**

ArkTS-Sta中只能和有类型声明的文件进行交互。
ArkTS-Sta中限制ESValue的动态行为，形成动静态更清晰的界限，减少开发者滥用ESValue导致性能劣化的场景。

**适配建议：**

使用ESValue接口访问索引和属性。

**示例：**

**ArkTS-Dyn**
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

**ArkTS-Sta**
```typescript
// file1.js
export let foo = { arr: [1, 2, 3] };

// file2.ets  ArkTS-Sta
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