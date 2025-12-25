### ArkTS-Sta继承JS的类

**规则：** `arkts-interop-js2s-inherit-js-class`

**规则解释：**

ArkTS-Sta不能直接继承JS的类。

**变更原因：**

ArkTS-Sta中只能和有类型声明的文件进行交互。
ArkTS-Sta中限制ESValue的动态行为，形成动静态更清晰的界限，减少开发者滥用ESValue导致性能劣化的场景。

**适配建议：**

使用ESValue的接口构造JS类并传递JS父类。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.js
export class A {}

// file2.ets
import { A } from './file1';
class B extends A {}
let b = new B();
```

**ArkTS-Sta**
```typescript
// file1.js
export class A {}

// file2.ets  // ArkTS-Sta
'use static'
let mod = ESValue.load('./file1');
let A = mod.getProperty('A');
let fixArr: FixedArray<ESValue> = [];
let esvalueCB = (argThis: ESValue, argNewTgt: ESValue, args: FixedArray<ESValue>, data?: ESValueCallbackData) => {
  return ESValue.Undefined;
};
let B: ESValue = ESValue.defineClass('B', esvalueCB, undefined, undefined, A);
let b = B.instantiate();
```