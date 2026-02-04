## 实例方法赋值给对象时会自动绑定this

**规则：** `arkts-instance-method-bind-this`

**规则解释：**

在ArkTS-Sta中，实例方法被赋值给对象时会自动绑定上下文中的`this`。

**变更原因：**

在ArkTS-Dyn中，实例方法的`this`指向取决于调用方式。当实例方法被赋值给对象时，`this`将不再指向实例，而是指向`undefined`。可以使用`bind`方法显式绑定，确保`this`正确指向实例。

在ArkTS-Sta中，实例方法直接赋值时会自动绑定`this`，确保方法调用时`this`始终指向原始实例，无需额外绑定。

**适配建议：**

开发者应移除不必要的bind绑定操作。

**示例：**

```typescript
// 类型定义
class A {
  n: string = 'a';
  foo() { console.info (this.n); }
}
```

ArkTS-Dyn

```typescript
let a = new A();
let foo = a.foo.bind(a) as Function;  // 显式绑定this
foo(); // 输出：'a'
```

ArkTS-Sta

```typescript
let a = new A();
const foo = a.foo;   // 直接赋值，自动绑定this
foo();   // 输出：'a'
```