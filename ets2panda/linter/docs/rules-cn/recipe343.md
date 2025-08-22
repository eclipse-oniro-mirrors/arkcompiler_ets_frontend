### ArkTS1.2判断js对象类型

**规则：** `arkts-interop-js2s-instanceof-js-type`

ArkTS1.2使用ESValue接口判断js对象类型。

**ArkTS1.1**
```typescript
// file1.js
export class Foo {}
export let foo = new Foo();

// file2.ets
import { Foo, foo } from './file1';
foo instanceof Foo;
```

**ArkTS1.2**
```typescript
// file1.js
export class Foo {}
export let foo = new Foo();

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1');
let Foo = mod.getProperty('Foo');
let foo = mod.getProperty('foo');

foo.isInstanceOf(Foo);
```