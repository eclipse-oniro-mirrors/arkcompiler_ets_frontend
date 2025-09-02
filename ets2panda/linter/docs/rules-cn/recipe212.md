## 不支持通过负数访问数组

**规则：**`arkts-array-index-negative`

**级别：error**

ArkTS1.2不支持使用负整数访问数组元素。

**ArkTS1.1**

```typescript
let an_array = [1, 2, 3];
let element = an_array [-1];
console.log(getElement(an_array, -1)); // 违反规则
for (let i: int = -1; i < an_array.length; i++) { // 违反规则
  console.log(an_array[i]);
}

function getElement(arr: number[], index: int) {
  return arr[index]; // 可能接收负数索引
}
```

**ArkTS1.2**

```typescript
let an_array = [1, 2, 3];
let element = an_array [1];
console.log(getElement(an_array, 1)); // 传递非负索引
for (let i: int = 0; i < an_array.length; i++) { // 仅允许非负索引
  console.log(an_array[i]);
}

function getElement(arr: number[], index: int) {
  if (index < 0) throw new Error("Index must be a non-negative integer");
  return arr[index]; // 仅允许非负整数
}
```
