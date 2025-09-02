## 子类有参构造函数需要显式定义，且必须调用父类的构造函数

**规则：**`arkts-subclass-must-call-super-constructor-with-args`

**级别：error**

1. ArkTS1.1在运行时没有对函数调用的检查，同时利用arguments机制获取所有参数（ArkTS1.2上不支持这个特性）并传入父类构造函数。ArkTS1.2对函数参数的个数和类型会进行编译时检查，确保程序的安全和正确性，因此ArkTS1.2上不支持这种写法。

2. ArkTS1.2支持方法重载，构造函数可能有多个实现体，在ArkTS1.2上支持这个特性会造成子类继承父类时的二义性。

**ArkTS1.1**

```typescript
class A {
  constructor(a: number) {}
}
class B extends A {}                // ArkTS1.2上编译报错
let b = new B(123);
```

**ArkTS1.2**

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
