## 不支持`$$this.value`形式的双向绑定

**规则：** `arkui-no-$$-bidirectional-data-binding`

**规则解释：**

在ArkTS-Sta中，不支持`$$this.value`形式的双向绑定，应改为`$$(this.value)`的形式。

**变更原因：**

在ArkTS-Sta中，不支持`$$this.value`形式的双向绑定。

**适配建议：**

把`$$this.value`改为`$$(this.value)`的形式。

**示例：**

ArkTS-Dyn

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

ArkTS-Sta

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