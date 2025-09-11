## 数值类型和bigint类型的比较

**规则：** `arkts-numeric-bigint-compare`

**规则解释：**

ArkTS1.2暂不支持数值类型和bigint类型的比较。

**变更原因：**

语言层面暂不支持。

**适配建议：**

开发者需将值转换为BigInt类型再进行比较。

**示例：**

**ArkTS1.1**

```typescript
let n1: number = 123;
let n2: bigint = 456n;

n1 <= n2;   // 编译通过
n1 == n2;   // 编译失败
n1 >= n2;   // 编译通过
```

**ArkTS1.2**

```typescript
let n1: number = 123;
let n2: bigint = 456n;

BigInt(n1) <= n2;
BigInt(n1) == n2;
BigInt(n1) >= n2;
```