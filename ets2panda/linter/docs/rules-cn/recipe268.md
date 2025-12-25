### ArkTS-Sta对JS对象进行条件判断

**规则：** `arkts-interop-js2s-condition-judgment`

**规则解释：**

ArkTS-Sta不支持直接对JS对象进行条件判断。

**变更原因：**

ArkTS-Sta中只能和有类型声明的文件进行交互。
ArkTS-Sta中限制ESValue的动态行为，形成动静态更清晰的界限，减少开发者滥用ESValue导致性能劣化的场景。

**适配建议：**

使用ESValue的接口转换为boolean类型后再判断。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.js
export let foo = { isGood: true };

// file2.ets
import { foo } from './file1';

if (foo.isGood) {}
```

**ArkTS-Sta**
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