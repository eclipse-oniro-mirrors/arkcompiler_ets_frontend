## 对象字面量只包含属性不包含方法

**规则：**`arkts-obj-literal-props`

**级别：error**

ArkTS1.2中不支持在对象字面量中定义方法。因为静态语言中类的方法被所有实例所共享，无法通过对象字面量重新定义方法。

**ArkTS1.1**

```typescript
class A {
  foo: () => void = () => {}
}

let a: A = {
  foo() { // 违反规则
    console.log('hello')
  }
}

interface Person {
  sayHello: () => void;
}

let p: Person = {
  sayHello() {  // 违反规则，方法定义方式错误
    console.log('Hi');
  }
};

type Handler = {
  foo(): void; 
};

let handler: Handler = {
  foo() {  // 违反规则
    console.log("Executing handler");
  }
};
```

**ArkTS1.2**

```typescript
class A {
  foo : () => void = () => {}
}

let a: A = {
  foo: () => {
    console.log('hello')
  }
}

let p: Person = {
  sayHello: () => {  // 使用属性赋值方式
    console.log('Hi');
  }
};

type Handler = {
  foo: () => void;  
};

let handler: Handler = {
  foo: () => {  // 修正方法定义方式
    console.log("Executing handler");
  }
};
```
