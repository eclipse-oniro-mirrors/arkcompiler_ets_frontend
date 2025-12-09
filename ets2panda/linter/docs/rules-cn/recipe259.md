## 使用UI接口前需要导入

**规则：** `arkui-modular-interface`

在ArkTS1.2中，使用UI接口前必须先导入，否则会违反该规则导致报错，非UI接口不会有报错信息。

**ArkTS1.1**

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

**ArkTS1.2**

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