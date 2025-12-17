## 非十进制bigint字面量

**规则：** `arkts-only-support-decimal-bigint-literal`

**规则解释：**

ArkTS-Sta暂不支持非十进制bigint字面量。

**变更原因：**

语言层面暂不支持。

**适配建议：**

开发者自行替换为BigInt()函数。

**示例：**

ArkTS-Dyn

```typescript
let a1: bigint = 0xBAD3n;  // 十六进制字面量，ArkTS-Sta暂不支持
let a2: bigint = 0o777n;   // 八进制字面量，ArkTS-Sta暂不支持
let a3: bigint = 0b101n;  // 二进制字面量，ArkTS-Sta暂不支持
```

ArkTS-Sta

```typescript
let a1: bigint = BigInt(0xBAD3);
let a2: bigint = BigInt(0o777);
let a3: bigint = BigInt(0b101);
```