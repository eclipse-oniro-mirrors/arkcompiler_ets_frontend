## 共享函数不添加装饰器@Concurrent

**规则：** arkts-limited-stdlib-no-concurrent-decorator

**级别：** error

新增对象天然共享特性，不再依赖Concurrent特性，无需添加@Concurrent装饰器。

**ArkTS1.1**
```typescript
@Concurrent
function func() {}
```

**ArkTS1.2**
```typescript
function func() {}
```
