# arkts-builtin-api-num2int

## 规则说明

部分内置 API 的参数或返回值在 ArkTS-Sta 中具有明确的整数语义。迁移时，如果相关变量仍声明为 `number`，可能导致类型约束不够精确。应将只按整数语义使用的 `number` 变量声明为 `int`。

## 违规示例

```ts
let index: number = 1;
let text = 'abc';
text.charAt(index);
```

## 修复建议

```ts
let index: int = 1;
let text = 'abc';
text.charAt(index);
```

对于 builtin API 返回值，如果返回值只按整数语义继续使用，也应补充 `int` 类型声明。
