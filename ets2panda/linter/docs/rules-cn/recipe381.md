## 传递给`stateStyles`的代码块必须是箭头函数

**规则：** `arkui-statestyles-block-need-arrow-func`

**规则解释：**

在ArkTS-Sta中，传递给`stateStyles`的匿名代码块必须是箭头函数。

**变更原因：**

在ArkTS-Dyn中，传递给`stateStyles`的匿名代码块不符合标准语法。

**适配建议：**

把传递给给`stateStyles`的匿名代码块改为箭头函数。

**示例：**

ArkTS-Dyn

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

ArkTS-Sta

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