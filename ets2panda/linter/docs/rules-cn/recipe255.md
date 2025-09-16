## 禁止extends/implements表达式

**规则：**`arkts-no-extends-expression`

**级别：error**

ArkTS1.2中规范了类的继承，类不能作为对象来继承一个表达式。

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
