## taskpool、process、ArkTSUtils、AsyncLock、utils不需要import

**规则：** `arkts-limited-stdlib-no-import-concurrency`

**规则解释：**

taskpool、process、ArkTSUtils、AsyncLock和utils已基于ArkTS-Sta内置模块实现，不依赖于其他模块，不再需要import，且AsyncLock不需要添加`ArkTSUtils.locks.`前缀。

**变更原因：**

ArkTS-Sta为taskpool、process、ArkTSUtils、AsyncLock和utils提供原生支持，无需import即可直接使用，且AsyncLock无需`ArkTSUtils.locks.`前缀。

**适配建议：**

删除taskpool、process、ArkTSUtils和utils前的import，删除AsyncLock的`ArkTSUtils.locks.`前缀，改为直接使用，并且process更名为StdProcess。

**示例：**

ArkTS-Dyn

```typescript
import { taskpool, process, ArkTSUtils } from '@kit.ArkTS';
import utils from '@arkts.utils';

@Concurrent
function test() {}

taskpool.execute(test);
let result = process.is64Bit();
let lock: ArkTSUtils.locks.AsyncLock = new ArkTSUtils.locks.AsyncLock();
let lockInfo: utils.locks.AsyncLockInfo = {
  name: "myLock",
  mode: 1,
  contextId: 0
};
```

ArkTS-Sta

```typescript
function test() {}

taskpool.execute(test);
let result = StdProcess.is64Bit();
let lock: AsyncLock = new AsyncLock();
let lockInfo: AsyncLockInfo = new AsyncLockInfo();
```