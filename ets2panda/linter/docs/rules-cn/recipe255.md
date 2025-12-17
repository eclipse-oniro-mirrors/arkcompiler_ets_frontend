## 禁止对表达式使用extends或implements

**规则：** `arkts-no-extends-expression`

**规则解释：**

ArkTS-Sta禁止对表达式使用extends或implements，如"extends a"，"extends getBase()"等。

**变更原因：**
 
ArkTS-Sta中规范了类的继承规则：类不能作为对象使用，且在继承时无法继承表达式。

**适配建议：**

改为extends/implements类或接口。

**示例：**

ArkTS-Dyn

```typescript
class A {
  v: number = 0;
}

let a = A;

class B extends a {
  u: number = 0;
}

function getBase() {
  class C {
    v: number = 0;
  }

  return C;
}

class D extends getBase() {
  u: number = 0;
}
```

ArkTS-Sta

```typescript
class A {
  v: number = 0;
}

class B extends A { // 直接继承类
  u: number = 0;
}

class C {
  v: number = 0;
}

class D extends C { // 直接继承类
  u: number = 0;
}
```