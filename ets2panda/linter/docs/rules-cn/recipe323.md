### ArkTS1.2导出js实体

**规则：** `arkts-interop-js2s-export-js`

ArkTS1.2不能以`export {A} from "./file1"`的形式直接在ets文件中导出js对象。

**ArkTS1.1**
```typescript
// file1.js
export function foo() {}
export class A {}

// file2.ets
import { foo } from './file1';
export { foo };

export { A } from './file1';

// 函数、类、变量、枚举
```

**ArkTS1.2**
```typescript
// file1.js
export function foo() {}
export class A {}

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
let A = mod.getProperty('A');

export { foo, A };
```