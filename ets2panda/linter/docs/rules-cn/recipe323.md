### ArkTS-Sta导出JS实体

**规则：** `arkts-interop-js2s-export-js`

**规则解释：**

ArkTS-Sta不能以`export {A} from "./file1"`的形式直接在ets文件中导出JS对象。

**变更原因：**

ArkTS-Sta中只能和有类型声明的文件进行交互。
ArkTS-Sta中限制ESValue的动态行为，形成动静态更清晰的界限，减少开发者滥用ESValue导致性能劣化的场景。

**适配建议：**

使用ESValue的接口导入JS模块和调用接口。

**示例：**

**ArkTS-Dyn**
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

**ArkTS-Sta**
```typescript
// file1.js
export function foo() {}
export class A {}

// file2.ets  // ArkTS-Sta
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
let A = mod.getProperty('A');

export { foo, A };
```