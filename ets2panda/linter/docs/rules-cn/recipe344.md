### ArkTS1.2对js对象自增自减

**规则：** `arkts-interop-js2s-self-addtion-reduction`

ArkTS1.2对js对象自增自减时，使用ESValue接口转换为数字后再操作。

**ArkTS1.1**
```typescript
// file1.js
export let foo = { num: 0 };

// file2.ets
import { foo } from './file1';
let a: number = 0;
a = foo.num++;
a = ++foo.num;
a = foo.num--;
a = --foo.num;
```

**ArkTS1.2**
```typescript
// file1.js
export let foo = { num: 0 };

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
let a: number = 0;

// a = foo.num++
let num = foo.getProperty('num');
let tmp: number = num.toNumber();
a = tmp;
foo.setProperty('num', ESValue(tmp + 1));

// a = ++foo.num

num = foo.getProperty('num');
tmp = num.toNumber() + 1;
foo.setProperty('num', ESValue(tmp));
a = tmp;

// the cases "foo.num--" and "--foo.num" are similar
```