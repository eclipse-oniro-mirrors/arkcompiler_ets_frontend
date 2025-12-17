## 不支持Function.bind方法

**规则：** `arkts-no-func-bind`

**规则解释：**

ArkTS-Sta不支持标准库函数Function.bind。

**变更原因：**
 
ArkTS-Sta中的方法会自动捕获上下文中的`this`，因此无需使用`Function.bind`显式绑定`this`。

**适配建议：**

使用“=”（等号）将函数赋值给变量。

**示例：**

ArkTS-Dyn

```typescript
class MyClass {
  name: string;

  constructor(name: string) {
    this.name = name;
  }

  greet() {
    console.info(`Hello, my name is ${this.name}`);
  }
}

const instance = new MyClass("Alice");
const boundGreet: Function = instance.greet.bind(instance);
boundGreet();
```

ArkTS-Sta

```typescript
class MyClass {
  name: string;

  constructor(name: string) {
    this.name = name;
  }

  greet() {
    console.info(`Hello, my name is ${this.name}`);
  }
}

const instance = new MyClass("Alice");
const boundGreet = instance.greet;
boundGreet(); // Hello, my name is Alice
```