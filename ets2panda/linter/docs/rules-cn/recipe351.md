## 移除taskpool setTransferList接口

**规则：** arkts-limited-stdlib-no-setTransferList

**级别：** error

内存默认共享，不需要提供setTransferList来跨线程传递ArrayBuffer对象。

**ArkTS1.1**
```typescript
import { taskpool } from '@kit.ArkTS';

@Concurrent
function testTransfer(arg1: ArrayBuffer, arg2: ArrayBuffer): number {
  console.info('testTransfer arg1 byteLength: ' + arg1.byteLength);
  console.info('testTransfer arg2 byteLength: ' + arg2.byteLength);
  return 100;
}

let buffer: ArrayBuffer = new ArrayBuffer(8);
let view: Uint8Array = new Uint8Array(buffer);
let buffer1: ArrayBuffer = new ArrayBuffer(16);
let view1: Uint8Array = new Uint8Array(buffer1);

console.info('testTransfer view byteLength: ' + view.byteLength);
console.info('testTransfer view1 byteLength: ' + view1.byteLength);
// 执行结果为：
// testTransfer view byteLength: 8
// testTransfer view1 byteLength: 16

let task: taskpool.Task = new taskpool.Task(testTransfer, view, view1);
task.setTransferList([view.buffer, view1.buffer]);
taskpool.execute(task).then((res: Object) => {
  console.info('test result: ' + res);
}).catch((e: string) => {
  console.error('test catch: ' + e);
})
console.info('testTransfer view2 byteLength: ' + view.byteLength);
console.info('testTransfer view3 byteLength: ' + view1.byteLength);
// 经过transfer转移之后值为0，执行结果为：
// testTransfer view2 byteLength: 0
// testTransfer view3 byteLength: 0
```

**ArkTS1.2**
```typescript
function testTransfer(arg1: Uint8Array, arg2: Uint8Array): number {
  console.info('testTransfer arg1 byteLength: ' + arg1.byteLength);
  console.info('testTransfer arg2 byteLength: ' + arg2.byteLength);
  return 100.0;
}

let buffer: ArrayBuffer = new ArrayBuffer(8);
let view: Uint8Array = new Uint8Array(buffer);
let buffer1: ArrayBuffer = new ArrayBuffer(16);
let view1: Uint8Array = new Uint8Array(buffer1);

let task: taskpool.Task = new taskpool.Task(testTransfer, view, view1);
taskpool.execute(task).then((res: Any):void => {
  console.info('test result: ' + res);
}).catch((e: Error): void => {
  console.error('test catch: ' + e);
})
// 内存共享，此处可直接访问view,view1的内容，不需要使用setTransferList
// 执行结果为：
// testTransfer arg1 byteLength: 8
// testTransfer arg2 byteLength: 16
```
