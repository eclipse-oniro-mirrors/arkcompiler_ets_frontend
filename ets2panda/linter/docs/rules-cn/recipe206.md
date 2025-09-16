## 不支持debugger 

**规则：**`arkts-no-debugger`

**级别：error**

1. 静态类型语言具备编译时检查和强类型约束，调试通常由IDE完成，已具备较强大的调试机制。

2. debugger会侵入式修改源码。

3. debugger语句会被优化，造成行为不一致。

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
