## 不支持Worker

**规则：** `arkts-no-need-stdlib-worker`

**规则解释：**

ArkTS-Sta内存天然共享，无需像传统的Actor模型一样通过消息传递来管理线程间的通信和状态隔离，不需要再基于Actor模型实现Worker。

**变更原因：**

ArkTS-Sta为内存天然共享模型，跨线程数据交互无需再依赖Actor模型的Worker机制。

**适配建议：**

使用ArkTS-Sta提供的新线程[API-EAWorker](../reference/native-lib/eaworker_managed.md)。

**示例：**

ArkTS-Dyn

```typescript
// Worker.ets
import { worker, MessageEvents, ThreadWorkerGlobalScope } from '@kit.ArkTS';

const workerPort: ThreadWorkerGlobalScope = worker.workerPort;

// 注册onmessage回调，当worker线程收到其宿主线程通过postMessage接口发送的消息时被调用
workerPort.onmessage = (e: MessageEvents) => {
  let data: string = e.data;
  console.info('workerPort onmessage is: ', data);
}

// 向宿主线程发送消息
workerPort.postMessage('hello hostThread!');
```

```typescript
// Index.ets
import { worker, MessageEvents } from '@kit.ArkTS';

let workerInstance = new worker.ThreadWorker('entry/ets/workers/Worker.ets');

// 注册onmessage回调，当宿主线程接收到其创建的worker通过workerPort.postMessage接口发送的消息时被调用
workerInstance.onmessage = (e: MessageEvents) => {
  let data: string = e.data;
  console.info('workerInstance onmessage is: ', data);
}
// 发送消息给worker线程
workerInstance.postMessage('hello worker!');

// 执行结果为：
// workerPort onmessage is: hello worker!
// workerInstance onmessage is: hello hostThread!
```

ArkTS-Sta

```typescript
let eaw = new EAWorker();
eaw.start();
eaw.run<void>(():void => {
    console.info('hello, eaworker!');
});

eaw.join();
```