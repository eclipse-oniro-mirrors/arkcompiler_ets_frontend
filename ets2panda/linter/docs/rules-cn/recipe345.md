# `thisArg` 参数的移除

## 规则概述

`arkts-builtin-thisArgs` 规则用于规范 ArkTS 从动态版本（ArkTS-Dyn）迁移到静态版本（ArkTS-Sta）时，内置对象回调方法中 `thisArg` 参数的移除。

## 核心变更

在 ArkTS-Dyn 中，内置对象（Array、TypedArray、Map、Set）的回调方法支持可选的 `thisArg` 参数，用于在执行回调函数时指定 `this` 值。在 ArkTS-Sta 中，该参数被移除。

## 变更范围

### 受影响的类型

| 类型 | 受影响的方法 |
|------|-------------|
| Array / ReadonlyArray | every, filter, find, findIndex, forEach, map, some, from (静态) |
| TypedArray (所有类型) | every, filter, find, findIndex, forEach, map, some |
| Map / ReadonlyMap | forEach |
| Set / ReadonlySet | forEach |

### 方法签名对比

| 方法 | ArkTS-Dyn 签名 | ArkTS-Sta 签名 |
|------|---------------|----------------|
| every | `every(predicate: ..., thisArg?: any): boolean` | `every(predicate: ...): boolean` |
| filter | `filter(predicate: ..., thisArg?: any): T[]` | `filter(predicate: ...): T[]` |
| find | `find(predicate: ..., thisArg?: any): T \| undefined` | `find(predicate: ...): T \| undefined` |
| forEach | `forEach(callback: ..., thisArg?: any): void` | `forEach(callback: ...): void` |
| map | `map(callback: ..., thisArg?: any): U[]` | `map(callback: ...): U[]` |
| some | `some(predicate: ..., thisArg?: any): boolean` | `some(predicate: ...): boolean` |

## 代码示例

### ArkTS-Dyn 写法（不支持）

```typescript
class Counter {
  base: number;
  constructor(base: number) {
    this.base = base;
  }
  check(arr: Int32Array): boolean {
    return arr.every(function(value) {
      return value < this.base;
    }, this); // 使用 thisArg
  }
}
```

### ArkTS-Sta 写法（推荐）

```typescript
class Counter {
  base: number;
  constructor(base: number) {
    this.base = base;
  }
  check(arr: Int32Array): boolean {
    // 使用箭头函数捕获 this
    return arr.every((value) => value < this.base);
  }
}
```

## 适配建议

1. **删除 thisArg 参数**：移除方法调用时的最后一个参数
2. **使用箭头函数**：箭头函数自动捕获外层 `this` 上下文
3. **使用闭包变量**：将需要的上下文保存到局部变量中
