## `Repeat`禁用默认的懒加载

**规则：** `arkui-repeat-disable-default-virtualscroll`

在ArkTS1.1中，`Repate`默认渲染全部子组件。

在ArkTS1.2中，`Repeat`默认支持懒加载，如果想要渲染全部子组件，需要禁用默认的懒加载。

**ArkTS1.1**

```typescript
@Entry
@ComponentV2
struct Index {
  @Local dataArr: Array<string> = [];

  aboutToAppear(): void {
    for (let i = 0; i < 50; i++) {
      this.dataArr.push(`data_${i}`);
    }
  }

  build() {
    Column() {
      List() {
        Repeat<string>(this.dataArr)
          .each((ri: RepeatItem<string>) => {
            ListItem() {
              Text('each_' + ri.item).fontSize(30)
            }
          })
      }
      .cachedCount(2)
      .height('70%')
      .border({ width: 1 })
    }
  }
}
```

**ArkTS1.2**

```typescript
'use static'
import {
  Entry,
  ComponentV2,
  Local,
  Column,
  List,
  Repeat,
  RepeatItem,
  ListItem,
  Text,
} from '@kit.ArkUI';

@Entry
@ComponentV2
struct Index {
  @Local dataArr: Array<string> = [];

  aboutToAppear(): void {
    for (let i: number = 0.0; i < 50.0; i++) {
      this.dataArr.push(`data_${i}`);
    }
  }

  build() {
    Column() {
      List() {
        Repeat<string>(this.dataArr)
          .each((ri: RepeatItem<string>) => {
            ListItem() {
              Text('each_' + ri.item).fontSize(30.0)
            }
          })
          .virtualScroll({ disableVirtualScroll: true })
      }
      .cachedCount(2.0)
      .height('70%')
      .border({ width: 1.0 })
    }
  }
}
```