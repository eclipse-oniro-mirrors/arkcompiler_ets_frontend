## 元组和数组是两种不同类型

**规则：** `arkts-no-tuples-arrays`

**规则解释：**

ArkTS1.2中数组和元组是不同的类型。

**变更原因：**
 
ArkTS1.2中数组和元组是不同的类型。运行时使用元组类型可以获得更好的性能。

**适配建议：**

不要使用数组类型标注元组，而应正确使用对象类型。

**示例：**

**ArkTS1.1**

```typescript
const tuple: [number, number, boolean] = [1, 3.14, true];
const array: (number|boolean) [] = tuple;

const tuple: Array<number | boolean> = [1, 3.14, true];  // 违反规则

function getTuple(): (number | boolean)[] {  // 违反规则
  return [1, 3.14, true];
}
getTuple([1, 3.14, true]);  // 传入元组

type Point = (number | boolean)[];  // 违反规则
const p: Point = [3, 5, true];
```

**ArkTS1.2**

```typescript
const tuple: [number, number, boolean] = [1, 3.14, true];
const array:  [number, number, boolean] = tuple;

const tuple: [number, number, boolean] = [1, 3.14, true];  // 正确使用元组

function getTuple(): [number, number, boolean] {  // 正确使用元组
  return [1, 3.14, true];
}
getTuple([1, 3.14, true]);

type Point = [number, number, boolean];  // 使用元组
const p: Point = [3, 5, true];
```