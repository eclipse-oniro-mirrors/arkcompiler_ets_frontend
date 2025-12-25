### ArkTS-Sta访问TS独有类型的实体

**规则：** `arkts-interop-ts2s-static-access-ts-type`

**规则解释：**

ArkTS-Sta中不支持TS中独有的类型。

**变更原因：**

TS独有类型包括如下类型：
- any
- unknown
- symbol
- Function
- object literal （例如 {x: number, y: string}）
- mixing enum （例如 enum X {a = 0, b = '1'}）
- call signature （例如 {(arg: number): string}）
- constructor signature （例如 {new(): Object}）
- index signature （例如 {[index: number]: string}）
- intersection （例如 TypeA & TypeB）
- keyof （例如 interface X<T> { props: keyof T}）
- typeof（例如 let p = {x: 1, y: ''}, let q: typeof p）
- indexed access type（例如 MyArray = [{ name: "Alice", age: 15 }] type Person = typeof MyArray[number]）
- conditional types （例如 type Swap<T extends A | B> = T extends A ? B : A）
- mapped types （例如 type A<T> = {[K in keyof T]: T[K]}）
- template literal types （例如 type AB = "A" | "B", type AllLocaleIDs = `${AB}_id`）
- Pick<Type, Keys>
- Omit<Type, Keys>
- Exclude<UnionType, ExcludedMembers>
- Extract<Type, Union>
- NonNullable<Type>
- Parameters<Type>
- ConstructorParameters<Type>
- ReturnType<Type>
- InstanceType<Type>
- NoInfer<Type>
- ThisParameterType<Type>
- OmitThisParameter<Type>
- ThisType<Type>
- Uppercase<StringType>
- Lowercase<StringType>
- Capitalize<StringType>
- Uncapitalize<StringType>

ArkTS-Sta中不支持这些类型。

**适配建议：**

使用ESValue接口进行交互。

**示例：**

**ArkTS-Dyn**
```typescript
// file1.ts
export let obj: Symbol;

// file2.ets
import { obj } from './file1';
let val = obj.prop;
obj.prop = 1;
obj.foo();
let item = obj[0];
```

**ArkTS-Sta**
```typescript
// file1.ts
export let obj: Symbol;
// 从ArkTS-Sta看来，这个声明为
// export let obj: ESValue

// file2.ets ArkTS-Sta
'use static'
import { obj } from './file1';
obj.setProperty('prop', ESValue.wrap(1));
```