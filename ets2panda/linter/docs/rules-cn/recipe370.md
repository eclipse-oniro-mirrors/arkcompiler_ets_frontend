## 不支持空数组/稀疏数组 

**规则：**`arkts-no-sparse-array`

**级别：error**

1. ArkTS1.2遵循静态类型，空数组需要能根据上下文推导出数组元素的类型，否则会有编译错误。

2. ArkTS1.2的数组是连续存储的，空位（如 [1, , , 2]）会浪费内存。‌

3. ArkTS1.2遵循空值安全，无法使用默认undefined表示空缺。

**ArkTS1.1**

```typescript
let a = []; // ArkTS1.2，编译错误，需要从上下文中推导数组类型
let b = [1, , , 2]; // 不支持数组中的空位
b[1];  // undefined 
```

**ArkTS1.2**

```typescript
let a: number[] = [];  // 支持，ArkTS1.2上可以从上下文推导出类型
let b = [1, undefined, undefined, 2];
```
