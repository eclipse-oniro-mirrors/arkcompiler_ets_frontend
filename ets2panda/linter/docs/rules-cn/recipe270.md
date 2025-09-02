### ArkTS1.2处理js非常规异常

**规则：** arkts-interop-js2s-js-exception

**级别：** error

**ArkTS1.1**
```typescript
// file1.js
export function foo() {
  throw 123;
}

// file2.ets
import { foo } from './file1';

try {
  foo();
} catch (e) {
  console.log("result is " + (e as number)); //123
}
```

**ArkTS1.2**
```typescript
// file1.js
export function foo() {
  throw 123;
}

// file2.ets
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');

try {
  foo.invoke();
} catch (e) {
  let err: ESValue = (e as ESError).getValue();
  err.toNumber(); // 123
}
```
