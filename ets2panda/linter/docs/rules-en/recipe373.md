## Arrays/Tuples Follow Invariance in Inheritance

**Rule:** `arkts-array-type-immutable`

**Severity: error**

In ArkTS1.2, arrays follow invariance in inheritance relationships. Compile-time checks ensure type safety, catching potential runtime errors early and avoiding runtime failures, thereby improving performance.

**ArkTS1.1**

```typescript
class A {
  a: number = 0;
}

class B {
  b: number = 0;
}

// ArkTS1.1 
let arr1: A[] = [new A()];
let arr2: (A | B)[] = arr1;      // ArkTS1.2 compile error
```

**ArkTS1.2**

```typescript
class A {
  a: number = 0;
}

class B {
  b: number = 0;
}

// ArkTS1.2 
let arr1: [ A | B ] = [new A()];
let arr2: [ A | B ] = arr1;       // Requires tuples of the same type
```