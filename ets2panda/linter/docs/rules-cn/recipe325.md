## 默认参数必须放在必选参数之后

**规则：**`arkts-default-args-behind-required-args`

**级别：error**

默认参数放在必选参数之前没有意义，ArkTS1.1上调用该接口时仍须传递每个默认参数。

**ArkTS1.1**

```typescript
function add(left: number = 0, right: number) { 
  return left + right;
}
```

**ArkTS1.2**

```typescript
function add(left: number, right: number) {
  return left + right;
}
```
