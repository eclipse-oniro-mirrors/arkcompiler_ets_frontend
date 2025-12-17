### ArkTS-Sta调用JS函数和传参

**规则：** `arkts-interop-js2s-call-js-func`

**规则解释：**

ArkTS-Sta中不能直接调用JS函数和传参。

**变更原因：**

ArkTS-Sta中只能和有类型声明的文件进行交互。
ArkTS-Sta中限制ESValue的动态行为，形成动静态更清晰的界限，减少开发者滥用ESValue导致性能劣化的场景。

**适配建议：**

使用ESValue的接口调用，接口接收参数为ESValue类型，传参时需要用wrap接口构造ESValue实例再传参。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.js
export function foo() {}
export function bar(a) {}

// file2.ets
import { foo, bar } from './file1';
foo();
bar(123);
```

**ArkTS-Sta**
```typescript
// file1.js
export function foo() {}
export function bar(a) {}

// file2.ets  // ArkTS-Sta
'use static'
let mod = ESValue.load('./file1');
let foo = mod.getProperty('foo');
let bar = mod.getProperty('bar');
foo.invoke();
bar.invoke(ESValue.wrap(123));
```