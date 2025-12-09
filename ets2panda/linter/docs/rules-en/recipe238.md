## Class Static Properties Must Have Initial Values

**Rule:** `arkts-class-static-initialization`

**Severity: error**

ArkTS1.2 follows null-safety principles, requiring properties to be initialized.

**ArkTS1.1**

```typescript
class B {}

class A {
  static b: B
}

class A {
  static count: number; // Violates the rule, must be initialized
}

class A {
  static config: { theme: string }; // Violates the rule, must be initialized
}

class A {
  static name: string;

  constructor() {
    A.name = "default"; // Violates the rule, static properties must be initialized at definition
  }
}
```

**ArkTS1.2**

```typescript
class B {}

class A {
  static b? : B
  static b: B | undefined = undefined
}

class A {
  static count: number = 0; // Provide initial value
}

class A {
  static config: { theme: string } = { theme: "light" }; // Provide initial value
}

class A {
  static name: string = "default"; // Initialize at definition
}

```