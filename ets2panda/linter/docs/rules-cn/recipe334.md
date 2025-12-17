### ArkTS-Sta获取JS对象类型

**规则：** `arkts-interop-js2s-typeof-js-type`

**规则解释：**

ArkTS-Sta不支持直接获取JS对象类型。

**变更原因：**

ArkTS-Sta中只能和有类型声明的文件进行交互。
ArkTS-Sta中限制ESValue的动态行为，形成动静态更清晰的界限，减少开发者滥用ESValue导致性能劣化的场景。

**适配建议：**

使用ESValue的接口获取类型。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.js
export let foo = { num: 123 };

// file2.ets
import { foo } from './file1';
typeof foo.num; // 'number'
```

**ArkTS-Sta**
```typescript
// file1.js
export let foo = 123;

// file2.ets  // ArkTS-Sta
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
let num = foo.getProperty('num');

num.typeOf(); // 'number'
```