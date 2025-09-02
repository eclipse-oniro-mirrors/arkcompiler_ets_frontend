## 不支持Funcion.bind方法

**规则：**`arkts-no-func-bind`

**级别：error**

ArkTS不允许使用标准库函数Function.bind。标准库使用这些函数来显式设置被调用函数的this参数。

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
  constructor(public name: string) {}

  greet() {
    console.log(`Hello, my name is ${this.name}`);
  }
}

const instance = new MyClass("Alice");
const boundGreet = () => instance.greet(); // 使用箭头函数
boundGreet(); // Hello, my name is Alice
```
