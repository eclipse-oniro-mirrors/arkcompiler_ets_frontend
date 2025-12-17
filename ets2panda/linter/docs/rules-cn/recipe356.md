## 共享函数不添加装饰器@Concurrent

**规则：** `arkts-limited-stdlib-no-concurrent-decorator`

**规则解释：**

ArkTS-Sta新增函数对象天然共享特性，无需添加@Concurrent装饰器。

**变更原因：**

ArkTS-Sta新增了对象天然共享特性，不再依赖Concurrent特性，无需再为共享函数添加@Concurrent装饰器。

**适配建议：**

删掉共享函数中的@Concurrent装饰器。

**示例：**

ArkTS-Dyn

```typescript
@Concurrent
function func() {}
```

ArkTS-Sta

```typescript
function func() {}
```