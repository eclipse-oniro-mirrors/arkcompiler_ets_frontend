### ArkTS-Sta对JS对象自增自减

**规则：** `arkts-interop-js2s-self-addtion-reduction`

**规则解释：**

ArkTS-Sta不支持直接对JS对象自增自减。

**变更原因：**

ArkTS-Sta中只能和有类型声明的文件进行交互。
ArkTS-Sta中限制ESValue的动态行为，形成动静态更清晰的界限，减少开发者滥用ESValue导致性能劣化的场景。

**适配建议：**

使用ESValue的接口转换为数字后再操作。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.js
export let foo = { num: 0 };

// file2.ets
import { foo } from './file1';
let a: number = 0;
a = foo.num++;
a = ++foo.num;
a = foo.num--;
a = --foo.num;
```

**ArkTS-Sta**
```typescript
// file1.js
export let foo = { num: 0 };

// file2.ets  // ArkTS-Sta
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
let a: number = 0;

// a = foo.num++
let num = foo.getProperty('num');
let tmp: number = num.toNumber();
a = tmp;
foo.setProperty('num', ESValue(tmp + 1));

// a = ++foo.num

num = foo.getProperty('num');
tmp = num.toNumber() + 1;
foo.setProperty('num', ESValue(tmp));
a = tmp;

// the cases "foo.num--" and "--foo.num" are similar
```