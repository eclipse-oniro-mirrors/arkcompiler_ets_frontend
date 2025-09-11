## 数组/元组类型在继承关系中遵循不变性原则

**规则：** `arkts-array-type-immutable`

**规则解释：**

在ArkTS1.2中，数组在继承关系中遵循不变性原则，会通过编译时检查保证类型安全。

**变更原因：**

在ArkTS1.2中，数组在继承关系中遵循不变性原则，编译时检查确保类型安全，将潜在的运行时错误提前到编译期，避免运行时失败，提高执行性能。

**适配建议：**

避免将不同类型的数组互相赋值。

**示例：**

**ArkTS1.1**

```typescript
class A {
  a: number = 0;
}

class B {
  b: number = 0;
}

// ArkTS1.1 
let arr1: A[] = [new A()];
let arr2: (A | B)[] = arr1;   // ArkTS1.2编译错误
```

**ArkTS1.2**

```typescript
class A {
  a: number = 0;
}

class B {
  b: number = 0;
}

// ArkTS1.2 
let arr1: [ A | B ] = [new A()];
let arr2: [ A | B ] = arr1;  // 需要相同类型的元组
```