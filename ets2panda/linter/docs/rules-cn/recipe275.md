## 自定义组件需要加上`@CustomLayout`装饰器以获得自定义布局能力

**规则：** `arkui-custom-layout-need-add-decorator`

**规则解释：**

在ArkTS-Sta中，自定义组件需要加上`@CustomLayout`装饰器，才能获得自定义布局能力。

**变更原因：**

受ArkTS-Sta静态类型系统的影响，自定义组件需要组件加上`@CustomLayout`装饰器，才能获得自定义布局能力。

**适配建议：**

为自定义组件加上`@CustomLayout`装饰器。

**示例：**

ArkTS-Dyn

```typescript
@Entry
@Component
struct Index {
  build() {
    Column() {
      MyComponent({ builder: ColumnChildren })
    }
  }
}

@Builder
function ColumnChildren() {
  ForEach([1, 2, 3], (index: number) => {
    Text('S' + index)
      .fontSize(30)
      .width(100)
      .height(100)
      .borderWidth(2)
      .offset({ x: 10, y: 20 } as Position)
  })
}

@Component
struct MyComponent {
  @Builder
  doNothingBuilder() {
  };

  @BuilderParam builder: () => void = this.doNothingBuilder;
  @State startSize: number = 100;
  result: SizeResult = {
    width: 0,
    height: 0
  } as SizeResult;

  onPlaceChildren(selfLayoutInfo: GeometryInfo, children: Array<Layoutable>, constraint: ConstraintSizeOptions) {
    let startPos = 300;
    children.forEach((child) => {
      let pos = startPos - child.measureResult.height;
      child.layout({ x: pos, y: pos });
    })
  }

  onMeasureSize(selfLayoutInfo: GeometryInfo, children: Array<Measurable>, constraint: ConstraintSizeOptions) {
    let size = 100;
    children.forEach((child) => {
      let result: MeasureResult = child.measure({ minHeight: size, minWidth: size, maxWidth: size, maxHeight: size });
      size += result.width / 2;
    })
    this.result.width = 100;
    this.result.height = 400;
    return this.result;
  }

  build() {
    this.builder()
  }
}
```

ArkTS-Sta

```typescript
'use static'
import { CustomLayout } from '@kit.ArkUI';

import {
  Entry,
  Component,
  Column,
  Builder,
  ForEach,
  Text,
  Position,
  BuilderParam,
  State,
  SizeResult,
  GeometryInfo,
  Layoutable,
  ConstraintSizeOptions,
  Measurable,
  MeasureResult
} from '@kit.ArkUI';

@Entry
@Component
struct Index {
  build() {
    Column() {
      MyComponent({ builder: ColumnChildren })
    }
  }
}

@Builder
function ColumnChildren() {
  ForEach([1, 2, 3], (item: Int, index: number) => {
    Text('S' + index)
      .fontSize(30)
      .width(100)
      .height(100)
      .borderWidth(2)
      .offset({ x: 10, y: 20 } as Position)
  })
}

@Component
@CustomLayout
struct MyComponent {
  @Builder
  doNothingBuilder() {
  };

  @BuilderParam builder: () => void = this.doNothingBuilder;
  @State startSize: number = 100;
  result: SizeResult = {
    width: 0,
    height: 0
  } as SizeResult;

  onPlaceChildren(selfLayoutInfo: GeometryInfo, children: Array<Layoutable>, constraint: ConstraintSizeOptions) {
    let startPos = 300;
    children.forEach((child) => {
      let pos = startPos - child.measureResult.height;
      child.layout({ x: pos, y: pos });
    })
  }

  onMeasureSize(selfLayoutInfo: GeometryInfo, children: Array<Measurable>, constraint: ConstraintSizeOptions) {
    let size = 100;
    children.forEach((child) => {
      let result: MeasureResult = child.measure({ minHeight: size, minWidth: size, maxWidth: size, maxHeight: size });
      size += result.width / 2;
    })
    this.result.width = 100;
    this.result.height = 400;
    return this.result;
  }

  build() {
    this.builder()
  }
}
```