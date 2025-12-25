### ArkTS-Sta判断JS对象类型

**规则：** `arkts-interop-js2s-instanceof-js-type`

**规则解释：**

ArkTS-Sta不能直接判断JS对象类型。

**变更原因：**

ArkTS-Sta中只能和有类型声明的文件进行交互。
ArkTS-Sta中限制ESValue的动态行为，形成动静态更清晰的界限，减少开发者滥用ESValue导致性能劣化的场景。

**适配建议：**

使用ESValue的接口判断类型。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.js
export class Foo {}
export let foo = new Foo();

// file2.ets
import { Foo, foo } from './file1';
foo instanceof Foo;
```

**ArkTS-Sta**
```typescript
// file1.js
export class Foo {}
export let foo = new Foo();

// file2.ets  // ArkTS-Sta
'use static'
let mod = ESValue.load('./file1');
let Foo = mod.getProperty('Foo');
let foo = mod.getProperty('foo');

foo.isInstanceOf(Foo);
```