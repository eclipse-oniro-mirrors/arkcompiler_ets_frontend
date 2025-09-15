## TypeScript Decorators Not Supported

**Rule:** `arkts-no-ts-decorators`

**Severity: error**

ArkTS1.2 does not support treating classes as objects or dynamically modifying them via decorators.

**ArkTS1.1**

```typescript
function decorateKlass(target: Object) {
  console.log("decorateKlass")
}

@decorateKlass // Violates the rule
class Person {
    age: number = 12
}
```

**ArkTS1.2**

```typescript
class Person {
    age: number = 12
}

class PersonHelper {
  static createPerson(): Person {
     console.log("decorateKlass")
     return new Person()
  }
}
```