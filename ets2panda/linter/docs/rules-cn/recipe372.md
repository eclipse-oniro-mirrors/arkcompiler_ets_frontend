## 智能类型差异

**规则：** `arkts-no-ts-like-smart-type`

**规则解释：**

在ArkTS-Sta中，线程共享对象在做[智能转换](#智能转换)时会表现的与ArkTS-Dyn不一致。

**变更原因：**

在ArkTS-Sta中，由于线程共享对象在多线程中使用，编译器在做类型推导和分析时需要考虑并发场景下变量类型/值的变化。在ArkTS-Dyn中支持全局变量、局部变量、函数参数和对象属性的智能转换，可以使用逻辑运算符、instanceof和typeof操作符进行类型收窄来触发智能转换。而在ArkTS-Sta中，只支持局部变量和函数参数的智能转换，只可以使用逻辑运算符和instanceof操作符进行类型收窄来触发智能转换。

**适配建议：**

要避免对全局变量和对象属性进行智能转换，避免使用typeof操作符进行类型收窄。

**示例：**

ArkTS-Dyn

```typescript
class A {
  a: number = 1.0;
}

class B {
  b: A | null = new A();
}

let globalA: A | null = new A();

function fun1() {
  if (globalA != null) {
    globalA.a = 2.0; // smart type，全局变量a的类型收窄为A
  }
}

function fun2() {
  let aObj: A | null = new A();
  if (aObj != null) {
    aObj.a = 2.0; // smart type，局部变量a的类型收窄为A
  }
}

function fun3(aObj: A | null) {
  if (aObj != null) {
    aObj.a = 2.0; // smart type，函数参数a的类型收窄为A
  }
}

function fun4() {
  let bObj = new B();
  if (bObj.b != null) {
    bObj.b.a = 2.0; // smart type，对象属性的类型收窄
  }
}

function fun5() {
  let aObj: A | null = new A();
  if (aObj instanceof A) {
    aObj.a = 2.0; // smart type，基于instanceof的局部变量a的类型收窄为A
  }
}

function fun6(aObj: string | null) {
  if (typeof aObj === 'string') {
    aObj = '2.0'; // smart type，基于typeof的函数参数a的类型收窄为A
  }
}
```

ArkTS-Sta

```typescript
class A {
  a: number = 1.0;
}

class B {
  b: A | null = new A();
}

let globalA: A | null = new A();

function fun1() {
  (globalA as A).a = 2.0; // 使用as断言明确类型，避免智能转换
}

function fun2() {
  let aObj: A | null = new A();
  if (aObj != null) {
    aObj.a = 2.0; // smart type，局部变量a的类型收窄为A
  }
}

function fun3(aObj: A | null) {
  if (aObj != null) {
    aObj.a = 2.0; // smart type，函数参数a的类型收窄为A
  }
}

function fun4() {
  let bObj = new B();
  (bObj.b as A).a = 2.0; // 使用as断言明确类型，避免智能转换
}

function fun5() {
  let aObj: A | null = new A();
  if (aObj instanceof A) {
    aObj.a = 2.0; // smart type，基于instanceof的局部变量a的类型收窄为A
  }
}

function fun6(aObj: string | null) {
  if (aObj instanceof string) {
    aObj = '2.0'; // 将typeof类型操作符改为instanceof操作符
  }
}
```