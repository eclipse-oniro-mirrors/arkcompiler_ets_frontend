## catch Statements Use error Type

**Rule:** `arkts-no-ts-like-catch-type`

**Severity: error**

In ArkTS1.1, the e in catch statements is of type any, so the compiler does not perform compile-time type checks on exceptions. In ArkTS1.2, when throw is restricted, only Error types can be thrown.

In ArkTS1.2's static mode, types must be explicit, and compatibility with ArkTS1.1 must be considered. For catch(e) syntax, e defaults to the Error type.

**ArkTS1.1**

```typescript
try {
  throw new Error();
} catch(e) {  // `e` is of type `any`
  e.message; // ArkTS1.1 compiles and runs normally
  e.prop;     // ArkTS1.1 compiles, outputs `undefined`
}
```

**ArkTS1.2**

```typescript
try {
  throw new Error();
} catch(e:Error) {  // `e` is of type `Error`
  e.message;   // ArkTS1.2 compiles and runs normally
  e.prop;      // ArkTS1.2 compile error, need to cast `e` to the desired exception type, e.g., `(e as SomeError).prop`
}
```