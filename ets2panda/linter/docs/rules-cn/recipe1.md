## enum/class/interface的属性/方法名称须使用合法标识符

**规则：** `arkts-identifiers-as-prop-names`

**规则解释：**

ArkTS1.2不支持将字符串作为class、interface、enum等属性或元素的名称，仅支持合法标识符作为属性。

**变更原因：**
 
在ArkTS1.2中，为了增强对边界场景的约束，对象的属性名不能使用数字或字符串。

**适配建议：**

将属性名从字符串改为标识符。

**示例：**

**ArkTS1.1**

```typescript
enum A{
 'red' = '1'
}
```

**ArkTS1.2**

```typescript
enum A{
  red = '1'
}
```