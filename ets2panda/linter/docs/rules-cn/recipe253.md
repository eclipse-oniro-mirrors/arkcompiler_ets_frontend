## 不支持`$value`形式的数据绑定

**规则：** `arkui-link-decorator-passing`

在ArkTS1.2中，不支持`$value`形式的数据绑定，要变更为`this.value`的形式。

**ArkTS1.1**

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

**ArkTS1.2**

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