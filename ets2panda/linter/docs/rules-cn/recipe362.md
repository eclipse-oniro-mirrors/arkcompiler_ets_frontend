## 使用`wrappBuilder`时，泛型参数必须为箭头函数

**规则：** `arkui-wrapbuilder-require-arrow-func-generic`

**级别：** error

在ArkTS1.2中，`wrapBuilder`的泛型是可选的。如果指定了泛型，则其参数必须为箭头函数。

**ArkTS1.1**
 
```typescript
@Builder
function MyBuilder(value: string, size: number) {
  Text(value)
    .fontSize(size);
}

const wrappedBuilder1: WrappedBuilder<[string, number]> = wrapBuilder(MyBuilder);
const wrappedBuilder2: WrappedBuilder<[string, number]> = wrapBuilder<[string, number]>(MyBuilder);

@Entry
@Component
struct TestWrappedBuilder1 {
  @State message: string = 'Hello World';

  build() {
    Row() {
      Column() {
        wrappedBuilder1.builder(this.message, 50);
        wrappedBuilder2.builder(this.message, 50);
      }
    }
    .height('100%')
    .width('100%')
  }
}
```

**ArkTS1.2**

```typescript
'use static'
import {
  Builder,
  Text,
  WrappedBuilder,
  wrapBuilder,
  Entry,
  Component,
  State,
  Row,
  Column,
} from '@kit.ArkUI';

@Builder
function MyBuilder(value: string, size: number) {
  Text(value)
    .fontSize(size);
}

const wrappedBuilder1: WrappedBuilder<@Builder (arg1: string, arg2: number) => void> = wrapBuilder(MyBuilder);
const wrappedBuilder2: WrappedBuilder<@Builder (arg1: string, arg2: number) => void> = wrapBuilder<@Builder (arg1: string, arg2: number) => void>(MyBuilder);

@Entry
@Component
struct TestWrappedBuilder1 {
  @State message: string = 'Hello World';

  build() {
    Row() {
      Column() {
        wrappedBuilder1.builder(this.message, 50.0);
        wrappedBuilder2.builder(this.message, 50.0);
      }
    }
    .height('100%')
    .width('100%')
  }
}
```