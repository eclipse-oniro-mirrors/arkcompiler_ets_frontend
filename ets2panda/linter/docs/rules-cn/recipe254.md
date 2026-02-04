## 不支持`@Extend`装饰器

**规则：** `arkui-no-extend-decorator`

**规则解释：**

在ArkTS-Sta中，不支持`@Extend`装饰器，使用`@Extend`装饰的函数需要进行修改。

**变更原因：**

在ArkTS-Sta中，`@Extend`装饰器被废弃。

**适配建议：**

使用`@Extend`装饰的函数需要按照示例进行修改。

**示例：**

ArkTS-Dyn

```typescript
@Extend(Text)
function fancy(fontSize: number) {
  .fontColor(Color.Red)
  .fontSize(fontSize)
}

@Entry
@Component
struct Index {
  build() {
    Column() {
      Text('Test').fancy(50)
    }
  }
}
```

ArkTS-Sta

```typescript
'use static'
import {
  TextAttribute,
  Color,
  Entry,
  Component,
  Column,
  Text,
} from '@kit.ArkUI';

// 使用`@Extend`装饰的函数需要参照下列代码进行修改
function fancy(this: TextAttribute, fontSize: number): TextAttribute {
    this.fontColor(Color.Red);
    this.fontSize(fontSize);
    return this;
}

@Entry
@Component
struct Index {
  build() {
    Column() {
      Text('Test').fancy(50.0)
    }
  }
}
```