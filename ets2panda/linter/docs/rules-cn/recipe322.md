## 不提供isConcurrent接口

**规则：** `arkts-limited-stdlib-no-support-isConcurrent`

**规则解释：**

ArkTS-Sta新增对象天然共享特性，所有函数都是共享的，不需要提供isConcurrent。

**变更原因：**

ArkTS-Sta新增对象天然共享特性，所有函数默认跨线程安全共享，无需再通过isConcurrent接口判断并发性。

**适配建议：**

删除共享函数中isConcurrent调用点。

**示例：**

ArkTS-Dyn

```typescript
import { taskpool } from '@kit.ArkTS';
@Concurrent
function test() {}
let result: Boolean = taskpool.isConcurrent(test);
```

ArkTS-Sta

NA