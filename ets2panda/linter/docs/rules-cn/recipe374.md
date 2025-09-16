## new Number/Boolean/String不再是"object"类型

**规则：**`arkts-primitive-type-normalization`

**级别：error**

1. 在ArkTS1.2中primitive type和boxed type是相同的类型，这样可以提高语言一致性和性能。
   比较Number/Boolean/String时比较的是值而不是对象。

2. 在ArkTS1.1上，boxed类型通过new创建。在获取其类型、比较boxed类型对象时会产生意外行为，这是因为对象比较时是通过引用进行比较，而不是值。通常直接使用primitive 
   type性能更高效，内存占用更少（相比之下对象会占用更多内存）。

**ArkTS1.1**

```typescript
typeof new Number(1) // 结果: "object"
new Number(1) == new Number(1);  //结果: false
if (new Boolean(false)) {} // 在if语句中new Boolean(false)为true
```

**ArkTS1.2**

```typescript
typeof new Number(1)// 结果: "number"
new Number(1) == new Number(1);  //结果: true
if (new Boolean(false)) {}      // 在if语句中new Boolean(false)为false
```
