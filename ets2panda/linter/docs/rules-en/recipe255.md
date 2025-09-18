## extends/implements Expressions Not Allowed

**Rule:** `arkts-no-extends-expression`

**Severity: error**

ArkTS1.2 standardizes class inheritance. Classes cannot inherit from expressions.

**ArkTS1.1**

```typescript
class A {
  v: number = 0
}

let a = A;

class B extends a { // Violates the rule
  u: number = 0
}

function getBase() {
  return class {
    w: number = 0;
  };
}

class B extends getBase() { // Violates the rule
  u: number = 0;
}

interface I {
  w: number;
}

let i = I;

class B implements i { // Violates the rule
  w: number = 0;
}

class A {
  v: number = 0;
}

class B extends new A() { // Violates the rule
  u: number = 0;
}
```

**ArkTS1.2**

```typescript
class A {
  v: number = 0
}

class B extends A { // Directly inherit the class
  u: number = 0
}

class Base {
  w: number = 0;
}

class B extends Base { // Directly inherit the class
  u: number = 0;
}

interface I {
  w: number;
}

class B implements I { // Use the interface directly
  w: number = 0;
}

class A {
  v: number = 0;
}

class B extends A { // Directly inherit the class
  u: number = 0;
}
```