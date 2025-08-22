### ArkTS1.2 await js Promise对象

**规则：** `arkts-interop-js2s-await-js-promise`

ArkTS1.2在await js中的Promise对象时，先使用ESValue接口转换为Promise对象后再await。

**ArkTS1.1**
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

**ArkTS1.2**
```typescript
// file1.js
async function foo(){}
export let p = foo()

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1')
let p = mod.getProperty('p')

async function bar() {
  await p.toPromise();
}
```