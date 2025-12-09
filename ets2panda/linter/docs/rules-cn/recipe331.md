### ArkTS1.2调用js函数和传参

**规则：** `arkts-interop-js2s-call-js-func`

ArkTS1.2中使用ESValue接口调用js函数和传参。

**ArkTS1.1**
```typescript
// file1.js
export function foo() {}
export function bar(a) {}

// file2.ets
import { foo, bar } from './file1';
foo();
bar(123);
```

**ArkTS1.2**
```typescript
// file1.js
export function foo() {}
export function bar(a) {}

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
let bar = mod.getProperty('bar');
foo.invoke();
bar.invoke(ESValue.wrap(123));
```