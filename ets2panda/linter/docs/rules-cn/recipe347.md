# 属性描述符相关属性和方法的移除

## 规则概述

`arkts-builtin-no-property-descriptor` 规则用于规范 ArkTS 从动态版本（ArkTS-Dyn）迁移到静态版本（ArkTS-Sta）时，属性描述符相关属性和方法的移除。

## 核心变更

在 ArkTS-Dyn 中，可使用 `PropertyDescriptor` 和 `TypedPropertyDescriptor` 动态定义属性特性。在 ArkTS-Sta 中，作为静态语言，不再支持运行时修改属性特性。

## 变更范围

### PropertyDescriptor 属性移除

| 属性 | ArkTS-Dyn 签名 | ArkTS-Sta 签名 |
|------|---------------|----------------|
| configurable | `configurable?: boolean` | 不支持 |
| enumerable | `enumerable?: boolean` | 不支持 |
| value | `value?: any` | 不支持 |
| writable | `writable?: boolean` | 不支持 |
| get | `get?(): any` | 不支持 |
| set | `set?(v: any): void` | 不支持 |

### TypedPropertyDescriptor 属性移除

| 属性 | ArkTS-Dyn 签名 | ArkTS-Sta 签名 |
|------|---------------|----------------|
| configurable | `configurable?: boolean` | 不支持 |
| enumerable | `enumerable?: boolean` | 不支持 |
| value | `value?: T` | 不支持 |
| writable | `writable?: boolean` | 不支持 |
| get | `get?: () => T` | 不支持 |
| set | `set?(value: T): void` | 不支持 |

### AggregateError.errors 类型变更

| 属性 | ArkTS-Dyn 类型 | ArkTS-Sta 类型 |
|------|---------------|----------------|
| errors | `any[]` | `Error[]` |

## 代码示例

### ArkTS-Dyn 写法（不支持）

```typescript
// 使用 PropertyDescriptor
Object.defineProperty(obj, "prop", {
  configurable: true,
  enumerable: false,
  value: 1,
  writable: false,
  get() { return this._value; },
  set(v) { this._value = v; }
});

// AggregateError 使用任意类型
let err = new AggregateError([1, 2, "error"]);
```

### ArkTS-Sta 写法（推荐）

```typescript
// 使用类定义替代属性描述符
class MyClass {
  private _value: number = 0;

  prop: number = 1;

  getValue(): number {
    return this._value;
  }

  setValue(v: number): void {
    this._value = v;
  }
}

// AggregateError 仅接受 Error 类型
let err = new AggregateError([
  Error("first error"),
  Error("second error")
]);
```

## 适配建议

1. **使用类定义**：通过定义类及其属性、方法来替代属性描述符
2. **使用访问器方法**：用 `get`/`set` 方法替代 `get`/`set` 属性描述符
3. **使用 readonly 属性**：用 `readonly` 替代 `writable: false`
4. **仅使用 Error 类型**：`AggregateError` 构造函数的第一个参数应仅为 `Error` 类型的可迭代对象
