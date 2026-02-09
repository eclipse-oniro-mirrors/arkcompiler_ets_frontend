## `@AnimatableExtend`装饰器使用方式变更

**规则：** `arkui-animatableextend-use-receiver`

**规则解释：**

在ArkTS-Sta中，使用`@AnimatableExtend`装饰器装饰的函数需要进行修改。

**变更原因：**

受ArkTS-Sta静态类型系统的影响，使用`@AnimatableExtend`装饰器装饰的函数需要进行修改。

**适配建议：**

使用`@AnimatableExtend`装饰器装饰的函数需要按照示例进行修改。

**示例：**

ArkTS-Dyn

```typescript
@AnimatableExtend(Text)
function animatableWidth(width: number) {
  .width(width)
}

@Entry
@Component
struct Index {
  @State textWidth: number = 80;

  build() {
    Column() {
      Text('AnimatableProperty')
        .animatableWidth(this.textWidth)
        .animation({ duration: 2000, curve: Curve.Ease })
      Button("Play")
        .onClick((e: ClickEvent) => {
          this.textWidth = this.textWidth == 80 ? 160 : 80;
        })
    }.width("100%")
    .padding(10)
  }
}
```

ArkTS-Sta

```typescript
'use static'
import {
  AnimatableExtend,
  TextAttribute,
  Entry,
  Component,
  State,
  Column,
  Text,
  Curve,
  Button,
  ClickEvent,
} from '@kit.ArkUI';

// 使用`@AnimatableExtend`装饰器装饰的函数需要参照下列代码进行修改
@AnimatableExtend
function animatableWidth(this: TextAttribute, width: number): TextAttribute {
    this.width(width);
    return this;
}

@Entry
@Component
struct Index {
  @State textWidth: number = 80.0;

  build() {
    Column() {
      Text('AnimatableProperty')
        .animatableWidth(this.textWidth)
        .animation({ duration: 2000, curve: Curve.Ease })
      Button("Play")
        .onClick((e: ClickEvent) => {
          this.textWidth = this.textWidth == 80.0 ? 160.0 : 80.0;
        })
    }.width("100%")
    .padding(10.0)
  }
}
```