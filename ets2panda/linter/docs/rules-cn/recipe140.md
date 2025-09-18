## 不支持Function.bind方法

**规则：** `arkts-no-func-bind`

**规则解释：**

ArkTS1.2不支持标准库函数Function.bind。

**变更原因：**
 
ArkTS1.2中的方法会自动捕获上下文中的`this`，因此无需使用`Function.bind`显式绑定`this`。

**适配建议：**

使用“=”（等号）将函数赋值给变量。

**示例：**

**ArkTS1.1**

```typescript
class MyClass {
  constructor(public name: string) {}

  greet() {
    console.log(`Hello, my name is ${this.name}`);
  }
}

const instance = new MyClass("Alice");
const boundGreet = instance.greet.bind(instance); // 违反规则，不允许使用 Function.bind
boundGreet();
```

**ArkTS1.2**

```typescript
class MyClass {
    name: string;
    constructor(name: string) { this.name = name; }
    greet() {
        console.log(`Hello, my name is ${this.name}`);
    }
}

const instance = new MyClass("Alice");
const boundGreet = () => instance.greet(); // 使用箭头函数
boundGreet(); // Hello, my name is Alice
```