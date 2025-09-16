## 增加数组越界运行时检查

**规则：**`arkts-runtime-array-check`

**级别：error**

为了保证类型安全，在访问数组元素时，ArkTS1.2会对索引的合法性进行校验。

**ArkTS1.1**

```typescript
let a: number[] = []
a[100] = 5; // 可能越界
```

**ArkTS1.2**

```typescript
let a: number[] = []
if (100 < a.length) {
  a[100] = 5  // a[100]的值为5
}
```
