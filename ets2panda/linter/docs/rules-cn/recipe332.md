### ArkTS1.2访问js属性

**规则：** arkts-interop-js2s-access-js-prop

**级别：** error

**ArkTS1.1**
```typescript
// file1.js
export let foo = { name: '123' };
// file2.ets
import { foo } from './file1';
foo.name;
foo.name = '456';
```

**ArkTS1.2**
```typescript
// file1.js
export let foo = {name: "123"}

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1')
let foo = mod.getProperty('foo')
foo.getProperty('name')
foo.setProperty('name', ESValue.wrap("456")）
```
