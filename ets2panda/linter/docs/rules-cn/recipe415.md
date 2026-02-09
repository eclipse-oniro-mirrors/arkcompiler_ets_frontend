## 不支持enableV2Compatibility和makeV1Observed方法

**规则：** `arkui-no-enableV2Compatibility-and-makeV1Observed-function`

**规则解释：**

ArkTS-Sta不支持enableV2Compatibility方法和makeV1Observed方法。

**变更原因：**
 
由于ArkTS-Sta状态管理V1和V2天然支持V1/V2状态变量的传递，因此无需使用enableV2Compatibility方法与makeV1Observed方法进行状态变量的转换。

**适配建议：**

删除UIUtils.enableV2Compatibility方法和UIUtils.makeV1Observed方法的调用。

**示例：**

ArkTS-Dyn

```typescript
// main.ets
import { UIUtils } from '@kit.ArkUI';

class CommonClass {
  name: string = 'a';
}

@Entry
@Component
struct CompV2 {
  @State commonObject: CommonClass = UIUtils.makeV1Observed(new CommonClass());
  build() {
    Column() {
      CompV1({ commonChildObject: UIUtils.enableV2Compatibility(this.commonObject) })
    }
  }
}

@ComponentV2
struct CompV1 {
  @Param @Require commonChildObject: CommonClass;

  build() {
    Column() {
      Text(`name =  ${this.commonChildObject.name}`)
    }
  }
}
```

ArkTS-Sta

```typescript
// main.ets
import { Entry, Component, State, Column, ComponentV2, Param, Require, Text } from '@kit.ArkUI';

class CommonClass {
  name: string = 'a';
}

@Entry
@Component
export struct V2DecoratorDecorateObserveV2 {
  @State commonObject: CommonClass = new CommonClass();
  build() {
    Column() {
      CompV1({ commonChildObject: this.commonObject })
    }
  }
}

@ComponentV2
struct CompV1 {
  @Param @Require commonChildObject: CommonClass;

  build() {
    Column() {
      Text(`name =  ${this.commonChildObject.name}`)
    }
  }
}
```
