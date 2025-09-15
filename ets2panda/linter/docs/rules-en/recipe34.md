## Generic Instances Require Type Arguments

**Rule:** `arkts-no-inferred-generic-params`

**Severity: error**

In ArkTS1.2, type arguments are required when creating generic instances.

**ArkTS1.1**

```typescript
new Array() // Violates the rule

new Map(); // Violates the rule

class Box<T> {
  value: T;
  constructor(value: T) {
    this.value = value;
  }
}

let box = new Box(42); // Violates the rule
```

**ArkTS1.2**

```typescript
new Array<SomeType>() // Specify the type

new Map<string, number>(); // Explicitly specify key-value types

let box = new Box<number>(42); // Explicitly specify the type
```