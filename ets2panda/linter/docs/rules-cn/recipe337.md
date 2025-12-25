### ArkTS-Sta对JS数据进行比较

**规则：** `arkts-interop-js2s-compare-js-data`

**规则解释：**

ArkTS-Sta不能直接对JS数据进行比较。

**变更原因：**

ArkTS-Sta中只能和有类型声明的文件进行交互。
ArkTS-Sta中限制ESValue的动态行为，形成动静态更清晰的界限，减少开发者滥用ESValue导致性能劣化的场景。

**适配建议：**

使用ESValue接口转换为数字再操作。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.js
export let foo = { a: 1, b: 2 };

// file2.ets
import { foo } from './file1';
let a = foo.a;
let b = foo.b;
a > b;
a < b;
a >= b;
a <= b;
```

**ArkTS-Sta**
```typescript
// file1.js
export let a = 1;
export let b = 2;

// file2.ets  // ArkTS-Sta
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
let a = foo.getProperty('a').toNumber();
let b = foo.getProperty('b').toNumber();

a > b;
a < b;
a >= b;
a <= b;
```