### ArkTS1.2实例化js对象

**规则：** arkts-interop-js2s-create-js-instance

**级别：** error

**ArkTS1.1**
```typescript
// file1.js
class foo {
  constructor(a) {}
}
// file2.ets
import { foo } from './file1';
new foo(123);
```

**ArkTS1.2**
```typescript
// file1.js
class foo {
  constructor(a) {}
}

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
foo.instantiate(ESValue.wrap(123));
```
