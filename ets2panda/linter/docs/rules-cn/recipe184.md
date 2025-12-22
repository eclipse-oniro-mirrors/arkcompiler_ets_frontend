## 不支持可选方法

**规则：** `arkts-optional-methods`

**规则解释：**

ArkTS1.2不支持类中的可选方法。

**变更原因：**

ArkTS1.2中，类的方法由所有实例共享。增加可选方法支持会增加开发者判断空值的成本，影响性能。

**适配建议：**

用可选属性代替可选方法。

**示例：**

**ArkTS1.1**

```typescript
interface InterfaceA {
  aboutToDisappear?(): void
}
class ClassA {
  aboutToDisappear?(): void {}
}
```

**ArkTS1.2**

```typescript
interface InterfaceA {
  aboutToDisappear?: () => void
}
class ClassA {
  aboutToDisappear?: () => void = () => {}
}
```