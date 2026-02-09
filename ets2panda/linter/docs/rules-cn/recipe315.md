## 对象名称不可以重复

**规则：** `sdk-no-decl-with-duplicate-name`

**规则解释：**

在ArkTS-Sta中，所有对象不再作为全局对象，而是置于各自的模块中，对象名称不可以重复。

**变更原因：**

在ArkTS-Sta中，模块化开发成为核心特性之一，所有对象不再作为全局对象，而是位于各自的模块中。开发者需要通过import语句显式引入后使用。这一变化提升了代码的可维护性和可扩展性，同时避免了全局命名空间污染。

**适配建议：**

import同名对象时用别名来避免冲突。

**示例：**

ArkTS-Dyn
```typescript
// ArkTS-Dyn
// a.ts
export declare interface LinearGradient {}
export declare class LinearGradient {}

// ArkTS-Dyn应用代码
import { LinearGradient } from './a';
const lg: LinearGradient = {};
const lg2: LinearGradient = new LinearGradient();
```

ArkTS-Sta
```typescript
// ArkTS-Sta，分别放在两个模块中
// @ohos.arkui.a.d.ets
declare interface LinearGradient {}
export { LinearGradient }
// @ohos.arkui.b.d.ets
declare class LinearGradient {}
export { LinearGradient }

// ArkTS-Sta应用代码
import { LinearGradient } from '@ohos.arkui.a.d.ets';
import { LinearGradient as LinearGradientClass } from '@ohos.arkui.b.d.ets';
const lg: LinearGradient = {};
const lg2: LinearGradientClass = new LinearGradientClass();
```