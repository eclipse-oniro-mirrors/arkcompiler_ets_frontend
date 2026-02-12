## 对象字面量生成类的实例

**规则：** `arkts-obj-literal-generate-class-instance`

**规则解释：**

在ArkTS-Sta中，对象字面量会生成类的实例。

**变更原因：**
 
ArkTS-Sta是静态类型语言，所有的对象都要有对应的类型，因此对象字面量也要生成对应类的实例。

**适配建议：**

ArkTS-Sta中用new实例化对象，不建议直接用对象字面量。

**示例：**

ArkTS-Dyn

```typescript
class A {
  v: number = 0;
}

let a: A = { v: 123 };
console.info((a instanceof A).toString()); // 输出：false
```

ArkTS-Sta

```typescript
class A {
  v: number = 0;
}

let a: A = { v: 123 };
console.info((a instanceof A).toString()); // 输出：true
```