## 数组索引必须是整型数据

**规则：**`arkts-array-index-expr-type`

**级别：error**

ArkTS1.2支持数值类型的细化，为了实现数组更快的访问，数组索引表达式必须是整数类型。

**ArkTS1.1**

```typescript
function foo (index: number) {
  let array = [1, 2, 3] 
  let element = array[index]
}

function getIndex(): number {
  return Math.random() * 10; // 可能返回小数
}

let array = [1, 2, 3];
for (let i: number = 0; i < array.length; i++) { // 违反规则
  console.log(array[i]);
}
```

**ArkTS1.2**

```typescript
function foo (index: int) {
  let array = [1, 2, 3] 
  let element = array[index]
}

function getIndex(): int {
  return Math.floor(Math.random() * 10);  // 转换为 `int`
}

let array = [1, 2, 3];
for (let i: int = 0; i < array.length; i++) { // 改为 `int`
  console.log(array[i]);
}
```
