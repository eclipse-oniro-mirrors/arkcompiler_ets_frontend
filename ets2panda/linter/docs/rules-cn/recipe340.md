### ArkTS-Sta await JS Promise对象

**规则：** `arkts-interop-js2s-await-js-promise`

**规则解释：**

ArkTS-Sta 不支持直接await JS中的Promise对象。

**变更原因：**

ArkTS-Sta中只能和有类型声明的文件进行交互。
ArkTS-Sta中限制ESValue的动态行为，形成动静态更清晰的界限，减少开发者滥用ESValue导致性能劣化的场景。

**适配建议：**

使用ESValue接口转换为Promise对象后再await。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.js
async function foo(){}
export let p = foo()

// file2.ets
import {p} from "./file1"
async function bar() {
  await p.toPromise();
}
```

**ArkTS-Sta**
```typescript
// file1.js
async function foo(){}
export let p = foo()

// file2.ets  // ArkTS-Sta
'use static'
let mod = ESValue.load('./file1')
let p = mod.getProperty('p')

async function bar() {
  await p.toPromise();
}
```