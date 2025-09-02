## 共享函数不需要use concurrent修饰

**规则：** arkts-limited-stdlib-no-use-concurrent

**级别：** error

新增对象天然共享特性，无需添加use concurrent。

**ArkTS1.1**
```typescript
function func() {
'use concurrent' 
}
```

**ArkTS1.2**
```typescript
function func() {}
```
