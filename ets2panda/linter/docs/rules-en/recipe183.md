## Object Literals Can Only Contain Properties, Not Methods

**Rule:** `arkts-obj-literal-props`

**Severity: error**

ArkTS1.2 does not support defining methods in object literals. In a static language, class methods are shared by all instances and cannot be redefined via object literals.

**ArkTS1.1**

```typescript
class A {
  foo: () => void = () => {}
}

let a: A = {
  foo() { // Violates the rule
    console.log('hello')
  }
}

interface Person {
  sayHello: () => void;
}

let p: Person = {
  sayHello() {  // Violates the rule, incorrect method definition
    console.log('Hi');
  }
};

type Handler = {
  foo(): void; 
};

let handler: Handler = {
  foo() {  // Violates the rule
    console.log("Executing handler");
  }
};
```

**ArkTS1.2**

```typescript
class A {
  foo : () => void = () => {}
}

let a: A = {
  foo: () => {
    console.log('hello')
  }
}

let p: Person = {
  sayHello: () => {  // Use property assignment
    console.log('Hi');
  }
};

type Handler = {
  foo: () => void;  
};

let handler: Handler = {
  foo: () => {  // Correct method definition
    console.log("Executing handler");
  }
};
```