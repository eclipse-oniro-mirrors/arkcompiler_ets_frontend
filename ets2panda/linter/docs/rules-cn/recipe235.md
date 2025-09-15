## 增强对联合类型属性访问的编译时检查

**规则：** `arkts-common-union-member-access`

**规则解释：**

ArkTS1.2在编译时会对联合类型的同名属性进行编译检查，要求同名属性具有相同的类型。

**变更原因：**

在ArkTS1.2中，对象的结构在编译时确定。为了避免运行时错误，ArkTS1.2在编译时会检查联合类型的同名属性，确保它们具有相同的类型。

**适配建议：**

避免使用联合类型。在使用联合类型时，可以通过as、重载等方式实现单一类型机制。

**示例：**

**ArkTS1.1**

```typescript
class A {
  v: number = 1
}

class B {
  u: string = ''
}

function foo(a: A | B) {
  console.log(a.v) // 违反规则
  console.log(a.u) // 违反规则
}
```

**ArkTS1.2**

```typescript
class A {
  v: number = 1
}

class B {
  u: string = ''
}

function foo(a: A) {
  console.log(a.v)
}

function foo(a: B) {
  console.log(a.u)
}
```