### ArkTS1.2对js数据进行相等判断

**规则：** `arkts-interop-js2s-equality-judgment`

ArkTS1.2对js数据进行相等判断时，使用ESValue接口判断。

**ArkTS1.1**
```typescript
// file1.js
class A {}
export let a = new A();
export let b = new A();

// file2.ets
import { a, b } from './file1';
a == b;
a != b;
a === b;
a !== b;
```

**ArkTS1.2**
```typescript
// file1.js
class A {}
export let a = new A();
export let b = new A();

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1');
let a = mod.getProperty('a');
let b = mod.getProperty('b');

a.isEqualTo(b);
!a.isEqualTo(b);
a.isStrictlyEqualTo(b);
!a.isStrictlyEqualTo(b);
```