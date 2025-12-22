## Structural Typing Not Supported

**Rule:** `arkts-no-structural-typing`

**Severity: error**

ArkTS1.2 does not support structural typing. The compiler cannot compare the public APIs of two types to determine if they are the same. Use other mechanisms like inheritance, interfaces, or type aliases.

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

let b: C<B> = new C<A>(); // Violates the rule
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
let b: C<A> = new C<A>(); // Use the same generic type

```