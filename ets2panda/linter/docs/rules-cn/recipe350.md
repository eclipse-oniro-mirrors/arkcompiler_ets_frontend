## 移除taskpool setCloneList接口

**规则：** arkts-limited-stdlib-no-setCloneList

**级别：** error

内存默认共享，不需要提供setCloneList来拷贝传递对象。

**ArkTS1.1**
```typescript
import { taskpool } from '@kit.ArkTS';

@Sendable
class BaseClass {
  public str: string = 'sendable: BaseClass';
}

@Concurrent
function testFunc(array: Array<BaseClass>) {
  let baseInstance = array[0];
  console.info('sendable: str1 is: ' + baseInstance.str);
  return baseInstance.str;
}

let baseInstance: BaseClass = new BaseClass();
let array = new Array<BaseClass>();
array.push(baseInstance);
let task = new taskpool.Task(testFunc, array);
task.setCloneList(array);
taskpool.execute(task).then((res: Object):void => {
  console.info('sendable: task res is: ' + res)
});
```

**ArkTS1.2**
```typescript
class BaseClass {
  public str: string = 'BaseClass';
}

function testFunc(array: Array<BaseClass>) {
  let baseInstance = array[0];
  console.info('str1 is: ' + baseInstance.str);
  return baseInstance.str;
}


let baseInstance: BaseClass = new BaseClass();
let array = new Array<BaseClass>();
array.push(baseInstance);
let task = new taskpool.Task(testFunc, array);
taskpool.execute(task).then((res: Any):void => {
  console.info('task res is: ' + res)
});
```
