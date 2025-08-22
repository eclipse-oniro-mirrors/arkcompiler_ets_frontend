## Subclass Parameterized Constructors Must Be Explicitly Defined and Call Parent Constructors

**Rule:** `arkts-subclass-must-call-super-constructor-with-args`

**Severity: error**

1. ArkTS1.1 has no runtime checks for function calls and uses the arguments mechanism (unsupported in ArkTS1.2) to pass all parameters to the parent constructor. ArkTS1.2 enforces compile-time checks on function parameter counts and types for safety and correctness, so this syntax is unsupported.

2. ArkTS1.2 supports method overloading, and constructors may have multiple implementations. Supporting this feature in ArkTS1.2 would create ambiguity during subclass inheritance.

**ArkTS1.1**

```typescript
class A {
  constructor(a: number) {}
}
class B extends A {}                // ArkTS1.2 compile error
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