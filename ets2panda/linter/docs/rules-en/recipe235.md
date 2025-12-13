## Enhanced Compile-Time Checks for Union Type Property Access

**Rule:** `arkts-common-union-member-access`

**Severity: error**

In ArkTS1.2, object structures are determined at compile time. To avoid runtime errors when accessing properties of union types, compile-time checks are enforced, requiring properties with the same name to have the same type.

**ArkTS1.1**

```typescript
class A {
  v: number = 1
}

class B {
  u: string = ''
}

function foo(a: A | B) {
  console.log(a.v) // Violates the rule
  console.log(a.u) // Violates the rule
}
```

**ArkTS1.2**

```typescript
class A {
  v: number = 1
}

class B {
  u: string = ''
}

function foo(a: A) {
  console.log(a.v)
}

function foo(a: B) {
  console.log(a.u)
}
```