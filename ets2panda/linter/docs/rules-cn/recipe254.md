## 不支持`@Extend`装饰器

**规则：** `arkui-no-extend-decorator`

在ArkTS1.2中，不支持`@Extend`装饰器，使用`@Extend`装饰的函数需要进行修改。

**ArkTS1.1**

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

**ArkTS1.2**

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
function fancy(this: TextAttribute, fontSize: number): this {
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