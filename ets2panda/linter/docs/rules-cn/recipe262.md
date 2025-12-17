## `UIUtils.makeObserved`接口不支持监听自定义类

**规则：** `arkui-makeobserved-cannot-observe-custom-class`

**规则解释：**

在ArkTS-Sta中，`UIUtils.makeObserved`不支持监听开发者自定义的类，需要按照示例进行修改。

**变更原因：**

在ArkTS-Sta中，`UIUtils.makeObserved`不支持监听开发者自定义的类。

**适配建议：**

按照示例进行修改。

### `makeObserved`和V1装饰器配合使用

如果`UIUtils.makeObserved`的入参是自定义类的对象，需为该类添加`@Observed`装饰器，并使用`@State`装饰监听的变量。

**示例：**

ArkTS-Dyn

```typescript
import { UIUtils } from '@kit.ArkUI';

class Info {
  id: number = 0;
  constructor(id: number) {
    this.id = id;
  }
}

@Entry
@Component
struct Index {
  message: Info = UIUtils.makeObserved(new Info(10));

  build() {
    Column() {
      Text(`${this.message.id}`)
      Button('change id').onClick((e: ClickEvent) => {
        this.message.id ++;
      })
    }
  }
}
```

ArkTS-Sta

```typescript
'use static'
import {
  Observed,
  Entry,
  Component,
  State,
  Column,
  Text,
  Button,
  ClickEvent,
} from '@kit.ArkUI';

@Observed
class Info {
  id: number = 0;
  constructor(id: number) {
    this.id = id;
  }
}

@Entry
@Component
struct Index {
  @State message: Info = new Info(10);

  build() {
    Column() {
      Text(`${this.message.id}`)
      Button('change id').onClick((e: ClickEvent) => {
        this.message.id ++;
      })
    }
  }
}
```

### `makeObserved`和V2装饰器配合使用

如果`UIUtils.makeObserved`的入参是自定义类的对象，需为该类添加`@ObservedV2`装饰器，为该类的属性添加`@Trace`装饰器，并使用`@Local`装饰监听的变量。

**示例：**

ArkTS-Dyn

```TypeScript
import { UIUtils } from '@kit.ArkUI';

class Info {
  id: number = 0;
  constructor(id: number) {
    this.id = id;
  }
}

@Entry
@ComponentV2
struct Index {
  message: Info = UIUtils.makeObserved(new Info(10));

  build() {
    Column() {
      Text(`${this.message.id}`)
      Button('change id').onClick((e: ClickEvent) => {
        this.message.id ++;
      })
    }
  }
}
```

ArkTS-Sta

```TypeScript
'use static'
import {
  ObservedV2,
  Trace,
  Entry,
  ComponentV2,
  Local,
  Column,
  Text,
  Button,
  ClickEvent,
} from '@kit.ArkUI';

@ObservedV2
class Info {
  @Trace id: number = 0;
  constructor(id: number) {
    this.id = id;
  }
}

@Entry
@ComponentV2
struct Index {
  @Local message: Info = new Info(10);

  build() {
    Column() {
      Text(`${this.message.id}`)
      Button('change id').onClick((e: ClickEvent) => {
        this.message.id ++;
      })
    }
  }
}
```