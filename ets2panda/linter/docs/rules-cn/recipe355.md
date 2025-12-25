## 共享对象不添加装饰器@Sendable

**规则：** `arkts-limited-stdlib-no-sendable-decorator`

**规则解释：**

ArkTS-Sta新增对象天然共享特性，无需添加@Sendable装饰器。

**变更原因：**

ArkTS-Sta新增了对象天然共享特性，不再依赖Sendable特性，无需再添加@Sendable装饰器。

**适配建议：**

删掉共享对象中的@Sendable装饰器。

**示例：**

ArkTS-Dyn

```typescript
@Sendable
class A {}
```

ArkTS-Sta

```typescript
class A {}
```