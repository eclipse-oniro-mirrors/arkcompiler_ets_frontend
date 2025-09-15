## arkts-no-ctor-prop-decls

**Rule:** `arkts-no-ctor-prop-decls`

**Severity: error**

ArkTS1.2 does not support declaring class fields in constructors. Declare these fields in the class instead.

**ArkTS1.1**

```typescript
class A {
  constructor(readonly a: string) {
  }
}

class Base {
  readonly b: string = "base";
}

class A extends Base {
  constructor(override readonly b: string) {  // Violates the rule
    super();
  }
}
```

**ArkTS1.2**

```typescript
class A {
  readonly a: string
  constructor(a: string) {
    this.a = a
  }
}

class Base {
  readonly b: string = "base";
}

class A extends Base {
  override readonly b: string;  // xplicitly declare the field
  constructor(b: string) {
    super();
    this.b = b;
  }
}

```