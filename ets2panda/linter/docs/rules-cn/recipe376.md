## 数值类型和bigint类型的比较

**规则：**`arkts-numeric-bigint-compare`

**级别：error**

 当前暂不支持数值类型和bigint类型的比较，迁移工具将提示开发者修改源码，不提供自动修复能力。

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
