# 构造函数（Constructor）调用方式的变更

## 规则概述

`arkts-builtin-cotr` 规则用于规范 ArkTS 从动态版本（ArkTS-Dyn）迁移到静态版本（ArkTS-Sta）时，内置构造函数（Constructor）调用方式的变更。

## 核心变更

在 ArkTS-Dyn 中，许多内置构造函数可以作为 `Constructor` 类型被传递和调用。在 ArkTS-Sta 中，这些构造函数变更为静态 `invoke` 方法，不再支持通过 `Constructor` 类型直接调用。

## 变更范围

### 基础类型构造函数

| 构造函数 | ArkTS-Dyn 签名 | ArkTS-Sta 签名 |
|---------|---------------|----------------|
| BigInt | `(value: bigint\|boolean\|number\|string): bigint` | `static BigInt.invoke(value: ...): bigint` |
| Boolean | `<T>(value?: T): boolean` | `static Boolean.invoke<T>(value?: T): boolean` |
| Number | `(value?: any): number` | `static Number.invoke(value?: String\|Number\|BigInt): number` |
| String | `(value?: any): string` | `static String.invoke(value?: Object\|undefined\|null): String` |

### 对象类型构造函数

| 构造函数 | ArkTS-Dyn 签名 | ArkTS-Sta 签名 |
|---------|---------------|----------------|
| Date | `(): string` | `static Date.invoke(): string` |
| Array (可选数字) | `<T>(arrayLength?: number): T[]` | `static Array.invoke(arrayLength?: number): T[]` |
| Array (剩余参数) | `<T>(...items: T[]): T[]` | `static Array.invoke(...items: T[]): Array<T>` |
| RegExp | `(pattern: RegExp\|string, flags?: string): RegExp` | `static RegExp.invoke(pattern: ..., flags?: ...): RegExp` |

### 错误类型构造函数

| 构造函数 | ArkTS-Dyn 签名 | ArkTS-Sta 签名 |
|---------|---------------|----------------|
| Error | `(message?: string): Error` | `static Error.invoke(message?: string): Error` |
| EvalError | `(message?: string): EvalError` | `static EvalError.invoke(message?: string): EvalError` |
| RangeError | `(message?: string): RangeError` | `static RangeError.invoke(message?: string): RangeError` |
| ReferenceError | `(message?: string): ReferenceError` | `static ReferenceError.invoke(message?: string): ReferenceError` |
| SyntaxError | `(message?: string): SyntaxError` | `static SyntaxError.invoke(message?: string): SyntaxError` |
| TypeError | `(message?: string): TypeError` | `static TypeError.invoke(message?: string): TypeError` |
| URIError | `(message?: string): URIError` | `static URIError.invoke(message?: string): URIError` |
| AggregateError | `(errors: Iterable<any>, ...): AggregateError` | `static AggregateError.invoke(errors: Iterable<Error>, ...): AggregateError` |

## 代码示例

### ArkTS-Dyn 写法（不支持）

```typescript
// 通过 Constructor 类型调用
function createBigInt(ctor: BigIntConstructor) {
  return ctor(1);
}

function createBoolean(ctor: BooleanConstructor) {
  return ctor(true);
}

function createArray(ctor: ArrayConstructor) {
  return ctor<number>(1, 2, 3);
}

function createError(ctor: ErrorConstructor) {
  return ctor("some error");
}
```

### ArkTS-Sta 写法（推荐）

```typescript
// 直接调用构造函数（自动转换为 invoke）
function createBigInt() {
  return BigInt(1);
}

function createBoolean() {
  return Boolean(true);
}

function createArray() {
  return Array<number>(1, 2, 3);
}

function createError() {
  return Error("some error");
}

// 或使用 new 关键字
let date = new Date();
let regex = new RegExp("pattern", "i");
```

## 适配建议

1. **不要使用 Constructor 类型**：避免将构造函数作为 `Constructor` 类型参数传递
2. **直接调用构造函数**：在 ArkTS-Sta 中，直接调用构造函数会自动转换为 `invoke` 方法调用
3. **使用 new 关键字**：对于支持 `new` 的类型，使用 `new` 创建对象
4. **注意类型参数变化**：`Number` 和 `String` 的 `invoke` 方法参数类型更严格，不再是 `any`
