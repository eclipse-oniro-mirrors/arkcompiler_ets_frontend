### ArkTS-Sta创建ArkTS-Dyn的没有无参构造函数的类的对象字面量

**规则：** `arkts-interop-d2s-object-literal-no-args-constructor`

**规则解释：**

ArkTS-Sta不支持创建ArkTS-Dyn中没有无参构造函数的类的对象字面量。

**变更原因：**

由于ArkTS-Sta的语法限制，当ArkTS-Sta创建ArkTS-Dyn的没有无参构造函数的类的对象字面量时，需要使用new关键字和构造函数。

**适配建议：**

使用new关键字进行创建。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.ets
export class X {
  name: string;
  constructor(arg: string) {
    this.name = arg;
  }
}
// file2.ets
import { X } from './file1';
let x = new X('hello');
```

**ArkTS-Sta**
```typescript
// file1.ets  ArkTS-Dyn
export class X {
  name: string;
  constructor(arg: string) {
    this.name = arg;
  }
}
// file2.ets  ArkTS-Sta
'use static'
import { X } from './file1';
let x1: X = {name: 'hello'}   // 编译报错
let x2: X = new X('hello')    // OK
```