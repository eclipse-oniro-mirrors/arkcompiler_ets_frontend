### ArkTS-Sta访问JS索引

**规则：** `arkts-interop-js2s-access-js-index`

**规则解释：**

ArkTS-Sta不支持直接访问JS索引。

**变更原因：**

ArkTS-Sta中只能和有类型声明的文件进行交互。
ArkTS-Sta中限制ESValue的动态行为，形成动静态更清晰的界限，减少开发者滥用ESValue导致性能劣化的场景。

**适配建议：**

使用ESValue接口访问索引，接口接收参数为ESValue类型，传参时需要用wrap接口构造ESValue实例再传参。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.js
export let foo = { arr: [1, 2, 3] };
// file2.ets
import { foo } from './file1';
let arr = foo.arr;
arr[1];
arr[3] = 4;
```

**ArkTS-Sta**
```typescript
// file1.js
export let foo = [1, 2, 3];

// file2.ets  // ArkTS-Sta
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
let arr = foo.getProperty('arr');
arr.getProperty(1);
arr.setProperty(3, ESValue.wrap(4));
```