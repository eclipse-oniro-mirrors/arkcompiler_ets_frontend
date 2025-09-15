## Class Methods Cannot Override Interface Fields

**Rule:** `arkts-no-method-overriding-field`

**Severity: error**

ArkTS1.2 does not support structural typing, so properties and methods cannot be interchanged.

**ArkTS1.1**

```typescript
interface Person {
  cb: () => void
}

class student implements Person{
  cb() {}
} 

interface Transformer<T> {
  transform: (value: T) => T; // Violates the rule
}

class StringTransformer implements Transformer<string> {
  transform(value: string) { return value.toUpperCase(); }  // Violates the rule
}
```

**ArkTS1.2**

```typescript
interface Person {
  cb(): void
}

class student implements Person{
  cb() {}
}

interface Transformer<T> {
  transform(value: T): T;  // Change to a method
}

class StringTransformer implements Transformer<string> {
  transform(value: string) { return value.toUpperCase(); }  // Correct
}
```