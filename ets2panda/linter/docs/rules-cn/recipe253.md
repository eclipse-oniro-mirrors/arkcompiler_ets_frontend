## 不支持`$value`形式的数据绑定

**规则：** `arkui-link-decorator-passing`

**规则解释：**

在ArkTS-Sta中，不支持`$value`形式的数据绑定，要变更为`this.value`的形式。

**变更原因：**

在ArkTS-Sta中，不支持`$value`形式的数据绑定。

**适配建议：**

把`$value`变更为`this.value`的形式。

**示例：**

ArkTS-Dyn

```typescript
@Entry
@Component
struct Index {
  @State count: number = 0;

  build() {
    Column() {
      MyCounter({
        count: $count
      })
      Text(`Double: ${this.count * 2}`)
    }
    .padding(60)
    .height('100%')
    .width('100%')
  }
}

@Component
struct MyCounter {
  @Link count: number;

  build() {
    Row() {
      Text(`${this.count}`)
      Blank()
      Button('+', { type: ButtonType.Circle }).onClick((e: ClickEvent) => {this.count++;})
      Button('-', { type: ButtonType.Circle }).onClick((e: ClickEvent) => {this.count--;})
    }
    .width('100%')
  }
}
```

ArkTS-Sta

```typescript
'use static'
import {
  Entry,
  Component,
  State,
  Column,
  Text,
  Link,
  Row,
  Blank,
  Button,
  ButtonType,
  ClickEvent,
  Circle,
} from '@kit.ArkUI';

@Entry
@Component
struct Index {
  @State count: number = 0;

  build() {
    Column() {
      MyCounter({
        count: this.count
      })
      Text(`Double: ${this.count * 2}`)
    }
    .padding(60)
    .height('100%')
    .width('100%')
  }
}

@Component
struct MyCounter {
  @Link count: number;

  build() {
    Row() {
      Text(`${this.count}`)
      Blank()
      Button('+', { type: ButtonType.Circle }).onClick((e: ClickEvent) => {this.count++;})
      Button('-', { type: ButtonType.Circle }).onClick((e: ClickEvent) => {this.count--;})
    }
    .width('100%')
  }
}
```