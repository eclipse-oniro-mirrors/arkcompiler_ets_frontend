## 对象没有constructor

**规则：** `arkts-obj-no-constructor`

**规则解释：**

ArkTS-Sta不支持通过constructor获取类型信息。

**变更原因：**

ArkTS-Sta支持天然共享的能力，运行时需要确定类型信息，实现上不再是基于原型的语言，而是基于class的语言。

**适配建议：**

使用反射接口获取类型。

**示例：**

ArkTS-Dyn

```typescript
class A {}
let a = new A().constructor;   // ArkTS-Sta上编译错误
```

ArkTS-Sta

```typescript
class A {}
let a = new A();
let cls = Type.of(a); 
```