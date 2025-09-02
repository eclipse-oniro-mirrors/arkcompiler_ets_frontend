## 内存默认共享，不提供ASON

**规则：** arkts-no-need-stdlib-ason

**级别：** error

新增对象天然共享特性，不再依赖Sendable特性，ASON.stringify()方法调用可直接更改为JSON.stringify()，且删除ArkTSUtils.前缀。

**ArkTS1.1**
```typescript
import { collections } from '@kit.ArkTS';
import { ArkTSUtils } from '@kit.ArkTS';
let arr = new collections.Array(1, 2, 3);
let str = ArkTSUtils.ASON.stringify(arr);
console.info(str);
```

**ArkTS1.2**
```typescript
let arr = new Array<number>(1, 2, 3);
let str = JSON.stringify(arr);
console.info(str);
```
