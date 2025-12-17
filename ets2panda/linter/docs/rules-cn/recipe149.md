## 不支持将类作为对象

**规则：** `arkts-no-classes-as-obj`

**规则解释：**

在ArkTS-Sta中，不支持将class用作对象。

**变更原因：**
 
在ArkTS-Sta中，class声明的是一个新的类型，而不是一个值。因此，不支持将class用作对象，例如赋值给变量。

**适配建议：**

通过反射来实现。

**示例：**

ArkTS-Dyn

```typescript
class MyClass {
  constructor() {
  }

  static test: string = "test";
}

let obj = MyClass; // obj是类型，并非对象

console.info(MyClass.test); // 输出：test
console.info((MyClass as object)['test']); // 输出：test
```

ArkTS-Sta

```typescript
class MyClass {
  constructor() {
  }

  static test: string = "test";
}

// 获取ClassType
let classType: ClassType | undefined = Type.from<MyClass>() as ClassType;

console.info(MyClass.test); // 输出：test
// console.info((MyClass as object)['test']) // 违反规则
```