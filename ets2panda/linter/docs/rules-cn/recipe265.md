### ArkTS1.2继承js的类

**规则：** `arkts-interop-js2s-inherit-js-class`

ArkTS1.2继承js的类时，使用ESValue接口构造JS类并传递js父类。

**ArkTS1.1**
```typescript
// file1.js
export class A {}

// file2.ets
import { A } from './file1';
class B extends A {}
let b = new B();
```

**ArkTS1.2**
```typescript
// file1.js
export class A {}

// file2.ets  // ArkTS1.2
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