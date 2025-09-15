## 通过new创建的Number/Boolean/String对象不再是object类型

**规则：** `arkts-primitive-type-normalization`

**规则解释：**

在ArkTS1.2中，在比较Number/Boolean/String对象时会自动拆箱，比较的是它们的值而不是对象。

而在ArkTS1.1中，比较的是对象而不是值。

**变更原因：**

在ArkTS1.2中，基本类型和其对应的包装类型在语言层面是相同的类型，这提高了语言的一致性和性能。

**适配建议：**

请注意，用new创建的Number/Boolean/String对象在操作时可能会表现出与ArkTS1.1不同的行为。

**示例：**

**ArkTS1.1**

```typescript
typeof new Number(1) // 结果："object"
new Number(1) == new Number(1);  // 结果：false
// 这里if语句判断的是Boolean对象是否为空，而不是拆箱后的结果，所以结果为true
if (new Boolean(false)) {}  // 结果：true
```

**ArkTS1.2**

```typescript
typeof new Number(1)// 结果："number"
new Number(1) == new Number(1);  // 结果：true
if (new Boolean(false)) {}      // 结果：false
```