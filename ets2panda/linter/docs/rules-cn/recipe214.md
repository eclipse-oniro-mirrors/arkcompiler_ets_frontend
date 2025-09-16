## 对象没有constructor

**规则：**`arkts-obj-no-constructor`

**级别：error**

ArkTS1.2支持天然共享的能力，运行时需要确定类型信息。实现上不再基于原型的语言，而是基于class的语言。

**ArkTS1.1**

```typescript
class A {}
let a = new A().constructor;   // ArkTS1.2上编译错误
```

**ArkTS1.2**

```typescript
class A {}
let a = new A();
let cls = Type.of(a); 
```
