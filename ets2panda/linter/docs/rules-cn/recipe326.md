### ArkTS-Dyn创建ArkTS-Sta对象字面量

**规则：** `arkts-interop-s2d-object-literal`

**规则解释：**

ArkTS-Dyn中使用构造函数创建ArkTS-Sta对象字面量。

**变更原因：**

ArkTS-Dyn的对象字面量是动态对象，不是真正的标注类型，所以ArkTS-Dyn中使用构造函数创建ArkTS-Sta对象字面量。

**适配建议：**

使用构造函数创建对象字面量。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.ets
export class X {
  name: string = '';
  constructor(arg: string) {
    this.name = arg;
  }
}
export interface Y {
  data: number;
}
export type MyRecord = Record<string, number>;
export function foo(arg: X) {}
export function bar(arg: Y) {}

// file2.ets
import { X, Y } from './file1';
let x = { name: 'hello' };
let y: Y = { data: 123 };
foo({ name: 'world' });
bar({ data: 456 });
// 返回值 zoo(): X { return {..}}
// 嵌套场景
interface Z {
  x: X;
}
let z: Z = {x: { name: 'hello' }};
```

**ArkTS-Sta**
```typescript
// file1.ets ArkTS-Sta
'use static'
export class X { name: string = '' }
export interface Y { data: number }
export function foo(arg: X) { }
export function bar(arg: Y) { }
export function createY(d: number): Y {
  let y: Y = { data: d }
  return y
}

// file2.ets ArkTS-Dyn
import { X, Y, createY } from "./file1"
let x: X = new X("hello")
let y: Y = createY(123)
foo(new X("world"))
bar(createY(456))
```