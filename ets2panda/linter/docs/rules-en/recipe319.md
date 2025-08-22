## Method Inheritance/Implementation Parameters Follow Contravariance, Return Types Follow Covariance

**Rule:** `arkts-method-inherit-rule`

**Severity: error**

In ArkTS1.2, when a subclass method overrides a parent class method, parameter types must follow contravariance rules. Compile-time checks ensure type safety, catching potential runtime errors early and avoiding runtime failures, eliminating the need for runtime checks and improving performance.

**Contravariance/Covariance：** Describes inheritance relationships after type conversion. If A and B represent types, f() represents type conversion, and ≤ represents inheritance (A ≤ B means A is a subclass derived from B), then:

- For contravariance, if A ≤ B, then f(B) ≤ f(A) holds.

- For covariance, if A ≤ B, then f(A) ≤ f(B) holds.

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
  override foo(obj: A): void {      // Can override parent method, ArkTS1.2 compile error
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