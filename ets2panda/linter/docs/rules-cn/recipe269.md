### js对ArkTS1.2对象进行展开语法

**规则：** `arkts-interop-js2s-js-expand-static-instance`

js对ArkTS1.2对象进行展开语法时，需重新适配代码。

**ArkTS1.1**
```typescript
// file1.js
export function foo(obj) {
let x = {...obj} // x会是{ a = 1; b = 2; c = 3 }
let {a, b, ...rest} = obj  // a会是1, b会是2, rest会是{c: 3}

// file2.ets
import {foo} from "./file1"
class X { a = 1; b = 2; c = 3 }
foo(new X())

// class interface Record
```

**ArkTS1.2**
```typescript
// file1.js
export function foo(obj) {
let x = {...obj} // x会是空对象{}，因为静态对象没有自有属性
// 解决方案：let x = {a: obj.a, b: obj.b, c: obj.c}
// 或者使用keys + Reflect.get
let {a, b, ...rest} = obj  // a会是1，b会是2，rest会是空对象{}，因为静态对象没有自有属性
// 解决方案: let rest = {c: obj.c}

// file2.ets  // ArkTS1.2
'use static'
let mod = ESValue.load('./file1')
let foo = mod.getProperty('foo')
class X { a = 1; b = 2; c = 3 }
foo.invoke(ESValue.wrap(new X()))
```