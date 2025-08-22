## 不支持`@Styles`装饰器

**规则：** `arkui-no-styles-decorator`

在ArkTS1.2中，不支持`@Styles`装饰器。使用`@Styles`装饰器装饰的函数需要进行修改。

### `@Styles`装饰器装饰全局函数

**ArkTS1.1**

```typescript
@Styles
function cardStyle() {
  .backgroundColor(Color.Red)
  .borderRadius(8)
  .padding(8)
}

@Entry
@Component
struct Index {
  build() {
    Column() {
      Text('Card')
    }
    .cardStyle()
    .width('100%')
  }
}
```

**ArkTS1.2**

```typescript
'use static'
import {
  CommonMethod,
  Color,
  Entry,
  Component,
  Column,
  Text,
  applyStyles,
} from '@kit.ArkUI';

// 使用`@Styles`装饰器装饰的函数需要参照下列代码进行修改
function cardStyle(instance: CommonMethod): void {
  instance.backgroundColor(Color.Red);
  instance.borderRadius(8.0);
  instance.padding(8.0);
}

@Entry
@Component
struct Index {
  build() {
    Column() {
      Text('Card')
    }
    .applyStyles(cardStyle)
    .width('100%')
  }
}
```

### `@Styles`装饰器装饰成员函数

**ArkTS1.1**

```typescript
@Entry
@Component
struct Index {
  @Styles
  cardStyles() {
    .backgroundColor(Color.Blue)
    .borderRadius(8)
    .padding(8)
  }

  build() {
    Column() {
      Text('Card')
    }
    .cardStyles()
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
  CommonMethod,
  Color,
  Column,
  Text,
  CustomStyles,
  applyStyles,
} from '@kit.ArkUI';

@Entry
@Component
struct Index {
  // 使用`@Styles`装饰器装饰的函数需要参照下列代码进行修改
  cardStyles: CustomStyles = (instance: CommonMethod): void => {
    instance.backgroundColor(Color.Blue);
    instance.borderRadius(8.0);
    instance.padding(8.0);
  }

  build() {
    Column() {
      Text('Card')
    }
    .applyStyles(this.cardStyles)
    .width('100%')
  }
}
```