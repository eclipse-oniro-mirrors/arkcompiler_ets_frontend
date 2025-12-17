## 不支持在constructor中声明字段

**规则：** `arkts-no-ctor-prop-decls`

**规则解释：**

ArkTS-Sta不支持在constructor中声明类字段。

**变更原因：**

ArkTS-Sta在编译期确定类型布局，运行期不允许修改，以提高性能。

**适配建议：**

改为在class中声明字段。

**示例：**

ArkTS-Dyn

```typescript
class A {
  constructor(readonly a: string) {
  }
}
```

ArkTS-Sta

```typescript
class A {
  readonly a: string;
  constructor(a: string) {
    this.a = a;
  }
}
```