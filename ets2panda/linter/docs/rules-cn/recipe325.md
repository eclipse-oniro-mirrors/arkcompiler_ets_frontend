## 默认参数必须放在必选参数之后

**规则：** `arkts-default-args-behind-required-args`

**规则解释：**

在ArkTS-Sta中，函数、方法及lambda表达式的默认参数必须放在必选参数之后。

**变更原因：**

将默认参数置于必选参数前没有实际意义，开发者仍需为每个默认参数提供值。

**适配建议：**

默认参数放在必选参数之后。

**示例：**

ArkTS-Dyn

```typescript
function add(left: number = 0, right: number) { 
  return left + right;
}
```

ArkTS-Sta

```typescript
function add(left: number, right: number = 0) {
  return left + right;
}
```