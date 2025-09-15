## 不支持Worker

**规则：** `arkts-no-need-stdlib-worker`

内存天然共享，不需要基于Actor模型实现ThreadWorker。使用ArkTS1.2提供的新线程api-EAWorker。

**ArkTS1.1**
```typescript
import { worker } from '@kit.ArkTS';

const workerInstance: worker.ThreadWorker = new worker.ThreadWorker('entry/ets/workers/Worker.ets')
```

**ArkTS1.2**
```typescript
let eaw = new EAWorker();
eaw.run<void>(():void => {
    console.info('hello, eaworker!');
});

eaw.join();
```