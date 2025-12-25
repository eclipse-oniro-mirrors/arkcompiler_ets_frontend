## 类实现接口时，不能用类方法替代对应interface属性

**规则：** `arkts-no-method-overriding-field`

**规则解释：**

在ArkTS-Sta中，类在实现接口时，lambda属性和方法不能混用。即不能用方法实现属性，也不能用属性实现方法。

**变更原因：**
 
在ArkTS-Dyn中，方法类型与函数属性类型兼容，类实现接口时可以混用。

在ArkTS-Sta中，属性和方法有本质区别，函数属性类型与方法类型不再兼容，因此不支持这种写法。

**适配建议：**

实现接口时，不要混用lambda属性和方法，确保实现与声明保持一致。

**示例：**

ArkTS-Dyn

```typescript
interface Person {
  cb1: () => void;
  cb2(): void;
}
class Student implements Person {
  cb1() { }          // 用方法实现lambda属性，ArkTS-Sta编译错误
  cb2:() => void = () => {}   // 用lambda属性实现方法，ArkTS-Sta编译错误
}
```

ArkTS-Sta

```typescript
interface Person {
  cb1: () => void;
  cb2();
}
class Student implements Person {
  cb1: () => void = () => { }  // 修改为lambda属性，与声明保持一致
  cb2() { }     // 修改为方法，与声明保持一致
}
```