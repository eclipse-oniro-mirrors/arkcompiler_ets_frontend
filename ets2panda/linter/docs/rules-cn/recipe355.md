## 共享对象不添加装饰器@Sendable

**规则：** arkts-limited-stdlib-no-sendable-decorator

**级别：** error

新增对象天然共享特性，不再依赖Sendable特性，无需添加@Sendable装饰器。

**ArkTS1.1**
```typescript
@Sendable
class A {}
```

**ArkTS1.2**
```typescript
class A {}
```
