## 扫描生命周期监听变更

**规则：** `sdk-ability-lifecycle-monitor`

**规则解释：**

在ArkTS-Sta中，UIAbility需要用新的接口StaticAbilityLifecycleCallback监听。

**变更原因：**

ArkTS-Sta的UIAbility不支持使用[AbilityLifecycleCallback](../reference/apis-ability-kit/js-apis-app-ability-abilityLifecycleCallback.md)监听UIAbility。

**适配建议：**

在ArkTS-Sta中，使用接口StaticAbilityLifecycleCallback监听UIAbility。

**示例：**

**ArkTS-Dyn**

```
import { UIAbility, AbilityStage, AbilityLifecycleCallback} from '@kit.AbilityKit';

class MyAbilityStage extends AbilityStage {
  onCreate() {
    let AbilityLifecycleCallback: AbilityLifecycleCallback = {
      onAbilityCreate(ability) {
        console.info(`AbilityLifecycleCallback onAbilityCreate ability: ${ability}`);
      },
      onWindowStageCreate(ability, windowStage) {
        console.info(`AbilityLifecycleCallback onWindowStageCreate ability: ${ability}`);
        console.info(`AbilityLifecycleCallback onWindowStageCreate windowStage: ${windowStage}`);
      },
      onWindowStageActive(ability, windowStage) {
        console.info(`AbilityLifecycleCallback onWindowStageActive ability: ${ability}`);
        console.info(`AbilityLifecycleCallback onWindowStageActive windowStage: ${windowStage}`);
      },
      onWindowStageInactive(ability, windowStage) {
        console.info(`AbilityLifecycleCallback onWindowStageInactive ability: ${ability}`);
        console.info(`AbilityLifecycleCallback onWindowStageInactive windowStage: ${windowStage}`);
      },
      onWindowStageDestroy(ability, windowStage) {
        console.info(`AbilityLifecycleCallback onWindowStageDestroy ability: ${ability}`);
        console.info(`AbilityLifecycleCallback onWindowStageDestroy windowStage: ${windowStage}`);
      },
      onAbilityDestroy(ability) {
        console.info(`AbilityLifecycleCallback onAbilityDestroy ability: ${ability}`);
      },
      onAbilityForeground(ability) {
        console.info(`AbilityLifecycleCallback onAbilityForeground ability: ${ability}`);
      },
      onAbilityBackground(ability) {
        console.info(`AbilityLifecycleCallback onAbilityBackground ability: ${ability}`);
      },
      onAbilityContinue(ability) {
        console.info(`AbilityLifecycleCallback onAbilityContinue ability: ${ability}`);
      }
    }
    // 1.通过context属性获取applicationContext
    let applicationContext = this.context.getApplicationContext();
    // 2.通过applicationContext注册监听应用内生命周期
    let lifecycleId = applicationContext.on('abilityLifecycle', AbilityLifecycleCallback);
    console.info(`registerAbilityLifecycleCallback lifecycleId: ${lifecycleId}`);
  }
}

```

**ArkTS-Sta**

```
import { UIAbility, AbilityStage, StaticAbilityLifecycleCallback} from '@kit.AbilityKit';

class MyAbilityStage extends AbilityStage {
  onCreate() {
    let StaticAbilityLifecycleCallback: StaticAbilityLifecycleCallback = {
      onAbilityCreate(ability) {
        console.info(`StaticAbilityLifecycleCallback onAbilityCreate ability: ${ability}`);
      },
      onWindowStageCreate(ability, windowStage) {
        console.info(`StaticAbilityLifecycleCallback onWindowStageCreate ability: ${ability}`);
        console.info(`StaticAbilityLifecycleCallback onWindowStageCreate windowStage: ${windowStage}`);
      },
      onWindowStageActive(ability, windowStage) {
        console.info(`StaticAbilityLifecycleCallback onWindowStageActive ability: ${ability}`);
        console.info(`StaticAbilityLifecycleCallback onWindowStageActive windowStage: ${windowStage}`);
      },
      onWindowStageInactive(ability, windowStage) {
        console.info(`StaticAbilityLifecycleCallback onWindowStageInactive ability: ${ability}`);
        console.info(`StaticAbilityLifecycleCallback onWindowStageInactive windowStage: ${windowStage}`);
      },
      onWindowStageDestroy(ability, windowStage) {
        console.info(`StaticAbilityLifecycleCallback onWindowStageDestroy ability: ${ability}`);
        console.info(`StaticAbilityLifecycleCallback onWindowStageDestroy windowStage: ${windowStage}`);
      },
      onAbilityDestroy(ability) {
        console.info(`StaticAbilityLifecycleCallback onAbilityDestroy ability: ${ability}`);
      },
      onAbilityForeground(ability) {
        console.info(`StaticAbilityLifecycleCallback onAbilityForeground ability: ${ability}`);
      },
      onAbilityBackground(ability) {
        console.info(`StaticAbilityLifecycleCallback onAbilityBackground ability: ${ability}`);
      },
      onAbilityContinue(ability) {
        console.info(`StaticAbilityLifecycleCallback onAbilityContinue ability: ${ability}`);
      }
    }
    // 1.通过context属性获取applicationContext
    let applicationContext = this.context.getApplicationContext();
    // 2.通过applicationContext注册监听应用内生命周期
    let lifecycleId = applicationContext.on('abilityLifecycle', StaticAbilityLifecycleCallback);
    console.info(`registerStaticAbilityLifecycleCallback lifecycleId: ${lifecycleId}`);
  }
}
```