## 数据监听需要增加`@Observed`装饰器

**规则：** `arkui-data-observation`

在ArkTS1.2中，被特定装饰器装饰的状态变量，如果要实现数据监听，需要为状态变量所属的类添加`@Observed`装饰器。

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

**ArkTS1.1**

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

**ArkTS1.2**

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