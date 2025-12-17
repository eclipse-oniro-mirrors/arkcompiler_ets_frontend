## 类的静态属性需要有初始值

**规则：** `arkts-class-static-initialization`

**规则解释：**

在ArkTS-Sta中，为了遵循null-safety（空安全），需要为属性赋上初始值。

**变更原因：**

ArkTS-Sta遵循null-safety（空安全），需要为类的静态属性赋初始值（具有默认值的类型除外）。

**适配建议：**

为静态属性赋初始值。

**示例：**

ArkTS-Dyn

```typescript
class O {
}

class A {
  static o: O;
}

class B {
  static count: number;
}

interface IConfig {
  theme: string;
}

class C {
  static config: IConfig;
}

class D {
  static msg: string;

  constructor() {
    D.msg = "default";
  }
}
```

ArkTS-Sta

```typescript
class O {
}

class A {
  static o: O = new O(); // 提供初始值
}

class B {
  static count: number = 1; // 提供初始值
}

interface IConfig {
  theme: string;
}

class C {
  static config: IConfig = { theme: "light" }; // 提供初始值
}

class D {
  static msg: string = "default"; // 在定义时初始化
}
```