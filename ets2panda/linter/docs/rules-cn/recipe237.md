## 元组和数组是两种不同类型

**规则：** `arkts-no-tuples-arrays`

**规则解释：**

ArkTS-Sta中数组和元组是不同的类型，元组不支持Array拥有的接口和属性。

**变更原因：**
 
ArkTS-Sta中数组和元组是不同的类型，运行时使用元组类型可以获得更好的性能。

**适配建议：**

不要使用数组类型标注元组，而应正确使用对象类型。

**示例：**

ArkTS-Dyn

```typescript
const tuple1: [number, number, boolean] = [1, 3.14, true];
const array: (number | boolean) [] = tuple1;

const tuple2: Array<number | boolean> = [1, 3.14, true]; // 违反规则

function getTuple(input: (number | boolean)[]): (number | boolean)[] { // 违反规则
  return input;
}

getTuple([1, 3.14, true]); // 传入元组

type Point = (number | boolean)[]; // 违反规则
const p: Point = [3, 5, true];

let a: [number, string] = [1, "a"];
console.info("length=" + a.length); // 可以通过.length获取元组长度
```

ArkTS-Sta

```typescript
const tuple1: [number, number, boolean] = [1, 3.14, true];
const array: [number, number, boolean] = tuple1;

const tuple2: [number, number, boolean] = [1, 3.14, true]; // 正确使用元组

function getTuple(input: [number, number, boolean]): [number, number, boolean] { // 正确使用元组
  return input;
}

getTuple([1, 3.14, true]);

type Point = [number, number, boolean]; // 使用元组
const p: Point = [3, 5, true];

let a: [number, string] = [1, "a"];
console.info("length=" + 2); // 元组不支持.length接口，元组长度固定，直接输入长度
```