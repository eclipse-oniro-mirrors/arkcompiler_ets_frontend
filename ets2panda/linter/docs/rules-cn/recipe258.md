## 数据监听需要增加`@Observed`装饰器

**规则：** `arkui-data-observation`

**规则解释：**

在ArkTS-Sta中，被特定装饰器装饰的状态变量，如果要实现数据监听，需要为状态变量所属的类添加`@Observed`装饰器。

装饰器范围如下：

- `@State`
- `@Prop`
- `@Link`
- `@Provide`
- `@Consume`
- `@LocalStorageProp`
- `@LocalStorageLink`
- `@StorageProp`
- `@StorageLink`

**变更原因：**

受ArkTS-Sta静态类型系统的影响，如果要实现数据监听，需要为状态变量所属的类添加`@Observed`装饰器。

**适配建议：**

为状态变量所属的类添加`@Observed`装饰器。

**示例：**

ArkTS-Dyn

```typescript
class Num {
  count: number = 1;
}

@Entry
@Component
struct Index {
  @State num: Num = new Num();

  build() {
    Column() {
      Text(`${this.num.count}`)
      Button('count++')
        .onClick((e: ClickEvent) => {
          this.num.count ++;
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
class Num {
  count: number = 1;
}

@Entry
@Component
struct Index {
  @State num: Num = new Num();

  build() {
    Column() {
      Text(`${this.num.count}`)
      Button('count++')
        .onClick((e: ClickEvent) => {
          this.num.count ++;
        })
    }
  }
}
```