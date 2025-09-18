## Function Types

**Rule:** `arkts-incompatible-function-types`

**Severity: error**

TypeScript allows more flexible assignments for function-type variables, while ArkTS1.2 enforces stricter checks. For function type conversions, parameters follow contravariance rules, and return types follow covariance rules.

**ArkTS1.1**

```typescript
type FuncType = (p: string) => void;
let f1: FuncType =
    (p: string): number => {
        return 0
    }
let f2: FuncType = (p: any): void => {};

class Animal {}
class Dog extends Animal {}
type FuncType = () => Animal;
let f: FuncType = (): Dog => new Dog(); // Allowed in TypeScript but not in ArkTS

type FuncType2 = (dog: Dog) => void;
let f: FuncType2 = (animal: Animal) => {}; // Violates the rule
```

**ArkTS1.2**

```typescript
type FuncType = (p: string) => void
let f1: FuncType =
  	(p: string) => {
        ((p: string): number => {
            return 0
        })(p) 
    }
let f2: FuncType = (p: string): void => {};

class Animal {}
class Dog extends Animal {}
type FuncType = () => Animal;
let f: FuncType = (): Animal => new Animal();// Return `Animal`

type FuncType2 = (dog: Dog) => void;
let f: FuncType = (dog: Dog) => {}; // Strict matching of parameter types
```