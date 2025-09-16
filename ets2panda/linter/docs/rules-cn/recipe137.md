## 不支持globalThis

**规则：**`arkts-no-globalthis`

**级别：error**

由于ArkTS1.2不支持动态更改对象的布局，因此不支持全局作用域和globalThis。

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
