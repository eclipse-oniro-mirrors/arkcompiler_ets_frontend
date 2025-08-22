## 不支持在constructor中声明字段

**规则：** `arkts-no-ctor-prop-decls`

**规则解释：**

ArkTS1.2不支持在constructor中声明类字段。

**变更原因：**

ArkTS1.2在编译期确定类型布局，运行期不允许修改，以提高性能。

**适配建议：**

改为在class中声明字段。

**示例：**

**ArkTS1.1**

```typescript
class A {
  constructor(readonly a: string) {
  }
}

class Base {
  readonly b: string = "base";
}

class A extends Base {
  constructor(override readonly b: string) {  // 违反规则
    super();
  }
}
```

**ArkTS1.2**

```typescript
class A {
  readonly a: string
  constructor(a: string) {
    this.a = a
  }
}

class Base {
  readonly b: string = "base";
}

class A extends Base {
  override readonly b: string;  // 显式声明字段
  constructor(b: string) {
    super();
    this.b = b;
  }
}

```