### ArkTS-Sta的Object内置方法作用在ArkTS-Dyn对象

**规则：** `arkts-interop-d2s-static-object-on-dynamic-instance`

**规则解释：**

ArkTS-Sta的Object内置方法作用在ArkTS-Dyn对象时参数类型不匹配。

**变更原因：**

Object的接口参数类型为静态Object。ArkTS-Dyn对象在ArkTS-Sta中不是静态Object实例，因此参数类型不匹配。

**适配建议：**

使用动态Object的接口。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.ets
export class X {
  a = 1;
}

// file2.ets
import { X } from 'file1';
export function foo(prx: Object) {
  Object.entries(prx); // [a, 1]
  Object.keys(prx); // ["a"]
  Object.values(prx); // [1]
}
foo(new X());
```

**ArkTS-Sta**
```typescript
// file1.ets  ArkTS-Dyn
export class X {
  a = 1;
}

// file2.ets  ArkTS-Sta
'use static'
import { X } from 'file1';
export function foo(prx: Object) {
  Object.entries(prx); // [a, 1]
  Object.keys(prx); // ["a"]
  Object.values(prx); // [1]
}
foo(new X()); // 编译报错
```