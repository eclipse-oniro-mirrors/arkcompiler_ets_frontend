## 共享模块不需要use shared修饰

**规则：** arkts-limited-stdlib-no-use-shared

**级别：** error

新增对象天然共享特性，无需添加use shared。

**ArkTS1.1**
```typescript
// test.ets
export let num = 1;
// shared.ets
'use shared'
export {num} from './test';
```

**ArkTS1.2**
```typescript
export let num = 1;
```
