## 对象名称不可以重复

**规则：** `sdk-no-decl-with-duplicate-name`

**规则解释：**

在ArkTS1.2中，所有对象不再作为全局对象，而是置于各自的模块中，对象名称不可以重复。

**变更原因：**

在ArkTS1.2中，模块化开发成为核心特性之一，所有对象不再作为全局对象，而是位于各自的模块中。开发者需要通过import语句显式引入后使用。这一变化提升了代码的可维护性和可扩展性，同时避免了全局命名空间污染。

**适配建议：**

import同名对象时用别名来避免冲突。

**示例：**

**ArkTS1.1**
```typescript
// ArkTS1.1
declare interface LinearGradient {}
declare class LinearGradient {}

// ArkTS1.1应用代码
const lg: LinearGradient = {};
const lg2: LinearGradient = new LinearGradient();
```

**ArkTS1.2**
```typescript
// ArkTS1.2，分别放在两个模块中
// @ohos.arkui.a.d.ts
declare interface LinearGradient {}
export { LinearGradient }
// @ohos.arkui.b.d.ts
declare class LinearGradient {}
export { LinearGradient }

// ArkTS1.2应用代码
import { LinearGradient } from '@ohos.arkui.a.d.ts';
import { LinearGradient as LinearGradientClass } from '@ohos.arkui.b.d.ts';
const lg: LinearGradient = {};
const lg2: LinearGradientClass = new LinearGradientClass();
```