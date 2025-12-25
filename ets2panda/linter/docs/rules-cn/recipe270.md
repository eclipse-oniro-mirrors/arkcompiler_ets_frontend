### ArkTS-Sta处理JS非常规异常

**规则：** `arkts-interop-js2s-js-exception`

**规则解释：**

ArkTS-Sta不支持直接处理JS的非常规异常。

**变更原因：**

ArkTS-Sta中throw和catch的对象只能是Error的实例，针对非常规的JS异常对象，交互时会被包装到ESError中。

**适配建议：**

通过getValue()方法获取包装了原始异常对象的ESValue实例后再进行处理。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.js
export function foo() {
  throw 123;
}

// file2.ets
import { foo } from './file1';

try {
  foo();
} catch (e) {
  console.log("result is " + (e as number)); //123
}
```

**ArkTS-Sta**
```typescript
// file1.js
export function foo() {
  throw 123;
}

// file2.ets
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');

try {
  foo.invoke();
} catch (e) {
  let err: ESValue = (e as ESError).getValue();
  err.toNumber(); // 123
}
```