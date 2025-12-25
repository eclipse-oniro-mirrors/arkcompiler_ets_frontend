## 通过new创建的Number/Boolean/String对象不再是object类型

**规则：** `arkts-primitive-type-normalization`

**规则解释：**

在ArkTS-Sta中，用new创建的Number、Boolean和String对象不再是object类型，进行判断、比较等操作时，与ArkTS-Dyn的表现不同。请开发者自行决定是否需要修改代码。

**变更原因：**

在ArkTS-Sta中，基本类型和其对应的包装类型在语言层面是相同的类型，这提高了语言的一致性和性能。

**适配建议：**

避免使用new创建的Number、Boolean和String对象进行比较和判断操作，建议使用基础类型。

**示例：**

**包装类型**

```typescript
// ArkTS-Dyn结果："object"
// ArkTS-Sta结果："number"
typeof new Number(1);

// ArkTS-Dyn结果：false
// ArkTS-Sta结果：true
new Number(1) == new Number(1); 

// ArkTS-Dyn结果：true（这里if语句判断的是Boolean对象是否为空，而不是拆箱后的结果，所以结果为true）
// ArkTS-Sta结果：false
if (new Boolean(false)) {}
```

**基础类型**

```typescript
// ArkTS-Dyn&ArkTS-Sta结果均为："number"
typeof 1;

// ArkTS-Dyn&ArkTS-Sta结果均为：true
1 == 1;

// ArkTS-Dyn&ArkTS-Sta结果均为：false
if (false) {}
```