## 类的静态属性需要有初始值

**规则：** `arkts-class-static-initialization`

**规则解释：**

在ArkTS1.2中，为了遵循null-safety（空安全），需要为属性赋上初始值。

**变更原因：**

ArkTS1.2遵循null-safety（空安全），需要为类的静态属性赋初始值（具有默认值的类型除外）。

**适配建议：**

为静态属性赋初始值。

**示例：**

**ArkTS1.1**

```typescript
class B {}

class A {
  static b: B
}

class A {
  static count: number; // 违反规则，必须初始化
}

class A {
  static config: { theme: string }; // 违反规则，必须初始化
}

class A {
  static name: string;

  constructor() {
    A.name = "default"; // 违反规则，静态属性必须在定义时初始化
  }
}
```

**ArkTS1.2**

```typescript
class B {}

class A {
  static b? : B
  static b: B | undefined = undefined
}

class A {
  static count: number = 0; // 提供初始值
}

class A {
  static config: { theme: string } = { theme: "light" }; // 提供初始值
}

class A {
  static name: string = "default"; // 在定义时初始化
}

```