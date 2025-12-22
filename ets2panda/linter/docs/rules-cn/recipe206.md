## 不支持debugger 

**规则：** `arkts-no-debugger`

**规则解释：**

不支持debugger语句。

**变更原因：**

1. 静态类型语言具备编译时检查和强类型约束，DevEco Studio已具备完善的调试机制。

2. 使用debugger会侵入式地修改源码。

3. debugger语句会被优化，可能导致行为不一致。

**适配建议：**

使用DevEco Studio断点调试代替debugger语句。

**示例：**

**ArkTS1.1**

```typescript
// ArkTS1.1 
// ...
debugger;
// ...
```

**ArkTS1.2**

```typescript
// ArkTS1.2   移除debugger语句
// ...
```