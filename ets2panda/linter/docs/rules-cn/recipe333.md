### ArkTS-Sta转换JS对象类型

**规则：** `arkts-interop-js2s-convert-js-type`

**规则解释：**

ArkTS-Sta不支持直接转换JS对象类型。

**变更原因：**

ArkTS-Sta中只能和有类型声明的文件进行交互。
ArkTS-Sta中限制ESValue的动态行为，形成动静态更清晰的界限，减少开发者滥用ESValue导致性能劣化的场景。

**适配建议：**

使用ESValue的接口转换类型。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.js
export let foo1 = { num: 123 };
export let foo2 = { bool: true };
export let foo3 = { str: '123' };
export let foo4 = { big: 123n };

// file2.ets
import { foo } from './file1';
let a: number = foo1.num as number;
let b: boolean = foo2.bool as boolean;
let c: string = foo3.str as string;
let d: bigint = foo4.big as bigint;
```

**ArkTS-Sta**
```typescript
// file1.js
export let foo1 = { num: 123 };
export let foo2 = { bool: true };
export let foo3 = { str: '123' };
export let foo4 = { big: 123n };

// file2.ets  // ArkTS-Sta
'use static'
let mod = ESValue.load('./file1');
let foo1 = mod.getProperty('foo1');
let num = foo1.getProperty('num');
let a1: number = num.toNumber();

let foo2 = mod.getProperty('foo2');
let bool = foo2.getProperty('bool');
let a2: boolean = bool.toBoolean();

let foo3 = mod.getProperty('foo3');
let str = foo3.getProperty('str');
let a3: string = str.toString();

let foo4 = mod.getProperty('foo4');
let big = foo4.getProperty('big');
let a4: bigint = big.toBigInt();
```