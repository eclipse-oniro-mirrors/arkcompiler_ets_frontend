## 不支持逻辑赋值运算

**规则：**`arkts-unsupport-operator`

**级别：error**

1. 当前暂不支持&&=, ||=, ??=逻辑赋值运算符，通过迁移工具提示开发者修改源码，不提供自动修复能力。

**ArkTS1.1**

```typescript
let a = 1;
a &&= 2;    // 结果: 2，ArkTS1.2暂不支持
a ||= 3;   // 结果: 2，ArkTS1.2暂不支持
a ??= 4;  // 结果: 2，ArkTS1.2暂不支持
```

**ArkTS1.2**

```typescript
let a = 1;
a = a && 2;   // 结果: 2
a = a || 3;   // 结果: 2
a = a ?? 4;   // 结果: 2
```
