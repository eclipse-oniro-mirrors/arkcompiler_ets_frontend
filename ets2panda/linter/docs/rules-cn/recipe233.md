## 不支持动态import

**规则：** `arkts-no-dynamic-import`

**规则解释：**

在ArkTS-Sta中，不支持动态import。

**变更原因：**
 
ArkTS-Sta中模块加载默认支持懒加载，无需动态import。

**适配建议：**

将动态import改为静态import。

**示例：**

ArkTS-Dyn

```typescript
// file1.ets
export const a = 'file1';

// file2.ets
import('./file1').then((m) => { // 在ArkTS-Sta中不支持动态import
  console.info('success');
})
async () => {
  const module = await import('./file1'); // 在ArkTS-Sta中不支持动态import
}
```

ArkTS-Sta

```typescript
// file1.ets
export const a = 'file1';

// file2.ets
import {a} from './file1'  // 支持静态import
```