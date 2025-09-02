## Task的function属性改名为taskFunction

**规则：** arkts-change-taskpool-Task-to-taskFunction

**级别：** error

function在ArkTS1.2中为关键字，不能作为类属性（限定关键字(arkts-invalid-identifier)）。

**ArkTS1.1**
```typescript
import { taskpool } from '@kit.ArkTS';

function testString(str: string) {
  console.info(str);

}
let task: taskpool.Task = new taskpool.Task(testString, "hello");
let func = task.function;
```

**ArkTS1.2**
```typescript
let func1 = (str: string): string => {
    return str;
};
let task: taskpool.Task = new taskpool.Task(func1, "hello");
let func = task.taskFunction;
```
