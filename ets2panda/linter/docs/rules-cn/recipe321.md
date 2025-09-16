## taskpool不需要import

**规则：** arkts-limited-stdlib-no-import-concurrency

**级别：** error

taskpool实现基于ArkTS提供，不依赖其他模块，不再需要import。

**ArkTS1.1**
```typescript
import { taskpool } from '@kit.ArkTS';

@Concurrent
function test() {}

taskpool.execute(test);
```

**ArkTS1.2**
```typescript
function test() {}

taskpool.execute(test);
```
