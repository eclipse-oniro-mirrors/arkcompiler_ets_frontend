## 不支持globalThis

**规则：** `arkts-no-globalthis`

**规则解释：**

ArkTS1.2不支持globalThis。

**变更原因：**
 
ArkTS1.2不支持动态更改对象布局，因此不支持全局作用域和globalThis。

**适配建议：**

按示例修改。

**示例：**

**ArkTS1.1**

```typescript
// 全局文件中
var abc = 100;

// 从上面引用'abc'
let x = globalThis.abc;
```

**ArkTS1.2**

```typescript
// file1
export let abc: number = 100;

// file2
import * as M from 'file1'

let x = M.abc;
```