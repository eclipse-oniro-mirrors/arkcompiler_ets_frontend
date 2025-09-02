### ArkTS1.2创建ArkTS1.1的没有无参构造函数的类的对象字面量

**规则：** arkts-interop-d2s-object-literal-no-args-constructor

**级别：** error

**ArkTS1.1**
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

**ArkTS1.2**
```typescript
// file1.ets  ArkTS1.1
export class X {
  name: string;
  constructor(arg: string) {
    this.name = arg;
  }
}
// file2.ets  ArkTS1.2
'use static'
import { X } from './file1';
let x: X = new X('hello') // 编译报错
```
