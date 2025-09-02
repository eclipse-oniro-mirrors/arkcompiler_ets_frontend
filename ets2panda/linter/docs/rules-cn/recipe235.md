## 增强对联合类型属性访问的编译时检查

**规则：**`arkts-common-union-member-access`

**级别：error**

在ArkTS1.2中，对象的结构在编译时就确定了。为了避免访问联合类型后出现运行时错误，ArkTS1.2在编译时会对联合类型的同名属性进行编译检查，要求同名属性具有相同的类型。

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
