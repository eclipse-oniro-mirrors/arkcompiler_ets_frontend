### ArkTS1.2对js对象进行二元运算

**规则：** arkts-interop-js2s-binary-op

**级别：** error

**ArkTS1.1**
```typescript
// file1.js
export let foo = { a: 1, b: 2 };

// file2.ets
import { foo } from './file1';
let a = foo.a;
let b = foo.b;
a + b;
a - b;
a * b;
a / b;
a % b;
a ** b;
```

**ArkTS1.2**
```typescript
// file1.js
export let foo = { a: 1, b: 2 };

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
let a = foo.getProperty('a').toNumber();
let b = foo.getProperty('b').toNumber();
a + b;
a - b;
a * b;
a / b;
a % b;
a ** b;
```
