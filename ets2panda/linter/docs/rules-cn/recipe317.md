## 共享模块不需要use shared修饰

**规则：** `arkts-limited-stdlib-no-use-shared`

**规则解释：**

ArkTS-Sta新增对象天然共享特性，无需添加use shared。

**变更原因：**

ArkTS-Sta新增对象天然共享特性，模块及对象默认支持跨线程共享，无需再使用use shared显示声明。

**适配建议：**

删除use shared显示声明。

**示例：**

ArkTS-Dyn

```typescript
// test.ets
export let num = 1;
// shared.ets
'use shared'
export {num} from './test';
```

ArkTS-Sta

```typescript
export let num = 1;
```