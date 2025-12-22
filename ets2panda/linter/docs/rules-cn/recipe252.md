## 不支持`$$this.value`形式的双向绑定

**规则：** `arkui-no-$$-bidirectional-data-binding`

在ArkTS1.2中，不支持`$$this.value`形式的双向绑定，应改为`$$(this.value)`的形式。

**ArkTS1.1**

```typescript
@Entry
@Component
struct Index {
  @State text: string = '';

  build() {
    Column() {
      Text(this.text)
      TextInput({text: $$this.text})
        .width(300)
    }.width('100%').height('100%')
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
  TextInput,
  $$,
} from '@kit.ArkUI';

@Entry
@Component
struct Index {
  @State text: string = '';

  build() {
    Column() {
      Text(this.text)
      TextInput({text: $$(this.text)})
        .width(300)
    }.width('100%').height('100%')
  }
}
```