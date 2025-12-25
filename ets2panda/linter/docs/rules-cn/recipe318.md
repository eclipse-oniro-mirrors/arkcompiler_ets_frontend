## 共享函数不需要use concurrent修饰

**规则：** `arkts-limited-stdlib-no-use-concurrent`

**规则解释：**

ArkTS-Sta新增对象天然共享特性，无需添加use concurrent。

**变更原因：**

ArkTS-Sta新增对象天然共享特性，函数默认支持跨线程安全共享，无需再使用use concurrent显示声明。

**适配建议：**

删除use concurrent显示声明。

**示例：**

ArkTS-Dyn

```typescript
function func() {
'use concurrent' 
}
```

ArkTS-Sta

```typescript
function func() {}
```