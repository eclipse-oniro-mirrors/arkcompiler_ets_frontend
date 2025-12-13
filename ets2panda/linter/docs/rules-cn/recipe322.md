## 不提供isConcurrent接口

**规则：** `arkts-limited-stdlib-no-support-isConcurrent`

新增对象天然共享特性，所有函数都是共享的，不需要提供isConcurrent。

**ArkTS1.1**
```typescript
import { taskpool } from '@kit.ArkTS';
@Concurrent
function test() {}
let result: Boolean = taskpool.isConcurrent(test);
```

**ArkTS1.2**
不支持isConcurrent接口。