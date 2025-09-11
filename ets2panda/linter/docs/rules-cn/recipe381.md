## 传递给`stateStyles`的代码块必须是箭头函数

**规则：** `arkui-statestyles-block-need-arrow-func`

在ArkTS1.2中，传递给`stateStyles`的匿名代码块必须是箭头函数。

**ArkTS1.1**

```typescript
@Entry
@Component
struct Index {
  build() {
    Column() {
      Button('Button')
        .stateStyles({
          normal: {
            .backgroundColor(Color.Red)
            .borderWidth(8)
          },
          pressed: {
            .backgroundColor(Color.Green)
          }
        })
    }
  }
}
```

**ArkTS1.2**

```typescript
'use static'
import {
  Entry,
  Component,
  Column,
  CommonMethod,
  Button,
  Color,
} from '@kit.ArkUI';

@Entry
@Component
struct Index {
  build() {
    Column() {
      Button('Button')
        .stateStyles({
          normal: (instance: CommonMethod): void => {
            instance.backgroundColor(Color.Red);
            instance.borderWidth(8.0);
          },
          pressed: (instance: CommonMethod): void => {
            instance.backgroundColor(Color.Green);
          }
        })
    }
  }
}
```