## 子类有参构造函数需要显式定义，且必须调用父类的构造函数

**规则：** `arkts-subclass-must-call-super-constructor-with-args`

**规则解释：**

ArkTS-Sta禁止隐式传递参数，子类有参构造函数需要显式定义，且必须调用父类的构造函数。

**变更原因：**

ArkTS-Dyn在运行时不对函数调用进行检查，并使用arguments机制获取所有参数并传入父类构造函数。

ArkTS-Sta不支持arguments机制，在编译时会对函数参数的个数和类型进行检查，以确保程序的安全性和正确性。子类显式定义有参构造函数，显式调用父类构造函数，可以避免继承二义性问题。

**适配建议：**

按规则声明对应的有参构造函数。

**示例：**

ArkTS-Dyn

```typescript
class A {
  constructor(a: number) {}
}
class B extends A {}       // ArkTS-Sta上编译报错
let b = new B(123);
```

ArkTS-Sta

```typescript
class A {
  constructor(a: number) {}
}
class B extends A {
  constructor(a: number) {
    super(a)
  }
}
let b = new B(123);
```