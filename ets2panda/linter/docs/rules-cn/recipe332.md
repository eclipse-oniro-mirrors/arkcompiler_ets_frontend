### ArkTS-Sta访问JS属性

**规则：** `arkts-interop-js2s-access-js-prop`

**规则解释：**

ArkTS-Sta不能直接访问JS属性。

**变更原因：**

ArkTS-Sta中只能和有类型声明的文件进行交互。
ArkTS-Sta中限制ESValue的动态行为，形成动静态更清晰的界限，减少开发者滥用ESValue导致性能劣化的场景。

**适配建议：**

使用ESValue的接口访问属性，接口接收参数为ESValue类型，传参时需要用wrap接口构造ESValue实例再传参。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.js
export let foo = { name: '123' };
// file2.ets
import { foo } from './file1';
foo.name;
foo.name = '456';
```

**ArkTS-Sta**
```typescript
// file1.js
export let foo = {name: "123"}

// file2.ets  // ArkTS-Sta
'use static'
let mod = ESValue.load('./file1')
let foo = mod.getProperty('foo')
foo.getProperty('name')
foo.setProperty('name', ESValue.wrap("456"))
```