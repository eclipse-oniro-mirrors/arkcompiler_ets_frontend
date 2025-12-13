### ArkTS1.2Object内置方法作用在ArkTS1.1对象

**规则：** `arkts-interop-d2s-static-object-on-dynamic-instance`

Object的接口参数类型为静态Object。ArkTS1.1对象在ArkTS1.2中不是静态Object实例，因此参数类型不匹配。

**ArkTS1.1**
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

**ArkTS1.2**
```typescript
// file1.ets  ArkTS1.1
export class X {
  a = 1;
}

// file2.ets  ArkTS1.2
'use static'
import { X } from 'file1';
export function foo(prx: Object) {
  Object.entries(prx); // [a, 1]
  Object.keys(prx); // ["a"]
  Object.values(prx); // [1]
}
foo(new X()); // 编译报错
```