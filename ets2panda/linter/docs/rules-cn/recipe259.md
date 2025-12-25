## 使用UI接口前需要导入

**规则：** `arkui-modular-interface`

**规则解释：**

在ArkTS-Sta中，使用UI接口前必须先导入，否则会违反该规则导致报错，非UI接口不会有报错信息。

**变更原因：**

在ArkTS-Sta中，使用UI接口时如果不导入，编译会报错。

**适配建议：**

使用UI接口前先导入。

**示例：**

ArkTS-Dyn

```typescript
@Entry
@Component
struct Index {
  build() {
    Column() {
      Text('UI import')
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
  Text,
} from '@kit.ArkUI';

@Entry
@Component
struct Index {
  build() {
    Column() {
      Text('UI import')
    }
  }
}
```