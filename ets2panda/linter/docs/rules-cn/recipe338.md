### ArkTS-Sta对JS数据进行相等判断

**规则：** `arkts-interop-js2s-equality-judgment`

**规则解释：**

ArkTS-Sta不支持直接对JS数据进行相等判断。

**变更原因：**

ArkTS-Sta中只能和有类型声明的文件进行交互。
ArkTS-Sta中限制ESValue的动态行为，形成动静态更清晰的界限，减少开发者滥用ESValue导致性能劣化的场景。

**适配建议：**

使用ESValue的接口进行判断。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.js
class A {}
export let a = new A();
export let b = new A();

// file2.ets
import { a, b } from './file1';
a == b;
a != b;
a === b;
a !== b;
```

**ArkTS-Sta**
```typescript
// file1.js
class A {}
export let a = new A();
export let b = new A();

// file2.ets  // ArkTS-Sta
'use static'
let mod = ESValue.load('./file1');
let a = mod.getProperty('a');
let b = mod.getProperty('b');

a.isEqualTo(b);
!a.isEqualTo(b);
a.isStrictlyEqualTo(b);
!a.isStrictlyEqualTo(b);
```