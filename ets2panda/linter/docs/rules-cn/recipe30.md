## 不支持structural typing

**规则：**`arkts-no-structural-typing`

**级别：error**

ArkTS1.2不支持structural typing，编译器无法比较两种类型的publicAPI并决定它们是否相同。使用其他机制，例如继承、接口或类型别名。

**ArkTS1.1**

```typescript
// case1
class A {
  v: number = 0
}

class B {
  v: number = 0
}

let a = new B() as A

// case2
class C<T> {
  u: T
}

let b: C<B> = new C<A>()

// case3
class A {
  u: number = 0
}

class B {
  u: number = 0
}

(): A => { return new B() }

class A {
  v: number = 0
}

class B {
  v: number = 0
}
class C<T> {
  u: T;
}

let b: C<B> = new C<A>(); // 违反规则
```

**ArkTS1.2**

```typescript
// case1
class A {
  v: number = 0
}

class B {
  v: number = 0
}

let a = new B()

// case2
class C<T> {
  u: T
}

let b: C<A> = new C<A>()

// case3
class A {
  u: number = 0
}

class B {
  u: number = 0
}

(): B => { return new B() }

class A {
  v: number = 0
}

class B {
  v: number = 0
}
let b: C<A> = new C<A>(); // 使用相同的泛型类型

```
