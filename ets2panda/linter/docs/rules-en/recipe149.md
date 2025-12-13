## Classes Cannot Be Used as Objects

**Rule:** `arkts-no-classes-as-obj`

**Severity: error**

In ArkTS, a class declares a new type, not a value. Therefore, classes cannot be used as objects (e.g., assigned to variables).

**ArkTS1.1**

```typescript
class MyClass {
  constructor(public name: string) {}
}

let obj = MyClass; // Violates the rule
```

**ArkTS1.2**

```typescript
class MyClass {
  constructor(name: string) {}
}

// Reflection is needed
let className = "path.to.MyClass";
let linker = Class.ofCaller()!.getLinker();
let classType: ClassType | undefined = linker.getType(className) as ClassType;
```