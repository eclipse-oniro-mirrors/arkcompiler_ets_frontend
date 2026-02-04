## 原生容器默认共享，不需要Sendable容器

**规则：** `arkts-no-need-stdlib-sendable-containers`

**规则解释：**

ArkTS-Sta新增对象天然共享特性，不再依赖Sendable特性。可直接使用ArkTS-Sta原生容器进行跨线程访问（非线程安全）。

**变更原因：**

ArkTS-Sta新增对象天然共享特性，原生容器默认支持跨线程访问（非线程安全），无需再依赖Sendable容器，无需使用collections.前缀（BitVector除外）。

**适配建议：**

使用ArkTS-Sta原生容器，删除collections.前缀。

**示例：**

ArkTS-Dyn

```typescript
import { collections } from '@kit.ArkTS';
import { collections as col} from '@kit.ArkTS';

let array = new collections.Array<number>();
let bitVector = new col.BitVector<number>();
```

ArkTS-Sta

```typescript
import { collections as col} from '@kit.ArkTS';

let array = new Array<number>();
let bitVector = new col.BitVector<number>();
```