# 迭代器访问方式的变更

## 规则概述

`arkts-builtin-symbol-iterator` 规则用于规范 ArkTS 从动态版本（ArkTS-Dyn）迁移到静态版本（ArkTS-Sta）时，`[Symbol.iterator]` 迭代器访问方式的变更。

## 核心变更

在 ArkTS-Dyn 中，可通过 `Reflect.get(obj, Symbol.iterator)` 访问迭代器。在 ArkTS-Sta 中：
- 不再支持通过 `Reflect.get()` 访问 `Symbol.iterator`
- `Symbol.iterator` 静态属性被移除
- 推荐使用 `for...of` 循环直接遍历

## 变更范围

### 受影响的类型

| 类型 | ArkTS-Dyn 签名 | ArkTS-Sta 签名 |
|------|---------------|----------------|
| Array / ReadonlyArray | `[Symbol.iterator](): IterableIterator<T>` | 使用 `for...of` |
| TypedArray (所有类型) | `[Symbol.iterator](): IterableIterator<number>` | 使用 `for...of` |
| Map / ReadonlyMap | `[Symbol.iterator](): IterableIterator<[K, V]>` | 使用 `for...of` |
| Set / ReadonlySet | `[Symbol.iterator](): IterableIterator<T>` | 使用 `for...of` |
| String | `[Symbol.iterator](): IterableIterator<string>` | 使用 `for...of` |
| Iterable | `[Symbol.iterator](): Iterator<T>` | 不支持 |
| IterableIterator | `[Symbol.iterator](): IterableIterator<T>` | 不支持 |
| Symbol | `static readonly iterator: unique symbol` | 不支持 |

## 代码示例

### ArkTS-Dyn 写法（不支持）

```typescript
// 通过 Reflect.get 访问迭代器
let arr = new Int32Array([1, 2, 3]);
let iter = Reflect.get(arr, Symbol.iterator);

// 访问 Symbol.iterator 静态属性
let sym = Symbol.iterator;
```

### ArkTS-Sta 写法（推荐）

```typescript
// 使用 for...of 直接遍历
let arr = new Int32Array([1, 2, 3]);
for (let item of arr) {
  console.info(item);
}

// Map 和 Set
let map = new Map<string, string>();
for (let [key, value] of map) {
  console.info(key, value);
}
```

## 适配建议

1. **使用 for...of 循环**：遍历可迭代对象时，直接使用 `for...of` 语法
2. **不显式访问迭代器**：避免使用 `Reflect.get(obj, Symbol.iterator)`
3. **使用 $_iterator 方法**：如必须访问迭代器，可使用字符串属性 `$_iterator()` 方法（仅部分类型）
