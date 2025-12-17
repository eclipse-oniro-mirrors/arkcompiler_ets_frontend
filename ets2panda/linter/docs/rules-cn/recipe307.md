### ArkTS-Sta处理TS非常规异常

**规则：** `arkts-interop-ts2s-ts-exception`

**规则解释：**

ArkTS-Sta不支持直接处理TS的非常规异常。

**变更原因：**

ArkTS-Sta中throw和catch的对象只能是Error的实例，针对非常规的TS异常对象，交互时会被包装到ESError中。

**适配建议：**

通过getValue()方法获取包装了原始异常对象的ESValue实例后再进行处理。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.ts
export function foo() {
  throw 123;
}

// file2.ets
import { foo } from './file1';

try {
  foo();
} catch (e) {
  console.log("result is " + (e as number)); // 123
}
```

**ArkTS-Sta**
```typescript
// file1.ts
export function foo() {
  throw 123;
}

// file2.ets  // ArkTS-Sta
'use static'
import { foo } from './file1';

try {
  foo();
} catch (e) {
  let err: ESValue = (e as ESError).getValue();
  err.toNumber(); // 123
}
```