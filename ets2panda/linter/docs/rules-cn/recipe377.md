## 非十进制bigint字面量

**规则：**`arkts-only-support-decimal-bigint-literal`

**级别：error**

 当前暂不支持非十进制bigint字面量，通过迁移工具提示开发者修改源码，不提供自动修复能力。

**ArkTS1.1**

```typescript
let a1: bigint = 0xBAD3n;  // 十六进制字面量，ArkTS1.2暂不支持
let a2: bigint = 0o777n;   // 八进制字面量，ArkTS1.2暂不支持
let a3: bigint = 0b101n;  // 二进制字面量，ArkTS1.2暂不支持
```

**ArkTS1.2**

```typescript
let a1: bigint = BigInt(0xBAD3);
let a2: bigint = BigInt(0o777);
let a3: bigint = BigInt(0b101);
```
