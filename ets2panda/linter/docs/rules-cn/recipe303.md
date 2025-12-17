### ArkTS-Sta的Reflect内置方法作用在ArkTS-Dyn对象

**规则：** `arkts-interop-d2s-static-reflect-on-dynamic-instance`

**规则解释：**

ArkTS-Sta的Reflect内置方法作用在ArkTS-Dyn对象时参数类型不匹配。

**变更原因：**

Reflect接口参数类型为静态Object。ArkTS-Dyn对象在ArkTS-Sta中不是静态Object实例，因此参数类型不匹配。

**适配建议：**

使用动态Reflect接口。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.ets 
class X {
  a: string = 'hello';
  getName() {
    return this.a;
  }
}

// file2.ets 
import { X } from './file1';
export function foo(prx: Object) {
  Reflect.get(prx, 'a'); // 'hello'
  Reflect.set(prx, 'a', 'world'); // true
  Reflect.ownKeys(prx); // ['a']
}
foo(new X());
```

**ArkTS-Sta**
```typescript
// file1.ets  ArkTS-Dyn
class X {
  a: string = 'hello';
  getName() {
    return this.a;
  }
}

// file2.ets  ArkTS-Sta
'use static'
import { X } from './file1';
export function foo(prx: Object) {
  Reflect.get(prx, 'a'); // 编译报错
  Reflect.set(prx, 'a', 'world'); // 编译报错
  Reflect.ownKeys(prx); // 编译报错
}
foo(new X()); 
```