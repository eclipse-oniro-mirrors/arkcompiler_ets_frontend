## 对象字面量只包含属性不包含方法

**规则：** `arkts-obj-literal-props`

**规则解释：**

ArkTS-Sta中不支持在对象字面量中定义方法。

**变更原因：**
 
静态语言中，类的方法被所有实例共享，无法通过对象字面量重新定义。

**适配建议：**

使用属性赋值方式。

**示例：**

ArkTS-Dyn

```typescript
class A {
  foo: () => void = () => {
  }
}

let a: A = {
  foo() {
    console.info('hello');
  }
}

interface Person {
  sayHello: () => void;
}

let p: Person = {
  sayHello() {
    console.info('Hi');
  }
};
```

ArkTS-Sta

```typescript
class A {
  foo: () => void = () => {
  }
}

let a: A = {
  foo: () => { // 使用属性赋值方式
    console.info('hello')
  }
}

interface Person {
  sayHello: () => void;
}

let p: Person = {
  sayHello: () => { // 使用属性赋值方式
    console.info('Hi');
  }
};
```