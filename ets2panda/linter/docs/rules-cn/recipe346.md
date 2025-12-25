### Map-Symbol.iterator变更

**规则：** `arkts-builtin-symbol-iterator`

**ArkTS-Dyn版本签名：**  
  `[Symbol.iterator](): IterableIterator<[K, V]>`

**返回值：**
  | 类型 | 说明 |
  | -------- | -------- |
  | IterableIterator\<[K, V]\> | Map的迭代器。 |

**示例：**  
  ```typescript
  let m: Map<string, string> = new Map<string, string>();
  let iter = Reflect.get(m, Symbol.iterator);
  ```

**ArkTS-Sta版本签名：**  
  `$_iterator(): IterableIterator<[K, V]>`

**返回值：**
  | 类型 | 说明 |
  | -------- | -------- |
  | IterableIterator\<[K, V]\> | Map的迭代器。 |

**示例：**  
  ```typescript
  const m: Map<string, string> = new Map<string, string>();
  // 不建议使用$_iterator()方法，应使用for...of替代
  for (let iter of m) {
    console.info(iter);
  }
  ```

**适配建议：** 
  建议仅使用for...of访问迭代器。
