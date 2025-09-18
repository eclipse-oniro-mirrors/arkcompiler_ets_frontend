## 原生容器默认共享，不需要Sendable容器

**规则：** `arkts-no-need-stdlib-sendable-containers`

新增对象天然共享特性，不再依赖Sendable特性。可直接使用ArkTS1.2原生容器，删除collections.前缀。

**ArkTS1.1**
```typescript
import { collections } from '@kit.ArkTS';

let array = new collections.Array<number>();
```

**ArkTS1.2**
```typescript
let array = new Array<number>();
```