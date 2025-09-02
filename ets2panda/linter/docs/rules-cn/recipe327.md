### ArkTS1.2创建ArkTS1.1具有二义性的对象字面量

**规则：** arkts-interop-d2s-object-literal-no-ambiguity

**级别：** error

当一个对象的类型被声明为联合类型，而右侧实际赋值的是一个类的实例时，会引发类型系统的二义性（对象可以是联合类型的任一类型，但实际运行时明确是一个类的实例，这种差异会导致类型检查或运行时的不确定性）。

**ArkTS1.1**
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

**ArkTS1.2**
```typescript
// file1.ets  // 1.0
export class X {
  name: string = '';
}
export interface Y {
  name: string;
  age?: number;
}

// file2.ets  // 1.2
'use static'
import { X, Y } from './file1';
let x: X | Y = { name: 'hello' }; //编译报错
let x: X | Y = new X('hello'); // OK
```
