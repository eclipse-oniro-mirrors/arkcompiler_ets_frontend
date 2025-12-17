### ArkTS-Sta创建ArkTS-Dyn具有二义性的对象字面量

**规则：** `arkts-interop-d2s-object-literal-no-ambiguity`

**规则解释：**

ArkTS-Sta不支持创建ArkTS-Dyn具有二义性的对象字面量。

**变更原因：**

由于ArkTS-Sta的语法限制，当一个对象的类型被声明为联合类型，而右侧实际赋值的是一个类的实例时，会引发类型系统的二义性（对象可以是联合类型的任一类型，但实际运行时明确是一个类的实例，这种差异会导致类型检查或运行时的不确定性）。

**适配建议：**

使用as确定类型。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.ets
export class X {
  name: string = '';
}
export interface Y {
  name: string;
  age?: number;
}

// file2.ets
import { X, Y } from './file1';
let x: X | Y = { name: 'hello' };
```

**ArkTS-Sta**
```typescript
// file1.ets  ArkTS-Dyn
export class X {
  name: string = '';
}
export interface Y {
  name: string;
  age?: number;
}

// file2.ets  ArkTS-Sta
'use static'
import { X, Y } from './file1';
let x1: X | Y = { name: 'hello' };   // 编译报错
let x2: X | Y = new X('hello') as X; // OK
```