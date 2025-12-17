## 不支持`@LocalBuilder`装饰器

**规则：** `arkui-no-localbuilder-decorator`

**规则解释：**

在ArkTS-Sta中，不支持`@LocalBuilder`装饰器，用`@Builder`装饰器替代。

**变更原因：**

在ArkTS-Sta中，`@LocalBuilder`装饰器和`@Builder`装饰器的作用相同，因此废弃`@LocalBuilder`装饰器。

**适配建议：**

用`@Builder`装饰器替代`@LocalBuilder`装饰器。

**示例：**

ArkTS-Dyn

```typescript
@Entry
@Component
struct Index {
  @State label: string = 'Hello World';

  @LocalBuilder
  citeLocalBuilder(params: string) {
    Row() {
      Text(`UseStateVarByReference: ${params}`)
    }
  };

  build() {
    Column() {
      this.citeLocalBuilder(this.label)
    }
  }
}
```

ArkTS-Sta

```typescript
'use static'
import { Builder } from '@kit.ArkUI';

import {
  Entry,
  Component,
  State,
  Row,
  Text,
  Column,
} from '@kit.ArkUI';

@Entry
@Component
struct Index {
  @State label: string = 'Hello World';

  @Builder
  citeLocalBuilder(params: string) {
    Row() {
      Text(`UseStateVarByReference: ${params}`)
    }
  };

  build() {
    Column() {
      this.citeLocalBuilder(this.label)
    }
  }
}
```