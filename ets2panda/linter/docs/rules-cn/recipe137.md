## 不支持globalThis

**规则：** `arkts-no-globalthis`

**规则解释：**

ArkTS-Sta不支持globalThis。

**变更原因：**
 
ArkTS-Sta不支持动态更改对象布局，因此不支持全局作用域和globalThis。

**适配建议：**

按示例修改。

**示例：**

ArkTS-Dyn

```typescript
// globalThis里设置abc
globalThis.abc = 123;

// 从globalThis引用'abc'
const x: number = globalThis.abc;
```

ArkTS-Sta

```typescript
// file1
export let abc: number = 100;

// file2
import * as M from 'file1'

let x = M.abc;
```