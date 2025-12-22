## No Duplicate Function Names in Namespaces

**Rule:** `arkts-no-duplicate-function-name`

**Severity: error**

Since ArkTS1.2 merges multiple namespaces with the same name into a single namespace, function names within a namespace cannot be duplicated to avoid conflicts.

**ArkTS1.1**

```typescript
namespace A {
  export function foo() {  // Error: Duplicate function 'foo' in namespace 'A'.
    console.log('test1');
  }
}

namespace A {
  export function foo() {  // Error: Duplicate function 'foo' in namespace 'A'.
    console.log('test2');
  }
}

```

**ArkTS1.2**

```typescript
namespace A {
  export function foo1() {  // Rename exported function
    console.log('test1');
  }
}

namespace A {
  export function foo2() {
    console.log('test2');
  }
}
```