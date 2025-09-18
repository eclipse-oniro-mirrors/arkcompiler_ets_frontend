## 不支持动态import

**规则：** `arkts-no-dynamic-import`

**规则解释：**

在ArkTS1.2中，不支持动态import。

**变更原因：**
 
ArkTS1.2中模块加载默认支持懒加载，无需动态import。

**适配建议：**

将动态import改为静态import。

**示例：**

**ArkTS1.1**

```typescript
// file1.ets
export const a = 'file1';
// file2.ets
import('./file1').then((m) => { // 在ArkTS1.2中动态import是不支持的
  console.log('success');
})
async () => {
  const module = await import('./file1'); // 在ArkTS1.2中动态import是不支持的
}
```

**ArkTS1.2**

```typescript
// file1.ets
export const a = 'file1';
// file2.ets
import {a} from './file1'  // 支持静态import
```