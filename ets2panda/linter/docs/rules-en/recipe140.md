## Function.bind Method Not Supported

**Rule:** `arkts-no-func-bind`

**Severity: error**

ArkTS does not allow the use of the standard library function Function.bind. The standard library uses these functions to explicitly set the this parameter of the called function.

**ArkTS1.1**

```typescript
class MyClass {
  constructor(public name: string) {}

  greet() {
    console.log(`Hello, my name is ${this.name}`);
  }
}

const instance = new MyClass("Alice");
const boundGreet = instance.greet.bind(instance); // Violates the rule, Function.bind not allowed
boundGreet();
```

**ArkTS1.2**

```typescript
class MyClass {
  constructor(public name: string) {}

  greet() {
    console.log(`Hello, my name is ${this.name}`);
  }
}

const instance = new MyClass("Alice");
const boundGreet = () => instance.greet(); // Use arrow functions
boundGreet(); // Hello, my name is Alice
```