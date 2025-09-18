## 不支持Structural Typing

**规则：** `arkts-no-structural-typing`

**规则解释：**
 
ArkTS1.2不支持Structural Typing。

Structural Typing（结构化类型系统）是一种类型系统，其类型兼容性基于类型的实际结构而非声明名称。例如，两个类的属性和方法完全相同，即使名称不同，也被认为是同一个类型，可以互相赋值。

**变更原因：**
 
Structural Typing存在以下劣势，故ArkTS1.2不支持。
- 意外匹配风险：结构相同但语义不同的类型可能被误用。
- 重构风险：修改结构可能影响远处代码。
- 可读性降低：类型关系不直观。

**适配建议：**

NA

**示例：**

**ArkTS1.1**

```typescript
// 类型定义
class A {
  v: number = 0
}
class B {
  v: number = 0
}
class C<T> {
  u?: T
}
// 场景1，类型转换
let a = new B() as A
// 场景2，泛型
let b: C<B> = new C<A>()
// 场景3，返回类型
let func = (): A => {
  return new B()
}
```

**ArkTS1.2**
```typescript
NA
```