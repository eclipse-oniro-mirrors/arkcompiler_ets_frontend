## 禁止对表达式使用extends或implements

**规则：** `arkts-no-extends-expression`

**规则解释：**

ArkTS1.2禁止对表达式使用extends或implements，如"extends a"，"extends getBase()"等。

**变更原因：**
 
ArkTS1.2中规范了类的继承规则：类不能作为对象使用，且在继承时无法继承表达式。

**适配建议：**

改为extends/implements类或接口。

**示例：**

**ArkTS1.1**

```typescript
class A {
  v: number = 0
}

let a = A;

class B extends a { // 违反规则
  u: number = 0
}

function getBase() {
  return class {
    w: number = 0;
  };
}

class B extends getBase() { // 违反规则
  u: number = 0;
}

interface I {
  w: number;
}

let i = I;

class B implements i { // 违反规则
  w: number = 0;
}

class A {
  v: number = 0;
}

class B extends new A() { // 违反规则
  u: number = 0;
}
```

**ArkTS1.2**

```typescript
class A {
  v: number = 0
}

class B extends A { // 直接继承类
  u: number = 0
}

class Base {
  w: number = 0;
}

class B extends Base { // 直接继承类
  u: number = 0;
}

interface I {
  w: number;
}

class B implements I { // 直接使用接口
  w: number = 0;
}

class A {
  v: number = 0;
}

class B extends A { // 直接继承类
  u: number = 0;
}
```