## 移除taskpool setCloneList接口

**规则：** `arkts-limited-stdlib-no-setCloneList`

内存默认共享，不需要提供setCloneList来拷贝传递对象。如果仍然需要以拷贝语义使用数组对象，可以调用Array.from()方法手动拷贝。

**ArkTS1.1**
```typescript
import { taskpool } from '@kit.ArkTS';

@Sendable
class TestClass {
  public str: string = 'sendable: TestClass';
}

@Concurrent
function testFunc(array: Array<TestClass>) {
  let testInstance = array[0];
  return testInstance.str;
}

let testInstance: TestClass = new TestClass();
let array = new Array<TestClass>();
array.push(testInstance);
let task = new taskpool.Task(testFunc, array);
task.setCloneList(array);
taskpool.execute(task).then((res: Object):void => {
  console.info('sendable: task res is: ' + res);
  console.info('sendable: array length: ' + array.length);
});
// 执行结果为
// sendable: task res is: sendable: TestClass
// sendable: array length: 1
```

**ArkTS1.2**
```typescript
class TestClass {
  public str: string = 'TestClass';
}

function testFunc(array: Array<TestClass>) {
  let testInstance = array[0];
  array.push(new TestClass());
  return testInstance.str;
}

let testInstance: TestClass = new TestClass();
let array1 = new Array<TestClass>();
array1.push(testInstance);
// 以拷贝语义传递数组，testFunc内的修改不会作用到array1对象上
let task1 = new taskpool.Task(testFunc, Array.from(array1));
taskpool.execute(task1).then((res: Any):void => {
  console.info('task1 res is: ' + res);
  console.info('array1 length: ' + array1.length);
});
// 使用引用传递数组，testFunc内的修改会作用到array2对象上
let array2 = new Array<TestClass>();
array2.push(testInstance);
let task2 = new taskpool.Task(testFunc, array2);
taskpool.execute(task2).then((res: Any):void => {
  console.info('task2 res is: ' + res);
  console.info('array2 length: ' + array2.length);
});
// 执行结果为：
// task1 res is: TestClass
// array1 length: 1
// task2 res is: TestClass
// array2 length: 2
```