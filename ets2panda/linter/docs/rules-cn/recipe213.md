## class默认支持懒加载

**规则：** `arkts-class-lazy-import`

**规则解释：**

ArkTS-Dyn中的类声明了就会加载，而ArkTS-Sta的类默认是懒加载的。

**变更原因：**

ArkTS-Sta的类默认是懒加载的，这可以提升启动性能并减少内存占用。

**适配建议：**

要加载类需要显式地实例化或调用；组件开发中，在指定时机执行的代码要移动到合适的组件、生命周期里。

**示例：**

- 场景1，类声明的时候不会自动加载执行static方法。

ArkTS-Dyn

```typescript
class C {
  static {
    console.info('init');  // ArkTS-Dyn中类会立即加载，并运行静态方法
  }
}
```

ArkTS-Sta

```typescript
class C {
  static {
    console.info('init');  // ArkTS-Sta中类不会立即加载
  }
}
new C(); // 通过实例化可以显式地加载类，才会执行里面的静态方法。
```

- 场景2，全局作用域的实现与类相同，均采用懒加载方式，因此全局作用域中的顶层代码不会立即执行。

ArkTS-Dyn

```typescript
import { AbilityConstant, ConfigurationConstant, UIAbility, Want } from '@kit.AbilityKit';

console.info("init UIAbility"); // 在ArkTS-Dyn中，文件被引入了就会执行这条代码

export default class EntryAbility extends UIAbility {
  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    this.context.getApplicationContext().setColorMode(ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET);
  }

  // ...
}
```

ArkTS-Sta

```typescript
import { AbilityConstant, ConfigurationConstant, UIAbility, Want } from '@kit.AbilityKit';

console.info("init UIAbility"); // 在ArkTS-Sta中，文件被引入不会直接执行这条的代码

export default class EntryAbility extends UIAbility {
  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    console.info("init UIAbility"); // 可以放在在onCreate生命周期里执行
    this.context.getApplicationContext().setColorMode(ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET);
  }

  // ...
}
```