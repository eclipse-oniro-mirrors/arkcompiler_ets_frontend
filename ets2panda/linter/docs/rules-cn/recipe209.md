## 数组索引必须是整型数据

**规则：** `arkts-array-index-expr-type`

**规则解释：**

数组索引必须为整数类型。当索引由其他模块或第三方库传递时，迁移工具可能无法解析其类型，导致数组索引处报错。请开发者确认变量类型是否为整数，并决定如何修改代码。

**变更原因：**

为了实现数组更快的访问，ArkTS-Sta支持数值类型的细化，并要求数组索引表达式必须是整数类型。
 
**适配建议：**

请将索引改为整数类型。

**示例：**

ArkTS-Dyn

```typescript
function foo(index: number) {
  let array = [1, 2, 3];
  let element = array[index];
}

function getIndex(): number {
  return Math.random() * 10; // 可能返回小数
}

let array = [1, 2, 3];
for (let i: number = 0; i < array.length; i++) {
  console.info(array[i].toString());
}
```

ArkTS-Sta

```typescript
function foo(index: int) {
  let array = [1, 2, 3];
  let element = array[index];
}

function getIndex(): int {
  return Math.floor(Math.random() * 10).toInt(); // 转换为 `int`
}

let array = [1, 2, 3];
for (let i: int = 0; i < array.length; i++) { // 改为 `int`
  console.info(array[i]);
}
```