## 方法继承/实现参数遵循逆变原则，返回类型遵循协变原则

**规则：**`arkts-method-inherit-rule`

**级别：error**

ArkTS1.2子类方法覆写父类方法，参数类型须遵循逆变原则，可以通过编译时检查保证类型安全，将潜在的运行时错误提前到编译期，避免运行时失败，无需运行时检查，从而提高执行性能。

**逆变/协变：** 用来描述类型转换后的继承关系，如果A、B表示类型，f()表示类型转换，≤表示继承关系（A≤B表示A是由B派生出来的子类），则有：

- f()为逆变时，当A≤B时有f(B)≤f(A)成立。

- f()为协变时，当A≤B时有f(A)≤f(B)成立。

**ArkTS1.1**

```typescript
// ArkTS1.1  
class A {
  a: number = 0;
}
class B {
  b: number = 0;
}

class Base {
  foo(obj: A | B): void {}
}
class Derived extends Base {
  override foo(obj: A): void {      // 可以覆写父类方法，ArkTS1.2编译错误
    console.info(obj.a.toString());
  }
}
```

**ArkTS1.2**

```typescript
// ArkTS1.2
class A {
  a: number = 0;
}
class B {
  b: number = 0;
}

class Base {
  foo(obj: A | B): void {}
}
class Derived extends Base {
  override foo(obj: A | B): void {
    if (obj instanceof A) {
      console.info(obj.a.toString());
    }
  }
}
```
