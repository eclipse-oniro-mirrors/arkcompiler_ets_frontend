## 异步生命周期变更

**规则：** `sdk-ability-asynchronous-lifecycle`

**规则解释：**

[onDestroy](../application-models/uiability-lifecycle.md#ondestroy)是UIAbility生命周期回调，当UIAbility被销毁时，系统会触发该回调。

开发者可以在该生命周期中执行资源清理等相关操作，使用同步回调或Promise异步回调。

在ArkTS-Sta中，void无法作为联合类型，需要把onDestroy()拆分为两个接口：同步调回onDestroy(): void或异步调回onDestroyAsync(): Promise\<void\>。

**变更原因：**

ArkTS-Sta对void类型的语义进行了收紧，限制其使用场景以增强类型安全性。

**适配建议：**

根据原onDestroy实现，将其拆分到对应的onDestroy或onDestroyAsync接口中。

**示例：**

ArkTS-Dyn

```
import { UIAbility } from '@kit.AbilityKit';

function sleep(ms: number): Promise<void> {
  return new Promise((resolve, reject) => {
    setTimeout(resolve, ms)
  })
}

export default class MyUIAbility extends UIAbility {
  async onDestroy(): Promise<void> {
    console.info('testTag', '%{public}s', 'Ability onDestroy');
    return sleep(1000);
  }
}
```

ArkTS-Sta

```
import { UIAbility } from '@kit.AbilityKit';

function sleep(ms: double): Promise<void> {
  return new Promise<void>((resolve, reject) => {
    setTimeout(resolve)
  })
}

class MyUIAbility extends UIAbility {
  onDestroyAsync(): Promise<void> {
    console.info('testTag', '%{public}s', 'Ability onDestroy');
    return sleep(1000);
  }
}
```