## 不支持通过负数访问数组

**规则：** `arkts-array-index-negative`

**规则解释：**

ArkTS-Sta不支持使用负整数访问数组元素。当数组索引由其他模块或第三方库传递的变量决定时，这些变量的值需要在运行时确定。迁移工具无法判断索引值是否为负，因此会发出警报，请开发者确认索引值是否为负数，并进行相应修改。

**变更原因：**

在ArkTS-Dyn中，使用负数索引访问数组时，实际上是访问属性名为该负数的属性。如果数组不存在此属性，返回值为`undefined`。如果向负数索引写入值，实际上是为数组对象动态增加一个属性名为该负数的属性并赋值。ArkTS-Sta是静态类型语言，无法动态为数组对象增加属性，因此不支持使用负数索引访问数组元素。

**适配建议：**

请使用非负整数来访问数组元素。

**示例：**

ArkTS-Dyn

```typescript
let an_array = [1, 2, 3];
let element = an_array [-1];
console.info(getElement(an_array, -1).toString()); // 违反规则
for (let i: number = -1; i < an_array.length; i++) { // 违反规则
  console.info(an_array[i].toString());
}

function getElement(arr: number[], index: number) {
  return arr[index]; // 可能接收负数索引
}
```

ArkTS-Sta

```typescript
let an_array = [1, 2, 3];
let element = an_array [1];
console.info(getElement(an_array, 1)); // 传递非负索引
for (let i: int = 0; i < an_array.length; i++) { // 仅允许非负索引
  console.info(an_array[i]);
}

function getElement(arr: number[], index: int) {
  if (index < 0) throw new Error("Index must be a non-negative integer");
  return arr[index]; // 仅允许非负整数
}
```