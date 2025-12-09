## 使用`WrappedBuilder`时，需添加参数为箭头函数的泛型

**规则：** `arkui-wrappedbuilder-require-arrow-func-generic`

**级别：** error

在ArkTS1.2中，使用`WrappedBuilder`时，必须添加泛型，且泛型的参数必须为箭头函数。

**ArkTS1.1**

```typescript
@Builder
function MyBuilder(value: string, size: number) {
  Text(value)
    .fontSize(size);
}

const wrappedBuilder1: WrappedBuilder<[string, number]> = new WrappedBuilder(MyBuilder);
const wrappedBuilder2: WrappedBuilder<[string, number]> = new WrappedBuilder<[string, number]>(MyBuilder);

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

const wrappedBuilder1: WrappedBuilder<@Builder (arg1: string, arg2: number) => void> = new WrappedBuilder<@Builder (arg1: string, arg2: number) => void>(MyBuilder);
const wrappedBuilder2: WrappedBuilder<@Builder (arg1: string, arg2: number) => void> = new WrappedBuilder<@Builder (arg1: string, arg2: number) => void>(MyBuilder);

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