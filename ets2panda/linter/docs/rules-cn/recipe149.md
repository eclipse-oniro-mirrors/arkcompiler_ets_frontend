## 不支持将类作为对象

**规则：** `arkts-no-classes-as-obj`

**规则解释：**

在ArkTS1.2中，不支持将class用作对象。

**变更原因：**
 
在ArkTS1.2中，class声明的是一个新的类型，而不是一个值。因此，不支持将class用作对象，例如赋值给变量。

**适配建议：**

通过反射来实现。

**示例：**

**ArkTS1.1**

```typescript
class MyClass {
  constructor(public name: string) {}
}

let obj = MyClass; // 违反规则
```

**ArkTS1.2**

```typescript
class MyClass {
  constructor(name: string) {}
}

// 需要通过反射来实现
let className = "path.to.MyClass";
let linker = Class.ofCaller()!.getLinker();
let classType: ClassType | undefined = linker.getType(className) as ClassType;
```