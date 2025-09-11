### ArkTS1.2Reflect内置方法作用在ArkTS1.1对象

**规则：** `arkts-interop-d2s-static-reflect-on-dynamic-instance`

Reflect接口参数类型为静态Object。ArkTS1.1对象在ArkTS1.2中不是静态Object实例，因此参数类型不匹配。

**ArkTS1.1**
```typescript
// file1.ets  ArkTS1.1
class X {
  a: string = 'hello';
  getName() {
    return this.a;
  }
}

// file2.ets  ArkTS1.2
'use static'
import { X } from './file1';
export function foo(prx: Object) {
  Reflect.get(prx, 'a'); // 'hello'
  Reflect.set(prx, 'a', 'world'); // true
  Reflect.ownKeys(prx); // ['a']
}
foo(new X());
```

**ArkTS1.2**
```typescript
// file1.ets  ArkTS1.1
class X {
  a: string = 'hello';
  getName() {
    return this.a;
  }
}

// file2.ets  ArkTS1.2
'use static'
import { X } from './file1';
export function foo(prx: Object) {
  Reflect.get(prx, 'a'); // 'hello'
  Reflect.set(prx, 'a', 'world'); // true
  Reflect.ownKeys(prx); // ['a']
}
foo(new X()); // 编译报错
```